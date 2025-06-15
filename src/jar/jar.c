#include <errno.h>

#include "jar/jar.h"
#include "libs/zip/zip.h"
#include "decompiler/klass.h"
#include "jvm/jvm_decompile.h"
#include "decompiler/expression_writter.h"
#include "common/file_tools.h"
#include "libs/threadpool/threadpool.h"
#include "output_tools.h"

void jar_status(jd_jar *jar)
{
    pthread_mutex_lock(jar->threadpool->lock);
    jar->done++;
    fflush(stdout);
    backspace(25);
    printf("Progress : %d (%d)", jar->done, jar->added);
    fflush(stdout);
    pthread_mutex_unlock(jar->threadpool->lock);
}

void jar_main_thread_status(jd_jar *jar)
{
    fflush(stdout);
    backspace(25);
    printf("Progress : %d", jar->done);
    fflush(stdout);
}

static string jar_cname_from_path(jd_jar *jar, string path)
{
    const char *last_slash = strrchr(path, '/');
    const char *start = (last_slash == NULL) ? path : last_slash + 1;

    const char *dot_class = strstr(start, ".class");
    size_t len = (dot_class != NULL) ? (dot_class - start) : strlen(start);

    string output = x_alloc_in(jar->pool, len+1);
    strncpy(output, start, len);
    output[len] = '\0';

    // 4. 处理 Dalvik 格式的修饰符（'L' 开头和 ';' 结尾）
    if (output[0] == 'L') {
        memmove(output, output + 1, len); // 移除开头的 'L'
        len--;
        output[len] = '\0';
    }
    if (len > 0 && output[len - 1] == ';') {
        output[len - 1] = '\0'; // 移除结尾的 ';'
    }
    return output;
}

static string jar_entry_parent_path(jd_jar *jar, jd_jar_entry *entry)
{
    string full = entry->path;
    const char *last_dollar = strrchr(full, '$');
    const char *last_dot = strrchr(full, '.');

    if (last_dot == NULL || last_dollar == NULL)
        return NULL;


    size_t last_dollar_index = last_dollar - full;
    size_t last_dot_index = last_dot - full;

    if (full[last_dollar_index-1] == '$')
        last_dollar_index --;

    string parent_full = x_alloc_in(jar->pool, strlen(full));
    strncpy(parent_full, full, last_dollar_index);
    int i = 0;
    for (; i < 6; ++i) {
        parent_full[last_dollar_index+i] = full[i+last_dot_index];
    }
    parent_full[last_dollar_index+i] = '\0';
    return parent_full;
}

static void prepare_jar_zip(jd_jar *jar)
{
    struct zip_t *zip = zip_open(jar->path, 0, 'r');
    jar->zip = zip;
    jar->entries_size = zip_entries_total(zip);
    jar->class_entries = linit_object_with_pool(jar->pool);

    for (int i = 0; i < jar->entries_size; ++i) {
        zip_entry_openbyindex(zip, i);
        string path_in_jar = (string)zip_entry_name(zip);
        string full_path = str_create_in(jar->pool, "%s", path_in_jar);
        if (!str_end_with(path_in_jar, ".class")) {
            zip_entry_close(zip); // only deal with .class files
            continue;
        }
        string cname = jar_cname_from_path(jar, path_in_jar);
        DEBUG_PRINT("[process file at]: %s %s\n", path_in_jar, cname);

        jd_jar_entry *entry = make_obj_in(jd_jar_entry, jar->pool);
        entry->path = full_path;
        entry->index = i;
        entry->cname = cname;
        entry->is_inner = is_inner_class(cname);
        entry->is_anoymous = is_anonymous_class(cname);
        entry->inner_classes = linit_object_with_pool(jar->pool);
        entry->anoymous_classes = linit_object_with_pool(jar->pool);
        entry->jar = jar;

        char *buf = NULL;
        size_t buf_size;
        buf_size = zip_entry_size(zip);
        buf = x_alloc_in(jar->pool, buf_size * sizeof(unsigned char));
        zip_entry_noallocread(zip, (void *)buf, buf_size);
        zip_entry_close(zip);

        entry->buf = buf;
        entry->buf_size = buf_size;
        ladd_obj(jar->class_entries, entry);

        hset_s2o(jar->name_to_index_map, full_path, entry);
        hset_i2o(jar->index_to_name_map, i, entry);
    }
}

static void jar_inner_and_anoymous_class(jd_jar *jar)
{
    for (int i = 0; i < jar->class_entries->size; ++i) {
        jd_jar_entry *entry = lget_obj(jar->class_entries, i);
        if (!entry->is_inner && !entry->is_anoymous)
            continue;
        string parent_path = jar_entry_parent_path(jar, entry);
        if (parent_path == NULL) {
            DEBUG_PRINT("[parent_path] %s parent path is NULL\n", entry->path);
            continue;
        }
        jd_jar_entry *parent = hget_s2o(jar->name_to_index_map, parent_path);
        if (parent == NULL) {
            DEBUG_PRINT("[parent] %s parent entry is NULL\n", entry->path);
            continue;
        }
        entry->parent = parent;
        if (entry->is_anoymous)
            ladd_obj(parent->anoymous_classes, entry);
        else if (entry->is_inner)
            ladd_obj(parent->inner_classes, entry);

    }
}

static void jar_entry_source_file(jclass_file *jc, string dir, string name)
{
    struct stat sb;
    string full_dir = str_create("%s/%s", dir, dirname(name));
    if (stat(full_dir, &sb) == -1)
        make_dir(full_dir);

    jcp_info *info = pool_item(jc, jc->this_class);
    string full = get_class_name(jc, info);
    string class_name = class_simple_name(full);
    string path = str_create("%s/%s.java", full_dir, class_name);
    FILE *stream = fopen(path, "w");
    if (stream == NULL)
        printf("[error]: path: %s, error: %s\n", path, strerror(errno));
    jc->jfile->source = stream;
}

static jd_jar* jar_obj_create(string path, string save_path, int thread_cnt)
{
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "[errorn]: %s not exist\n", path);
        exit(0);
    }
    mem_pool *pool = mem_create_pool();
    jd_jar *jar = make_obj_in(jd_jar, pool);
    jar->path = path;
    jar->save = save_path;
    jar->pool = pool;
    jar->inner_class_map = hashmap_init_in(jar->pool, s2o_cmp, 0);
    jar->anoymous_class_map = hashmap_init_in(jar->pool, s2o_cmp, 0);
    jar->name_to_index_map = hashmap_init_in(jar->pool, s2o_cmp, 0);
    jar->index_to_name_map = hashmap_init_in(jar->pool, i2obj_cmp, 0);

    mkdir_p(jar->save);

    prepare_jar_zip(jar);

    if (thread_cnt > 1) {
        jar->threadpool = threadpool_create_in(jar->pool, thread_cnt, 0);
    }

    jar_inner_and_anoymous_class(jar);

    return jar;
}

static void jar_obj_release(jd_jar *jar)
{
    if (jar->threadpool)
        threadpool_destroy(jar->threadpool, 1);
    zip_close(jar->zip);
    mem_pool_free(jar->pool);
}

void jar_entry_thread_task(jd_jar_entry *entry)
{
    thread_local_data *tls = get_thread_local_data();
    tls->pool = mem_create_pool();

    jsource_file *jf = jar_entry_analyse(entry->jar, entry, NULL);
    if (jf->parent == NULL) {
        writter_for_class(jf, NULL);
        fclose(jf->source);
    }
    mem_pool_free(tls->pool);

    jar_status(entry->jar);
}

static void jar_threadpool_start(jd_jar *jar)
{
    for (int i = 0; i < jar->class_entries->size; ++i) {
        jd_jar_entry *entry = lget_obj(jar->class_entries, i);
        if (entry->is_inner || entry->is_anoymous)
            continue;
        threadpool_add(jar->threadpool, &jar_entry_thread_task, entry, 0);
        jar->added++;
    }
}

jsource_file* jar_entry_anonymous_analyse(jd_jar *jar,
                                        jd_jar_entry *entry,
                                        jsource_file *parent)
{

//    if (entry->jf == NULL) {
        jclass_file *jc = parse_class_content_from_jar_entry(entry);
        jsource_file *jf = jc->jfile;
        jf->jar = jar;
        jf->jar_entry = entry;
        entry->jf = jf;
        entry->parsed = true;
        jf->parent = parent;
        jf->source = parent->source;

        jvm_analyse_class_file_inside(jf);

        tire_merge(jf->imports, parent->imports);
        return jc->jfile;
//    }
//    else {
//        jsource_file *_jf = entry->jf;
//        _jf->parent = parent;
//        _jf->source = parent->source;
//        return _jf;
//    }
}

jsource_file* jar_entry_analyse(jd_jar *jar,
                                jd_jar_entry *entry,
                                jsource_file *parent)
{
    jclass_file *jc = parse_class_content_from_jar_entry(entry);
    jsource_file *jf = jc->jfile;
    jf->jar = jar;
    jf->jar_entry = entry;
    entry->jf = jf;
    entry->parsed = true;

    if (parent == NULL)
        jar_entry_source_file(jc, jar->save, entry->path);
    else {
        jf->parent = parent;
        jf->source = parent->source;
    }

    jvm_analyse_class_file_inside(jf);
    if (parent != NULL) {
        tire_merge(jf->imports, parent->imports);
        jd_node *inner_block = class_body_block(jf);
        jd_node *parent_body = class_body_block(parent);
        inner_block->parent = parent_body;
        ladd_obj(parent_body->children, inner_block);
    }

    for (int i = 0; i < entry->inner_classes->size; ++i) {
        jd_jar_entry *inner_entry = lget_obj(entry->inner_classes, i);
        jsource_file *inner_jfile = jar_entry_analyse(jar, inner_entry, jf);
        jclass_file *inner_jclass = inner_jfile->jclass;
        inner_jclass->jfile->jar = jar;

        tire_merge(jf->imports, inner_jfile->imports);
        jd_node *inner_block = class_body_block(inner_jfile);
        jd_node *parent_body = class_body_block(jf);
        inner_block->parent = parent_body;
        ladd_obj(parent_body->children, inner_block);
    }
    return jc->jfile;
}

static void jar_main_thread(jd_jar *jar)
{
    for (int i = 0; i < jar->class_entries->size; ++i) {
        jd_jar_entry *entry = lget_obj(jar->class_entries, i);

        /*
            if (!STR_EQL(entry->cname, "Camera2CameraControlImpl")) {
                continue;
            }
        */

        if (entry->is_inner || entry->is_anoymous)
            continue;

        mem_init_pool();

        jsource_file *jf = jar_entry_analyse(jar, entry, NULL);
        if (jf->parent == NULL) {
            writter_for_class(jf, NULL);
            fclose(jf->source);
        }
        jar->added ++;
        jar->done ++;
        jar_main_thread_status(jar);
        mem_free_pool();
    }
}

void jar_file_analyse(string path, string save_path, int thread_cnt) {
    jd_jar *jar = jar_obj_create(path, save_path, thread_cnt);

    if (thread_cnt > 1) {
        jar_threadpool_start(jar);
    }
    else {
        jar_main_thread(jar);
    }

    jar_obj_release(jar);
}

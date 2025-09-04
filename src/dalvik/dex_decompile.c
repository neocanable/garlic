#include <errno.h>
#include "dalvik/dex_decompile.h"
#include "dalvik/dex_structure.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_class.h"
#include "dalvik/dex_exception.h"
#include "dalvik/dex_optimizer.h"
#include "dalvik/dex_simulator.h"
#include "dalvik/dex_meta_helper.h"

#include "decompiler/descriptor.h"
#include "decompiler/method.h"
#include "decompiler/expression_writter.h"
#include "parser/dex/metadata.h"
#include "jvm/jvm_ins.h"
#include "dex_pre_optimizer.h"
#include "decompiler/control_flow.h"
#include "jar/jar.h"
#include "file_tools.h"
#include "dex_annotation.h"
#include "dex_dump.h"
#include "output_tools.h"
#include "dex_smali.h"

void dex_status(jd_dex *dex)
{
    pthread_mutex_lock(dex->threadpool->lock);
    dex->done++;
    fflush(stdout);
    backspace(25);
    printf("Progress : %d (%d)", dex->done, dex->added);
    fflush(stdout);
    pthread_mutex_unlock(dex->threadpool->lock);
}

void dex_main_thread_status(jd_dex *dex)
{
    fflush(stdout);
    backspace(25);
    printf("Progress : %d", dex->done);
    fflush(stdout);
}

void dex_init_ins_fn(jd_dex *dex)
{
    // setup instruction's interface
    jd_ins_fn *fn = make_obj(jd_ins_fn);
    fn->is_compare = dex_ins_is_compare;
    fn->is_return = dex_ins_is_return_op;
    fn->is_void_return = dex_ins_is_return_void;
    fn->is_switch = dex_ins_is_switch;
    fn->is_goto = dex_ins_is_goto_jump;
    fn->is_unconditional_jump = dex_ins_is_unconditional_jump;
    fn->is_conditional_jump = dex_ins_is_conditional_jump;
    fn->is_goto_back = dex_ins_is_goto_back;
    fn->is_jump_dst = dex_ins_is_jump_destination;
    fn->is_block_start = dex_ins_is_block_start;
    fn->is_block_end = dex_ins_is_block_end;
    fn->is_store = dex_ins_is_store;
    fn->is_load = dex_ins_is_load;
    fn->is_if = dex_ins_is_if;
    fn->is_branch = dex_ins_is_branch;
    fn->is_athrow = dex_ins_is_throw;
    fn->is_invoke_static = dex_ins_is_invokestatic;
    fn->is_invoke_virtual = dex_ins_is_invokevirtual;
    fn->is_invoke_interface = dex_ins_is_invokeinterface;
    fn->is_invoke_dynamic = dex_ins_is_invokedymamic;
    fn->is_invoke_special = dex_ins_is_invokespecial;

    dex->ins_fn = fn;
}

void dex_init_method_fn(jd_dex *dex)
{
    jd_method_fn *fn = make_obj(jd_method_fn);
    fn->access_flags_fn = dex_method_access_flags;
    fn->param_val_fn = dex_method_parameter_val;
    fn->param_annotation_fn = dex_method_parameter_annotation;
    fn->is_native = dex_method_is_native;
    fn->is_init = dex_method_is_init;
    fn->is_clinit = dex_method_is_clinit;
    fn->is_member = dex_method_is_member;
    fn->is_synthetic = dex_method_is_synthetic;
    fn->is_varargs = dex_method_is_varargs;
    fn->is_abstract = dex_method_is_abstract;
    dex->method_fn = fn;
}

jd_method *dex_method(jsource_file *jf, encoded_method *em)
{
    jd_method *m = make_obj(jd_method);

    dex_method_init(jf, m, em);

    if (method_is_empty(m))
        return m;

    dex_method_exception_edge(m);

    dex_simulator(m);

    cfg_remove_exception_block(m);

    pre_optimize_dex_method(m);

    optimize_dex_method(m);

    return m;
}

static void dex_methods(jsource_file *jf)
{
    dex_class_def *cf = jf->jclass;
    dex_class_data_item *data = cf->class_data;
    jf->methods = linit_object();

    for (int i = 0; i < data->direct_methods_size; ++i) {
        encoded_method *em = &data->direct_methods[i];
        jd_method *m = dex_method(jf, em);
        ladd_obj(jf->methods, m);
    }

    for (int i = 0; i < data->virtual_methods_size; ++i) {
        encoded_method *em = &data->virtual_methods[i];
        jd_method *m = dex_method(jf, em);
        ladd_obj(jf->methods, m);
    }
}

static void dex_class_source_save_dir(jd_dex *dex, jsource_file *jf)
{
    jd_meta_dex *meta = dex->meta;
    if (meta->source_dir == NULL || jf->is_anonymous || jf->is_inner)
        return;
    string full_dir = str_create("%s/%s", meta->source_dir, jf->pname);
    mkdir_p(full_dir);

    string path = str_create("%s/%s.java", full_dir, jf->sname);
    FILE *stream = fopen(path, "w");
    if (stream == NULL) {
        fprintf(stdout, "[error]: open file %s failed: %d\n", path, errno);
        return;
    }
    jf->source = stream;
}

static FILE* dex_class_smali_save_dir(jd_dex *dex, dex_class_def *cf)
{
    jd_meta_dex *meta = dex->meta;
    string desc = dex_str_of_type_id(dex->meta, cf->class_idx);
    string fname = class_full_name(desc);
    string sname = class_simple_name(fname);
    string pname = class_package_name_of(fname);

    string full_dir = str_create("%s/%s", meta->source_dir, pname);
    mkdir_p(full_dir);

    string path = str_create("%s/%s.smali", full_dir, sname);
    FILE *stream = fopen(path, "w");
    if (stream == NULL) {
        fprintf(stdout, "[error]: open file %s failed: %d\n", path, errno);
        return NULL;
    }
    return stream;
}

static void dex_inner_class_list(jsource_file *jf)
{
    dex_class_def *cf = jf->jclass;
    jd_dex *dex = jf->meta;
    for (int i = 0; i < cf->inner_classes->size; ++i) {
        dex_class_def *inner_cf = lget_obj(cf->inner_classes, i);
        dex_inner_class(dex, jf, inner_cf);
    }
}

jsource_file* dex_class_inside(jd_dex *dex,
                               dex_class_def *cf,
                               jsource_file *parent)
{
    dex_class_data_item *class_data = cf->class_data;
    jsource_file *jf = make_obj(jsource_file);
    string desc = dex_str_of_type_id(dex->meta, cf->class_idx);
    jf->fname = class_full_name(desc);
    jf->sname = class_simple_name(jf->fname);
    jf->pname = class_package_name(jf);
    jf->imports = trie_create_node("");
    jf->meta = dex;
    jf->jclass = cf;
    string super_cname = dex_str_of_type_id(dex->meta, cf->superclass_idx);
    jf->super_cname = class_simple_name(super_cname);
    jf->interfaces = linit_object();

    if (cf->interfaces != NULL) {
        for (int i = 0; i < cf->interfaces->size; ++i) {
            dex_type_item *item = &cf->interfaces->list[i];
            string name = dex_str_of_type_id(dex->meta, item->type_idx);
            ladd_obj(jf->interfaces, class_simple_name(name));
        }
    }

    jf->type = JD_TYPE_DALVIK;
    jf->access_flags_fn = dex_class_access_flag;
    jf->is_anonymous = dex_class_is_anonymous_class(dex->meta, cf);
    jf->is_inner = dex_class_is_inner_class(dex->meta, cf);
    jf->parent = parent;
    if (jf->parent != NULL) {
        jf->source = jf->parent->source;
    }
    else {
        dex_class_source_save_dir(dex, jf);
    }

    if (class_data == NULL) {
        jf->fields_count = 0;
        jf->fields = NULL;
        jf->methods = linit_object();
        jf->methods_count = 0;
        class_create_definations(jf);
        dex_class_annotations(jf);
        class_create_blocks(jf);
        return jf;
    }

    dex_fields(jf);

    dex_methods(jf);

    dex_class_annotations(jf);

    dex_class_import(jf);

    class_create_blocks(jf);

    class_create_definations(jf);

    dex_inner_class_list(jf);

    return jf;
}

jsource_file* dex_inner_class(jd_dex *dex,
                              jsource_file *parent,
                              dex_class_def *cf)
{
    jsource_file *inner = dex_class_inside(dex, cf, parent);
    if (!inner->is_anonymous) {
        tire_merge(parent->imports, inner->imports);
        jd_node *inner_block = class_body_block(inner);
        jd_node *parent_body = class_body_block(parent);
        inner_block->parent = parent_body;
        ladd_obj(parent_body->children, inner_block);
    }
    return inner;
}

void dex_decompile_class(jd_dex *dex, dex_class_def *cf)
{
    mem_init_pool();
    if (dex_class_is_inner_class(dex->meta, cf) ||
        dex_class_is_anonymous_class(dex->meta, cf))
        return;

    jsource_file *jf = dex_class_inside(dex, cf, NULL);
    if (jf->parent == NULL) {
        writter_for_class(jf, NULL);
        fclose(jf->source);
    }
    mem_free_pool();
}

void dex_smali_class(jd_dex *dex, dex_class_def *cf)
{
    mem_init_pool();
    FILE *stream = dex_class_smali_save_dir(dex, cf);
    dex_class_def_to_smali(dex->meta, cf, stream);
    mem_free_pool();
}

void dex_to_source(string dex_path, string save_dir)
{
    jd_meta_dex *meta = parse_dex_file(dex_path);
    meta->source_dir = save_dir;
    mkdir_p(meta->source_dir);
    dex_analyse(meta);
}

static void dex_inner_and_anonymous_class(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (cf->is_anonymous) {
            string cname = dex_str_of_type_id(meta, cf->class_idx);
            char *last_dollar = strrchr(cname, '$');
            int index = last_dollar - cname;
            string pname = x_alloc(index + 2);
            memcpy(pname, cname, index);
            pname[index] = ';';
            pname[index + 1] = '\0';
            dex_class_def *parent_cf = hget_s2o(meta->class_name_map, pname);
            if (parent_cf != NULL) {
                ladd_obj(parent_cf->anonymous_classes, cf);
            }

            DEBUG_PRINT("[anonymous] class: %s parent: %s %p\n",
                        cname, pname, parent_cf);
        }
        else if (cf->is_inner) {
            string cname = dex_str_of_type_id(meta, cf->class_idx);
            char *last_dollar = strrchr(cname, '$');
            int index = last_dollar - cname;
            string pname = x_alloc(index + 2);
            memcpy(pname, cname, index);
            pname[index] = ';';
            pname[index + 1] = '\0';
            dex_class_def *parent_cf = hget_s2o(meta->class_name_map, pname);
            if (parent_cf != NULL)
                ladd_obj(parent_cf->inner_classes, cf);

            DEBUG_PRINT("[inner] class: %s parent: %s %p\n",
                        cname, pname, parent_cf);
        }
    }
}

jd_dex* dex_init(jd_meta_dex *meta, int thread_num)
{
    jd_dex *dex = make_obj(jd_dex);
    dex->meta = meta;
    dex->classes = linit_object();
    dex_init_ins_fn(dex);
    dex_init_method_fn(dex);
    dex_inner_and_anonymous_class(dex);

    if (thread_num > 1) {
        dex->threadpool = threadpool_create_in(meta->pool, thread_num, 0);
    }

    return dex;
}

jd_dex* dex_init_without_thread(jd_meta_dex *meta)
{
    jd_dex *dex = make_obj(jd_dex);
    dex->meta = meta;
    dex->classes = linit_object();
    dex_init_ins_fn(dex);
    dex_init_method_fn(dex);
    dex_inner_and_anonymous_class(dex);
    return dex;
}

void dex_decompile_thread_task(jd_dex_task *task)
{
    thread_local_data *tls = get_thread_local_data();
    tls->pool = mem_create_pool();

    jd_dex *dex = task->dex;
    dex_class_def *cf = task->cf;

    jsource_file *jf = dex_class_inside(dex, cf, NULL);
    if (jf->parent == NULL) {
        writter_for_class(jf, NULL);
        fclose(jf->source);
    }
    mem_pool_free(tls->pool);

    dex_status(dex);
}

void dex_smali_thread_task(jd_dex_task *task)
{
    thread_local_data *tls = get_thread_local_data();
    tls->pool = mem_create_pool();

    jd_dex *dex = task->dex;
    dex_class_def *cf = task->cf;

    FILE *stream = dex_class_smali_save_dir(dex, cf);

    dex_class_def_to_smali(dex->meta, cf, stream);

    if (stream != NULL)
        fclose(stream);

    mem_pool_free(tls->pool);

    dex_status(dex);
}

void dex_decompile_threadpool_start(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (dex_class_is_inner_class(dex->meta, cf) ||
            dex_class_is_anonymous_class(dex->meta, cf))
            continue;

        jd_dex_task *t = make_obj(jd_dex_task);
        t->dex = dex;
        t->cf = cf;
        threadpool_add(dex->threadpool, &dex_decompile_thread_task, t, 0);
        dex->added++;
    }
}

void dex_decompile_main_thread_start(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        mem_init_pool();
        dex_class_def *cf = &meta->class_defs[i];
        if (dex_class_is_inner_class(dex->meta, cf) ||
            dex_class_is_anonymous_class(dex->meta, cf))
            continue;

        jsource_file *jf = dex_class_inside(dex, cf, NULL);
        if (jf->parent == NULL) {
            writter_for_class(jf, NULL);
            fclose(jf->source);
        }
        mem_free_pool();
        dex->done ++;
        dex_main_thread_status(dex);
    }
}

void dex_smali_threadpool_start(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        jd_dex_task *t = make_obj(jd_dex_task);
        t->dex = dex;
        t->cf = cf;
        t->type = JD_DEX_TASK_SMALI;
        threadpool_add(dex->threadpool, &dex_smali_thread_task, t, 0);
        dex->added++;
    }
}

void dex_smali_main_thread_start(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        mem_init_pool();
        dex_class_def *cf = &meta->class_defs[i];

        FILE *stream = dex_class_smali_save_dir(dex, cf);

        dex_class_def_to_smali(dex->meta, cf, stream);

        mem_free_pool();
        dex->done ++;
        dex_main_thread_status(dex);
    }
}

void dex_release(jd_dex *dex)
{
    if (dex->threadpool) {
        threadpool_destroy(dex->threadpool, 1);
        mem_pool_free(dex->meta->pool);
        mem_free_pool();
    }
    else {
        mem_pool_free(dex->meta->pool);
    }

}

void dex_file_analyse(string path, string save_dir, int thread_num, jd_dex_task_type type)
{
    mem_init_pool();
    jd_meta_dex *meta = parse_dex_file(path);
    meta->source_dir = save_dir;
    jd_dex *dex = dex_init(meta, thread_num);

    if (type == JD_DEX_TASK_DECOMPILE) {
        if (thread_num > 1) {
            dex_decompile_threadpool_start(dex);
        } else {
            dex_decompile_main_thread_start(dex);
        }
    }
    else if (type == JD_DEX_TASK_SMALI) {
        if (thread_num > 1) {
            dex_smali_threadpool_start(dex);
        } else {
            dex_smali_main_thread_start(dex);
        }
    }

    dex_release(dex);
}

void dex_file_dump(string path)
{
    mem_init_pool();
    jd_meta_dex *meta = parse_dex_file(path);
    dexdump(meta);
    mem_free_pool();
}

void dex_analyse(jd_meta_dex *meta)
{
    jd_dex *dex = dex_init_without_thread(meta);
    dex_header *header = meta->header;

    for (int i = 0; i < header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];

        // string name = dex_str_of_type_id(meta, cf->class_idx);
        if (access_flags_contains(cf->access_flags, ACC_DEX_SYNTHETIC))
            continue;

        // if (!dex_class_filter(meta, cf)) continue;

        dex_decompile_class(dex, cf);
    }

    mem_pool_free(meta->pool);
}

void dex_analyse_in_apk_task(jd_meta_dex *meta)
{
    jd_dex *dex = dex_init(meta, 1);

    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (dex_class_is_inner_class(dex->meta, cf) ||
            dex_class_is_anonymous_class(dex->meta, cf))
            continue;

        jsource_file *jf = dex_class_inside(dex, cf, NULL);
        if (jf->parent == NULL) {
            writter_for_class(jf, NULL);
            fclose(jf->source);
        }
    }

    mem_pool_free(meta->pool);
}

#include <errno.h>
#include "common/output_error.h"
#include "dalvik/dex_decompile.h"
#include "dalvik/dex_structure.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_class.h"
#include "dalvik/dex_lambda.h"
#include "dalvik/dex_exception.h"
#include "dalvik/dex_optimizer.h"
#include "dalvik/dex_simulator.h"
#include "dalvik/dex_meta_helper.h"

#include "decompiler/descriptor.h"
#include "decompiler/method.h"
#include "decompiler/expression_enum.h"
#include "decompiler/expression_synchronized.h"
#include "decompiler/expression_writter.h"
#include "common/output_path.h"
#include "parser/dex/metadata.h"
#include "jvm/jvm_ins.h"
#include "dex_pre_optimizer.h"
#include "decompiler/control_flow.h"
#include "jar/jar.h"
#include "file_tools.h"
#include "dex_annotation.h"
#include "dex_dump.h"
#include "dex_smali.h"
#include "common/jd_progress.h"

static int dex_progress_len = 0;

void dex_status(jd_dex *dex)
{
    if (dex->threadpool)
        pthread_mutex_lock(dex->threadpool->lock);
    dex->done++;
    const int done = dex->done;
    const int total = dex->added;
    if (!jd_progress_has()) {
        for (int i = 0; i < dex_progress_len; i++) putchar('\b');
        dex_progress_len = printf("Progress : %d (%d)", done, total);
        fflush(stdout);
    }
    if (dex->threadpool)
        pthread_mutex_unlock(dex->threadpool->lock);
    if (jd_progress_has())
        jd_progress_report(done, total, "dex");
}

void dex_main_thread_status(jd_dex *dex)
{
    for (int i = 0; i < dex_progress_len; i++) putchar('\b');
    dex_progress_len = printf("Progress : %d", dex->done);
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
    jd_method *m = make_obj_zero(jd_method);

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

static int dex_hide_rebuilt_lambda_bodies(jsource_file *jf)
{
    jd_dex *dex = jf->meta;
    jd_meta_dex *meta = dex->meta;
    if (meta->rebuilt_lambda_bodies == NULL || jf->methods == NULL)
        return 0;

    int hidden = 0;
    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (m == NULL || m->meta_method == NULL)
            continue;
        encoded_method *em = m->meta_method;
        pthread_mutex_lock(&meta->lambda_lock);
        void *hit = hget_u4obj(meta->rebuilt_lambda_bodies, em->method_id);
        pthread_mutex_unlock(&meta->lambda_lock);
        if (hit == NULL)
            continue;
        method_mark_lambda(m);
        hidden++;
    }
    return hidden;
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

void dex_reserve_output_paths(jd_meta_dex *meta, bool decompile)
{
    if (meta->source_dir == NULL)
        return;

    int collided = 0;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (decompile &&
            (dex_class_is_inner_class(meta, cf) ||
             dex_class_is_anonymous_class(meta, cf) ||
             dex_class_is_rebuilt_lambda_class(meta, cf)))
            continue;

        string fname = class_full_name(dex_str_of_type_id(meta, cf->class_idx));
        string sname = class_simple_name_without_primitive(fname);
        string pname = class_package_name_of(fname);
        if (pname == NULL)
            pname = (string) g_str_default;

        if (output_path_reserve(
                    str_create("%s/%s", meta->source_dir, pname),
                    decompile ? str_create("%s.java", sname)
                              : str_create("%s.smali", sname)))
            collided++;
    }

    if (collided > 0) {
        fprintf(stderr, "[garlic] warning: %d classes under %s share a file "
                        "name with a class differing only in case; the later "
                        "ones are written with a _N suffix\n",
                collided, meta->source_dir);
    }
}

static void dex_class_source_save_dir(jd_dex *dex, jsource_file *jf)
{
    jd_meta_dex *meta = dex->meta;
    if (meta->source_dir == NULL || jf->is_anonymous || jf->is_inner)
        return;
    string full_dir = str_create("%s/%s", meta->source_dir, jf->pname);
    mkdir_p(full_dir);

    string path = str_create("%s/%s", full_dir,
                             output_path_resolve(
                                     full_dir, str_create("%s.java", jf->sname)));
    FILE *stream = fopen(path, "wb");
    if (stream == NULL) {
        output_open_failed(path);
        return;
    }
    jf->source = stream;
}

FILE* dex_class_smali_save_dir(jd_dex *dex, dex_class_def *cf)
{
    jd_meta_dex *meta = dex->meta;
    string desc = dex_str_of_type_id(dex->meta, cf->class_idx);
    string fname = class_full_name(desc);
    string sname = class_simple_name_without_primitive(fname);
    string pname = class_package_name_of(fname);
    // if package name is null, make it as default
    if (pname == NULL)
        pname = (string) g_str_default;

    string full_dir = str_create("%s/%s", meta->source_dir, pname);
    mkdir_p(full_dir);

    string path = str_create("%s/%s", full_dir,
                             output_path_resolve(
                                     full_dir, str_create("%s.smali", sname)));
    FILE *stream = fopen(path, "wb");
    if (stream == NULL) {
        output_open_failed(path);
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
    jsource_file *jf = make_obj_zero(jsource_file);
    string desc = dex_str_of_type_id(dex->meta, cf->class_idx);
    jf->fname = class_full_name(desc);
    jf->sname = class_simple_name_without_primitive(jf->fname);
    jf->pname = class_package_name(jf);
    if (jf->pname == NULL)
        jf->pname = (string) g_str_default;
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
    jf->enum_constants = NULL;
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
        dex_class_annotations(jf);
        class_create_blocks(jf);
        class_create_definations(jf);
        return jf;
    }

    dex_fields(jf);

    dex_methods(jf);

    dex_hide_rebuilt_lambda_bodies(jf);

    optimize_enum_class(jf);

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
    mem_pool *outer = mem_scratch_enter();
    if (!dex_class_is_inner_class(dex->meta, cf) &&
        !dex_class_is_anonymous_class(dex->meta, cf) &&
        !dex_class_is_rebuilt_lambda_class(dex->meta, cf)) {
        jsource_file *jf = dex_class_inside(dex, cf, NULL);
        if (jf->parent == NULL) {
            writter_for_class(jf, NULL);
            output_close(jf->source, jf->fname);
        }
    }
    mem_scratch_leave(outer);
}

void dex_smali_class(jd_dex *dex, dex_class_def *cf)
{
    mem_pool *outer = mem_scratch_enter();
    FILE *stream = dex_class_smali_save_dir(dex, cf);
    if (stream != NULL) {
        dex_class_def_to_smali(dex->meta, cf, stream);
        output_close(stream, dex_str_of_type_id(dex->meta, cf->class_idx));
    }
    mem_scratch_leave(outer);
}

void dex_to_source(string dex_path, string save_dir)
{
    jd_meta_dex *meta = parse_dex_file(dex_path);
    meta->source_dir = save_dir;
    mkdir_p(meta->source_dir);
    dex_reserve_output_paths(meta, true);
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

    dex_lambda_collect_bodies(dex);

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

    dex_lambda_collect_bodies(dex);
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
        output_close(jf->source, jf->fname);
    }
    mem_pool_free(tls->pool);
    tls->pool = NULL;

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
        output_close(stream, dex_str_of_type_id(dex->meta, cf->class_idx));

    mem_pool_free(tls->pool);
    tls->pool = NULL;

    dex_status(dex);
}

void dex_decompile_threadpool_start(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (dex_class_is_inner_class(dex->meta, cf) ||
            dex_class_is_anonymous_class(dex->meta, cf) ||
            dex_class_is_rebuilt_lambda_class(dex->meta, cf))
            continue;

        if (dex->threadpool) {
            jd_dex_task *t = make_obj(jd_dex_task);
            t->dex = dex;
            t->cf = cf;
            int ret = threadpool_add(dex->threadpool, &dex_decompile_thread_task, t, 0);
            if (ret != 0) {
                fprintf(stderr, "[garlic] Warning: threadpool_add failed with %d\n", ret);
            }
            dex->added++;
        } else {
            /* Single-threaded: process synchronously */
            dex_decompile_class(dex, cf);
            dex->done++;
            dex_main_thread_status(dex);
        }
    }
}

void dex_decompile_main_thread_start(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        mem_pool *outer = mem_scratch_enter();
        dex_class_def *cf = &meta->class_defs[i];
        /* Skipped classes still close the pool they opened; only the
         * counted ones advance the progress. */
        if (!dex_class_is_inner_class(dex->meta, cf) &&
            !dex_class_is_anonymous_class(dex->meta, cf)) {
            jsource_file *jf = dex_class_inside(dex, cf, NULL);
            if (jf->parent == NULL) {
                writter_for_class(jf, NULL);
                output_close(jf->source, jf->fname);
            }
            dex->done ++;
            dex_main_thread_status(dex);
        }
        mem_scratch_leave(outer);
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
        mem_pool *outer = mem_scratch_enter();
        dex_class_def *cf = &meta->class_defs[i];

        FILE *stream = dex_class_smali_save_dir(dex, cf);

        dex_class_def_to_smali(dex->meta, cf, stream);

        mem_scratch_leave(outer);
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
    dex_reserve_output_paths(meta, type == JD_DEX_TASK_DECOMPILE);
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

    dex_lambda_report(meta);

    expand_stat_report();
    sync_stat_report();
}

void dex_file_dump(string path)
{
    mem_init_pool();
    jd_meta_dex *meta = parse_dex_file(path);
    dexdump(meta);
    mem_free_pool();
}

static bool dex_class_filter(jd_meta_dex *meta, dex_class_def *cf)
{
    string class_name = dex_str_of_type_id(meta, cf->class_idx);
    return STR_EQL(class_name, "Lcom/vivo/push/e;");
}

void dex_analyse(jd_meta_dex *meta)
{
    jd_dex *dex = dex_init_without_thread(meta);
    dex_header *header = meta->header;

    for (int i = 0; i < header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];

        if (access_flags_contains(cf->access_flags, ACC_DEX_SYNTHETIC))
            continue;

        /**
         * if (!dex_class_filter(meta, cf)) continue;
         *
         * debug dex special class
         * if want to debug **Lcom/vivo/push/e;**
         * in dex_class_filter, check current class's descriptor equals wanted
         **/

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
            output_close(jf->source, jf->fname);
        }
    }

    mem_pool_free(meta->pool);
}

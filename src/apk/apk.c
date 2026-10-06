#include <errno.h>
//#include "apk/apk.h"
#include "common/output_error.h"
#include "parser/dex/metadata.h"
#include "dalvik/dex_decompile.h"
#include "dalvik/dex_structure.h"
#include "dalvik/dex_class.h"
#include "dalvik/dex_pre_optimizer.h"
#include "decompiler/expression_branches.h"
#include "decompiler/exception.h"
#include "decompiler/expression_branches.h"
#include "decompiler/expression_synchronized.h"
#include "decompiler/expression_writter.h"
#include "dex_smali.h"
#include "apk_manifest.h"
#include "common/jd_progress.h"
#include "libs/memory/mem_pool.h"

static int apk_progress_len = 0;

void apk_status(jd_apk *apk)
{
    if (apk->threadpool)
        pthread_mutex_lock(apk->threadpool->lock);
    apk->done++;
    const int done = apk->done;
    const int total = apk->added;
    if (!jd_progress_has()) {
        for (int i = 0; i < apk_progress_len; i++) putchar('\b');
        apk_progress_len = printf("Progress : %d (%d)", done, total);
        fflush(stdout);
    }
    if (apk->threadpool)
        pthread_mutex_unlock(apk->threadpool->lock);
    if (jd_progress_has())
        jd_progress_report(done, total, "apk");
}

void apk_entry_thread_task(jd_meta_dex *meta)
{
    thread_local_data *tls = get_thread_local_data();
    tls->pool = mem_create_pool();

    dex_analyse_in_apk_task(meta);

    mem_pool_free(tls->pool);
    tls->pool = NULL;
}

void apk_decompile_thread_task(jd_dex_task *task)
{
    thread_local_data *tls = get_thread_local_data();
    tls->pool = mem_create_pool();

    jd_dex *dex = task->dex;
    jd_apk *apk = task->apk;
    dex_class_def *cf = task->cf;

    jsource_file *jf = dex_class_inside(dex, cf, NULL);

    if (jf->parent == NULL) {
        writter_for_class(jf, NULL);
        output_close(jf->source, jf->fname);
    }

    mem_pool_free(tls->pool);
    tls->pool = NULL;

    apk_status(apk);
}

void apk_smali_thread_task(jd_dex_task *task)
{
    thread_local_data *tls = get_thread_local_data();
    tls->pool = mem_create_pool();

    jd_dex *dex = task->dex;
    jd_apk *apk = task->apk;
    dex_class_def *cf = task->cf;

    FILE *stream = dex_class_smali_save_dir(dex, cf);
    if (stream != NULL) {
        dex_class_def_to_smali(dex->meta, cf, stream);
        output_close(stream, dex_str_of_type_id(dex->meta, cf->class_idx));
    }

    mem_pool_free(tls->pool);
    tls->pool = NULL;

    apk_status(apk);
}

static void apk_run_dex_tasks(jd_apk *apk, jd_meta_dex *meta);
static void apk_process_dex_file(jd_apk *apk, string path);

static void apk_process_dex_from_zip(jd_apk *apk, struct zip_t *zip)
{
    if (zip == NULL) return;
    int total = zip_entries_total(zip);
    for (int i = 0; i < total; ++i) {
        zip_entry_openbyindex(zip, i);
        string path_in_zip = (string)zip_entry_name(zip);

        if (strchr(path_in_zip, '/') != NULL) {
            zip_entry_close(zip);
            continue;
        }

        if (str_end_with(path_in_zip, ".apk")) {
            size_t buf_size = zip_entry_size(zip);
            char *buf = malloc(buf_size);
            if (buf) {
                zip_entry_noallocread(zip, (void *)buf, buf_size);
                struct zip_t *nested = zip_stream_open(buf, buf_size, 0, 'r');
                if (nested) {
                    apk_process_dex_from_zip(apk, nested);
                    zip_stream_close(nested);
                }
                free(buf);
            }
            zip_entry_close(zip);
            continue;
        }

        if (!str_end_with(path_in_zip, ".dex")) {
            zip_entry_close(zip);
            continue;
        }

        size_t buf_size = zip_entry_size(zip);
        char *buf = x_alloc_in(apk->pool, buf_size * sizeof(unsigned char));
        zip_entry_noallocread(zip, (void *)buf, buf_size);
        zip_entry_close(zip);

        jd_meta_dex *meta = parse_dex_from_buffer(buf, buf_size);
        apk_run_dex_tasks(apk, meta);
    }
}

static void apk_process_dex_file(jd_apk *apk, string path)
{
    jd_meta_dex *meta = parse_dex_file(path);
    if (meta == NULL)
        return;
    apk_run_dex_tasks(apk, meta);
}

static void apk_run_dex_tasks(jd_apk *apk, jd_meta_dex *meta)
{
    if (meta == NULL)
        return;

    if (apk->dex_pools != NULL)
        ladd_obj(apk->dex_pools, meta->pool);

    jd_dex *dex = dex_init_without_thread(meta);
    meta->source_dir = apk->save_dir;
    dex_reserve_output_paths(meta, apk->type == JD_DEX_TASK_DECOMPILE);

    {
        string class_filter = getenv("GARLIC_CLASS_FILTER");

        for (int j = 0; j < meta->header->class_defs_size; ++j) {
            dex_class_def *cf = &meta->class_defs[j];
            if (class_filter != NULL &&
                strstr(dex_str_of_type_id(meta, cf->class_idx),
                       class_filter) == NULL)
                continue;
            if (apk->type == JD_DEX_TASK_DECOMPILE) {
                if (dex_class_is_inner_class(dex->meta, cf) ||
                    dex_class_is_anonymous_class(dex->meta, cf) ||
                    dex_class_is_rebuilt_lambda_class(dex->meta, cf))
                    continue;
            }

            if (apk->threadpool) {
                jd_dex_task *t = make_obj(jd_dex_task);
                t->dex = dex;
                t->cf = cf;
                t->apk = apk;
                t->type = apk->type;
                int ret;
                if (t->type == JD_DEX_TASK_SMALI) {
                    ret = threadpool_add(apk->threadpool,
                                   &apk_smali_thread_task,
                                   t,
                                   0);
                }
                else {
                    ret = threadpool_add(apk->threadpool,
                                   &apk_decompile_thread_task,
                                   t,
                                   0);
                }
                if (ret != 0) {
                    fprintf(stderr, "[garlic] Warning: threadpool_add failed with %d\n", ret);
                }
                apk->added++;
            } else {
                /* Single-threaded: process synchronously */
                if (apk->type == JD_DEX_TASK_SMALI) {
                    dex_smali_class(dex, cf);
                } else {
                    dex_decompile_class(dex, cf);
                }
                apk->done++;
                apk_status(apk);
            }
        }
    }
}

static void apk_decompile_task_start(jd_apk *apk)
{
    bool manifest_done = false;

    for (int s = 0; s < apk->source_count; ++s) {
        string src = apk->sources[s];
        if (src == NULL)
            continue;

        if (str_end_with(src, ".dex")) {
            apk_process_dex_file(apk, src);
            continue;
        }

        struct zip_t *zip = zip_open(src, 0, 'r');
        if (zip == NULL) {
            fprintf(stderr, "\n[garlic] Failed to open APK: %s (invalid zip?)\n", src);
            continue;
        }
        apk->zip = zip;
        apk->entries_size = zip_entries_total(zip);

        if (!manifest_done) {
            for (int i = 0; i < (int)apk->entries_size; ++i) {
                zip_entry_openbyindex(zip, i);
                string path_in_zip = (string)zip_entry_name(zip);
                if (path_in_zip == NULL) {
                    continue;
                }

                if (str_end_with(path_in_zip, "AndroidManifest.xml")) {
                    apk_parse_manifest_from_zip(apk);
                    manifest_done = true;
                    zip_entry_close(zip);
                    break;
                }
                zip_entry_close(zip);
            }
        }

        apk_process_dex_from_zip(apk, zip);
        zip_close(zip);
        apk->zip = NULL;
    }
}

static void apk_release(jd_apk *apk)
{
    if (apk->threadpool)
        threadpool_destroy(apk->threadpool, 1);

    if (getenv("GARLIC_POOL_STAT") != NULL) {
        double mb = 1024.0 * 1024.0;
        fprintf(stderr, "[garlic] pool apk: %.0f MB\n",
                apk->pool->total_size / mb);
        if (apk->dex_pools != NULL) {
            size_t total = 0;
            for (size_t i = 0; i < apk->dex_pools->size; ++i) {
                mem_pool *p = (mem_pool *) lget_obj(apk->dex_pools, i);
                total += p->total_size;
            }
            fprintf(stderr, "[garlic] pool dex x%zu: %.0f MB\n",
                    apk->dex_pools->size, total / mb);
        }
        if (apk->sources != NULL && apk->source_count > 0)
            fprintf(stderr, "[garlic] source: %s\n", apk->sources[0]);
    }

    // must follow order 
    if (apk->dex_pools != NULL) {
        for (size_t i = 0; i < apk->dex_pools->size; ++i)
            mem_pool_free((mem_pool *) lget_obj(apk->dex_pools, i));
    }

    mem_pool_free(apk->pool);
    mem_free_pool();
}

void apk_decompile_analyse(string path,
                           string save_dir,
                           int thread_num,
                           jd_dex_task_type type)
{
    string one[1];
    one[0] = path;
    apk_decompile_analyse_sources(one, 1, save_dir, thread_num, type);
}

void apk_decompile_analyse_sources(string *sources,
                                   int source_count,
                                   string save_dir,
                                   int thread_num,
                                   jd_dex_task_type type)
{
    if (sources == NULL || source_count <= 0)
        return;

    mem_init_pool();

    mem_pool *pool = mem_create_pool();
    jd_apk *apk = make_obj_in(jd_apk, pool);
    apk->pool = pool;
    apk->sources = sources;
    apk->source_count = source_count;
    apk->path = sources[0];
    apk->save_dir = save_dir;
    apk->thread_num = thread_num;
    apk->type = type;
    apk->dex_pools = linit_object_with_pool(apk->pool);

    apk->threadpool = threadpool_create_in(apk->pool, thread_num, 0);

    apk_decompile_task_start(apk);

    string done_path = apk->path;

    apk_release(apk);

    if (getenv("GARLIC_SUFFIX_STAT") != NULL) {
        fprintf(stderr, "[garlic] shared suffixes copied: %d (after %s)\n",
                share_suffix_copy_count(), done_path);
    }

    if (getenv("GARLIC_IF_STAT") != NULL) {
        int promoted = 0;
        int merged = 0;
        int shared = 0;
        empty_if_branch_stat(&promoted, &merged, &shared);
        fprintf(stderr,
                "[garlic] empty if branches taken out: %d, "
                "nested ifs merged: %d, "
                "branches left empty by a shared target: %d (after %s)\n",
                promoted, merged, shared, done_path);
    }

    exc_time_report();
    empty_if_left_report();
    expand_stat_report();
    sync_stat_report();
}

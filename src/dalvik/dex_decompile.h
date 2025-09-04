#ifndef GARLIC_DEX_DECOMPILE_H
#define GARLIC_DEX_DECOMPILE_H

#include "dalvik/dex_structure.h"
#include "apk/apk.h"

typedef enum {
    JD_DEX_TASK_DECOMPILE = 0,
    JD_DEX_TASK_SMALI,
    JD_DEX_TASK_ALL,
} jd_dex_task_type;

typedef struct {
    jd_dex *dex;
    jd_apk *apk;
    dex_class_def *cf;
    jd_dex_task_type type;
} jd_dex_task;

void dex_decompile_threadpool_start(jd_dex *dex);

void dex_decompile_main_thread_start(jd_dex *dex);

void dex_file_analyse(string path, string save_dir, int thread_num, jd_dex_task_type type);

jd_dex* dex_init_without_thread(jd_meta_dex *meta);

void dex_decompile_thread_task(jd_dex_task *task);

void dex_analyse_in_apk_task(jd_meta_dex *meta);

void dex_file_dump(string path);

void dex_analyse(jd_meta_dex *meta);

jsource_file* dex_class_inside(jd_dex *dex, 
                               dex_class_def *cf, 
                               jsource_file *parent);

jsource_file* dex_inner_class(jd_dex *dex, 
                              jsource_file *parent, 
                              dex_class_def *cf);

jd_method *dex_method(jsource_file *jf, encoded_method *em);

void dex_to_source(string dex_path, string save_dir);

#endif

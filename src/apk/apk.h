#ifndef GARLIC_APK_H
#define GARLIC_APK_H

#include "dex_structure.h"
#include "dex_decompile.h"

void apk_decompile_analyse(string path,
                           string save_dir,
                           int thread_num,
                           jd_dex_task_type type);

void apk_decompile_analyse_sources(string *sources,
                                   int source_count,
                                   string save_dir,
                                   int thread_num,
                                   jd_dex_task_type type);

#endif //GARLIC_APK_H

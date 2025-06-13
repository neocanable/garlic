#ifndef GARLIC_APK_H
#define GARLIC_APK_H

#include "dex_structure.h"

typedef struct jd_apk {
    string path;
    string save_dir;
    int thread_num;
    struct zip_t *zip;
    size_t entries_size;
    mem_pool *pool;
    threadpool_t *threadpool;
    int added;
    int done;
} jd_apk;

void apk_file_analyse(string path, string save_dir, int thread_num);

#endif //GARLIC_APK_H

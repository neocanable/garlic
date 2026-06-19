#ifndef GARLIC_APK_MANIFEST_H
#define GARLIC_APK_MANIFEST_H

#include "dex_class.h"

typedef struct {
    string path;
    mem_pool *pool;
    jd_bin *bin;
} apk_manifest;


typedef struct {
    char** table;
    uint32_t count;
} axml_string_table_t;

void apk_parse_manifest(string path);

void apk_parse_manifest_from_zip(jd_apk *apk);

#endif //GARLIC_APK_MANIFEST_H

#ifndef GARLIC_DEX_SMALI_H
#define GARLIC_DEX_SMALI_H
#include "dex_structure.h"

void dex_to_smali(string path);

void dex_class_def_to_smali(jd_meta_dex *dex, dex_class_def *cf, FILE *stream);

#endif //GARLIC_DEX_SMALI_H

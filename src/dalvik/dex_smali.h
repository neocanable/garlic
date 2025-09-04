#ifndef GARLIC_DEX_SMALI_H
#define GARLIC_DEX_SMALI_H
#include "dex_structure.h"

void dex2smali(string path);

void class2smali(jd_meta_dex *dex, dex_class_def *cf);

#endif //GARLIC_DEX_SMALI_H

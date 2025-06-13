#ifndef GARLIC_DEX_OPTIMIZER_H
#define GARLIC_DEX_OPTIMIZER_H

#include "dalvik/dex_structure.h"

void optimize_dex_method(jd_method *m);

void dexdump_method(jd_method *m);

#endif //GARLIC_DEX_OPTIMIZER_H

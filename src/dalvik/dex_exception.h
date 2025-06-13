#ifndef GARLIC_DEX_EXCEPTION_H
#define GARLIC_DEX_EXCEPTION_H

#include "dalvik/dex_structure.h"

void dex_method_exception_init(jd_method *m, encoded_method *em);

void dex_method_exception_edge(jd_method *m);

#endif //GARLIC_DEX_EXCEPTION_H

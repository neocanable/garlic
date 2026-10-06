#ifndef GARLIC_DEX_PRE_OPTIMIZER_H
#define GARLIC_DEX_PRE_OPTIMIZER_H

#include "dalvik/dex_structure.h"

void pre_optimize_dex_method(jd_method *m);

void optimize_move_exception_goto(jd_method *m);

void share_suffix_count_one(void);

int share_suffix_copy_count(void);

#endif //GARLIC_DEX_PRE_OPTIMIZER_H

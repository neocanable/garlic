#ifndef GARLIC_DEX_TYPE_ANALYSE_H
#define GARLIC_DEX_TYPE_ANALYSE_H

#include "dalvik/dex_structure.h"

bool match_dex_debug(jd_method *m, jd_dex_ins *ins, jd_val *val, int slot);

bool match_dex_parameter(jd_method *m, jd_stack *stack);

#endif //GARLIC_DEX_TYPE_ANALYSE_H

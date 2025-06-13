#ifndef GARLIC_DEX_SIMULATOR_H
#define GARLIC_DEX_SIMULATOR_H

#include "dalvik/dex_structure.h"
#include "dalvik/dex_method.h"

void dex_simulator(jd_method *m);

void dex_variable_name(jd_method *m, jd_dex_ins *ins, jd_val *val, int slot);

#endif //GARLIC_DEX_SIMULATOR_H

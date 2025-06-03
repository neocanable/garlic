#ifndef GARLIC_EXPRESSION_INLINE_H
#define GARLIC_EXPRESSION_INLINE_H

#include "decompiler/structure.h"

bool inline_variables(jd_method *m);

bool inline_variables_v2(jd_method *m);

int inline_variables_round2(jd_method *m);

static inline bool stack_var_can_inline(jd_var *var)
{
    return var->def_count == var->use_count;
}


#endif //GARLIC_EXPRESSION_INLINE_H

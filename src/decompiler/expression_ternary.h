#ifndef GARLIC_EXPRESSION_TERNARY_H
#define GARLIC_EXPRESSION_TERNARY_H

#include "decompiler/structure.h"

bool identify_ternary_operator(jd_method *m);

bool identify_ternary_operator_in_condition(jd_method *m);

#endif //GARLIC_EXPRESSION_TERNARY_H

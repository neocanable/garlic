#ifndef GARLIC_EXPRESSION_LOGICAL_H
#define GARLIC_EXPRESSION_LOGICAL_H

#include "decompiler/structure.h"

bool identify_logical_operations(jd_method *m);

void identify_cmp_after_if(jd_method *m);

void make_logic_not(jd_exp *expression);

bool identify_reverse_logical_operation(jd_method *m);

bool identify_logical_with_assignment(jd_method *m);

#endif //GARLIC_EXPRESSION_LOGICAL_H

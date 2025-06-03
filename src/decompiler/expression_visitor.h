

#ifndef GARLIC_EXPRESSION_VISITOR_H
#define GARLIC_EXPRESSION_VISITOR_H

#include "decompiler/structure.h"

list_object* get_expression_for_loop(jd_exp *expression);

bool has_stack_var(jd_exp *expression, jd_var *var);

#endif //GARLIC_EXPRESSION_VISITOR_H

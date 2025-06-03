#ifndef GARLIC_EXPRESSION_IF_H
#define GARLIC_EXPRESSION_IF_H

#include "decompiler/structure.h"

void negative_if_expression(jd_method *m);

void identify_boolean_in_if(jd_method *m);

void identify_if_break_or_if_continue(jd_method *m);

#endif //GARLIC_EXPRESSION_IF_H

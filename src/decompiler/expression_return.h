#ifndef GARLIC_EXPRESSION_RETURN_H
#define GARLIC_EXPRESSION_RETURN_H

#include "decompiler/structure.h"

bool duplicate_return_if_goto(jd_method *m);

void nop_node_last_return(jd_method *m);

#endif //GARLIC_EXPRESSION_RETURN_H

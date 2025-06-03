#ifndef GARLIC_EXPRESSION_CHAIN_H
#define GARLIC_EXPRESSION_CHAIN_H

#include "decompiler/structure.h"

bool identify_assignment_chain_store(jd_method *m);

bool identify_assignment_chain(jd_method *m);

bool identify_define_stack_variable_chain(jd_method *m);

#endif //GARLIC_EXPRESSION_CHAIN_H

#ifndef GARLIC_EXPRESSION_BRANCHES_H
#define GARLIC_EXPRESSION_BRANCHES_H

#include "decompiler/structure.h"

void remove_empty_if_else_of_method(jd_method *m);

void identify_branches(jd_method *m);

void identify_else_if_of_method(jd_method *m);

void add_basic_blocks_to_node(jd_method *m,
                              jd_node *parent,
                              jd_node *node,
                              list_object *blocks);


#endif //GARLIC_EXPRESSION_BRANCHES_H

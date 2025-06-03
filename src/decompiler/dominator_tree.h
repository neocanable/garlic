#ifndef GARLIC_DOMINATOR_TREE_H
#define GARLIC_DOMINATOR_TREE_H

#include "decompiler/structure.h"

typedef int (*traversal_cb)(jd_bblock *block);

void create_dominator_tree(jd_method *m);

void compute_dominates_block(jd_method *m, jd_bblock *block);

void clear_dominator_data(jd_method *m);

void clear_dominator_tree(jd_method *m);

void dominator_tree(jd_method *m);

void dominance_frontier(jd_method *m);

bool dominates(const jd_bblock *check, const jd_bblock *other);

#endif //GARLIC_DOMINATOR_TREE_H

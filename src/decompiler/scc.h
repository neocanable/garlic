#ifndef GARLIC_SCC_H
#define GARLIC_SCC_H

#include "structure.h"

typedef struct jd_tarjan_state {
    int index;
    int low_link;
    bool on_stack;
} jd_tarjan_state;

list_object* compute_scc(jd_method *m);

#endif //GARLIC_SCC_H

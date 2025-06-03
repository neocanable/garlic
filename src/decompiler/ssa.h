
#ifndef GARLIC_SSA_H
#define GARLIC_SSA_H

#include "decompiler/structure.h"


#define DEBUG_SSA false
#define DEBUG_SSA_PRINT(FMT, ARGS...) do {                  \
    if ( DEBUG_SSA )                                        \
    fprintf(stdout, "" FMT "", ## ARGS);                    \
} while (0)


void sform_prepare_for_local_variable(jd_method *m);

void sform_for_local_variable(jd_method *m);

void sform_for_stack_variable(jd_method *m);

void print_sform_and_liveness(jd_method *m);

#endif //GARLIC_SSA_H

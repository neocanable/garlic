
#ifndef GARLIC_EXPRESSION_SYNCHRONIZED_H
#define GARLIC_EXPRESSION_SYNCHRONIZED_H

#include "decompiler/structure.h"

int identify_synchronized(jd_method *m);

/* Diagnostics, on with GARLIC_SYNC_STAT. */
void sync_stat_count_leftover(jd_method *m);
void sync_stat_report(void);

#endif //GARLIC_EXPRESSION_SYNCHRONIZED_H

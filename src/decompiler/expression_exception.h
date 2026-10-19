#ifndef GARLIC_EXPRESSION_EXCEPTION_H
#define GARLIC_EXPRESSION_EXCEPTION_H

#include "structure.h"

void optimize_synchronized_variable(jd_method *m);

void optimize_exception_block(jd_method *m);

void fix_exception_handler_blocks(jd_method *m);

void nop_catch_handler_gotos(jd_method *m);

#endif //GARLIC_EXPRESSION_EXCEPTION_H

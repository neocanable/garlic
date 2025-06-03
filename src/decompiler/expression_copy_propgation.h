#ifndef GARLIC_EXPRESSION_COPY_PROPGATION_H
#define GARLIC_EXPRESSION_COPY_PROPGATION_H

#include "decompiler/structure.h"

bool copy_propagation_of_expression(jd_method *m);

bool copy_propagation_of_dup_local_variable(jd_method *m);

#endif //GARLIC_EXPRESSION_COPY_PROPGATION_H

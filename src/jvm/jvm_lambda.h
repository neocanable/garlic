#ifndef GARLIC_JVM_LAMBDA_H
#define GARLIC_JVM_LAMBDA_H

#include "decompiler/structure.h"

jd_lambda* identify_lambda_expression(jd_method *m, jd_exp *exp);

void identify_string_concat_expression(jd_method *m, jd_exp *exp);

jclass_bootstrap_method* get_bootstrap_method_attr(jd_ins *ins);

jd_method_sig* get_method_sig_of_ins(jd_ins *ins);

#endif //GARLIC_JVM_LAMBDA_H

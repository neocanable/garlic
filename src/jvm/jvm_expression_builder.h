
#ifndef GARLIC_JVM_EXPRESSION_BUILDER_H
#define GARLIC_JVM_EXPRESSION_BUILDER_H

#include "decompiler/structure.h"

jd_exp* build_string_exp(jd_exp *exp, string str);

void build_assignment_expression(jd_exp *exp, jd_ins *ins);

void build_stack_var_exp(jd_exp *exp, jd_val *val);

void build_stack_var_exp_of_store(jd_exp *exp, jd_val *val);

void build_expression(jd_exp *exp, jd_ins *ins);

void instruction_to_expression(jd_method *m);

#endif //GARLIC_JVM_EXPRESSION_BUILDER_H

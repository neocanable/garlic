#ifndef GARLIC_JVM_SIMULATOR_H
#define GARLIC_JVM_SIMULATOR_H

#include "decompiler/structure.h"

// https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-2.html
static inline bool jd_stack_val_is_compute_category1(jd_val *var)
{
    return var->type != JD_VAR_LONG_T && var->type != JD_VAR_DOUBLE_T;
}

static inline bool jd_stack_val_is_compute_category2(jd_val *val)
{
    // jd_var_long_t or jd_var_double_t
    return !jd_stack_val_is_compute_category1(val);
}

int dupped_ins_count(jd_ins *ins);

void build_jvm_ins_dup_action(jd_ins *ins);

void build_jvm_ins_dup_x1_action(jd_ins *ins);

void build_jvm_ins_dup_x2_action(jd_ins *ins);

void build_jvm_ins_dup2_action(jd_ins *ins);

void build_jvm_ins_dup2_x1_action(jd_ins *ins);

void build_jvm_ins_dup2_x2_action(jd_ins *ins);

void build_jvm_ins_swap_action(jd_ins *ins);

void build_jvm_ins_pop_action(jd_ins *ins);

void build_jvm_ins_pop2_action(jd_ins *ins);

void instruction_stack_action(jd_ins *ins);

void jvm_simulator(jd_method *m);

static inline jd_val* find_local_variable(jd_stack *stack, int slot)
{
    return stack->local_vars[slot];
}

static inline int find_local_variable_index(jd_stack *stack, int slot)
{
    return slot;
}

#endif //GARLIC_JVM_SIMULATOR_H

#include "dalvik/dex_optimizer.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_expression_builder.h"

#include "decompiler/control_flow.h"
#include "decompiler/dominator_tree.h"
#include "decompiler/method.h"
#include "decompiler/expression_enum.h"

#include "decompiler/expression_if.h"
#include "decompiler/expression_logical.h"
#include "decompiler/expression_array.h"
#include "decompiler/expression_new.h"
#include "decompiler/expression_ternary.h"
#include "decompiler/expression_chain.h"
#include "decompiler/expression_assign.h"
#include "decompiler/expression_loop.h"
#include "decompiler/expression_branches.h"
#include "decompiler/expression_loop_type.h"
#include "decompiler/expression_local_variable.h"
#include "decompiler/expression_goto.h"
#include "decompiler/expression_synchronized.h"
#include "decompiler/expression_copy_propgation.h"
#include "decompiler/expression_node_param.h"
#include "decompiler/expression_return.h"
#include "decompiler/expression_exception.h"


static bool dex_ins_is_const_int(jd_dex_ins *ins)
{
    if (ins == NULL)
        return false;

    switch (ins->code) {
        case DEX_INS_CONST_4:
        case DEX_INS_CONST_16:
        case DEX_INS_CONST:
        case DEX_INS_CONST_HIGH16:
            return true;
        default:
            return false;
    }
}

static jd_exp* dex_stored_value(jd_exp *expression)
{
    if (expression == NULL)
        return NULL;

    if (exp_is_store(expression)) {
        jd_exp_store *store = expression->data;
        if (store->list == NULL || store->list->len < 2)
            return NULL;
        return &store->list->args[1];
    }

    if (exp_is_assignment(expression)) {
        jd_exp_assignment *assignment = expression->data;
        return assignment->right;
    }

    return NULL;
}

static bool identify_fill_array_data(jd_method *m)
{
    bool folded = false;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);
        if (exp_is_nopped(expression) || !exp_is_new_array(expression))
            continue;

        jd_dex_ins *ins = (jd_dex_ins *) expression->ins;
        if (ins == NULL || ins->code != DEX_INS_FILL_ARRAY_DATA)
            continue;

        jd_dex_ins *previous = ins->prev;
        if (previous == NULL || previous->code != DEX_INS_NEW_ARRAY)
            continue;

        if (dex_ins_parameter(previous, 0) != dex_ins_parameter(ins, 0))
            continue;

        jd_exp *created = dex_stored_value(previous->expression);
        if (created == NULL || !exp_is_new_array(created))
            continue;

        jd_exp_new_array *array = created->data;
        jd_exp_new_array *payload = expression->data;
        if (array->list == NULL || array->list->len != 1 ||
            payload->list == NULL || payload->list->len == 0)
            continue;

        if (previous->stack_in == NULL || previous->stack_in->local_vars == NULL)
            continue;

        u1 count_reg = dex_ins_parameter(previous, 1);
        if (count_reg >= previous->stack_in->local_vars_count)
            continue;

        jd_val **locals = previous->stack_in->local_vars;
        jd_val *count = locals[count_reg];
        if (count == NULL || count->data == NULL || count->data->primitive == NULL)
            continue;
        if (!stack_val_is_int(count) || !dex_ins_is_const_int(count->ins))
            continue;

        if ((u4) count->data->primitive->int_val != payload->list->len - 1)
            continue;

        array->list = payload->list;
        build_empty_expression(expression, ins);
        folded = true;
    }

    return folded;
}

void optimize_dex_method(jd_method *m)
{
    if (method_is_empty(m))
        return;

    dex_instruction_to_expression(m);

    negative_if_expression(m);

    nop_empty_expression(m);

    identify_cmp_after_if(m);

    optimize_enum_constructor(m);

    create_node_tree(m);

    bool changed = false;

    do {
        changed = identify_logical_operations(m);

        changed |= identify_reverse_logical_operation(m);

//        changed |= identify_ternary_operator(m);

//        changed |= identify_ternary_operator_in_condition(m);

        changed |= identify_initialize(m);

        changed |= identify_array_initialize(m);

        changed |= identify_fill_array_data(m);

        changed |= copy_propagation_of_expression(m);

    } while (changed);

    identify_assignment(m);

    identify_loop(m);

    identify_branches(m);

    identify_if_break_or_if_continue(m);

    identify_synchronized(m);

    optimize_goto_expression(m);

    expand_suffix_copy_nodes(m);

    analyse_local_variables(m);

    identify_loop_type(m);

//    remove_empty_if_else_of_method(m);

    nop_node_last_return(m);

    setup_expression_node_param(m);

    flatten_empty_if_branches(m);

    attach_dropped_terminals(m);

    optimize_exception_block(m);

    sync_stat_count_leftover(m);

    if (getenv("GARLIC_TREE2") != NULL)
        print_node_tree(m, NULL);
}

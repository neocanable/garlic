#include "dalvik/dex_optimizer.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_expression_builder.h"

#include "decompiler/control_flow.h"
#include "decompiler/dominator_tree.h"
#include "decompiler/method.h"

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


void optimize_dex_method(jd_method *m)
{
    if (method_is_empty(m))
        return;

    dex_instruction_to_expression(m);

    negative_if_expression(m);

    nop_empty_expression(m);

    identify_cmp_after_if(m);

    create_node_tree(m);

    bool changed = false;

    do {
        changed = identify_logical_operations(m);

        changed |= identify_reverse_logical_operation(m);

//        changed |= identify_ternary_operator(m);

//        changed |= identify_ternary_operator_in_condition(m);

        changed |= identify_initialize(m);

        changed |= identify_array_initialize(m);

        changed |= copy_propagation_of_expression(m);

    } while (changed);

    identify_assignment(m);

    identify_loop(m);

    identify_branches(m);

    identify_if_break_or_if_continue(m);

    identify_synchronized(m);

    optimize_goto_expression(m);

    analyse_local_variables(m);

    identify_loop_type(m);

//    remove_empty_if_else_of_method(m);

    nop_node_last_return(m);

    setup_expression_node_param(m);

    optimize_exception_block(m);
}

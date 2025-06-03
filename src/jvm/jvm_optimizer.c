#include "decompiler/method.h"
#include "common/str_tools.h"
#include "jvm/jvm_ins.h"
#include "decompiler/expression.h"
#include "decompiler/expression_inline.h"
#include "decompiler/expression_loop.h"
#include "decompiler/expression_branches.h"
#include "jvm/jvm_expression_builder.h"
#include "decompiler/expression_node.h"
#include "decompiler/expression_helper.h"
#include "decompiler/expression_if.h"
#include "decompiler/expression_return.h"
#include "decompiler/expression_ternary.h"
#include "decompiler/expression_chain.h"
#include "decompiler/expression_logical.h"
#include "decompiler/expression_array.h"
#include "decompiler/expression_new.h"
#include "decompiler/expression_assign.h"
#include "decompiler/expression_local_variable.h"
#include "decompiler/expression_goto.h"
#include "decompiler/expression_synchronized.h"
#include "decompiler/expression_loop_type.h"
#include "jvm/jvm_type_analyse.h"
#include "decompiler/expression_enum.h"
#include "decompiler/expression_remove_useless.h"
#include "decompiler/expression_node_param.h"
#include "decompiler/expression_writter.h"
#include "decompiler/expression_assert.h"
#include "decompiler/expression_copy_propgation.h"
#include "decompiler/expression_exception.h"

static void init_method_data(jd_method *m)
{
    m->declarations = bitset_create();
}

void optimize_jvm_method(jd_method *m)
{
    if (method_is_empty(m))
        return;

    instruction_to_expression(m);

    init_method_data(m);

    jvm_fix_type(m);

    negative_if_expression(m);

    nop_empty_expression(m);

    inline_variables(m);

    identify_cmp_after_if(m);

    optimize_enum_constructor(m);

    create_node_tree(m);

    bool changed = false;

    do {
        changed = identify_logical_operations(m);

        changed |= identify_reverse_logical_operation(m);

        changed |= identify_initialize(m);

        changed |= identify_ternary_operator(m);

        changed |= identify_assignment_chain(m);

        changed |= identify_define_stack_variable_chain(m);

        changed |= identify_assignment_chain_store(m);

        changed |= identify_logical_with_assignment(m);

        changed |= identify_ternary_operator_in_condition(m);

        changed |= identify_array_initialize(m);

        changed |= inline_variables(m);

    } while (changed);

    inline_variables_round2(m);

    identify_assignment(m);

    identify_loop(m);

    identify_branches(m);

    identify_if_break_or_if_continue(m);

    identify_synchronized(m);

    optimize_goto_expression(m);

    analyse_local_variables(m);

    copy_propagation_of_dup_local_variable(m);

    identify_loop_type(m);

    remove_empty_if_else_of_method(m);

    nop_node_last_return(m);

    setup_expression_node_param(m);

    optimize_exception_block(m);
}



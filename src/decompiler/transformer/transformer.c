#include "decompiler/transformer/transformer.h"
#include "jvm/jvm_ins_helper.h"

string exp_to_s(jd_exp *expression)
{
    switch(expression->type)
    {
        case JD_EXPRESSION_INVOKE:
        {
            jd_ins_fn *fn = expression->ins->fn;
            jd_ins *ins = expression->ins;
            if (fn->is_invoke_static(ins))
                return exp_invokestatic_to_s(expression);
            else if (fn->is_invoke_special(ins))
                return exp_invokespecial_to_s(expression);
            else if (fn->is_invoke_virtual(ins))
                return exp_invokevirtual_to_s(expression);
            else if (fn->is_invoke_interface(ins))
                return exp_invokeinterface_to_s(expression);
            else if (fn->is_invoke_dynamic(ins))
                return exp_invokedynamic_to_s(expression);
            else
                return str_dup(g_str_unknown);
        }
        case JD_EXPRESSION_STACK_VALUE:
            return exp_stack_value_to_s(expression);
        case JD_EXPRESSION_LOCAL_VARIABLE:
            return exp_local_variable_to_s(expression);
        case JD_EXPRESSION_CONST:
            return exp_const_to_s(expression);
        case JD_EXPRESSION_IF:
            return exp_if_to_s(expression);
        case JD_EXPRESSION_GET_FIELD:
            return exp_get_field_to_s(expression);
        case JD_EXPRESSION_PUT_FIELD:
            return exp_put_field_to_s(expression);
        case JD_EXPRESSION_GET_STATIC:
            return exp_get_static_to_s(expression);
        case JD_EXPRESSION_PUT_STATIC:
            return exp_put_static_to_s(expression);
        case JD_EXPRESSION_RETURN:
            return exp_return_to_s(expression);
        case JD_EXPRESSION_ARRAY_STORE:
            return exp_array_store_to_s(expression);
        case JD_EXPRESSION_NEW_ARRAY:
            return exp_new_array_to_s(expression);
        case JD_EXPRESSION_SWITCH:
            return exp_switch_to_s(expression);
        case JD_EXPRESSION_GOTO:
            return exp_goto_to_s(expression);
        case JD_EXPRESSION_LVALUE:
            return exp_lvalue_to_s(expression);
        case JD_EXPRESSION_OPERATOR:
            return exp_operator_to_s(expression);
        case JD_EXPRESSION_SINGLE_OPERATOR:
            return exp_single_operator_to_s(expression);
        case JD_EXPRESSION_SINGLE_LIST:
            return exp_single_list_to_s(expression);
        case JD_EXPRESSION_ARRAY_LOAD:
            return exp_array_load_to_s(expression);
        case JD_EXPRESSION_ARRAYLENGTH:
            return exp_arraylength_to_s(expression);
        case JD_EXPRESSION_INSTANCEOF:
            return exp_instanceof_to_s(expression);
        case JD_EXPRESSION_TERNARY:
            return exp_ternary_to_s(expression);
        case JD_EXPRESSION_BREAK:
            return exp_break_to_s(expression);
        case JD_EXPRESSION_CONTINUE:
            return exp_continue_to_s(expression);
        case JD_EXPRESSION_WHILE:
            return exp_while_to_s(expression);
        case JD_EXPRESSION_DO_WHILE:
            return exp_do_while_to_s(expression);
        case JD_EXPRESSION_FOR:
            return exp_for_to_s(expression);
        case JD_EXPRESSION_LOGIC_NOT:
            return exp_logic_not_to_s(expression);
        case JD_EXPRESSION_ASSIGNMENT:
            return exp_assignment_to_s(expression);
        case JD_EXPRESSION_STACK_VAR:
            return exp_stack_var_to_s(expression);
        case JD_EXPRESSION_UNINITIALIZE:
            return exp_uninitialize_to_s(expression);
        case JD_EXPRESSION_CAST:
            return exp_cast_to_s(expression);
        case JD_EXPRESSION_STORE:
            return exp_store_to_s(expression);
        case JD_EXPRESSION_INITIALIZE:
            return exp_initialize_to_s(expression);
        case JD_EXPRESSION_DEFINE_STACK_VAR:
            return exp_define_stack_var_to_s(expression);
        case JD_EXPRESSION_ATHROW:
            return exp_athrow_to_s(expression);
        case JD_EXPRESSION_ASSIGNMENT_CHAIN:
            return exp_assignment_chain_to_s(expression);
        case JD_EXPRESSION_IINC:
            return exp_iinc_to_s(expression);
        case JD_EXPRESSION_DECLARATION:
            return exp_declaration_to_s(expression);
        case JD_EXPRESSION_ASSERT:
            return exp_assert_to_s(expression);
        case JD_EXPRESSION_LAMBDA:
            return exp_lambda_to_s(expression);
        case JD_EXPRESSION_ANONYMOUS:
            return exp_anonymous_to_s(expression);
        case JD_EXPRESSION_MONITOR_ENTER:
            return exp_monitorenter_to_s(expression);
        case JD_EXPRESSION_MONITOR_EXIT:
            return exp_monitorexit_to_s(expression);
        case JD_EXPRESSION_STRING_CONCAT:
            return exp_str_concat_to_s(expression);
        case JD_EXPRESSION_ENUM:
            return exp_enum_to_s(expression);
        case JD_EXPRESSION_IF_BREAK:
            return exp_if_break_to_s(expression);
        default: {
            return (string)g_str_unknown;
        }
    }
}

void expression_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    switch(expression->type) {
        case JD_EXPRESSION_INVOKE: {
            jd_ins_fn *fn = expression->ins->fn;
            jd_ins *ins = expression->ins;
            if (fn->is_invoke_static(ins))
                exp_invokestatic_to_stream(stream, node, expression);
            else if (fn->is_invoke_special(ins))
                exp_invokespecial_to_stream(stream, node, expression);
            else if (fn->is_invoke_virtual(ins))
                exp_invokevirtual_to_stream(stream, node, expression);
            else if (fn->is_invoke_interface(ins))
                exp_invokeinterface_to_stream(stream, node, expression);
            else if (fn->is_invoke_dynamic(ins))
                exp_invokedynamic_to_stream(stream, node, expression);
            else {
                fprintf(stderr, "error of find invoke\n");
                return;
            }
            return;
        }
        case JD_EXPRESSION_STACK_VALUE:
            exp_stack_value_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_LOCAL_VARIABLE:
            exp_local_variable_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_CONST:
            exp_const_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_IF:
            return exp_if_to_stream(stream, node, expression);
        case JD_EXPRESSION_GET_FIELD:
            exp_get_field_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_PUT_FIELD:
            exp_put_field_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_GET_STATIC:
            exp_get_static_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_PUT_STATIC:
            exp_put_static_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_RETURN:
            exp_return_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ARRAY_STORE:
            exp_array_store_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_NEW_ARRAY:
            exp_new_array_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_SWITCH:
            exp_switch_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_GOTO:
            exp_goto_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_LVALUE:
            exp_lvalue_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_OPERATOR:
            exp_operator_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_SINGLE_OPERATOR:
            exp_single_operator_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_SINGLE_LIST:
            exp_single_list_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ARRAY_LOAD:
            exp_array_load_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ARRAYLENGTH:
            exp_arraylength_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_INSTANCEOF:
            exp_instanceof_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_TERNARY:
            exp_ternary_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_BREAK:
            exp_break_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_CONTINUE:
            exp_continue_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_WHILE:
            exp_while_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_DO_WHILE:
            exp_do_while_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_FOR:
            exp_for_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_LOGIC_NOT:
            exp_logic_not_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ASSIGNMENT:
            exp_assignment_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_STACK_VAR:
            exp_stack_var_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_UNINITIALIZE:
            exp_uninitialize_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_CAST:
            exp_cast_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_STORE:
            exp_store_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_INITIALIZE:
            exp_initialize_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_DEFINE_STACK_VAR:
            exp_define_stack_var_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ATHROW:
            exp_athrow_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ASSIGNMENT_CHAIN:
            exp_assignment_chain_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_IINC:
            exp_iinc_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_DECLARATION:
            exp_declaration_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ASSERT:
            exp_assert_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_LAMBDA:
            exp_lambda_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ANONYMOUS:
            exp_anonymous_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_MONITOR_ENTER:
            exp_monitorenter_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_MONITOR_EXIT:
            exp_monitorexit_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_STRING_CONCAT:
            exp_str_concat_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_ENUM:
            exp_enum_to_stream(stream, node, expression);
            return;
        case JD_EXPRESSION_IF_BREAK:
            exp_if_break_to_stream(stream, node, expression);
            return;
        default: {
            return;
        }
    }
}
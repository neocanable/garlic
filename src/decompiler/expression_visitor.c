#include "decompiler/expression_visitor.h"
#include "expression_writter.h"
#include "decompiler/expression_node_helper.h"
#include "decompiler/expression.h"

typedef void (*exp_visitor_fn)(jd_exp *expression, ...);

typedef struct {
    jd_exp *target;
    jd_exp *result;
    bool matched;
} jd_exp_visitor_box;

void visit_expression(jd_exp *expression, exp_visitor_fn fn)
{
    switch (expression->type) {
        case JD_EXPRESSION_LOCAL_VARIABLE: {
            jd_val *val = expression->data;
            fn(expression, val);
            break;
        }
        case JD_EXPRESSION_STACK_VAR: {
            jd_val *val = expression->data;
            fn(expression, val);
            break;
        }
        case JD_EXPRESSION_LVALUE: {
            jd_exp_lvalue *exp_lvalue = expression->data;
            jd_var *var = exp_lvalue->stack_var;
            fn(expression, var);
            break;
        }
        case JD_EXPRESSION_STACK_VALUE: {
            jd_val *val = expression->data;
            fn(expression, val);
            break;
        }
        case JD_EXPRESSION_ASSIGNMENT: {
            jd_exp_assignment *other_assignment = expression->data;
            jd_exp *other_right = other_assignment->right;
            visit_expression(other_right, fn);
            break;
        }
        case JD_EXPRESSION_IF:
        case JD_EXPRESSION_IF_BREAK: {
            jd_exp_if *if_exp = expression->data;
            jd_exp *exp = if_exp->expression;
            visit_expression(exp, fn);
            break;
        }
        case JD_EXPRESSION_CONST: {
            jd_exp_const *const_exp = expression->data;
            break;
        }
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_GET_FIELD:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_ANONYMOUS:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_UNINITIALIZE:
        case JD_EXPRESSION_FOR:
        case JD_EXPRESSION_WHILE:
        case JD_EXPRESSION_DO_WHILE:
        case JD_EXPRESSION_LOGIC_NOT: {
            jd_exp_reader *reader_exp = expression->data;
            for (int i = 0; i < reader_exp->list->len; ++i) {
                jd_exp *exp = &reader_exp->list->args[i];
                visit_expression(exp, fn);
            }
            break;
        }
        case JD_EXPRESSION_ASSERT:
        case JD_EXPRESSION_UNKNOWN:
        case JD_EXPRESSION_EMPTY:
        case JD_EXPRESSION_ASSIGNMENT_CHAIN:
        case JD_EXPRESSION_INVOKE_DYNAMIC:
        case JD_EXPRESSION_INVOKE_INTERFACE:
        case JD_EXPRESSION_INVOKE_SPECIAL:
        case JD_EXPRESSION_INVOKE_STATIC:
        case JD_EXPRESSION_INVOKE_VIRTUAL:
        case JD_EXPRESSION_CONTINUE:
        case JD_EXPRESSION_ANNOTATION:
        case JD_EXPRESSION_GOTO:
        case JD_EXPRESSION_LITERAL:
        case JD_EXPRESSION_SWAP:
        case JD_EXPRESSION_DUP:
        case JD_EXPRESSION_POP:
        case JD_EXPRESSION_BREAK:
        case JD_EXPRESSION_LABEL:
        case JD_EXPRESSION_DECLARATION:
        case JD_EXPRESSION_MONITOR_ENTER:
        case JD_EXPRESSION_MONITOR_EXIT:
        case JD_EXPRESSION_STRING_CONCAT:
        case JD_EXPRESSION_ENUM:
            break;
    }
}

void visit_expression_for_loop(jd_exp *expression, list_object *list)
{
    switch (expression->type) {
        case JD_EXPRESSION_LOCAL_VARIABLE:
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_STACK_VAR:
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_GET_FIELD: {
            ladd_obj(list, expression);
            break;
        }
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_ANONYMOUS:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_UNINITIALIZE:
        case JD_EXPRESSION_FOR:
        case JD_EXPRESSION_WHILE:
        case JD_EXPRESSION_DO_WHILE:
        case JD_EXPRESSION_LOGIC_NOT: {
            jd_exp_reader *reader_exp = expression->data;
            for (int i = 0; i < reader_exp->list->len; ++i) {
                jd_exp *exp = &reader_exp->list->args[i];
                visit_expression_for_loop(exp, list);
            }
            break;
        }
        case JD_EXPRESSION_ASSERT:
        case JD_EXPRESSION_EMPTY:
        case JD_EXPRESSION_ASSIGNMENT_CHAIN:
        case JD_EXPRESSION_INVOKE_DYNAMIC:
        case JD_EXPRESSION_INVOKE_INTERFACE:
        case JD_EXPRESSION_INVOKE_SPECIAL:
        case JD_EXPRESSION_INVOKE_STATIC:
        case JD_EXPRESSION_INVOKE_VIRTUAL:
        case JD_EXPRESSION_CONTINUE:
//        case JD_EXPRESSION_CONDITION:
        case JD_EXPRESSION_ANNOTATION:
        case JD_EXPRESSION_GOTO:
        case JD_EXPRESSION_LITERAL:
        case JD_EXPRESSION_SWAP:
        case JD_EXPRESSION_DUP:
        case JD_EXPRESSION_POP:
        case JD_EXPRESSION_BREAK:
        case JD_EXPRESSION_LABEL:
        case JD_EXPRESSION_DECLARATION:
        case JD_EXPRESSION_MONITOR_ENTER:
        case JD_EXPRESSION_MONITOR_EXIT:
        case JD_EXPRESSION_UNKNOWN:
        case JD_EXPRESSION_IF:
        case JD_EXPRESSION_IF_BREAK:
        case JD_EXPRESSION_CONST:
        case JD_EXPRESSION_ASSIGNMENT:
        case JD_EXPRESSION_LVALUE:
        case JD_EXPRESSION_STACK_VALUE:
        case JD_EXPRESSION_STRING_CONCAT:
        case JD_EXPRESSION_ENUM:
            break;
    }
}

bool has_stack_var(jd_exp *expression, jd_var *var)
{
    switch (expression->type) {
        case JD_EXPRESSION_ASSIGNMENT: {
            jd_exp_assignment *assign = expression->data;
            jd_exp *r = assign->right;
            jd_exp *l = assign->left;
            return has_stack_var(r, var) || has_stack_var(l, var);
        }
        case JD_EXPRESSION_STACK_VAR: {
            return expression->data == var;
        }
        case JD_EXPRESSION_LVALUE: {
            jd_exp_lvalue *exp_lvalue = expression->data;
            return exp_lvalue->stack_var == var;
        }
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_ANONYMOUS:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_UNINITIALIZE:
        case JD_EXPRESSION_FOR:
        case JD_EXPRESSION_WHILE:
        case JD_EXPRESSION_DO_WHILE:
        case JD_EXPRESSION_GET_FIELD:
        case JD_EXPRESSION_LOGIC_NOT: {
            jd_exp_reader *reader_exp = expression->data;
            for (int i = 0; i < reader_exp->list->len; ++i) {
                jd_exp *exp = &reader_exp->list->args[i];
                if (has_stack_var(exp, var))
                    return true;
            }
            break;
        }
        case JD_EXPRESSION_LOCAL_VARIABLE:
        case JD_EXPRESSION_ASSERT:
        case JD_EXPRESSION_EMPTY:
        case JD_EXPRESSION_ASSIGNMENT_CHAIN:
        case JD_EXPRESSION_INVOKE_DYNAMIC:
        case JD_EXPRESSION_INVOKE_INTERFACE:
        case JD_EXPRESSION_INVOKE_SPECIAL:
        case JD_EXPRESSION_INVOKE_STATIC:
        case JD_EXPRESSION_INVOKE_VIRTUAL:
        case JD_EXPRESSION_CONTINUE:
//        case JD_EXPRESSION_CONDITION:
        case JD_EXPRESSION_ANNOTATION:
        case JD_EXPRESSION_GOTO:
        case JD_EXPRESSION_LITERAL:
        case JD_EXPRESSION_SWAP:
        case JD_EXPRESSION_DUP:
        case JD_EXPRESSION_POP:
        case JD_EXPRESSION_BREAK:
        case JD_EXPRESSION_LABEL:
        case JD_EXPRESSION_DECLARATION:
        case JD_EXPRESSION_MONITOR_ENTER:
        case JD_EXPRESSION_MONITOR_EXIT:
        case JD_EXPRESSION_UNKNOWN:
        case JD_EXPRESSION_IF:
        case JD_EXPRESSION_IF_BREAK:
        case JD_EXPRESSION_CONST:
        case JD_EXPRESSION_STACK_VALUE:
        case JD_EXPRESSION_STRING_CONCAT:
        case JD_EXPRESSION_ENUM:
            break;
    }
    return false;
}

void visit_expression_for_local_variable(jd_exp *expression, list_object *list)
{
    switch (expression->type) {
        case JD_EXPRESSION_LOCAL_VARIABLE:
        case JD_EXPRESSION_STACK_VAR: {
            ladd_obj(list, expression);
            break;
        }
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_ANONYMOUS:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_UNINITIALIZE:
        case JD_EXPRESSION_FOR:
        case JD_EXPRESSION_WHILE:
        case JD_EXPRESSION_DO_WHILE:
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_GET_FIELD:
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_LOGIC_NOT: {
            jd_exp_reader *reader_exp = expression->data;
            for (int i = 0; i < reader_exp->list->len; ++i) {
                jd_exp *exp = &reader_exp->list->args[i];
                visit_expression_for_local_variable(exp, list);
            }
            break;
        }
        case JD_EXPRESSION_ASSIGNMENT_CHAIN: {
            break;
        }
        case JD_EXPRESSION_ASSERT: {
            // TODO;
            break;
        }
        case JD_EXPRESSION_ASSIGNMENT: {
            break;
        }
        case JD_EXPRESSION_EMPTY:
        case JD_EXPRESSION_INVOKE_DYNAMIC:
        case JD_EXPRESSION_INVOKE_INTERFACE:
        case JD_EXPRESSION_INVOKE_SPECIAL:
        case JD_EXPRESSION_INVOKE_STATIC:
        case JD_EXPRESSION_INVOKE_VIRTUAL:
        case JD_EXPRESSION_CONTINUE:
        case JD_EXPRESSION_ANNOTATION:
        case JD_EXPRESSION_GOTO:
        case JD_EXPRESSION_LITERAL:
        case JD_EXPRESSION_SWAP:
        case JD_EXPRESSION_DUP:
        case JD_EXPRESSION_POP:
        case JD_EXPRESSION_BREAK:
        case JD_EXPRESSION_LABEL:
        case JD_EXPRESSION_DECLARATION:
        case JD_EXPRESSION_MONITOR_ENTER:
        case JD_EXPRESSION_MONITOR_EXIT:
        case JD_EXPRESSION_UNKNOWN:
        case JD_EXPRESSION_IF:
        case JD_EXPRESSION_IF_BREAK:
        case JD_EXPRESSION_CONST:
        case JD_EXPRESSION_LVALUE:
        case JD_EXPRESSION_STACK_VALUE:
        case JD_EXPRESSION_STRING_CONCAT:
        case JD_EXPRESSION_ENUM:
            break;
    }
}

list_object* get_expression_for_loop(jd_exp *expression)
{
    list_object *list = linit_object();
    visit_expression_for_loop(expression, list);
    return list;
}

list_object* get_expression_for_local_variable(jd_method *m)
{
    list_object *list = linit_object();

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_expression(node))
            continue;
        jd_exp *exp = node->data;
        if (exp_is_nopped(exp))
            continue;

        visit_expression_for_local_variable(exp, list);
    }
    return list;
}
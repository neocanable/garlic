#include "common/str_tools.h"
#include "decompiler/expression_inline.h"
#include "decompiler/expression.h"
#include "control_flow.h"

static bool assignment_exp_can_inline(jd_exp *exp)
{
    jd_exp_assignment *assignment = exp->data;
    jd_exp_lvalue *lvalue = assignment->left->data;
    jd_var *var = lvalue->stack_var;

    jd_exp *right = assignment->right;
    if (exp_is_uninitialize(right))
        return false;

    if (exp_is_new_array(assignment->right)) {
        if (assignment->dupped_count > 0)
            return false;
    }
    return stack_var_can_inline(var);
}

static bool assignment_exp_can_inline_round2(jd_exp *exp)
{
    jd_exp_assignment *assignment = exp->data;
    jd_exp_lvalue *lvalue = assignment->left->data;
    jd_var *var = lvalue->stack_var;

    if (var->def_count == 1 && var->use_count == 1)
        return true;
    else if (var->def_count == 1)
        return true;
    return false;
}

static bool inline_variable_for_expression(jd_exp *expression, jd_exp *other);

static bool replace_expression_with_lvalue(jd_exp *exp, int len, jd_exp *args)
{
    bool replaced = false;
    jd_exp_assignment *assignment = exp->data;
    jd_exp_lvalue *lvalue = assignment->left->data;
    jd_var *stack_var = lvalue->stack_var;
    jd_exp *right = assignment->right;
    for (int i = 0; i < len; ++i) {
        jd_exp *arg = &args[i];
        if (exp_is_stack_var(arg)) {
            jd_var *other_var = arg->data;
            if (stack_var == other_var) {
                arg->type = right->type;
                arg->ins = right->ins;
                arg->data = right->data;

                assignment->def_count --;
                if (assignment->def_count == 0)
                    exp_mark_nopped(exp);

                replaced = true;
                break;
            }
        }
        else {
            replaced = inline_variable_for_expression(exp, arg);
            if (replaced)
                break;
        }
    }

    /**
     * 如果lvalue能被替换
     * 那么lvalue就是一个栈上的临时变量, expression可以忽略掉
     **/
//    if (replaced && stack_var->use_count == 0) {
//        exp_mark_nopped(exp);
//    }
//    else if (replaced && exp_is_assignment(exp)) {
//        jd_exp_assignment *assignment = exp->data;
//        jd_exp *right = assignment->right;
//        if (exp_is_local_variable(right)) {
//            exp_mark_nopped(exp);
//        }
//    }

    return replaced;
}

static bool inline_variable_for_expression(jd_exp *expression, jd_exp *other)
{
    switch(other->type) {
        case JD_EXPRESSION_ASSIGNMENT: {
            jd_exp_assignment *other_assignment = other->data;
            jd_exp *o = other_assignment->right;
            return inline_variable_for_expression(expression, o);
        }
        case JD_EXPRESSION_IF: {
            jd_exp_if *if_exp = other->data;
            jd_exp *exp = if_exp->expression;
            return inline_variable_for_expression(expression, exp);
        }
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_LAMBDA:
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
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_MONITOR_ENTER:
        case JD_EXPRESSION_MONITOR_EXIT:
        case JD_EXPRESSION_STRING_CONCAT: {
            jd_exp_reader *reader_exp = other->data;
            jd_exp_list *list = reader_exp->list;
            return replace_expression_with_lvalue(expression,
                    list->len, list->args);
        }
//        case JD_EXPRESSION_STACK_VAR: {
//            jd_var *var = other->data;
//            jd_exp_assignment *assignment = expression->data;
//            jd_exp_lvalue *lvalue = assignment->left->data;
//            jd_var *stack_var = lvalue->stack_var;
//            if (var == stack_var) {
//                var->def_count --;
//                var->use_count --;
//                other->type = assignment->right->type;
//                other->data = assignment->right->data;
//                exp_mark_nopped(expression);
//                return true;
//            }
//        }
        default:
            return 0;

    }
}

bool inline_variables(jd_method *m)
{
    bool inlined = false;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);

        if (exp_is_nopped(expression) ||
            !exp_is_assignment(expression) ||
            !assignment_exp_can_inline(expression))
            continue;

        bool tmp = false;
        for (int j = i+1; j < m->expressions->size; ++j) {
            jd_exp *other = lget_obj(m->expressions, j);
            if (exp_is_nopped(other))
                continue;

            tmp = inline_variable_for_expression(expression, other);
            if (tmp)
                break;
        }
        inlined |= tmp;
    }
    return inlined;
}

bool inline_variables_v2(jd_method *m)
{
    int inlined = 0;

    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (!basic_block_is_normal_live(block))
            continue;

        jd_node *node = block->node;
        for (int j = node->start_idx; j <= node->end_idx ; ++j) {
            jd_exp *expression = lget_obj(m->expressions, j);
            if (exp_is_nopped(expression) ||
                !exp_is_assignment(expression) ||
                !assignment_exp_can_inline(expression))
                continue;
            bool tmp = false;
            for (int k = j+1; k < node->end_idx; ++k) {
                jd_exp *other = lget_obj(m->expressions, k);
                if (exp_is_nopped(other))
                    continue;
                tmp = inline_variable_for_expression(expression, other);
                if (tmp)
                    break;
            }
            inlined |= tmp;
        }
    }
    return inlined;
#if 0
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);
        if (exp_is_nopped(expression) ||
            !exp_is_assignment(expression) ||
            !assignment_exp_can_inline_round2(expression))
            continue;

        bool tmp = false;
        for (int j = i; j < m->expressions->size; ++j) {
            jd_exp *other = lget_obj(m->expressions, j);
            if (exp_is_nopped(other))
                continue;

            tmp = inline_variable_for_expression(expression, other);
            if (tmp)
                break;
        }
        inlined |= tmp;
    }
    return inlined;
#endif
}

int inline_variables_round2(jd_method *m)
{
    int inlined = 0;
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);

        if (exp_is_nopped(expression) ||
            !exp_is_assignment(expression) ||
            !assignment_exp_can_inline_round2(expression))
            continue;

        int tmp = 0;
        for (int j = i; j < m->expressions->size; ++j) {
            jd_exp *other = lget_obj(m->expressions, j);
            if (exp_is_nopped(other))
                continue;

            tmp = inline_variable_for_expression(expression, other);
        }
        inlined |= tmp;
    }
    return inlined;
}

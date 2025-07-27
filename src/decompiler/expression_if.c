#include "common/str_tools.h"
#include "decompiler/expression_if.h"
#include "decompiler/expression_helper.h"
#include "decompiler/expression.h"
#include "expression_logical.h"

static jd_operator operator_logical_not(jd_operator op)
{
    switch (op) {
        case JD_OP_EQ: return JD_OP_NE;
        case JD_OP_NE: return JD_OP_EQ;
        case JD_OP_GT: return JD_OP_LE;
        case JD_OP_GE: return JD_OP_LT;
        case JD_OP_LT: return JD_OP_GE;
        case JD_OP_LE: return JD_OP_GT;
        default: return JD_OP_UNKNOWN;
    }
}

void negative_if_expression(jd_method *m)
{
    // java bytecode's if statement is negative, reverse logical operator
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_is_if(exp))
            continue;
        jd_exp_if *if_exp = exp->data;
        jd_exp *condition = if_exp->expression;

        if (condition->type == JD_EXPRESSION_SINGLE_LIST) {
            jd_exp_single_list *single_list = condition->data;
            jd_exp_list *exp_list = single_list->list;

            jd_exp_single_operator *sop = make_obj(jd_exp_single_operator);
            sop->list = exp_list;
            sop->operator = JD_OP_LOGICAL_NOT;
        }
        else if (condition->type == JD_EXPRESSION_SINGLE_OPERATOR) {
            jd_exp_single_operator *single_op = condition->data;
            jd_exp_list *exp_list = single_op->list;

            condition->type = JD_EXPRESSION_SINGLE_LIST;
            jd_exp_single_list *single_list = make_obj(jd_exp_single_list);
            single_list->list = exp_list;
            condition->data = single_list;
        }
        else {
            jd_exp_operator *op = condition->data;
            op->operator = operator_logical_not(op->operator);
        }
    }
}

void identify_boolean_in_if(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_is_if(exp) || exp_is_nopped(exp))
            continue;
        jd_ins *if_ins = exp->ins;
        if (if_ins == NULL ||
            jvm_ins_is_ifnonnull(if_ins) ||
            jvm_ins_is_ifnull(if_ins))
            continue;


        jd_exp_if *if_exp = exp->data;
        jd_exp *if_op = if_exp->expression;
        if (!exp_is_operator(if_op))
            continue;
        jd_exp_operator *op = if_op->data;
        jd_exp *left = &op->list->args[0];
        jd_exp *right = &op->list->args[1];

        if (op->operator == JD_OP_NE &&
            exp_is_const(right)) {
            jd_exp_const *const_exp = right->data;
            if (const_exp->val->data->primitive == NULL)
                continue;
            if (const_exp->val->data->primitive->int_val == 0)
                if_exp->expression = left;
        }
        else if (op->operator == JD_OP_EQ &&
                exp_is_const(right)) {
            jd_exp_const *const_exp = right->data;
            if (const_exp->val->data->primitive == NULL)
                continue;

        }
    }
}

void identify_if_break_or_if_continue(jd_method *m)
{
    if (m->loops == NULL)
        return;
    for (int i = 0; i < m->loops->size; ++i) {
        jd_loop *loop = lget_obj(m->loops, i);
        jd_exp *last = get_exp(m, loop->end_idx);

        if (!exp_is_if(last) || exp_is_nopped(last))
            continue;

        if (loop->is_post_condition && loop->can_write_condition)
            continue;

        jd_exp_if *if_exp = last->data;
        uint32_t target_offset = if_exp->offset;
        // negative_if_exp
        last->type = JD_EXPRESSION_IF_BREAK;

    }
}

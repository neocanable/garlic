#include "decompiler/expression_ternary.h"
#include "decompiler/control_flow.h"
#include "jvm/jvm_ins.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "decompiler/expression_helper.h"
#include "jvm/jvm_expression_builder.h"

static bool ternary_on_basic_block(jd_method *m, jd_bblock *block)
{
    bool result = false;
    jd_node *node = block->node;
    jd_exp *exp = get_exp(m, node->end_idx);
    jd_ins *ins = exp->ins;
    if (ins == NULL)
        return result;

    if (ins_is_copy_if_true_block(ins))
        return result;

    if (!exp_is_alive_if(exp))
        return result;

    if (jvm_ins_is_goto_back(ins))
        return result;

    jd_exp_if *if_exp = exp->data;
    jd_exp *true_exp = exp_of_offset(m, if_exp->offset);
//    jd_exp *false_exp = ins->next->expression;
    jd_exp *false_exp = get_exp(m, exp->idx + 1);

    jd_bblock *true_block = exp_block(true_exp);
    jd_bblock *false_block = exp_block(false_exp);

    jd_exp *true_valid_exp = true_exp;
    jd_exp *false_valid_exp = false_exp;
    if (exp_is_nopped(true_exp))
        true_valid_exp = next_valid_exp(m, true_exp->idx + 1);
    if (exp_is_nopped(false_exp))
        false_valid_exp = next_valid_exp(m, false_valid_exp->idx + 1);

    if (true_block == NULL || false_block == NULL)
        return result;
//    if (true_block->in->size > 1 ||
//        false_block->in->size > 1)
//        return result;
//
//    if (lcontains_obj(block->frontier, block))
//        return result;

    if (true_valid_exp == NULL || false_valid_exp == NULL)
        return result;

    if (!basic_block_contains_exp(true_block, true_valid_exp) ||
        !basic_block_contains_exp(false_block, false_valid_exp))
        return result;

    jd_var *true_var = NULL;
    jd_var *false_var = NULL;
    jd_exp *true_val_exp = NULL;
    jd_exp *false_val_exp = NULL;

    if (exp_is_define_stack_var(true_valid_exp)) {
        jd_exp_def_var *true_def = true_valid_exp->data;
        true_var = (&true_def->list->args[0])->data;
        true_val_exp = &true_def->list->args[1];
    }

    if (exp_is_define_stack_var(false_valid_exp)) {
        jd_exp_def_var *false_def = false_valid_exp->data;
        false_var = (&false_def->list->args[0])->data;
        false_val_exp = &false_def->list->args[1];
    }

    if (exp_is_assignment(true_valid_exp)) {
        jd_exp_assignment *ori_ass_exp = true_valid_exp->data;
        true_var = ((jd_exp_lvalue*)ori_ass_exp->left->data)->stack_var;
        true_val_exp = ori_ass_exp->right;
    }

    if (exp_is_assignment(false_valid_exp)) {
        jd_exp_assignment *ori_ass_exp = false_valid_exp->data;
        false_var = ((jd_exp_lvalue*)ori_ass_exp->left->data)->stack_var;
        false_val_exp = ori_ass_exp->right;
    }

    if (exp_is_assignment_chain(true_valid_exp) &&
        exp_is_assignment_chain(false_valid_exp)) {
        // make sure true left == false_left
        jd_exp_assignment_chain *true_chain = true_valid_exp->data;
        jd_exp_assignment_chain *false_chain = false_valid_exp->data;
        true_val_exp = true_chain->right;
        false_val_exp = false_chain->right;
    }

    if (exp_is_assignment_chain(true_valid_exp) &&
        exp_is_assignment_chain(false_valid_exp)) {
        // chain left equals
        jd_exp_ternary *ternary_exp = make_obj(jd_exp_ternary);
        ternary_exp->list = make_exp_list(3);
        jd_exp *condition = &ternary_exp->list->args[0];
        jd_exp *ternary_true_val_exp = &ternary_exp->list->args[1];
        jd_exp *ternary_false_val_exp = &ternary_exp->list->args[2];

        condition->type = if_exp->expression->type;
        condition->data = if_exp->expression->data;
        condition->ins  = if_exp->expression->ins;

        memcpy(ternary_true_val_exp, true_val_exp, sizeof(jd_exp));
        memcpy(ternary_false_val_exp, false_val_exp, sizeof(jd_exp));

        jd_exp *out_expression = make_obj(jd_exp);
        out_expression->type = JD_EXPRESSION_TERNARY;
        out_expression->data = ternary_exp;

        jd_exp_assignment_chain *true_chain = true_valid_exp->data;
        true_chain->right = out_expression;
        int from_id = exp->idx;
        int to_id = true_valid_exp->idx - 1;
        exp_mark_nopped_of_range(m, from_id, to_id);
        result = true;
        DEBUG_TERNARY_PRINT("[ternary] find ternary operator:\n");
        DEBUG_TERNARY_PRINT("[ternary] found a ternary true_exp: %d, "
                            "false_exp: %d\n",
                            true_valid_exp->idx,
                            false_valid_exp->idx);
        for (int k = 0; k < true_chain->left->size; ++k) {
            jd_exp *e = lget_obj(true_chain->left, k);
            if (exp_is_stack_var(e)) {
                jd_var *var = e->data;
                var->def_count --;
            }
        }
    }
    else if (exp_is_save(true_valid_exp) &&
            exp_is_save(false_valid_exp) &&
            exp_saved_same(true_valid_exp, false_valid_exp)) {
        jd_exp *true_save_right = exp_saved_value(true_valid_exp);
        jd_exp *false_save_right = exp_saved_value(false_valid_exp);

        jd_exp_ternary *ternary_exp = make_obj(jd_exp_ternary);
        ternary_exp->list = make_exp_list(3);
        jd_exp *condition = &ternary_exp->list->args[0];
        jd_exp *ternary_true_val_exp = &ternary_exp->list->args[1];
        jd_exp *ternary_false_val_exp = &ternary_exp->list->args[2];
        memcpy(condition, if_exp->expression, sizeof(jd_exp));
        memcpy(ternary_true_val_exp, true_save_right, sizeof(jd_exp));
        memcpy(ternary_false_val_exp, false_save_right, sizeof(jd_exp));

        true_save_right->type = JD_EXPRESSION_TERNARY;
        true_save_right->data = ternary_exp;
        int from_id = exp->idx;
        int to_id = true_valid_exp->idx - 1;
        exp_mark_nopped_of_range(m, from_id, to_id);
        result = true;
    }
    else {
        if (true_var != false_var ||
            true_var == NULL ||
            false_var == NULL)
            return result;

        true_var->def_count --;
        jd_exp_ternary *ternary_exp = make_obj(jd_exp_ternary);
        ternary_exp->list = make_exp_list(3);
        jd_exp *condition = &ternary_exp->list->args[0];
        jd_exp *ternary_true_val_exp = &ternary_exp->list->args[1];
        jd_exp *ternary_false_val_exp = &ternary_exp->list->args[2];

        memcpy(condition, if_exp->expression, sizeof(jd_exp));
        memcpy(ternary_true_val_exp, true_val_exp, sizeof(jd_exp));
        memcpy(ternary_false_val_exp, false_val_exp, sizeof(jd_exp));

        jd_exp_assignment *assignment = make_obj(jd_exp_assignment);
        assignment->left = make_obj(jd_exp);
        assignment->left->data = make_obj(jd_exp_lvalue);
        assignment->left->type = JD_EXPRESSION_LVALUE;
        assignment->assign_operator = JD_OP_ASSIGN;
        jd_exp_lvalue *lvalue = assignment->left->data;
        lvalue->stack_var = true_var;
        assignment->def_count ++;

        jd_exp *ternary_expression = make_obj(jd_exp);
        ternary_expression->type = JD_EXPRESSION_TERNARY;
        ternary_expression->data = ternary_exp;
        assignment->right = ternary_expression;

        true_valid_exp->type = JD_EXPRESSION_ASSIGNMENT;
        true_valid_exp->data = assignment;

        int from_id = exp->idx;
        int to_id = true_valid_exp->idx - 1;
        exp_mark_nopped_of_range(m, from_id, to_id);
        result = true;
        DEBUG_PRINT("[ternary] found a ternary true_exp: %d, "
                            "false_exp: %d\n",
                            true_valid_exp->idx,
                            false_valid_exp->idx);
    }
    return result;
}

bool identify_ternary_operator(jd_method *m)
{
    bool find_ternary = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            if (!node_is_basic_block(child))
                continue;

            jd_bblock *block = child->data;
            if (!basic_block_is_normal_live(block))
                continue;

            find_ternary |= ternary_on_basic_block(m, block);
        }
    }
    return find_ternary;
}

static bool ternary_in_if_condition(jd_method *m, jd_bblock *block)
{
    // if (something)
    // if (something)
    // goto
    // if (something)

    bool result = false;
    jd_nblock *nblock = block->ub->nblock;
    jd_exp *first_exp = get_exp(m, nblock->end_idx);

    if (!exp_is_alive_if(first_exp))
        return result;

    jd_exp *second_exp = next_valid_exp(m, first_exp->idx + 1);
    if (!exp_is_alive_if(second_exp))
        return result;

    jd_exp *third_exp = next_valid_exp(m, second_exp->idx + 1);
    if (!exp_is_alive_goto(third_exp))
        return result;

    jd_exp *fourth_exp = next_valid_exp(m, third_exp->idx + 1);
    if (!exp_is_alive_if(fourth_exp))
        return result;

    jd_exp_if *first_if = first_exp->data;
    jd_exp_if *second_if = second_exp->data;
    jd_exp_if *fourth_if = fourth_exp->data;

    jd_exp *first_true_exp = exp_of_offset(m, first_if->offset);
    if (exp_is_nopped(first_true_exp))
        first_true_exp = next_valid_exp(m, first_true_exp->idx + 1);

    jd_exp *second_true_exp = exp_of_offset(m, second_if->offset);
    if (exp_is_nopped(second_true_exp))
        second_true_exp = next_valid_exp(m, second_true_exp->idx + 1);

    if (first_true_exp != fourth_exp && second_true_exp != fourth_exp)
        return result;

    jd_exp_ternary *ternary = make_obj(jd_exp_ternary);
    ternary->list = make_exp_list(3);
    jd_exp *condition = &ternary->list->args[0];
    jd_exp *true_exp = &ternary->list->args[1];
    jd_exp *false_exp = &ternary->list->args[2];


    memcpy(condition, first_if->expression, sizeof(jd_exp));
    memcpy(true_exp, fourth_if->expression, sizeof(jd_exp));
    memcpy(false_exp, second_if->expression, sizeof(jd_exp));

    jd_exp *ternary_exp = make_obj(jd_exp);
    ternary_exp->type = JD_EXPRESSION_TERNARY;
    ternary_exp->data = ternary;
    fourth_if->expression = ternary_exp;

    exp_mark_nopped(first_exp);
    exp_mark_nopped(second_exp);
    exp_mark_nopped(third_exp);
    return true;
}

bool identify_ternary_operator_in_condition(jd_method *m)
{
    bool find_ternary = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;


        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            if (!node_is_basic_block(child))
                continue;
            jd_bblock *block = child->data;
            if (!basic_block_is_normal_live(block))
                continue;

            find_ternary |= ternary_in_if_condition(m, block);
        }
    }
    return find_ternary;
}

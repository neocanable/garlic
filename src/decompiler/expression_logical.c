#include "common/str_tools.h"
#include "decompiler/expression_logical.h"
#include "decompiler/control_flow.h"
#include "decompiler/expression.h"
#include "jvm/jvm_expression_builder.h"
#include "expression_writter.h"
#include "dominator_tree.h"

void make_logic_not(jd_exp *expression)
{
    if (exp_is_operator(expression)) {
        jd_exp_operator *exp_operator = expression->data;
        switch (exp_operator->operator) {
            case JD_OP_EQ:
                exp_operator->operator = JD_OP_NE;
                break;
            case JD_OP_NE:
                exp_operator->operator = JD_OP_EQ;
                break;
            case JD_OP_LT:
                exp_operator->operator = JD_OP_GE;
                break;
            case JD_OP_LE:
                exp_operator->operator = JD_OP_GT;
                break;
            case JD_OP_GT:
                exp_operator->operator = JD_OP_LE;
                break;
            case JD_OP_GE:
                exp_operator->operator = JD_OP_LT;
                break;
            case JD_OP_LOGICAL_AND:
                exp_operator->operator = JD_OP_LOGICAL_OR;
                break;
            case JD_OP_LOGICAL_OR:
                exp_operator->operator = JD_OP_LOGICAL_AND;
                break;
            default:
                break;
        }
    }
    else {
        jd_exp_operator *op = make_obj(jd_exp_operator);
        op->operator = JD_OP_LOGICAL_NOT;
        op->list = make_exp_list(1);
        memcpy(&op->list->args[0], expression, sizeof(jd_exp));
        expression->type = JD_EXPRESSION_SINGLE_OPERATOR;
        expression->data = op;
    }
}

bool identify_logical_operations(jd_method *m)
{
    bool find_short_logical_operator = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            if (!node_is_basic_block(child))
                continue;

            jd_exp *exp = get_exp(m, child->end_idx);
            // assert生成的if表达式不合并
            if (!exp_is_if(exp) ||
                exp_is_nopped(exp) ||
                if_expression_is_assert(exp))
                continue;

            jd_bblock *basic_block = exp_block(exp);
            jd_exp_if *if_exp = exp->data;
            uint32_t true_offset = if_exp->offset;
            jd_exp *current_true_exp = exp_of_offset(m, true_offset);
            jd_exp *current_false_exp = exp->ins->next->expression;

            for (int k = 0; k < 2; ++k) {
                jd_bblock *next = k == 0 ?
                                  exp_block(current_true_exp) :
                                  exp_block(current_false_exp);
                jd_bblock *other = k == 0 ?
                                   exp_block(current_false_exp) :
                                   exp_block(current_true_exp);

                int negate = k == 1;

                if (next == basic_block || next == NULL)
                    continue;
                jd_nblock *nnext = next->ub->nblock;
                jd_ins *next_end_ins = get_ins(m, nnext->end_idx);
                jd_exp *next_exp = next_end_ins->expression;
//                jd_exp *next_exp = get_exp(m, nnext->end_idx);
                if (!exp_is_if(next_exp))
                    continue;
                jd_exp_if *next_if_exp = next_exp->data;
                jd_ins *next_ins = next_exp->ins;

                jd_exp *next_valid = next_valid_exp(m, exp->idx + 1);
                if (next_valid != next_exp)
                    continue;

                jd_bblock *next_block = exp_block(next_exp);

                jd_exp *next_true_exp = exp_of_offset(m, next_if_exp->offset);
                jd_exp *next_false_exp = next_ins->next->expression;
                jd_bblock *next_t = exp_block(next_true_exp);
                jd_bblock *next_f = exp_block(next_false_exp);

                if (other != next_t && other != next_f)
                    continue;

                jd_exp_operator *op = make_obj(jd_exp_operator);
                op->list = make_exp_list(2);
                memcpy(&op->list->args[1],
                       next_if_exp->expression,
                       sizeof(jd_exp));
                next_if_exp->expression->type = JD_EXPRESSION_OPERATOR;
                next_if_exp->expression->data = op;

                jd_exp *first = &op->list->args[0];
                if (other == next_f) {
                    op->operator = JD_OP_LOGICAL_OR;
                    if (negate)
                        make_logic_not(if_exp->expression);
                    memcpy(first,
                           if_exp->expression,
                           sizeof(jd_exp));
                }
                else {
                    op->operator = JD_OP_LOGICAL_AND;
                    if (!negate)
                        make_logic_not(if_exp->expression);
                    memcpy(first,
                           if_exp->expression,
                           sizeof(jd_exp));
                }

                // TODO: 这里需要unlink_blocks(basic_block, target)
                       compute_dominates_block(m, basic_block);
                       compute_dominates_block(m, next_block);
                cfg_unlink_blocks(basic_block, other);

                for (int l = 0; l < basic_block->dominates->size; ++l) {
                    jd_bblock *tmp = lget_obj(basic_block->dominates, l);
                    ladd_obj_no_dup(next_block->dominates, tmp);
                }

//                if (original_next_f != next_f) {
//                    jd_exp_goto *goto_exp = next_false_exp->data;
//                    next_if_exp->goto_offset = goto_exp->goto_offset;
//                    exp_mark_nopped(next_false_exp);
//                }

                find_short_logical_operator = true;
                exp_mark_nopped(exp);
            }
        }
    }

    return find_short_logical_operator;
}

bool identify_reverse_logical_operation(jd_method *m)
{
    bool find_short_logical_operator = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            if (!node_is_basic_block(child))
                continue;
            jd_exp *exp = get_exp(m, node->end_idx);
            jd_ins *ins = exp->ins;
            jd_bblock *basic_block = child->data;
            if (!basic_block_is_normal_live(basic_block))
                continue;

            // assert生成的if表达式不合并
            if (!exp_is_if(exp) ||
                exp_is_nopped(exp) ||
                if_expression_is_assert(exp))
                continue;

            jd_exp_if *if_exp = exp->data;
            jd_exp *current_true_exp = exp_of_offset(m, if_exp->offset);
            jd_exp *current_false_exp = ins->next->expression;

            for (int k = 0; k < 2; ++k) {
                jd_bblock *next = k == 0 ?
                                  exp_block(current_true_exp) :
                                  exp_block(current_false_exp);
                jd_bblock *other = k == 0 ?
                                   exp_block(current_false_exp) :
                                   exp_block(current_true_exp);

                int negate = k == 1;

                if (next == basic_block || next == NULL)
                    continue;
                jd_nblock *nnext = next->ub->nblock;
                jd_exp *next_exp = get_exp(m, nnext->end_idx);
                if (!exp_is_if(next_exp) || exp_is_nopped(next_exp))
                    continue;
                jd_exp_if *next_if_exp = next_exp->data;
                jd_ins *next_ins = next_exp->ins;

                jd_exp *next_valid = next_valid_exp(m, exp->idx + 1);
                if (next_valid == next_exp)
                    continue;

                jd_exp *next_true_exp = exp_of_offset(m, next_if_exp->offset);
                jd_exp *next_false_exp = next_ins->next->expression;

                jd_bblock *next_t = exp_block(next_true_exp);
                jd_bblock *next_f = exp_block(next_false_exp);
                if (other != next_f)
                    continue;


                jd_exp_operator *op = make_obj(jd_exp_operator);
                op->list = make_exp_list(2);
                memcpy(&op->list->args[1], if_exp->expression, sizeof(jd_exp));
                if_exp->expression->type = JD_EXPRESSION_OPERATOR;
                if_exp->expression->data = op;
                jd_bblock *next_block = exp_block(next_exp);

                if (other == next_f) {
                    op->operator = JD_OP_LOGICAL_OR;
                    if (negate)
                        make_logic_not(if_exp->expression);
                    memcpy(&op->list->args[0],
                           next_if_exp->expression,
                           sizeof(jd_exp));
                }
                else {
                    op->operator = JD_OP_LOGICAL_AND;
                    if (!negate)
                        make_logic_not(if_exp->expression);
                    memcpy(&op->list->args[0],
                           next_if_exp->expression,
                           sizeof(jd_exp));
                }

                for (int l = 0; l < next_block->dominates->size; ++l) {
                    jd_bblock *tmp = lget_obj(next_block->dominates, l);
                    ladd_obj_no_dup(basic_block->dominates, tmp);
                }

                find_short_logical_operator = true;
                exp_mark_nopped(next_exp);
            }
        }

    }

    return find_short_logical_operator;
}

void identify_cmp_after_if(jd_method *m)
{
    // ins: lcmp, fcmpg, fcmpl, dcmpg, dcmpl
    // next: ifne, ifge, ifeq, iflt, ifgt, ifle
    //
    // stack is: ... v1, v2 ->
    // lcmp  v1 > v2 push 1
    // lcmp  v1 < v2 push -1
    // lcmp  v1 = v2 push 0
    // dcmpg/l v1 > v2 push 1
    // dcmpg/l v1 < v2 push -1
    // dcmpg/l v1 = v2 push 0
    // fcmpg/l v1 > v2 push 1
    // fcmpg/l v1 < v2 push -1
    // fcmpg/l v1 = v2 push 0

    // ifne == if v1 != v2
    // ifeq == if v1 == v2
    // iflt == if v1 < v2
    // ifgt == if v1 > v2
    // ifle == if v1 <= v2
    // ifgt == if v1 >= v2
    for (int i = 0; i < m->expressions->size - 1; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        jd_ins *ins = exp->ins;
        jd_ins_fn *fn = ins->fn;
        if (!fn->is_compare(ins))
            continue;
        jd_exp *second_exp = lget_obj(m->expressions, i + 1);
        if (!exp_is_if(second_exp))
            continue;
        jd_exp_if *if_exp = second_exp->data;
        jd_exp *other = if_exp->expression;
        jd_exp_operator *operator_exp = other->data;

        jd_exp *right = NULL;
        if (exp_is_store(exp)) {
            jd_exp_store *store = exp->data;
            right = &store->list->args[1];
        }
        else if (exp_is_assignment(exp)) {
            jd_exp_assignment *assignment = exp->data;
            right = assignment->right;
        }
        else {
            abort();
        }
        jd_exp_operator *right_exp = right->data;
        operator_exp->list->args[0] = right_exp->list->args[0];
        operator_exp->list->args[1] = right_exp->list->args[1];
        exp_mark_nopped(exp);
        i += 1;

//        switch (ins->next->code) {
//            case INS_IFNE:
//                operator_exp->operator = JD_OP_NE;
//                break;
//            case INS_IFEQ:
//                operator_exp->operator = JD_OP_EQ;
//                break;
//            case INS_IFLT:
//                operator_exp->operator = JD_OP_LT;
//                break;
//            case INS_IFGT:
//                operator_exp->operator = JD_OP_GT;
//                break;
//            case INS_IFLE:
//                operator_exp->operator = JD_OP_LE;
//                break;
//            case INS_IFGE:
//                operator_exp->operator = JD_OP_GE;
//                break;
//            default:
//                fprintf(stderr, "unknown if compare: %d\n", ins->code);
//                break;
//        }
    }
}

bool identify_logical_with_assignment(jd_method *m)
{
    bool find_short_logical_with_assignment = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        for (int j = 0; j < node->children->size; ++j)
        {
            jd_node *child = lget_obj(node->children, j);
            if (!node_is_basic_block(child))
                continue;

            jd_bblock *basic_block = child->data;
            if (!basic_block_is_normal_live(basic_block))
                continue;

            jd_exp *exp = get_exp(m, child->end_idx);
            jd_ins *ins = exp->ins;
            // assert生成的if表达式不合并
            if (!exp_is_if(exp) ||
                exp_is_nopped(exp) ||
                if_expression_is_assert(exp))
                continue;

            jd_exp_if *if_exp = exp->data;
            jd_exp *current_true_exp = exp_of_offset(m, if_exp->offset);
            jd_exp *current_false_exp = ins->next->expression;

            for (int k = 0; k < 2; ++k) {
                jd_bblock *next = k == 0 ?
                                  exp_block(current_true_exp) :
                                  exp_block(current_false_exp);
                jd_bblock *other = k == 0 ?
                                   exp_block(current_false_exp) :
                                   exp_block(current_true_exp);

                if (next == basic_block || next == NULL)
                    continue;
                jd_nblock *nnext = next->ub->nblock;
                jd_exp *next_exp = get_exp(m, nnext->end_idx);
                if (!exp_is_if(next_exp))
                    continue;
                jd_exp_if *next_if_exp = next_exp->data;
                jd_ins *next_ins = next_exp->ins;

                jd_exp *next_true_exp = exp_of_offset(m, next_if_exp->offset);
                jd_exp *next_false_exp = next_ins->next->expression;

                jd_bblock *next_t = exp_block(next_true_exp);
                jd_bblock *next_f = exp_block(next_false_exp);

                if (other != next_f)
                    continue;

                if (next_valid_exp(m, exp->idx + 1) == next_exp)
                    continue;


                jd_exp *condition = next_if_exp->expression;
                bool is_logical_not = false;
                if (exp_is_operator(condition)) {
                    jd_exp_operator *op = condition->data;
                    jd_exp *op_left = &op->list->args[0];
                    jd_exp *op_right = &op->list->args[1];
                    if (exp_is_local_variable(op_left) ||
                        exp_is_get_field(op_left) ||
                        exp_is_get_static(op_left) ||
                        exp_is_array_load(op_left)) {
                        is_logical_not = true;
                    }
                    if (exp_is_local_variable(op_right) ||
                        exp_is_get_field(op_right) ||
                        exp_is_get_static(op_right) ||
                        exp_is_array_load(op_right)) {
                        is_logical_not = true;
                    }
                }
                else if (exp_is_local_variable(condition) ||
                         exp_is_get_field(condition) ||
                         exp_is_get_static(condition) ||
                         exp_is_array_load(condition)) {
                    is_logical_not = true;
                }
                if (!is_logical_not) {
                    continue;
                }

                jd_exp_assignment_chain *chain = NULL;
                int next_idx = next_exp->idx + 1;
                jd_exp *_iterator_exp = next_valid_exp(m, next_idx);
                while (_iterator_exp != NULL &&
                       exp_is_save(_iterator_exp)) {
                    jd_exp *left = exp_saved_left(_iterator_exp);
                    jd_exp *value = exp_saved_value(_iterator_exp);

                    if (chain == NULL) {
                        chain = make_obj(jd_exp_assignment_chain);
                        chain->left = linit_object();
                        ladd_obj(chain->left, left);
                        chain->right = value;
                    } else {
                        ladd_obj(chain->left, left);
                    }
                    exp_mark_nopped(_iterator_exp);

                    next_idx = _iterator_exp->idx + 1;
                    _iterator_exp = next_valid_exp(m, next_idx);
                    if (_iterator_exp == NULL ||
                        _iterator_exp == next_exp)
                        break;
                }

                if (_iterator_exp == next_exp && chain != NULL) {
                    jd_exp_if *t_next_if_exp = next_exp->data;
                    jd_exp *t_condition = t_next_if_exp->expression;
                    if (exp_is_operator(t_condition)) {
                        jd_exp_operator *op = t_condition->data;
                        jd_exp *op_left = &op->list->args[0];
                        jd_exp *op_right = &op->list->args[1];
                        if (exp_is_local_variable(op_left) ||
                            exp_is_get_field(op_left) ||
                            exp_is_get_static(op_left) ||
                            exp_is_array_load(op_left)) {
                            op_left->type = JD_EXPRESSION_ASSIGNMENT_CHAIN;
                            op_left->data = chain;
                            find_short_logical_with_assignment = true;
                        }
                        if (exp_is_local_variable(op_right) ||
                            exp_is_get_field(op_right) ||
                            exp_is_get_static(op_right) ||
                            exp_is_array_load(op_right)) {
                            op_right->type = JD_EXPRESSION_ASSIGNMENT_CHAIN;
                            op_right->data = chain;
                            find_short_logical_with_assignment = true;
                        }
                    }
                    else if (exp_is_local_variable(t_condition) ||
                             exp_is_get_field(t_condition) ||
                             exp_is_get_static(t_condition) ||
                             exp_is_array_load(t_condition)) {
                        t_condition->type = JD_EXPRESSION_ASSIGNMENT_CHAIN;
                        t_condition->data = chain;
                        find_short_logical_with_assignment = true;
                    }
                }
            }
        }
    }
    return find_short_logical_with_assignment;
}

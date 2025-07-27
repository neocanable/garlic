#include "decompiler/expression_chain.h"
#include "decompiler/expression.h"
#include "decompiler/control_flow.h"
#include "decompiler/expression_inline.h"
#include "jvm/jvm_expression_builder.h"
#include "expression_local_variable.h"

static void need_insert_initializer(jd_method *m,
                                    jd_exp *saved_exp,
                                    jd_exp *local_variable_exp,
                                    jd_node *node)
{
    jd_exp *next = get_exp(m, saved_exp->idx + 1);
    if (next != NULL && exp_is_monitor_enter(next))
        return;
}

static bool identify_assignment_chain_of_basic_block(jd_method *m,
                                                     jd_node *node,
                                                     jd_bblock *basic_block)
{
    bool result = false;

    for (int k = node->start_idx; k <= node->end_idx; ++k) {
        jd_exp *exp = lget_obj(m->expressions, k);
        if (exp_is_nopped(exp) || !exp_is_assignment(exp))
            continue;

        jd_exp_assignment *assignment = exp->data;
        jd_exp_lvalue *lvalue = assignment->left->data;
        jd_var *def_var = lvalue->stack_var;

        DEBUG_PRINT("[assignment chain]stack_var %s"
                    " use_count: %d - store_count %d = %d\n",
                    def_var->name,
                    def_var->use_count,
                    def_var->store_count,
                    def_var->use_count - def_var->store_count);

        if (stack_var_can_inline(def_var)) {
            continue;
        }
        if (def_var->dupped_count == 0) {
            continue;
        }

        jd_exp *next_exp = next_valid_exp(m, k + 1);

        while (next_exp != NULL && next_exp->block == basic_block
               ) {
            if (!exp_is_save(next_exp))
                break;

            if (def_var->dupped_count == 0)
                break;

            jd_exp *value_exp = exp_saved_value(next_exp);
            jd_exp *left_exp = exp_saved_left(next_exp);

            if (!exp_is_stack_var(value_exp))
                break;
            jd_var *value_var = value_exp->data;
            if (value_var != def_var)
                break;

            need_insert_initializer(m, next_exp, left_exp, node);

            jd_exp_operator *operator = make_obj(jd_exp_operator);
            operator->operator = JD_OP_ASSIGN;
            operator->list = make_exp_list(2);
            jd_exp *op0 = &operator->list->args[0];
            jd_exp *op1 = &operator->list->args[1];

            memcpy(op0, left_exp, sizeof(jd_exp));
            memcpy(op1, assignment->right, sizeof(jd_exp));
            DEBUG_PRINT("[assignment chain] founded assignment inline :"
                        "%s idx: %d use: %d store: %d def: %d\n",
                        def_var->name,
                        def_var->idx,
                        def_var->use_count,
                        def_var->store_count,
                        def_var->def_count);
            def_var->use_count --;
            def_var->def_count --;
            def_var->store_count --;
            def_var->dupped_count --;


            assignment->right->type = JD_EXPRESSION_OPERATOR;
            assignment->right->data = operator;
            exp_mark_nopped(next_exp);
            result = true;
            next_exp = next_valid_exp(m, next_exp->idx + 1);
        }
    }
    return result;
}


static bool identify_assignment_chain_of_node(jd_method *m, jd_node *node)
{
    bool result = false;
    for (int j = 0; j < node->children->size; ++j) {
        jd_node *child = lget_obj(node->children, j);
        if (!node_is_basic_block(child))
            continue;

        jd_bblock *b = child->data;
        if (!basic_block_is_normal_live(b))
            continue;
        result |= identify_assignment_chain_of_basic_block(m, node, b);
    }
    return result;
}

bool identify_assignment_chain(jd_method *m)
{
    // assignment chain int a,b,c
    // a = b = c = 0;
    // all of them are store local variable, not define new stack variable
    bool result = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        result |= identify_assignment_chain_of_node(m, node);
    }
    return result;
}


static bool assignment_chain_can_be_assignment(jd_exp_assignment_chain *c)
{
    int stack_var_size = 0;
    for (int i = 0; i < c->left->size; ++i) {
        jd_exp *e = lget_obj(c->left, i);
        if (exp_is_stack_var(e))
            stack_var_size ++;
    }
    return stack_var_size == 1;
}

static jd_exp* chain_left_stack_var(jd_exp_assignment_chain *c)
{
    for (int i = 0; i < c->left->size; ++i) {
        jd_exp *e = lget_obj(c->left, i);
        if (exp_is_stack_var(e)) 
            return e;
    }
    return NULL;
}

bool identify_assignment_chain_store(jd_method *m)
{
    bool store_in_chain = false;
    for (int i = 0; i < m->assignment_chains->size; ++i)
    {
        int idx = lget_int(m->assignment_chains, i);
        jd_exp *expression = lget_obj(m->expressions, idx);
        if (exp_is_nopped(expression) ||
            !exp_is_assignment_chain(expression))
            continue;
        jd_exp_assignment_chain *assignment_chain = expression->data;
        for (int j = 0; j < assignment_chain->left->size; ++j) {
            jd_exp *exp = lget_obj(assignment_chain->left, j);
            if (!exp_is_stack_var(exp)) 
                continue;
            jd_var *var = exp->data;
            DEBUG_PRINT("[inlining stack var]: %s def: %d use: %d store: %d\n",
                        var->name,
                        var->def_count,
                        var->use_count,
                        var->store_count);
            for (int k = idx + 1; k < m->expressions->size; ++k) {
                jd_exp *other = lget_obj(m->expressions, k);
                if (exp_is_nopped(other) ||
                    !exp_is_save(other))
                    continue;
                jd_exp *stored_value_exp = exp_saved_value(other);
                jd_exp *stored_left_exp = exp_saved_left(other);
                if (!exp_is_stack_var(stored_value_exp)) 
                    continue;
                jd_var *stored_var = stored_value_exp->data;
                if (stored_var != var) 
                    continue;
                exp->type = stored_left_exp->type;
                exp->data = stored_left_exp->data;
                var->def_count --;
                if (var->def_count == 0) {
                    var->store_count--;
                    exp_mark_nopped(other);
                }
                break;
            }
        }

        if (assignment_chain_can_be_assignment(assignment_chain)) {
            jd_exp *stack_var_left = chain_left_stack_var(assignment_chain);
            jd_var *left_var = stack_var_left->data;

            jd_exp_assignment *assignment = make_obj(jd_exp_assignment);
            assignment->left = make_obj(jd_exp);
            assignment->left->data = make_obj(jd_exp_lvalue);
            assignment->left->type = JD_EXPRESSION_LVALUE;
            assignment->assign_operator = JD_OP_ASSIGN;
            jd_exp_lvalue *lvalue = assignment->left->data;
            lvalue->stack_var = left_var;
            jd_exp *right = assignment_chain->right;

            for (int j = 0; j < assignment_chain->left->size; ++j) {
                jd_exp *exp = lget_obj(assignment_chain->left, j);
                if (exp_is_stack_var(exp)) 
                    continue;
                jd_exp *new_right = make_obj(jd_exp);
                new_right->type = JD_EXPRESSION_OPERATOR;
                new_right->data = make_obj(jd_exp_operator);
                jd_exp_operator *op = new_right->data;
                op->operator = JD_OP_ASSIGN;
                op->list = make_exp_list(2);
                memcpy(&op->list->args[0], exp, sizeof(jd_exp));
                memcpy(&op->list->args[1], right, sizeof(jd_exp));
                right = new_right;
            }
            assignment->right = right;
            expression->type = JD_EXPRESSION_ASSIGNMENT;
            expression->data = assignment;
            store_in_chain = true;
            ldel_int(m->assignment_chains, expression->idx);
            i--;
        }
    }
    return store_in_chain;
}

bool identify_define_stack_variable_chain(jd_method *m)
{
    bool find_define_stack_variable_chain = false;
    if (m->assignment_chains == NULL)
        m->assignment_chains = linit_int();

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            if (!node_is_basic_block(child))
                continue;

            jd_bblock *basic_block = child->data;
            if (!basic_block_is_normal_live(basic_block))
                continue;
            jd_nblock *nblock = basic_block->ub->nblock;

            for (int k = child->start_idx; k <= child->end_idx; ++k) {
                jd_exp *exp = lget_obj(m->expressions, k);
                if (exp_is_nopped(exp) || !exp_is_assignment(exp))
                    continue;
                jd_exp_assignment *assignment = exp->data;
                jd_exp_lvalue *lvalue = assignment->left->data;
                jd_var *def_var = lvalue->stack_var;
                if (def_var->def_count != 1 || def_var->use_count <= 1)
                    continue;
                if (def_var->use_count > def_var->redef_count)
                    continue;

                jd_exp *next_exp = next_valid_exp(m, k + 1);
                jd_exp_assignment_chain *chain = NULL;

                while (next_exp != NULL &&
                       basic_block_contains_exp(basic_block, next_exp) &&
                       exp_is_define_stack_var(next_exp) ) {

                    jd_exp_def_var *def_exp = next_exp->data;
                    jd_exp *left_exp = &def_exp->list->args[0];
                    jd_var *left_var = left_exp->data;
                    jd_exp *value_exp = &def_exp->list->args[1];
                    jd_var *value_var = value_exp->data;

                    if (value_var != def_var)
                        break;

                    if (chain == NULL) {
                        chain = make_obj(jd_exp_assignment_chain);
                        chain->left = linit_object();
                        chain->right = make_obj(jd_exp);
                        chain->right->type = assignment->right->type;
                        chain->right->data = assignment->right->data;
                        chain->right->ins = assignment->right->ins;
                    }

                    jd_exp *stack_var_exp = make_obj(jd_exp);
                    stack_var_exp->type = JD_EXPRESSION_STACK_VAR;
                    stack_var_exp->data = left_var;
                    ladd_obj(chain->left, stack_var_exp);

                    debug_stack_var_info(def_var);
                    // make inline assignment chain

                    exp_mark_nopped(next_exp);
                    find_define_stack_variable_chain = true;
                    def_var->redef_count --;
                    def_var->use_count --;
                    next_exp = next_valid_exp(m, next_exp->idx + 1);
                }

                if (find_define_stack_variable_chain &&
                    chain != NULL) {
                    exp->type = JD_EXPRESSION_ASSIGNMENT_CHAIN;
                    exp->data = chain;
                    ladd_int(m->assignment_chains, exp->idx);
                    DEBUG_PRINT("[variable chain] chain: %d \n", exp->idx);
                }
            }
        }
    }
    return find_define_stack_variable_chain;
}

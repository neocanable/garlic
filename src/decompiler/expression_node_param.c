#include "decompiler/expression_node_param.h"
#include "decompiler/expression_node.h"

static void setup_first_effective_to_node_param(jd_method *m, jd_node *node)
{
    jd_node *first_child = lget_obj_first(node->children);
    jd_exp *last_exp = get_exp(m, first_child->end_idx);
    node->param_exp = last_exp;
    exp_mark_nopped(last_exp);
}

static void setup_synchronized_node_param(jd_method *m, jd_node *node)
{
//    jd_exp *exp = get_exp(m, node->start_idx - 1);
    jd_exp *exp = prev_valid_exp(m, node->start_idx - 1);
    assert(exp_is_monitor_enter(exp));
    jd_exp_monitorenter *monitorenter = exp->data;
    jd_exp *first = &monitorenter->list->args[0];
    if (exp_is_local_variable(first)) {
        node->param_exp = first;
    }
    else if (exp_is_operator(first)) {
        jd_exp_operator *op = first->data;
        jd_exp *right = &op->list->args[1];
        node->param_exp = right;
    }
}

static void setup_catch_node_param(jd_method *m, jd_node *node)
{
    jd_exp *exp = get_exp(m, node->start_idx);
    if (exp_is_store(exp)) {
        jd_exp_store *exp_store = exp->data;
        jd_exp *left = &exp_store->list->args[0];
        node->param_exp = left;
        exp_mark_nopped(exp);
    } else {
        DEBUG_PRINT("[error]: no catch parameters: %s, node_id: %d\n",
                m->name, node->node_id);
    }
}

static void setup_for_loop_node_param(jd_method *m, jd_node *node)
{
    jd_node *n = lget_obj_first(node->children);
//    jd_node *first = next_valid_node(m, n);
//    jd_exp *exp = first->data;
    jd_exp *exp = get_exp(m, n->end_idx);
    node->param_exp = exp;
    node->type = JD_NODE_FOR;
    exp_mark_nopped(exp);
}

static void setup_while_loop_node_param(jd_method *m, jd_node *node)
{
    jd_node *n = lget_obj_first(node->children);
//    jd_node *first = next_valid_node(m, n);
//    assert(node_is_expression(first));
//    jd_exp *exp = first->data;
    jd_exp *exp = get_exp(m, n->end_idx);
    node->param_exp = exp;
    node->type = JD_NODE_WHILE;
    exp_mark_nopped(exp);
}

static void setup_do_while_node_param(jd_method *m, jd_node *node)
{
    jd_node *n = lget_obj_last(node->children);
//    assert(node_is_expression(n));
//    jd_exp *exp = n->data;
    jd_exp *exp = get_exp(m, n->end_idx);
    node->param_exp = exp;
    exp_mark_nopped(exp);
    node->type = JD_NODE_DO_WHILE;
}

static jd_exp* make_true_exp()
{
    jd_exp *exp = make_obj(jd_exp);
    jd_exp_const *exp_const = make_obj(jd_exp_const);
    exp_const->data = make_obj(jd_val_data);
    exp_const->data->primitive = make_obj(jd_primitive_union);
    exp_const->data->primitive->int_val = 1;
    exp_const->data->cname = (string)g_str_boolean;
    exp->data = exp_const;
    exp->type = JD_EXPRESSION_CONST;
    return exp;
}

void setup_expression_node_param(jd_method *m)
{
    int i;
    for (i = 0; i < m->nodes->size; i++) {
        jd_node *node = lget_obj(m->nodes, i);
        if (!node_need_param(node))
            continue;

        switch (node->type) {
            case JD_NODE_CATCH: {
                setup_catch_node_param(m, node);
                break;
            }
            case JD_NODE_SYNCHRONIZED: {
                setup_synchronized_node_param(m, node);
                break;
            }
            case JD_NODE_LOOP: {
                jd_loop *loop = node->data;
                if (loop->type == JD_LOOP_FOR) {
                    setup_for_loop_node_param(m, node);
                }
                else if (loop->type == JD_LOOP_WHILE) {
                    setup_while_loop_node_param(m, node);
                }
                else if (loop->type == JD_LOOP_DO_WHILE) {
                    setup_do_while_node_param(m, node);
                }
                else {
                    jd_exp *exp = make_true_exp();
                    node->param_exp = exp;
                    // infinite loop
                }
                break;
            }
            default:
                break;
        }
    }

    for (i = 0; i < m->nodes->size; i++) {
        jd_node *node = lget_obj(m->nodes, i);
        if (!node_need_param(node))
            continue;
        switch(node->type) {
            case JD_NODE_ELSE_IF:
            case JD_NODE_SWITCH:
            case JD_NODE_IF: {
                setup_first_effective_to_node_param(m, node);
                break;
            }
            default:
                break;
        }
    }
}

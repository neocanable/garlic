#include "decompiler/expression_node_param.h"
#include "decompiler/expression_node.h"
#include "decompiler/klass.h"
#include "decompiler/stack.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_simulator.h"

static void setup_first_effective_to_node_param(jd_method *m, jd_node *node)
{
    jd_node *first_child = lget_obj_first(node->children);
    jd_exp *last_exp = get_exp(m, first_child->end_idx);
    node->param_exp = last_exp;
    exp_mark_nopped(last_exp);
}

static void setup_synchronized_node_param(jd_method *m, jd_node *node)
{
    jd_exp *exp = prev_valid_exp(m, node->start_idx - 1);
    if (exp == NULL)
        return;
    if (!exp_is_monitor_enter(exp))
        return;

    jd_exp_monitorenter *monitorenter = exp->data;
    node->param_exp = &monitorenter->list->args[0];
    exp_mark_nopped(exp);
}

static jd_exc* exception_of_handler(jd_method *m, jd_node *node)
{
    jd_ins *first = get_ins(m, node->start_idx);
    if (first == NULL || m->cfg_exceptions == NULL)
        return NULL;

    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->cfg_exceptions, i);
        if (e->handler_start == first->offset)
            return e;
    }
    return NULL;
}

static void setup_catch_param_from_table(jd_method *m, jd_node *node)
{
    jd_exc *e = exception_of_handler(m, node);
    if (e == NULL)
        return;

    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex->meta;
    string class_desc = e->catch_type_index == 0 ?
                        "Ljava/lang/Throwable" :
                        meta->strings[meta->type_ids[e->catch_type_index]
                                      .descriptor_idx].data;

    jd_val *val = stack_create_empty_val();
    val->type = JD_VAR_REFERENCE_T;
    val->data->cname = class_simple_name(class_full_name(class_desc));
    val->ins = NULL;

    jd_ins *first = get_ins(m, node->start_idx);
    int reg = first == NULL ? 0 : (int) dex_ins_parameter((jd_dex_ins *) first, 0);
    val->slot = reg;

    jd_val *in_reg = (first != NULL && first->stack_out != NULL &&
                      reg < first->stack_out->local_vars_count)
                     ? first->stack_out->local_vars[reg] : NULL;
    bool name_free = in_reg != NULL && in_reg->name != NULL &&
                     strcmp(in_reg->name, "this") != 0 &&
                     (m->var_name_taken == NULL ||
                      hget_s2i(m->var_name_taken, in_reg->name) == -1);

    if (name_free)
        val->name = in_reg->name;
    else {
        dex_variable_name(m, NULL, val, reg);
        if (val->name == NULL)
            stack_val_name(m, NULL, val, reg);
    }

    jd_exp *param = make_obj_zero(jd_exp);
    param->type = JD_EXPRESSION_LOCAL_VARIABLE;
    param->data = val;
    node->param_exp = param;
}

static void setup_catch_node_param(jd_method *m, jd_node *node)
{
    jd_exp *exp = get_exp(m, node->start_idx);

    if (exp_is_store(exp) && m->type != JD_TYPE_DALVIK) {
        jd_exp_store *exp_store = exp->data;
        jd_exp *left = &exp_store->list->args[0];
        node->param_exp = left;
        exp_mark_nopped(exp);
        return;
    }

    jd_ins *ins = exp == NULL ? NULL : exp->ins;
    if (ins != NULL && ins->type == JD_TYPE_DALVIK && ins->stack_in != NULL &&
        dex_ins_is_move_exception((jd_dex_ins *) ins)) {
        int reg = move_exception_reg_num((jd_dex_ins *) ins);
        if (reg < 0 || reg >= ins->stack_in->local_vars_count)
            return;
        jd_val *val = ins->stack_in->local_vars[reg];
        if (val == NULL || val->data == NULL)
            return;

        exp->type = JD_EXPRESSION_LOCAL_VARIABLE;
        exp->data = val;
        node->param_exp = exp;
        exp_mark_nopped(exp);
        return;
    }

    if (m->type == JD_TYPE_DALVIK)
        setup_catch_param_from_table(m, node);
}

static void setup_for_loop_node_param(jd_method *m, jd_node *node)
{
    jd_node *n = lget_obj_first(node->children);
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

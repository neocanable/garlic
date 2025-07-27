#include "decompiler/expression.h"
#include "decompiler/expression_synchronized.h"
#include "decompiler/expression_node.h"

static void nop_monitor_exit_recursive(jd_method *m, jd_node *node)
{
    if (node_is_expression(node))
        return;

    if (node_is_basic_block(node)) {
        jd_bblock *b = node->data;
        jd_nblock *nb = b->ub->nblock;
        for (int i = nb->start_idx; i <= nb->end_idx ; ++i) {
            jd_exp *exp = get_exp(m, i);
            if (exp_is_monitor_exit(exp))
                exp_mark_nopped(exp);
        }
    }
    else {
        for (int i = 0; i < node->children->size; ++i) {
            jd_node *child = lget_obj(node->children, i);
            if (node_is_exception(child))
                continue;
            nop_monitor_exit_recursive(m, child);
        }
    }
}

int identify_synchronized(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_exception(node))
            continue;
        jd_exp *exp = prev_valid_exp(m, node->start_idx - 1);
        if (exp == NULL || !exp_is_monitor_enter(exp))
            continue;

        jd_node *finally = child_of_type(node, JD_NODE_FINALLY);
        if (finally == NULL)
            continue;

        jd_node *try = child_of_type(node, JD_NODE_TRY);
        ldel_obj(node->children, finally);

        nop_monitor_exit_recursive(m, try);

        jd_node *parent = node->parent;
        int index = lfind_object(parent->children, node);
        ladd_obj_at(parent->children, try, index);
        ldel_obj(parent->children, node);
        try->parent = parent;
        try->type = JD_NODE_SYNCHRONIZED;
        ldel_obj(m->nodes, node);
        i--;
    }
    return 0;
}

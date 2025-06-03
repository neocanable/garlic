#include "decompiler/expression_return.h"
#include "decompiler/control_flow.h"

static jd_node* get_parent(jd_node *n)
{
    jd_node *p = n->parent;
    if (node_is_try(p) || node_is_catch(p) || node_is_finally(p))
        p = p->parent;
    return p;
}

void nop_node_last_return(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *n = lget_obj(m->nodes, i);
        if (!node_is_basic_block(n))
            continue;
        jd_exp *exp = get_exp(m, n->end_idx);
        if (!exp_is_return(exp) || exp_is_nopped(exp))
            continue;

        jd_ins *ins = exp->ins;
        jd_ins_fn *fn = NULL;
        if (ins != NULL)
            fn = ins->fn;

        if (fn == NULL || !fn->is_void_return(ins))
            continue;

        jd_node *parent = get_parent(n);
        jd_node *parent_next = parent_next_node(parent);
        if (parent_next == NULL) {
            exp_mark_nopped(exp);
        }
    }
}
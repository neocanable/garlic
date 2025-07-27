#include "expression_goto.h"
#include "control_flow.h"
#include "expression_writter.h"

static void goto_to_continue(jd_exp *e)
{
    e->type = JD_EXPRESSION_CONTINUE;
}

static void goto_to_break(jd_exp *e)
{
    e->type = JD_EXPRESSION_BREAK;
}

static jd_node* closest_case(jd_node *n)
{
    jd_node *p = n->parent;
    while (p != NULL && p->type != JD_NODE_METHOD_ROOT) {
        if (node_is_case(p))
            return p;
        else
            p = p->parent;
    }
    return NULL;
}

static jd_node* closest_exception(jd_node *n)
{
    jd_node *p = n->parent;
    while (p != NULL) {
        if (node_is_exception(p))
            return p;
        else
            p = p->parent;
    }
    return NULL;
}

static bool has_case_parent(jd_node *n)
{
    return closest_case(n) != NULL;
}

static jd_node* closest_loop(jd_node *n)
{
    jd_node *p = n->parent;
    while (p != NULL && p->type != JD_NODE_METHOD_ROOT) {
        if (node_is_loop(p))
            return p;
        else
            p = p->parent;
    }
    return NULL;
}

static bool has_loop_parent(jd_node *n)
{
    return closest_loop(n) != NULL;
}

static bool goto_is_loop_last(jd_node *loop,
                              jd_node *node,
                              jd_node *target)
{
    return node == lget_obj_last(loop->children);
}

static bool goto_is_goto_loop_head(jd_node *loop,
                                   jd_node *node,
                                   jd_node *target)
{
    return target == lget_obj_first(loop->children);
}

static bool goto_is_goto_post_condition_last(jd_node *loop,
                                             jd_node *node,
                                             jd_node *target)
{
    return target == lget_obj_last(loop->children);
}

static jd_node* get_parent(jd_node *n)
{
    jd_node *p = n->parent;
    if (node_is_try(p) || node_is_catch(p) || node_is_finally(p))
        p = p->parent;
    return p;
}

static bool goto_jump_to_loop_last(jd_node *loop, jd_node *target)
{
    jd_node *last_node = lget_obj_last(loop->children);
    return last_node == target;
}

static void optimize_goto_in_case(jd_method *m, jd_node *node, jd_node *target)
{
    jd_exp *exp = get_exp(m, node->end_idx);
    jd_node *parent = get_parent(node);
    jd_node *parent_next = parent_next_node(parent);

    jd_node *case_node = closest_case(node);
    jd_node *switch_node = case_node->parent;
    jd_node *switch_next = parent_next_node(switch_node);
    assert(switch_node != NULL);
    if (switch_next == NULL ||
        target->end_idx >= switch_next->start_idx) {
        // case break;
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s: case break: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_break(exp);
    }
    else if ((parent_next != NULL &&
              (node_is_ancestor_of(parent_next, target) || 
               parent_next == target)) ||
             parent_next == NULL) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto optimized]: %s %d\n",
                                  m->name,
                                  exp->ins->offset);
        exp_mark_nopped(exp);
    }
    else {
        // TODO: fix here
        DEBUG_PRINT("[goto optimized unknown]: %s %d\n",
               m->name,
               exp->ins->offset);
    }
}

static void optimize_goto_in_loop(jd_method *m, jd_node *node, jd_node *target)
{
    // 1. target_node->idx > loop->end_idx break;
    // 2. target_node->idx == loop->last_idx && loop_last is go_back
    //  continue
    // 3. target_node->idx == loop->first_idx => continue
    jd_exp *exp = get_exp(m, node->end_idx);
    jd_node *parent = get_parent(node);
    jd_node *parent_next = parent_next_node(parent);


    jd_node *loop = closest_loop(node);
    if (loop->end_idx < target->start_idx) {
        // goto is break;
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s break: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_break(exp);
    }
    else if (goto_jump_to_loop_last(loop, target)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s continue: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_continue(exp);
    }
    else if (goto_is_loop_last(loop, node, target)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s last continue: %d\n",
                                  m->name,
                                  exp->ins->offset);
        exp_mark_nopped(exp);
    }
    else if (goto_is_goto_loop_head(loop, node, target)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s loop condinue: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_continue(exp);
    }
    else if (goto_is_goto_post_condition_last(loop, node, target)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s post condition continue: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_continue(exp);
    }
    else if ((parent_next != NULL &&
              (node_is_ancestor_of(parent_next, target) ||
                parent_next == target)) ||
             parent_next == NULL) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto optimized]: %s %d\n",
                                  m->name,
                                  exp->ins->offset);
        exp_mark_nopped(exp);
    }
    else if (has_case_parent(node)) {
        optimize_goto_in_case(m, node, target);
    }
    else {
        // TODO: fix here
        DEBUG_PRINT("[goto optimized unknown]: %s %d\n",
               m->name,
               exp->ins->offset);
    }
}

static jd_node* goto_target_node(jd_method *m, jd_node *node)
{
    jd_bblock *bblock = node->data;
    if (bblock->out->size == 0)
        return NULL;
    jd_exp *last_exp = get_exp(m, node->end_idx);
    jd_exp_goto *goto_exp = last_exp->data;
    jd_exp* target_exp = exp_of_offset(m, goto_exp->goto_offset);
    jd_bblock *target = target_exp->block;
//    jd_edge *edge = lget_obj(bblock->out, 0);
//    jd_bblock *target = edge->target_block;
    if (!basic_block_is_normal_live(target))
        return NULL;
    jd_node *target_node = target->node;
#if 0
    jd_exp *exp = get_exp(m, target_node->end_idx);
    while (exp_is_goto(exp) &&
            !exp_is_nopped(exp) &&
            target->out->size > 0 &&
            target_node->start_idx == target_node->end_idx) {
        jd_edge* edge = lget_obj(target->out, 0);
        target = edge->target_block;
        if (!basic_block_is_normal_live(target))
            break;

        target_node = target->node;
        if (target_node->start_idx != target_node->end_idx)
            break;
        exp = get_exp(m, target_node->end_idx);
    }
#endif
    return target_node;
}

static bool optimize_goto_core(jd_method *m, jd_node *node)
{
    jd_exp *exp = get_exp(m, node->end_idx);
    jd_node *target = goto_target_node(m, node);
    if (target == NULL)
        return false;
    jd_node *parent = get_parent(node);
    jd_node *parent_next = parent_next_node(parent);

    if (has_loop_parent(node)) {
        optimize_goto_in_loop(m, node, target);
    }
    else if (has_case_parent(node)) {
        optimize_goto_in_case(m, node, target);
    }
    else if (target->start_idx < exp->idx) {
    }
    else if ((parent_next != NULL &&
            (node_is_ancestor_of(parent_next, target) ||
             parent_next == target)) ||
             parent_next == NULL) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto optimized]: %s %d\n",
                                  m->name,
                                  exp->ins->offset);
        exp_mark_nopped(exp);
    }

    return false;
}


void optimize_goto_expression(jd_method *m)
{
    bool removed;
    do {
        removed = 0;
        for (int i = 0; i < m->nodes->size; ++i) {
            jd_node *n = lget_obj(m->nodes, i);
            if (!node_is_basic_block(n))
                continue;
            jd_exp *exp = get_exp(m, n->end_idx);
            if (!exp_is_goto(exp) || exp_is_nopped(exp))
                continue;
            jd_ins *ins = exp->ins;
            jd_ins_fn *fn = ins == NULL ? NULL : ins->fn;
            if (fn != NULL && fn->is_goto(ins)) {
                if (ins_is_copy_block(ins))
                    continue;
            }

            removed |= optimize_goto_core(m, n);
        }
    } while (removed);
}

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

static jd_node* loop_header_node(jd_node *loop)
{
    if (loop->children == NULL || loop->children->size == 0)
        return NULL;

    if (loop->type == JD_NODE_DO_WHILE)
        return lget_obj_last(loop->children);

    return lget_obj_first(loop->children);
}

static bool goto_jump_to_loop_header(jd_node *loop, int target_idx)
{
    jd_node *header = loop_header_node(loop);
    if (header == NULL || target_idx < 0)
        return false;

    return header->start_idx <= target_idx &&
           target_idx <= header->end_idx;
}

static bool goto_target_is_next_written(jd_node *next, int target_idx)
{
    if (next == NULL || target_idx < 0)
        return false;

    return next->start_idx <= target_idx && target_idx <= next->end_idx;
}

static bool goto_target_is_next_block(jd_method *m,
                                      jd_node *next,
                                      int target_idx)
{
    if (next == NULL || target_idx < 0 || next->start_idx < 0)
        return false;

    jd_exp *target = get_exp(m, target_idx);
    jd_exp *start = get_exp(m, next->start_idx);
    if (target == NULL || start == NULL)
        return false;

    return target->block != NULL && target->block == start->block;
}

static bool nothing_runs_between(jd_method *m, int from_idx, int to_idx)
{
    for (int i = from_idx + 1; i < to_idx; ++i) {
        jd_exp *exp = get_exp(m, i);
        if (exp != NULL && !exp_is_nopped(exp) && !exp_is_empty(exp))
            return false;
    }
    return true;
}

static jd_node* get_parent(jd_node *n)
{
    jd_node *p = n->parent;
    if (node_is_try(p) || node_is_catch(p) || node_is_finally(p))
        p = p->parent;
    return p;
}

static void optimize_goto_in_case(jd_method *m, jd_node *node, int target_idx)
{
    jd_exp *exp = get_exp(m, node->end_idx);
    jd_node *parent = get_parent(node);
    jd_node *parent_next = parent_next_node(parent);

    jd_node *case_node = closest_case(node);
    jd_node *switch_node = case_node->parent;
    jd_node *switch_next = parent_next_node(switch_node);
    assert(switch_node != NULL);
    if (switch_next == NULL ||
        target_idx >= switch_next->start_idx) {
        // case break;
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s: case break: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_break(exp);
    }
    else if (parent_next == NULL ||
             goto_target_is_next_written(parent_next, target_idx) ||
             nothing_runs_between(m, exp->idx, target_idx)) {
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

static void optimize_goto_in_loop(jd_method *m, jd_node *node, int target_idx)
{
    jd_exp *exp = get_exp(m, node->end_idx);
    jd_node *parent = get_parent(node);
    jd_node *parent_next = parent_next_node(parent);

    jd_node *loop = closest_loop(node);
    if (goto_jump_to_loop_header(loop, target_idx)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s continue: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_continue(exp);
    }
    else if (has_case_parent(node) && loop->end_idx < target_idx) {
        DEBUG_PRINT("[goto] %s %d leaves the loop from inside a switch\n",
                    m->name, exp->ins->offset);
    }
    else if (has_case_parent(node)) {
        optimize_goto_in_case(m, node, target_idx);
    }
    else if (loop->end_idx < target_idx) {
        // goto is break;
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s break: %d\n",
                                  m->name,
                                  exp->ins->offset);
        goto_to_break(exp);
    }
    else if (goto_is_loop_last(loop, node, 0)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto] %s last continue: %d\n",
                                  m->name,
                                  exp->ins->offset);
        exp_mark_nopped(exp);
    }
    else if (parent_next == NULL ||
             goto_target_is_next_written(parent_next, target_idx) ||
             nothing_runs_between(m, exp->idx, target_idx)) {
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

static jd_node* goto_target_node(jd_method *m, jd_node *node, int *out_idx)
{
    if (out_idx != NULL)
        *out_idx = -1;

    jd_bblock *bblock = node->data;
    if (bblock == NULL || bblock->out == NULL || bblock->out->size == 0)
        return NULL;

    jd_exp *last_exp = get_exp(m, node->end_idx);
    if (last_exp == NULL || !exp_is_goto(last_exp))
        return NULL;

    jd_exp_goto *goto_exp = last_exp->data;
    jd_exp *target_exp = exp_of_offset(m, goto_exp->goto_offset);
    if (target_exp == NULL || target_exp->block == NULL)
        return NULL;

    jd_bblock *target = target_exp->block;
    if (!basic_block_is_normal_live(target))
        return NULL;

    int at = target_exp->idx;
    if (out_idx != NULL)
        *out_idx = at;

    jd_node *found = NULL;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *n = lget_obj(m->nodes, i);
        if (n->type == JD_NODE_DELETED ||
            n->start_idx > at || n->end_idx < at)
            continue;
        if (found == NULL ||
            (n->start_idx >= found->start_idx && n->end_idx <= found->end_idx))
            found = n;
    }

    return found;
}

static void goto_trace_kept(const char *why, jd_method *m, jd_node *node,
                            jd_node *parent, jd_node *parent_next,
                            int target_idx)
{
    if (getenv("GARLIC_GOTO_TRACE") == NULL)
        return;

    jd_exp *exp = get_exp(m, node->end_idx);
    jd_exp *te = (target_idx >= 0 && target_idx < m->expressions->size)
                 ? get_exp(m, target_idx) : NULL;
    jd_exp *ne = (parent_next != NULL && parent_next->start_idx >= 0 &&
                  parent_next->start_idx < m->expressions->size)
                 ? get_exp(m, parent_next->start_idx) : NULL;
    fprintf(stderr,
            "[garlic] goto kept (%s) %s ins %u: node %d %s [%d..%d], "
            "parent %s [%d..%d], next %s [%d..%d], target %d exp %d "
            "| target off %x, next off %x\n",
            why, m->name, exp->ins->offset,
            node->node_id, node_name(node), node->start_idx, node->end_idx,
            parent == NULL ? "null" : node_name(parent),
            parent == NULL ? -1 : parent->start_idx,
            parent == NULL ? -1 : parent->end_idx,
            parent_next == NULL ? "null" : node_name(parent_next),
            parent_next == NULL ? -1 : parent_next->start_idx,
            parent_next == NULL ? -1 : parent_next->end_idx,
            target_idx, exp->idx,
            te == NULL || te->ins == NULL ? 0 : te->ins->offset,
            ne == NULL || ne->ins == NULL ? 0 : ne->ins->offset);
}

static bool optimize_goto_core(jd_method *m, jd_node *node)
{
    jd_exp *exp = get_exp(m, node->end_idx);
    int target_idx = -1;
    jd_node *target = goto_target_node(m, node, &target_idx);
    jd_node *parent = get_parent(node);
    jd_node *parent_next = parent_next_node(parent);

    if (target == NULL) {
        DEBUG_PRINT("[goto] %s %d no target\n", m->name, exp->ins->offset);
        goto_trace_kept("no target", m, node, parent, parent_next, target_idx);
        return false;
    }

    if (has_loop_parent(node)) {
        optimize_goto_in_loop(m, node, target_idx);
    }
    else if (has_case_parent(node)) {
        optimize_goto_in_case(m, node, target_idx);
    }
    else if (target_idx < exp->idx) {
        DEBUG_PRINT("[goto] %s %d backward target_idx=%d exp_idx=%d\n",
                    m->name, exp->ins->offset, target_idx, exp->idx);
        goto_trace_kept("backward", m, node, parent, parent_next, target_idx);
    }
    else if (parent_next == NULL ||
             goto_target_is_next_written(parent_next, target_idx) ||
             goto_target_is_next_block(m, parent_next, target_idx) ||
             nothing_runs_between(m, exp->idx, target_idx)) {
        DEBUG_GOTO_OPTIMIZE_PRINT("[goto optimized]: %s %d\n",
                                  m->name,
                                  exp->ins->offset);
        exp_mark_nopped(exp);
    }
    else {
        DEBUG_PRINT("[goto] %s %d forward, next is not the target\n",
                    m->name, exp->ins->offset);
        goto_trace_kept("forward, next is not the target",
                        m, node, parent, parent_next, target_idx);
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
            removed |= optimize_goto_core(m, n);
        }
    } while (removed);
}

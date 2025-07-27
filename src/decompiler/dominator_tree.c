#include "decompiler/dominator_tree.h"
#include "decompiler/control_flow.h"

static int compute_dominance_frontier_callback(jd_bblock *block)
{
    lclear_object(block->frontier);

    for (int i = 0; i < block->out->size; ++i) {
        jd_edge *edge = lget_obj(block->out, i);
        jd_bblock *target_block = edge->target_block;
        if (target_block->idom != block)
            ladd_obj_no_dup(block->frontier, target_block);
    }

    for (int i = 0; i < block->dom_children->size; ++i) {
        jd_bblock *child_block = lget_obj(block->dom_children, i);
        for (int j = 0; j < child_block->frontier->size; ++j) {
            jd_bblock *frontier_block = lget_obj(child_block->frontier, j);
            if (frontier_block->idom != block)
                ladd_obj_no_dup(block->frontier, frontier_block);
        }
    }
    return 0;
}

static void post_order_traversal(jd_bblock *block, traversal_cb cb);

bool dominates(const jd_bblock *check, const jd_bblock *other)
{
    if (check->block_id == other->block_id)
        return true;

    jd_bblock *_tmp = other->idom;
    while (_tmp != NULL) {
        if (_tmp->block_id == check->block_id)
            return true;
        _tmp = _tmp->idom;
    }
    return false;
}

void dominance_frontier(jd_method *m)
{
    basic_block_clear_visited_flag(m);

    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (!basic_block_is_live(block))
            continue;
        post_order_traversal(block, compute_dominance_frontier_callback);
    }
}

void compute_dominates_block(jd_method *m, jd_bblock *block)
{
    lclear_object(block->dominates);
    for (int j = 0; j < m->basic_blocks->size; ++j) {
        jd_bblock *other = lget_obj(m->basic_blocks, j);
        // self dominates self
        if (other->block_id == block->block_id) {
            ladd_obj(block->dominates, other);
            continue;
        }
        if (dominates(block, other))
            ladd_obj(block->dominates, other);
    }
}

static int pre_order_traversal(jd_bblock *block, traversal_cb callback)
{
    if (block->visited)
        return 0;
    block->visited = 1;
    int result = callback(block);
    for (int i = 0; i < block->out->size; ++i) {
        jd_edge *edge = lget_obj(block->out, i);
        if (!edge->target_block->visited)
            result |= pre_order_traversal(edge->target_block, callback);
    }
    return result;
}

static void post_order_traversal(jd_bblock *block, traversal_cb cb)
{
    if (block->visited || !basic_block_is_live(block)) return;
    block->visited = 1;

    for (int i = 0; i < block->dom_children->size; ++i) {
        jd_bblock *child_block = lget_obj(block->dom_children, i);
        post_order_traversal(child_block, cb);
    }
    cb(block);
}

static inline bool block_in_list(const jd_bblock *block, const jd_bblock *list)
{
    if (list == NULL)
        return false;
    if (list->block_id == block->block_id)
        return true;

    return block_in_list(block, list->idom);
}

static const jd_bblock* same_dom_block(const jd_bblock *b1, 
                                       const jd_bblock *b2)
{
    if (b1 == NULL || b2 == NULL)
        return NULL;
    if (b1->block_id == b2->block_id)
        return b1;

    if (block_in_list(b1, b2->idom))
        return b1;

    return same_dom_block(b2, b1->idom);
}

static bool compute_dominator_cb(jd_bblock *block)
{
    jd_bblock *dominator_block = NULL;

    for (int i = 0; i < block->in->size; ++i) {
        jd_edge *edge = lget_obj(block->in, i);
        jd_bblock *source_block = edge->source_block;
        if (source_block->visited) {
            dominator_block = source_block;
            break;
        }
    }

    for (int i = 0; i < block->in->size; ++i) {
        jd_edge *edge = lget_obj(block->in, i);
        jd_bblock *source_block = edge->source_block;
        if (source_block->idom != NULL) {
            jd_bblock *_tmp = NULL;
            _tmp = (jd_bblock *) same_dom_block(dominator_block, source_block);
            if (_tmp != NULL)
                dominator_block = _tmp;
        }
    }

    if (block->idom != dominator_block &&
        block != dominator_block) {
        block->idom = dominator_block;
        return true;
    }
    return false;
}

static void dominate_blocks_v2(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        for (int j = 0; j < m->basic_blocks->size; ++j) {
            jd_bblock *other = lget_obj(m->basic_blocks, j);
            if (other->block_id == block->block_id) {
                ladd_obj(block->dominates, other);
                continue;
            }

            if (dominates(block, other))
                ladd_obj(block->dominates, other);
        }
    }
}

void dominate_blocks(jd_method *m, jd_bblock *block)
{
    if (block == NULL)
        block = lget_object(m->basic_blocks, 3);

    ladd_obj_no_dup(block->dominates, block);

    for (int i = 0; i < block->dom_children->size; ++i) {
        jd_bblock *child = lget_obj(block->dom_children, i);
        ladd_obj_no_dup(block->dominates, child);
        dominate_blocks(m, child);
    }

    for (int i = 0; i < block->dom_children->size; ++i) {
        jd_bblock *child = lget_obj(block->dom_children, i);
        for (int j = 0; j < child->dominates->size; ++j) {
            jd_bblock *b = lget_obj(child->dominates, j);
            ladd_obj_no_dup(block->dominates, b);
        }
    }
}

void dominator_tree(jd_method *m)
{
    jd_bblock *enter_block = block_enter(m);
    enter_block->idom = enter_block;
    int changed = 1;
    int result_tmp = 0;
    while (changed) {
        changed = 0;
        basic_block_clear_visited_flag(m);
        for (int i = 0; i < m->basic_blocks->size; ++i) {
            jd_bblock *block = lget_obj(m->basic_blocks, i);
            if (!basic_block_is_live(block))
                continue;
            result_tmp = pre_order_traversal(block, compute_dominator_cb);
            changed |= result_tmp == 1;
        }
    }

    enter_block->idom = NULL;
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (!basic_block_is_live(block))
            continue;
        jd_bblock *idom_block = block->idom;
        if (idom_block != NULL)
            ladd_obj_no_dup(idom_block->dom_children, block);
    }
}

void create_dominator_tree(jd_method *m)
{
    dominator_tree(m);
    dominance_frontier(m);
    // dominate_blocks(m, NULL);
}

void clear_dominator_data(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        lclear_object(block->frontier);
        lclear_object(block->dom_children);
        lclear_object(block->dominates);
        block->idom = NULL;
    }
}

void clear_dominator_tree(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        lclear_object(block->dom_children);
        lclear_object(block->frontier);
        lclear_object(block->dominates);
        block->idom = NULL;
    }
    lclear_object(m->cfg_exceptions);
}

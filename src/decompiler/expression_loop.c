#include "jvm/jvm_ins.h"
#include "decompiler/control_flow.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "decompiler/dominator_tree.h"
#include "str_tools.h"
#include "debug_pointer.h"

static bool loop_already_exist(jd_method *m, jd_loop *loop)
{
    for (int j = 0; j < m->loops->size; ++j) {
        jd_loop *l = lget_obj(m->loops, j);
        if (loop->start_idx == l->start_idx &&
            loop->end_idx == l->end_idx)
            return true;
    }
    return false;
}

static jd_range* basic_block_to_range(jd_bblock *block)
{
    jd_nblock *nblock = block->ub->nblock;
    jd_range *r = make_obj(jd_range);
    r->start_idx = nblock->start_idx;
    r->end_idx = nblock->end_idx;
    return r;
}

static int node_valid_exp_count(jd_method *m, jd_node *node)
{
    assert(node_is_basic_block(node));
    int count = 0;
    for (int i = node->start_idx; i <= node->end_idx; ++i) {
        jd_exp *exp = get_exp(m, i);
        if (!exp_is_nopped(exp))
            count++;
    }
    return count;
}

static jd_bblock* loop_first_block(jd_loop *loop)
{
    jd_bblock *first = NULL;
    jd_nblock *nblock = NULL;
    for (int i = 0; i < loop->blocks->size; ++i) {
        jd_bblock *block = lget_obj(loop->blocks, i);
        if (!basic_block_is_normal_live(block))
            continue;
        if (block->is_dup)
            continue;
        nblock = block->ub->nblock;
        if (first == NULL || nblock->start_idx < first->ub->nblock->start_idx)
            first = block;
    }
    return first;
}

static jd_bblock* loop_last_block(jd_loop *loop)
{
    jd_bblock *last = NULL;
    jd_nblock *nblock = NULL;
    for (int i = 0; i < loop->blocks->size; ++i) {
        jd_bblock *block = lget_obj(loop->blocks, i);
        if (!basic_block_is_normal_live(block))
            continue;
        if (block->is_dup)
            continue;
        nblock = block->ub->nblock;
        if (last == NULL || last->ub->nblock->end_idx < nblock->end_idx)
            last = block;
    }
    return last;
}

static int count_jumps(jd_loop *loop, jd_bblock *target, jd_exp *exp)
{
    int jump = 0;
    for (int i = 0; i < loop->blocks->size; ++i) {
        jd_bblock *block = lget_obj(loop->blocks, i);
        jd_nblock *nblock = block->ub->nblock;

        bool found = false;
        bool found_exp = false;
        for (int j = 0; j < block->out->size; ++j) {
            jd_edge *edge = lget_obj(block->out, j);
            jd_bblock *t = edge->target_block;
            if (t != target)
                continue;

            for (int k = nblock->start_idx; k <= nblock->end_idx; ++k) {
                jd_exp *e = get_exp(block->method, k);
                if (e == exp) {
                    found_exp = true;
                    break;
                }
            }
            found = !found_exp;
            if (found)
                jump++;
        }
    }
    return jump;
}

static void find_loop_condition(jd_method *m, jd_loop *loop, jd_bblock *header)
{
    jd_bblock *first = loop_first_block(loop);
    jd_bblock *last = loop_last_block(loop);

    for (int i = 0; i <= 1; ++i) {
        bool is_post_condition = i == 1;
        jd_bblock *block = NULL;
        jd_node *node = NULL;
        bool found_condition = false;
        jd_exp *last_exp = NULL;
        if (is_post_condition) {
            block = last;
            node = block->node;
            last_exp = get_exp(m, node->end_idx);
            found_condition = exp_is_if(last_exp);
        }
        else {
            block = first;
            node = block->node;
            last_exp = get_exp(m, node->end_idx);
            found_condition = exp_is_if(last_exp);
            found_condition = found_condition &&
                              node_valid_exp_count(m, node) == 1;
        }
        if (!found_condition)
            continue;
        jd_exp_if *if_exp = last_exp->data;
        uint32_t offset = if_exp->offset;
        jd_exp *true_exp = exp_of_offset(m, offset);
        jd_bblock *true_target = true_exp->block;
        jd_ins *if_ins = last_exp->ins;
        jd_ins *if_next_ins = if_ins->next;
        jd_exp *if_next_exp = if_next_ins->expression;
        jd_bblock *false_target = exp_block(if_next_exp);

        if ((!lcontains_obj(loop->blocks, false_target) ||
            lcontains_obj(loop->blocks, true_target)) &&
                (!lcontains_obj(loop->blocks, true_target) ||
                 lcontains_obj(loop->blocks, false_target))) {
            continue;
        }

        loop->condition = block;
        bool reversed = lcontains_obj(loop->blocks, false_target) || 
                        false_target == header;
        if (!reversed) {
            jd_bblock *tmp = true_target;
            true_target = false_target;
            false_target = tmp;
            // flip the condition
        }
        bool can_write_condition = false;
        if (is_post_condition) {
            int count = count_jumps(loop, true_target, last_exp);
            can_write_condition = count == 0;
        }
        else {
            can_write_condition = true;
        }
        loop->is_post_condition = is_post_condition;
        loop->can_write_condition = can_write_condition;

        if (can_write_condition && !is_post_condition) {
            compute_dominates_block(m, false_target);
            for (int j = 0; j < false_target->dominates->size; ++j) {
                jd_bblock *dom = lget_obj(false_target->dominates, j);
                if (basic_block_is_normal_live(dom))
                    ladd_obj_no_dup(loop->blocks, dom);
            }
            break;
        }
    }
}

static void find_loop_exit_blocks(jd_loop *loop)
{
    loop->exit_to_blocks = linit_object();
    int no_normal_exit = 0;
    for (int i = 0; i < loop->blocks->size; ++i) {
        jd_bblock *block = lget_obj(loop->blocks, i);

        for (int j = 0; j < block->out->size; ++j) {
            jd_edge *edge = lget_obj(block->out, j);
            jd_bblock *target = edge->target_block;
            if (!basic_block_is_normal_live(target))
                continue;

            if (lcontains_obj(loop->blocks, target))
                continue;

            if (target->in->size == 1) {
                if (!lcontains_obj(loop->exit_to_blocks, target) &&
                        basic_block_is_normal_live(target))
                    ladd_obj(loop->exit_to_blocks, target);
            }
            else if (loop->exit_block == NULL)
                loop->exit_block = target;
            else if (loop->exit_block != target)
                no_normal_exit = 1;
        }
    }
    if (no_normal_exit)
        loop->exit_block = NULL;
}

static inline void setup_loop_ranges(jd_loop *loop)
{
    jd_bblock *first = loop_first_block(loop);
    jd_bblock *last = loop_last_block(loop);

    loop->header = first;
    loop->tail = last;
    loop->start_idx = first->node->start_idx;
    loop->end_idx = last->node->end_idx;

    if (loop->header != NULL)
        loop->header_range = basic_block_to_range(loop->header);
    if (loop->tail != NULL)
        loop->tail_range = basic_block_to_range(loop->tail);
    if (loop->condition != NULL)
        loop->condition_range = basic_block_to_range(loop->condition);
}

static bool switch_contains_offset(jd_ins *sw, uint32_t offset)
{
    for (int i = 0; i < sw->targets->size; ++i) {
        jd_ins *target_ins = lget_obj(sw->targets, i);
        if (target_ins->offset == offset) 
            return true;
    }
    return false;
}

static inline bool is_loop_exist(jd_method *m, jd_bblock *block)
{
    for (int i = 0; i < m->loops->size; ++i) {
        jd_loop *loop = lget_obj(m->loops, i);
        if (loop->start_idx == block->ub->nblock->start_idx)
            return true;
        if (loop->end_idx == block->ub->nblock->end_idx)
            return true;
    }
    return false;
}

static jd_node* parent_of_loop(jd_node *parent, jd_loop *loop)
{
    jd_node *p = parent;
    for (int i = 0; i < loop->blocks->size; ++i) {
        jd_bblock *b = lget_obj(loop->blocks, i);
        if (!basic_block_is_normal_live(b))
            continue;
        jd_node *n = b->node;
        if (node_is_ancestor_of(p, n))
            continue;

        while (!node_is_ancestor_of(p, n)) {
            p = p->parent;
            if (node_is_exception(p))
                p = p->parent;
        }
    }
    return p;
}

jd_node* loop_to_node(jd_method *m, jd_node *parent, jd_loop *loop)
{
    jd_node *node = make_obj(jd_node);
    loop->node = node;

    node->start_idx = loop->start_idx;
    node->end_idx = loop->end_idx;
    node->type = JD_NODE_LOOP;
    node->node_id = m->nodes->size;
    node->children = linit_object();
    node->data = loop;
    ladd_obj(m->nodes, node);

    jd_node *new_parent = parent_of_loop(parent, loop);
    for (int i = 0; i < loop->blocks->size; ++i) {
        jd_bblock *block = lget_obj(loop->blocks, i);
        jd_node *n = block->node;

        while (n->parent != NULL &&
               n->parent != new_parent &&
               n->parent != node) {
            n = n->parent;
        }
        if (n->parent == node)
            continue;

        ldel_obj(new_parent->children, n);
        ladd_obj_no_dup(node->children, n);
        n->parent = node;
    }
    node->parent = new_parent;
    ladd_obj(new_parent->children, node);
    node_children_sort(node);
    node_children_sort(new_parent);
    return node;
}

static void find_loop_content(jd_loop *loop, jd_node *node, jd_bblock *block)
{
    if (block->visited)
        return;
    else
        block->visited = 1;

    if (lcontains_obj(loop->header->dominates, block) &&
        basic_block_is_normal_live(block)
        ) {

        ladd_obj(loop->blocks, block);
        for (int i = 0; i < block->in->size; ++i) {
            jd_edge *edge = lget_obj(block->in, i);
            jd_bblock *source = edge->source_block;
            find_loop_content(loop, node, source);
        }
    }
}

static void loop_content_with_exit_block(jd_method *m,
                                         jd_node *node,
                                         jd_bblock *block,
                                         jd_loop *loop)
{
    if (loop->exit_block->in->size == 1) {
        jd_edge *edge = lget_obj(loop->exit_block->in, 0);
        jd_bblock *source = edge->source_block;
        if (source != NULL && lcontains_obj(loop->blocks, source)) {
            jd_nblock *source_nblock = source->ub->nblock;
            jd_nblock *exit_nblock = loop->exit_block->ub->nblock;
            jd_exp *source_end_exp = get_exp(m, source_nblock->end_idx);
            jd_exp *exit_start_exp = get_exp(m, exit_nblock->start_idx);
            jd_ins *source_end_ins = source_end_exp->ins;
            // if source's end instruction is swith
            // then switch's default target is loop's header
            if (source_end_ins != NULL &&
                source_end_ins->fn->is_switch(source_end_ins) &&
                switch_contains_offset(source_nblock->end_ins,
                                       exit_start_exp->ins->offset)) {
                compute_dominates_block(m, loop->exit_block);
                for (int j = 0; j < loop->exit_block->dominates->size; ++j) {
                    jd_bblock *c = lget_obj(loop->exit_block->dominates, j);
                    if (lcontains_obj(block->dominates, c) &&
                        node_contains_block_v2(node, c))
                        ladd_obj_no_dup(loop->blocks, c);
                }
            }
        }
    }

    if (!lcontains_obj(loop->blocks, loop->exit_block)) {

        for (int j = 0; j < m->basic_blocks->size; ++j) {
            jd_bblock *c = lget_obj(m->basic_blocks, j);
            if (!basic_block_is_normal_live(c))
                continue;

            jd_nblock *nc = c->ub->nblock;
            jd_nblock *exit_nblock = loop->exit_block->ub->nblock;
            if (nc->start_idx >= exit_nblock->start_idx)
                continue;

            if (lcontains_obj(block->dominates, c) &&
                node_contains_block_v2(node, c))
                ladd_obj_no_dup(loop->blocks, c);
        }
    }
}

static void loop_content_with_exit_blocks(jd_method *m,
                                          jd_node *node,
                                          jd_bblock *block,
                                          jd_loop *loop)
{
    jd_bblock *exit_block = lget_obj(loop->exit_to_blocks, 0);
    jd_edge *edge = lget_obj(exit_block->in, 0);
    jd_bblock *source_block = edge->source_block;

    if (!lcontains_obj(loop->blocks, source_block))
        return;
    jd_nblock *snb = source_block->ub->nblock;
    jd_nblock *enb = exit_block->ub->nblock;
    jd_exp *exit_start_exp = lget_obj(m->expressions, enb->start_idx);
    jd_exp *end_exp = lget_obj(m->expressions, snb->end_idx);
    jd_ins *end_ins = end_exp->ins;
    if (end_ins != NULL &&
        end_ins->fn->is_switch(end_ins) &&
        switch_contains_offset(end_ins,
                               exit_start_exp->ins->offset)) {
        compute_dominates_block(m, source_block);
        for (int j = 0; j < source_block->dominates->size; ++j) {
            jd_bblock *c = lget_obj(source_block->dominates, j);
            if (!basic_block_is_normal_live(c))
                continue;
            if (lcontains_obj(block->dominates, c) &&
                node_contains_block(node, c))
                ladd_obj_no_dup(loop->blocks, c);
        }
    }
    else {
        if (lcontains_obj(loop->header->dominates, exit_block)) {
            ladd_obj_no_dup(loop->blocks, exit_block);
        }

//        for (int j = 0; j < loop->header->dominates->size; ++j) {
//            jd_bblock *c = lget_obj(loop->header->dominates, j);
//            if (basic_block_is_normal_live(c) &&
//                node_contains_block(node, c))
//                ladd_obj_no_dup(loop->blocks, c);
//        }
    }
}

static void loop_content_with_preheader(jd_method *m,
                                        jd_node *node,
                                        jd_bblock *block,
                                        jd_loop *loop)
{
    for (int i = 0; i < loop->exit_to_blocks->size; ++i) {
        jd_bblock *exit_block = lget_obj(loop->exit_to_blocks, i);
        jd_edge *edge = lget_obj(exit_block->in, 0);
        jd_bblock *source_block = edge->source_block;

        if (!lcontains_obj(loop->blocks, source_block))
            return;
        jd_nblock *snb = source_block->ub->nblock;
        jd_nblock *enb = exit_block->ub->nblock;
        jd_exp *exit_start_exp = lget_obj(m->expressions, enb->start_idx);
        jd_exp *end_exp = lget_obj(m->expressions, snb->end_idx);
        jd_ins *end_ins = end_exp->ins;
        if (end_ins != NULL &&
            end_ins->fn->is_switch(end_ins) &&
            switch_contains_offset(end_ins,
                                   exit_start_exp->ins->offset)) {
            compute_dominates_block(m, source_block);
            for (int j = 0; j < source_block->dominates->size; ++j) {
                jd_bblock *c = lget_obj(source_block->dominates, j);
                if (!basic_block_is_normal_live(c))
                    continue;
                if (lcontains_obj(block->dominates, c) &&
                    node_contains_block(node, c))
                    ladd_obj_no_dup(loop->blocks, c);
            }
        }
        else {
            if (lcontains_obj(loop->header->dominates, exit_block))
                ladd_obj_no_dup(loop->blocks, exit_block);
        }
    }
}

static void identify_loops_recursive(jd_method *m,
                                     jd_node *node,
                                     bool exclude_head)
{
    for (int i = 0; i < node->children->size; ++i)
    {
        jd_node *child = lget_obj(node->children, i);
        if (!node_is_basic_block(child))
            continue;
        jd_bblock *block = child->data;
        if (
                !basic_block_is_normal_live(block) ||
                (node_is_loop(node) && exclude_head && i == 0) ||
                !lcontains_obj(block->frontier, block)
//                is_loop_exist(m, block)
                )
            continue;

        compute_dominates_block(m, block);

        jd_loop *loop = make_obj(jd_loop);
        loop->header = block;
        loop->blocks = linit_object();
        loop->condition_blocks = linit_object();
        ladd_object(loop->condition_blocks, block);

        basic_block_clear_visited_flag(m);

        find_loop_content(loop, node, block);

        find_loop_condition(m, loop, block);

        find_loop_exit_blocks(loop);

        if (loop->condition == NULL) {
            // for obfuscator
            if (loop->exit_block != NULL) {
                loop_content_with_exit_block(m, node, block, loop);
            }
            else if (loop->exit_to_blocks->size == 1) {
                loop_content_with_exit_blocks(m, node, block, loop);
            }
            else if (loop->exit_to_blocks->size > 1) {
                loop_content_with_preheader(m, node, block, loop);
            }
        }

        setup_loop_ranges(loop);

        if (loop_already_exist(m, loop))
            return;

        ladd_obj(m->loops, loop);

        jd_node *loop_node = loop_to_node(m, node, loop);
        identify_loops_recursive(m, loop_node, true);
    }
}

void identify_loop(jd_method *m)
{
    m->loops = linit_object();

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;

        identify_loops_recursive(m, node, false);
    }
}

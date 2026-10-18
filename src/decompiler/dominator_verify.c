#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decompiler/dominator_tree.h"
#include "decompiler/control_flow.h"
#include "libs/bitset/bitset.h"

#define DOM_NONE ((size_t) -1)

static int verify_enabled(void)
{
    static int enabled = -1;
    if (enabled < 0)
        enabled = getenv("GARLIC_VERIFY_DOM") != NULL;
    return enabled;
}

static void dom_fail(jd_method *m, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[dom-verify] method %s: ",
            m->name != NULL ? m->name : "?");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

static bool bitset_same(const bitset_t *left, const bitset_t *right)
{
    if (left->arraysize != right->arraysize)
        return false;
    return memcmp(left->array, right->array,
                  left->arraysize * sizeof(uint64_t)) == 0;
}

static bool list_contains_block(list_object *list, jd_bblock *block)
{
    for (int i = 0; i < list->size; ++i) {
        if (lget_object(list, i) == block)
            return true;
    }
    return false;
}

static bool edge_exists(jd_bblock *source, jd_bblock *target)
{
    for (int i = 0; i < target->in->size; ++i) {
        jd_edge *edge = lget_object(target->in, i);
        if (edge->source_block == source && edge->target_block == target)
            return true;
    }
    return false;
}

static size_t dominates_in_set(bitset_t **dom, size_t n,
                               size_t block, size_t other)
{
    (void) n;
    return bitset_get(dom[other], block);
}

/*
 * Independent check of the computed dominator tree:
 *  - reachability and edge symmetry
 *  - idom is the strict dominator with the largest dominator set
 *  - dom_children is the reverse idom mapping
 *  - the dominance frontier matches the classic "runner" definition
 *
 * The expected dominators are recomputed here with the standard iterative
 * data-flow equations, so this does not share the implementation it checks.
 */
void verify_dominator_tree(jd_method *m)
{
    if (!verify_enabled())
        return;

    size_t n = m->basic_blocks->size;
    if (n == 0)
        return;

    jd_bblock **by_id = x_alloc(n * sizeof(jd_bblock *));
    memset(by_id, 0, n * sizeof(jd_bblock *));
    for (size_t i = 0; i < n; ++i) {
        jd_bblock *block = lget_object(m->basic_blocks, i);
        if (block->block_id >= n || by_id[block->block_id] != NULL)
            dom_fail(m, "block_id %zu is not a dense index", block->block_id);
        by_id[block->block_id] = block;
    }

    jd_bblock *enter = block_enter(m);
    size_t enter_id = enter->block_id;

    /* reachability from the entry block, following the same out edges the
     * dominator pass walks */
    bool *reach = x_alloc(n * sizeof(bool));
    memset(reach, 0, n * sizeof(bool));
    size_t *stack = x_alloc(n * sizeof(size_t));
    size_t top = 0;
    reach[enter_id] = true;
    stack[top++] = enter_id;
    while (top > 0) {
        size_t id = stack[--top];
        jd_bblock *block = by_id[id];
        for (int i = 0; i < block->out->size; ++i) {
            jd_edge *edge = lget_object(block->out, i);
            jd_bblock *target = edge->target_block;
            if (target == NULL || target->block_id >= n)
                dom_fail(m, "block %zu has a dangling out edge", id);
            if (edge->source_block != block)
                dom_fail(m, "block %zu has an out edge owned by block %zu",
                         id, edge->source_block->block_id);
            if (!edge_exists(block, target))
                dom_fail(m, "edge %zu -> %zu is missing from the target in list",
                         id, target->block_id);
            if (!reach[target->block_id]) {
                reach[target->block_id] = true;
                stack[top++] = target->block_id;
            }
        }
        for (int i = 0; i < block->in->size; ++i) {
            jd_edge *edge = lget_object(block->in, i);
            jd_bblock *source = edge->source_block;
            if (source == NULL || edge->target_block != block)
                dom_fail(m, "block %zu has a malformed in edge", id);
            if (edge->source_block->block_id >= n)
                dom_fail(m, "block %zu has an in edge from an unknown block", id);
            if (!edge_exists(source, block))
                dom_fail(m, "edge %zu -> %zu is missing from the source out list",
                         source->block_id, id);
        }
    }

    bitset_t *all = bitset_create_with_capacity(n);
    for (size_t i = 0; i < n; ++i) {
        if (reach[i])
            bitset_set(all, i);
    }

    bitset_t **dom = x_alloc(n * sizeof(bitset_t *));
    bitset_t **next = x_alloc(n * sizeof(bitset_t *));
    for (size_t i = 0; i < n; ++i) {
        dom[i] = bitset_create_with_capacity(n);
        next[i] = bitset_create_with_capacity(n);
        if (!reach[i])
            continue;
        if (i == enter_id)
            bitset_set(dom[i], i);
        else {
            bitset_inplace_union(dom[i], all);
            bitset_set(dom[i], i);
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < n; ++i) {
            if (!reach[i] || i == enter_id)
                continue;

            jd_bblock *block = by_id[i];
            bitset_clear(next[i]);
            bool first = true;
            for (int j = 0; j < block->in->size; ++j) {
                jd_edge *edge = lget_object(block->in, j);
                jd_bblock *source = edge->source_block;
                if (source == NULL || !reach[source->block_id])
                    continue;
                if (first) {
                    bitset_inplace_union(next[i], dom[source->block_id]);
                    first = false;
                }
                else {
                    bitset_inplace_intersection(next[i],
                                                dom[source->block_id]);
                }
            }
            if (first)
                continue;
            bitset_set(next[i], i);
            if (!bitset_same(dom[i], next[i])) {
                bitset_t *tmp = dom[i];
                dom[i] = next[i];
                next[i] = tmp;
                changed = true;
            }
        }
    }

    size_t *idom = x_alloc(n * sizeof(size_t));
    for (size_t i = 0; i < n; ++i)
        idom[i] = DOM_NONE;

    if (enter->idom != NULL)
        dom_fail(m, "entry block has an idom");

    for (size_t i = 0; i < n; ++i) {
        if (!reach[i] || i == enter_id)
            continue;

        size_t best = DOM_NONE;
        size_t best_count = 0;
        bool tie = false;
        for (size_t d = 0; d < n; ++d) {
            if (d == i || !reach[d] || !bitset_get(dom[i], d))
                continue;
            size_t count = bitset_count(dom[d]);
            if (count > best_count) {
                best = d;
                best_count = count;
                tie = false;
            }
            else if (count == best_count) {
                tie = true;
            }
        }
        if (best == DOM_NONE)
            dom_fail(m, "block %zu has no dominator", i);
        if (tie)
            dom_fail(m, "block %zu has no unique immediate dominator", i);
        idom[i] = best;

        jd_bblock *block = by_id[i];
        size_t actual = block->idom != NULL ? block->idom->block_id : DOM_NONE;
        if (actual != best) {
            if (block->idom == NULL)
                dom_fail(m, "block %zu has no idom, expected %zu", i, best);
            else
                dom_fail(m, "block %zu idom is %zu, expected %zu",
                         i, actual, best);
        }
        if (!dominates_in_set(dom, n, best, i))
            dom_fail(m, "block %zu is not dominated by its idom %zu", i, best);

        if (!list_contains_block(block->idom->dom_children, block))
            dom_fail(m, "block %zu is missing from idom %zu's children",
                     i, best);
    }

    for (size_t i = 0; i < n; ++i) {
        jd_bblock *block = by_id[i];
        for (int j = 0; j < block->dom_children->size; ++j) {
            jd_bblock *child = lget_object(block->dom_children, j);
            if (child->idom != block)
                dom_fail(m, "block %zu lists child %zu with a different idom",
                         i, child->block_id);
        }
        size_t steps = 0;
        jd_bblock *walk = block->idom;
        while (walk != NULL && walk->block_id != enter_id) {
            if (++steps > n)
                dom_fail(m, "block %zu has a cycle in its idom chain", i);
            walk = walk->idom;
        }
        if (walk == NULL && i != enter_id && reach[i])
            dom_fail(m, "block %zu idom chain does not reach the entry", i);
    }

    /* dominance frontier, computed from the verified idom */
    bitset_t **frontier = x_alloc(n * sizeof(bitset_t *));
    for (size_t i = 0; i < n; ++i)
        frontier[i] = bitset_create_with_capacity(n);

    for (size_t i = 0; i < n; ++i) {
        if (!reach[i])
            continue;
        jd_bblock *block = by_id[i];
        size_t stop = idom[i];
        for (int j = 0; j < block->in->size; ++j) {
            jd_edge *edge = lget_object(block->in, j);
            jd_bblock *pred = edge->source_block;
            if (pred == NULL || !reach[pred->block_id])
                continue;
            size_t runner = pred->block_id;
            size_t guard = 0;
            while (runner != stop && runner != DOM_NONE) {
                bitset_set(frontier[runner], i);
                runner = idom[runner];
                if (++guard > n)
                    dom_fail(m, "dominance frontier walk from %zu loops", i);
            }
        }
    }

    for (size_t i = 0; i < n; ++i) {
        if (!reach[i])
            continue;
        jd_bblock *block = by_id[i];
        size_t listed = 0;
        for (int j = 0; j < block->frontier->size; ++j) {
            jd_bblock *target = lget_object(block->frontier, j);
            if (target->block_id >= n || !bitset_get(frontier[i], target->block_id))
                dom_fail(m, "block %zu frontier has an unexpected block %zu",
                         i, target->block_id);
            listed++;
        }
        if (listed != bitset_count(frontier[i]))
            dom_fail(m, "block %zu frontier has %zu entries, expected %zu",
                     i, listed, bitset_count(frontier[i]));
    }
}

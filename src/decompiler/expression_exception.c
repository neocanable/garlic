#include "expression_exception.h"
#include "expression_node.h"
#include "decompiler/control_flow.h"

void optimize_synchronized_variable(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *e = lget_obj(m->expressions, i);
        if (exp_is_nopped(e))
            continue;
        if (exp_is_monitor_enter(e) || exp_is_monitor_exit(e))
            exp_mark_nopped(e);
    }
}

void optimize_exception_block(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (!node_is_exception(node))
            continue;

        jd_node *parent = node->parent;
        int index = lfind_object(parent->children, node);
        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            ladd_obj_at(parent->children, child, index + j);
            child->parent = parent;
        }
        ldel_obj(m->nodes, node);
        ldel_obj(parent->children, node);
        i --;
    }
}

void identify_empty_exception_blocks(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (!node_is_exception(node))
            continue;

        bool has_expression = false;
        for (int j = 0; j < node->children->size; ++j) {
            jd_node *child = lget_obj(node->children, j);
            for (int k = child->start_idx; k <= child->end_idx; ++k) {
                jd_exp *e = get_exp(m, k);
                if (!exp_is_nopped(e)) {
                    has_expression = true;
                }
            }
            if (has_expression) break;
        }

        if (!has_expression) {
            ldel_obj(m->nodes, node);
            i--;
        }
    }
}
static jd_node* find_block_containing(jd_node *node, int idx)
{
    if (node == NULL)
        return NULL;
    if (node_is_basic_block(node))
        return (node->start_idx <= idx && idx <= node->end_idx) ? node : NULL;
    if (node->children == NULL)
        return NULL;
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *found = find_block_containing(lget_obj(node->children, i), idx);
        if (found != NULL)
            return found;
    }
    return NULL;
}

static void collect_normal_reachable(jd_bblock *block, list_object *seen)
{
    if (block == NULL || !basic_block_is_normal_live(block) ||
        lcontains_obj(seen, block))
        return;
    ladd_obj(seen, block);
    for (int i = 0; i < block->out->size; ++i) {
        jd_edge *edge = lget_obj(block->out, i);
        if (edge->type == JD_EDGE_NORMAL)
            collect_normal_reachable(edge->target_block, seen);
    }
}

static void sort_part_children(jd_node *part)
{
    if (part->children == NULL || part->children->size < 2)
        return;

    list_object *groups = linit_object();
    list_object *group = NULL;
    for (int i = 0; i < part->children->size; ++i) {
        jd_node *child = lget_obj(part->children, i);
        if ((node_is_else(child) || node_is_else_if(child)) &&
            group != NULL) {
            ladd_obj(group, child);
        }
        else {
            group = linit_object();
            ladd_obj(group, child);
            ladd_obj(groups, group);
        }
    }

    for (int i = 0; i < groups->size; ++i) {
        for (int j = 0; j + 1 < groups->size - i; ++j) {
            list_object *g1 = lget_obj(groups, j);
            list_object *g2 = lget_obj(groups, j + 1);
            jd_node *n1 = lget_obj_first(g1);
            jd_node *n2 = lget_obj_first(g2);
            if (n1->start_idx > n2->start_idx)
                lswap_obj(groups, j, j + 1);
        }
    }

    list_object *ordered = linit_object();
    for (int i = 0; i < groups->size; ++i) {
        list_object *g = lget_obj(groups, i);
        for (int j = 0; j < g->size; ++j)
            ladd_obj(ordered, lget_obj(g, j));
    }

    for (int i = 0; i < ordered->size; ++i) {
        jd_node *want = lget_obj(ordered, i);
        if (lget_obj(part->children, i) == want)
            continue;
        for (int j = i + 1; j < part->children->size; ++j) {
            if (lget_obj(part->children, j) == want) {
                lswap_obj(part->children, i, j);
                break;
            }
        }
    }
}

static void move_handler_block_to_catch(jd_method *m, jd_node *node,
                                        jd_node *catch_node)
{
    if (!node_is_basic_block(node))
        return;
    jd_bblock *block = node->data;
    if (block == NULL || block->ub == NULL || block->ub->nblock == NULL)
        return;
    jd_nblock *nb = block->ub->nblock;
    for (int i = nb->start_idx; i <= nb->end_idx; ++i) {
        jd_exp *exp = get_exp(m, i);
        if (exp != NULL && exp_is_goto(exp))
            exp_mark_nopped(exp);
    }
    jd_node *parent = node->parent;
    if (parent != NULL)
        ldel_obj(parent->children, node);
    ladd_obj_at(catch_node->children, node, 0);
    node->parent = catch_node;
}

void fix_exception_handler_blocks(jd_method *m)
{
    if (m->nodes == NULL)
        return;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *exception = lget_obj(m->nodes, i);
        if (!node_is_exception(exception))
            continue;
        for (int j = 0; j < exception->children->size; ++j) {
            jd_node *part = lget_obj(exception->children, j);
            if ((node_is_try(part) || node_is_catch(part) ||
                 node_is_finally(part)) && part->children != NULL)
                sort_part_children(part);
        }
        jd_node *try_node = child_of_type(exception, JD_NODE_TRY);
        if (try_node == NULL)
            continue;

        for (int j = 0; j < exception->children->size; ++j) {
            jd_node *catch_node = lget_obj(exception->children, j);
            if (!node_is_catch(catch_node))
                continue;

            jd_node *handler = find_block_containing(try_node,
                                                     catch_node->start_idx);
            if (handler == NULL)
                continue;   // handler already lives under the catch node

            list_object *reachable = linit_object();
            collect_normal_reachable(handler->data, reachable);

            for (int k = 0; k < try_node->children->size; ++k) {
                jd_node *child = lget_obj(try_node->children, k);
                if (node_is_basic_block(child) &&
                    lcontains_obj(reachable, child->data)) {
                    move_handler_block_to_catch(m, child, catch_node);
                    k--;
                }
            }

            for (int k = 0; k < catch_node->children->size; ++k) {
                jd_node *child = lget_obj(catch_node->children, k);
                if (!node_is_basic_block(child))
                    continue;
                jd_bblock *block = child->data;
                if (block == NULL || block->ub == NULL ||
                    block->ub->nblock == NULL)
                    continue;
                if (child->start_idx > try_node->end_idx &&
                    !lcontains_obj(reachable, block)) {
                    ldel_obj(catch_node->children, child);
                    ladd_obj(try_node->children, child);
                    child->parent = try_node;
                }
            }
        }
    }
}

void nop_catch_handler_gotos(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *exception = lget_obj(m->nodes, i);
        if (!node_is_exception(exception))
            continue;

        int starts[32];
        int start_count = 0;
        for (int j = 0; j < exception->children->size &&
                        start_count < 32; ++j) {
            jd_node *child = lget_obj(exception->children, j);
            if (child->type == JD_NODE_CATCH)
                starts[start_count++] = child->start_idx;
        }
        if (start_count == 0)
            continue;

        for (int j = 0; j < exception->children->size; ++j) {
            jd_node *catch_node = lget_obj(exception->children, j);
            if (catch_node->type != JD_NODE_CATCH ||
                catch_node->children == NULL)
                continue;
            for (int k = 0; k < catch_node->children->size; ++k) {
                jd_node *block_node = lget_obj(catch_node->children, k);
                if (!node_is_basic_block(block_node))
                    continue;
                bool handler_start = false;
                for (int s = 0; s < start_count; ++s) {
                    if (block_node->start_idx == starts[s]) {
                        handler_start = true;
                        break;
                    }
                }
                if (!handler_start)
                    continue;
                jd_bblock *block = block_node->data;
                if (block == NULL || block->ub == NULL ||
                    block->ub->nblock == NULL)
                    continue;
                jd_nblock *nb = block->ub->nblock;
                for (int e = nb->start_idx; e <= nb->end_idx; ++e) {
                    jd_exp *exp = get_exp(m, e);
                    if (exp != NULL && exp_is_goto(exp))
                        exp_mark_nopped(exp);
                }
            }
        }
    }
}

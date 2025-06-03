#ifndef GARLIC_EXPRESSION_NODE_H
#define GARLIC_EXPRESSION_NODE_H

#include "decompiler/structure.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node_helper.h"

jd_node* create_expression_node(jd_method *m, jd_exp *exp);

void create_node_tree(jd_method *m);

void print_node_tree(jd_method *m, jd_node *node);

void node_children_sort(jd_node *node);

bool node_contains_expression(jd_node *node, jd_exp *expression);

bool node_is_ancestor_sibling_of(jd_node *node, jd_node *other);

bool node_is_ancestor_of(jd_node *node, jd_node *other);

jd_node* node_of_offset(jd_method *m, uint32_t offset);

bool node_contains_other(jd_node *node, jd_node *other);

// it means that node have no children or perticular children
// exception only have try/catch/finally and can not split
// switch only have case and can't split
// expression is atomic
#define node_is_atomic(node) (node_is_exception(node) || \
                                node_is_expression(node) || \
                                node_is_switch(node) || \
                                node_is_basic_block(node))

jd_node* parent_next_node(jd_node *node);

static inline bool node_contains_block(jd_node *node, jd_bblock *block)
{
    // 这里永远不会出现JD_BLOCK_EXCEPTION
    jd_nblock *_nblock = block->ub->nblock;
    return node->start_idx <= _nblock->start_idx &&
           node->end_idx >= _nblock->end_idx;
}

static inline bool node_contains_block_v2(jd_node *node, jd_bblock *block)
{
//    if (block->is_dup)
//        return true;
    jd_node *bnode = block->node;
    return node_is_ancestor_of(node, bnode);
//    return node->start_idx <= block->ub->nblock->start_idx &&
//           node->end_idx >= block->ub->nblock->end_idx;
}

static inline bool node_need_param(jd_node *node)
{
    switch(node->type) {
        case JD_NODE_CATCH:
        case JD_NODE_SWITCH:
        case JD_NODE_CASE:
        case JD_NODE_SYNCHRONIZED:
        case JD_NODE_IF:
        case JD_NODE_ELSE_IF:
        case JD_NODE_LOOP:
        case JD_NODE_WHILE:
        case JD_NODE_FOR:
        case JD_NODE_DO_WHILE:
            return true;
        default:
            return false;
    }
}

static inline jd_node* node_first_effective_child(jd_node *node)
{
    if (node_is_expression(node)) {
        return node;
    }
    else {
        for (int i = 0; i < node->children->size; ++i) {
            jd_node *b = lget_obj(node->children, i);
            if (node_is_expression(b)) {
                if (!exp_is_nopped((jd_exp *) b->data))
                    return b;
            }
            else
                return b;
        }
    }
    return NULL;
}

static inline jd_node* child_of_type(jd_node *node, jd_node_type t)
{
    for (int i = 0; i < node->children->size; ++i)
    {
        jd_node *child = lget_obj(node->children, i);
        if (child->type == t)
            return child;
    }
    return NULL;
}

static inline int node_valid_children_count(jd_node *node)
{
    int count = 0;
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        if (node_is_expression(child) && 
            exp_is_nopped((jd_exp *) child->data))
            continue;
        ++count;
    }
    return count;
}

static inline int node_scope(jd_node *node)
{
    if (node->parent == NULL)
        return 0;
    jd_node *parent = node->parent;
    int scope = 0;
    while (parent != NULL) {
        ++scope;
        parent = parent->parent;
    }
    return scope;
}

#endif //GARLIC_EXPRESSION_NODE_H

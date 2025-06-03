#include "expression_exception.h"
#include "expression_node.h"

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
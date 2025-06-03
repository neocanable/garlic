#include "decompiler/expression_assert.h"
#include "decompiler/expression_node.h"
#include "decompiler/method.h"
#include "common/str_tools.h"

/**
 * example:
 * if { // block_id: 25  range: 1 - 13  parent_id: 0
 *	 [3 ifne] 0 == AssetTest.$assertionsDisabled : goto => 32
 *	 if_false_block { // block_id: 26  range: 2 - 13  parent_id: 25
 *		 if { // block_id: 27  range: 9 - 13  parent_id: 26
 *			 [21 ifne] var_1_1.equals("bar") || var_1_1.startsWith("foo")
 *			 if_false_block { // block_id: 28  range: 10 - 13  parent_id: 27
 *			    [31 athrow] athrow(new AssertionError())
 *		    }
 *		 }
 *	 }
 * }
 **/

static inline bool exp_is_assert(jd_exp *e)
{
    if (exp_is_put_static(e)) {
        jd_exp_put_static *put_static = e->data;
        if (STR_EQL(put_static->name, "$assertionsDisabled"))
            return true;
    }
    return false;
}

void identify_assert(jd_method *m)
{
    // $assertionsDisabled
    if (!method_is_jvm(m))
        return;

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *outer_if = lget_obj(m->nodes, i);
        if (node_is_not_if(outer_if))
            continue;
        jd_node *fnode = child_of_type(outer_if, JD_NODE_IF_FALSE);
        if (fnode == NULL || outer_if->children->size == 0)
            continue;

        jd_node *outer_if_node_first = lget_obj_first(outer_if->children);
        jd_exp *outer_if_expression = outer_if_node_first->data;
        if (outer_if_expression == NULL)
            continue;
        if ((outer_if_expression != NULL &&
            !if_expression_is_assert(outer_if_expression)) ||
            fnode == NULL)
            continue;
        jd_node *inner_if = child_of_type(fnode, JD_NODE_IF);
        if (inner_if == NULL)
            continue;

        jd_node *inner_if_node_first = lget_obj_first(inner_if->children);
        jd_exp *inner_if_expression = inner_if_node_first->data;
        jd_exp_if *inner_if_exp = inner_if_expression->data;
        jd_exp *condition_expression = inner_if_exp->expression;
        outer_if_expression->type = JD_EXPRESSION_ASSERT;
        outer_if_expression->data = condition_expression;

        jd_node *parent = outer_if->parent;
        int index = lfind_object(parent->children, outer_if);
        ladd_obj_at(parent->children, outer_if_node_first, index);
        outer_if_node_first->parent = parent;
        ldel_obj(parent->children, outer_if);
        ldel_obj(m->nodes, outer_if);
        i --;

        DEBUG_PRINT("[assert finded] exp: %d\n", outer_if_expression->idx);
    }

    if (!method_is_clinit(m))
        return;
    
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_assert(exp)) {
            DEBUG_PRINT("[assert nopped] exp: %d\n", exp->idx);
            exp_mark_nopped(exp);
        }
    }
}

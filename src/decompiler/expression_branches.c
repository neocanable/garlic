#include "jvm/jvm_ins.h"
#include "decompiler/control_flow.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "decompiler/dominator_tree.h"
#include "decompiler/expression_assert.h"
#include "decompiler/expression_logical.h"
#include "common/str_tools.h"
#include "expression_writter.h"

static inline bool basic_block_is_single_enter(jd_bblock *block)
{
    int count = 0;
    for (int i = 0; i < block->in->size; ++i) {
        jd_edge *edge = lget_obj(block->in, i);
        jd_bblock *source = edge->source_block;
        if (!basic_block_is_normal_live(source))
            continue;
        // if (!basic_block_has_effect_expression(source, block))
        //    continue;

        if (!lcontains_obj(block->dominates, source))
            count++;

        if (count > 1)
            return false;
    }
    return count == 1;
}

static int switch_key_by_offset(jd_exp_switch *exp_switch, uint32_t offset)
{
    for (int i = 0; i < exp_switch->targets->size; ++i) {
        jd_switch_param *param = lget_obj(exp_switch->targets, i);
        if (param->offset == offset)
            return param->ikey;
    }
    return -1;
}

static void buble_sort_basic_blocks(list_object *blocks)
{
    int size = blocks->size;
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - 1-i; ++j) {
            jd_bblock *b1 = lget_obj(blocks, j);
            jd_bblock *b2 = lget_obj(blocks, j + 1);
            if (b1->ub->nblock->start_idx - b2->ub->nblock->start_idx > 0)
                lswap_obj(blocks, j, j + 1);
        }
    }
}

static int basic_blocks_max_idx(list_object *blocks)
{
    int max_idx = -1;
    for (int i = 0; i < blocks->size; ++i) {
        jd_bblock *basic_block = lget_obj(blocks, i);
        jd_nblock *nblock = basic_block->ub->nblock;
        if (max_idx < nblock->end_idx)
            max_idx = nblock->end_idx;
    }
    return max_idx;
}

static bool case_target_exist(jd_switch *sw, jd_exp *target_exp)
{
    for (int i = 0; i < sw->cases->size; ++i) {
        jd_case *c = lget_obj(sw->cases, i);
        if (c->start_idx == target_exp->idx)
            return true;
    }
    return false;
}

static bool node_is_loop_start(jd_method *m, jd_node *node)
{
    for (int i = 0; i < m->loops->size; ++i) {
        jd_loop *loop = lget_obj(m->loops, i);
        jd_node *n = loop->node;
        if (n->start_idx == node->start_idx &&
            loop->can_write_condition && !loop->is_post_condition)
            return true;
    }
    return false;
}

static bool if_exp_is_copy_if_true_block(jd_exp *exp)
{
    jd_exp_if *if_exp = exp->data;
    jd_ins *ins = exp->ins;
    return ins != NULL && ins_is_copy_if_true_block(ins);
}

static bool node_is_continue_or_break(jd_method *m, jd_exp *exp, jd_node *t)
{
    // 这里的意思是判断一个if expression是否是一个循环的continue或者break
    // dalvik中有if (condition) 中，if的target直接是break或者continue
    // jvm中大部分编译出来的if是if (condition) goto name
    // loop {
    //     if (condition) break;
    //     if (condition) continue;
    // }
    bool is_loop = false;
    jd_node *loop_node = NULL;
    for (int i = 0; i < m->loops->size; ++i) {
        jd_loop *loop = lget_obj(m->loops, i);
        jd_node *n = loop->node;
        if (n->start_idx == t->start_idx) {
            is_loop = true;
            loop_node = n;
            break;
        }
    }
    if (!is_loop)
        return false;
    if (!node_contains_expression(loop_node, exp))
        return false;
    jd_exp_if *exp_if = exp->data;
    uint32_t jump_offset = exp_if->offset;
    jd_exp *jump_exp = exp_of_offset(m, jump_offset);
    bool is_continue = jump_exp->idx == t->start_idx;
    if (is_continue)
        return true;

    bool is_break = !node_is_ancestor_of(loop_node, t);
    if (is_break)
        return true;

    return false;
}

static bool block_is_loop_condition(jd_method *m, jd_node *n, jd_bblock *b)
{
    if (node_is_loop(n)) {
        jd_loop *loop = NULL;
        for (int i = 0; i < m->loops->size; ++i) {
            jd_loop *l = lget_obj(m->loops, i);
            if (l->node == n) {
                loop = l;
                break;
            }
        }
        return loop != NULL && loop->condition == b;
    }
    return false;
}

static bool branch_exist(jd_method *m, jd_bblock *block)
{
//    jd_node *n = block->node;
//    jd_node *parent = n->parent;
//    if (parent != NULL &&
//        node_is_if(parent) &&
//        lget_obj_first(parent->children) == n) {
//        return true;
//    }

    for (int i = 0; i < m->branches->size; ++i) {
        jd_if_branch *branch = lget_obj(m->branches, i);
        jd_node *node = branch->node;

        if (branch->start_block == block)
            return true;

        for (int j = 0; j < node->children->size; ++j) {
            jd_node *c = lget_obj(node->children, j);
            if (c->type == JD_NODE_BASIC_BLOCK && c == block->node)
                return true;
        }
        /**
            jd_node *first_child = lget_obj_first(node->children);
            if (first_child == block->node)
                return true;
        */
    }

    return false;
}

static bool switch_exist(jd_method *m, jd_bblock *block)
{
    jd_node *n = block->node;
    jd_node *parent = n->parent;
    if (parent != NULL &&
        node_is_switch(parent) &&
        lget_obj_first(parent->children) == n) {
        return true;
    }
    return false;
}

static int switch_max_idx(jd_switch *sw)
{
    int max_idx = sw->start_idx;
    for (int i = 0; i < sw->cases->size; ++i) {
        jd_case *c = lget_obj(sw->cases, i);
        if (c->start_idx == 0)
            continue;
        if (c->end_idx > max_idx)
            max_idx = c->end_idx;
    }
    return max_idx;
}

static void remove_single_true_node(jd_method *m)
{
    /**
     * 这里先不用这个函数, if_branch里面只有一个true的情况, 一般是if-else结构
     **/
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_if(node) && node_is_not_else_if(node))
            continue;
        jd_node *if_true_node = child_of_type(node, JD_NODE_IF_TRUE);

        if (if_true_node == NULL || node_valid_children_count(node) != 3)
            continue;

        jd_node *first_node = node_first_effective_child(node);
        jd_exp *first_exp = first_node->data;
        assert(exp_is_if(first_exp));
        jd_exp_if *first_if = first_exp->data;
        make_logic_not(first_if->expression);

        size_t index = lfind_object(node->children, if_true_node);
        ldel_obj(node->children, if_true_node);
        for (int j = 0; j < if_true_node->children->size; ++j) {
            jd_node *child = lget_obj(if_true_node->children, j);
            ladd_obj_at(node->children, child, index + j);
            child->parent = node;
        }
    }
}

static void remove_single_false_node(jd_method *m)
{
    // TODO: there are lots of magic number here, need explain
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_if(node) && node_is_not_else_if(node))
            continue;
        jd_node *if_false_node = child_of_type(node, JD_NODE_IF_FALSE);
        // if_false 块在处理完成后，应该只剩下if expression和if_false两个
        if (if_false_node == NULL || node_valid_children_count(node) != 2)
            continue;
        size_t index = lfind_object(node->children, if_false_node);
        ldel_obj(node->children, if_false_node);
        for (int j = 0; j < if_false_node->children->size; ++j) {
            jd_node *child = lget_obj(if_false_node->children, j);
            ladd_obj_at(node->children, child, index + j);
            child->parent = node;
        }
    }
}

static void remove_single_node_of_if_branch(jd_method *m)
{
    remove_single_false_node(m);

    // remove_single_true_node(m);
}

void remove_empty_if_else_of_method(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *if_node = lget_obj(m->nodes, i);
        if (node_is_not_if(if_node))
            continue;
        jd_node *parent = if_node->parent;
        jd_node *else_if = child_of_type(parent, JD_NODE_ELSE_IF);
        if (else_if != NULL && else_if->start_idx > if_node->start_idx &&
            else_if->end_idx <= if_node->end_idx)
            continue;
        jd_node *else_node = child_of_type(parent, JD_NODE_ELSE);
        if (else_node == NULL)
            continue;

        if (node_valid_children_count(else_node) == 0) {
            ldel_obj(parent->children, else_node);
        }

        if (node_valid_children_count(if_node) == 1) {
             ldel_obj(parent->children, else_node);
             for (int j = 0; j < else_node->children->size; ++j) {
                 jd_node *c = lget_obj(else_node->children, j);
                 ladd_obj(if_node->children, c);
                 c->parent = if_node;
             }
             jd_exp *if_exp = get_exp(m, if_node->end_idx);
             if (!exp_is_if(if_exp)) {
                 DEBUG_PRINT("[remove empty if error] -------->\n");
                 continue;
             }
             jd_exp_if *exp_if = if_exp->data;
             jd_exp *condition = exp_if->expression;
             make_logic_not(condition);
         }
    }
}

void identify_else_if_of_method(jd_method *m)
{
    /**
     * 识别else if 结构
     * 方法：如果if_true_block的起始语句就是一个if语句
     * 那么就是一个else if结构
     **/

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_if(node) && node_is_not_else_if(node))
            continue;

        jd_node *if_true_node = child_of_type(node, JD_NODE_IF_TRUE);
        jd_node *if_false_node = child_of_type(node, JD_NODE_IF_FALSE);
        if (if_true_node == NULL || if_false_node == NULL)
            continue;

        jd_node *first_child = lget_obj_first(if_true_node->children);
        jd_node *last_child = lget_obj_last(if_true_node->children);
        if (first_child == NULL ||
            !node_is_if(first_child) ||
            first_child != last_child)
            continue;

//        jd_exp *exp = get_exp(m, first_child->start_idx);
        jd_exp *exp = next_valid_exp(m, first_child->start_idx);
//        if (exp_is_nopped(exp))
//            exp = next_valid_exp(m, exp->idx);
        if (!exp_is_if(exp))
            continue;

        if_true_node->type = JD_NODE_ELSE_IF;
        jd_node *parent = if_true_node->parent;
        jd_node *grand = parent->parent;

        ldel_obj(parent->children, if_true_node);
        size_t index = lfind_object(grand->children, parent);
        ladd_obj_at(grand->children, if_true_node, index + 1);
        if_true_node->parent = grand;

        // remove first_child of if_true_block
        // must delete from m->nodes
        ldel_obj(if_true_node->children, first_child);
        ldel_obj(m->nodes, first_child);
        i--;
        for (int j = 0; j < first_child->children->size; ++j) {
            jd_node *b = lget_obj(first_child->children, j);
            ladd_obj(if_true_node->children, b);
            b->parent = if_true_node;
        }
    }
}

void identify_else_of_method(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_if(node) && node_is_not_else_if(node))
            continue;

        jd_node *if_true_node = child_of_type(node, JD_NODE_IF_TRUE);
        jd_node *if_false_node = child_of_type(node, JD_NODE_IF_FALSE);
        if (if_true_node == NULL)
            continue;

        // then make if_true_block as else block
        if_true_node->type = JD_NODE_ELSE;
        jd_node *grand = node->parent;

        ldel_obj(node->children, if_true_node);
        size_t index = lfind_object(grand->children, node);
        ladd_obj_at(grand->children, if_true_node, index + 1);
        if_true_node->parent = grand;
    }
}

static jd_node* basic_blocks_to_node(list_object *blocks)
{
    jd_node *node = make_obj(jd_node);
    int32_t min_start_idx   = -1;
    int32_t max_end_idx     = -1;
    for (int i = 0; i < blocks->size; ++i) {
        // TODO: 这里是有错误的
        // 如果exception的try和catch是不相连的，就会出现bug
        jd_bblock *basic_block = lget_obj(blocks, i);
        jd_node *n = basic_block->node;
        if (max_end_idx == -1 || max_end_idx < n->end_idx)
            max_end_idx = n->end_idx;
        if (min_start_idx == -1 || min_start_idx > n->start_idx)
            min_start_idx = n->start_idx;
    }
    node->start_idx = min_start_idx;
    node->end_idx = max_end_idx;

    return node;
}

void add_basic_blocks_to_node(jd_method *m,
                              jd_node *parent,
                              jd_node *node,
                              list_object *blocks,
                              jd_bblock *skip)
{
    for (int i = 0; i < blocks->size; ++i) {
        jd_bblock *block = lget_obj(blocks, i);
        jd_node *n = block->node;
        if (block == skip)
            continue;
        if (!node_is_ancestor_of(parent, n))
            continue;

        if (n->parent != parent) {
            while (n->parent != NULL &&
                    n->parent != parent) {
                    n = n->parent;
                    DEBUG_PRINT("parent_id: %d -> %s, "
                                "current: %d -> %s, "
                                "child_id: %d -> %s\n",
                                parent->node_id,
                                node_name(parent),
                                node->node_id,
                                node_name(node),
                                n->node_id,
                                node_name(n));
            }
            if (n == node)
                continue;
        }
        ldel_obj(parent->children, n);
        ladd_obj_no_dup(node->children, n);
        n->parent = node;
    }
    ladd_obj(parent->children, node);
    node->parent = parent;
}

static jd_node* if_branch_to_node(jd_method *m,
                                  jd_node *parent,
                                  jd_if_branch *branch)
{

    jd_node *if_node = make_obj(jd_node);
    if_node->type = JD_NODE_IF;
    if_node->children = linit_object();
    if_node->start_idx = branch->if_start_idx;
    if_node->node_id = m->nodes->size;
    ladd_obj(m->nodes, if_node);
    int max_exp_idx = 0;
    jd_node *if_true_node = NULL;
    jd_node *if_false_node = NULL;

    /**
     * 这里的逻辑应该是这样的：
     *      找到if表达式的第一个basic block
     *      1. 如果if语句是第一个有效的expression，那么可以直接处理
     *      2. 如果if语句不是第一个有效的expression，那么需要将前面所有的expression移动到parent里
     *          并且将除if语句外的expression都创建一个node
     **/

    jd_bblock *block = branch->start_block;
    jd_nblock *nb = block->ub->nblock;
    add_basic_blocks_to_node(m, parent, if_node, branch->blocks, block);
    ldel_obj(parent->children, block->node);
    ldel_obj(m->nodes, block->node);

    for (int i = nb->start_idx; i < nb->end_idx ; ++i) {
        jd_exp *e = get_exp(m, i);
        if (exp_is_nopped(e) || exp_is_empty(e))
            continue;

        jd_node *exp_node = create_expression_node(m, e);
        exp_node->parent = parent;
        ladd_obj(parent->children, exp_node);
    }

    jd_exp *last = get_exp(m, nb->end_idx);
    jd_node *last_exp_node = create_expression_node(m, last);
    last_exp_node->parent = if_node;
    ladd_obj(if_node->children, last_exp_node);


    if (!is_list_empty(branch->true_blocks)) {
        if_true_node = basic_blocks_to_node(branch->true_blocks);
        if_true_node->type = JD_NODE_IF_TRUE;
        if_true_node->children = linit_object();
        if_true_node->node_id = m->nodes->size;
        branch->true_start_idx = if_true_node->start_idx;
        branch->true_end_idx = if_true_node->end_idx;
        max_exp_idx = if_true_node->end_idx;

        add_basic_blocks_to_node(m,
                                 if_node,
                                 if_true_node,
                                 branch->true_blocks,
                                 NULL);
        node_children_sort(if_true_node);
        ladd_obj(m->nodes, if_true_node);
    }

    if (!is_list_empty(branch->false_blocks)) {
        if_false_node = basic_blocks_to_node(branch->false_blocks);
        if_false_node->type = JD_NODE_IF_FALSE;
        if_false_node->children = linit_object();
        if_false_node->node_id = m->nodes->size;
        branch->false_start_idx = if_false_node->start_idx;
        branch->false_end_idx = if_false_node->end_idx;
        if (max_exp_idx < if_false_node->end_idx)
            max_exp_idx = if_false_node->end_idx;

        add_basic_blocks_to_node(m,
                                 if_node,
                                 if_false_node,
                                 branch->false_blocks,
                                 NULL);
        node_children_sort(if_false_node);
        ladd_obj(m->nodes, if_false_node);
    }

    node_children_sort(if_node);
    branch->node = if_node;
    if_node->end_idx = max_exp_idx;
    return if_node;
}

static jd_node* switch_to_node(jd_method *m, jd_node *parent, jd_switch *sw)
{
    jd_node *switch_node = make_obj(jd_node);
    switch_node->type = JD_NODE_SWITCH;
    switch_node->children = linit_object();
    switch_node->start_idx = sw->start_idx;
    switch_node->end_idx = sw->end_idx;
    switch_node->node_id = m->nodes->size;
    ladd_obj(m->nodes, switch_node);

    add_basic_blocks_to_node(m,
                             parent,
                             switch_node,
                             sw->blocks,
                             sw->switch_block);

    jd_bblock *block = sw->switch_block;
    jd_nblock *nb = block->ub->nblock;
    ldel_obj(parent->children, block->node);
    ldel_obj(m->nodes, block->node);



    for (int i = nb->start_idx; i < nb->end_idx; ++i) {
        jd_exp *e = get_exp(m, i);
        if (exp_is_nopped(e) || exp_is_empty(e))
            continue;
        jd_node *exp_node = create_expression_node(m, e);
        ladd_obj(parent->children, exp_node);
    }
    jd_exp *last = get_exp(m, nb->end_idx);
    jd_node *last_exp_node = create_expression_node(m, last);
    ladd_obj(switch_node->children, last_exp_node);


    for (int i = 0; i < sw->cases->size; ++i) {
        jd_case *c = lget_obj(sw->cases, i);
        if (c->blocks == NULL)
            continue;

        jd_node *case_node = make_obj(jd_node);
        case_node->type = JD_NODE_CASE;
        case_node->children = linit_object();
        case_node->start_idx = c->start_idx;
        case_node->end_idx = c->end_idx;
        case_node->node_id = m->nodes->size;
        case_node->data = c;
        add_basic_blocks_to_node(m,
                                 switch_node,
                                 case_node,
                                 c->blocks,
                                 NULL);
        ladd_obj(m->nodes, case_node);
    }
    node_children_sort(switch_node);
    return switch_node;
}

static bool identify_branches_recursive(jd_method *m, jd_node *node);

static void debug_switch_target_offset(jd_method *m, jd_exp *exp, jd_ins *ins)
{
    jd_exp_switch *exp_switch = exp->data;
    for (int i = 0; i < ins->targets->size; ++i) {
        jd_ins *target_start_ins = lget_obj(ins->targets, i);
        uint32_t jump_off = target_start_ins->offset;
        int key = switch_key_by_offset(exp_switch, jump_off);
        printf("target_start_ins: %d offset: %d\n", 
                key, target_start_ins->offset);
    }
}

static bool identify_switch_branches(jd_method *m, jd_node *node, jd_bblock *b)
{
    jd_node *bnode = b->node;
    jd_exp *end_exp = lget_obj(m->expressions, bnode->end_idx);
    jd_ins *end_ins = end_exp->ins;
    if (switch_exist(m, b))
        return false;

    jd_switch *sw = make_obj(jd_switch);
    sw->switch_block = b;
    sw->cases = linit_object();
    sw->blocks = linit_object();
    ladd_obj(sw->blocks, b);
    sw->start_idx = end_exp->idx;
    jd_exp_switch *exp_switch = end_exp->data;
    uint32_t default_offset = exp_switch->default_offset;
    jd_exp *default_exp = exp_of_offset(m, default_offset);
    jd_bblock *default_block = exp_block(default_exp);
    jd_bblock *dominates_block = NULL;

    int default_block_after_switch = 0;
    for (int j = 0; j < end_ins->targets->size; ++j) {
        jd_ins *target_start_ins = lget_obj(end_ins->targets, j);
        jd_exp *target_start_exp = target_start_ins->expression;
        jd_bblock *target = exp_block(target_start_exp);
        if (target == NULL || target_start_ins->offset == default_offset)
            continue;

        /**
         * case 7:
         * case 8:
         * case 9: {
         *      bla bla bla
         * }
         **/
        if (case_target_exist(sw, target_start_exp))
            continue;

        jd_case *case_block = make_obj(jd_case);
        uint32_t jump_off = target_start_ins->offset;
        case_block->key = switch_key_by_offset(exp_switch, jump_off);

        if (target_start_ins->offset < end_ins->offset) {
            DEBUG_PRINT("[switch] case b jump "
                        "goto_offset %d < switch end ins "
                        "goto_offset %d, empty it\n",
                        target_start_ins->offset, end_ins->offset);
            ladd_obj(sw->cases, case_block);
            continue;
        }

        if (default_block != NULL &&
            lcontains_obj(target->frontier, default_block))
            default_block_after_switch = 1;

        case_block->blocks = linit_object();
        case_block->start_idx = target_start_exp->idx;
        case_block->block = target;
        compute_dominates_block(m, target);
        for (int k = 0; k < target->dominates->size; ++k) {
            dominates_block = lget_obj(target->dominates, k);
            if (!basic_block_is_normal_live(dominates_block))
                continue;
            ladd_obj_no_dup(sw->blocks, dominates_block);
            ladd_obj_no_dup(case_block->blocks, dominates_block);
        }

        case_block->end_idx = basic_blocks_max_idx(case_block->blocks);
        ladd_obj(sw->cases, case_block);
    }

    if (!default_block_after_switch && default_block != NULL) {
        jd_case *default_case = make_obj(jd_case);
        default_case->blocks = linit_object();
        default_case->block = default_block;
        default_case->start_idx = default_block->ub->nblock->start_idx;
        default_case->is_default = 1;
        compute_dominates_block(m, default_block);
        for (int k = 0; k < default_block->dominates->size; ++k) {
            dominates_block = lget_obj(default_block->dominates, k);
            if (!basic_block_is_normal_live(dominates_block))
                continue;
            ladd_obj_no_dup(sw->blocks, dominates_block);
            ladd_obj_no_dup(default_case->blocks, dominates_block);
        }
        default_case->end_idx = basic_blocks_max_idx(default_case->blocks);
        sw->default_case = default_case;
        ladd_obj(sw->cases, default_case);
    }

    sw->end_idx = switch_max_idx(sw);
    sw->break_idx = sw->end_idx + 1;
    ladd_obj(m->switches, sw);

    jd_node *switch_node = switch_to_node(m, node, sw);
    jd_node *tmp_b = NULL;
    for (int j = 0; j < switch_node->children->size; ++j)
    {
        tmp_b = lget_obj(switch_node->children, j);
        if (node_is_atomic(tmp_b))
            continue;
        identify_branches_recursive(m, tmp_b);
    }
    return true;
}

static jd_if_branch* make_if_branch(jd_bblock *block)
{
    jd_node *node = block->node;
    jd_if_branch *branch = make_obj(jd_if_branch);
    branch->if_start_idx = node->start_idx;
    branch->blocks = linit_object();
    branch->true_blocks = linit_object();
    branch->false_blocks = linit_object();
    branch->start_block = block;
    ladd_obj_no_dup(branch->blocks, block);
    return branch;
}

static bool identify_if_branches(jd_method *m, jd_node *node, jd_bblock *block)
{
    jd_node *bnode = block->node;
    jd_exp *exp = get_exp(m, bnode->end_idx);

    if (branch_exist(m, block))
        return false;

    jd_exp_if *if_exp = exp->data;
    jd_ins *end_ins = exp->ins;

    // if statement
    uint32_t jump_offset = if_exp->offset;
    jd_exp *jump_exp = exp_of_offset(m, jump_offset);
    jd_ins *next_ins = end_ins->next;
    jd_exp *next_exp = next_ins->expression;
    jd_bblock *true_block = jump_exp->block;
    jd_bblock *false_block = next_exp->block;

//    jd_bblock *true_block = NULL;
//    jd_bblock *false_block = NULL;
//    for (int i = 0; i < block->out->size; ++i) {
//        jd_edge *edge = lget_obj(block->out, i);
//        jd_bblock *target = edge->target_block;
//        if (!basic_block_is_normal_live(target)) continue;
//
//        if (target->ub->nblock->start_offset == if_exp->offset)
//            true_block = target;
//        else
//            false_block = target;
//    }

    jd_node *true_node = true_block->node;
    jd_node *false_node = false_block->node;

    if (true_block == false_block)
        return false;

    jd_if_branch *branch = make_if_branch(block);
    branch->if_start_idx = exp->idx;

    jd_bblock *dom_block = NULL;
    if (false_block != NULL) {
        compute_dominates_block(m, false_block);
    }

    if (false_block != NULL
        && basic_block_is_single_enter(false_block)
        ) {
        for (int j = 0; j < false_block->dominates->size; ++j) {
            dom_block = lget_obj(false_block->dominates, j);
            if (!basic_block_is_normal_live(dom_block))
                continue;
            if (node_contains_block_v2(node, dom_block)) {
                ladd_obj_no_dup(branch->false_blocks, dom_block);
                ladd_obj_no_dup(branch->blocks, dom_block);
            }
        }
    }

    if (true_block != NULL &&
        !node_is_continue_or_break(m, exp, true_node))
        compute_dominates_block(m, true_block);
    if (true_block != NULL &&
        basic_block_is_single_enter(true_block) &&
        !node_is_continue_or_break(m, exp, true_node) &&
        !if_exp_is_copy_if_true_block(exp)) {

        for (int j = 0; j < true_block->dominates->size; ++j) {
            dom_block = lget_obj(true_block->dominates, j);
            if (!basic_block_is_normal_live(dom_block))
                continue;
            if (node_contains_block_v2(node, dom_block)) {
                ladd_obj_no_dup(branch->true_blocks, dom_block);
                ladd_obj_no_dup(branch->blocks, dom_block);
            }
        }
    }

    jd_node *if_node = if_branch_to_node(m, node, branch);
    ladd_obj(m->branches, branch);

    for (int j = 0; j < if_node->children->size; ++j) {
        jd_node *child = lget_obj(if_node->children, j);
        if (node_is_atomic(child))
            continue;
        identify_branches_recursive(m, child);
    }
    return true;
}

static bool identify_branches_recursive(jd_method *m, jd_node *node)
{
    bool founded = false;
    bool result = false;
    do {
        founded = false;
        for (int i = 0; i < node->children->size; ++i)
        {
            jd_node *child = lget_obj(node->children, i);
            if (!node_is_basic_block(child))
                continue;

            jd_bblock *basic_block = child->data;
            if (!basic_block_is_normal_live(basic_block) ||
                block_is_loop_condition(m, node, basic_block) ||
                node_is_loop_start(m, child))
                continue;

            jd_exp *end_exp = lget_obj(m->expressions, child->end_idx);

            if (exp_is_if(end_exp) && !exp_is_nopped(end_exp)) {
                founded = identify_if_branches(m, node, basic_block);
            }

            if (exp_is_switch(end_exp) && !exp_is_nopped(end_exp)) {
                if (switch_exist(m, basic_block))
                    continue;
                founded = identify_switch_branches(m, node, basic_block);
            }

            result |= founded;
            if (founded)
                break;
        }
    } while (founded);

    node_children_sort(node);
    return result;
}

static void identify_if_and_switch(jd_method *m)
{
    m->switches = linit_object();
    m->branches = linit_object();
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_atomic(node))
            continue;
        bool founded = identify_branches_recursive(m, node);
        if (founded) {
            i--;
        }
    }
}

void identify_branches(jd_method *m)
{
    identify_if_and_switch(m);

    identify_assert(m);

    identify_else_if_of_method(m);

    identify_else_of_method(m);

    remove_single_node_of_if_branch(m);
}



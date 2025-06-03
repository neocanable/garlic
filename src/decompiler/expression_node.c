#include "common/debug.h"
#include "jvm/jvm_ins.h"
#include "decompiler/expression.h"
#include "decompiler/expression_writter.h"
#include "decompiler/expression_node.h"
#include "control_flow.h"

#include <time.h>


/**
 * 这里极其蹩脚
 * try: 18 -> 32
 *	catch: 35 -> 77
 *	catch: 276 -> 495
 *
 * 这样的exception是混淆后的，控制流已经被打乱了
 * 解决方法就是try就是一个exception的开始，一个exception不能有其他的child
 * 只允许有try, catch, finally, 但是try/catch/finally里面可以有exception
 * exception 类型的block是一种特殊的block
 **/

static int quick_sort_partition(jd_node *node, int low, int high)
{
    jd_node *b = lget_obj(node->children, node->children->size - 1);
    int pivot = b->start_idx;
    int i = (low - 1);

    for (int j = low; j < high; j++) {
        jd_node *b1 = lget_obj(node->children, j);
        if (b1->start_idx <= pivot) {
            i ++;
            lswap_obj(node->children, i, j);
        }
    }
    size_t index1 = i+1;
    size_t index2 = node->children->size-1;
    lswap_obj(node->children, index1, index2);
    return (i + 1);
}

void quick_sort_node_children(jd_node *node, int low, int high) {
    if (low < high) {
        int pi = quick_sort_partition(node, low, high);
        quick_sort_node_children(node, low, pi - 1);
        quick_sort_node_children(node, pi + 1, high);
    }
}

void node_children_sort(jd_node *node)
{
    int size = node->children->size;
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - 1-i; ++j) {
            jd_node *e1 = lget_obj(node->children, j);
            jd_node *e2 = lget_obj(node->children, j + 1);

            if (e1->start_idx - e2->start_idx > 0)
                lswap_obj(node->children, j, j + 1);

//            if (e1->node_id - e2->node_id > 0)
//                lswap_obj(node->children, j, j + 1);
        }
    }
}

jd_node* parent_next_node(jd_node *node)
{
    if (node->node_id == 0)
        return NULL;
    jd_node *parent = node->parent;
    int index = lfind_object(parent->children, node);
    if (index == parent->children->size - 1)
        return NULL;
    jd_node *next = lget_obj(parent->children, index + 1);
    if (node_is_else(next) ||
        node_is_else_if(next)) {
        return parent_next_node(next);
    }
    return lget_obj(parent->children, index + 1);
}

static void reorder_nodes(jd_method *m, jd_node *node)
{
    // 每个parent下面的block按照start_offset排序
    // 确定每个block的next和prev
    if (node == NULL)
        node = lget_obj(m->nodes, 0);

    node_children_sort(node);
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        if (node_is_not_expression(child))
            reorder_nodes(m, child);
    }
}

static int node_none_empty_child_count(jd_node *node)
{
    int count = 0;
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        if (node_is_expression(child) &&
            !exp_is_nopped((jd_exp *) child->data)) {
            count++;
            continue;
        }
        else if (node_is_not_expression(child) && 
                child->type != JD_NODE_DELETED) {
            count ++;
        }
    }
    return count;
}

static void remove_empty_node(jd_method *m, jd_node *node)
{

    if (node == NULL)
        node = lget_obj(m->nodes, 0);
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        if (node_is_not_expression(child))
            remove_empty_node(m, child);
    }

    if (node_none_empty_child_count(node) == 0) {
        node->type = JD_NODE_DELETED;
    }
}

static u4 expression_end_index(jd_method *m, jd_exp *expression)
{
    if (exp_is_assignment(expression)) {
        jd_exp *next = get_exp(m, expression->idx + 1);
        if (next != NULL && !exp_is_define_stack_var(next)) {
            return expression->idx;
        }
        else {
            while (next != NULL && exp_is_define_stack_var(next)) {
                next = get_exp(m, next->idx + 1);
            }
            return next->idx;
        }
    }
    else
        return expression->idx;
}

static jd_node* create_root_node(jd_method *m)
{
    jd_node *root = make_obj(jd_node);
    root->node_id = 0;
    root->type = JD_NODE_METHOD_ROOT;
    root->start_idx = 0;
    root->parent = NULL;

    size_t index = m->expressions->size - 1;
    jd_exp *last = lget_obj(m->expressions, index);
    root->end_idx = last->idx;
    root->children = linit_object();
    ladd_obj(m->nodes, root);
    return root;
}

jd_node* create_expression_node(jd_method *m, jd_exp *exp)
{
    jd_node *node = make_obj(jd_node);
    node->type = JD_NODE_EXPRESSION;
    node->start_idx = exp->idx;
    node->end_idx = exp->idx;
    node->node_id = m->nodes->size;
    node->data = exp;
    exp->node = node;
    node->method = m;
    ladd_obj(m->nodes, node);
    return node;
}

jd_node* create_new_node(jd_method *m, jd_node_type type, int start, int end)
{
    jd_node *node = make_obj(jd_node);
    node->type = type;
    node->start_idx = start;
    node->end_idx = end;
    node->node_id = m->nodes->size;
    node->children = linit_object();
    node->method = m;
    ladd_obj(m->nodes, node);
    return node;
}

bool node_is_ancestor_of(jd_node *node, jd_node *other)
{
    jd_node *parent = other->parent;
    while (parent != NULL) {
        if (parent == node)
            return true;
        parent = parent->parent;
    }
    return false;
}

bool node_is_ancestor_sibling_of(jd_node *node, jd_node *other)
{
    int scope = node_scope(node);
    jd_node *parent = other->parent;
    while (parent != NULL) {
        if (node_scope(parent) == scope && parent->parent == node->parent)
            return true;
        parent = parent->parent;
    }
    return false;
}

bool node_contains_expression(jd_node *node, jd_exp *expression)
{
    // 这里确定block必须是连续的, block的type不能是exception
    return node->start_idx <= expression->idx &&
           node->end_idx >= expression->idx;
}

static inline jd_range* exception_to_node_range(jd_method *m, jd_range *r)
{
    jd_range *new_range = make_obj(jd_range);
    jd_ins *start_ins = get_ins(m, r->start_idx);
    jd_ins *end_ins = get_ins(m, r->end_idx);
    jd_exp *start_exp = start_ins->expression;
    jd_exp *end_exp = end_ins->expression;
    new_range->start_idx = start_exp->idx;
    new_range->end_idx = expression_end_index(m, end_exp);
    return new_range;
}

static bool node_contains_exception_block(jd_node *b, jd_node *eb)
{
    assert(b->type != JD_NODE_EXCEPTION && eb->type == JD_NODE_EXCEPTION);
    return b->start_idx <= eb->start_idx && b->end_idx >= eb->end_idx;
}

static jd_node* find_exception_parent_node(jd_method *m, jd_node *node)
{
    jd_node *result = NULL;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *b = lget_obj(m->nodes, i);
        if (node_is_exception(b) ||
            node_is_method_root(b) ||
            node_is_basic_block(b) ||
            b->start_idx > node->start_idx)
            continue;

        if (node_contains_exception_block(b, node) &&
            b->parent != node) {
            if (result == NULL || 
                (b->start_idx >= result->start_idx &&
                b->end_idx <= result->end_idx))
                result = b;
        }
    }
    return result;
}

static list_object* basic_blocks_of_node(jd_method *m, jd_node *node)
{
    list_object *blocks = linit_object();
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *n = lget_obj(m->nodes, i);

        if (node_is_not_basic_block(n))
            continue;
        if (node->start_idx <= n->start_idx &&
            node->end_idx >= n->end_idx) {
            jd_bblock *block = n->data;
            ladd_obj_no_dup(blocks, block);
        }
    }

    return blocks;
}

static void add_basic_blocks(jd_method *m, jd_node *node, list_object *blocks)
{
    jd_node *root = lget_obj_first(m->nodes);
    for (int i = 0; i < blocks->size; ++i) {
        jd_bblock *block = lget_obj(blocks, i);
        jd_node *n = block->node;
        if (n->parent != root)
            continue;
        ldel_obj(n->parent->children, n);
        n->parent = node;
        ladd_obj(node->children, n);
    }
}

static void create_basic_block_node(jd_method *m)
{
    jd_node *root = lget_obj(m->nodes, 0);
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (!basic_block_is_normal_live(b))
            continue;
        jd_nblock *nb = b->ub->nblock;
        jd_node *node = make_obj(jd_node);
        node->type = JD_NODE_BASIC_BLOCK;
        // basic block存的是instruction的idx
//        node->start_idx = nb->start_idx;
        jd_ins *start_ins = get_ins(m, nb->start_idx);
        jd_ins *end_ins = get_ins(m, nb->end_idx);
        jd_exp *start_exp = start_ins->expression;
        jd_exp *end_exp = end_ins->expression;

        // TODO: fix some bugs

        // if (ins_is_copy_block(end_ins)) {
        //    list_object *list = end_ins->extra;
        //    if (list->size > 0) {
        //        jd_ins *first = lget_obj_first(list);
        //        start_exp = first->expression;
        //    }
        // }

        node->start_idx = start_exp->idx;
        node->end_idx = end_exp->idx;
        node->parent = root;
        node->node_id = m->nodes->size;
        node->children = linit_object();
        node->method = m;
        node->data = b;
        b->node = node;
        ladd_obj(m->nodes, node);
        ladd_obj(root->children, node);
    }
}

static void create_exception_node(jd_method *m)
{
    jd_node *root = lget_obj(m->nodes, 0);
    int max_idx = 0;
    for (int i = 0; i < m->mix_exceptions->size; ++i) {
        jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
        jd_range *new_try_range = exception_to_node_range(m, e->try);
        jd_node *expcetion = create_new_node(m, JD_NODE_EXCEPTION,
                                             new_try_range->start_idx,
                                             0);

        jd_node *try_node = create_new_node(m, JD_NODE_TRY,
                                            new_try_range->start_idx,
                                            new_try_range->end_idx);
        max_idx = new_try_range->end_idx;

        try_node->parent = expcetion;
        ladd_obj(expcetion->children, try_node);

        if (!is_list_empty(e->catches)) {
            for (int j = 0; j < e->catches->size; ++j) {
                jd_range *r = lget_obj(e->catches, j);
                jd_range *nr = exception_to_node_range(m, r);
                jd_node *catch_node = create_new_node(m,
                                                      JD_NODE_CATCH,
                                                      nr->start_idx,
                                                      nr->end_idx);
                max_idx = nr->end_idx > max_idx ? nr->end_idx : max_idx;
                catch_node->parent = expcetion;
                ladd_obj(expcetion->children, catch_node);
            }
        }

        if (e->finally != NULL) {
            jd_range *nfr = exception_to_node_range(m, e->finally);
            jd_node *finally_node = create_new_node(m,
                                                    JD_NODE_FINALLY,
                                                    nfr->start_idx,
                                                    nfr->end_idx);
            max_idx = nfr->end_idx > max_idx ? nfr->end_idx : max_idx;
            finally_node->parent = expcetion;
            ladd_obj(expcetion->children, finally_node);
        }
        expcetion->end_idx = max_idx;
    }

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_method_root(node))
            continue;
        if (node_is_not_exception(node))
            continue;

        jd_node *parent = find_exception_parent_node(m, node);
        if (parent == NULL)
            parent = root;

        DEBUG_EXCEPTION_PRINT("[code_block] parent %d to %d, "
                              "block: %s parent: %s\n",
                              node->node_id,
                              parent->node_id,
                              node_name(node),
                              node_name(parent));

        ladd_obj(parent->children, node);
        node->parent = parent;
    }
}

static void fill_basic_blocks_to_exception(jd_method *m, jd_node *node)
{
    // 后序遍历exception tree，将创建的basic block类型的node添加到exception node下面
    if (node == NULL)
        node = lget_obj_first(m->nodes); // root node

    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);

        if (node_is_exception(child)) {
            fill_basic_blocks_to_exception(m, child);
        }
        else if (node_is_try(child) ||
                node_is_catch(child) ||
                node_is_finally(child)) {
            fill_basic_blocks_to_exception(m, child);
            list_object *blocks = basic_blocks_of_node(m, child);
            add_basic_blocks(m, child, blocks);
        }
    }
}

static void move_basic_blocks_to_exception(jd_method *m)
{
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_exception(node)) {
            fill_basic_blocks_to_exception(m, node);
        }
        else if (node_is_try(node) ||
                 node_is_catch(node) ||
                 node_is_finally(node)) {
            fill_basic_blocks_to_exception(m, node);
            list_object *blocks = basic_blocks_of_node(m, node);
            add_basic_blocks(m, node, blocks);
        }
    }
}

int node_level_ident(jd_node *node)
{
    if (node->type == JD_NODE_METHOD_ROOT)
        return 0;
    else {
        int level = 0;
        jd_node *n = node;
        while (n->type != JD_NODE_METHOD_ROOT) {
            n = n->parent;
            level++;
        }
        return level;
    }
}

void print_node_tree(jd_method *m, jd_node *node)
{
    if (node == NULL) {
        printf("============ m: %s, "
               "node_size: %zu ============\n\n",
               m->name, m->nodes->size);
        node = lget_obj(m->nodes, 0);
    }
    int level = node_level_ident(node);
    printf("%*s", level * 4, " ");
    jd_exp *start_exp = get_exp(m, node->start_idx);
    jd_exp *end_exp = get_exp(m, node->end_idx);
    jd_ins *start_ins = start_exp->ins;
    jd_ins *end_ins = end_exp->ins;
    printf("node: %d, %s, %d -> %d offset: %x -> %x\n",
           node->node_id,
           node_name(node),
           node->start_idx,
           node->end_idx,
           start_ins->offset, end_ins->offset);
    if (node->children == NULL)
        return;
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        print_node_tree(m, child);
    }
}

void create_node_tree(jd_method *m)
{
    m->nodes = linit_object();

    create_root_node(m);

    create_basic_block_node(m);

    create_exception_node(m);

    move_basic_blocks_to_exception(m);

    reorder_nodes(m, NULL);
}


#if 0
static void add_copy_block_recursive(jd_method *m,
                                     jd_node *parent,
                                     jd_bblock *parent_block,
                                     jd_node *node)
{
    jd_bblock *block = node->data;
    jd_nblock *nblock = block->ub->nblock;
    jd_exp *last = get_exp(m, nblock->end_idx);

    if (!exp_is_goto(last))
        return;
    jd_ins *ins = last->ins;
    if (!ins_is_copy_block(ins))
        return;
    list_object *list = ins->extra;
    for (int j = 0; j < list->size; ++j) {
        jd_bblock *b = lget_obj(list, j);
        jd_node *n = b->node;
        if (lcontains_obj(parent_block->dominates, b)) {
            ldel_obj(n->parent->children, n);
            n->parent = parent;
            ladd_obj(parent->children, n);
        }
        add_copy_block_recursive(m, parent, parent_block, n);
    }
}
#endif
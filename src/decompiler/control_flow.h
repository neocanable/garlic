#ifndef GARLIC_CONTROL_FLOW_H
#define GARLIC_CONTROL_FLOW_H

#define JD_STATUS_BUSY 1
#define JD_STATUS_FREE 0
#define NORMAL_BLOCK_START_ID 3;

#include "common/debug.h"
#include "decompiler/structure.h"
#include "decompiler/exception.h"
#include "decompiler/expression_node.h"

void cfg_create(jd_method *m);

jd_bblock* block_by_id(jd_method *m, int block_id);

jd_bblock* block_handler_equals_offset(jd_method *m, uint32_t offset);

jd_bblock* block_handler_equals_ins(jd_method *m, jd_ins *ins);

jd_bblock* block_start_offset(jd_method *m, uint32_t offset);

jd_bblock* block_contains_idx(jd_method *m, int idx);

jd_bblock* exp_block(jd_exp *exp);

jd_bblock* block_closest_finally(jd_method *m, jd_bblock *e);

jd_bblock* block_closest_handler(jd_method *m, jd_bblock *block);

// find basic block contains the **goto_offset**, include expcetion block
jd_bblock* block_by_offset(jd_method *m, uint32_t offset);

jd_bblock* block_exception_exit(jd_method *m);

jd_bblock* block_enter(jd_method *m);

bool ins_is_handler_start(jd_method *m, jd_ins *ins);

bool ins_is_try_end(jd_method *m, jd_ins *ins);

jd_bblock* block_parent_exception(jd_method *m, jd_bblock *block);

jd_edge* create_link_edge(jd_bblock *source, jd_bblock *target);

bool basic_block_id_comparator(const jd_bblock *a, const jd_bblock *b);

void basic_block_clear_visited_flag(jd_method *m);

jd_bblock* dup_basic_block(jd_method *m, jd_bblock *src);

string list_block_id_join(list_object *list, string delimiter);

string list_edge_id_join(list_object *list, int type);

string list_in_edge_id_join(list_object *list);

string list_out_edge_id_join(list_object *list);

void print_basic_block(jd_method *m);

void print_dominator_tree(jd_method *m, jd_bblock *block);

void cfg_remove_exception_block(jd_method *m);

void cfg_unlink_blocks(jd_bblock *source, jd_bblock *target);

jd_bblock* make_goto_basic_block(jd_method *m,
                                 jd_support_type type,
                                 uint32_t offset);

jd_bblock* make_dalvik_goto_block(jd_method *m, uint32_t offset);

list_object* compute_scc(jd_method *m);

static inline bool basic_block_contains_exp(jd_bblock *block, jd_exp *exp)
{
    if (block->source == JD_NODE_EXCEPTION) {
        jd_node *exception_node = block->data;
        for (int i = 0; i < exception_node->children->size; ++i) {
            jd_node *child = lget_obj(exception_node->children, i);
            if (node_contains_expression(child, exp))
                return true;
        }
        return false;
    }
    else
        return block->ub->nblock->start_idx <= exp->idx &&
               block->ub->nblock->end_idx >= exp->idx;
}

static inline bool basic_block_is_live(jd_bblock *b)
{
    return b->live == JD_STATUS_BUSY;
}

static inline bool basic_block_is_normal_live(jd_bblock *b)
{
    return b->live == JD_STATUS_BUSY && b->type == JD_BB_NORMAL;
}

static inline bool basic_block_has_effect_expression(jd_bblock *source,
                                                     jd_bblock *target)
{
    jd_node *snode = source->node;
    jd_exp *exp = get_exp(source->method, snode->end_idx);
    if (!exp_is_nopped(exp) && !exp_is_empty(exp))
        return true;
    return false;
}

#endif //GARLIC_CONTROL_FLOW_H

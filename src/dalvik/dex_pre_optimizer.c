#include "dalvik/dex_ins.h"
#include "dalvik/dex_pre_optimizer.h"

#include "decompiler/dominator_tree.h"
#include "decompiler/control_flow.h"
#include "jvm/jvm_ins.h"

static inline bool is_goto_edge(jd_edge *edge)
{
    jd_bblock *source = edge->source_block;
    if (source->type != JD_BB_NORMAL)
        return false;
    jd_nblock *source_nb = source->ub->nblock;
    jd_dex_ins *ins = ins_of_offset(source->method, source_nb->end_offset);
    return dex_ins_is_goto_jump(ins);
}

static bool basic_block_has_live_jump_in_goto(jd_bblock *block)
{
    for (int i = 0; i < block->in->size; ++i) {
        jd_edge *edge = lget_obj(block->in, i);
        jd_bblock *source = edge->source_block;
        if (source->type != JD_BB_NORMAL) continue;
        jd_nblock *nsb = source->ub->nblock;
        jd_dex_ins *end_ins = ins_of_offset(source->method, nsb->end_offset);
        if (dex_ins_is_goto_jump(end_ins) && !ins_is_nopped(end_ins))
            return true;
    }
    return false;
}

static bool basic_block_is_finally_start(jd_method *m, jd_bblock *block)
{
    if (block->type != JD_BB_NORMAL)
        return false;

    jd_nblock *nblock = block->ub->nblock;
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (b->type != JD_BB_EXCEPTION)
            continue;
        jd_eblock *eblock = b->ub->eblock;
        if (eblock->handler_start_offset == nblock->start_offset &&
            eblock->type == JD_EXCEPTION_FINALLY) {
            return true;
        }
    }
    return false;
}

static void modify_handler_start_to_goto_target(jd_method *m,
                                                jd_bblock *b,
                                                bool is_finally)
{
    jd_nblock *nblock = b->ub->nblock;
    jd_dex_ins *start_ins = nblock->start_ins;
    jd_dex_ins *end_ins = nblock->end_ins;
    u4 offset = dex_goto_offset(end_ins);
    jd_dex_ins *target_ins = dex_ins_of_offset(m, offset);

    // 这里是catch共享基本块的情况
    // move-exception v0
    // goto 12
    // move-exception v0
    // move v1, v2
    // [12] bla bla
    jd_bblock *prev_block = target_ins->prev->block;
    jd_nblock *prev_nblock = prev_block->ub->nblock;
    jd_dex_ins *prev_start_ins = prev_nblock->start_ins;
    if (!dex_ins_is_move_exception(prev_start_ins))
        return;
    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->cfg_exceptions, i);
        if (e->handler_start == start_ins->offset
            /* && e->catch_type_index != 0*/) {
            e->handler_start = prev_start_ins->offset;
            e->handler_start_idx = prev_start_ins->idx;
        }
    }

    ins_mark_nopped(start_ins);
    ins_mark_nopped(end_ins);
}

static void optimize_goto_to_return(jd_method *m)
{
    // goto goto_offset
    // goto_offset is return
    // replace current goto to return

    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (b->type != JD_BB_NORMAL)
            continue;
        jd_nblock *nblock = b->ub->nblock;
        jd_dex_ins *end_ins = nblock->end_ins;

        if (!dex_ins_is_goto_jump(end_ins))
            continue;

        s4 offset = (s4)dex_goto_offset(end_ins);
        jd_dex_ins *target_ins = dex_ins_of_offset(m, offset);
        while (dex_ins_is_goto_jump(target_ins)) {
            offset = dex_goto_offset(target_ins);
            target_ins = dex_ins_of_offset(m, offset);
        }
        if (!dex_ins_is_return_op(target_ins))
            continue;

        end_ins->param = target_ins->param;
        end_ins->code = target_ins->code;
        end_ins->name = target_ins->name;
        end_ins->format = target_ins->format;
        end_ins->param_length = target_ins->param_length;

        lclear_object(end_ins->targets);
        lclear_object(end_ins->jumps);
        ldel_object(target_ins->comings, end_ins);
        DEBUG_PRINT("[replace goto to return]: %s %d\n",
                    end_ins->name,
                    end_ins->offset);
    }
}

void optimize_move_exception_goto(jd_method *m)
{
    // d8和dx都有这项优化
    // 在某个基本块前面插入另外一个exception的基本块
    // 插入的基本块只有两条move-exception和goto
    // goto的target是真正的exception handler
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (b->type != JD_BB_NORMAL)
            continue;

        jd_nblock *nblock = b->ub->nblock;
        jd_dex_ins *start_ins = nblock->start_ins;
        jd_dex_ins *end_ins = nblock->end_ins;
        if (!dex_ins_is_move_exception(start_ins) ||
            !dex_ins_is_goto_jump(end_ins) ||
            start_ins->next != end_ins)
            continue;

        bool is_finally = basic_block_is_finally_start(m, b);

        DEBUG_PRINT("[found move-exception goto] %zu\n", b->block_id);

        modify_handler_start_to_goto_target(m, b, is_finally);
    }
}

static void optimize_share_suffix_v2(jd_method *m)
{
    bool changed = false;
    do {
        changed = false;
        DEBUG_PRINT("start optimize share suffix\n");
        for (int i = 0; i < m->basic_blocks->size; ++i) {
            jd_bblock *b = lget_obj(m->basic_blocks, i);
            if (!basic_block_is_normal_live(b)) continue;
            if (!is_list_empty(b->dom_children)) continue;
            if (b->out->size != 1) continue;
            if (lcontains_obj(b->frontier, b)) continue;

            if (!basic_block_has_live_jump_in_goto(b)) continue;

            jd_edge *out_edge = lget_obj_first(b->out);
            jd_bblock *out_block = out_edge->target_block;
            if (out_block->type != JD_BB_NORMAL) continue;

            jd_nblock *nb = b->ub->nblock;
            jd_dex_ins *start_ins = nb->start_ins;
            jd_dex_ins *end_ins = nb->end_ins;
            if (dex_ins_is_switch(end_ins)) continue;

            jd_nblock *out_nb = out_block->ub->nblock;
            jd_dex_ins *out_start_ins = out_nb->start_ins;

            for (int j = 0; j < b->in->size; ++j) {
                jd_edge *edge = lget_obj(b->in, j);
                jd_bblock *sb = edge->source_block;
                if (!is_goto_edge(edge)) continue;
                jd_nblock *snb = sb->ub->nblock;

                jd_dex_ins *goto_ins = dex_ins_of_offset(m, snb->end_offset);
                if (goto_ins->extra == NULL)
                    goto_ins->extra = linit_object();
                ins_mark_copy_block(goto_ins);

                cfg_unlink_blocks(sb, b);
                create_link_edge(sb, out_block);

                // TODO: 如果nb->end_idx是一个return, 那么需要将goto_ins nop掉
                for (int k = nb->start_idx; k <= nb->end_idx ; ++k) {
                    jd_dex_ins *ins = get_dex_ins(m, k);
                    if (dex_ins_is_goto_jump(ins) && ins == nb->end_ins)
                        continue;
                    ladd_obj(goto_ins->extra, ins);
                }
                dex_setup_goto_offset(goto_ins, out_start_ins->offset);

                --j;
                changed = true;
            }

            if (b->in->size == 0) {
                jd_dex_ins *nopped = start_ins;
                while (nopped != NULL && nopped->offset <= end_ins->offset) {
                    ins_mark_nopped(nopped);
                    nopped = nopped->next;
                }
            }
        }

        if (changed) {
            clear_dominator_data(m);
            create_dominator_tree(m);
        }
    } while (changed);
}

void pre_optimize_dex_method(jd_method *m)
{
    cfg_create(m);

    optimize_goto_to_return(m);

    optimize_share_suffix_v2(m);
}

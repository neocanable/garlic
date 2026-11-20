#include "dalvik/dex_cfg_opt.h"
#include "dalvik/dex_ins.h"
#include "decompiler/control_flow.h"
#include "decompiler/method.h"
#include "decompiler/exception.h"
#include "parser/dex/metadata.h"
#include "bitset.h"

#include <stdlib.h>

#define CFG_MAX_ROOTS 32
#define CFG_MAX_CHAIN 32
#define CFG_MAX_COPIES 64
#define CFG_MAX_JOIN 16

typedef struct cfg_share {
    int h;
    bool root;
    bool fall;
    int ng;
    int nc;
    jd_dex_ins *gotos[CFG_MAX_JOIN];
    jd_dex_ins *conds[CFG_MAX_JOIN];
} cfg_share;

static bool cfg_off_covered(jd_method *m, uint32_t off)
{
    for (int x = 0; x < m->cfg_exceptions->size; ++x) {
        jd_exc *e = lget_obj(m->cfg_exceptions, x);
        if (off >= e->try_start && off <= e->try_end)
            return true;
    }
    return false;
}

static bool cfg_same_coverage(jd_method *m, uint32_t a, uint32_t b)
{
    for (int x = 0; x < m->cfg_exceptions->size; ++x) {
        jd_exc *e = lget_obj(m->cfg_exceptions, x);
        bool ca = a >= e->try_start && a <= e->try_end;
        bool cb = b >= e->try_start && b <= e->try_end;
        if (ca != cb)
            return false;
    }
    return true;
}

static bool cfg_tail_safe(jd_method *m, jd_bblock **chain, int cn,
                          uint32_t at)
{
    int total = (int) m->instructions->size;
    u1 *inchain = make_obj_arr_zero(u1, total < 1 ? 1 : (size_t) total);
    for (int i = 0; i < cn; ++i) {
        jd_nblock *nb = chain[i]->ub->nblock;
        for (int k = nb->start_idx; k <= nb->end_idx; ++k) {
            if (k >= 0 && k < total)
                inchain[k] = 1;
        }
    }
    jd_dex_ins *chain_end = (jd_dex_ins *)
        chain[cn - 1]->ub->nblock->end_ins;
    bool terminal = chain_end != NULL &&
                    (dex_ins_is_return_op(chain_end) ||
                     dex_ins_is_throw(chain_end));
    bitset_t *outside = bitset_create_with_capacity(m->max_locals);
    if (!terminal) {
        for (int i = 0; i < total; ++i) {
            if (inchain[i])
                continue;
            jd_dex_ins *oi = (jd_dex_ins *) get_ins(m, i);
            if (oi != NULL && oi->uses != NULL)
                bitset_inplace_union(outside, oi->uses);
        }
    }
    for (int i = 0; i < cn; ++i) {
        jd_nblock *nb = chain[i]->ub->nblock;
        for (int k = nb->start_idx; k <= nb->end_idx; ++k) {
            jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, k);
            if (ins == NULL)
                return false;
            if ((ins->code == 0 && ins->param_length > 1) ||
                dex_ins_is_switch(ins) ||
                dex_ins_is_fill_array_data(ins))
                return false;
            if (!terminal && ins->defs != NULL &&
                bitsets_intersect(ins->defs, outside))
                return false;
            if (!cfg_same_coverage(m, ins->offset, at))
                return false;
        }
    }
    return true;
}

static jd_dex_ins *cfg_clone_ins(jd_method *m, jd_dex_ins *src,
                                 jd_dex_ins *prev)
{
    jd_dex_ins *ins = make_obj_zero(jd_dex_ins);
    memcpy(ins, src, sizeof(jd_dex_ins));
    ins->method = m;
    size_t units = src->param_length < 2 ? 2 : src->param_length;
    ins->param = x_alloc(sizeof(u2) * units);
    if (src->param != NULL && src->param_length > 0)
        memcpy(ins->param, src->param, sizeof(u2) * src->param_length);
    ins->stack_in = NULL;
    ins->stack_out = NULL;
    ins->expression = NULL;
    ins->block = NULL;
    ins->targets = NULL;
    ins->jumps = NULL;
    ins->comings = NULL;
    ins->extra = NULL;
    ins->uses = src->uses == NULL ? NULL : bitset_copy(src->uses);
    ins->defs = src->defs == NULL ? NULL : bitset_copy(src->defs);
    ins->prev = prev;
    ins->next = NULL;
    if (prev != NULL)
        prev->next = ins;
    ins->old_offset = src->offset;
    ins_mark_duplicate((jd_ins *) ins);
    return ins;
}

static void cfg_add_jump(jd_dex_ins *ins, jd_dex_ins *target)
{
    if (ins->jumps == NULL)
        ins->jumps = linit_object();
    if (ins->targets == NULL)
        ins->targets = linit_object();
    if (target->comings == NULL)
        target->comings = linit_object();
    ladd_obj_no_dup(ins->jumps, target);
    ladd_obj_no_dup(ins->targets, target);
    ladd_obj_no_dup(target->comings, ins);
}

static bool cfg_method_plain(jd_method *m)
{
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, i);
        if (ins == NULL)
            continue;
        if (dex_ins_is_switch(ins) ||
            dex_ins_is_fill_array_data(ins) ||
            (ins->code == 0 && ins->param_length > 1))
            return false;
    }
    return true;
}

static bool cfg_terminal_chain(jd_method *m, jd_bblock *b0,
                               jd_bblock **chain, int *out_n)
{
    int n = (int) m->basic_blocks->size;
    u1 *seen = make_obj_arr_zero(u1, n < 1 ? 1 : (size_t) n);
    int cn = 0;
    jd_bblock *cur = b0;
    bool ok = true;
    while (cur != NULL && cn < CFG_MAX_CHAIN) {
        if (cur->type != JD_BB_NORMAL || seen[cur->block_id]) {
            ok = false;
            break;
        }
        seen[cur->block_id] = 1;
        chain[cn++] = cur;
        jd_dex_ins *end = (jd_dex_ins *) cur->ub->nblock->end_ins;
        if (end == NULL) {
            ok = false;
            break;
        }
        if (dex_ins_is_return_op(end) || dex_ins_is_throw(end))
            break;
        if (dex_ins_is_conditional_jump(end) || dex_ins_is_switch(end)) {
            ok = false;
            break;
        }
        jd_bblock *succ = NULL;
        int count = 0;
        for (int i = 0; cur->out != NULL && i < cur->out->size; ++i) {
            jd_edge *edge = lget_obj(cur->out, i);
            if (edge->type != JD_EDGE_NORMAL || edge->target_block == NULL)
                continue;
            count++;
            succ = edge->target_block;
        }
        if (count != 1 || succ == NULL) {
            ok = false;
            break;
        }
        cur = succ;
    }
    if (cn == 0)
        ok = false;
    *out_n = cn;
    return ok;
}

static bool cfg_commit_order(jd_method *m, jd_dex_ins **order,
                             u1 *is_new, int *new_pos, int on,
                             int old_n, int old_len,
                             jd_dex_ins **ins_list,
                             jd_dex_ins **ins_target, int ni,
                             jd_dex_ins *removed)
{
    uint32_t *newoff = x_alloc(sizeof(uint32_t) * (size_t) (on + 1));
    uint32_t run = 0;
    for (int i = 0; i < on; ++i) {
        newoff[i] = run;
        run += order[i]->param_length;
    }
    uint32_t new_len = run;

    jd_dex_ins **jump_target = x_alloc(sizeof(jd_dex_ins *) * (size_t) (on + 1));
    jd_dex_ins **old2ins = make_obj_arr_zero(jd_dex_ins *,
                                             (size_t) old_len + 1);
    for (int i = 0; i < old_n; ++i) {
        jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, i);
        if (ins->offset < (uint32_t) old_len)
            old2ins[ins->offset] = ins;
    }

    bool ok = true;
    int new_i = 0;
    for (int i = 0; i < on && ok; ++i) {
        jd_dex_ins *ins = order[i];
        jd_dex_ins *target = NULL;
        if (is_new[i]) {
            while (new_i < ni && ins_list[new_i] != ins)
                new_i++;
            if (new_i < ni) {
                target = ins_target[new_i];
                new_i++;
            }
        }
        else {
            if (dex_ins_is_goto_jump(ins)) {
                uint32_t t = (uint32_t) dex_goto_offset(ins);
                target = t < (uint32_t) old_len ? old2ins[t] : NULL;
                if (target == NULL)
                    ok = false;
            }
            else if (dex_ins_is_conditional_jump(ins)) {
                uint32_t t = (uint32_t) dex_ins_if_jump_offset(ins);
                target = t < (uint32_t) old_len ? old2ins[t] : NULL;
                if (target == NULL)
                    ok = false;
            }
        }
        if (!ok)
            break;
        if (removed != NULL && ni > 0 && target == removed)
            target = ins_list[0];
        if (target != NULL) {
            if (target->idx < 0 || target->idx > old_n + ni)
                ok = false;
            else {
                uint32_t toff = newoff[new_pos[target->idx]];
                int32_t disp = (int32_t) toff - (int32_t) newoff[i];
                if (dex_ins_is_conditional_jump(ins) &&
                    (disp < -32768 || disp > 32767))
                    ok = false;
            }
        }
        jump_target[i] = target;
    }
    if (!ok)
        return false;

    for (int i = 0; i < on; ++i) {
        jd_dex_ins *ins = order[i];
        ins->offset = newoff[i];
        ins->idx = i;
        if (ins->jumps != NULL) lclear_object(ins->jumps);
        if (ins->targets != NULL) lclear_object(ins->targets);
        if (ins->comings != NULL) lclear_object(ins->comings);
    }
    for (int i = 0; i < on; ++i) {
        jd_dex_ins *target = jump_target[i];
        if (target == NULL)
            continue;
        jd_dex_ins *ins = order[i];
        if (dex_ins_is_conditional_jump(ins))
            dex_setup_if_offset(ins, target->offset);
        else
            dex_setup_goto_offset(ins, target->offset);
        cfg_add_jump(ins, target);
    }
    for (int i = 0; i < on; ++i) {
        order[i]->prev = i > 0 ? order[i - 1] : NULL;
        order[i]->next = i + 1 < on ? order[i + 1] : NULL;
    }
    lclear_object(m->instructions);
    for (int i = 0; i < on; ++i)
        ladd_obj(m->instructions, order[i]);
    m->offset2id_map = hashmap_init((hcmp_fn) i2i_cmp,
                                    on < 1 ? 1 : (size_t) on);
    for (int i = 0; i < on; ++i)
        hset_i2i(m->offset2id_map, order[i]->offset, order[i]->idx);

    for (int x = 0; x < m->cfg_exceptions->size; ++x) {
        jd_exc *e = lget_obj(m->cfg_exceptions, x);
        jd_dex_ins *start = e->try_start < (uint32_t) old_len ?
                            old2ins[e->try_start] : NULL;
        jd_dex_ins *last = e->try_end < (uint32_t) old_len ?
                           old2ins[e->try_end] : NULL;
        jd_dex_ins *handler = e->handler_start < (uint32_t) old_len ?
                              old2ins[e->handler_start] : NULL;
        uint32_t old_end = e->end_pc;
        uint32_t new_end = old_end >= (uint32_t) old_len ?
                           new_len :
                           (old2ins[old_end] != NULL ?
                            old2ins[old_end]->offset : new_len);
        if (start != NULL) {
            e->try_start = start->offset;
            e->try_start_idx = start->idx;
        }
        if (last != NULL) {
            e->try_end = last->offset;
            e->try_end_idx = last->idx;
        }
        if (handler != NULL) {
            e->handler_start = handler->offset;
            e->handler_start_idx = handler->idx;
        }
        e->end_pc = (u2) new_end;
        if (start != NULL && start->offset <= 65535)
            e->start_pc = (u2) start->offset;
        if (handler != NULL && handler->offset <= 65535)
            e->handler_pc = (u2) handler->offset;
    }
    return true;
}

static bool cfg_inline_commit(jd_method *m, int pos,
                              jd_dex_ins **ins_list,
                              jd_dex_ins **ins_target, int ni)
{
    int old_n = (int) m->instructions->size;
    if (pos < 0 || pos >= old_n)
        return false;
    int old_len = 0;
    for (int i = 0; i < old_n; ++i) {
        jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, i);
        int end = (int) ins->offset + ins->param_length;
        if (end > old_len)
            old_len = end;
    }
    jd_dex_ins *removed = (jd_dex_ins *) get_ins(m, pos);
    int on = old_n - 1 + ni;
    jd_dex_ins **order = x_alloc(sizeof(jd_dex_ins *) * (size_t) (on + 1));
    u1 *is_new = make_obj_arr_zero(u1, (size_t) (on + 1));
    int *new_pos = make_obj_arr_zero(int, (size_t) (old_n + ni + 1));
    int oi = 0;
    for (int i = 0; i < old_n; ++i) {
        if (i == pos) {
            for (int z = 0; z < ni; ++z) {
                order[oi] = ins_list[z];
                is_new[oi] = 1;
                new_pos[ins_list[z]->idx] = oi;
                oi++;
            }
            continue;
        }
        jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, i);
        order[oi] = ins;
        new_pos[ins->idx] = oi;
        oi++;
    }
    if (oi != on)
        return false;

    return cfg_commit_order(m, order, is_new, new_pos, on, old_n,
                            old_len, ins_list, ins_target, ni, removed);
}



static bool cfg_end_or_jump(jd_dex_ins *ins)
{
    return ins != NULL &&
           (dex_ins_is_return_op(ins) || dex_ins_is_throw(ins) ||
            dex_ins_is_goto_jump(ins));
}



static bool cfg_linear_prologue(jd_method *m, uint32_t handler_start,
                                int *out_start, int *out_end,
                                int *out_target)
{
    jd_bblock *hb = block_by_offset(m, handler_start);
    if (hb == NULL || hb->type != JD_BB_NORMAL)
        return false;
    jd_nblock *hnb = hb->ub->nblock;
    if (hnb->start_idx <= 0 || hnb->end_idx < hnb->start_idx + 1)
        return false;

    int start = hnb->start_idx;
    int end = hnb->end_idx;
    jd_dex_ins *first = (jd_dex_ins *) get_ins(m, start);
    jd_dex_ins *last = (jd_dex_ins *) get_ins(m, end);
    if (!dex_ins_is_move_exception(first) || !dex_ins_is_goto_jump(last))
        return false;

    jd_bblock *target = block_by_offset(m, dex_goto_offset(last));
    if (target == NULL || target->type != JD_BB_NORMAL)
        return false;
    int target_start = target->ub->nblock->start_idx;
    if (target_start <= end + 1)
        return false;

    if (!cfg_end_or_jump((jd_dex_ins *) get_ins(m, start - 1)))
        return false;
    if (!cfg_end_or_jump((jd_dex_ins *) get_ins(m, target_start - 1)))
        return false;

    *out_start = start;
    *out_end = end;
    *out_target = target_start;
    return true;
}

static bool cfg_prologue_endpoints_safe(jd_method *m,
                                        int start, int end)
{
    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->cfg_exceptions, i);
        int ts = hget_i2i(m->offset2id_map, e->try_start);
        int te = hget_i2i(m->offset2id_map, e->try_end);
        int hs = hget_i2i(m->offset2id_map, e->handler_start);
        if ((ts >= start && ts <= end) || (te >= start && te <= end))
            return false;
        if (hs >= start && hs <= end && hs != start)
            return false;
    }
    return true;
}

static bool cfg_move_prologue(jd_method *m, int start, int end,
                              int target_start)
{
    int old_n = (int) m->instructions->size;
    if (start <= 0 || target_start <= end || target_start >= old_n)
        return false;

    int old_len = 0;
    for (int i = 0; i < old_n; ++i) {
        jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, i);
        int ins_end = (int) ins->offset + ins->param_length;
        if (ins_end > old_len)
            old_len = ins_end;
    }

    jd_dex_ins **order = x_alloc(sizeof(jd_dex_ins *) * (size_t) old_n);
    u1 *is_new = make_obj_arr_zero(u1, (size_t) old_n);
    int *new_pos = make_obj_arr_zero(int, (size_t) old_n + 1);
    int oi = 0;
    for (int i = 0; i < old_n; ++i) {
        if (i >= start && i <= end)
            continue;
        if (i == target_start) {
            for (int k = start; k <= end; ++k)
                order[oi++] = (jd_dex_ins *) get_ins(m, k);
        }
        order[oi++] = (jd_dex_ins *) get_ins(m, i);
    }
    if (oi != old_n)
        return false;

    for (int i = 0; i < old_n; ++i)
        new_pos[order[i]->idx] = i;
    return cfg_commit_order(m, order, is_new, new_pos, old_n, old_n,
                            old_len, NULL, NULL, 0, NULL);
}

static void cfg_linearize_handlers(jd_method *m)
{
    if (m->cfg_exceptions->size > 6 || m->instructions->size > 64)
        return;
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_dex_ins *ins = (jd_dex_ins *) get_ins(m, i);
        if (dex_ins_is_monitor_enter(ins) || dex_ins_is_monitor_exit(ins))
            return;
    }
    for (int round = 0; round < CFG_MAX_COPIES; ++round) {
        bool changed = false;
        int n = (int) m->cfg_exceptions->size;
        for (int i = 0; i < n && !changed; ++i) {
            jd_exc *e = lget_obj(m->cfg_exceptions, i);
            int start = 0;
            int end = 0;
            int target_start = 0;
            if (!cfg_linear_prologue(m, e->handler_start,
                                     &start, &end, &target_start))
                continue;
            if (!cfg_prologue_endpoints_safe(m, start, end))
                continue;
            if (!cfg_move_prologue(m, start, end, target_start))
                continue;
            changed = true;
        }
        if (!changed)
            break;
        cfg_create(m);
    }
}

void optimize_exception_cfg(jd_method *m)
{
    if (m == NULL || m->type != JD_TYPE_DALVIK ||
        m->basic_blocks == NULL || m->instructions == NULL)
        return;
    if (!cfg_method_plain(m))
        return;

    for (int round = 0; round < CFG_MAX_COPIES; ++round) {
        int n = (int) m->basic_blocks->size;
        bool changed = false;
        for (int bid = 0; bid < n && !changed; ++bid) {
            jd_bblock *pred = block_by_id(m, bid);
            if (pred == NULL || pred->type != JD_BB_NORMAL)
                continue;
            jd_dex_ins *last = (jd_dex_ins *) pred->ub->nblock->end_ins;
            if (last == NULL || !dex_ins_is_goto_jump(last))
                continue;
            bool near_try = false;
            for (int x = 0; x < m->cfg_exceptions->size; ++x) {
                jd_exc *e = lget_obj(m->cfg_exceptions, x);
                jd_dex_ins *tend = dex_ins_of_offset(m, e->try_end);
                if ((tend != NULL && tend->next == last) ||
                    e->handler_start == last->offset ||
                    e->try_start == last->offset ||
                    e->try_end == last->offset)
                    near_try = true;
            }
            if (near_try)
                continue;
            jd_bblock *target = block_by_offset(m, dex_goto_offset(last));
            if (target == NULL || target->type != JD_BB_NORMAL ||
                target == pred)
                continue;

            int in_count = 0;
            for (int e = 0; target->in != NULL && e < target->in->size; ++e) {
                jd_edge *edge = lget_obj(target->in, e);
                if (edge->type == JD_EDGE_NORMAL &&
                    edge->source_block != NULL &&
                    edge->source_block->type == JD_BB_NORMAL)
                    in_count++;
            }
            if (in_count < 2)
                continue;

            jd_bblock *chain[CFG_MAX_CHAIN];
            int cn = 0;
            if (!cfg_terminal_chain(m, target, chain, &cn))
                continue;
            if (!cfg_tail_safe(m, chain, cn, last->offset))
                continue;

            int chain_ins = 0;
            for (int i = 0; i < cn; ++i) {
                jd_nblock *nb = chain[i]->ub->nblock;
                chain_ins += nb->end_idx - nb->start_idx + 1;
            }
            int cap = chain_ins + 8;
            jd_dex_ins **ins_list = x_alloc(sizeof(jd_dex_ins *) *
                                            (size_t) cap);
            jd_dex_ins **ins_target = make_obj_arr_zero(jd_dex_ins *,
                                                        (size_t) cap);
            int ni = 0;
            int old_n = (int) m->instructions->size;
            jd_dex_ins *block_head[CFG_MAX_CHAIN];
            memset(block_head, 0, sizeof(block_head));
            bool build_ok = true;
            for (int i = 0; i < cn && build_ok; ++i) {
                jd_nblock *nb = chain[i]->ub->nblock;
                for (int k = nb->start_idx; k <= nb->end_idx; ++k) {
                    jd_dex_ins *src = (jd_dex_ins *) get_ins(m, k);
                    jd_dex_ins *copy = cfg_clone_ins(m, src, NULL);
                    copy->idx = old_n + ni;
                    if (block_head[i] == NULL)
                        block_head[i] = copy;
                    ins_list[ni] = copy;
                    ins_target[ni] = NULL;
                    if (dex_ins_is_goto_jump(src)) {
                        uint32_t t = (uint32_t) dex_goto_offset(src);
                        jd_dex_ins *tgt = NULL;
                        for (int z = 0; z < cn; ++z) {
                            if (chain[z]->ub->nblock->start_offset == t) {
                                tgt = block_head[z];
                                break;
                            }
                        }
                        if (tgt == NULL)
                            tgt = dex_ins_of_offset(m, t);
                        if (tgt == NULL) {
                            build_ok = false;
                            break;
                        }
                        ins_target[ni] = tgt;
                    }
                    ni++;
                    if (ni >= cap) {
                        build_ok = false;
                        break;
                    }
                }
            }
            if (!build_ok || ni < 1)
                continue;
            int pos = (int) last->idx;
            if (!cfg_inline_commit(m, pos, ins_list, ins_target, ni))
                continue;
            changed = true;
        }
        if (!changed)
            break;
        cfg_create(m);
    }

    cfg_linearize_handlers(m);
}

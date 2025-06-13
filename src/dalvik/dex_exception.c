#include "dalvik/dex_exception.h"
#include "dalvik/dex_ins.h"

#include "decompiler/dominator_tree.h"
#include "decompiler/exception.h"
#include "debug.h"
#include "decompiler/method.h"
#include "decompiler/control_flow.h"
#include "dex_pre_optimizer.h"

static jd_dex_ins *try_item_end_ins(jd_method *m, dex_try_item *try)
{
    u4 end_next_off = try->start_addr + try->insn_count;
    int idx = hget_i2i(m->offset2id_map, end_next_off);
    if (idx == -1)
        return dex_ins_of_offset(m, try->start_addr);

    jd_dex_ins *end_next_ins = dex_ins_of_offset(m, end_next_off);
    jd_dex_ins *end_ins = end_next_ins->prev;
    return end_ins;
}

static void dex_handler_pair(jd_method *m, dex_try_item *try, encoded_tp *pair)
{
    jd_dex_ins *sins = dex_ins_of_offset(m, try->start_addr);
    jd_dex_ins *end_ins = try_item_end_ins(m, try);
    jd_dex_ins *hs_ins = dex_ins_of_offset(m, pair->addr);

    jd_exc *e = make_obj(jd_exc);

    e->try_start = sins->offset;
    e->try_start_idx = sins->idx;
    e->try_end = end_ins->offset;
    e->try_end_idx = end_ins->idx;

    e->handler_start = hs_ins->offset;
    e->handler_start_idx = hs_ins->idx;
    e->catch_type_index = pair->type_idx;
    e->start_pc = sins->offset;
    e->end_pc = end_ins->offset;
    DEBUG_EXCEPTION_PRINT("[meta exception]: try: %d -> %d, "
                          "handler: %d, type: %d\n",
                          e->try_start,
                          e->try_end,
                          e->handler_start,
                          e->catch_type_index);
    ladd_obj(m->cfg_exceptions, e);
}

static void dex_handler_of_catch_all(jd_method *m,
                                     dex_try *try,
                                     encoded_catch_handler *ch)
{
    jd_dex_ins *tstart_ins = dex_ins_of_offset(m, try->start_addr);
    jd_dex_ins *te_ins = try_item_end_ins(m, try);
    jd_dex_ins *hstart_ins = dex_ins_of_offset(m, ch->catch_all_addr);

    jd_exc *e = make_obj(jd_exc);
    e->try_start = tstart_ins->offset;
    e->try_start_idx = tstart_ins->idx;
    e->try_end = te_ins->offset;
    e->try_end_idx = te_ins->idx;
    e->handler_start = hstart_ins->offset;
    e->handler_start_idx = hstart_ins->idx;
    e->start_pc = tstart_ins->offset;
    e->end_pc = te_ins->offset;
    e->catch_type_index = 0;
    DEBUG_EXCEPTION_PRINT("[meta exception]: "
                            "try: %d -> %d, handler: %d, type: %d\n",
                            e->try_start, 
                            e->try_end, 
                            e->handler_start, 
                            e->catch_type_index);
    ladd_obj(m->cfg_exceptions, e);
}

void dex_method_exception_init(jd_method *m, encoded_method *em)
{
    dex_code_item *code_item = em->code;
    m->cfg_exceptions = linit_object();
    for (int i = 0; i < code_item->tries_size; ++i) {
        dex_try_item *try = &code_item->tries[i];
        for (int j = 0; j < code_item->handlers->size; ++j) {
            encoded_catch_handler *ch = &code_item->handlers->list[j];
            if (ch->handler_off != try->handler_off)
                continue;

            for (int k = 0; k < abs(ch->size); ++k) {
                encoded_type_addr_pair *pair = &ch->handlers[k];
                dex_handler_pair(m, try, pair);
            }

            if (ch->catch_all_addr == 0)
                continue;
            dex_handler_of_catch_all(m, try, ch);
        }
    }
}

void dex_method_exception_edge(jd_method *m)
{
    if (method_is_empty(m))
        return;

    cfg_create(m);

    if (m->cfg_exceptions == NULL ||
        m->cfg_exceptions->size == 0) {
        m->mix_exceptions = linit_object();
        return;
    }

    optimize_move_exception_goto(m);

    identify_exception_handler_block_end(m);

    cleanup_full_exception_table(m);

    clear_dominator_tree(m);

    copy_exceptions_closed2cfg(m);

    cfg_create(m);

    identify_finally_excpetion_handler_block_end(m);

    flatten_exceptions(m);
}

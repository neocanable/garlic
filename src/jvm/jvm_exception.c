#include "decompiler/exception.h"
#include "jvm/jvm_exception.h"
#include "decompiler/method.h"
#include "decompiler/dominator_tree.h"
#include "decompiler/instruction.h"
#include "jvm_ins.h"
#include "decompiler/control_flow.h"


static jd_mix_exception* find_mix_exception(jd_method *m, jd_exc *ex)
{
    jd_mix_exception *result = NULL;
    for (int i = 0; i < m->mix_exceptions->size; ++i) {
        jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
        if (e->finally == NULL)
            continue;
        if (e->finally->start_offset == ex->handler_start &&
            e->finally->end_offset == ex->handler_end) {
            result = e;
            break;
        }
    }
    return result;
}

static void nop_instructions(jd_ins *from, jd_ins *to)
{
    if (from == NULL || to == NULL)
        return;

    while (from->offset <= to->offset) {
        if (ins_is_nopped(from)) {
            from = from->next;
            continue;
        }
        else {
            ins_mark_nopped(from);
            from = from->next;
        }
    }
}

static void reverse_compare_instruction(jd_method *m, jd_mix_exception *ex)
{
    assert(ex->finally != NULL);
    jd_range *finally = ex->finally;
    jd_range *try = ex->try;

    jd_ins *finally_end = get_ins(m, finally->end_idx);
    jd_ins *finally_start = get_ins(m, finally->start_idx);
    jd_ins *try_start = get_ins(m, try->start_idx);
    jd_ins *try_end = get_ins(m, try->end_idx);

    if (jvm_ins_is_athrow(finally_end)) finally_end = finally_end->prev;
    if (jvm_ins_is_load(finally_end)) finally_end = finally_end->prev;

    if (jvm_ins_is_unconditional_jump(try_end)) try_end = try_end->prev;
    if (jvm_ins_is_return(try_end)) try_end = try_end->prev;
    if (jvm_ins_is_store(finally_start)) finally_start = finally_start->next;
    if (jvm_ins_is_store(try_start)) try_start = try_start->next;

    int instruction_count = 0;
    jd_ins *cnt_ins = finally_end;
    while (cnt_ins != NULL && cnt_ins->offset >= finally_start->offset) {
        cnt_ins = cnt_ins->prev;
        instruction_count++;
    }

    jd_ins *finally_end_cmp = finally_end;
    jd_ins *try_end_cmp = try_end;
    jd_ins *try_copy_start = NULL;

    int counting = 0;
    while (finally_end_cmp != NULL &&
           try_end_cmp != NULL &&
           finally_end_cmp->offset >= finally_start->offset) {
        if (try_end_cmp->offset < try->start_offset)
            break;
        int result = jvm_ins_compares(finally_end_cmp, try_end_cmp);
        if (!result)
            break;
        try_copy_start = try_end_cmp;
        finally_end_cmp = finally_end_cmp->prev;
        try_end_cmp = try_end_cmp->prev;
        counting++;
    }

    if (counting == instruction_count)
    {
        DEBUG_EXCEPTION_PRINT("finally: %d -> %d, try block: %d -> %d ok, "
                              "try: %d -> %d will be nopped, count: %d\n",
                              finally->start_offset,
                              finally->end_offset,
                              try->start_offset,
                              try->end_offset,
                              try_start->offset,
                              try_end->offset, instruction_count);

        nop_instructions(try_copy_start, try_end);
    }

    DEBUG_EXCEPTION_PRINT("------- try finished --------->\n");

    for (int i = 0; i < ex->catches->size; ++i)
    {
        jd_range *catch_range = lget_obj(ex->catches, i);
        jd_ins *catch_start = get_ins(m, catch_range->start_idx);
        jd_ins *catch_end = get_ins(m, catch_range->end_idx);
        if (jvm_ins_is_unconditional_jump(catch_end)) 
            catch_end = catch_end->prev;
        if (jvm_ins_is_return(catch_end)) 
            catch_end = catch_end->prev;
        if (jvm_ins_is_store(catch_start)) 
            catch_start = catch_start->next;

        finally_end_cmp = finally_end;
        jd_ins *catch_end_cmp = catch_end;
        jd_ins *catch_copy_start_ins = NULL;

        int inner_counting = 0;
        while (finally_end_cmp != NULL &&
               finally_end_cmp->offset >= finally_start->offset &&
               catch_end_cmp != NULL &&
               catch_end_cmp->offset >= catch_start->offset) {
            int result = jvm_ins_compares(finally_end_cmp, catch_end_cmp);
            if (!result)
                break;
            catch_copy_start_ins = catch_end_cmp;
            DEBUG_EXCEPTION_PRINT("finally_end: %02x, "
                                  "catch_end: %02x, result: %d\n",
                                  finally_end_cmp->code,
                                  catch_end_cmp->code,
                                  result);

            finally_end_cmp = finally_end_cmp->prev;
            catch_end_cmp = catch_end_cmp->prev;
            inner_counting ++;
        }

        if (inner_counting == instruction_count) {
            DEBUG_EXCEPTION_PRINT("\nfinally block: %d -> %d, "
                                  "catch block: %d -> %d matched, "
                                  "catch: %d -> %d will be nopped, "
                                  "instruction count is: %d\n",
                                  finally->start_offset,
                                  finally->end_offset,
                                  catch_range->start_offset,
                                  catch_range->end_offset,
                                  catch_start->offset,
                                  catch_end->offset,
                                  instruction_count);
            nop_instructions(catch_copy_start_ins, catch_end);
        }
        DEBUG_EXCEPTION_PRINT("--------- catch finished ------------->\n");
    }
}

static void inline_finally_block(jd_method *m)
{
    buble_sort_cfg_exception(m, (list_cmp_fn) handler_contains_cmp);
    buble_sort_cfg_exception(m, (list_cmp_fn) try_contains_cmp);

    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->cfg_exceptions, i);
        if (exception->catch_type_index > 0) continue;

        jd_mix_exception *mix_ex = find_mix_exception(m, exception);
        if (mix_ex == NULL)
            fprintf(stderr, "出事了, finally block的range不见了\n");
        reverse_compare_instruction(m, mix_ex);
    }
}

void jvm_method_exception_edge(jd_method *m)
{
    if (method_is_empty(m))
        return;

    cfg_create(m);

    identify_exception_handler_block_end(m);

    cleanup_full_exception_table(m);

    clear_dominator_tree(m);

    copy_exceptions_closed2cfg(m);

    cfg_create(m);

    identify_finally_excpetion_handler_block_end(m);

    flatten_exceptions(m);

    inline_finally_block(m);

    pullin_block_jump_into_exception_try_block(m);
}

#include <assert.h>
#include "parser/class/class_structure.h"
#include "decompiler/structure.h"
#include "decompiler/control_flow.h"
#include "jvm/jvm_ins.h"
#include "decompiler/dominator_tree.h"
#include "decompiler/method.h"

static void print_cfg_exception_table(jd_method *m)
{
    if (!DEBUG_EXCEPTION)
        return;
    if (m->cfg_exceptions->size == 0)
        return;
    fprintf(stdout, "========= m: %s, "
                    "cfg exception table size: %zu\n\n",
            m->name,
            m->cfg_exceptions->size);
    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->cfg_exceptions, i);
        fprintf(stdout, "exception: try: %d -> %d, "
                        "handler: %d -> %d, type: %d\n",
                        exception->try_start,
                        exception->try_end,
                        exception->handler_start,
                        exception->handler_end,
                        exception->catch_type_index);
    }
}

static void print_full_exception_table(jd_method *m)
{
    if (!DEBUG_EXCEPTION) return;
    if (m->closed_exceptions->size == 0)
        return;
    jd_ins *ins = lget_obj_first(m->instructions);
    fprintf(stdout, "=========m: %s, "
                    "full exception table size: %zu ins: %zu %s\n\n",
            m->name,
            m->closed_exceptions->size,
            m->instructions->size, ins->name);
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->closed_exceptions, i);
        fprintf(stdout, "exception: try: %d -> %d, "
                        "handler: %d -> %d, type: %d\n",
                exception->try_start,
                exception->try_end,
                exception->handler_start,
                exception->handler_end,
                exception->catch_type_index);
    }
}

static void print_instruction_with_nopped(jd_method *m)
{
    for (int i = 0; i < m->ins_size; ++i) {
        jd_ins *ins = get_ins(m, i);
        if (ins_is_nopped(ins))
            fprintf(stdout, "%d: nop\n", ins->offset);
        else
            fprintf(stdout, "%d: %s\n", ins->offset, ins->name);
    }
}

static jd_range init_exception_range(uint32_t start, uint32_t end)
{
    jd_range range;
    range.start_offset = start;
    range.end_offset = end;
    return range;
}

static inline jd_range init_try_block_range(jd_exc *e)
{
    return init_exception_range(e->try_start,
                                e->try_end);
}

static inline jd_range init_handler_block_range(jd_exc *e)
{
    return init_exception_range(e->handler_start,
                                e->handler_end);
}

static inline bool exception_same_try_range(jd_exc *s, jd_exc *t)
{
    return (s->try_start == t->try_start &&
            s->try_end == t->try_end);
}

static inline bool exception_same_handler_range(jd_exc *s,
                                                jd_exc *t)
{
    return (s->handler_start == t->handler_start &&
            s->handler_end == t->handler_end);
}

static inline bool exception_range_cross(jd_range *s, jd_range *t)
{
    if (s->start_offset <= t->start_offset &&
        s->end_offset >= t->end_offset)
        return true;
    if (s->start_offset >= t->start_offset &&
        s->start_offset <= t->end_offset)
        return true;
    return false;
}

static inline bool exception_range_contains(jd_range *s, jd_range *t)
{
    return (s->start_offset <= t->start_offset &&
        s->end_offset >= t->end_offset);
}

static inline bool exception_range_same(jd_range *s, jd_range *t)
{
    return (s->start_offset == t->start_offset &&
        s->end_offset == t->end_offset);
}

static inline bool exception_in_range(jd_range *s, uint32_t offset)
{
    return (offset >= s->start_offset && offset <= s->end_offset);
}

static void buble_sort_exceptions(list_object *exceptions, list_cmp_fn fn)
{
    int size = exceptions->size;
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - 1-i; ++j) {
            jd_exc *e1 = lget_obj(exceptions, j);
            jd_exc *e2 = lget_obj(exceptions, j + 1);
            if (fn(e1, e2) > 0)
                lswap_obj(exceptions, j, j + 1);
        }
    }
}

void buble_sort_cfg_exception(jd_method *m, list_cmp_fn fn)
{
    buble_sort_exceptions(m->cfg_exceptions, fn);
}

//static void buble_sort_full_exception(jd_method *m, list_cmp_fn ins_fn)
//{
//    buble_sort_exceptions(m, m->closed_exceptions, ins_fn);
//}

void copy_exceptions_closed2cfg(jd_method *m)
{
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->closed_exceptions, i);
        jd_exc *e = make_obj(jd_exc);
        e->idx = exception->idx;
        e->try_start = exception->try_start;
        e->try_end = exception->try_end;
        e->try_start_idx = exception->try_start_idx;
        e->try_end_idx = exception->try_end_idx;
        e->handler_start = exception->handler_start;
        e->handler_end = exception->handler_end;
        e->handler_start_idx = exception->handler_start_idx;
        e->handler_end_idx = exception->handler_end_idx;
        e->catch_type_index = exception->catch_type_index;
        e->start_pc = exception->start_pc;
        e->end_pc = exception->end_pc;
        e->handler_pc = exception->handler_pc;
        e->catch_type = exception->catch_type;
        ladd_obj(m->cfg_exceptions, e);
    }
}

static void copy_exceptions_to_closed(jd_method *m, jd_eblock *eblock)
{
    jd_exc *e = make_obj(jd_exc);
    e->idx = eblock->idx;
    e->try_start = eblock->try_start_offset;
    e->try_end = eblock->try_end_offset;
    e->try_start_idx = eblock->try_start_idx;
    e->try_end_idx = eblock->try_end_idx;
    e->handler_start = eblock->handler_start_offset;
    e->handler_end = eblock->handler_end_offset;
    e->handler_start_idx = eblock->handler_start_idx;
    e->handler_end_idx = eblock->handler_end_idx;
    e->catch_type_index = eblock->exception->catch_type_index;

    e->start_pc = eblock->exception->start_pc;
    e->end_pc = eblock->exception->end_pc;
    e->handler_pc = eblock->exception->handler_pc;
    e->catch_type = eblock->exception->catch_type;

    ladd_obj(m->closed_exceptions, e);

}

void identify_exception_handler_block_end(jd_method *m)
{
    /**
     * 这里的作用是找到所有的exception block的end_offset和end_idx, 方法如下:
     * 1. 根据exception block的handler_offset找到对应的normal block
     * 2. 找到对应的normal block后, 找到所有这个nblock 支配的nblock
     * 3. 找到所有支配的normal block后, 排序, 并且删除不连续的block
     * 4. 最后一个normal block就是这个exception block的end goto_offset
     **/
    m->closed_exceptions = linit_object();
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_EXCEPTION)
            continue;
        jd_eblock *eblock = block->ub->eblock;
        uint32_t _hoffset = eblock->handler_start_offset;
        jd_bblock *hb = block_start_offset(m, _hoffset);

        if (hb == NULL) {
            fprintf(stderr, "can not find basic "
                            "block with goto_offset: %d is null\n",
                    _hoffset);
            abort();
        }

        int last_block_idx = -1;
        compute_dominates_block(m, hb);
        for (int j = 0; j < hb->dominates->size; ++j) {
            jd_bblock *dominate = lget_obj(hb->dominates, j);
            if (dominate->type != JD_BB_NORMAL) 
                continue;
            if (last_block_idx == -1) {
                last_block_idx = dominate->block_id;
                continue;
            }
            if (dominate->block_id - 1 != last_block_idx &&
                    j+1 < hb->dominates->size) {
                jd_bblock *next = lget_obj(hb->dominates, j + 1);
                if (next->block_id-1 == dominate->block_id) {
                    last_block_idx = dominate->block_id;
                    continue;
                }
                break;

            }
            last_block_idx = dominate->block_id;
        }

        jd_bblock *last_block = block_by_id(m, last_block_idx);
        assert(last_block != NULL);
        jd_nblock *last_nblock = last_block->ub->nblock;
        eblock->handler_end_offset = last_nblock->end_offset;
        eblock->handler_end_idx = last_nblock->end_idx;
        DEBUG_EXCEPTION_PRINT("excpetion block: %zu, "
                              "try: %d -> %d, handler: %d -> %d, type: %d\n",
                              block->block_id,
                              eblock->try_start_offset,
                              eblock->try_end_offset,
                              eblock->handler_start_offset,
                              eblock->handler_end_offset,
                              eblock->type);
        copy_exceptions_to_closed(m, eblock);
    }
}

void identify_finally_excpetion_handler_block_end(jd_method *m)
{
    /** 找出所有finally exception的handler块对应的normal块 **/
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_EXCEPTION) 
            continue;
        jd_exc *exception = block->ub->eblock->exception;
        if (exception->catch_type_index > 0) 
            continue;
        uint32_t hsoffset = exception->handler_start;
        jd_bblock *handler_block = block_start_offset(m, hsoffset);
        assert(handler_block != NULL);
        compute_dominates_block(m, handler_block);
        size_t dominates_block_size = handler_block->dominates->size;
        if (dominates_block_size == 0) {
            fprintf(stderr, "wrong, finally handler's normal block "
                            "must dominates itself\n");
            continue;
        }

        lsort_object(handler_block->dominates,
                     (list_cmp_fn) block_id_cmp);
        size_t _last_idx = dominates_block_size - 1;
        jd_bblock *last_block = NULL;
        do {
            last_block = lget_obj(handler_block->dominates, _last_idx);
            if (last_block->type == JD_BB_NORMAL) 
                break;
            _last_idx --;
        } while (last_block != NULL && _last_idx >= 0);

        if (last_block->type != JD_BB_NORMAL) {
            fprintf(stderr, "wrong, finally handler's "
                            "dominates block need normal block\n");
            continue;
        }
        jd_ins *first_ins = handler_block->ub->nblock->start_ins;
        jd_ins *end = last_block->ub->nblock->end_ins;
        jd_ins *prev = end->prev;
        // if (jvm_ins_is_store(first_ins) &&
        //     ins_is_athrow(end) &&
        //     jvm_ins_is_load(prev) &&
        //     jvm_ins_load_slot(prev) == jvm_ins_store_slot(first_ins))
        // {}
        // else
        // {
        //     end = handler_block->ub->nblock->end_ins;
        //     prev = end->prev;
        //     if (jvm_ins_is_store(first_ins) &&
        //         ins_is_athrow(end) &&
        //         jvm_ins_is_load(prev) &&
        //         jvm_ins_load_slot(prev) == jvm_ins_store_slot(first_ins))
        //     {}
        // }
    }
}

static void merge_exception_split_by_branch_without_finally(jd_method *m)
{
    if (m->closed_exceptions->size == 0) return;
    for (int i = 0; i < m->closed_exceptions->size - 1; ++i) {
        jd_exc *cur = lget_obj(m->closed_exceptions, i);
        jd_exc *next = lget_obj(m->closed_exceptions, i + 1);
        /**
         * 在这里有几种情况
         * 1. try block被分开了，但是实际上是同一个Exception
         * 2. try block被分开了，但是不是同一种情况
         **/
        if (cur->handler_start == next->handler_start &&
            cur->handler_end == next->handler_end) {
            jd_ins *cur_last = get_ins(m, cur->try_end_idx);
            jd_ins *next_first = get_ins(m, next->try_start_idx);
            jd_ins *split_branch = next_first->prev;
            jd_ins *branch_prev = NULL;
            if (split_branch == NULL)
                continue;
            else
                branch_prev = split_branch->prev;

            jd_ins_fn *fn = split_branch->fn;
            if (!fn->is_block_end(split_branch))
                continue;

            if (cur_last != branch_prev && cur_last != split_branch)
                continue;

            // 这种情况下，就是try block被分开了
            // 但是实际上是同一个try block
            // 那么就把这个try block合并起来
//                cur->try_end = next->try_end;
//                cur->try_end_idx = next->try_end_idx;
//                cur->end_pc = next->end_pc;
            cur->try_end = MAX(cur->try_end, next->try_end);
            cur->try_end_idx = MAX(cur->try_end_idx, next->try_end_idx);
            cur->try_start = MIN(cur->try_start, next->try_start);
            cur->try_start_idx = MIN(cur->try_start_idx, next->try_start_idx);
            cur->end_pc = MAX(cur->end_pc, next->end_pc);
            cur->start_pc = MIN(cur->start_pc, next->start_pc);

            lremove_object(m->closed_exceptions, i + 1);
            i--;
        }
    }
}

static jd_exc* closest_same_handler(jd_method *m, jd_exc *ex)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);
        if (ex->handler_start == other->handler_start &&
            ex->handler_end == other->handler_end) {
            if(other == ex)
                continue;
            if ((ex->catch_type_index == 0 &&
                other->catch_type_index == 0) ||
                (other->catch_type_index > 0 && ex->catch_type_index > 0)) {
                // finally block same
                result = other;
                break;
            }
        }
    }
    return result;
}

static bool finally_in_try_handler(jd_method *m, jd_exc *try, jd_exc *finally)
{
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->closed_exceptions, i);
        if (e == finally)
            continue;
        if (e->try_start == try->try_start && e->try_end == try->try_end){
            if (e->handler_start <= finally->handler_start &&
                e->handler_end >= finally->handler_end)
                return true;
        }
    }
    return false;
}

static void merge_exception_split_by_branch_with_finally(jd_method *m)
{
    if (m->closed_exceptions->size == 0)
        return;

    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *cur = lget_obj(m->closed_exceptions, i);
        jd_exc *other = closest_same_handler(m, cur);
        if (other == NULL)
            continue;

        jd_ins *cur_end_ins = get_ins(m, cur->try_end_idx);
        jd_ins *other_start_ins = get_ins(m, other->try_start_idx);

        if (cur_end_ins->next == other_start_ins->prev)
            continue;
        // 这里是确保cur的try_end不要超过cur的handler_start
        // 如果超过了，就是错误的

        if (other->try_end < cur->handler_start /*&&
                !finally_in_try_handler(m, cur, other)*/) {
//                cur->try_end = other->try_end;
//                cur->try_end_idx = other->try_end_idx;
//                cur->end_pc = other->end_pc;
            cur->try_end = MAX(cur->try_end, other->try_end);
            cur->try_end_idx = MAX(cur->try_end_idx, other->try_end_idx);
            cur->try_start = MIN(cur->try_start, other->try_start);
            cur->try_start_idx = MIN(cur->try_start_idx, other->try_start_idx);
            cur->end_pc = MAX(cur->end_pc, other->end_pc);
            cur->start_pc = MIN(cur->start_pc, other->start_pc);

        }
        /** NOTICE: here do not compare catch type index **/
        int index = lfind_object(m->closed_exceptions,other);
        if (index >= 0) {
            lremove_object(m->closed_exceptions, index);
            i--;
        }
    }
}

static jd_exc* near_finally_catch(jd_method *m, jd_exc *finally)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *catch = lget_obj(m->closed_exceptions, i);
        if (catch->catch_type_index == 0)
            continue;
        jd_ins *catch_end = get_ins(m, catch->handler_end_idx);
        jd_ins *handler_start = get_ins(m,finally->handler_start_idx);
        if (catch_end->next == handler_start) {
            if (catch->try_start == finally->try_start &&
                catch->try_end < finally->try_end &&
                finally->try_end > catch->handler_start) {
                result = catch;
                break;
            }
        }
    }
    return result;
}

static void narrow_finally_block_near_catch_exception(jd_method *m)
{
    /**
     * TODO:
     * 这里是一个折中的方案，分块以后，finally会跟catch有交叉的部分
     * 如果判定catch的数量或者可以直接将finally的try_end_offset设置到
     * 最后一个catch的handler_end_offset
     **/

    /** 
     * 最后一个finally的try_end_offset应该是他
     * 挨着的catch的handler_start_offset的上一个位置 
     **/
    if (m->closed_exceptions->size == 0)
        return;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *finally = lget_obj(m->closed_exceptions, i);
        if (finally->catch_type_index > 0)
            continue;
        jd_exc *catch = near_finally_catch(m, finally);
        if (catch == NULL)
            continue;

        jd_ins *catch_handler_start = get_ins(m,catch->handler_start_idx);
        jd_ins *catch_handler_start_prev = catch_handler_start->prev;
        if (catch_handler_start_prev != NULL) {
            finally->try_end = catch_handler_start_prev->offset;
            finally->try_end_idx = catch_handler_start_prev->idx;
        }

        if (catch->try_end != finally->try_end &&
            catch->try_end < finally->try_end) {
            catch->try_end = finally->try_end;
            catch->try_end_idx = finally->try_end_idx;
        }
        i--;
    }

}

static jd_exc* overlapping_try_with_handler_exception(jd_method *m, jd_exc *ex)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);
        jd_range source = init_try_block_range(ex);
        jd_range target = init_handler_block_range(other);
        jd_ins *_last_ins = get_ins(m, other->handler_end_idx);
        jd_ins *_last_next_ins = _last_ins->next;
        if (exception_range_cross(&source, &target) &&
            !exception_range_contains(&source, &target) &&
            !exception_range_contains(&target, &source) &&
            exception_in_range(&target, ex->try_start) &&
            !exception_in_range(&target, ex->try_end) &&
            _last_next_ins != NULL &&
            exception_in_range(&source, _last_next_ins->offset)) {
            DEBUG_PRINT("try overlapping: %d -> %d, %d -> %d\n",
                        other->try_start,
                        other->try_end,
                        other->handler_start,
                        other->handler_end);
            result = other;
            break;
        }
    }
    return result;
}

static void fix_overlapping_try_with_handler_exception(jd_method *m)
{
    if (m->closed_exceptions->size == 0)
        return;

    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->closed_exceptions, i);
        jd_exc *other = overlapping_try_with_handler_exception(m, e);
        if (other == NULL)
            continue;

        jd_ins *other_handler_last = get_ins(m,other->handler_end_idx);
        jd_ins *other_handler_last_next = other_handler_last->next;

        e->try_start = other_handler_last_next->offset;
        e->try_start_idx = other_handler_last_next->idx;
        i--;
    }
}

static jd_exc* overlapping_try_with_try_exception(jd_method *m, jd_exc *ex)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);
        jd_range source = init_try_block_range(ex);
        jd_range target = init_try_block_range(other);
        jd_ins *_last_ins = get_ins(m, other->try_end_idx);
        jd_ins *_last_next_ins = _last_ins->next;
        if (exception_range_cross(&source, &target) &&
            !exception_range_contains(&source, &target) &&
            !exception_range_contains(&target, &source) &&
            exception_in_range(&target, ex->try_start) &&
            !exception_in_range(&target, ex->try_end) &&
            _last_next_ins != NULL &&
            exception_in_range(&source, _last_next_ins->offset)) {
            DEBUG_PRINT("try overlapping: %d -> %d, %d -> %d\n",
                        other->try_start,
                        other->try_end,
                        other->handler_start,
                        other->handler_end);
            result = other;
            break;
        }
    }
    return result;
}

static void fix_overlapping_try_with_try_exception(jd_method *m)
{
    if (m->closed_exceptions->size == 0)
        return;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->closed_exceptions, i);
        jd_exc *other = overlapping_try_with_try_exception(m, e);
        if (other == NULL)
            continue;

        jd_ins *other_try_last = get_ins(m,other->try_end_idx);
        jd_ins *other_try_last_next = other_try_last->next;

        uint32_t min_start_offset = e->try_start < other->try_start ?
                              e->try_start : other->try_start;
        uint32_t min_end_offset = e->try_end < other->try_end ?
                                e->try_end : other->try_end;

        if (e->try_start == min_start_offset) {
            other->try_end = min_end_offset;
            other->try_end_idx = e->try_end_idx;
        }
        else {
            e->try_end = min_end_offset;
            e->try_end_idx = other->try_end_idx;
        }
        i--;
    }
}

static jd_exc* same_try_exception_by_range(jd_method *m, jd_range *r)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);
        if (other->try_start == r->start_offset &&
            other->try_end == r->end_offset) {
            result = other;
            break;
        }
    }
    return result;
}

static jd_exc* smallest_try_of_range(jd_method *m, jd_range *r)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);
        if (other->try_start == r->start_offset &&
            other->try_end == r->end_offset) {
            if (result == NULL)
                result = other;
            else {
                if (other->handler_start < result->handler_start)
                    result = other;
            }
        }
    }
    return result;
}

static void fix_same_try_end_offset(jd_method *m)
{
    /**
     * dalvik的异常和jvm的异常是不一样的
     * dalvik的异常会跳转并且try块和catch块可以不连续
     */

     /**
     * 这里是确保相同的try block的end offset是一样的
     * 取值就是取第一个exception的handler block的前一个instruction的offset 
     **/
    if (m->closed_exceptions->size == 0)
        return;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->closed_exceptions, i);
        jd_range range = init_try_block_range(exception);
        jd_exc *other = same_try_exception_by_range(m, &range);
        jd_exc *smallest = smallest_try_of_range(m, &range);
//         if (other == exception)
//             continue;
        int small_idx = smallest->handler_start_idx;
        jd_ins *other_handler_start = get_ins(m, small_idx);
        jd_ins *other_handler_start_prev = other_handler_start->prev;

        while (other != NULL && other_handler_start_prev != NULL) {
            if (other->try_end != other_handler_start_prev->offset) {
                other->try_end = other_handler_start_prev->offset;
                other->try_end_idx = other_handler_start_prev->idx;
                other = same_try_exception_by_range(m, &range);
            }
            else
                other = NULL;
        }
    }
}

static jd_exc* find_next_sibling(jd_method *m, jd_range *r, uint32_t offset)
{
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);

        if (other->handler_end <= offset)
            continue;
        if (other->try_start == r->start_offset &&
            other->try_end == r->end_offset) {
            result = other;
            break;
        }
    }
    return result;
}

static void make_sure_same_try_handler_consequent(jd_method *m)
{
    // 在jvm里编译时连续的，但是dalvik和其它一些编译器不是连续的
    /** 这里是确保相同的try block的handler一直到finally是连续的 **/
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->closed_exceptions, i);
        if (exception->catch_type_index > 0)
            continue;
        jd_range range = init_try_block_range(exception);
        jd_exc *other = find_next_sibling(m, &range, 0);
        uint32_t _end_offset = other->handler_end;
        while (other != NULL) {
            jd_exc *next = find_next_sibling(m,
                                             &range,
                                             _end_offset);
            if (next == NULL)
                break;
            jd_ins *other_handler_end = get_ins(m,other->handler_end_idx);
            jd_ins *next_handler_start = get_ins(m,next->handler_start_idx);
            if (next_handler_start->prev != other_handler_end) {
                // other->try_end = next_handler_start->prev->goto_offset;
                // other->try_end_idx = next_handler_start->prev->idx;
                other->handler_end = next_handler_start->prev->offset;
                other->handler_end_idx = next_handler_start->prev->idx;
                _end_offset = next->handler_end;
            }
            else {
                other = next;
                _end_offset = next->handler_end;
            }
        }

    }
}

static void remove_duplicate_finally_for_catch_block(jd_method *m)
{
    /** 同样的finally块在java的编译过程中，会产生对每一个catch块的finally
        只留下一个finally块，其他的finally块都删除掉 **/
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *finally = lget_obj(m->closed_exceptions, i);
        if (finally->catch_type_index > 0)
            continue;

        for (int j = 0; j < m->closed_exceptions->size; ++j) {
            jd_exc *catch = lget_obj(m->closed_exceptions, j);
            if (catch->catch_type_index == 0)
                continue;
            if (catch->try_start != finally->try_start &&
                catch->try_end != finally->try_end)
                continue;
            for (int k = 0; k < m->closed_exceptions->size; ++k) {
                jd_exc *other = lget_obj(m->closed_exceptions, k);
                if (other->catch_type_index > 0 || other == finally)
                    continue;
                if (other->try_start == catch->try_start &&
                    other->handler_start == finally->handler_start &&
                    other->handler_end == finally->handler_end) {
                    int index = ldel_object(m->closed_exceptions,
                                            other);
                    if (index >= 0) {
                        k--;
                        i--;
                        j--;
                    }
                }
            }
            // if (catch->try_start == finally->try_start &&
            //    catch->try_end == finally->try_end) {
            // }
        }
    }

}

static void remove_duplicate_finally_for_try_block(jd_method *m)
{
    /**
     * finally在编译过程中，会对try块做一份copy, 这里就是删除掉这些copy的try块
     * Exception table:
         from    to  target type
             0     4    11   Class java/lang/ExceptionInInitializerError
             0     4    23   Class java/util/ServiceConfigurationError
             0     4    35   Class java/lang/StackOverflowError
             0     4    47   Class java/lang/Exception
             0     4    59   any
            11    16    59   any
            23    28    59   any
            35    40    59   any
            47    52    59   any
     **/

    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *finally = lget_obj(m->closed_exceptions, i);
        if (finally->catch_type_index > 0)
            continue;
        for (int j = 0; j < m->closed_exceptions->size; ++j) {
            jd_exc *other = lget_obj(m->closed_exceptions, j);
            if (other->catch_type_index > 0 ||
                other == finally ||
                !exception_same_handler_range(finally, other))
                continue;

            jd_range source = init_try_block_range(finally);
            jd_range target = init_try_block_range(other);
            if (!exception_range_contains(&source, &target))
                continue;
            if (finally->try_end == other->try_end) {
                int index = lfind_object(m->closed_exceptions,
                                         other);
                if (index > 0) {
                    lremove_object(m->closed_exceptions, index);
                    if (j < i) {
                        i--;
                        break;
                    }
                    j--;
                }
            }
        }
    }
}

static jd_exc* find_first_handler(jd_method *m, jd_exc *e)
{
    /** 同一个try, 对应着多个catch块, 这里找到最靠前的那个catch块, 
     * 也就是handler_start_offset最小的那个 **/
    jd_exc *result = NULL;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *other = lget_obj(m->closed_exceptions, i);
        if (other->try_start == e->try_start &&
            other->try_end == e->try_end) {
            if (result == NULL) {
                result = other;
                continue;
            }
            if (other->handler_start < result->handler_start)
                result = other;
        }
    }
    return result;
}

static void fix_same_try_edge(jd_method *m)
{
    /** 
     * 这里是确保相同的try block每个try_end_offset结束后
     * 必须先进入到最小的那个catch的块里 
     **/
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->closed_exceptions, i);
        jd_exc *smallest = find_first_handler(m, exception);
        if (smallest == NULL)
            continue;
        jd_ins *exception_try_end = get_ins(m,
                                            exception->try_end_idx);
        jd_ins *exception_try_end_next = exception_try_end->next;
        jd_ins *small_handler_start = get_ins(m,
                                              smallest->handler_start_idx);
        jd_ins *smallest_handler_start_prev = small_handler_start->prev;
        if (exception_try_end_next != small_handler_start &&
            exception_try_end_next != NULL ) {
            // somthing is wrong
            jd_range new_range = init_exception_range(-1, -1);
            // 如果两个块之间只有一个instruction,那么是可以把try block向后推的
            // 否则无法确认try和第一个catch之间是什么
            if (smallest_handler_start_prev != exception_try_end_next) 
                continue;

            jd_ins_fn *fn = smallest_handler_start_prev->fn;
            if (fn->is_return(smallest_handler_start_prev) ||
                fn->is_branch(smallest_handler_start_prev)) {
                new_range.start_offset = exception->try_start;
                new_range.end_offset = smallest_handler_start_prev->offset;
            }
            else if (fn->is_athrow(smallest_handler_start_prev) ||
                    (fn->is_return(smallest_handler_start_prev) &&
                    smallest_handler_start_prev->code != INS_RETURN)) {
                new_range.start_offset = exception->try_start;
                new_range.end_offset = smallest_handler_start_prev->offset;
            }

            if (new_range.start_offset == -1 && new_range.end_offset == -1) 
                continue;

            for (int j = 0; j < m->closed_exceptions->size; ++j) {
                jd_exc *same_try = lget_obj(m->closed_exceptions, j);
                if (exception_same_try_range(same_try, exception)) {
                    same_try->try_end = new_range.end_offset;
                    same_try->try_end_idx = smallest_handler_start_prev->idx;
                }
            }
        }
    }
}

static void remove_crossed_finally_handler(jd_method *m)
{
    // 某些finally在计算过程中，变成try块是某一个catch块的try块
    // handler块对应着一个finally块，删除这样的exception
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *finally = lget_obj(m->closed_exceptions, i);
        if (finally->catch_type_index > 0)
            continue;
        jd_exc *closest_ex = NULL;
        for (int j = 0; j < m->closed_exceptions->size; ++j)
        {
            jd_exc *other = lget_obj(m->closed_exceptions, j);
            if (other == finally)
                continue;
            if (closest_ex == NULL)
                closest_ex = other;
            else
            {
                if (other->try_start > closest_ex->try_start)
                    closest_ex = other;
            }
        }

        if (closest_ex == NULL ||
            closest_ex->catch_type_index == 0)
            continue;

        for (int j = 0; j < m->closed_exceptions->size; ++j) {
            jd_exc *other = lget_obj(m->closed_exceptions, j);
            if (other == finally || other == closest_ex)
                continue;
            jd_range other_try = init_try_block_range(other);
            jd_range other_handler = init_handler_block_range(other);
            jd_range exception_handler = init_handler_block_range(finally);
            jd_range closet_handler = init_handler_block_range(closest_ex);
            if (exception_range_same(&other_try, &exception_handler) &&
                exception_range_same(&other_handler, &closet_handler)) {
                int index = lfind_object(m->closed_exceptions,
                                         other);
                if (index > 0) {
                    lremove_object(m->closed_exceptions, index);
                    if (j < i) {
                        i--;
                        break;
                    }
                    j--;
                }
            }
        }
    }
}

static void remove_share_handler_finally(jd_method *m)
{
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *finally = lget_obj(m->closed_exceptions, i);
        if (finally->catch_type_index > 0)
            continue;
        bool crossed = false;
        for (int j = 0; j < m->closed_exceptions->size; ++j)
        {
            jd_exc *other = lget_obj(m->closed_exceptions, j);
            if (other == finally)
                continue;
            if (finally->try_start >= other->handler_start &&
                ((finally->handler_start >= other->try_start &&
                  finally->handler_start <= other->try_end) || (finally->handler_start <= other->try_start))) {
                crossed = true;
                break;
            }
        }

        if (crossed) {
            ldel_obj(m->closed_exceptions, finally);
            i--;
        }
    }
}

static void remove_share_hanlder_catch(jd_method *m)
{
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *catch_block = lget_obj(m->closed_exceptions, i);
        if (catch_block->catch_type_index == 0)
            continue;
        bool crossed = false;
        for (int j = 0; j < m->closed_exceptions->size; ++j)
        {
            jd_exc *other = lget_obj(m->closed_exceptions, j);
            if (other == catch_block)
                continue;
            if (catch_block->try_start >= other->handler_start &&
                ((catch_block->handler_start >= other->try_start &&
                  catch_block->handler_start <= other->try_end) || (catch_block->handler_start <= other->try_start))) {
                crossed = true;
                break;
            }
        }

        if (crossed) {
            ldel_obj(m->closed_exceptions, catch_block);
            i--;
        }
    }
}

static void remove_empty_catch_body_exception(jd_method *m)
{
    /**
     *  try {
     *      // do something
     *  } catch(Exception e) {
     *      // empty block
     *  }
     *  TODO: 这样的catch block里面只有一个astore的instruction，是不是要NOP掉？
     **/
    if (m->closed_exceptions->size == 0)
        return;
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->closed_exceptions, i);
        if (e->catch_type_index == 0) 
            continue;
        jd_ins *start = get_ins(m, e->handler_start_idx);
        jd_ins *end = get_ins(m, e->handler_end_idx);
        if (start != end || !jvm_ins_is_store(start))
            continue;
    }
}

static void remove_useless_finally_exception(jd_method *m)
{
    /** 
     * 这种情况是try里面只有一条指令
     * 那么就是一个空的try block,这种情况下,就删除这个block 
     **/
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->closed_exceptions, i);
        if (e->catch_type_index != 0) 
            continue;
        if (e->try_start == e->handler_start) {
            ldel_obj(m->closed_exceptions, e);
            DEBUG_EXCEPTION_PRINT("remove empty try block\n");
            i--;
            continue;
        }
    }
}

void pullin_block_jump_into_exception_try_block(jd_method *m)
{
    // there are some m generate by obf tools or dex2jar..
    // exception's try block have some predecessors which not in exception

    for (int i = 0; i < m->mix_exceptions->size; ++i) {
        jd_mix_exception *exception = lget_obj(m->mix_exceptions, i);
        uint32_t start = exception->try->start_offset;
        uint32_t end = exception->try->end_offset;

        jd_range *last_catch = is_list_empty(exception->catches) ? NULL :
            lget_obj_last(exception->catches);

        jd_range *finally = exception->finally;
        jd_range *last = finally == NULL ? last_catch : finally;
        end = MAX(last->end_offset, end);

        for (int j = 0; j < m->basic_blocks->size; ++j) {
            jd_bblock *block = lget_obj(m->basic_blocks, j);
            if (!basic_block_is_normal_live(block)) continue;
            jd_nblock *nblock = block->ub->nblock;
            if (nblock->start_offset >= start &&
                nblock->end_offset <= end)
                continue;

            for (int k = 0; k < block->out->size; ++k) {
                jd_edge *edge = lget_obj(block->out, k);
                jd_bblock *target = edge->target_block;
                if (!basic_block_is_normal_live(target))
                    continue;
                jd_nblock *tnb = target->ub->nblock;
                if (tnb->start_offset > start && tnb->end_offset < end) {
                    // jump into exception

                    if (block->in->size == 1 &&
                        jvm_ins_is_goto(block->ub->nblock->end_ins)) {
                        // 这种情况是一个goto指令跳转到异常块的开始
                        jd_edge *in_edge = lget_obj(block->in, 0);
                        jd_bblock *in_block = in_edge->source_block;
                        if (basic_block_is_normal_live(in_block))
                            nblock = in_block->ub->nblock;
                    }

                    if (nblock->start_offset < start) {
                        exception->try->start_offset = nblock->start_offset;
                        exception->try->start_idx = nblock->start_idx;
                        start = MIN(exception->try->start_offset, start);
                    }

                    if (nblock->start_offset > end) {
                        last->end_offset = nblock->end_offset;
                        last->end_idx = nblock->end_idx;
                        end = last->end_offset;
                    }
                }
            }


        }
    }

    if (DEBUG_EXCEPTION)
    {
        for (int i = 0; i < m->mix_exceptions->size; ++i) {
            jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
            jd_range *try = e->try;
            jd_range *f = e->finally;
            printf("----------------- start ------------------------\n");
            printf("try: %d -> %d\n", try->start_offset, try->end_offset);
            for (int j = 0; j < e->catches->size; ++j) {
                jd_range *c = lget_obj(e->catches, j);
                printf("\tcatch: %d -> %d\n", c->start_offset, c->end_offset);
            }
            if (f != NULL)
                printf("\tfinally: %d -> %d\n",
                       f->start_offset, f->end_offset);

            printf("----------------- end ------------------------\n");
        }
    }

}

static void expand_exception_with_jump(jd_method *m)
{
    /**
     * 这里的作用是如果一个跳转到try块里的指令，但是不是跳转到try块的开始
     * 那么就把这个try块扩展到这个指令的位置
     **/
    for (int i = 0; i < m->closed_exceptions->size; ++i) {
        jd_exc *exc = lget_obj(m->closed_exceptions, i);
        jd_ins *start = get_ins(m, exc->try_start_idx);
        jd_ins *end = get_ins(m, exc->try_end_idx);
        start = start->next;

        while (start != NULL && start->offset <= end->offset) {
            if (!is_list_empty(start->comings)) {
                for (int j = 0; j < start->comings->size; ++j) {
                    jd_ins *jump = lget_obj(start->comings, j);
                    if (jump->offset < exc->try_start) {
                        exc->try_start = jump->offset;
                        exc->try_start_idx = jump->idx;
                    }
                }
            }
            start = start->next;
        }
    }
}

void cleanup_full_exception_table(jd_method *m)
{
    if (m->closed_exceptions->size == 0)
        return;
    DEBUG_EXCEPTION_PRINT("\n1 ----------->\n");
    print_full_exception_table(m);
    remove_useless_finally_exception(m);

    remove_empty_catch_body_exception(m);

    DEBUG_EXCEPTION_PRINT("\n2 ----------->\n");
    print_full_exception_table(m);
    expand_exception_with_jump(m);

    DEBUG_EXCEPTION_PRINT("\n2.1 ----------->\n");
    print_full_exception_table(m);
    remove_share_handler_finally(m);
    remove_share_hanlder_catch(m);


    DEBUG_EXCEPTION_PRINT("\n3 ----------->\n");
    print_full_exception_table(m);
    merge_exception_split_by_branch_without_finally(m);

    DEBUG_EXCEPTION_PRINT("\n4 ----------->\n");
    print_full_exception_table(m);
    merge_exception_split_by_branch_with_finally(m);

    DEBUG_EXCEPTION_PRINT("\n5 ----------->\n");
    print_full_exception_table(m);
    narrow_finally_block_near_catch_exception(m);

    DEBUG_EXCEPTION_PRINT("\n6 ----------->\n");
    print_full_exception_table(m);
    fix_overlapping_try_with_handler_exception(m);

    DEBUG_EXCEPTION_PRINT("\n7 ----------->\n");
    print_full_exception_table(m);
    fix_same_try_end_offset(m);

    DEBUG_EXCEPTION_PRINT("\n8 ----------->\n");
    print_full_exception_table(m);
    make_sure_same_try_handler_consequent(m);

    DEBUG_EXCEPTION_PRINT("\n9 ----------->\n");
    print_full_exception_table(m);
    remove_duplicate_finally_for_catch_block(m);

    DEBUG_EXCEPTION_PRINT("\n10 ----------->\n");
    print_full_exception_table(m);
    remove_duplicate_finally_for_try_block(m);

    DEBUG_EXCEPTION_PRINT("\n11 ----------->\n");
    print_full_exception_table(m);
    fix_same_try_edge(m);

    remove_crossed_finally_handler(m);
}

void flatten_exceptions(jd_method *m)
{
    buble_sort_cfg_exception(m, (list_cmp_fn) try_end_cmp);
    buble_sort_cfg_exception(m, (list_cmp_fn) handler_end_cmp);
    buble_sort_cfg_exception(m, (list_cmp_fn) handler_start_cmp);
    buble_sort_cfg_exception(m, (list_cmp_fn) try_start_cmp);


//     print_cfg_exception_table(m);
    m->mix_exceptions = linit_object();
    int new_exception = 1;
    jd_mix_exception *current = NULL;

    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->cfg_exceptions, i);
        jd_exc *next_exception = NULL;
        if (i < m->cfg_exceptions->size - 1)
            next_exception = lget_obj(m->cfg_exceptions, i + 1);

        if (new_exception) {
            current = make_obj(jd_mix_exception);
            current->catches = linit_object();
        }

        if (exception->catch_type_index > 0) {
            jd_range *try = make_obj(jd_range);
            try->start_offset = exception->try_start;
            try->end_offset = exception->try_end;
            try->start_idx = exception->try_start_idx;
            try->end_idx = exception->try_end_idx;
            if (current->try == NULL)
                current->try = try;

            jd_range *catch = make_obj(jd_range);
            catch->start_offset = exception->handler_start;
            catch->start_idx = exception->handler_start_idx;
            catch->end_offset = exception->handler_end;
            catch->end_idx = exception->handler_end_idx;
            ladd_obj(current->catches, catch);
        }
        else {
            jd_range *try = make_obj(jd_range);
            try->start_offset = exception->try_start;
            try->end_offset = exception->try_end;
            try->start_idx = exception->try_start_idx;
            try->end_idx = exception->try_end_idx;
            if (current->try == NULL)
                current->try = try;

            jd_range *finally = make_obj(jd_range);
            finally->start_offset = exception->handler_start;
            finally->end_offset = exception->handler_end;
            finally->start_idx = exception->handler_start_idx;
            finally->end_idx = exception->handler_end_idx;
            current->finally = finally;
        }

        if (
                // exception->catch_type_index == 0 ||
                (next_exception == NULL) ||
                (next_exception != NULL &&
                (next_exception->try_start != exception->try_start ||
                 next_exception->try_end != exception->try_end))
            ) {
            new_exception = 1;
            ladd_obj(m->mix_exceptions, current);
        }
        else
            new_exception = 0;
    }

    if (DEBUG_EXCEPTION)
    {
        for (int i = 0; i < m->mix_exceptions->size; ++i) {
            jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
            jd_range *try = e->try;
            jd_range *f = e->finally;
            printf("----------------- start ------------------------\n");
            printf("try: %d -> %d\n", try->start_offset, try->end_offset);
            for (int j = 0; j < e->catches->size; ++j) {
                jd_range *c = lget_obj(e->catches, j);
                printf("\tcatch: %d -> %d\n", c->start_offset, c->end_offset);
            }
            if (f != NULL)
                printf("\tfinally: %d -> %d\n", 
                        f->start_offset, f->end_offset);

            printf("----------------- end ------------------------\n");
        }
    }
}

jd_exc* closest_exception_of(jd_method *m, uint32_t offset)
{
    jd_exc *current = NULL;
    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *exception = lget_obj(m->cfg_exceptions, i);
        if (offset >= exception->try_start &&
            offset <= exception->try_end) {
            // instruction->offset在赋值过来的时候，已经-1了, 所以要用等于
            if (current == NULL) {
                current = exception;
                continue;
            }
            if (exception->start_pc > current->start_pc)
                current = exception;
            if (exception->start_pc == current->start_pc &&
                exception->end_pc < current->end_pc)
                current = exception;

            // 在第二轮时候，try_end和handler的范围都会有重新修改,
            // 所以closest exception需要判定try_end的范围
            if (exception->try_start >= current->try_start &&
                exception->try_end <= current->try_end)
                current = exception;
        }

    }
    return current;
}



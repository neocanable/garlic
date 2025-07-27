#include "decompiler/control_flow.h"
#include "jvm/jvm_ins.h"
#include "decompiler/expression.h"
#include "decompiler/dominator_tree.h"

bool ins_is_handler_start(jd_method *m, jd_ins *ins)
{
    for (int i = 0; i < m->mix_exceptions->size; ++i) {
        jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
        if (!is_list_empty(e->catches)) {
            for (int j = 0; j < e->catches->size; ++j) {
                jd_range *range = lget_obj(e->catches, j);
                if (range->start_offset == ins->offset)
                    return true;
            }
        }
        if (e->finally != NULL && e->finally->start_offset == ins->offset)
            return true;
    }

    return false;
}

bool ins_is_try_end(jd_method *m, jd_ins *ins)
{
    jd_ins *next = ins->next;
    if (next == NULL)
        return false;
    if (ins_is_handler_start(m, next))
        return true;
    return false;
}

static inline jd_edge* find_edge_with(jd_bblock *source, jd_bblock *target)
{
    jd_edge *result = NULL;
    jd_edge *edge = NULL;
    for (int i = 0; i < source->out->size; ++i) {
        edge = lget_obj(source->out, i);
        if (edge->target_block_id == target->block_id)
            result = edge;
    }
    return result;
}

bool basic_block_id_comparator(const jd_bblock *a, const jd_bblock *b)
{
    return a->block_id == b->block_id;
}

void basic_block_clear_visited_flag(jd_method *m)
{
    int size = m->basic_blocks->size - 1;
    jd_bblock *f = NULL;
    jd_bblock *l = NULL;
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        f = lget_obj(m->basic_blocks, i);
        l = lget_object(m->basic_blocks, size - i);
        f->visited = 0;
        l->visited = 0;
        if (size - i == i)
            break;
    }
}

jd_edge* create_link_edge(jd_bblock *source, jd_bblock *target)
{
    if (source == NULL || target == NULL)
        return NULL;
    jd_edge *edge = find_edge_with(source, target);
    if (edge != NULL)
        return edge;
    edge = make_obj(jd_edge);
    edge->source_block_id = source->block_id;
    edge->target_block_id = target->block_id;
    edge->source_block = source;
    edge->target_block = target;
    if (source->type == JD_BB_EXCEPTION ||
        target->type == JD_BB_EXCEPTION)
        edge->type = JD_EDGE_EXCEPTION;
    else
        edge->type = JD_EDGE_NORMAL;

    ladd_obj(source->out, edge);
    ladd_obj(target->in, edge);
    return edge;
}

static void basic_block_init_common_object(jd_bblock *block)
{
    block->in = linit_object_with_capacity(2);
    block->out = linit_object_with_capacity(2);
    block->dom_children = linit_object();
    block->frontier = linit_object_with_capacity(2);
    block->frontier->cmp_fn = (list_cmp_fn) basic_block_id_comparator;
    block->dominates = linit_object();
}

jd_bblock* cfg_create_basic_block(jd_method *m, int id, jd_bblock_type t)
{
    jd_bblock *block = make_obj(jd_bblock);
    block->method = m;
    block->block_id = id;
    block->type = t;
    block->live = JD_STATUS_BUSY;
    basic_block_init_common_object(block);
    return block;
}

static void cfg_default_block(jd_method *m)
{
    jd_bblock *enter = block_by_id(m, JD_BB_ENTER_ID);
    jd_bblock *exit = block_by_id(m, JD_BB_EXIT_ID);
    jd_bblock *exception = block_by_id(m, JD_BB_EXCEPTION_ID);

    if (enter == NULL) {
        enter = cfg_create_basic_block(m,
                                       JD_BB_ENTER_ID,
                                       JD_BB_ENTER);
        ladd_obj(m->basic_blocks, enter);
    }

    if (exit == NULL) {
        exit = cfg_create_basic_block(m,
                                      JD_BB_EXIT_ID,
                                      JD_BB_EXIT);
        ladd_obj(m->basic_blocks, exit);
    }

    if (exception == NULL) {
        exception = cfg_create_basic_block(m,
                                           JD_BB_EXCEPTION_ID,
                                           JD_BB_EXCEPTION_EXIT);
        ladd_obj(m->basic_blocks, exception);
    }

    enter->live = JD_STATUS_BUSY;
    exit->live = JD_STATUS_BUSY;
    exception->live = JD_STATUS_BUSY;
}

jd_bblock* block_by_id(jd_method *m, int block_id)
{
    if (block_id < m->basic_blocks->size)
        return lget_obj(m->basic_blocks, block_id);
    return NULL;
}

jd_bblock* block_handler_equals_offset(jd_method *m, uint32_t offset)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *basic_block = lget_obj(m->basic_blocks, i);
        if (basic_block->type != JD_BB_EXCEPTION)
            continue;

        jd_eblock *eblock = basic_block->ub->eblock;
        if (eblock->handler_start_offset == offset)
            return basic_block;
    }
    return NULL;
}

jd_bblock* block_handler_equals_ins(jd_method *m, jd_ins *ins)
{
    return block_handler_equals_offset(m, ins->offset);
}

jd_bblock* block_start_offset(jd_method *m, uint32_t offset)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type == JD_BB_NORMAL) {
            jd_nblock *nblock = block->ub->nblock;
            if (nblock->start_offset == offset)
                return block;
        }
    }
    return NULL;
}

jd_bblock* block_contains_idx(jd_method *m, int idx)
{
    jd_exp *exp = get_exp(m, idx);
    if (exp == NULL || exp->block == NULL)
        return NULL;

    if (!basic_block_is_normal_live(exp->block))
        return NULL;

    return exp->block;
}

jd_bblock* exp_block(jd_exp *exp)
{
    if (exp == NULL || exp->block == NULL)
        return NULL;
    if (!basic_block_is_normal_live(exp->block))
        return NULL;
    return exp->block;
}

jd_bblock* block_closest_finally(jd_method *m, jd_bblock *e)
{
    jd_bblock *closest_finally = NULL;
    jd_eblock *eblock = e->ub->eblock;
    for (int j = 0; j < m->basic_blocks->size; ++j) {
        jd_bblock *other = lget_obj(m->basic_blocks, j);
        if (other->type != JD_BB_EXCEPTION)
            continue;
        jd_eblock *_oe = other->ub->eblock;

        if (_oe->try_start_offset == eblock->try_start_offset &&
            _oe->type == JD_EXCEPTION_FINALLY) {
            if (closest_finally == NULL)
                closest_finally = other;
            else {
                jd_eblock *_ce = closest_finally->ub->eblock;
                if (_oe->handler_start_offset > _ce->handler_start_offset)
                    closest_finally = other;
                if (_oe->handler_start_offset == _ce->handler_start_offset &&
                    _oe->handler_end_offset < _ce->handler_end_offset)
                    closest_finally = other;
            }
        }
    }
    return closest_finally;
}

jd_bblock* block_closest_handler(jd_method *m, jd_bblock *block)
{
    jd_bblock *result = NULL;
    jd_nblock *nblock = block->ub->nblock;
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *other_block = lget_obj(m->basic_blocks, i);
        if (other_block->type != JD_BB_EXCEPTION)
            continue;
        jd_eblock *other = other_block->ub->eblock;
        if (other->handler_start_offset <= nblock->start_offset &&
            other->handler_end_offset >= nblock->end_offset) {
            if (result == NULL) {
                result = other_block;
                continue;
            }
            jd_eblock *eblock = result->ub->eblock;
            if (other->handler_start_offset > eblock->handler_start_offset)
                result = other_block;
            if (other->handler_start_offset == eblock->handler_start_offset &&
                other->handler_end_offset < eblock->handler_end_offset)
                result = other_block;
        }
    }
    return result;
}

jd_bblock* block_by_offset(jd_method *m, uint32_t offset)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *basic_block = lget_obj(m->basic_blocks, i);
        if (basic_block->type == JD_BB_NORMAL) {
            jd_nblock *nblock = basic_block->ub->nblock;
            if (nblock->start_offset <= offset &&
                nblock->end_offset >= offset)
                return basic_block;
        }
        else if (basic_block->type == JD_BB_EXCEPTION) {
            jd_eblock *eblock = basic_block->ub->eblock;
            if (eblock->try_start_offset <= offset &&
                eblock->try_end_offset >= offset)
                return basic_block;
        }
    }
    return NULL;
}

jd_bblock* block_exception_exit(jd_method *m)
{
    return block_by_id(m, JD_BB_EXCEPTION_ID);
}

jd_bblock* block_enter(jd_method *m)
{
    return block_by_id(m, JD_BB_ENTER_ID);
}

jd_bblock* block_parent_exception(jd_method *m, jd_bblock *block)
{
    assert(block->type == JD_BB_EXCEPTION);
    jd_eblock *eblock = block->ub->eblock;
    jd_bblock *parent_block = NULL;
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *basic_block = lget_obj(m->basic_blocks, i);
        if (basic_block->type != JD_BB_EXCEPTION)
            continue;

        jd_eblock *eblock2 = basic_block->ub->eblock;
        if (eblock->handler_start_offset >= eblock2->try_start_offset &&
                eblock->handler_start_offset <= eblock2->try_end_offset) {
            if (parent_block == NULL) {
                parent_block = basic_block;
                continue;
            }
            jd_eblock *_pe = parent_block->ub->eblock;
            if (eblock2->try_start_offset > _pe->try_start_offset)
                parent_block = basic_block;
            if (eblock2->try_start_offset == _pe->try_start_offset &&
                eblock2->try_end_offset < _pe->try_end_offset)
                parent_block = basic_block;
        }
    }
    return parent_block;
}

jd_bblock* dup_basic_block(jd_method *m, jd_bblock *src)
{
    jd_bblock *block = make_obj(jd_bblock);
    block->method = m;
    block->block_id = m->basic_blocks->size;
    block->type = src->type;
    block->live = JD_STATUS_BUSY;
    block->is_dup = true;
    block->node = NULL;

    block->in = linit_object_with_capacity(2);
    block->out = linit_object_with_capacity(2);
    block->dom_children = linit_object();
    block->frontier = linit_object_with_capacity(2);
    block->frontier->cmp_fn = (list_cmp_fn) basic_block_id_comparator;
    block->dominates = linit_object();

    block->ub = make_obj(jd_union_basic_block);
    block->ub->nblock = make_obj(jd_nblock);

    jd_nblock *nb = block->ub->nblock;
    jd_nblock *src_nb = src->ub->nblock;
    memcpy(nb, src_nb, sizeof(jd_nblock));

    jd_ins *end_ins = src_nb->end_ins;
    jd_ins_fn *fn = end_ins->fn;
    if (fn->is_goto(end_ins) && end_ins->prev != NULL) {
        nb->end_offset = end_ins->prev->offset;
        nb->end_idx = end_ins->prev->idx;
        nb->end_ins = end_ins->prev;
    }

    ladd_obj(m->basic_blocks, block);

    DEBUG_PRINT("[dup-basic-block] %zu %d -> %d\n",
           block->block_id, nb->start_offset, nb->end_offset);

    return block;
}


string list_block_id_join(list_object *list, string delimiter)
{
    if (list->size == 0)
        return NULL;
    size_t total_length = list->size * 12 + (list->size - 1) + 1;
    char *result = x_alloc(total_length);

    size_t index = 0;
    for (size_t i = 0; i < list->size; ++i) {
        jd_bblock *block = lget_obj(list, i);
        int written = snprintf(result + index,
                               total_length - index,
                               "%zu",
                               block->block_id);

        if (written < 0 || written >= total_length - index)
            perror("String formatting error");
        index += written;
        if (i < list->size - 1) 
            result[index++] = ',';
    }
    result[index] = '\0';
    return result;
}

string list_edge_id_join(list_object *list, int type)
{
    if (list->size == 0)
        return NULL;
    size_t total_length = list->size * 12 + (list->size - 1) + 1;
    char *result = x_alloc(total_length);

    size_t index = 0;
    for (size_t i = 0; i < list->size; ++i) {
        jd_edge *edge = lget_obj(list, i);
        int blk_id = type == 0 ? edge->source_block_id :
                                 edge->target_block_id;

        int written = snprintf(result + index,
                               total_length - index,
                               "%d",
                               blk_id);

        if (written < 0 || written >= total_length - index)
            perror("String formatting error");
        index += written;

        if (i < list->size - 1)
            result[index++] = ',';
    }
    result[index] = '\0';
    return result;
}

string list_in_edge_id_join(list_object *list)
{
    return list_edge_id_join(list, 0);
}

string list_out_edge_id_join(list_object *list)
{
    return list_edge_id_join(list, 1);
}

void print_dominator_tree(jd_method *m, jd_bblock *block)
{
    if (block == NULL)
        block = lget_object(m->basic_blocks, 3);

    jd_bblock *idom = block->idom;
    while (idom != NULL && idom->block_id != 0) {
        printf("    ");
        idom = idom->idom;
    }
    if (block->type == JD_BB_NORMAL) {
        jd_nblock *nb = block->ub->nblock;
        if (block->is_dup)
            printf("BL: %zu[dupped] %x - %x\n", 
                    block->block_id, 
                    nb->start_offset, 
                    nb->end_offset);
        else
            printf("BL: %zu %x - %x\n", 
                    block->block_id, 
                    nb->start_offset, 
                    nb->end_offset);
    }
    else {
        printf("BL: %zu\n", block->block_id);
    }

    for (int i = 0; i < block->dom_children->size; ++i) {
        jd_bblock *child = lget_obj(block->dom_children, i);
        print_dominator_tree(m, child);
    }
}

void print_basic_block(jd_method *m)
{
    if (!DEBUG_PRINT_BASIC_BLOCK) return;
    fprintf(stdout, "============ m: %s, "
                    "block_size: %zu ============\n\n",
            m->name, m->basic_blocks->size);

    for (int i = 0; i < m->basic_blocks->size; ++i)
    {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (!basic_block_is_normal_live(block))
            continue;
        switch (block->type) {
            case JD_BB_ENTER: {
                fprintf(stdout, "block_id: %zu\n", block->block_id);
                break;
            }
            case JD_BB_EXCEPTION_EXIT: {
                fprintf(stdout, "block_id: %zu\n", block->block_id);
                break;
            }
            case JD_BB_NORMAL: {
                jd_nblock *nblock = block->ub->nblock;
                fprintf(stdout, "block_id: %zu, %d -> %d %d -> %d\n",
                        block->block_id,
                        nblock->start_offset,
                        nblock->end_offset,
                        nblock->start_idx,
                        nblock->end_idx);
                break;
            }
            case JD_BB_EXCEPTION: {
                jd_eblock *eblock = block->ub->eblock;
                fprintf(stdout, 
                        "block_id: %zu, %d -> %d, "
                        "handler: %d -> %d, exception: %d\n",
                        block->block_id,
                        eblock->try_start_offset,
                        eblock->try_end_offset,
                        eblock->handler_start_offset,
                        eblock->handler_end_offset,
                        eblock->exception->catch_type_index);
                break;
            }
            case JD_BB_EXIT:
                fprintf(stdout, "block_id: %zu\n", block->block_id);
                break;
            default:
                break;
        }
        if (block->idom != NULL)
            fprintf(stdout, "\tidom: %zu\n", block->idom->block_id);

        fprintf(stdout, "\tin: %s cap: %zu\n",
                list_in_edge_id_join(block->in), block->in->capacity);
        fprintf(stdout, "\tout: %s cap: %zu\n",
                list_out_edge_id_join(block->out), block->out->capacity);
        fprintf(stdout, "\tdominator_children: %s\n",
                list_block_id_join(block->dom_children, ","));
        fprintf(stdout, "\tdominace_frontier: %s\n",
                list_block_id_join(block->frontier, ","));
        fprintf(stdout, "\tdominates_blocks: %s\n",
                list_block_id_join(block->dominates, ","));
    }
}

void cfg_unlink_edge(jd_edge *edge)
{
    jd_bblock *source = edge->source_block;
    jd_bblock *target = edge->target_block;
    ldel_obj(source->out, edge);
    ldel_obj(target->in, edge);
}

void cfg_unlink_blocks(jd_bblock *source, jd_bblock *target)
{
    jd_edge *edge = find_edge_with(source, target);
    if (edge != NULL)
        cfg_unlink_edge(edge);
}

void cfg_remove_exception_block(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (b->type != JD_BB_NORMAL)
            continue;

        for (int j = 0; j < b->out->size; ++j) {
            jd_edge *edge = lget_obj(b->out, j);
            if (edge->type != JD_EDGE_EXCEPTION)
                continue;

            jd_bblock *exception_block = edge->target_block;
            jd_eblock *eblock = exception_block->ub->eblock;
            uint32_t hstart = eblock->handler_start_offset;
            jd_bblock *hblock = block_start_offset(m, hstart);
            create_link_edge(b, hblock);
        }
    }

    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (b->type != JD_BB_NORMAL) continue;

        for (int j = 0; j < b->out->size; ++j) {
            jd_edge *edge = lget_obj(b->out, j);
            if (edge->target_block->type == JD_BB_EXCEPTION) {
                cfg_unlink_blocks(b, edge->target_block);
                j--;
            }
        }
    }
}

static void cfg_create_normal_blocks(jd_method *m)
{
    cfg_default_block(m);

    int block_id = NORMAL_BLOCK_START_ID;
    jd_ins_fn *fn = get_ins(m, 0)->fn;
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_ins *start_ins = get_ins(m, i);
        jd_exc *start_exc = closest_exception_of(m, start_ins->offset);

        jd_bblock *basic_block = cfg_create_basic_block(m,
                                                        block_id,
                                                        JD_BB_NORMAL);
        uint32_t off = 0;
        for (int j = i; j < m->instructions->size; ++j) {
            jd_ins *ins = get_ins(m, j);
            jd_ins *next_ins = ins->next;
            ins->block = basic_block;
            if (next_ins != NULL) {
                off = next_ins->offset;
                jd_exc *next_exc = closest_exception_of(m, off);

                if (start_exc != next_exc) {
                    i = j;
                    break;
                }

                if (fn->is_block_end(ins) || fn->is_block_start(next_ins)) {
                    i = j;
                    break;
                }
            }
            else {
                i = j;
                break;
            }

        }
        jd_ins *instruction = get_ins(m, i);
        jd_nblock *normal_block = make_obj(jd_nblock);
        normal_block->start_idx = start_ins->idx;
        normal_block->end_idx = instruction->idx;
        normal_block->start_offset = start_ins->offset;
        normal_block->end_offset = instruction->offset;
        normal_block->start_ins = start_ins;
        normal_block->end_ins = instruction;
        basic_block->ub = make_obj(jd_union_basic_block);
        basic_block->ub->nblock = normal_block;
        ladd_obj(m->basic_blocks, basic_block);
        block_id++;
    }
}

static void cfg_create_exception_blocks(jd_method *m)
{
    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        int block_id = m->basic_blocks->size;
        jd_exc *exception = lget_obj(m->cfg_exceptions, i);
        jd_bblock *bblock = cfg_create_basic_block(m,
                                                   block_id,
                                                   JD_BB_EXCEPTION);
        jd_eblock *eblock = make_obj(jd_eblock);
        bblock->ub = make_obj(jd_union_basic_block);
        bblock->ub->eblock = eblock;

        jd_ins *ex_end_ins = get_ins(m,
                                     exception->try_end_idx);
        jd_ins *last_ins = lget_obj_last(m->instructions);
        if (last_ins->idx == exception->try_end_idx)
            ex_end_ins = last_ins;

        eblock->idx = exception->idx;
        eblock->exception = exception;
        eblock->try_start_offset = exception->try_start;
        eblock->try_end_offset = ex_end_ins->offset;
        eblock->handler_start_offset = exception->handler_start;

        eblock->try_start_idx = exception->try_start_idx;
        eblock->try_end_idx = ex_end_ins->idx;
        eblock->handler_start_idx = exception->handler_start_idx;
        if (exception->catch_type_index == 0)
            eblock->type = JD_EXCEPTION_FINALLY;
        else
            eblock->type = JD_EXCEPTION_CATCH;

        eblock->handler_end_offset = exception->handler_end;
        eblock->handler_end_idx = exception->handler_end_idx;

        ladd_obj(m->basic_blocks, bblock);
        block_id++;
    }
}

static void cfg_link_normal_block(jd_method *m)
{
    jd_bblock *enter_block = block_by_id(m, JD_BB_ENTER_ID);
    jd_ins *start_ins = get_ins(m, 0);
    jd_bblock *start_normal_block = start_ins->block;
    jd_ins *method_end_ins = lget_obj_last(m->instructions);
    create_link_edge(enter_block, start_normal_block);

    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_NORMAL)
            continue;
        jd_nblock *nblock = block->ub->nblock;
        jd_ins *end_ins = nblock->end_ins;
        jd_ins_fn *fn = end_ins->fn;
        if (fn->is_return(end_ins) ||
            (end_ins->idx == method_end_ins->idx &&
             !fn->is_unconditional_jump(end_ins))) {
            jd_bblock *exit_block = block_by_id(m, JD_BB_EXIT_ID);
            create_link_edge(block, exit_block);
            continue;
        }
        if (fn->is_athrow(end_ins)) {
            continue;
        }

        for (int j = 0; j < end_ins->targets->size; ++j) {
            jd_ins *target_ins = lget_obj(end_ins->targets, j);
            jd_bblock *target = block_by_offset(m, target_ins->offset);
            assert(target != NULL);
            create_link_edge(block, target);
        }
    }
}

static void cfg_link_exception_block(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_NORMAL)
            continue;

        jd_nblock *nblock = block->ub->nblock;
        jd_ins *start_ins = nblock->start_ins;
        jd_ins *end_ins = nblock->end_ins;

        jd_bblock *_eblock = NULL;
        for (int j = 0; j < m->basic_blocks->size; ++j) {
            jd_bblock *other_block = lget_obj(m->basic_blocks, j);
            if (other_block->type != JD_BB_EXCEPTION)
                continue;
            jd_eblock *other_eblock = other_block->ub->eblock;
            if (start_ins->offset < other_eblock->try_start_offset ||
                start_ins->offset > other_eblock->try_end_offset)
                continue;
            if (_eblock == NULL)
                _eblock = other_block;
            else {
                jd_eblock *_eb = _eblock->ub->eblock;
                if (other_eblock->try_start_offset > _eb->try_start_offset)
                    _eblock = other_block;
                if (other_eblock->try_start_offset == _eb->try_start_offset &&
                    other_eblock->try_end_offset < _eb->try_end_offset)
                    _eblock = other_block;
            }
        }

        if (_eblock != NULL) {
            for (int j = 0; j < m->basic_blocks->size; ++j) {
                jd_bblock *other = lget_obj(m->basic_blocks, j);
                if (other->type != JD_BB_EXCEPTION)
                    continue;
                jd_eblock *_eb = _eblock->ub->eblock;
                jd_eblock *_oeb = other->ub->eblock;
                if (_oeb->try_start_offset == _eb->try_start_offset &&
                    _oeb->try_end_offset == _eb->try_end_offset)
                    create_link_edge(block, other);
            }
            jd_bblock *closest_fb = block_closest_finally(m, _eblock);
            jd_bblock *closest_hb = block_closest_handler(m, block);
            if (closest_fb != NULL && closest_hb == NULL)
                create_link_edge(block, closest_fb);
        }
        else
        {
            jd_bblock *closest_hb = block_closest_handler(m, block);
            jd_bblock *closest_fb = block_closest_finally(m, block);
            if (closest_fb != NULL && closest_hb != NULL) {
                jd_eblock *_feb = closest_fb->ub->eblock;
                jd_eblock *_heb = closest_hb->ub->eblock;
                if (_feb->try_start_offset == _heb->try_start_offset)
                    create_link_edge(block, closest_fb);
            }
        }

        jd_ins_fn *fn = end_ins->fn;
        if (fn->is_athrow(end_ins) && _eblock == NULL) {
            jd_bblock *exception_exit_block = block_exception_exit(m);
            create_link_edge(block, exception_exit_block);
        }

    }

    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_EXCEPTION)
            continue;
        jd_eblock *eblock = block->ub->eblock;

        jd_bblock *parent_block = block_parent_exception(m, block);
        if (parent_block != NULL &&
            parent_block->block_id != block->block_id)
            create_link_edge(block, parent_block);

        jd_bblock *_finally_block = block_closest_finally(m, block);
        if (_finally_block != NULL &&
            _finally_block->block_id != block->block_id)
            create_link_edge(block, _finally_block);

        uint32_t offset = eblock->handler_start_offset;
        jd_bblock *handler_basic_block = block_by_offset(m, offset);

        if (handler_basic_block != NULL)
            create_link_edge(block, handler_basic_block);
    }
}

void cfg_create(jd_method *m)
{
    if (m->basic_blocks == NULL)
        m->basic_blocks = linit_object();
    else
        lclear_object(m->basic_blocks);

    cfg_create_normal_blocks(m);

    cfg_create_exception_blocks(m);

    cfg_link_normal_block(m);

    cfg_link_exception_block(m);

    // print_basic_block(m);
    create_dominator_tree(m);
}

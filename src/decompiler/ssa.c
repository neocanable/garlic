#include "decompiler/ssa.h"
#include "common/endian.h"
#include "common/str_tools.h"
#include "jvm/jvm_ins.h"
#include "control_flow.h"
#include "expression_writter.h"

void sform_local_variable_data_initialize(jd_method *m)
{
    int block_size = m->basic_blocks->size;
    int max_locals = m->max_locals;
    m->local_phi_list = linit_object();
    m->uses = linit_object_with_capacity(block_size);
    m->defs = linit_object_with_capacity(block_size);
    m->live_ins = linit_object_with_capacity(block_size);
    m->live_outs = linit_object_with_capacity(block_size);
    m->reaching_in = linit_object_with_capacity(block_size);
    m->reaching_out = linit_object_with_capacity(block_size);
    m->live_intervals = linit_object_with_capacity(max_locals);
    m->assigns = linit_object_with_capacity(max_locals);

    for (int i = 0; i < block_size; ++i) {
        bitset_t *bitset_use = NULL;
        bitset_t *bitset_def = NULL;
        bitset_t *bitset_live_in = NULL;
        bitset_t *bitset_live_out = NULL;
        bitset_t *bitset_reaching_in = NULL;
        bitset_t *bitset_reaching_out = NULL;
        if (max_locals > 0) {
            bitset_use = bitset_create_with_capacity(max_locals);
            bitset_def = bitset_create_with_capacity(max_locals);
            bitset_live_in = bitset_create_with_capacity(max_locals);
            bitset_live_out = bitset_create_with_capacity(max_locals);
            bitset_reaching_in = bitset_create_with_capacity(max_locals);
            bitset_reaching_out = bitset_create_with_capacity(max_locals);
        }
        else {
            bitset_use = bitset_create();
            bitset_def = bitset_create();
            bitset_live_in = bitset_create();
            bitset_live_out = bitset_create();
            bitset_reaching_in = bitset_create();
            bitset_reaching_out = bitset_create();
        }

        ladd_obj(m->uses, bitset_use);
        ladd_obj(m->defs, bitset_def);
        ladd_obj(m->live_ins, bitset_live_in);
        ladd_obj(m->live_outs, bitset_live_out);
        ladd_obj(m->reaching_in, bitset_reaching_in);
        ladd_obj(m->reaching_out, bitset_reaching_out);
    }

    for (int i = 0; i < max_locals; ++i) {
        list_object *list = linit_object();
        bitset_t *assign = bitset_create();
        ladd_obj(m->live_intervals, list);
        ladd_obj(m->assigns, assign);
    }
}

void prepare_local_variable_kill_and_gen(jd_method *m)
{
    // fill basic blocks info
    int block_size = m->basic_blocks->size;
    for (int i = 0; i < block_size; ++i) {
        jd_bblock *bblock = lget_obj(m->basic_blocks, i);
        if (bblock->type != JD_BB_NORMAL)
            continue;

        jd_nblock *nblock = bblock->ub->nblock;
        bitset_t *use = lget_obj(m->uses, i);
        bitset_t *def = lget_obj(m->defs, i);
        for (int j = nblock->start_idx; j <= nblock->end_idx; ++j) {
            jd_ins *ins = get_ins(m, j);

            if (ins->defs != NULL) {
                for (size_t k = 0; bitset_next_set_bit(ins->defs, &k); k++) {
                    bitset_t *assign = lget_obj(m->assigns, k);
                    bitset_set(assign, bblock->block_id);
                    bitset_set(def, k);
                }
            }

            if (ins->uses != NULL) {
                for (size_t k = 0; bitset_next_set_bit(ins->uses, &k); k++) {
                    if (!bitset_get(def, k))
                        bitset_set(use, k);
                }
            }
        }
    }
}

void process_local_variable_liveness(jd_method *m)
{
    int block_size = m->basic_blocks->size;
    int max_locals = m->max_locals;
    bool changed = true;
    int k = 0;
    do {
        changed = false;
        for (int i = block_size-1; i >= 0; --i) {
            jd_bblock *bblock = lget_obj(m->basic_blocks, i);
            if (bblock->type != JD_BB_NORMAL)
                continue;
            bitset_t *prev_live_in = lget_object(m->live_ins, i);
            bitset_t *prev_live_out = lget_object(m->live_outs, i);
            bitset_t *new_live_in = bitset_create_with_capacity(max_locals);
            bitset_t *new_live_out = bitset_create_with_capacity(max_locals);

            for (int j = 0; j < bblock->out->size; ++j) {
                jd_edge *edge = lget_obj(bblock->out, j);
                bitset_t *in = lget_obj(m->live_ins, edge->target_block_id);
                bitset_inplace_union(new_live_out, in);
            }

            bitset_t *copy_live_out = bitset_copy(new_live_out);
            bitset_inplace_difference(copy_live_out, lget_obj(m->defs, i));
            bitset_inplace_union(new_live_in, lget_obj(m->uses, i));
            bitset_inplace_union(new_live_in, copy_live_out);

            if (bitset_symmetric_difference_count(prev_live_in, new_live_in) > 0) {
                memcpy(lget_obj(m->live_ins, i), new_live_in, sizeof(bitset_t));
                changed = true;
            }

            if (bitset_symmetric_difference_count(prev_live_out, new_live_out) > 0) {
                memcpy(lget_obj(m->live_outs, i), new_live_out, sizeof(bitset_t));
                changed = true;
            }

            k++;
            if (k > block_size*10)
                DEBUG_SSA_PRINT("error\n");
        }

    } while (changed);
}

void process_local_variable_live_intervals(jd_method *m)
{
    for (int i = m->basic_blocks->size - 1; i >= 0; --i) {
        jd_bblock *bblock = lget_obj(m->basic_blocks, i);
        if (bblock->type != JD_BB_NORMAL)
            continue;

        jd_nblock *nblock = bblock->ub->nblock;
        bitset_t *live_out = lget_obj(m->live_outs, i);
        for (size_t k = 0; bitset_next_set_bit(live_out, &k); k++) {
            // k is local variable index

            list_object *live_interval = lget_obj(m->live_intervals, k);
            jd_range *range = NULL;
            jd_ins *last_ins = get_ins(m, nblock->end_idx);
            if (live_interval->size == 0) {
                range = make_obj(jd_range);
                range->start_offset = nblock->start_offset;
                range->end_offset = nblock->end_offset;
                ladd_obj(live_interval, range);
            }
            else {
                range = lget_obj_last(live_interval);
                jd_ins *range_start_ins = ins_of_offset(m, range->start_offset);
                if (range_start_ins->prev != NULL && 
                        range_start_ins->prev->offset == last_ins->offset) {
                    range->start_offset = nblock->start_offset;
                }
                else {
                    range = make_obj(jd_range);
                    range->start_offset = nblock->start_offset;
                    range->end_offset = nblock->end_offset;
                    ladd_obj(live_interval, range);
                }
            }

            jd_ins *iter = get_ins(m, nblock->end_idx);
            while (iter != NULL && iter->offset >= nblock->start_offset) {
                if (iter->defs != NULL) {
                    if (bitset_get(iter->defs, k)) {
                        range->start_offset = iter->offset;
                        break;
                    }
                }

                iter = iter->prev;
            }
        }
    }
}

void process_local_variable_reaching_defination(jd_method *m)
{
    int block_size = m->basic_blocks->size;
    int max_locals = m->max_locals;
    bool changed = true;
    do {
        changed = false;
        for (int i = 0; i < block_size; ++i) {
            jd_bblock *bblock = lget_obj(m->basic_blocks, i);
            if (bblock->type != JD_BB_NORMAL)
                continue;
            bitset_t *prev_reaching_out = lget_obj(m->reaching_out, i);
            bitset_t *prev_reaching_in = lget_obj(m->reaching_in, i);
            bitset_t *new_reaching_in = bitset_create_with_capacity(max_locals);
            bitset_t *new_reaching_out = NULL;
            for (int j = 0; j < bblock->in->size; ++j) {
                jd_edge *edge = lget_obj(bblock->in, j);
                bitset_t *in_reaching_out = lget_obj(m->reaching_out, edge->source_block_id);
                bitset_inplace_union(new_reaching_in, in_reaching_out);
            }

            bitset_t *def = lget_obj(m->defs, i);
            new_reaching_out = bitset_copy(new_reaching_in);
            for (size_t k = 0; bitset_next_set_bit(def, &k); k++) {
                bitset_set_to_value(new_reaching_out, k, false);
            }
            bitset_inplace_union(new_reaching_out, def);
            if (bitset_symmetric_difference_count(prev_reaching_out, new_reaching_out) > 0) {
                memcpy(lget_obj(m->reaching_out, i), new_reaching_out, sizeof(bitset_t));
                changed = true;
            }
            if (bitset_symmetric_difference_count(prev_reaching_in, new_reaching_in) > 0) {
                memcpy(lget_obj(m->reaching_in, i), new_reaching_in, sizeof(bitset_t));
                changed = true;
            }
        }

    } while (changed);

}

void print_local_variable_liveness(jd_method *m)
{
    DEBUG_SSA_PRINT("\n------- m: %s\n", m->name);
    int block_size = m->basic_blocks->size;
    for (int i = 0; i < block_size; ++i) {
        jd_bblock *bblock = lget_obj(m->basic_blocks, i);
        if (bblock->type != JD_BB_NORMAL)
            continue;
        bitset_t *reaching_in = lget_obj(m->reaching_in, i);
        bitset_t *reaching_out = lget_obj(m->reaching_out, i);
        DEBUG_SSA_PRINT("block %4d: %4d -> %4d ", 
                i, 
                bblock->ub->nblock->start_offset, 
                bblock->ub->nblock->end_offset);
        DEBUG_SSA_PRINT(" | ");
        DEBUG_SSA_PRINT("defs: %20s", 
                bitset_string(lget_obj(m->defs, i)));
        DEBUG_SSA_PRINT(" | ");
        DEBUG_SSA_PRINT("uses: %20s", 
                bitset_string(lget_obj(m->uses, i)));
        DEBUG_SSA_PRINT(" | ");
        DEBUG_SSA_PRINT("live_in: %40s", 
                bitset_string(lget_obj(m->live_ins, i)));
        DEBUG_SSA_PRINT(" | ");
        DEBUG_SSA_PRINT("live_out: %40s", 
                bitset_string(lget_obj(m->live_outs, i)));
        DEBUG_SSA_PRINT("\n");
    }
}

void print_local_variables_live_intervals(jd_method *m)
{
    DEBUG_SSA_PRINT("\n------- m: %s\n", m->name);
    int max_locals = m->max_locals;
    for (int i = 0; i < max_locals; ++i) {
        list_object *live_interval = lget_obj(m->live_intervals, i);
        DEBUG_SSA_PRINT("local %d: ", i);
        for (int j = live_interval->size-1; j >= 0; --j) {
            jd_range *range = lget_obj(live_interval, j);
            DEBUG_SSA_PRINT("[%d, %d] ", range->start_offset, range->end_offset);
        }
        DEBUG_SSA_PRINT("\n");
    }
}

jd_local_phi_node* sform_local_variable_phi_node(jd_method *m, jd_bblock *b, int slot)
{
    for (int i = 0; i < m->local_phi_list->size; ++i) {
        jd_local_phi_node *p = lget_obj(m->local_phi_list, i);
        if (p->slot == slot && p->block_id == b->block_id) {
            return p;
        }
    }
    return NULL;
}

static jd_local_phi_node* sform_local_variable_prepare_phi_node(jd_method *m, jd_bblock *b, int slot)
{
    jd_local_phi_node *phi_node = NULL;
    for (int i = 0; i < m->local_phi_list->size; ++i) {
        jd_local_phi_node *p = lget_obj(m->local_phi_list, i);
        if (p->slot == slot && p->block_id == b->block_id) {
            phi_node = p;
            break;
        }
    }
    if (phi_node == NULL) {
        phi_node = make_obj(jd_local_phi_node);
        phi_node->slot = slot;
        phi_node->block_id = b->block_id;
        phi_node->params = linit_object();
        phi_node->ins_offset = b->ub->nblock->start_offset;
        ladd_obj(m->local_phi_list, phi_node);
    }
    return phi_node;
}

void sform_local_variable_insert_phi(jd_method *m)
{
    size_t block_size = m->basic_blocks->size;
    int max_locals = m->max_locals;
    bitset_t *processed = bitset_create_with_capacity(block_size);
    queue_object *work_list = queue_init_object();
    for (int i = 0; i < max_locals; ++i) {
        bitset_t *assign = lget_obj(m->assigns, i);
        for (size_t k = 0; bitset_next_set_bit(assign, &k); k++) {
            jd_bblock *b = lget_obj(m->basic_blocks, k);
            queue_add_object(work_list, b);
            bitset_set(processed, k);
        }
        while (!queue_empty(work_list)) {
            jd_bblock *block = queue_pop_object(work_list);
            for (int l = 0; l < block->frontier->size; ++l) {
                jd_bblock *frontier = lget_obj(block->frontier, l);
                if (frontier->type != JD_BB_NORMAL)
                    continue;

//                if (frontier == block)
//                    continue;
                // TODO: if block live_in's slot i is dead, continue
                 bitset_t *live_in = lget_object(m->live_ins, frontier->block_id);
                 if (!bitset_get(live_in, i))
                     continue;

                 bitset_t *live_out = lget_object(m->live_outs, block->block_id);
                 if (!bitset_get(live_out, i))
                    continue;

                jd_local_phi_node *phi_node = sform_local_variable_prepare_phi_node(m, frontier, i);
                jd_ssa_param *v = make_obj(jd_ssa_param);
                v->block_id = frontier->block_id;
                v->def_block_id = block->block_id;
                v->slot = i;
                v->version = 0;
                ladd_obj(phi_node->params, v);
                DEBUG_SSA_PRINT("[insert phi]: trie_insert block: %zu type: %d slot: %d def_block: %zu\n",
                       frontier->block_id, frontier->type, i, block->block_id);

                if (!bitset_get(processed, frontier->block_id)) {
                    bitset_set(processed, frontier->block_id);
                    queue_add_object(work_list, frontier);
                }
            }
        }
    }
}

static int sform_local_variable_new_name(int counter[], queue_object *stack, int slot)
{
    jd_ssa_state *state = make_obj(jd_ssa_state);
    state->version = counter[slot];
    counter[slot] += 1;
    queue_push_object(stack, state);
    return state->version;
}

static void sform_local_variable_renames(jd_method *m,
                                         jd_bblock *block,
                                         int counter[],
                                         list_object *stacks)
{
    m->ssa_vars = linit_object();
    jd_nblock *nblock = block->ub->nblock;

    for (int i = 0; i < m->local_phi_list->size; ++i) {
        jd_local_phi_node *phi_node = lget_obj(m->local_phi_list, i);
        if (phi_node->block_id != block->block_id)
            continue;
        queue_object *stack = lget_obj(stacks, phi_node->slot);
        phi_node->version = sform_local_variable_new_name(counter, stack, phi_node->slot);
    }

    for (int i = nblock->start_idx; i <= nblock->end_idx; ++i) {
        jd_ins *ins = get_ins(m, i);

        if (ins->defs != NULL) {
            for (size_t k = 0; bitset_next_set_bit(ins->defs, &k); k++) {
                jd_ssa_var *ssa_var = make_obj(jd_ssa_var);
                ssa_var->ins = ins;
                ssa_var->slot = k;
                queue_object *stack = lget_obj(stacks, k);
                ssa_var->version = sform_local_variable_new_name(counter, stack, k);
                DEBUG_SSA_PRINT("[def_ssa_var def]: %zu %d: %s, ver: %d slot: %d\n",
                                block->block_id,
                                ins->offset,
                                ins->name,
                                ssa_var->version, ssa_var->slot);
                ladd_obj(m->ssa_vars, ssa_var);
            }
        }

        if (ins->uses != NULL) {
            for (size_t k = 0; bitset_next_set_bit(ins->uses, &k) ; k++) {
                jd_ssa_var *ssa_var = make_obj(jd_ssa_var);
                ssa_var->slot = k;
                ssa_var->ins = ins;
                queue_object *s = lget_obj(stacks, k);
                jd_ssa_state *_state = queue_peek_object(s);
                ssa_var->version = _state->version;
                DEBUG_SSA_PRINT("[def_ssa_var use]: %zu %d: %s, ver: %d slot: %d\n",
                                block->block_id,
                                ins->offset,
                                ins->name,
                                ssa_var->version, ssa_var->slot);
                ladd_obj(m->ssa_vars, ssa_var);
            }
        }
    }

    for (int i = 0; i < block->out->size; ++i) {
        jd_edge *edge = lget_obj(block->out, i);
        jd_bblock *target = edge->target_block;
        if (target->type != JD_BB_NORMAL)
            continue;
        for (int j = 0; j < m->local_phi_list->size; ++j) {
            jd_local_phi_node *phi_node = lget_obj(m->local_phi_list, j);
            if (phi_node->block_id != target->block_id)
                continue;
            for (int k = 0; k < phi_node->params->size; ++k) {
                jd_ssa_param *param = lget_obj(phi_node->params, k);
                if (param->def_block_id != block->block_id)
                    continue;
                jd_stack *_stack = lget_object(stacks, param->slot);
                jd_ssa_state *_state = queue_peek_object(_stack);
                param->version = _state->version;
            }
        }
    }

    for (int i = 0; i < block->dom_children->size; ++i) {
        jd_bblock *b = lget_obj(block->dom_children, i);
        if (b->type != JD_BB_NORMAL)
            continue;
        sform_local_variable_renames(m, b, counter, stacks);
    }

    for (int i = 0; i < m->local_phi_list->size; ++i) {
        jd_local_phi_node *phi_node = lget_obj(m->local_phi_list, i);
        if (phi_node->block_id != block->block_id)
            continue;
        queue_object *stack = lget_obj(stacks, phi_node->slot);
        queue_pop_object(stack);
    }

    for (int i = nblock->start_idx; i <= nblock->end_idx; ++i) {
        jd_ins *ins = get_ins(m, i);

        if (ins->defs != NULL) {
            for (size_t k = 0; bitset_next_set_bit(ins->defs, &k); k++) {
                jd_local_phi_node *phi_node = sform_local_variable_phi_node(m, block, k);
                if (phi_node != NULL) {
                    queue_object *stack = lget_obj(stacks, k);
                    queue_pop_object(stack);
                }
            }
        }
    }
}

static void sform_init_enter_state(jd_method *m,
                                   int counter[],
                                   list_object *stacks)
{
    jd_bblock *enter = lget_obj(m->basic_blocks, 3);
    for (int slot = 0; slot < m->max_locals; ++slot) {
        jd_ssa_state *state = make_obj(jd_ssa_state);
        state->version = 0;
        queue_object *stack = lget_object(stacks, slot);
        queue_push_object(stack, state);
    }
}

void sform_local_variable_rename(jd_method *m)
{
    int max_locals = m->max_locals;

    int counter[max_locals];
    list_object *stacks = linit_object_with_capacity(max_locals);
    for (int i = 0; i < max_locals; ++i) {
        queue_object *stack = queue_init_object();
        ladd_obj(stacks, stack);
        counter[i] = 0;
    }
    sform_init_enter_state(m, counter, stacks);
    jd_bblock *enter = lget_obj(m->basic_blocks, 3);
    sform_local_variable_renames(m, enter, counter, stacks);
}

void sform_local_variable_phi_node_dfs_copies(jd_method *m, jd_bblock *b)
{
    if (b == NULL)
        b = lget_obj(m->basic_blocks, 3);

    if (b->visited)
        return;
    b->visited = true;

    for (int i = 0; i < b->out->size; ++i) {
        jd_edge *edge = lget_obj(b->out, i);
        jd_bblock *target = edge->target_block;
        if (target->type != JD_BB_NORMAL)
            continue;
        sform_local_variable_phi_node_dfs_copies(m, target);
    }

    for (int i = 0; i < m->local_phi_list->size; ++i) {
        jd_local_phi_node *phi = lget_obj(m->local_phi_list, i);

        // 这里的phi节点对于循环导出变量十分有用
        // if (phi->params->size < 2)
        //    continue;
        if (phi->block_id != b->block_id)
            continue;
        DEBUG_SSA_PRINT("[phi node]: slot: %d -> version: %d \n", phi->slot, phi->version);
        jd_bblock *block = lget_obj(m->basic_blocks, phi->block_id);
        jd_nblock *nblock = block->ub->nblock;
        jd_ins *start_ins = get_ins(m, nblock->start_idx);

        if (start_ins->stack_in == NULL) {
            DEBUG_SSA_PRINT("[SSA COPY]can not find stack_in: %zu %d\n", block->block_id, phi->slot);
            continue;
        }

        jd_val *val = start_ins->stack_in->local_vars[phi->slot];
        if (val == NULL) {
            DEBUG_SSA_PRINT("[SSA COPY]can not find val: %zu %d\n", block->block_id, phi->slot);
            continue;
        }

        bool result = false;
        for (int j = 0; j < phi->params->size; ++j) {
            jd_ssa_param *p = lget_obj(phi->params, j);
            if (p->version == 0)
                continue;
            jd_bblock *def_block = lget_obj(m->basic_blocks, p->def_block_id);
            jd_nblock *nb = def_block->ub->nblock;
            jd_ins *out_end_ins = get_ins(m, nb->end_idx);
            if (ins_is_unreached(out_end_ins))
                continue;

            jd_val *other = out_end_ins->stack_out->local_vars[phi->slot];

            // TODO: check why other is null
            if (other != NULL && other->stack_var != val->stack_var) {
                DEBUG_SSA_PRINT("[need def merge phi node] %d -> %d "
                                "slot: %d %s %s "
                                "ins: %d\n",
                                nb->start_offset, nb->end_offset,
                                phi->slot,
                                val->name,
                                other->name,
                                out_end_ins->offset);
                other->name = val->name;
                other->stack_var = val->stack_var;
                result = true;
            }
        }
    }
}

static jd_stack_phi_node* sform_stack_variable_prepare_phi_node(jd_method *m, jd_bblock *b, int slot)
{
    jd_stack_phi_node *phi_node = NULL;
    for (int i = 0; i < m->stack_phi_list->size; ++i) {
        jd_stack_phi_node *p = lget_obj(m->stack_phi_list, i);
        if (p->slot == slot && p->block_id == b->block_id) {
            phi_node = p;
            break;
        }
    }
    if (phi_node == NULL) {
        phi_node = make_obj(jd_stack_phi_node);
        phi_node->slot = slot;
        phi_node->block_id = b->block_id;
        phi_node->params = linit_object();
        phi_node->ins_offset = b->ub->nblock->start_offset;
        ladd_obj(m->stack_phi_list, phi_node);
    }
    return phi_node;
}

static void sform_stack_variable_insert_phi(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_NORMAL)
            continue;
        jd_nblock *nblock = block->ub->nblock;
        jd_ins *end_ins = nblock->end_ins;
        jd_stack *stack_out = end_ins->stack_out;
        if (stack_out == NULL || stack_out->depth == 0)
            continue;
        // 这个basic block需要在其的df前插入一个phi节点
        // phi节点的数量根据stack_out->depth的数量决定
        for (int j = 0; j < stack_out->depth; ++j) {
            jd_val *val = stack_out->vals[j];

            if (block->frontier->size == 0)
                continue;

            for (int k = 0; k < block->out->size; ++k) {
                jd_edge *edge = lget_obj(block->out, k);
                jd_bblock *df = edge->target_block;
                if (df->type != JD_BB_NORMAL)
                    continue;
                jd_ins *start_ins = df->ub->nblock->start_ins;
                jd_stack *stack_in = start_ins->stack_in;
                if (stack_in->depth != stack_out->depth)
                    continue;

                jd_stack_phi_node *phi_node = sform_stack_variable_prepare_phi_node(m, df, j);

                jd_ssa_param *param = make_obj(jd_ssa_param);
                param->slot = j;
                param->def_block_id = block->block_id;
                param->block_id = df->block_id;
                param->stack_var = val->stack_var;
                ladd_obj(phi_node->params, param);
            }
        }
    }
}

static void sform_stack_variable_insert_phi_v2(jd_method *m)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *block = lget_obj(m->basic_blocks, i);
        if (block->type != JD_BB_NORMAL)
            continue;

        jd_ins *start_ins = block->ub->nblock->start_ins;
        jd_stack *stack_in = start_ins->stack_in;
        if (stack_in == NULL || stack_in->depth == 0)
            continue;

        if (block->in->size < 1)
            continue;

        for (int j = 0; j < stack_in->depth; ++j) {
            jd_val *val = stack_in->vals[j];
            jd_var *stack_var = val->stack_var;

//            jd_stack_phi_node *phi_node = sform_stack_variable_prepare_phi_node(m, block, j);
            for (int k = 0; k < block->in->size; ++k) {
                jd_edge *edge = lget_obj(block->in, k);
                jd_bblock *df = edge->source_block;
                if (df->type != JD_BB_NORMAL)
                    continue;
                jd_ins *end_ins = df->ub->nblock->end_ins;
                jd_stack *stack_out = end_ins->stack_out;
                if (stack_out->depth != stack_in->depth)
                    continue;

                jd_val *out_val = stack_out->vals[j];
                if (out_val->stack_var == stack_var)
                    continue;

                DEBUG_PRINT("[replace stack var]: %d %s %s -> %s\n",
                       end_ins->offset,
                       end_ins->name,
                       out_val->stack_var->name,
                       stack_var->name);
                out_val->name = val->name;
//                out_val->name = val->name;
                if (out_val->name == NULL) {
//                    out_val->name = stack_var->name;
                    out_val->name = str_create("v_%d", stack_var->idx);
                }
                out_val->stack_var = stack_var;

            }
        }
    }
}

static void sform_stack_variable_phi_node_copies(jd_method *m)
{
    m->stack_phi_node_copies = hashmap_init((hcmp_fn) i2obj_cmp, 0);
    for (int i = 0; i < m->stack_phi_list->size; ++i) {
        jd_stack_phi_node *phi_node = lget_obj(m->stack_phi_list, i);
        jd_bblock *block = lget_obj(m->basic_blocks, phi_node->block_id);
        jd_ins *start_ins = block->ub->nblock->start_ins;
        jd_stack *stack_in = start_ins->stack_in;
        if (stack_in == NULL || stack_in->depth == 0)
            continue;
        jd_val *val = stack_in->vals[phi_node->slot];
        jd_var *stack_var = val->stack_var;
        for (int j = 0; j < phi_node->params->size; ++j) {
            jd_ssa_param *param = lget_obj(phi_node->params, j);
            if (param->stack_var == stack_var)
                continue;

            jd_var *var = param->stack_var;
            jd_ins *ins = var->ins;
            list_object *list = hget_i2o(m->stack_phi_node_copies, ins->offset);
            if (list == NULL) {
                list = linit_object();
                hset_i2o(m->stack_phi_node_copies, ins->offset, list);
            }
            jd_var_copy *copy = make_obj(jd_var_copy);
            copy->ins = ins;
            copy->left = stack_var;
            copy->right = var;
            ladd_obj(list, copy);

            DEBUG_SSA_PRINT("[trie_insert expression]: %d %s %s = %s\n",
                   ins->offset,
                   ins->name,
                   stack_var->name,
                   var->name);
        }
    }
}

void sform_prepare_for_local_variable(jd_method *m)
{
    sform_local_variable_data_initialize(m);
    prepare_local_variable_kill_and_gen(m);
    process_local_variable_liveness(m);
}

void sform_for_local_variable(jd_method *m)
{
    sform_local_variable_insert_phi(m);
    sform_local_variable_rename(m);

    basic_block_clear_visited_flag(m);
    sform_local_variable_phi_node_dfs_copies(m, NULL);
}

void sform_for_stack_variable(jd_method *m)
{    
    sform_stack_variable_insert_phi_v2(m);
}

void print_sform_and_liveness(jd_method *m)
{
    printf("m: %s\n", m->name);
    print_all_expression_with_ssa(m);
    printf("\n");
}

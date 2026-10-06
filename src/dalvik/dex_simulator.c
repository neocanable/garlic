#include "decompiler/klass.h"
#include "dalvik/dex_simulator.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_ins_action.h"
#include "dalvik/dex_type_analyse.h"

#include "decompiler/stack.h"
#include "decompiler/control_flow.h"
#include "decompiler/ssa.h"
#include "decompiler/method.h"

void dex_variable_name(jd_method *m, jd_dex_ins *ins, jd_val *val, int slot)
{
    if (match_dex_debug(m, ins, val, slot))
        return;

    stack_val_name(m, ins, val, slot);
}

static jd_stack* dex_exception_stack(jd_method *m,
                                     jd_dex_ins *ins,
                                     jd_stack *src)
{
    jd_bblock *block = block_handler_equals_ins(m, ins);
    jd_eblock *eblock = block->ub->eblock;
    string class_desc = NULL;

    int catch_type_index = eblock->exception->catch_type_index;
    if (catch_type_index == 0) {
        class_desc = str_dup("Ljava/lang/Throwable");
    }
    else {
        jd_dex *dex = m->meta;
        jd_meta_dex *meta = dex->meta;
        dex_type_id *type_id = &meta->type_ids[catch_type_index];
        class_desc = meta->strings[type_id->descriptor_idx].data;
    }

    jd_stack *clone = stack_clone(src);
    jd_val *eval = stack_create_empty_val();
    eval->type = JD_VAR_REFERENCE_T;
    string full = class_full_name(class_desc);
    eval->data->cname = class_simple_name(full);
    eval->ins = ins;

    if (dex_ins_is_move_exception(ins)) {
        int reg_num = move_exception_reg_num(ins);
        eval->slot = reg_num;
        clone->local_vars[reg_num] = eval;
        dex_variable_name(m, ins, eval, reg_num);
    }
    return clone;
}

static void dex_method_enter_stack(jd_method *m)
{
    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex->meta;

    stack_create_method_enter(m);
    jd_stack *stack = m->enter;
    bool instance = method_is_member(m);

    int param_itor = m->max_locals - 1;
    for (int i = m->desc->list->size - 1; i >= 0 ; --i) {
        string item = lget_string(m->desc->list, i);
        jd_val *val = stack_create_val_with_descriptor(m, item,
                                                       param_itor);
        dex_variable_name(m, NULL, val, val->slot);
        stack->local_vars[param_itor] = val;
        m->parameters[i] = val;
        param_itor --;

        if (val->type == JD_VAR_LONG_T ||
            val->type == JD_VAR_DOUBLE_T) {
            stack->local_vars[param_itor] = val;
            param_itor--;
        }
    }

    match_dex_parameter(m, stack);

    if (instance) {
        string desc = dex_method_class_descriptor(meta, m->meta_method);
        string full = class_full_name(desc);
        string cname = class_simple_name_without_primitive(full);
        stack_create_method_this_val(m, param_itor, cname);
    }

    stack_define_var_for_enter_stack(m);
}

static void perform_dex_stack_var_defination(jd_dex_ins *ins)
{
    jd_method *m = ins->method;
    jd_stack *stack_out = ins->stack_out;

    if (dex_ins_is_filled_new_array(ins) ||
        dex_ins_is_filled_new_array_range(ins)) {
        jd_val *val = stack_create_empty_val();
        val->type = JD_VAR_REFERENCE_T;
        val->data->cname = (string)g_str_Object;
        val->ins = ins;
        val->slot = 0;
        dex_variable_name(m, ins, val, 0);
        jd_var *stack_var = stack_define_var(m, val, 0);
        hset_i2o(m->offset2var_map, ins->offset, stack_var);
        return;
    }

    for (size_t slot = 0; bitset_next_set_bit(ins->defs, &slot); slot++) {
        jd_val *val = stack_out->local_vars[slot];
        jd_var *stack_var = stack_define_var(m, val, slot);
        hset_i2o(m->offset2var_map, ins->offset, stack_var);
    }

}

static void dex_run_instruction_action(jd_dex_ins *ins)
{
    dex_ins_action(ins);
    perform_dex_stack_var_defination(ins);
}

static bool block_can_execute(jd_method *m, jd_dex_ins *ins, jd_dex_ins *start)
{
    jd_bblock *block = start->block;
    bitset_t *live_ins = lget_object(m->live_ins, block->block_id);
    for (size_t i = 0; bitset_next_set_bit(live_ins, &i) ; i++) {
        if (ins->stack_out->local_vars[i] == NULL)
            return false;
    }
    return true;
}

static void merge_local_variables_for_exception_block(jd_dex_ins *ins,
                                                     jd_dex_ins *suc_ins)
{
    for (int j = 0; j < ins->stack_out->local_vars_count; ++j) {
        jd_val *local_var = ins->stack_out->local_vars[j];
        if (local_var == NULL)
            continue;
        jd_val *suc_var = suc_ins->stack_in->local_vars[j];
        if (suc_var == NULL) {
            suc_ins->stack_in->local_vars[j] = local_var;
        }
    }
}

static void dex_fill_watch_successors(jd_method *m, jd_dex_ins *ins)
{
    jd_bblock *block = ins->block;
    assert(block != NULL);
    jd_dex_ins *end_ins = block->ub->nblock->end_ins;

    lclear_object(m->ins_watch_successors);

    if (ins->offset < end_ins->offset)
        ladd_obj(m->ins_watch_successors, ins->next);
    else {
        // find next block
        for (int i = 0; i < block->out->size; ++i) {
            jd_edge *edge = lget_obj(block->out, i);
            jd_bblock *target_block = edge->target_block;
            if (target_block->type == JD_BB_NORMAL) {
                jd_nblock *tnb = target_block->ub->nblock;
                jd_dex_ins *block_start_ins = tnb->start_ins;
                ladd_obj(m->ins_watch_successors, block_start_ins);
            }
        }
        for (int i = 0; i < block->out->size; ++i) {
            jd_edge *edge = lget_obj(block->out, i);
            jd_bblock *target_block = edge->target_block;
            if (target_block->type == JD_BB_EXCEPTION /* &&
                ins_is_try_end(m, block_end_ins)*/) {
                jd_eblock *eblock = target_block->ub->eblock;
                uint32_t hstart_off = eblock->handler_start_offset;
                jd_bblock *handler_block = block_start_offset(m, hstart_off);
                ladd_obj(m->ins_watch_successors,
                         handler_block->ub->nblock->start_ins);
            }
        }
    }
}

static jd_mix_exception* exception_of_handler(jd_method *m, jd_ins *handler)
{
    for (int i = 0; i < m->mix_exceptions->size; ++i) {
        jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
        if (!is_list_empty(e->catches)) {
            for (int j = 0; j < e->catches->size; ++j) {
                jd_range *range = lget_obj(e->catches, j);
                if (range->start_offset == handler->offset)
                    return e;
            }
        }
        if (e->finally != NULL && e->finally->start_offset == handler->offset)
            return e;
    }
    return NULL;
}

// Give a handler start the registers it runs with, so that its body is 
// simulated like any other code and its expressions are built.
static bool seed_exception_handler(jd_method *m, jd_dex_ins *handler)
{
    jd_mix_exception *e = exception_of_handler(m, handler);
    if (e == NULL || e->try == NULL)
        return false;

    jd_stack *base = NULL;
    for (int i = e->try->start_idx;
         base == NULL && i <= e->try->end_idx; ++i) {
        jd_ins *ins = get_ins(m, i);
        if (ins != NULL)
            base = ins->stack_out;
    }
    if (base == NULL)
        return false;

    jd_stack *stack = dex_exception_stack(m, handler, base);

    for (size_t k = 0; k < stack->local_vars_count; ++k) {
        if (stack->local_vars[k] != NULL)
            continue;

        jd_val *empty = stack_create_empty_val();
        empty->type = JD_VAR_REFERENCE_T;
        empty->data->cname = (string) g_str_Object;
        empty->ins = NULL;
        empty->slot = k;
        empty->name = NULL;

        dex_variable_name(m, NULL, empty, k);
        if (empty->name == NULL)
            stack_val_name(m, NULL, empty, k);
        stack_define_var(m, empty, k);
        stack->local_vars[k] = empty;
    }

    handler->stack_in = stack;
    return true;
}

static void dex_fill_visit_queue(jd_method *m, jd_dex_ins *ins)
{
    for (int i = 0; i < m->ins_watch_successors->size; ++i) {
        jd_dex_ins *suc_ins = lget_obj(m->ins_watch_successors, i);
        int is_handler_start = ins_is_handler_start(m, suc_ins);
        if (is_handler_start && block_can_execute(m, ins, suc_ins)) {
            if (suc_ins->stack_in != NULL)
                merge_local_variables_for_exception_block(ins, suc_ins);
        }
        else if (suc_ins->stack_in == NULL && !is_handler_start) {
            suc_ins->stack_in = stack_clone(ins->stack_out);
            queue_push_object(m->ins_visit_queue, suc_ins);
        }
    }
}

static void dex_ins_cb(jd_method *m, jd_dex_ins *ins)
{
    dex_fill_watch_successors(m, ins);

    dex_run_instruction_action(ins);

    dex_fill_visit_queue(m, ins);
}


static void dex_process_instruction_action(jd_method *m, ins_action_cb cb)
{
    int times = 0;
    jd_dex_ins *ins = NULL;
    while ((ins = queue_pop_object(m->ins_visit_queue)) != NULL) {
        times ++;
        cb(m, ins);
    }
}

void dex_simulator(jd_method *m)
{
    if (method_is_empty(m))
        return;
    m->ins_watch_successors = linit_object();
    m->ins_visit_queue = queue_init_object();
    m->stack_variables = linit_object();
    m->offset2var_map = hashmap_init((hcmp_fn)i2obj_cmp, 0);
    m->slot_counter_map = hashmap_init((hcmp_fn) i2i_cmp, 0);
    m->class_counter_map = hashmap_init((hcmp_fn) s2i_cmp, 0);
    m->var_name_taken = hashmap_init((hcmp_fn) s2i_cmp, 0);
    m->var_name_map = hashmap_init((hcmp_fn) s2s_cmp, 0);
    m->types = linit_object();

    dex_method_enter_stack(m);

    sform_prepare_for_local_variable(m);

    jd_dex_ins *start = lget_obj_first(m->instructions);

    queue_push_object(m->ins_visit_queue, start);

    dex_process_instruction_action(m, dex_ins_cb);

    bool progress = true;
    while (progress) {
        progress = false;
        for (int i = 0; i < m->mix_exceptions->size; ++i) {
            jd_mix_exception *e = lget_obj(m->mix_exceptions, i);
            jd_ins *handlers[128];
            int handler_count = 0;

            if (!is_list_empty(e->catches)) {
                for (int j = 0; j < e->catches->size &&
                                handler_count < 128; ++j) {
                    jd_range *r = lget_obj(e->catches, j);
                    handlers[handler_count++] = get_ins(m, r->start_idx);
                }
            }
            if (e->finally != NULL && handler_count < 128)
                handlers[handler_count++] = get_ins(m, e->finally->start_idx);

            for (int j = 0; j < handler_count; ++j) {
                jd_dex_ins *handler = handlers[j];
                if (handler == NULL || handler->stack_in != NULL)
                    continue;
                if (!seed_exception_handler(m, handler))
                    continue;

                queue_push_object(m->ins_visit_queue, handler);
                dex_process_instruction_action(m, dex_ins_cb);
                progress = true;
            }
        }
    }

    mark_unreachable_instruction(m);

    sform_for_local_variable(m);
}

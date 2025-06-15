#include "common/debug.h"
#include "common/str_tools.h"
#include "parser/class/class_tools.h"

#include "jvm_ins.h"
#include "decompiler/stack.h"
#include "decompiler/method.h"
#include "jvm_simulator.h"
#include "decompiler/control_flow.h"
#include "jvm/jvm_type_analyse.h"
#include "decompiler/klass.h"
#include "decompiler/ssa.h"
#include "decompiler/descriptor.h"

static void jvm_variable_name(jd_method *m, jd_ins *ins, jd_val *val, int slot)
{
    if (jvm_has_debug(m, ins, slot)) {
        jvm_debug(m, ins, val, slot);
        return;
    }
    stack_val_name(m, ins, val, slot);
}

static void jvm_run_store_local_variable(jd_ins *ins)
{
    if (!jvm_ins_is_store(ins))
        return;

    jd_method *m = ins->method;
    int slot = jvm_ins_store_slot(ins);
    jd_var_types type = jvm_ins_store_type(ins);
    jd_stack *out = ins->stack_out;
    jd_val *pop0 = ins->stack_in->vals[0];
    jd_val *local_var = find_local_variable(out, slot);

    if (local_var == NULL) {
        pop0->slot = slot;
        jd_val *new_val = stack_create_empty_val();
        out->local_vars[slot] = new_val;
        stack_clone_val(new_val, pop0);
        new_val->name_type = JD_VAR_NAME_DEF;
        jvm_variable_name(m, ins, new_val, slot);
        new_val->slot = slot;
    }
    else {
        if (pop0->type == local_var->type) {
            bool mapped = jvm_has_debug(m, ins, slot);
            if (mapped) {
                jd_matched_debug *matched = matched_local_variable(m,
                                                                   ins,
                                                                   slot);
                string class_name = matched->sname;

                if (STR_EQL(class_name, local_var->data->cname) &&
                    STR_EQL(class_name, pop0->data->cname) &&
                    STR_EQL(matched->name, local_var->name)) {
                }
                else {
                    jd_val *other_new_local = stack_create_empty_val();
                    out->local_vars[slot] = other_new_local;
                    other_new_local->slot = slot;
                    other_new_local->type = type;
                    other_new_local->name = matched->name;
                    other_new_local->data->cname = matched->sname;
                }

            }
            else {
                if (STR_EQL(pop0->data->cname,
                            local_var->data->cname)) {
                }
                else {
                    jd_val *other_new_local = stack_create_empty_val();
                    out->local_vars[slot] = other_new_local;
                    stack_clone_val(other_new_local, pop0);
                    other_new_local->slot = slot;
                    other_new_local->type = type;
                    jvm_variable_name(m,
                                      ins,
                                      other_new_local,
                                      slot);
                }
            }
        }
        else {
            jd_val *other_new_local = stack_create_empty_val();
            out->local_vars[slot] = other_new_local;
            stack_clone_val(other_new_local, pop0);
            other_new_local->slot = slot;
            other_new_local->type = type;
            jvm_variable_name(m, ins, other_new_local, slot);
        }
    }

}

static void jvm_run_load_local_variable(jd_ins *ins)
{
    if (!jvm_ins_is_load(ins))
        return;
    int slot = jvm_ins_load_slot(ins);
    jd_stack *stack_out = ins->stack_out;
    jd_val *local_var = find_local_variable(stack_out, slot);

    if (local_var == NULL) {
        // TODO: 这里需要处理
        DEBUG_PRINT("[%s %d]load local var: %d not found\n",
               ins->name,
               ins->offset,
               slot);
        jd_val *push0 = stack_out->vals[0];
        push0->data = make_obj(jd_val_data);
        push0->data->cname = (string)g_str_Object;
        push0->type = JD_VAR_REFERENCE_T;
        push0->slot = slot;
        push0->ins  = ins;
        jvm_variable_name(ins->method, ins, push0, slot);
    }
    else {

        // TODO: 这里需要注意
        stack_out->vals[0] = local_var;
        if (local_var->ins == NULL)
            local_var->ins = ins;
    }
}

static void jvm_run_athrow(jd_ins *ins)
{
    if (!jvm_ins_is_athrow(ins))
        return;
    jd_stack *stack_out = ins->stack_out;
    jd_val *pop0 = ins->stack_in->vals[0];
    jd_val *push0 = stack_out->vals[0];
    stack_clone_val(push0, pop0);
    push0->ins = ins;
}

static void jvm_run_array_load_local_variable(jd_ins *ins)
{
    // ..., arrayref, index →
    // ..., value
    if (!jvm_ins_is_array_load(ins))
         return;
    jd_stack *stack_in  = ins->stack_in;
    jd_stack *stack_out = ins->stack_out;
    // 找到array对应的local variable
    jd_val *push_val = stack_out->vals[0];
    jd_val *popped_array_var = stack_in->vals[1];

    string arr_cname = popped_array_var->data->cname;
    push_val->data->cname = descriptor_item_class_name(arr_cname);
    push_val->ins = ins;
}

static void jvm_stack_var_defination(jd_ins *ins)
{
    jd_method *m = ins->method;
    for (int i = 0; i < ins->pushed_cnt; ++i) {
        jd_val *val = ins->stack_out->vals[i];
        jd_var *stack_var = stack_find_var(m, ins, i);
        if (stack_var == NULL) {
            stack_var = stack_define_var(m, val, i);
            hset_i2o(m->offset2var_map, ins->offset, stack_var);
        }
        val->stack_var = stack_var;
    }
}

static void jvm_run_stable_instruction_action(jd_ins *ins)
{
    ins->stack_out = make_obj(jd_stack);
    jd_stack *stack_out = ins->stack_out;
    jd_stack *stack_in = ins->stack_in;

    if (stack_in == NULL)
        stack_out->depth = ins->pushed_cnt;
    else
        stack_out->depth = stack_in->depth + ins->pushed_cnt - ins->popped_cnt;

    if (stack_out->depth == -1) {
        stack_out->depth = 0;
        stack_out->vals = NULL;
        DEBUG_STACK_PRINT("\tpopped[%d]: %s pop: %d\n",
                          ins->offset, ins->name, 0);
        return;
    }
    int current_depth = stack_in == NULL ? 0 : stack_in->depth;
    if (stack_out->depth > 0) {
        stack_out->vals = make_obj_arr(jd_val*, stack_out->depth);
    }
    current_depth -= ins->popped_cnt;

    DEBUG_STACK_PRINT("\t\tpushed: %d ", ins->offset);
    int tmp_pushed = 0;

    if (ins->pushed_cnt > 0) {
        // 除了几个动态的instruction外
        // 其余的instruction push到栈上的都是0或者1
        for (int i = 0; i < ins->pushed_cnt; ++i) {
            jd_val *val = stack_create_empty_val();
            val->ins = ins;
            stack_out->vals[i] = val;
        }
        instruction_stack_action(ins);
        tmp_pushed ++;
    }

    DEBUG_STACK_PRINT("\n");

    int i = stack_in->depth - current_depth;
    int j = 0;
    for (;i < stack_in->depth; ++i, ++j) {
        int stack_in_index = i;
        int stack_out_index = tmp_pushed + j;
        jd_val *item = stack_in->vals[stack_in_index];

        stack_out->vals[stack_out_index] = item;
    }

    stack_clone_local_variables(stack_out, stack_in);
    // 存储局部变量
    jvm_int_type_analyze(ins);
    jvm_run_store_local_variable(ins);
    jvm_run_load_local_variable(ins);
    jvm_run_array_load_local_variable(ins);
    jvm_run_athrow(ins);
    jvm_stack_var_defination(ins);
}

static void jvm_run_instruction_action(jd_ins *ins)
{
    switch (ins->code) {
        case INS_DUP: // dup
            build_jvm_ins_dup_action(ins);
            break;
        case INS_DUP_X1: // dup_x1
            build_jvm_ins_dup_x1_action(ins);
            break;
        case INS_DUP_X2: // dup_x2
            build_jvm_ins_dup_x2_action(ins);
            break;
        case INS_DUP2: // dup2
            build_jvm_ins_dup2_action(ins);
            break;
        case INS_DUP2_X1: // dup2_x1
            build_jvm_ins_dup2_x1_action(ins);
            break;
        case INS_DUP2_X2: // dup2_x2
            build_jvm_ins_dup2_x2_action(ins);
            break;
        case INS_SWAP: // swap
            build_jvm_ins_swap_action(ins);
            break;
        case INS_POP: // pop
            build_jvm_ins_pop_action(ins);
            break;
        case INS_POP2:
            build_jvm_ins_pop2_action(ins);
            break;
        default:
            jvm_run_stable_instruction_action(ins);
            break;
    }
}


static jd_stack* jvm_exception_stack(jd_method *m, jd_ins *ins, jd_stack *src)
{
    // exception的handler块，入栈只有一个exception的reference_ref
    // 找到ins对应的exception的class_name
    jd_bblock *block = block_handler_equals_ins(m, ins);
    jd_eblock *eblock = block->ub->eblock;
    string class_name = NULL;
    u2 class_index = eblock->exception->catch_type;
    if (class_index == 0) {
        class_name = str_dup("java/lang/Throwable");
    }
    else {
        jcp_info *info = pool_item(m->meta, class_index);
        class_name = get_class_name(m->meta, info);
    }

    jd_stack *clone = make_obj(jd_stack);
    clone->depth = 1;
    clone->vals = make_obj_arr(jd_val*, clone->depth);
    jd_val *exception_val = stack_create_empty_val();
    exception_val->type = JD_VAR_REFERENCE_T;
//    exception_val->data->cname = cname;
    exception_val->data->cname = class_simple_name(class_name);
    exception_val->ins = ins;
    exception_val->stack_var = stack_define_var(m, exception_val, 0);
    clone->vals[0] = exception_val;

    stack_clone_local_variables(clone, src);
    return clone;
}

static void jvm_fill_watch_successors(jd_method *m, jd_ins *ins)
{
    jd_bblock *block = ins->block;
    assert(block != NULL);
    jd_ins *end_ins = block->ub->nblock->end_ins;

    lclear_object(m->ins_watch_successors);

    if (ins->offset < end_ins->offset)
        ladd_obj(m->ins_watch_successors, ins->next);
    else {
        // find next block
        for (int i = 0; i < block->out->size; ++i) {
            jd_edge *edge = lget_obj(block->out, i);
            jd_bblock *target_block = edge->target_block;
            if (target_block->type == JD_BB_NORMAL) {
                if (jvm_ins_is_unconditional_jump(ins) &&
                        jvm_ins_is_goto_back(ins)) {
//                    jd_ins *ins_next = ins->next;
//                    if (ins_next != NULL)
//                        ladd_obj(m->ins_watch_successors, ins_next);
                    continue;
                }
                jd_ins *block_start_ins = target_block->ub->nblock->start_ins;
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
                jd_ins *hstart = handler_block->ub->nblock->start_ins;
                ladd_obj(m->ins_watch_successors, hstart);
            }
        }
    }
}

static void jvm_fill_visit_queue(jd_method *m, jd_ins *ins)
{
    for (int i = 0; i < m->ins_watch_successors->size; ++i) {
        jd_ins *suc_ins = lget_obj(m->ins_watch_successors, i);
        int is_handler_start = ins_is_handler_start(m, suc_ins);
        if (is_handler_start && suc_ins->stack_in == NULL) {
            suc_ins->stack_in = jvm_exception_stack(m,
                                                    suc_ins,
                                                    ins->stack_out);
            queue_add_object(m->ins_visit_queue, suc_ins);
        }
        else if (suc_ins->stack_in == NULL && !is_handler_start) {
            suc_ins->stack_in = stack_clone(ins->stack_out);
            queue_push_object(m->ins_visit_queue, suc_ins);
        }
        else {
            // intersection
        }
    }
}

static void jvm_ins_cb(jd_method *m, jd_ins *ins)
{
    jvm_fill_watch_successors(m, ins);

    jvm_run_instruction_action(ins); // 拿到ins的stack_out

    jvm_fill_visit_queue(m, ins);
}

static void jvm_process_instruction_action(jd_method *m, ins_action_cb cb)
{
    DEBUG_STACK_PRINT("[stack simulator]: %s \n", m->name);
    int times = 0;
    jd_ins *ins = NULL;
    while ((ins = queue_pop_object(m->ins_visit_queue)) != NULL) {
        times ++;
        cb(m, ins);
    }

    DEBUG_STACK_PRINT("[stack simulator]: %s process => %d, "
                      "instruction: %d\n\n",
                      m->name,
                      times,
                      m->ins_size);
}

static void jvm_method_enter_stack(jd_method *m)
{
    jclass_file *jc = m->meta;

    stack_create_method_enter(m);
    jd_stack *stack = m->enter;
    bool instance = method_is_member(m);
    int slot = instance ? 1 : 0;

    if (instance) {
        jcp_info *info = pool_item(m->meta, jc->this_class);
        string full = get_class_name(m->meta, info);
        string cname = class_simple_name(full);
        stack_create_method_this_val(m, 0, cname);
    }

    for (int i = 0; i < m->desc->list->size; ++i) {
        string item = lget_string(m->desc->list, i);
        jd_val *val = stack_create_val_with_descriptor(m, item, slot);
        jvm_variable_name(m, NULL, val, slot);
        if (val->type == JD_VAR_LONG_T ||
            val->type == JD_VAR_DOUBLE_T) {
            stack->local_vars[slot] = val;
            slot++;
        }
        m->parameters[i] = val;
        stack->local_vars[slot] = val;
        slot ++;
        // double和long的数据类型占据两个slot
        // 实现的时候，local variable和operand stack的slot
        // 都是一个slot对应一个变量
    }
}

void jvm_simulator(jd_method *m)
{
    if (method_is_empty(m))
        return;
    m->ins_watch_successors = linit_object();
    m->ins_visit_queue = queue_init_object();
    m->stack_variables = linit_object();
    // m->offset2varidx_map = hashmap_init((hcmp_fn) i2i_cmp, 0);
    m->offset2var_map = hashmap_init((hcmp_fn)i2obj_cmp, 0);
    m->class_counter_map = hashmap_init((hcmp_fn) s2i_cmp, 0);
    m->slot_counter_map = hashmap_init((hcmp_fn) i2i_cmp, 0);
    m->class_counter_map = hashmap_init((hcmp_fn) s2i_cmp, 0);
    m->var_name_map = hashmap_init((hcmp_fn)s2s_cmp, 0);
    m->types = linit_object();

    jd_ins *start = lget_obj_first(m->instructions);

    sform_prepare_for_local_variable(m);

    jvm_method_enter_stack(m);

    queue_push_object(m->ins_visit_queue, start);

    jvm_process_instruction_action(m, jvm_ins_cb);

    mark_unreachable_instruction(m);

    sform_for_local_variable(m);

    sform_for_stack_variable(m);
}

#if false
static int merge_stack_jd_var(jd_val *merged,
                              jd_val *other,
                              jd_ins *suc_ins)
{
    int type_changed = 0;
    if (other->type != JD_VAR_TOP_T &&
        merged->type == JD_VAR_TOP_T) {
        merged->type = JD_VAR_TOP_T;
        type_changed = 1;
    }
    else if (other->type == JD_VAR_TOP_T &&
             merged->type != JD_VAR_TOP_T) {
        merged->type = JD_VAR_TOP_T;
    }
    return type_changed;
}

static int merge_intersection_stack(jd_ins *ins,
                                    jd_stack *prev,
                                    jd_stack *in,
                                    jd_stack *merged,
                                    jd_ins *suc_ins)
{
    int changed = 0;
    // 这里是分支汇聚时候，创建一个新的stack，
    // 然后用新的stack和in进行对比，如果不一样，就把in放入队列

    if (prev->depth > 0) {
        merged->vals = make_obj_arr(jd_val*, prev->depth);
        merged->depth = prev->depth;
        memcpy(merged->vals, prev->vals, sizeof(jd_val*) * prev->depth);
    }
    else {
        merged->depth = 0;
    }
    clone_local_variables(merged, prev);

    jd_val *merge_jd_val;
    jd_val *in_jd_val;
    for (int i = 0; i < merged->depth; ++i) {
        merge_jd_val = merged->vals[i];
        in_jd_val    = in->vals[i];
        changed |= merge_stack_jd_var(merge_jd_val,
                                      in_jd_val,
                                      suc_ins);
    }

    jd_val *in_local;
    jd_val *merged_local;

    for (int i = 0; i < in->local_vars_count; ++i) {
        in_local = in->local_vars[i];
        if (in_local == NULL)
            continue;

        merged_local = find_local_variable(merged, in_local->slot);

        if (merged_local != NULL && merged_local == in_local)
            continue;

        if (merged_local == NULL) {
            jd_val *new_local = make_stack_val();
            stack_clone_val(new_local, in_local);
            merged->local_vars[in_local->slot] = new_local;
            changed = 1;
            continue;
        }

        if (merged_local != in_local) {
            changed = 1;
        }
    }

    return changed;
}

static string get_array_item_descriptor(jd_val *val, string descriptor)
{
    if (descriptor != NULL && descriptor[0] == '[') {
        size_t len = strlen(descriptor);
        // [Ljava/lang/String
        string buf = x_alloc(len - 1 + 1);
        memcpy(buf, &descriptor[1], len - 1);
        buf[len] = '\0';
        return buf;
    }
    else {
        switch (val->type) {
            case JD_VAR_INT_T:
                return str_dup("I");
            case JD_VAR_LONG_T:
                return str_dup("J");
            case JD_VAR_FLOAT_T:
                return str_dup("F");
            case JD_VAR_DOUBLE_T:
                return str_dup("D");
            case JD_VAR_REFERENCE_T:
                return str_dup("Ljava/lang/Object");
            default:
                return str_dup(g_str_unknown);
        }
    }
}
#endif

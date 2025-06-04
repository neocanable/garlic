#include "decompiler/stack.h"
#include "jvm/jvm_ins.h"
#include "klass.h"
#include "descriptor.h"

jd_var* stack_define_var(jd_method *m, jd_val *val, int slot)
{
    jd_var *var = make_obj(jd_var);
    var->slot = slot;
    var->ins = val->ins;
    var->idx = m->stack_variables->size;
    var->cname = val->data->cname;
//    string label;
//    if (val->ins == NULL)
//        label = str_create("v_%x_%x", 0, var->idx);
//    else
//        label = str_create("v_%x_%x", val->ins->offset, var->idx);

    var->name = str_create("v_%d", var->idx);
    val->stack_var = var;
    var->def_val = val;
    ladd_obj(m->stack_variables, var);
    return var;
}

void stack_create_method_this_val(jd_method *m, int slot, string cname)
{
    jd_val *this = stack_create_empty_val();
    this->slot = slot;
    this->type = JD_VAR_REFERENCE_T;
    this->ins  = NULL;
    this->name = g_str_this;
    m->variable_counter++;
    this->data->cname = cname;
    m->enter->local_vars[slot] = this;
}

void stack_create_method_enter(jd_method *m)
{
    m->enter = make_obj(jd_stack);
    jd_stack *stack = m->enter;
    stack->depth = 0;
    stack->vals = NULL;

    jd_ins *start_ins = lget_obj_first(m->instructions);
    start_ins->stack_in = m->enter;

    stack->local_vars_count = m->max_locals;
    int local_vars_count = stack->local_vars_count;
    stack->local_vars = make_obj_arr(jd_val*, local_vars_count);
    memset(stack->local_vars, 0, sizeof(jd_val*) * local_vars_count);

    if (!is_list_empty(m->desc->list))
        m->parameters = make_obj_arr(jd_val*, m->desc->list->size);
}

jd_val* stack_create_val_with_descriptor(jd_method *m, string desc, int slot)
{
    jd_val *val = stack_create_empty_val();
    val->slot = slot;
    val->type = descriptor_data_type(desc);
    string full = class_full_name(desc);
    val->data->cname = class_simple_name(full);
    return val;
}

void stack_define_var_for_enter_stack(jd_method *m)
{
    jd_stack *stack = m->enter;
    for (int i = 0; i < stack->local_vars_count; ++i) {
        jd_val *val = stack->local_vars[i];
        if (val != NULL)
            stack_define_var(m, val, i);
    }
}

jd_stack* stack_clone(jd_stack *source)
{
    jd_stack *clone = make_obj(jd_stack);
    clone->depth = source->depth;
    if (clone->depth > 0) {
        clone->vals = make_obj_arr(jd_val*, clone->depth);
        memcpy(clone->vals, source->vals, clone->depth*sizeof(jd_val *));
    }
    stack_clone_local_variables(clone, source);
    return clone;
}

void stack_val_name(jd_method *m, jd_ins *ins, jd_val *val, int slot)
{
    if (ins != NULL) {
        int counter = hget_i2i(m->slot_counter_map, slot);
        if (counter == -1)
            counter = 0;
        hset_i2i(m->slot_counter_map, slot, counter + 1);
        val->name = str_create("var_%d_%d", slot, counter);
//        int len = number_digits(slot) + number_digits(counter) + 6; // "var_"
//        val->name = x_alloc(len);
//        snprintf(val->name, len, "var_%d_%d", slot, counter);
    }
    else {
//        int len = number_digits(m->variable_counter) + 5; // "var_"
//        val->name = x_alloc(len);
//        snprintf(val->name, len, "var_%d", m->variable_counter);
        val->name = str_create("var_%d", m->variable_counter);

        m->variable_counter++;
    }
}

static jd_var* stack_find_var_by_idx(jd_method *m, int idx)
{
    size_t sz = m->stack_variables->size;
    if (sz == 0 || idx >= sz)
        return NULL;
    return lget_obj(m->stack_variables, idx);
}

jd_var* stack_find_var(jd_method *m, jd_ins *ins, int slot)
{
//    int idx = hget_i2i(m->offset2varidx_map, ins->offset);
//    if (idx == -1)
//        return NULL;
//    return stack_find_var_by_idx(m, idx);

    return hget_i2o(m->offset2var_map, ins->offset);
}

void mark_unreachable_instruction(jd_method *m)
{
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_ins *ins = lget_obj(m->instructions, i);
        if (ins->stack_in == NULL) {
            DEBUG_PRINT("[unreach]: m: %s %s %d\n",
                        m->name,
                        ins->name,
                        ins->offset);
            ins_mark_unreached(ins);
        }
    }
}
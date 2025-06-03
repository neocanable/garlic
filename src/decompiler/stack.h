#ifndef GARLIC_STACK_H
#define GARLIC_STACK_H

#include "decompiler/structure.h"

typedef void (*ins_action_cb)(jd_method *m, jd_ins *ins);

jd_var* stack_define_var(jd_method *m, jd_val *val, int slot);

jd_var* stack_find_var(jd_method *m, jd_ins *ins, int slot);

void stack_define_var_for_enter_stack(jd_method *m);

void mark_unreachable_instruction(jd_method *m);

void stack_create_method_enter(jd_method *m);

void stack_create_method_this_val(jd_method *m, int slot, string cname);

jd_val* stack_create_val_with_descriptor(jd_method *m, string desc, int slot);

void stack_val_name(jd_method *m, jd_ins *ins, jd_val *val, int slot);

jd_stack* stack_clone(jd_stack *source);

static inline jd_val* stack_create_empty_val()
{
    jd_val *val = make_obj(jd_val);
    val->data = make_obj(jd_val_data);
    return val;
}

static inline jd_val* stack_make_primitive_val(jd_var_types type)
{
    jd_val *val = make_obj(jd_val);
    val->data = make_obj(jd_val_data);
    val->data->primitive = make_obj(jd_primitive_union);
    val->type = type;
    return val;
}

static inline void stack_clone_val(jd_val *dst, jd_val *src)
{
    memcpy(dst, src, sizeof(jd_val));
    dst->data = make_obj(jd_val_data);

//    if (src->defs != NULL)
//        dst->defs = bitset_copy(src->defs);

    if (src->data != NULL && dst->data != NULL)
        memcpy(dst->data, src->data, sizeof(jd_val_data));
}

static inline void stack_clone_local_variables(jd_stack *dst, jd_stack *src)
{
    dst->local_vars_count = src->local_vars_count;
    size_t size = dst->local_vars_count * sizeof(jd_val*);
    dst->local_vars = x_alloc(size);
    memcpy(dst->local_vars, src->local_vars, size);
}

static inline bool stack_val_is_int(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_int);
}

static inline bool stack_val_is_long(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_long);
}

static inline bool stack_val_is_float(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_float);
}

static inline bool stack_val_is_double(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_double);
}

static inline bool stack_val_is_byte(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_byte);
}

static inline bool stack_val_is_short(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_short);
}

static inline bool stack_val_is_char(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_char);
}

static inline bool stack_val_is_boolean(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_boolean);
}

static inline bool stack_val_is_string(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_String);
}

static inline bool stack_val_is_wide(jd_val *val)
{
    if (val == NULL || val->data == NULL)
        return false;
    return STR_EQL(val->data->cname, g_str_long) ||
           STR_EQL(val->data->cname, g_str_double);
}
#endif //GARLIC_STACK_H

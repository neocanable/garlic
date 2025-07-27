#include "dalvik/dex_ins.h"
#include "dalvik/dex_structure.h"
#include "dalvik/dex_simulator.h"
#include "dalvik/dex_meta_helper.h"

#include "decompiler/stack.h"
#include "decompiler/descriptor.h"
#include "decompiler/klass.h"
#include "debug.h"

static inline string dex_desc_full_name(jd_dex_ins *ins, string desc)
{
    string fname = class_full_name(desc);
    jsource_file *jf = ins->method->jfile;
    class_import(jf, fname);
    return fname;
}

static inline jd_val* dex_stack_val(jd_dex_ins *ins, int slot, char desc)
{
    jd_val *val = make_obj(jd_val);
    val->data = make_obj(jd_val_data);
    val->type = descriptor_data_type_of_char(desc);
    val->data->cname = descriptor_class_name_of_primitive(desc);
    val->ins = ins;
    val->slot = slot;
    return val;
}

static inline jd_val* dex_primitive_val(jd_dex_ins *ins, int slot, char desc)
{
    jd_val *val = dex_stack_val(ins, slot, desc);
    val->data->primitive = make_obj(jd_primitive_union);
    return val;
}

static inline void save_stack_val(jd_dex_ins *ins, jd_val *val, int slot)
{
    ins->stack_out->local_vars[slot] = val;
}

static inline void dex_build_ins_default_act(jd_dex_ins *ins)
{
    ins->stack_out = stack_clone(ins->stack_in);
}

static inline void move_act(jd_dex_ins *ins, int dst, int src)
{
    jd_val *src_val = ins->stack_in->local_vars[src];
    jd_val *dst_val = stack_create_empty_val();

    if (src_val == NULL) {
        fprintf(stderr, "[%s] register empty: %d not found\n", ins->name, src);
        src_val = stack_create_empty_val();
        src_val->data = make_obj(jd_val_data);
        src_val->data->cname = (string)g_str_Object;
        src_val->type = JD_VAR_REFERENCE_T;
        src_val->slot = src;
        src_val->ins  = NULL;
    }

    stack_clone_val(dst_val, src_val);
    dst_val->slot = dst;
    dst_val->ins = ins;
    save_stack_val(ins, dst_val, dst);

    if (dex_ins_is_move_wide_16(ins) ||
            dex_ins_is_move_wide(ins) ||
            dex_ins_is_move_wide_from16(ins)) {
        save_stack_val(ins, ins->stack_out->local_vars[dst], dst + 1);
    }
}

static inline void build_dex_ins_move_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_from16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_16_act(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_wide_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_wide_from16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_wide_16_act(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_object_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_object_from16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_object_16_act(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    move_act(ins, u_a, u_b);
}

static inline void build_dex_ins_move_result_act(jd_dex_ins *ins)
{
    jd_dex_ins *prev = ins->prev;
    s8 meth_idx = 0;
    jd_meta_dex *meta = dex_ins_meta(ins);
    switch (prev->code) {
        case DEX_INS_INVOKE_VIRTUAL:
        case DEX_INS_INVOKE_SUPER:
        case DEX_INS_INVOKE_DIRECT:
        case DEX_INS_INVOKE_STATIC:
        case DEX_INS_INVOKE_INTERFACE:
        case DEX_INS_INVOKE_VIRTUAL_RANGE:
        case DEX_INS_INVOKE_SUPER_RANGE:
        case DEX_INS_INVOKE_DIRECT_RANGE:
        case DEX_INS_INVOKE_STATIC_RANGE:
        case DEX_INS_INVOKE_INTERFACE_RANGE: {
            meth_idx = dex_ins_parameter(prev, 1);
            break;
        }
        case DEX_INS_INVOKE_POLYMORPHIC:
        case DEX_INS_INVOKE_POLYMORPHIC_RANGE: {
            meth_idx = dex_ins_parameter(prev, 1);
            break;
        }
        default: {
            meth_idx = -1;
            break;
        }
    }
    if (meth_idx == -1) {
        // prev is fill_new_array or fill_new_array_range
        assert(dex_ins_is_filled_new_array(prev) ||
                dex_ins_is_filled_new_array_range(prev));
        u1 u_a = dex_ins_parameter(ins, 0);
        u2 type_idx = dex_ins_parameter(prev, 1);

        string desc = dex_str_of_type_id(meta, type_idx);
        jd_val *val = stack_create_empty_val();
        val->type = JD_VAR_REFERENCE_T;
        string full = dex_desc_full_name(ins, desc);
        val->data->cname = class_simple_name(full);
        val->ins = ins;
        val->slot = u_a;
        save_stack_val(ins, val, u_a);
    }
    else {
        dex_method_id *method_id = &meta->method_ids[meth_idx];
        dex_proto_id *proto_id = &meta->proto_ids[method_id->proto_idx];
        string desc = dex_str_of_type_id(meta, proto_id->return_type_idx);

        u1 u_a = dex_ins_parameter(ins, 0);
        jd_val *val = stack_create_empty_val();
        val->type = descriptor_data_type_of_char(desc[0]);
        if (val->type == JD_VAR_REFERENCE_T) {
            string full = dex_desc_full_name(ins, desc);
            val->data->cname = class_simple_name(full);
        }
        else {
            string class_name = descriptor_class_name_of_primitive(desc[0]);
            val->data->cname = class_name;
        }
        val->ins = ins;
        val->slot = u_a;
        save_stack_val(ins, val, u_a);

        if (dex_ins_is_move_result_wide(ins))
            save_stack_val(ins, val, u_a + 1);
    }
}

static inline void build_dex_ins_move_result_wide_act(jd_dex_ins *ins)
{
    build_dex_ins_move_result_act(ins);
}

static inline void build_dex_ins_move_result_object_act(jd_dex_ins *ins)
{
    build_dex_ins_move_result_act(ins);
}

static inline void build_dex_ins_move_exception_act(jd_dex_ins *ins){}

static inline void build_dex_ins_return_act(jd_dex_ins *ins)
{
    if (!dex_ins_is_return(ins))
        return;

    u8 u_a = dex_ins_parameter(ins, 0);
    jd_val *val = ins->stack_in->local_vars[u_a];

    jd_method *m = ins->method;
    string return_type = m->desc->str_return;
    if (STR_EQL(return_type, "Z")) {
        val->data->cname = (string)g_str_boolean;
        val->stack_var->cname = (string)g_str_boolean;
    }
    else if (STR_EQL(return_type, "B")) {
        val->data->cname = (string)g_str_byte;
        val->stack_var->cname = (string)g_str_byte;
    }
    else if (STR_EQL(return_type, "C")) {
        val->data->cname = (string)g_str_char;
        val->stack_var->cname = (string)g_str_char;
    }
    else if (STR_EQL(return_type, "S")) {
        val->data->cname = (string)g_str_short;
        val->stack_var->cname = (string)g_str_short;
    }
}

static inline void build_dex_ins_const_int_act(jd_dex_ins *ins, int reg, int v)
{
    jd_val *val = dex_primitive_val(ins, reg, 'I');
    val->data->primitive->int_val = v;
    save_stack_val(ins, val, reg);
}

static inline void build_dex_ins_const_4_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s1 u_b = (s1)dex_ins_parameter(ins, 1);
    build_dex_ins_const_int_act(ins, u_a, u_b);
}

static inline void build_dex_ins_const_16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s2 u_b = (s2)dex_ins_parameter(ins, 1);
    build_dex_ins_const_int_act(ins, u_a, u_b);
}

static inline void build_dex_ins_const_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 u_b = dex_ins_parameter(ins, 1);
    build_dex_ins_const_int_act(ins, u_a, u_b);
}

static inline void build_dex_ins_const_high16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 u_b = dex_ins_parameter(ins, 1) << 16;

    jd_val *val = dex_primitive_val(ins, u_a, 'F');
    val->data->primitive->float_val = (float)u_b;
    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_long_act(jd_dex_ins *ins, int reg, long l)
{
    jd_val *val = dex_primitive_val(ins, reg, 'J');
    val->data->primitive->long_val = l;
    save_stack_val(ins, val, reg);
    save_stack_val(ins, val, reg + 1);
}

static inline void build_dex_ins_double_act(jd_dex_ins *ins, 
                                            int reg, 
                                            double lval)
{
    jd_val *val = dex_primitive_val(ins, reg, 'D');
    val->data->primitive->double_val = lval;
    save_stack_val(ins, val, reg);
    save_stack_val(ins, val, reg + 1);
}

static inline void build_dex_ins_const_wide_16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s2 u_b = (s2)dex_ins_parameter(ins, 1);
    build_dex_ins_long_act(ins, u_a, u_b);
}

static inline void build_dex_ins_const_wide_32_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 u_b = dex_ins_parameter(ins, 1);

    build_dex_ins_long_act(ins, u_a, u_b);
}

static inline void build_dex_ins_const_wide_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s8 u_b = (s8)dex_ins_parameter(ins, 1);
    build_dex_ins_double_act(ins, u_a, u_b);
}

static inline void build_dex_ins_const_wide_high16_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s4 b1 = (s4)dex_ins_parameter(ins, 1);
    s8 u_b = (s8)b1 << 48;

    jd_val *val = dex_primitive_val(ins, u_a, 'J');
    val->data->primitive->long_val = u_b;

    save_stack_val(ins, val, u_a);
    save_stack_val(ins, val, u_a + 1);
}

static inline void build_dex_ins_string_act(jd_dex_ins *ins, int reg, u4 idx)
{
    jd_meta_dex *meta = dex_ins_meta(ins);
    string str = dex_str_of_idx(meta, idx);

    jd_val *val = stack_create_empty_val();
    jd_val_data *data = val->data;
    data->val = str_dup(str);
    val->type = JD_VAR_REFERENCE_T;
    data->cname = (string)g_str_String;

    val->ins = ins;
    val->slot = reg;
    save_stack_val(ins, val, reg);
}

static inline void build_dex_ins_const_string_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 string_index = dex_ins_parameter(ins, 1);

    build_dex_ins_string_act(ins, u_a, string_index);
}

static inline void build_dex_ins_const_string_jumbo_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 string_index = dex_ins_parameter(ins, 1);
    build_dex_ins_string_act(ins, u_a, string_index);
}

static inline void build_dex_ins_const_class_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);

    jd_meta_dex *meta = dex_ins_meta(ins);
    string type_desc = dex_str_of_type_id(meta, type_index);
    jd_val *val = stack_create_empty_val();
    jd_val_data *data = val->data;
    data->cname = (string)g_str_Class;

    data->val = type_desc;
    val->type = JD_VAR_REFERENCE_T;
    val->ins = ins;
    val->slot = u_a;

    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_monitor_act(jd_dex_ins *ins){}

static inline void build_dex_ins_check_cast_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    jd_meta_dex *meta = dex_ins_meta(ins);

    string type_desc = dex_str_of_type_id(meta, type_index);
    jd_val *out_val = ins->stack_out->local_vars[u_a];
    string full = dex_desc_full_name(ins, type_desc);
    out_val->data->cname = class_simple_name(full);
}

static inline void build_dex_ins_instance_of_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);

    jd_val *val = dex_stack_val(ins, u_a, 'Z');

    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_array_length_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);

    jd_val *val = dex_stack_val(ins, u_a, 'I');
    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_new_instance_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);

    jd_meta_dex *meta = dex_ins_meta(ins);
    string type_desc = dex_str_of_type_id(meta, type_index);

    jd_val *val = stack_create_empty_val();
    jd_val_data *data = val->data;
    string full = dex_desc_full_name(ins, type_desc);
    data->cname = class_simple_name(full);
    val->type = JD_VAR_UNINITIALIZED_T;
    val->ins = ins;
    val->slot = u_a;

    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_new_array_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u2 type_index = dex_ins_parameter(ins, 2);

    jd_meta_dex *meta = dex_ins_meta(ins);
    string type_desc = dex_str_of_type_id(meta, type_index);

    jd_val *val = stack_create_empty_val();
    jd_val_data *data = val->data;
    string full = dex_desc_full_name(ins, type_desc);
    data->cname = class_simple_name(full);
    val->type = JD_VAR_REFERENCE_T;
    val->ins = ins;

    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_fill_new_array_act(jd_dex_ins *ins){}

static inline void build_dex_ins_fill_new_array_range_act(jd_dex_ins *ins){}

static inline void build_dex_ins_fill_array_data_act(jd_dex_ins *ins){}

static inline void build_dex_ins_throw_act(jd_dex_ins *ins){}

static inline void build_dex_ins_goto_act(jd_dex_ins *ins){}

static inline void build_dex_ins_switch_act(jd_dex_ins *ins){}

static inline void build_dex_ins_cmp_kind_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);

    jd_val *val = dex_primitive_val(ins, u_a, 'I');
    val->data->primitive->int_val = 0;

    save_stack_val(ins, val, u_a);
}

static inline void build_dex_ins_if_kind_act(jd_dex_ins *ins){}

static inline void build_dex_ins_ifz_act(jd_dex_ins *ins){}

static inline void build_dex_ins_arrayop_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);

    if (dex_ins_is_aget(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'I');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_aget_wide(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'J');
        save_stack_val(ins, val, u_a);
        save_stack_val(ins, val, u_a + 1);
    }
    else if (dex_ins_is_aget_object(ins)) {
        jd_val *arr_val = ins->stack_in->local_vars[u_c];
        string arr_cname = arr_val->data->cname;

        jd_val *val = stack_create_empty_val();
        jd_val_data *data = val->data;
        data->cname = descriptor_item_class_name(arr_cname);

        val->type = JD_VAR_REFERENCE_T;
        val->slot = u_a;
        val->ins = ins;
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_aget_boolean(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'Z');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_aget_byte(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'B');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_aget_char(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'C');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_aget_short(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'S');
        save_stack_val(ins, val, u_a);
    }
    else {
        // aput action
    }
}

static inline void build_dex_ins_instanceop_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u2 field_index = dex_ins_parameter(ins, 1);

    jd_meta_dex *meta = dex_ins_meta(ins);
    dex_field_id *field_id = &meta->field_ids[field_index];
    string field_name = dex_str_of_idx(meta, field_id->name_idx);
    string type_name = dex_str_of_type_id(meta, field_id->type_idx);
    string desc = dex_str_of_type_id(meta, field_id->class_idx);

    if (dex_ins_is_iget(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'I');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_iget_wide(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'J');
        save_stack_val(ins, val, u_a);
        save_stack_val(ins, val, u_a + 1);
    }
    else if (dex_ins_is_iget_object(ins)) {
        jd_val *val = stack_create_empty_val();
        jd_val_data *data = val->data;
        val->slot = u_a;
        val->ins = ins;
        string full = dex_desc_full_name(ins, desc);
        data->cname = class_simple_name(full);
        val->type = JD_VAR_REFERENCE_T;

        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_iget_boolean(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'Z');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_iget_byte(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'B');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_iget_char(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'C');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_iget_short(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'S');
        save_stack_val(ins, val, u_a);
    }
    else {
        // iput action
        jd_val *val = ins->stack_in->local_vars[u_a];
        if (dex_ins_is_iput_boolean(ins)) {
            val->data->cname = (string)g_str_boolean;
            val->stack_var->cname = (string)g_str_boolean;
        }
        else if (dex_ins_is_iput_byte(ins)) {
            val->data->cname = (string)g_str_byte;
            val->stack_var->cname = (string)g_str_byte;
        }
        else if (dex_ins_is_iput_char(ins)) {
            val->data->cname = (string)g_str_char;
            val->stack_var->cname = (string)g_str_char;
        }
        else if (dex_ins_is_iput_short(ins)) {
            val->data->cname = (string)g_str_short;
            val->stack_var->cname = (string)g_str_short;
        }
    }
}

static inline void build_dex_ins_staticop_act(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 field_index = dex_ins_parameter(ins, 1);

    jd_meta_dex *meta = dex_ins_meta(ins);
    dex_field_id *field_id = &meta->field_ids[field_index];
    string field_name = dex_str_of_idx(meta, field_id->name_idx);
    string desc = dex_str_of_type_id(meta, field_id->class_idx);
    string type_name = dex_str_of_type_id(meta, field_id->type_idx);

    if (dex_ins_is_sget(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'I');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_sget_wide(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'J');
        save_stack_val(ins, val, u_a);
        save_stack_val(ins, val, u_a + 1);
    }
    else if (dex_ins_is_sget_object(ins)) {
        jd_val *val = stack_create_empty_val();
        val->ins = ins;
        val->slot = u_a;
        jd_val_data *data = val->data;
        string full = dex_desc_full_name(ins, desc);
        data->cname = class_simple_name(full);
        data->val = field_name;
        val->type = JD_VAR_REFERENCE_T;
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_sget_boolean(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'Z');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_sget_byte(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'B');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_sget_char(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'C');
        save_stack_val(ins, val, u_a);
    }
    else if (dex_ins_is_sget_short(ins)) {
        jd_val *val = dex_stack_val(ins, u_a, 'S');
        save_stack_val(ins, val, u_a);
    }
    else {
        // sput action
        jd_val *val = ins->stack_in->local_vars[u_a];
        if (dex_ins_is_sput_boolean(ins)) {
            val->data->cname = (string)g_str_boolean;
            val->stack_var->cname = (string)g_str_boolean;
        }
        else if (dex_ins_is_sput_byte(ins)) {
            val->data->cname = (string)g_str_byte;
            val->stack_var->cname = (string)g_str_byte;
        }
        else if (dex_ins_is_sput_char(ins)) {
            val->data->cname = (string)g_str_char;
            val->stack_var->cname = (string)g_str_char;
        }
        else if (dex_ins_is_sput_short(ins)) {
            val->data->cname = (string)g_str_short;
            val->stack_var->cname = (string)g_str_short;
        }
    }
}

static inline void build_dex_ins_invoke_act(jd_dex_ins *ins)
{
    // TODO: 这里需要根据方法的参数类型来校正jd_val的class name
    //       invoke和invoke-range都需要校正

    jd_dex *dex = ins->method->meta;
    jd_meta_dex *meta = dex->meta;
    u1 param_size = dex_ins_parameter(ins, 0);
    u2 method_index = dex_ins_parameter(ins, 1);
    dex_method_id *method_id = &meta->method_ids[method_index];
    dex_proto_id *proto_id = &meta->proto_ids[method_id->proto_idx];
    string name = dex_str_of_idx(meta, method_id->name_idx);

    if (proto_id->parameters_off == 0) {
        // printf("[invoke-void] : %s()\n", name);
        return;
    }

    u1 real_param_size = param_size;
    for (int i = 0; i < param_size; ++i) {
        u2 slot = dex_ins_parameter(ins, i + 2);
        jd_val *val = ins->stack_in->local_vars[slot];
        if (stack_val_is_wide(val)) {
            real_param_size--;
            i++;
        }
    }
    param_size = real_param_size;


    DEBUG_PRINT("[invoke]: %s(", name);
    int increase = 2;
    int type_increase = 0;
    int param_itor = dex_ins_is_invokestatic(ins) ? 0 : 1;
    for (int i = param_itor; i < param_size; ++i) {
        u2 slot = dex_ins_parameter(ins, i + increase);
        jd_val *val = ins->stack_in->local_vars[slot];

        dex_type_item *type_item = &proto_id->type_list->list[type_increase];
        dex_type_id *tid = &meta->type_ids[type_item->type_idx];
        string type = meta->strings[tid->descriptor_idx].data;
        if (STR_EQL(type, "Z") && !stack_val_is_boolean(val)) {
            val->data->cname = (string)g_str_boolean;
            val->stack_var->cname = (string)g_str_boolean;
        }
        else if (STR_EQL(type, "B") && !stack_val_is_byte(val)) {
            val->data->cname = (string)g_str_byte;
            val->stack_var->cname = (string)g_str_byte;
        }
        else if (STR_EQL(type, "C") && !stack_val_is_char(val)) {
            val->data->cname = (string)g_str_char;
            val->stack_var->cname = (string)g_str_char;
        }
        else if (STR_EQL(type, "S") && !stack_val_is_short(val)) {
            val->data->cname = (string)g_str_short;
            val->stack_var->cname = (string)g_str_short;
        }
        DEBUG_PRINT("%s -> %s,", val->data->cname, type);

        if (stack_val_is_wide(val))
            increase++;

        type_increase++;
    }
    DEBUG_PRINT(")\n");
}

static void build_dex_ins_invoke_range_act(jd_dex_ins *ins)
{
    jd_dex *dex = ins->method->meta;
    jd_meta_dex *meta = dex->meta;
    u2 method_index = dex_ins_parameter(ins, 1);
    dex_method_id *method_id = &meta->method_ids[method_index];
    dex_proto_id *proto_id = &meta->proto_ids[method_id->proto_idx];
    string name = dex_str_of_idx(meta, method_id->name_idx);

    if (proto_id->parameters_off == 0) {
        return;
    }

    u1 start = dex_ins_parameter(ins, 0);
    u2 start_index = dex_ins_parameter(ins, 2);
    u2 count = start_index + start - 1;
    u2 param_size = count - start_index + 1;

    u1 real_param_size = param_size;
    for (int i = start_index; i <= count; ++i) {
        jd_val *val = ins->stack_in->local_vars[i];
        if (stack_val_is_wide(val)) {
            real_param_size--;
            i++;
        }
    }
    param_size = real_param_size;

    int increase = 0;
    int type_increase = 0;
    int param_itor_start = dex_ins_is_invoke_static_range(ins) ? 0 : 1;
    for (int i = param_itor_start; i < param_size; ++i) {
        jd_val *val = ins->stack_in->local_vars[i + start_index + increase];

        dex_type_item *type_item = &proto_id->type_list->list[type_increase];
        dex_type_id *tid = &meta->type_ids[type_item->type_idx];
        string type = meta->strings[tid->descriptor_idx].data;
        if (STR_EQL(type, "Z") && !stack_val_is_boolean(val)) {
            val->data->cname = (string)g_str_boolean;
            val->stack_var->cname = (string)g_str_boolean;
        }
        else if (STR_EQL(type, "B") && !stack_val_is_byte(val)) {
            val->data->cname = (string)g_str_byte;
            val->stack_var->cname = (string)g_str_byte;
        }
        else if (STR_EQL(type, "C") && !stack_val_is_char(val)) {
            val->data->cname = (string)g_str_char;
            val->stack_var->cname = (string)g_str_char;
        }
        else if (STR_EQL(type, "S") && !stack_val_is_short(val)) {
            val->data->cname = (string)g_str_short;
            val->stack_var->cname = (string)g_str_short;
        }

        if (stack_val_is_wide(val)) {
            increase++;
        }
        type_increase++;
    }
}

static inline void build_dex_ins_op_act(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);

    switch (ins->code) {
        case DEX_INS_NEG_INT: // unop
        case DEX_INS_NOT_INT:
        case DEX_INS_LONG_TO_INT:
        case DEX_INS_FLOAT_TO_INT:
        case DEX_INS_DOUBLE_TO_INT:
        case DEX_INS_ADD_INT: // binop
        case DEX_INS_SUB_INT:
        case DEX_INS_MUL_INT:
        case DEX_INS_DIV_INT:
        case DEX_INS_REM_INT:
        case DEX_INS_AND_INT:
        case DEX_INS_OR_INT:
        case DEX_INS_XOR_INT:
        case DEX_INS_SHL_INT:
        case DEX_INS_SHR_INT:
        case DEX_INS_USHR_INT:
        case DEX_INS_ADD_INT_2ADDR: // binop/2addr
        case DEX_INS_SUB_INT_2ADDR:
        case DEX_INS_MUL_INT_2ADDR:
        case DEX_INS_DIV_INT_2ADDR:
        case DEX_INS_REM_INT_2ADDR:
        case DEX_INS_AND_INT_2ADDR:
        case DEX_INS_OR_INT_2ADDR:
        case DEX_INS_XOR_INT_2ADDR:
        case DEX_INS_SHL_INT_2ADDR:
        case DEX_INS_SHR_INT_2ADDR:
        case DEX_INS_USHR_INT_2ADDR:
        case DEX_INS_ADD_INT_LIT16: // binop/lit16
        case DEX_INS_RSUB_INT:
        case DEX_INS_MUL_INT_LIT16:
        case DEX_INS_DIV_INT_LIT16:
        case DEX_INS_REM_INT_LIT16:
        case DEX_INS_AND_INT_LIT16:
        case DEX_INS_OR_INT_LIT16:
        case DEX_INS_XOR_INT_LIT16:
        case DEX_INS_ADD_INT_LIT8: // binop/lit8
        case DEX_INS_RSUB_INT_LIT8:
        case DEX_INS_MUL_INT_LIT8:
        case DEX_INS_DIV_INT_LIT8:
        case DEX_INS_REM_INT_LIT8:
        case DEX_INS_AND_INT_LIT8:
        case DEX_INS_OR_INT_LIT8:
        case DEX_INS_XOR_INT_LIT8:
        case DEX_INS_SHL_INT_LIT8:
        case DEX_INS_SHR_INT_LIT8:
        case DEX_INS_USHR_INT_LIT8: {
            jd_val *val = dex_stack_val(ins, u_a, 'I');
            save_stack_val(ins, val, u_a);
            break;
        }
        case DEX_INS_NEG_LONG:
        case DEX_INS_NOT_LONG:
        case DEX_INS_INT_TO_LONG:
        case DEX_INS_FLOAT_TO_LONG:
        case DEX_INS_DOUBLE_TO_LONG:
        case DEX_INS_ADD_LONG:
        case DEX_INS_SUB_LONG:
        case DEX_INS_MUL_LONG:
        case DEX_INS_DIV_LONG:
        case DEX_INS_REM_LONG:
        case DEX_INS_AND_LONG:
        case DEX_INS_OR_LONG:
        case DEX_INS_XOR_LONG:
        case DEX_INS_SHL_LONG:
        case DEX_INS_SHR_LONG:
        case DEX_INS_USHR_LONG:
        case DEX_INS_ADD_LONG_2ADDR:
        case DEX_INS_SUB_LONG_2ADDR:
        case DEX_INS_MUL_LONG_2ADDR:
        case DEX_INS_DIV_LONG_2ADDR:
        case DEX_INS_REM_LONG_2ADDR:
        case DEX_INS_AND_LONG_2ADDR:
        case DEX_INS_OR_LONG_2ADDR:
        case DEX_INS_XOR_LONG_2ADDR:
        case DEX_INS_SHL_LONG_2ADDR:
        case DEX_INS_SHR_LONG_2ADDR:
        case DEX_INS_USHR_LONG_2ADDR: {
            jd_val *val = dex_stack_val(ins, u_a, 'J');
            save_stack_val(ins, val, u_a);
            save_stack_val(ins, val, u_a + 1);
            break;
        }
        case DEX_INS_NEG_FLOAT:
        case DEX_INS_INT_TO_FLOAT:
        case DEX_INS_LONG_TO_FLOAT:
        case DEX_INS_DOUBLE_TO_FLOAT:
        case DEX_INS_ADD_FLOAT:
        case DEX_INS_SUB_FLOAT:
        case DEX_INS_MUL_FLOAT:
        case DEX_INS_DIV_FLOAT:
        case DEX_INS_REM_FLOAT:
        case DEX_INS_ADD_FLOAT_2ADDR:
        case DEX_INS_SUB_FLOAT_2ADDR:
        case DEX_INS_MUL_FLOAT_2ADDR:
        case DEX_INS_DIV_FLOAT_2ADDR:
        case DEX_INS_REM_FLOAT_2ADDR: {
            jd_val *val = dex_stack_val(ins, u_a, 'F');
            save_stack_val(ins, val, u_a);
            break;
        }
        case DEX_INS_NEG_DOUBLE:
        case DEX_INS_INT_TO_DOUBLE:
        case DEX_INS_LONG_TO_DOUBLE:
        case DEX_INS_FLOAT_TO_DOUBLE:
        case DEX_INS_ADD_DOUBLE:
        case DEX_INS_SUB_DOUBLE:
        case DEX_INS_MUL_DOUBLE:
        case DEX_INS_DIV_DOUBLE:
        case DEX_INS_REM_DOUBLE:
        case DEX_INS_ADD_DOUBLE_2ADDR:
        case DEX_INS_SUB_DOUBLE_2ADDR:
        case DEX_INS_MUL_DOUBLE_2ADDR:
        case DEX_INS_DIV_DOUBLE_2ADDR:
        case DEX_INS_REM_DOUBLE_2ADDR: {
            jd_val *val = dex_stack_val(ins, u_a, 'D');
            save_stack_val(ins, val, u_a);
            save_stack_val(ins, val, u_a + 1);
            break;
        }
        case DEX_INS_INT_TO_BYTE: {
            jd_val *val = dex_stack_val(ins, u_a, 'B');
            save_stack_val(ins, val, u_a);
            break;
        }
        case DEX_INS_INT_TO_CHAR: {
            jd_val *val = dex_stack_val(ins, u_a, 'C');
            save_stack_val(ins, val, u_a);
            break;
        }
        case DEX_INS_INT_TO_SHORT: {
            jd_val *val = dex_stack_val(ins, u_a, 'C');
            save_stack_val(ins, val, u_a);
            break;
        }
        default: {
            break;
        }
    }
}

void dex_ins_action(jd_dex_ins *ins)
{
    dex_build_ins_default_act(ins);
    switch (ins->code) {
        case DEX_INS_MOVE:
            build_dex_ins_move_act(ins);
            break;
        case DEX_INS_MOVE_FROM16:
            build_dex_ins_move_from16_act(ins);
            break;
        case DEX_INS_MOVE_16:
            build_dex_ins_move_16_act(ins);
            break;
        case DEX_INS_MOVE_WIDE:
            build_dex_ins_move_wide_act(ins);
            break;
        case DEX_INS_MOVE_WIDE_FROM16:
            build_dex_ins_move_wide_from16_act(ins);
            break;
        case DEX_INS_MOVE_WIDE_16:
            build_dex_ins_move_wide_16_act(ins);
            break;
        case DEX_INS_MOVE_OBJECT:
            build_dex_ins_move_object_act(ins);
            break;
        case DEX_INS_MOVE_OBJECT_FROM16:
            build_dex_ins_move_object_from16_act(ins);
            break;
        case DEX_INS_MOVE_OBJECT_16:
            build_dex_ins_move_object_16_act(ins);
            break;
        case DEX_INS_MOVE_RESULT:
            build_dex_ins_move_result_act(ins);
            break;
        case DEX_INS_MOVE_RESULT_WIDE:
            build_dex_ins_move_result_wide_act(ins);
            break;
        case DEX_INS_MOVE_RESULT_OBJECT:
            build_dex_ins_move_result_object_act(ins);
            break;
        case DEX_INS_RETURN_VOID:
        case DEX_INS_RETURN:
        case DEX_INS_RETURN_WIDE:
        case DEX_INS_RETURN_OBJECT:
            build_dex_ins_return_act(ins);
            break;
        case DEX_INS_CONST_4:
            build_dex_ins_const_4_act(ins);
            break;
        case DEX_INS_CONST_16:
            build_dex_ins_const_16_act(ins);
            break;
        case DEX_INS_CONST:
            build_dex_ins_const_act(ins);
            break;
        case DEX_INS_CONST_HIGH16:
            build_dex_ins_const_high16_act(ins);
            break;
        case DEX_INS_CONST_WIDE_16:
            build_dex_ins_const_wide_16_act(ins);
            break;
        case DEX_INS_CONST_WIDE_32:
            build_dex_ins_const_wide_32_act(ins);
            break;
        case DEX_INS_CONST_WIDE:
            build_dex_ins_const_wide_act(ins);
            break;
        case DEX_INS_CONST_WIDE_HIGH16:
            build_dex_ins_const_wide_high16_act(ins);
            break;
        case DEX_INS_CONST_STRING:
            build_dex_ins_const_string_act(ins);
            break;
        case DEX_INS_CONST_STRING_JUMBO:
            build_dex_ins_const_string_jumbo_act(ins);
            break;
        case DEX_INS_CONST_CLASS:
            build_dex_ins_const_class_act(ins);
            break;
        case DEX_INS_MONITOR_ENTER:
        case DEX_INS_MONITOR_EXIT:
            build_dex_ins_monitor_act(ins);
            break;
        case DEX_INS_CHECK_CAST:
            build_dex_ins_check_cast_act(ins);
            break;
        case DEX_INS_INSTANCE_OF:
            build_dex_ins_instance_of_act(ins);
            break;
        case DEX_INS_ARRAY_LENGTH:
            build_dex_ins_array_length_act(ins);
            break;
        case DEX_INS_NEW_INSTANCE:
            build_dex_ins_new_instance_act(ins);
            break;
        case DEX_INS_NEW_ARRAY:
            build_dex_ins_new_array_act(ins);
            break;
        case DEX_INS_FILLED_NEW_ARRAY:
        case DEX_INS_FILLED_NEW_ARRAY_RANGE:
        case DEX_INS_FILL_ARRAY_DATA:
        case DEX_INS_THROW:
        case DEX_INS_GOTO:
        case DEX_INS_GOTO_16:
        case DEX_INS_GOTO_32:
        case DEX_INS_PACKED_SWITCH:
        case DEX_INS_SPARSE_SWITCH:
            dex_build_ins_default_act(ins);
            break;
        case DEX_INS_CMPL_FLOAT:
        case DEX_INS_CMPG_FLOAT:
        case DEX_INS_CMPL_DOUBLE:
        case DEX_INS_CMPG_DOUBLE:
        case DEX_INS_CMP_LONG:
            build_dex_ins_cmp_kind_act(ins);
            break;
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_NE:
        case DEX_INS_IF_LT:
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GT:
        case DEX_INS_IF_LE:
        case DEX_INS_IF_EQZ:
        case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ:
        case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ:
        case DEX_INS_IF_LEZ:
            dex_build_ins_default_act(ins);
            break;
        case DEX_INS_AGET:
        case DEX_INS_AGET_WIDE:
        case DEX_INS_AGET_OBJECT:
        case DEX_INS_AGET_BOOLEAN:
        case DEX_INS_AGET_BYTE:
        case DEX_INS_AGET_CHAR:
        case DEX_INS_AGET_SHORT:
        case DEX_INS_APUT:
        case DEX_INS_APUT_WIDE:
        case DEX_INS_APUT_OBJECT:
        case DEX_INS_APUT_BOOLEAN:
        case DEX_INS_APUT_BYTE:
        case DEX_INS_APUT_CHAR:
        case DEX_INS_APUT_SHORT:
            build_dex_ins_arrayop_act(ins);
            break;
        case DEX_INS_IGET:
        case DEX_INS_IGET_WIDE:
        case DEX_INS_IGET_OBJECT:
        case DEX_INS_IGET_BOOLEAN:
        case DEX_INS_IGET_BYTE:
        case DEX_INS_IGET_CHAR:
        case DEX_INS_IGET_SHORT:
        case DEX_INS_IPUT:
        case DEX_INS_IPUT_WIDE:
        case DEX_INS_IPUT_OBJECT:
        case DEX_INS_IPUT_BOOLEAN:
        case DEX_INS_IPUT_BYTE:
        case DEX_INS_IPUT_CHAR:
        case DEX_INS_IPUT_SHORT:
            build_dex_ins_instanceop_act(ins);
            break;
        case  DEX_INS_SGET:
        case  DEX_INS_SGET_WIDE:
        case  DEX_INS_SGET_OBJECT:
        case  DEX_INS_SGET_BOOLEAN:
        case  DEX_INS_SGET_BYTE:
        case  DEX_INS_SGET_CHAR:
        case  DEX_INS_SGET_SHORT:
        case  DEX_INS_SPUT:
        case  DEX_INS_SPUT_WIDE:
        case  DEX_INS_SPUT_OBJECT:
        case  DEX_INS_SPUT_BOOLEAN:
        case  DEX_INS_SPUT_BYTE:
        case  DEX_INS_SPUT_CHAR:
        case  DEX_INS_SPUT_SHORT:
            build_dex_ins_staticop_act(ins);
            break;
        case  DEX_INS_INVOKE_VIRTUAL:
        case  DEX_INS_INVOKE_SUPER:
        case  DEX_INS_INVOKE_DIRECT:
        case  DEX_INS_INVOKE_STATIC:
        case  DEX_INS_INVOKE_INTERFACE:
            build_dex_ins_invoke_act(ins);
            break;
        case  DEX_INS_INVOKE_VIRTUAL_RANGE:
        case  DEX_INS_INVOKE_SUPER_RANGE:
        case  DEX_INS_INVOKE_DIRECT_RANGE:
        case  DEX_INS_INVOKE_STATIC_RANGE:
        case  DEX_INS_INVOKE_INTERFACE_RANGE:
            build_dex_ins_invoke_range_act(ins);
            break;
        case  DEX_INS_NEG_INT:
        case  DEX_INS_NOT_INT:
        case  DEX_INS_NEG_LONG:
        case  DEX_INS_NOT_LONG:
        case  DEX_INS_NEG_FLOAT:
        case  DEX_INS_NEG_DOUBLE:
        case  DEX_INS_INT_TO_LONG:
        case  DEX_INS_INT_TO_FLOAT:
        case  DEX_INS_INT_TO_DOUBLE:
        case  DEX_INS_LONG_TO_INT:
        case  DEX_INS_LONG_TO_FLOAT:
        case  DEX_INS_LONG_TO_DOUBLE:
        case  DEX_INS_FLOAT_TO_INT:
        case  DEX_INS_FLOAT_TO_LONG:
        case  DEX_INS_FLOAT_TO_DOUBLE:
        case  DEX_INS_DOUBLE_TO_INT:
        case  DEX_INS_DOUBLE_TO_LONG:
        case  DEX_INS_DOUBLE_TO_FLOAT:
        case  DEX_INS_INT_TO_BYTE:
        case  DEX_INS_INT_TO_CHAR:
        case  DEX_INS_INT_TO_SHORT:
        case  DEX_INS_ADD_INT:
        case  DEX_INS_SUB_INT:
        case  DEX_INS_MUL_INT:
        case  DEX_INS_DIV_INT:
        case  DEX_INS_REM_INT:
        case  DEX_INS_AND_INT:
        case  DEX_INS_OR_INT:
        case  DEX_INS_XOR_INT:
        case  DEX_INS_SHL_INT:
        case  DEX_INS_SHR_INT:
        case  DEX_INS_USHR_INT:
        case  DEX_INS_ADD_LONG:
        case  DEX_INS_SUB_LONG:
        case  DEX_INS_MUL_LONG:
        case  DEX_INS_DIV_LONG:
        case  DEX_INS_REM_LONG:
        case  DEX_INS_AND_LONG:
        case  DEX_INS_OR_LONG:
        case  DEX_INS_XOR_LONG:
        case  DEX_INS_SHL_LONG:
        case  DEX_INS_SHR_LONG:
        case  DEX_INS_USHR_LONG:
        case  DEX_INS_ADD_FLOAT:
        case  DEX_INS_SUB_FLOAT:
        case  DEX_INS_MUL_FLOAT:
        case  DEX_INS_DIV_FLOAT:
        case  DEX_INS_REM_FLOAT:
        case  DEX_INS_ADD_DOUBLE:
        case  DEX_INS_SUB_DOUBLE:
        case  DEX_INS_MUL_DOUBLE:
        case  DEX_INS_DIV_DOUBLE:
        case  DEX_INS_REM_DOUBLE:
        case  DEX_INS_ADD_INT_2ADDR:
        case  DEX_INS_SUB_INT_2ADDR:
        case  DEX_INS_MUL_INT_2ADDR:
        case  DEX_INS_DIV_INT_2ADDR:
        case  DEX_INS_REM_INT_2ADDR:
        case  DEX_INS_AND_INT_2ADDR:
        case  DEX_INS_OR_INT_2ADDR:
        case  DEX_INS_XOR_INT_2ADDR:
        case  DEX_INS_SHL_INT_2ADDR:
        case  DEX_INS_SHR_INT_2ADDR:
        case  DEX_INS_USHR_INT_2ADDR:
        case  DEX_INS_ADD_LONG_2ADDR:
        case  DEX_INS_SUB_LONG_2ADDR:
        case  DEX_INS_MUL_LONG_2ADDR:
        case  DEX_INS_DIV_LONG_2ADDR:
        case  DEX_INS_REM_LONG_2ADDR:
        case  DEX_INS_AND_LONG_2ADDR:
        case  DEX_INS_OR_LONG_2ADDR:
        case  DEX_INS_XOR_LONG_2ADDR:
        case  DEX_INS_SHL_LONG_2ADDR:
        case  DEX_INS_SHR_LONG_2ADDR:
        case  DEX_INS_USHR_LONG_2ADDR:
        case  DEX_INS_ADD_FLOAT_2ADDR:
        case  DEX_INS_SUB_FLOAT_2ADDR:
        case  DEX_INS_MUL_FLOAT_2ADDR:
        case  DEX_INS_DIV_FLOAT_2ADDR:
        case  DEX_INS_REM_FLOAT_2ADDR:
        case  DEX_INS_ADD_DOUBLE_2ADDR:
        case  DEX_INS_SUB_DOUBLE_2ADDR:
        case  DEX_INS_MUL_DOUBLE_2ADDR:
        case  DEX_INS_DIV_DOUBLE_2ADDR:
        case  DEX_INS_REM_DOUBLE_2ADDR:
        case  DEX_INS_ADD_INT_LIT16:
        case  DEX_INS_RSUB_INT:
        case  DEX_INS_MUL_INT_LIT16:
        case  DEX_INS_DIV_INT_LIT16:
        case  DEX_INS_REM_INT_LIT16:
        case  DEX_INS_AND_INT_LIT16:
        case  DEX_INS_OR_INT_LIT16:
        case  DEX_INS_XOR_INT_LIT16:
        case  DEX_INS_ADD_INT_LIT8:
        case  DEX_INS_RSUB_INT_LIT8:
        case  DEX_INS_MUL_INT_LIT8:
        case  DEX_INS_DIV_INT_LIT8:
        case  DEX_INS_REM_INT_LIT8:
        case  DEX_INS_AND_INT_LIT8:
        case  DEX_INS_OR_INT_LIT8:
        case  DEX_INS_XOR_INT_LIT8:
        case  DEX_INS_SHL_INT_LIT8:
        case  DEX_INS_SHR_INT_LIT8:
        case  DEX_INS_USHR_INT_LIT8:
            build_dex_ins_op_act(ins);
            break;
        case DEX_INS_INVOKE_POLYMORPHIC:
        case DEX_INS_INVOKE_POLYMORPHIC_RANGE:
        case DEX_INS_INVOKE_CUSTOM:
        case DEX_INS_INVOKE_CUSTOM_RANGE:
            dex_build_ins_default_act(ins);
            break;
        case DEX_INS_CONST_METHOD_HANDLE:
        case DEX_INS_CONST_METHOD_TYPE:
            dex_build_ins_default_act(ins);
            break;
        default:
            dex_build_ins_default_act(ins);
            break;
    }

    for (size_t i = 0; bitset_next_set_bit(ins->defs, &i) ; i++) {
        jd_val *val = ins->stack_out->local_vars[i];
        dex_variable_name(ins->method, ins, val, i);
    }
}

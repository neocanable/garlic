
/**
 * instructions that push data to operand stack are have action
 * push0/push1/push2 ....
 * swap, dup, pop are dynamic action
 **/
#include "parser/class/class_tools.h"
#include "common/str_tools.h"
#include "jvm_ins.h"
#include "decompiler/klass.h"
#include "decompiler/descriptor.h"
#include "decompiler/stack.h"
#include "jvm_simulator.h"

static inline string get_sname(jd_ins *ins, string full)
{
    string sname = class_simple_name(full);
    jsource_file *jf = ins->method->jfile;
    class_import(jf, full);
    return sname;
}

static void primitive_val_type(jd_ins *ins, jd_val *val, jd_var_types type)
{
    val->type = type;
    val->ins = ins;
    switch (type) {
        case JD_VAR_INT_T:
            val->data->cname = g_str_int;
            break;
        case JD_VAR_LONG_T:
            val->data->cname = g_str_long;
            break;
        case JD_VAR_FLOAT_T:
            val->data->cname = g_str_float;
            break;
        case JD_VAR_DOUBLE_T:
            val->data->cname = g_str_double;
            break;
        case JD_VAR_REFERENCE_T:
            val->data->cname = g_str_Object;
            break;
        default:
            break;
    }
}

static void build_jvm_ins_ldc_action(jd_ins *ins)
{
    jd_method *m = ins->method;
    jd_val *push0 = ins->stack_out->vals[0];
    push0->ins = ins;

    jclass_file *jc = m->meta;
    jcp_info *info = NULL;
    if (jvm_ins_is_ldc2_w(ins)) //ldc2_w
        info = pool_item(jc, be16toh(ins->param[0] << 8 | ins->param[1]));
    else if (jvm_ins_is_ldc_w(ins))
        info = pool_item(jc, be16toh(ins->param[0] << 8 | ins->param[1]));
    else if (jvm_ins_is_ldc(ins))
        info = pool_u1_item(jc, ins->param[0]);

    switch (info->tag) {
        case CONST_LONG_TAG: {
            push0->type = JD_VAR_LONG_T;
            push0->data->primitive = make_obj(jd_primitive_union);
            push0->data->cname = g_str_long;
            push0->data->primitive->long_val = get_const_long(info);
            break;
        }
        case CONST_DOUBLE_TAG: {
            push0->type = JD_VAR_DOUBLE_T;
            push0->data->primitive = make_obj(jd_primitive_union);
            push0->data->cname = g_str_double;
            push0->data->primitive->double_val = get_const_double(info);
            break;
        }
        case CONST_STRING_TAG: {
            push0->type = JD_VAR_REFERENCE_T;
            push0->data->cname = g_str_String;
            push0->data->val = get_const_string(jc, info);
            break;
        }
        case CONST_INTEGER_TAG: {
            push0->type = JD_VAR_INT_T;
            push0->data->primitive = make_obj(jd_primitive_union);
            push0->data->primitive->int_val = get_const_int(info);
            push0->data->cname = g_str_int;
            break;
        }
        case CONST_FLOAT_TAG: {
            push0->type = JD_VAR_FLOAT_T;
            push0->data->primitive = make_obj(jd_primitive_union);
            push0->data->primitive->float_val = get_const_float(info);
            push0->data->cname = g_str_float;
            break;
        }
        case CONST_CLASS_TAG: {
            push0->type = JD_VAR_REFERENCE_T;
            push0->data->val = get_class_name(jc, info);
            push0->data->cname = g_str_Class;
            break;
        }
        case CONST_METHODHANDLE_TAG: {
            jconst_method_handle *handle = info->info->method_handle;
            push0->type = JD_VAR_REFERENCE_T;
            u1 kind_index = handle->reference_kind;
            u2 reference_index = handle->reference_index;
            if (kind_index >= 1 && kind_index <= 4) {
                // field
                jcp_info *f = pool_item(jc, reference_index);
                string field_class = get_field_class(jc, f);
                push0->data->cname = get_sname(ins, field_class);
                push0->data->val = get_field_name(jc, f);

            }
            else if (kind_index == 5 || kind_index == 8) {
                // m ref
                jcp_info *ref = pool_item(jc, reference_index);
                string method_class = get_method_class(jc, ref);
                push0->data->cname = get_sname(ins, method_class);
                push0->data->val = get_method_name(jc, ref);
            }
            else if (kind_index == 6 || kind_index == 7) {
                // m ref or interface m ref
                jcp_info *ref = pool_item(jc, reference_index);
                string method_class = get_method_class(jc, ref);
                push0->data->cname = get_sname(ins, method_class);
                push0->data->val = get_method_name(jc, ref);
            }
            else if (kind_index == 9 ) {
                // interface m ref
                jcp_info *ref = pool_item(jc, reference_index);
                string method_class = get_method_class(jc, ref);
                push0->data->cname = get_sname(ins, method_class);
                push0->data->val = get_method_name(jc, ref);
            }
            break;
        }
        default:
            break;
    }
}

static void build_jvm_ins_newarray_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_REFERENCE_T;
    push0->ins = ins;
    if (jvm_ins_is_newarray(ins)) { // newarray
        uint8_t atype = ins->param[0];
        switch (atype) {
            case 4:
                push0->data->cname = str_dup("boolean[]");
                break;
            case 5:
                push0->data->cname = str_dup("char[]");
                break;
            case 6:
                push0->data->cname = str_dup("float[]");
                break;
            case 7:
                push0->data->cname = str_dup("double[]");
                break;
            case 8:
                push0->data->cname = str_dup("byte[]");
                break;
            case 9:
                push0->data->cname = str_dup("short[]");
                break;
            case 10:
                push0->data->cname = str_dup("int[]");
                break;
            case 11:
                push0->data->cname = str_dup("long[]");
                break;
            default:
                break;
        }
    }
    else { // anewarray
        u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
        jcp_info *info = pool_item(ins->method->meta, index);
        string class_name = get_class_name(ins->method->meta, info);
        push0->data->cname = get_sname(ins, class_name);
    }

}

static void build_jvm_ins_multianewarray_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_REFERENCE_T;
    push0->ins = ins;
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *info = pool_item(ins->method->meta, index);
    string class_name = get_class_name(ins->method->meta, info);
    string fname = class_full_name(class_name);
    push0->data->cname = get_sname(ins, fname);
}

static void build_jvm_ins_xconst_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->ins = ins;
    push0->type = type;
    push0->data->primitive = make_obj(jd_primitive_union);
    jd_primitive_union *pri = push0->data->primitive;
    switch (type) {
        case JD_VAR_INT_T: {
            pri->int_val = jvm_ins_iconst_value(ins);
            push0->data->cname = g_str_int;
            break;
        }
        case JD_VAR_LONG_T: {
            pri->long_val = jvm_ins_lconst_value(ins);
            push0->data->cname = g_str_long;
            break;
        }
        case JD_VAR_FLOAT_T: {
            pri->float_val = jvm_ins_fconst_value(ins);
            push0->data->cname = g_str_float;
            break;
        }
        case JD_VAR_DOUBLE_T: {
            pri->double_val = jvm_ins_dconst_value(ins);
            push0->data->cname = g_str_double;
            break;
        }
        case JD_VAR_REFERENCE_T: {
            push0->data->cname = g_str_Object;
            push0->data->val = g_str_null;
            break;
        }
        default:
            break;
    }
}

static void build_jvm_ins_xload_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->ins = ins;
    primitive_val_type(ins, push0, type);
}

static void build_jvm_ins_xaload_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    primitive_val_type(ins, push0, type);
}

static void build_jvm_ins_xstore_action(jd_ins *ins)
{
}

static void build_jvm_ins_cmp_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    primitive_val_type(ins, push0, JD_VAR_INT_T);
}

static void build_jvm_ins_cast_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    primitive_val_type(ins, push0, type);
}

static void build_jvm_ins_operator_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    primitive_val_type(ins, push0, type);
}

static void build_jvm_ins_logical_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    primitive_val_type(ins, push0, type);
}

static void build_jvm_ins_shift_action(jd_ins *ins, jd_var_types type)
{
    jd_val *push0 = ins->stack_out->vals[0];
    primitive_val_type(ins, push0, type);
}

static void build_jvm_ins_if_action(jd_ins *ins)
{
}

static void build_jvm_ins_xreturn_action(jd_ins *ins)
{
}

static void build_jvm_ins_get_static_action(jd_ins *ins)
{
    jd_method *m = ins->method;
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *info = pool_item(m->meta, index);

    jd_val *push0 = ins->stack_out->vals[0];
    push0->data->val = get_field_name(m->meta, info);
    string descriptor = get_field_descriptor(m->meta, info);
    string fname = class_full_name(descriptor);
    push0->data->cname = get_sname(ins, fname);
    push0->ins = ins;
    push0->type = descriptor_data_type(descriptor);
}

static void build_jvm_ins_put_static_action(jd_ins *ins)
{
}

static void build_jvm_ins_get_field_action(jd_ins *ins)
{
    jd_method *m = ins->method;
    uint16_t field_index = ins->param[0] << 8 | ins->param[1];
    jcp_info *info = pool_item(m->meta, be16toh(field_index));

    jd_val *push0 = ins->stack_out->vals[0];
    string descriptor = get_field_descriptor(m->meta, info);
    string fname = class_full_name(descriptor);
    push0->data->cname = get_sname(ins, fname);
    push0->type = descriptor_data_type(descriptor);
    push0->ins = ins;
    push0->data->val = get_field_name(m->meta, info);
}

static void build_jvm_ins_put_field_action(jd_ins *ins)
{
}

static void build_jvm_ins_invoke_action(jd_ins *ins)
{
    jd_descriptor *descriptor = jvm_invoke_descriptor(ins);

    if (STR_EQL(descriptor->str_return, "V"))
        return;
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = descriptor_data_type(descriptor->str_return);
    push0->ins = ins;
    string fname = class_full_name(descriptor->str_return);
    push0->data->cname = get_sname(ins, fname);
}

static void build_jvm_ins_new_action(jd_ins *ins)
{
    jd_method *m = ins->method;
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_REFERENCE_T;
    push0->ins = ins;
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *class_info = pool_item(m->meta, index);
    string full_name = get_class_name(m->meta, class_info);
    push0->data->cname = get_sname(ins, full_name);
}

static void build_jvm_ins_array_length_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_INT_T;
    push0->data->cname = g_str_int;
    push0->ins = ins;
}

static void build_jvm_ins_athrow_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_REFERENCE_T;
    push0->data->cname = g_str_Throwable;
    push0->ins = ins;
}

static void build_jvm_ins_checkcast_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_REFERENCE_T;
    push0->ins = ins;
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *info = pool_item(ins->method->meta, index);
    string full = get_class_name(ins->method->meta, info);
    push0->data->cname = get_sname(ins, full);
}

static void build_jvm_ins_instanceof_action(jd_ins *ins)
{
    jd_val *push0 = ins->stack_out->vals[0];
    push0->type = JD_VAR_INT_T;
    push0->ins = ins;
    push0->data->cname = g_str_boolean;
}

static void dup_copy_action(jd_ins *ins, int to_index, int copy)
{
    jd_stack *stack_out = make_obj(jd_stack);
    ins->stack_out = stack_out;
    stack_out->depth = ins->stack_in->depth + copy;
    stack_out->vals = make_obj_arr(jd_val*, stack_out->depth);

    jd_val *src = NULL;
    for (int i = 0; i < copy; ++i) {
        jd_val *v = ins->stack_in->vals[i];
        stack_out->vals[to_index + i] = v;
    }

    for (int i = 0; i < ins->stack_in->depth; ++i) {
        int idx = (i >= to_index ? i + copy : i);
        stack_out->vals[idx] = ins->stack_in->vals[i];
    }


    stack_clone_local_variables(stack_out, ins->stack_in);
}

int dupped_ins_count(jd_ins *ins)
{
    switch (ins->code) {
        case INS_DUP:
            return 1;
        case INS_DUP_X1:
            return 1;
        case INS_DUP_X2:
            return 1;
        case INS_DUP2: {
            jd_val *item0 = ins->stack_in->vals[0];
            return jd_stack_val_is_compute_category2(item0) ? 1 : 2;
        }
        case INS_DUP2_X1: {
            jd_val *item0 = ins->stack_in->vals[0];
            jd_val *item1 = ins->stack_in->vals[1];
            if (jd_stack_val_is_compute_category2(item0) &&
                jd_stack_val_is_compute_category1(item1))
                return 1;
            else
                return 2;
        }
        case INS_DUP2_X2: {
            jd_val *item0 = ins->stack_in->vals[0];
            jd_val *item1 = ins->stack_in->vals[1];
            if (jd_stack_val_is_compute_category2(item0) &&
                jd_stack_val_is_compute_category2(item1))
                return 1;
            jd_val *item2 = ins->stack_in->vals[2];
            if (jd_stack_val_is_compute_category2(item0) &&
                jd_stack_val_is_compute_category1(item1) &&
                jd_stack_val_is_compute_category1(item2))
                return 2;
            return 2;
        }
        default:
            return 0;
    }
}

void build_jvm_ins_dup_action(jd_ins *ins)
{
    dup_copy_action(ins, 0, 1);
}

void build_jvm_ins_dup_x1_action(jd_ins *ins)
{
    dup_copy_action(ins, 2, 1);
}

void build_jvm_ins_dup_x2_action(jd_ins *ins)
{

    jd_val *item0 = ins->stack_in->vals[0];
    jd_val *item1 = ins->stack_in->vals[1];
    if (jd_stack_val_is_compute_category2(item1) &&
        jd_stack_val_is_compute_category1(item0)) {
        // ..., value2, value1 →
        // ..., value1, value2, value1
        dup_copy_action(ins, 2, 1);
    }
    else {
        // ..., value3, value2, value1 →
        // ..., value1, value3, value2, value1
        dup_copy_action(ins, 3, 1);
    }
}

void build_jvm_ins_dup2_action(jd_ins *ins)
{
    jd_val *item0 = ins->stack_in->vals[0];
    if (jd_stack_val_is_compute_category2(item0)) {
        // ..., value →
        // ..., value, value
        dup_copy_action(ins, 0, 1);
    }
    else {
        // ..., value2, value1 →
        // ..., value2, value1, value2, value1
        dup_copy_action(ins, 2, 2);
    }
}

void build_jvm_ins_dup2_x1_action(jd_ins *ins)
{
    jd_val *item0 = ins->stack_in->vals[0];
    jd_val *item1 = ins->stack_in->vals[1];
    if (jd_stack_val_is_compute_category2(item0) &&
        jd_stack_val_is_compute_category1(item1)) {
        // ..., value2, value1 →
        // ..., value1, value2, value1
        dup_copy_action(ins, 2, 1);
    }
    else {
        // ..., value3, value2, value1 →
        // ..., value2, value1, value3, value2, value1
        dup_copy_action(ins, 2, 2);
    }
}

void build_jvm_ins_dup2_x2_action(jd_ins *ins)
{
    jd_val *item0 = ins->stack_in->vals[0];
    jd_val *item1 = ins->stack_in->vals[1];

    if (jd_stack_val_is_compute_category2(item0) &&
        jd_stack_val_is_compute_category2(item1)) {
        // ..., value2, value1 →
        //..., value1, value2, value1
        dup_copy_action(ins,  2, 1);
        return;
    }
    jd_val *item2 = ins->stack_in->vals[2];

    if (jd_stack_val_is_compute_category2(item0) &&
        jd_stack_val_is_compute_category1(item1) &&
        jd_stack_val_is_compute_category1(item2)) {
        // ..., value3, value2, value1 →
        // ..., value1, value3, value2, value1
        build_jvm_ins_dup_x1_action(ins);
        return;
    }

    if (jd_stack_val_is_compute_category1(item0) &&
        jd_stack_val_is_compute_category1(item1) &&
        jd_stack_val_is_compute_category2(item2)) {
        // ..., value3, value2, value1 →
        // ..., value2, value1, value3, value2, value1
        dup_copy_action(ins, 3, 2);
        return;
    }

    // ..., value4, value3, value2, value1 →
    // ..., value2, value1, value4, value3, value2, value1
    dup_copy_action(ins, 4, 2);

}

void build_jvm_ins_swap_action(jd_ins *ins)
{
    dup_copy_action(ins,  0, 0);

    jd_stack *stack = ins->stack_out;
    jd_val *item0   = stack->vals[0];
    jd_val *item1   = stack->vals[1];
    jd_val *tmp     = item0;
    item0           = item1;
    item1           = tmp;
}

void perform_pop_action_with_count(jd_ins *ins, int pops)
{
    jd_stack *stack_out = make_obj(jd_stack);
    ins->stack_out = stack_out;
    stack_out->depth = ins->stack_in->depth - pops;
    if (stack_out->depth > 0) {
        stack_out->vals = make_obj_arr(jd_val*, stack_out->depth);
        for (int i = 0; i < stack_out->depth; ++i)
            stack_out->vals[i] = ins->stack_in->vals[i+pops];
    }
    stack_clone_local_variables(stack_out, ins->stack_in);
}

void build_jvm_ins_pop_action(jd_ins *ins)
{
    perform_pop_action_with_count(ins, 1);
}

void build_jvm_ins_pop2_action(jd_ins *ins)
{
    jd_val *item0 = ins->stack_in->vals[0];
    if (jd_stack_val_is_compute_category2(item0)) {
        // ..., value →
        // ...
        perform_pop_action_with_count(ins, 1);
    }
    else {
        // ..., value2, value1 →
        // ..., value1
        perform_pop_action_with_count(ins, 2);
    }
}

void instruction_stack_action(jd_ins *ins)
{
    u1 *parameters = ins->param;

    switch (ins->code) {
        case INS_NOP: //nop
            break;
        case INS_ACONST_NULL: //aconst_null
            build_jvm_ins_xconst_action(ins, JD_VAR_REFERENCE_T);
            break;
        case INS_ICONST_M1: //iconst_m1
        case INS_ICONST_0: //iconst_0
        case INS_ICONST_1: //iconst_1
        case INS_ICONST_2: //iconst_2
        case INS_ICONST_3: //iconst_3
        case INS_ICONST_4: //iconst_4
        case INS_ICONST_5: //iconst_5
        case INS_BIPUSH: //bipush
        case INS_SIPUSH: //sipush
            build_jvm_ins_xconst_action(ins, JD_VAR_INT_T);
            break;
        case INS_LCONST_0: //lconst_0
        case INS_LCONST_1: //lconst_1
            build_jvm_ins_xconst_action(ins, JD_VAR_LONG_T);
            break;
        case INS_FCONST_0: //fconst_0
        case INS_FCONST_1: //fconst_1
        case INS_FCONST_2: //fconst_2
            build_jvm_ins_xconst_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_DCONST_0: //dconst_0
        case INS_DCONST_1: //dconst_1
            build_jvm_ins_xconst_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_LDC: //ldc
        case INS_LDC_W: //ldc_w
        case INS_LDC2_W: //ldc2_w
            build_jvm_ins_ldc_action(ins);
            break;
        case INS_ILOAD: //iload
        case INS_ILOAD_0: //iload_0
        case INS_ILOAD_1: //iload_1
        case INS_ILOAD_2: //iload_2
        case INS_ILOAD_3: //iload_3
            build_jvm_ins_xload_action(ins, JD_VAR_INT_T);
            break;
        case INS_LLOAD: //lload
        case INS_LLOAD_0: //lload_0
        case INS_LLOAD_1: //lload_1
        case INS_LLOAD_2: //lload_2
        case INS_LLOAD_3: //lload_3
            build_jvm_ins_xload_action(ins, JD_VAR_LONG_T);
            break;
        case INS_FLOAD: //fload
        case INS_FLOAD_0: //fload_0
        case INS_FLOAD_1: //fload_1
        case INS_FLOAD_2: //fload_2
        case INS_FLOAD_3: //fload_3
            build_jvm_ins_xload_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_DLOAD: //dload
        case INS_DLOAD_0: //dload_0
        case INS_DLOAD_1: //dload_1
        case INS_DLOAD_2: //dload_2
        case INS_DLOAD_3: //dload_3
            build_jvm_ins_xload_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_ALOAD: //aload
        case INS_ALOAD_0: //aload_0
        case INS_ALOAD_1: //aload_1
        case INS_ALOAD_2: //aload_2
        case INS_ALOAD_3: //aload_3
            build_jvm_ins_xload_action(ins, JD_VAR_REFERENCE_T);
            break;
        case INS_BALOAD: //baload
        case INS_CALOAD: //caload
        case INS_SALOAD: //saload
        case INS_IALOAD: //iaload
            build_jvm_ins_xaload_action(ins, JD_VAR_INT_T);
            break;
        case INS_LALOAD: //laload
            build_jvm_ins_xaload_action(ins, JD_VAR_LONG_T);
            break;
        case INS_FALOAD: //faload
            build_jvm_ins_xaload_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_DALOAD: //daload
            build_jvm_ins_xaload_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_AALOAD: //aaload
            build_jvm_ins_xaload_action(ins, JD_VAR_REFERENCE_T);
            break;
        case INS_ISTORE: //istore
        case INS_ISTORE_0: //istore_0
        case INS_ISTORE_1: //istore_1
        case INS_ISTORE_2: //istore_2
        case INS_ISTORE_3: //istore_3
        case INS_LSTORE: //lstore
        case INS_LSTORE_0: //lstore_0
        case INS_LSTORE_1: //lstore_1
        case INS_LSTORE_2: //lstore_2
        case INS_LSTORE_3: //lstore_3
        case INS_FSTORE: //fstore
        case INS_FSTORE_0: //fstore_0
        case INS_FSTORE_1: //fstore_1
        case INS_FSTORE_2: //fstore_2
        case INS_FSTORE_3: //fstore_3
        case INS_DSTORE: //dstore
        case INS_DSTORE_0: //dstore_0
        case INS_DSTORE_1: //dstore_1
        case INS_DSTORE_2: //dstore_2
        case INS_DSTORE_3: //dstore_3
        case INS_ASTORE: //astore
        case INS_ASTORE_0: //astore_0
        case INS_ASTORE_1: //astore_1
        case INS_ASTORE_2: //astore_2
        case INS_ASTORE_3: //astore_3
        case INS_IASTORE: //iastore
        case INS_LASTORE: //lastore
        case INS_FASTORE: //fastore
        case INS_DASTORE: //dastore
        case INS_AASTORE: //aastore
        case INS_BASTORE: //bastore
        case INS_CASTORE: //castore
        case INS_SASTORE: //sastore
            build_jvm_ins_xstore_action(ins);
            break;
        case INS_POP: //pop
        case INS_POP2: //pop2
        case INS_DUP: //dup
        case INS_DUP_X1: //dup_x1
        case INS_DUP_X2: //dup_x2
        case INS_DUP2: //dup2
        case INS_DUP2_X1: //dup2_x1
        case INS_DUP2_X2: //dup2_x2
        case INS_SWAP: //swap
            break;
        case INS_IADD: //iadd
        case INS_ISUB: //isub
        case INS_IMUL: //imul
        case INS_IDIV: //idiv
        case INS_IREM: //irem
        case INS_IAND: //iand
        case INS_IOR: //ior
        case INS_IXOR: //ixor
            build_jvm_ins_operator_action(ins, JD_VAR_INT_T);
            break;
        case INS_LADD: //ladd
        case INS_LSUB: //lsub
        case INS_LMUL: //lmul
        case INS_LDIV: //ldiv
        case INS_LREM: //lrem
        case INS_LAND: //land
        case INS_LOR: //lor
        case INS_LXOR: //lxor
            build_jvm_ins_operator_action(ins, JD_VAR_LONG_T);
            break;
        case INS_FADD: //fadd
        case INS_FSUB: //fsub
        case INS_FMUL: //fmul
        case INS_FDIV: //fdiv
        case INS_FREM: //frem
            build_jvm_ins_operator_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_DADD: //dadd
        case INS_DSUB: //dsub
        case INS_DMUL: //dmul
        case INS_DDIV: //ddiv
        case INS_DREM: //drem
            build_jvm_ins_operator_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_INEG: //ineg
            build_jvm_ins_logical_action(ins, JD_VAR_INT_T);
            break;
        case INS_LNEG: //lneg
            build_jvm_ins_logical_action(ins, JD_VAR_LONG_T);
            break;
        case INS_FNEG: //fneg
            build_jvm_ins_logical_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_DNEG: //dneg
            build_jvm_ins_logical_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_ISHL: //ishl
        case INS_ISHR: //ishr
        case INS_IUSHR: //iushr
            build_jvm_ins_shift_action(ins, JD_VAR_INT_T);
            break;
        case INS_LSHL: //lshl
        case INS_LSHR: //lshr
        case INS_LUSHR: //lushr
            build_jvm_ins_shift_action(ins, JD_VAR_LONG_T);
            break;
        case INS_IINC: //iinc
            break;
        case INS_I2L: //i2l
            build_jvm_ins_cast_action(ins, JD_VAR_LONG_T);
            break;
        case INS_I2F: //i2f
            build_jvm_ins_cast_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_I2D: //i2d
            build_jvm_ins_cast_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_L2I: //l2i
            build_jvm_ins_cast_action(ins, JD_VAR_INT_T);
            break;
        case INS_L2F: //l2f
            build_jvm_ins_cast_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_L2D: //l2d
            build_jvm_ins_cast_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_F2I: //f2i
            build_jvm_ins_cast_action(ins, JD_VAR_INT_T);
            break;
        case INS_F2L: //f2l
            build_jvm_ins_cast_action(ins, JD_VAR_LONG_T);
            break;
        case INS_F2D: //f2d
            build_jvm_ins_cast_action(ins, JD_VAR_DOUBLE_T);
            break;
        case INS_D2I: //d2i
            build_jvm_ins_cast_action(ins, JD_VAR_INT_T);
            break;
        case INS_D2L: //d2l
            build_jvm_ins_cast_action(ins, JD_VAR_LONG_T);
            break;
        case INS_D2F: //d2f
            build_jvm_ins_cast_action(ins, JD_VAR_FLOAT_T);
            break;
        case INS_I2B: //i2b
            build_jvm_ins_cast_action(ins, JD_VAR_INT_T);
            break;
        case INS_I2C: //i2c
            build_jvm_ins_cast_action(ins, JD_VAR_INT_T);
            break;
        case INS_I2S: //i2s
            build_jvm_ins_cast_action(ins, JD_VAR_INT_T);
            break;
        case INS_LCMP: //lcmp
        case INS_FCMPL: //fcmpl
        case INS_FCMPG: //fcmpg
        case INS_DCMPL: //dcmpl
        case INS_DCMPG: //dcmpg
            build_jvm_ins_cmp_action(ins);
            break;
        case INS_IFEQ: //ifeq
        case INS_IFNE: //ifne
        case INS_IFLT: //iflt
        case INS_IFGE: //ifge
        case INS_IFGT: //ifgt
        case INS_IFLE: //ifle
        case INS_IF_ICMPEQ: //if_icmpeq
        case INS_IF_ICMPNE: //if_icmpne
        case INS_IF_ICMPLT: //if_icmplt
        case INS_IF_ICMPGE: //if_icmpge
        case INS_IF_ICMPGT: //if_icmpgt
        case INS_IF_ICMPLE: //if_icmple
        case INS_IF_ACMPEQ: //if_acmpeq
        case INS_IF_ACMPNE: //if_acmpne
            build_jvm_ins_if_action(ins);
            break;
        case INS_GOTO: //goto
        case INS_JSR: //jsr
        case INS_RET: //ret
            //unsupport
            break;
        case INS_TABLESWITCH: //tableswitch
        case INS_LOOKUPSWITCH: //lookupswitch
        {
            break;
        }
        case INS_IRETURN: //ireturn
        case INS_LRETURN: //lreturn
        case INS_FRETURN: //freturn
        case INS_DRETURN: //dreturn
        case INS_ARETURN: //areturn
            build_jvm_ins_xreturn_action(ins);
            break;
        case INS_RETURN: //return
            break;
        case INS_GETSTATIC: //getstatic
            build_jvm_ins_get_static_action(ins);
            break;
        case INS_PUTSTATIC: //putstatic
            build_jvm_ins_put_static_action(ins);
            break;
        case INS_GETFIELD: //getfield
            build_jvm_ins_get_field_action(ins);
            break;
        case INS_PUTFIELD: //putfield
            build_jvm_ins_put_field_action(ins);
            break;
        case INS_INVOKEVIRTUAL: //invokevirtual
        case INS_INVOKESPECIAL: //invokespecial
        case INS_INVOKESTATIC: //invokestatic
        case INS_INVOKEINTERFACE: //invokeinterface
        case INS_INVOKEDYNAMIC: //invokedynamic
            build_jvm_ins_invoke_action(ins);
            break;
        case INS_NEW: //new
            build_jvm_ins_new_action(ins);
            break;
        case INS_NEWARRAY: //newarray
        case INS_ANEWARRAY: //anewarray
            build_jvm_ins_newarray_action(ins);
            break;
        case INS_ARRAYLENGTH: //arraylength
            build_jvm_ins_array_length_action(ins);
            break;
        case INS_ATHROW: //athrow
            build_jvm_ins_athrow_action(ins);
            break;
        case INS_CHECKCAST: //checkcast
            build_jvm_ins_checkcast_action(ins);
            break;
        case INS_INSTANCEOF: //instanceof
            build_jvm_ins_instanceof_action(ins);
            break;
        case INS_MONITORENTER: //monitorenter
        case INS_MONITOREXIT: //monitorexit
            break;
        case INS_WIDE: //wide
        {
            u1 opcode = parameters[0];
            switch (opcode) {
                case INS_ILOAD: //iload
                    build_jvm_ins_xload_action(ins, JD_VAR_INT_T);
                    break;
                case INS_LLOAD: //lload
                    build_jvm_ins_xload_action(ins, JD_VAR_LONG_T);
                    break;
                case INS_FLOAD: //fload
                    build_jvm_ins_xload_action(ins, JD_VAR_FLOAT_T);
                    break;
                case INS_DLOAD: //dload
                    build_jvm_ins_xload_action(ins, JD_VAR_DOUBLE_T);
                    break;
                case INS_ALOAD: //aload
                    build_jvm_ins_xload_action(ins, JD_VAR_REFERENCE_T);
                    break;
                case INS_ISTORE: //istore
                case INS_LSTORE: //lstore
                case INS_FSTORE: //fstore
                case INS_DSTORE: //dstore
                case INS_ASTORE: //astore
                    build_jvm_ins_xstore_action(ins);
                    break;
                case INS_IINC:
                    break;
                default:
                    break;
            }
            break;
        }
        case INS_MULTIANEWARRAY: //multianewarray
            build_jvm_ins_multianewarray_action(ins);
            break;
        case INS_IFNULL: //ifnull
        case INS_IFNONNULL: //ifnonnull
            build_jvm_ins_if_action(ins);
            break;
        case INS_GOTO_W: //goto_w
            break;
        case INS_JSR_W: //jsr_w
            break;
        default:
            break;
    }
}

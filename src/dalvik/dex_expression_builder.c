#include "dalvik/dex_ins.h"
#include "dalvik/dex_meta_helper.h"
#include "dalvik/dex_lambda.h"

#include "decompiler/instruction.h"
#include "decompiler/stack.h"
#include "decompiler/klass.h"
#include "decompiler/descriptor.h"
#include "decompiler/expression.h"

#include "common/str_tools.h"
#include "parser/dex/dex.h"
#include "dex_decompile.h"
#include "dex_class.h"

static void dex_copy_block(jd_dex_ins *copy);

int dex_store_slot(jd_dex_ins *ins)
{
    size_t i = 0;
    bitset_next_set_bit(ins->defs, &i);
    return (int)i;
}

static void dex_build_assignment(jd_exp *exp, jd_dex_ins *ins)
{
    exp->data = make_obj(jd_exp_store);
    exp->type = JD_EXPRESSION_STORE;
    jd_exp_store *store = exp->data;
    store->list = make_exp_list(2);
    jd_exp *left = &store->list->args[0];
    jd_exp *right = &store->list->args[1];
    left->ins = ins;
    right->ins = ins;

    left->type = JD_EXPRESSION_LOCAL_VARIABLE;
    int slot = dex_store_slot(ins);
    if (slot == -1)
        abort();
    jd_val *val = ins->stack_out->local_vars[slot];
    left->data = val;
    val->stack_var->def_count ++;
}

jd_exp* get_store_right(jd_exp *store)
{
    jd_exp_store *exp_store = store->data;
    return &exp_store->list->args[1];
}

static void dex_local_variable_exp(jd_exp *exp, jd_val *val)
{
    exp->type = JD_EXPRESSION_LOCAL_VARIABLE;
    exp->data = val;
    val->stack_var->use_count ++;
}

static void dex_move_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    u8 slot = dex_ins_parameter(ins, 1);
    jd_val *val = ins->stack_out->local_vars[slot];
    dex_local_variable_exp(right, val);
}

static void dex_return_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_RETURN;
    jd_exp_return *exp_return = make_obj(jd_exp_return);
    exp->data = exp_return;
    if (!dex_ins_is_return_void(ins)) {
        exp_return->list = make_exp_list(1);
        jd_exp *rval_exp = &exp_return->list->args[0];
        rval_exp->ins = ins;
        u8 slot = dex_ins_parameter(ins, 0);
        jd_val *val = ins->stack_in->local_vars[slot];
        dex_local_variable_exp(rval_exp, val);
    }
    else
        exp_return->list = make_obj(jd_exp_list);
}

static void dex_const_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->type = JD_EXPRESSION_CONST;
    jd_exp_const *const_exp = make_obj(jd_exp_const);
    u8 slot = dex_ins_parameter(ins, 0);
    const_exp->val = ins->stack_out->local_vars[slot];
    right->data = const_exp;
}

static void dex_monitor_expression(jd_exp *exp, jd_dex_ins *ins)
{
    if (dex_ins_is_monitor_enter(ins)) {
        exp->type = JD_EXPRESSION_MONITOR_ENTER;
        exp->data = make_obj(jd_exp_monitorenter);
        jd_exp_monitorenter *menter = exp->data;
        menter->list = make_exp_list(1);
        jd_exp *e = &menter->list->args[0];
        u8 slot = dex_ins_parameter(ins, 0);
        jd_val *val = ins->stack_in->local_vars[slot];
        dex_local_variable_exp(e, val);
    }
    else {
        exp->type = JD_EXPRESSION_MONITOR_EXIT;
        exp->data = make_obj(jd_exp_monitorexit);
        jd_exp_monitorexit *mexit = exp->data;
        mexit->list = make_exp_list(1);
        u8 slot = dex_ins_parameter(ins, 0);
        jd_exp *e = &mexit->list->args[0];
        jd_val *val = ins->stack_in->local_vars[slot];
        dex_local_variable_exp(e, val);
    }
}

static void dex_check_cast_expression(jd_exp *exp, jd_dex_ins *ins)
{
    build_empty_expression(exp, ins);
}

static void dex_instance_of_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->type = JD_EXPRESSION_INSTANCEOF;
    right->data = make_obj(jd_exp_instanceof);
    jd_exp_instanceof *instanceof = right->data;
    instanceof->list = make_exp_list(1);
    jd_exp *first = &instanceof->list->args[0];

    // u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u2 type_index = dex_ins_parameter(ins, 2);

    jd_meta_dex *meta = dex_ins_meta(ins);
    string type_desc = dex_str_of_type_id(meta, type_index);
    string class_name = descriptor_to_s(type_desc);
    instanceof->class_name = class_simple_name(class_name);

    jd_val **locals = ins->stack_in->local_vars;
    dex_local_variable_exp(first, locals[u_b]);
}

static void dex_array_length_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->type = JD_EXPRESSION_ARRAYLENGTH;
    jd_exp_arraylength *arraylength = make_obj(jd_exp_arraylength);
    arraylength->list = make_exp_list(1);
    jd_exp *arrayref = &arraylength->list->args[0];
    u8 u_b = dex_ins_parameter(ins, 1);
    jd_val **locals = ins->stack_in->local_vars;
    dex_local_variable_exp(arrayref, locals[u_b]);
    right->data = arraylength;
}

static void dex_new_instance_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->type = JD_EXPRESSION_UNINITIALIZE;
    jd_exp_uninitialize *uninit = make_obj(jd_exp_uninitialize);
    u8 slot = dex_ins_parameter(ins, 0);
    jd_val *val = ins->stack_out->local_vars[slot];
    uninit->val = val;
    right->data = uninit;
}

static void dex_new_array_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->type = JD_EXPRESSION_NEW_ARRAY;
    right->data = make_obj(jd_exp_new_array);
    jd_exp_new_array *new_array = right->data;

    jd_meta_dex *meta = dex_ins_meta(ins);
    u8 type_idx = dex_ins_parameter(ins, 2);
    string desc = dex_str_of_type_id(meta, type_idx);
    string class_name = descriptor_to_s(desc);

    u8 slot = dex_ins_parameter(ins, 1);
    jd_val *val = ins->stack_out->local_vars[slot];
    new_array->class_name = class_name;
    new_array->list = make_exp_list(1);
    jd_exp *count_exp = &new_array->list->args[0];
    count_exp->type = JD_EXPRESSION_STACK_VAR;
    dex_local_variable_exp(count_exp, val);

}

static void dex_filled_new_array_expression(jd_exp *exp, jd_dex_ins *ins)
{
//    dex_build_assignment(exp, ins);
//    jd_exp *right = get_store_right(exp);

//    right->type = JD_EXPRESSION_NEW_ARRAY;
//    right->data = make_obj(jd_exp_new_array);
    jd_exp_new_array *new_array = make_obj(jd_exp_new_array);

    jd_meta_dex *meta = dex_ins_meta(ins);
    u8 type_idx = dex_ins_parameter(ins, 1);
    string desc = dex_str_of_type_id(meta, type_idx);
    string class_name = descriptor_to_s(desc);
    new_array->class_name = class_name;

    u1 size = dex_ins_parameter(ins, 0);
    if (size == 0)
        return;

    new_array->list = make_exp_list(size);
    for (int i = 0; i < size; ++i) {
        jd_exp *count_exp = &new_array->list->args[i];
        count_exp->type = JD_EXPRESSION_STACK_VAR;
        u8 slot = dex_ins_parameter(ins, i + 2);
        jd_val *val = ins->stack_out->local_vars[slot];
        dex_local_variable_exp(count_exp, val);
    }

    jd_ins *next = ins->next;
    if (next != NULL && (dex_ins_is_move_result(next) ||
                         dex_ins_is_move_result_wide(next) ||
                         dex_ins_is_move_result_object(next))) {

        dex_build_assignment(exp, next);
        jd_exp *right = get_store_right(exp);
        right->ins = ins;
        right->type = JD_EXPRESSION_NEW_ARRAY;
        right->data = new_array;
    }
    else {
        exp->type = JD_EXPRESSION_NEW_ARRAY;
        exp->data = new_array;
    }
}

static void dex_filled_new_array_range_expression(jd_exp *exp, jd_dex_ins *ins)
{
    jd_exp_new_array *new_array = make_obj(jd_exp_new_array);

    jd_meta_dex *meta = dex_ins_meta(ins);
    u8 type_idx = dex_ins_parameter(ins, 1);
    string desc = dex_str_of_type_id(meta, type_idx);
    string class_name = descriptor_to_s(desc);
    new_array->class_name = class_name;

    u1 u_a = dex_ins_parameter(ins, 0);
    u2 start_index = dex_ins_parameter(ins, 2);
    u2 count = start_index + u_a - 1;
    u2 param_size = count - start_index + 1;
    if (param_size > 0) {
        new_array->list = make_exp_list(param_size);
        for (int i = start_index; i <= count; ++i) {
            jd_exp *count_exp = &new_array->list->args[i - start_index];
            count_exp->type = JD_EXPRESSION_STACK_VAR;
            jd_val *val = ins->stack_out->local_vars[i];
            dex_local_variable_exp(count_exp, val);
        }
    }

    jd_ins *next = ins->next;
    if (next != NULL && (dex_ins_is_move_result(next) ||
                         dex_ins_is_move_result_wide(next) ||
                         dex_ins_is_move_result_object(next))) {

        dex_build_assignment(exp, next);
        jd_exp *right = get_store_right(exp);
        right->ins = ins;
        right->type = JD_EXPRESSION_NEW_ARRAY;
        right->data = new_array;
    }
    else {
        exp->type = JD_EXPRESSION_NEW_ARRAY;
        exp->data = new_array;
    }
}

static void dex_fill_array_data_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->type = JD_EXPRESSION_NEW_ARRAY;
    exp->data = make_obj(jd_exp_new_array);

    jd_exp_new_array *new_array = exp->data;
    s4 offset = (s4)dex_ins_parameter(ins, 1);
    // TODO: check instruction is copy
    if (ins_is_duplicate(ins)) {
        offset = offset + ins->old_offset;
    }
    else {
        offset = offset + ins->offset;
    }
    //    offset = offset + ins->old_offset;

    jd_dex_ins *payload_ins = dex_ins_of_offset(ins->method, offset);
    u2 element_size = payload_ins->param[1];
    u4 size = payload_ins->param[3] << 16 | payload_ins->param[2];

    DEBUG_PRINT("[payload size]: %d\n", size);

    u1 slot = dex_ins_parameter(ins, 0);
    jd_val *val = ins->stack_out->local_vars[slot];
    new_array->class_name = val->data->cname;
    new_array->list = make_exp_list(size);
    u1 *params = &payload_ins->param[4];

    for (int i = 0; i < size; ++i) {
        jd_exp *count_exp = &new_array->list->args[i];
        count_exp->type = JD_EXPRESSION_CONST;
        jd_exp_const *const_exp = make_obj(jd_exp_const);
        count_exp->data = const_exp;
        count_exp->ins = ins;
        const_exp->val = stack_create_empty_val();
        jd_val *const_val = const_exp->val;
        const_val->data->primitive = make_obj(jd_primitive_union);
        jd_primitive_union *primitive = const_val->data->primitive;

        if (stack_val_is_int(val)) {
            const_val->type = JD_VAR_INT_T;
            int int_value = 0;
            memcpy(&int_value, params, element_size);
            primitive->int_val = int_value;
            const_val->data->cname = (string)g_str_int;
        }
        else if (stack_val_is_long(val)) {
            const_val->type = JD_VAR_LONG_T;
            long long_val = 0;
            memcpy(&long_val, params, element_size);
            primitive->long_val = long_val;
            const_val->data->cname = (string)g_str_long;
        }
        else if (stack_val_is_float(val)) {
            const_val->type = JD_VAR_FLOAT_T;
            float fvalue = 0;
            memcpy(&fvalue, params, element_size);
            primitive->float_val = fvalue;
            const_val->data->cname = (string)g_str_float;
        }
        else if (stack_val_is_double(val)) {
            const_val->type = JD_VAR_DOUBLE_T;
            double dval = 0;
            memcpy(&dval, params, element_size);
            primitive->double_val = dval;
            const_val->data->cname = (string)g_str_double;
        }
        else if (stack_val_is_boolean(val)) {
            const_val->type = JD_VAR_INT_T;
            int int_value = 0;
            memcpy(&int_value, params, element_size);
            primitive->int_val = int_value;
            const_val->data->cname = (string)g_str_boolean;
        }
        else if (stack_val_is_byte(val)) {
            const_val->type = JD_VAR_INT_T;
            int int_value = 0;
            memcpy(&int_value, params, element_size);
            primitive->int_val = int_value;
            const_val->data->cname = (string)g_str_byte;
        }
        else if (stack_val_is_short(val)) {
            const_val->type = JD_VAR_INT_T;
            int int_value = 0;
            memcpy(&int_value, params, element_size);
            primitive->int_val = int_value;
            const_val->data->cname = (string)g_str_short;
        }
        else if (stack_val_is_char(val)) {
            const_val->type = JD_VAR_INT_T;
            int int_value = 0;
            memcpy(&int_value, params, element_size);
            primitive->int_val = int_value;
            const_val->data->cname = (string)g_str_char;
        }
        else {
            const_val->type = JD_VAR_REFERENCE_T;
            const_val->data->cname = (string)g_str_Object;
        }
        params += element_size;
    }
}

static void dex_throw_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_ATHROW;
    jd_exp_athrow *throw_exp = make_obj(jd_exp_athrow);
    throw_exp->list = make_exp_list(1);
    u2 slot = dex_ins_parameter(ins, 0);
    jd_exp *first = &throw_exp->list->args[0];

    jd_val **locals = ins->stack_out->local_vars;
    dex_local_variable_exp(first, locals[slot]);
    exp->data = throw_exp;
}

static void dex_goto_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_GOTO;
    exp->data = make_obj(jd_exp_goto);
    jd_exp_goto *goto_exp = exp->data;
    goto_exp->goto_offset = dex_goto_offset(ins);
}

static void dex_switch_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_SWITCH;
    exp->data = make_obj(jd_exp_switch);
    jd_exp_switch *switch_exp = exp->data;
    switch_exp->default_offset = ins->next->offset;
    if (ins->next != NULL)
        switch_exp->default_offset = ins->next->offset;
    else
        switch_exp->default_offset = 0;

    switch_exp->targets = linit_object();
    DEBUG_PRINT("[switch]: %d -> %s\n", ins->offset, ins->name);
    for (int i = 0; i < ins->targets->size; ++i) {
        jd_dex_ins *target_ins = lget_obj(ins->targets, i);
        if (target_ins->offset == switch_exp->default_offset)
            continue;
        jd_switch_param *param = make_obj(jd_switch_param);
        param->offset = target_ins->offset;
        param->ikey = dex_switch_key(ins, target_ins->offset);
        ladd_obj(switch_exp->targets, param);
    }

    switch_exp->list = make_exp_list(1);
    jd_exp *first = &switch_exp->list->args[0];
    u2 slot = dex_ins_parameter(ins, 0);
    jd_val **locals = ins->stack_out->local_vars;
    dex_local_variable_exp(first, locals[slot]);
}

static void dex_cmp_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->type = JD_EXPRESSION_OPERATOR;
    right->data = make_obj(jd_exp_compare);
    jd_exp_compare *compare = right->data;

    compare->list = make_exp_list(2);
    compare->operator = dex_ins_operator(ins);
    jd_exp *left_cmp = &compare->list->args[0];
    jd_exp *right_cmp = &compare->list->args[1];

    u8 slot_a = dex_ins_parameter(ins, 1);
    u8 slot_b = dex_ins_parameter(ins, 2);

    jd_val **locals = ins->stack_out->local_vars;
    dex_local_variable_exp(left_cmp, locals[slot_a]);
    dex_local_variable_exp(right_cmp, locals[slot_b]);

    switch (ins->code) {
        case DEX_INS_CMPL_FLOAT:
            compare->operator = JD_OP_LT;
            break;
        case DEX_INS_CMPG_FLOAT:
            compare->operator = JD_OP_GT;
            break;
        case DEX_INS_CMPL_DOUBLE:
            compare->operator = JD_OP_LT;
            break;
        case DEX_INS_CMPG_DOUBLE:
            compare->operator = JD_OP_GT;
            break;
        case DEX_INS_CMP_LONG:
            compare->operator = JD_OP_EQ;
            break;
        default:
            break;
    }
}

static void dex_if_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_IF;
    exp->data = make_obj(jd_exp_if);
    jd_exp_if *if_exp = exp->data;
    if_exp->offset = dex_ins_if_jump_offset(ins);
    if_exp->expression = make_obj(jd_exp);
    jd_exp *condition = if_exp->expression;

    condition->ins = ins;
    condition->type = JD_EXPRESSION_OPERATOR;
    condition->data = make_obj(jd_exp_operator);
    jd_exp_operator *exp_operator = condition->data;
    exp_operator->list = make_exp_list(2);
    exp_operator->operator = dex_ins_operator(ins);
    jd_exp *left = &exp_operator->list->args[0];
    jd_exp *right = &exp_operator->list->args[1];
    u1 slot_a = dex_ins_parameter(ins, 0);
    u1 slot_b = dex_ins_parameter(ins, 1);

    jd_val **locals = ins->stack_out->local_vars;
    dex_local_variable_exp(left, locals[slot_a]);
    dex_local_variable_exp(right, locals[slot_b]);
}

static void dex_ifz_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_IF;
    exp->data = make_obj(jd_exp_if);
    jd_exp_if *if_exp = exp->data;
    if_exp->offset = dex_ins_if_jump_offset(ins);
    if_exp->expression = make_obj(jd_exp);
    jd_exp *condition = if_exp->expression;

    u1 slot = dex_ins_parameter(ins, 0);
    jd_val *left_val = ins->stack_out->local_vars[slot];

    if (stack_val_is_boolean(left_val)) {
        if (ins->code == DEX_INS_IF_EQZ) {
            condition->type = JD_EXPRESSION_SINGLE_OPERATOR;
            jd_exp_single_operator *sop = make_obj(jd_exp_single_operator);
            sop->operator = JD_OP_LOGICAL_NOT;
            sop->list = make_exp_list(1);
            jd_exp *single_op_exp = &sop->list->args[0];
            dex_local_variable_exp(single_op_exp, left_val);
            condition->data = sop;
        }
        else {
            condition->type = JD_EXPRESSION_SINGLE_LIST;
            jd_exp_single_list *single_list = make_obj(jd_exp_single_list);
            single_list->list = make_exp_list(1);
            jd_exp *single_op_exp = &single_list->list->args[0];
            dex_local_variable_exp(single_op_exp, left_val);
            condition->data = single_list;
        }
    }
    else {
        condition->ins = ins;
        condition->type = JD_EXPRESSION_OPERATOR;
        condition->data = make_obj(jd_exp_operator);
        jd_exp_operator *exp_operator = condition->data;
        exp_operator->list = make_exp_list(2);
        exp_operator->operator = dex_ins_operator(ins);
        jd_exp *left = &exp_operator->list->args[0];
        dex_local_variable_exp(left, left_val);

        jd_exp *right = &exp_operator->list->args[1];
        right->type = JD_EXPRESSION_CONST;
        if (left_val->type == JD_VAR_REFERENCE_T) {
            jd_exp_const *const_exp = make_obj(jd_exp_const);
            right->data = const_exp;
            jd_val *val = stack_make_primitive_val(JD_VAR_NULL_T);
            val->data->cname = (string)g_str_null;
            val->data->primitive->int_val = 0;
            const_exp->val = val;
            right->data = const_exp;
        }
        else {
            jd_exp_const *const_exp = make_obj(jd_exp_const);
            right->data = const_exp;
            const_exp->val = stack_create_empty_val();
            jd_val *const_val = const_exp->val;
            const_val->type = JD_VAR_INT_T;
            const_val->data->primitive = make_obj(jd_primitive_union);
            jd_primitive_union *primitive = const_val->data->primitive;
            primitive->int_val = 0;
            const_val->data->cname = (string)g_str_int;
        }

        switch (ins->code) {
            case DEX_INS_IF_EQZ:
                exp_operator->operator = JD_OP_EQ;
                break;
            case DEX_INS_IF_NEZ:
                exp_operator->operator = JD_OP_NE;
                break;
            case DEX_INS_IF_LTZ:
                exp_operator->operator = JD_OP_LT;
                break;
            case DEX_INS_IF_GEZ:
                exp_operator->operator = JD_OP_GE;
                break;
            case DEX_INS_IF_GTZ:
                exp_operator->operator = JD_OP_GT;
                break;
            case DEX_INS_IF_LEZ:
                exp_operator->operator = JD_OP_LE;
                break;
            default:
                break;
        }
    }
}

static void dex_array_get_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);

    right->data = make_obj(jd_exp_array_load);
    right->type = JD_EXPRESSION_ARRAY_LOAD;
    jd_exp_array_load *array_load = right->data;
    array_load->list = make_exp_list(2);
    jd_exp *array = &array_load->list->args[0];
    jd_exp *index = &array_load->list->args[1];

    u2 array_slot = dex_ins_parameter(ins, 1);
    u2 index_slot = dex_ins_parameter(ins, 2);

    jd_val **locals = ins->stack_out->local_vars;
    dex_local_variable_exp(index, locals[index_slot]);
    dex_local_variable_exp(array, locals[array_slot]);
}

static void dex_array_put_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_ARRAY_STORE;

    jd_exp_array_store *array_store = make_obj(jd_exp_array_store);
    array_store->list = make_exp_list(3);
    jd_exp *value = &array_store->list->args[0];
    jd_exp *index = &array_store->list->args[1];
    jd_exp *array = &array_store->list->args[2];

    u2 value_slot = dex_ins_parameter(ins, 0);
    u2 index_slot = dex_ins_parameter(ins, 1);
    u2 array_slot = dex_ins_parameter(ins, 2);

    jd_val **locals = ins->stack_out->local_vars;

    dex_local_variable_exp(value, locals[value_slot]);
    dex_local_variable_exp(index, locals[index_slot]);
    dex_local_variable_exp(array, locals[array_slot]);
    exp->data = array_store;
}

static void dex_instance_get_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);
    right->type = JD_EXPRESSION_GET_FIELD;
    jd_exp_get_field *getfield = make_obj(jd_exp_get_field);
    jd_meta_dex *meta = dex_ins_meta(ins);
    u1 obj_slot = dex_ins_parameter(ins, 1);
    u2 field_slot = dex_ins_parameter(ins, 2);
    string field_name = dex_str_of_field_name(meta, field_slot);

    jd_val *val = ins->stack_in->local_vars[obj_slot];
    getfield->class_name = val->data->cname;
    getfield->name = field_name;
    getfield->list = make_exp_list(1);
    jd_exp *field = &getfield->list->args[0];
    jd_val **locals = ins->stack_in->local_vars;
    dex_local_variable_exp(field, locals[obj_slot]);

    right->ins = ins;
    right->data = getfield;
}

static void dex_instance_put_expression(jd_exp *exp, jd_dex_ins *ins)
{
    exp->type = JD_EXPRESSION_PUT_FIELD;
    jd_exp_put_field *putfield = make_obj(jd_exp_put_field);
    exp->data = putfield;

    jd_val **locals = ins->stack_in->local_vars;
    jd_meta_dex *meta = dex_ins_meta(ins);
    u1 obj_slot = dex_ins_parameter(ins, 1);
    u2 field_idx = dex_ins_parameter(ins, 2);
    string field_name = dex_str_of_field_name(meta, field_idx);

    jd_val *val = locals[obj_slot];
    putfield->name = field_name;
    putfield->class_name = val->data->cname;
    putfield->list = make_exp_list(2);
    jd_exp *value_exp = &putfield->list->args[0];
    jd_exp *objref_exp = &putfield->list->args[1];
    objref_exp->type = JD_EXPRESSION_GET_FIELD;
    jd_exp_get_field *exp_get_field = make_obj(jd_exp_get_field);
    exp_get_field->class_name = putfield->class_name;
    exp_get_field->name = putfield->name;
    exp_get_field->list = make_exp_list(1);
    objref_exp->data = exp_get_field;

    u1 value_slot = dex_ins_parameter(ins, 0);
    dex_local_variable_exp(value_exp, locals[value_slot]);
    dex_local_variable_exp(&exp_get_field->list->args[0],
                           locals[obj_slot]);
}

static void dex_static_get_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);
    right->ins = ins;
    jd_meta_dex *meta = dex_ins_meta(ins);

    jd_exp_get_static *get_static = make_obj(jd_exp_get_static);
    u2 val_slot = dex_ins_parameter(ins, 0);
    jd_val *val = ins->stack_out->local_vars[val_slot];
    get_static->class_name = val->data->cname;

    u2 field_slot = dex_ins_parameter(ins, 1);
    string field_name = dex_str_of_field_name(meta, field_slot);
    get_static->name = field_name;

    get_static->list = make_exp_list(1);
    string owner_class_name = dex_str_of_field_class(meta,
                                                     field_slot);
    get_static->owner_class_name = descriptor_to_s(owner_class_name);
    get_static->name = field_name;
    get_static->list = make_exp_list(1);
    right->type = JD_EXPRESSION_GET_STATIC;
    right->data = get_static;
}

static void dex_static_put_expression(jd_exp *exp, jd_dex_ins *ins)
{
    jd_meta_dex *meta = dex_ins_meta(ins);
    jd_exp_put_static *put_static = make_obj(jd_exp_put_static);

    u2 field_slot = dex_ins_parameter(ins, 1);
    string field_name = dex_str_of_field_name(meta, field_slot);
    string desc = dex_str_of_field_type(meta, field_slot);
    put_static->name = field_name;
    put_static->class_name = descriptor_to_s(desc);
    put_static->owner_class_name = put_static->class_name;

    put_static->list = make_exp_list(1);
    string owner_class_name = dex_str_of_field_class(meta, field_slot);
    put_static->class_name = descriptor_to_s(owner_class_name);

    jd_exp *value_exp = &put_static->list->args[0];
    u2 value_slot = dex_ins_parameter(ins, 0);
    jd_val **locals = ins->stack_out->local_vars;
    dex_local_variable_exp(value_exp, locals[value_slot]);

    exp->type = JD_EXPRESSION_PUT_STATIC;
    exp->data = put_static;
}

static void dex_invoke_expression(jd_exp *exp, jd_dex_ins *ins)
{
    jd_exp_invoke *invoke = make_obj(jd_exp_invoke);

    jd_dex *dex = ins->method->meta;
    jd_meta_dex *meta = dex->meta;
    jsource_file *jf = ins->method->jfile;
    u1 param_size = dex_ins_parameter(ins, 0);
    u2 method_index = dex_ins_parameter(ins, 1);
    dex_method_id *method_id = &meta->method_ids[method_index];

    string class_name = dex_str_of_type_id(meta, method_id->class_idx);
    string name = dex_str_of_idx(meta, method_id->name_idx);

    invoke->class_name = descriptor_to_s(class_name);
    invoke->method_name = name;

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

    invoke->list = make_exp_list(param_size);

    int increase = 2;
    for (int i = 0; i < param_size; ++i) {
        u2 slot = dex_ins_parameter(ins, i + increase);
        jd_val *val = ins->stack_in->local_vars[slot];
        if (dex_ins_is_invoke_static(ins)) {
            jd_exp *arg = &invoke->list->args[i];
            dex_local_variable_exp(arg, val);
        }
        else {
            jd_exp *list_args = invoke->list->args;
            jd_exp *arg = i == 0 ? &list_args[param_size-1] : &list_args[i-1];
            dex_local_variable_exp(arg, val);
        }

        if (stack_val_is_wide(val))
            increase++;
    }

    jd_ins *next = ins->next;
    if (next != NULL && (dex_ins_is_move_result(next) ||
                         dex_ins_is_move_result_wide(next) ||
                         dex_ins_is_move_result_object(next))) {
        dex_build_assignment(exp, next);
        jd_exp *right = get_store_right(exp);
        right->ins = ins;
        exp->ins = ins;
        right->type = JD_EXPRESSION_INVOKE;
        right->data = invoke;
    }
    else {
        exp->type = JD_EXPRESSION_INVOKE;
        exp->data = invoke;
    }

    jd_exp_lambda *lambda_exp = NULL;
    if (dex_ins_is_invoke_direct(ins) &&
        STR_EQL(invoke->method_name, g_str_init)) {
        dex_class_def *cf = hget_u4obj(meta->synthetic_classes_map,
                                       method_id->class_idx);

        // nested loop here, cf == jf->jclass
        if (cf != NULL && cf != jf->jclass) {
            jsource_file *_inner = dex_class_inside(dex, cf, jf);
            _inner->parent = ins->method->jfile;
            _inner->source = _inner->parent->source;
            lambda_exp = dex_lambda(_inner, invoke);
            if (lambda_exp != NULL)
                invoke->lambda = lambda_exp;
        }

        cf = hget_u4obj(meta->class_type_id_map, method_id->class_idx);
        if (cf != NULL &&
            cf != jf->jclass &&
            dex_class_is_anonymous_class(meta, cf)) {
            jsource_file *parent = ins->method->jfile;
            jsource_file *_inner = dex_inner_class(dex, parent, cf);
            jd_exp_anonymous *anonymous_exp = make_obj(jd_exp_anonymous);
            anonymous_exp->list = invoke->list;
            anonymous_exp->jfile = _inner;
            string cname = NULL;

            if (cf->interfaces != NULL) {
                dex_type_item *item = &cf->interfaces->list[0];
                cname = dex_str_of_type_id(meta, item->type_idx);
            }
            else {
                cname = dex_str_of_type_id(meta, cf->superclass_idx);
            }

            anonymous_exp->cname = class_simple_name(cname);
            invoke->anonymous = anonymous_exp;
        }
    }
}

static void dex_invoke_range_expression(jd_exp *exp, jd_dex_ins *ins)
{
    jd_exp_invoke *invoke = make_obj(jd_exp_invoke);
    jd_meta_dex *meta = dex_ins_meta(ins);
    jsource_file *jf = ins->method->jfile;
    u1 start = dex_ins_parameter(ins, 0);
    u2 method_index = dex_ins_parameter(ins, 1);
    dex_method_id *method_id = &meta->method_ids[method_index];

    string class_name = dex_str_of_type_id(meta, method_id->class_idx);
    string name = dex_str_of_idx(meta, method_id->name_idx);

    invoke->class_name = descriptor_to_s(class_name);
    invoke->method_name = name;

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

    // 这里的param_size是不准的, double和long类型的需要处理
    invoke->list = make_exp_list(param_size);
    jd_exp *list_args = invoke->list->args;

    // TODO: not tested for invoke-range
    int increase = 0;
    for (int i = 0; i < param_size; ++i) {
        jd_val *val = ins->stack_in->local_vars[i + start_index + increase];
        if (dex_ins_is_invoke_static_range(ins)) {
            jd_exp *arg = &invoke->list->args[i];
            dex_local_variable_exp(arg, val);
        }
        else {
            jd_exp *arg;
            if (i == 0) {
                // v1, v2, v3, v4, v5, v6, count = 5, start_index = 2
                // object reference
                arg = &list_args[param_size-1];
            }
            else {
                arg = &list_args[i-1];
            }
            dex_local_variable_exp(arg, val);
        }

        if (stack_val_is_wide(val)) {
            increase++;
        }
    }

    jd_ins *next = ins->next;
    if (next != NULL && (dex_ins_is_move_result(next) ||
                         dex_ins_is_move_result_wide(next) ||
                         dex_ins_is_move_result_object(next))) {
        dex_build_assignment(exp, next);
        jd_exp *right = get_store_right(exp);
        right->ins = ins;
        exp->ins = ins;
        right->type = JD_EXPRESSION_INVOKE;
        right->data = invoke;
    }
    else {
        exp->type = JD_EXPRESSION_INVOKE;
        exp->data = invoke;
    }

    jd_exp_lambda *lambda_exp = NULL;
    if (dex_ins_is_invoke_direct(ins) &&
        STR_EQL(invoke->method_name, g_str_init)) {
        dex_class_def *cf = hget_u4obj(meta->synthetic_classes_map,
                                       method_id->class_idx);
        jd_dex *dex = ins->method->meta;
        if (cf != NULL) {
            jsource_file *_inner = dex_class_inside(dex, cf, jf);
            lambda_exp = dex_lambda(_inner, invoke);
            if (lambda_exp != NULL)
                invoke->lambda = lambda_exp;
        }

        cf = hget_u4obj(meta->class_type_id_map, method_id->class_idx);
        if (cf != NULL && dex_class_is_anonymous_class(meta, cf)) {
            jsource_file *parent = ins->method->jfile;
            jsource_file *_inner = dex_inner_class(dex, parent, cf);
            jd_exp_anonymous *anonymous_exp = make_obj(jd_exp_anonymous);
            anonymous_exp->list = invoke->list;
            anonymous_exp->jfile = _inner;
            string cname = NULL;

            if (cf->interfaces != NULL) {
                dex_type_item *item = &cf->interfaces->list[0];
                cname = dex_str_of_type_id(meta, item->type_idx);
            }
            else {
                cname = dex_str_of_type_id(meta, cf->superclass_idx);
            }

            anonymous_exp->cname = class_simple_name(cname);
            invoke->anonymous = anonymous_exp;
        }
    }
}

static void dex_single_operator_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);
    right->type = JD_EXPRESSION_SINGLE_OPERATOR;
    right->data = make_obj(jd_exp_single_operator);
    jd_exp_single_operator *op = right->data;
    op->list = make_exp_list(1);
    jd_exp *first = &op->list->args[0];

    u2 slot = dex_ins_parameter(ins, 1);
    dex_local_variable_exp(first, ins->stack_out->local_vars[slot]);

    switch (ins->code) {
        case DEX_INS_NEG_INT:
        case DEX_INS_NEG_LONG:
        case DEX_INS_NEG_DOUBLE:
        case DEX_INS_NEG_FLOAT:
            op->operator = JD_OP_NEG;
            break;
        case DEX_INS_NOT_INT:
        case DEX_INS_NOT_LONG:
            op->operator = JD_OP_NOT;
            break;
        default:
            break;
    }
}

static void dex_cast_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);
    right->ins = ins;
    jd_exp_cast *cast_exp = make_obj(jd_exp_cast);
    cast_exp->list = make_exp_list(1);
    u1 src_slot = dex_ins_parameter(ins, 1);
    jd_exp *first = &cast_exp->list->args[0];
    jd_val **in_locals = ins->stack_in->local_vars;
    dex_local_variable_exp(first, in_locals[src_slot]);
    right->type = JD_EXPRESSION_CAST;
    right->data = cast_exp;

    switch(ins->code) {
        case DEX_INS_INT_TO_LONG:
        case DEX_INS_FLOAT_TO_LONG:
        case DEX_INS_DOUBLE_TO_LONG:
            cast_exp->class_name = (string)g_str_long;
            break;
        case DEX_INS_INT_TO_FLOAT:
        case DEX_INS_LONG_TO_FLOAT:
        case DEX_INS_DOUBLE_TO_FLOAT:
            cast_exp->class_name = (string)g_str_float;
            break;
        case DEX_INS_INT_TO_DOUBLE:
        case DEX_INS_LONG_TO_DOUBLE:
        case DEX_INS_FLOAT_TO_DOUBLE:
            cast_exp->class_name = (string)g_str_double;
            break;
        case DEX_INS_LONG_TO_INT:
        case DEX_INS_FLOAT_TO_INT:
        case DEX_INS_DOUBLE_TO_INT:
            cast_exp->class_name = (string)g_str_int;
            break;
        case DEX_INS_INT_TO_BYTE:
            cast_exp->class_name = (string)g_str_byte;
            break;
        case DEX_INS_INT_TO_CHAR:
            cast_exp->class_name = (string)g_str_char;
            break;
        case DEX_INS_INT_TO_SHORT:
            cast_exp->class_name = (string)g_str_short;
            break;
        default:
            break;
    }
}

static void dex_binop_2addr_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);
    right->type = JD_EXPRESSION_OPERATOR;
    jd_exp_operator *exp_op = make_obj(jd_exp_operator);
    exp_op->list = make_exp_list(2);
    jd_exp *left_exp = &exp_op->list->args[0];
    jd_exp *right_exp = &exp_op->list->args[1];

    u2 slot_a = dex_ins_parameter(ins, 0);
    u2 slot_b = dex_ins_parameter(ins, 1);
    jd_val **locals = ins->stack_in->local_vars;
    dex_local_variable_exp(left_exp, locals[slot_a]);
    dex_local_variable_exp(right_exp, locals[slot_b]);

    exp_op->operator = dex_ins_operator(ins);
    right->data = exp_op;
}

static void dex_binop_lit_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_build_assignment(exp, ins);
    jd_exp *right = get_store_right(exp);
    right->type = JD_EXPRESSION_OPERATOR;

    jd_exp_operator *exp_op = make_obj(jd_exp_operator);
    exp_op->list = make_exp_list(2);
    jd_exp *left_exp = &exp_op->list->args[0];
    jd_exp *right_exp = &exp_op->list->args[1];

    u2 slot_b = dex_ins_parameter(ins, 1);
    jd_val **locals = ins->stack_in->local_vars;
    dex_local_variable_exp(left_exp, locals[slot_b]);

    right_exp->type = JD_EXPRESSION_CONST;
    jd_exp_const *const_exp = make_obj(jd_exp_const);
    right_exp->data = const_exp;
    const_exp->val = stack_create_empty_val();
    jd_val *const_val = const_exp->val;
    const_val->type = JD_VAR_INT_T;
    const_val->data->primitive = make_obj(jd_primitive_union);
    jd_primitive_union *primitive = const_val->data->primitive;
    primitive->int_val = (int)dex_ins_parameter(ins, 2);
    const_val->data->cname = (string)g_str_int;

    exp_op->operator = dex_ins_operator(ins);
    right->data = exp_op;
}

static void dex_invoke_polymorphic_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_invoke_expression(exp, ins);
}

static void dex_invoke_polymorphic_range_expression(jd_exp *exp, 
        jd_dex_ins *ins)
{
    dex_invoke_range_expression(exp, ins);
}

static void dex_invoke_custom_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_invoke_expression(exp, ins);
}

static void dex_invoke_custom_range_expression(jd_exp *exp, jd_dex_ins *ins)
{
    dex_invoke_range_expression(exp, ins);
}

void dex_build_expression(jd_exp *exp, jd_dex_ins *ins)
{
    if (ins_is_unreached(ins)) {
        build_empty_expression(exp, ins);
        return;
    }

    switch(ins->code) {
        case DEX_INS_NOP:
            build_empty_expression(exp, ins);
            break;
        case DEX_INS_MOVE:
        case DEX_INS_MOVE_FROM16:
        case DEX_INS_MOVE_16:
        case DEX_INS_MOVE_WIDE:
        case DEX_INS_MOVE_WIDE_16:
        case DEX_INS_MOVE_OBJECT:
        case DEX_INS_MOVE_OBJECT_FROM16:
        case DEX_INS_MOVE_OBJECT_16:
            dex_move_expression(exp, ins);
            break;
        case DEX_INS_MOVE_RESULT:
        case DEX_INS_MOVE_RESULT_WIDE:
        case DEX_INS_MOVE_RESULT_OBJECT:
        case DEX_INS_MOVE_EXCEPTION:
            build_empty_expression(exp, ins);
            break;
        case DEX_INS_RETURN_VOID:
        case DEX_INS_RETURN:
        case DEX_INS_RETURN_WIDE:
        case DEX_INS_RETURN_OBJECT:
            dex_return_expression(exp, ins);
            break;
        case DEX_INS_CONST_4:
        case DEX_INS_CONST_16:
        case DEX_INS_CONST:
        case DEX_INS_CONST_HIGH16:
        case DEX_INS_CONST_WIDE_16:
        case DEX_INS_CONST_WIDE_32:
        case DEX_INS_CONST_WIDE:
        case DEX_INS_CONST_WIDE_HIGH16:
        case DEX_INS_CONST_STRING:
        case DEX_INS_CONST_STRING_JUMBO:
        case DEX_INS_CONST_CLASS:
            dex_const_expression(exp, ins);
            break;
        case DEX_INS_MONITOR_ENTER:
        case DEX_INS_MONITOR_EXIT:
            dex_monitor_expression(exp, ins);
            break;
        case DEX_INS_CHECK_CAST:
            dex_check_cast_expression(exp, ins);
            break;
        case DEX_INS_INSTANCE_OF:
            dex_instance_of_expression(exp, ins);
            break;
        case DEX_INS_ARRAY_LENGTH:
            dex_array_length_expression(exp, ins);
            break;
        case DEX_INS_NEW_INSTANCE:
            dex_new_instance_expression(exp, ins);
            break;
        case DEX_INS_NEW_ARRAY:
            dex_new_array_expression(exp, ins);
            break;
        case DEX_INS_FILLED_NEW_ARRAY:
            dex_filled_new_array_expression(exp, ins);
            break;
        case DEX_INS_FILLED_NEW_ARRAY_RANGE:
            dex_filled_new_array_range_expression(exp, ins);
            break;
        case DEX_INS_FILL_ARRAY_DATA:
            dex_fill_array_data_expression(exp, ins);
            break;
        case DEX_INS_THROW:
            dex_throw_expression(exp, ins);
            break;
        case DEX_INS_GOTO:
        case DEX_INS_GOTO_16:
        case DEX_INS_GOTO_32:
            dex_goto_expression(exp, ins);
            break;
        case DEX_INS_PACKED_SWITCH:
        case DEX_INS_SPARSE_SWITCH:
            dex_switch_expression(exp, ins);
            break;
        case DEX_INS_CMPL_FLOAT:
        case DEX_INS_CMPG_FLOAT:
        case DEX_INS_CMPL_DOUBLE:
        case DEX_INS_CMPG_DOUBLE:
        case DEX_INS_CMP_LONG:
            dex_cmp_expression(exp, ins);
            break;
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_NE:
        case DEX_INS_IF_LT:
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GT:
        case DEX_INS_IF_LE:
            dex_if_expression(exp, ins);
            break;
        case DEX_INS_IF_EQZ:
        case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ:
        case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ:
        case DEX_INS_IF_LEZ:
            dex_ifz_expression(exp, ins);
            break;
        case DEX_INS_AGET:
        case DEX_INS_AGET_WIDE:
        case DEX_INS_AGET_OBJECT:
        case DEX_INS_AGET_BOOLEAN:
        case DEX_INS_AGET_BYTE:
        case DEX_INS_AGET_CHAR:
        case DEX_INS_AGET_SHORT:
            dex_array_get_expression(exp, ins);
            break;
        case DEX_INS_APUT:
        case DEX_INS_APUT_WIDE:
        case DEX_INS_APUT_OBJECT:
        case DEX_INS_APUT_BOOLEAN:
        case DEX_INS_APUT_BYTE:
        case DEX_INS_APUT_CHAR:
        case DEX_INS_APUT_SHORT:
            dex_array_put_expression(exp, ins);
            break;
        case DEX_INS_IGET:
        case DEX_INS_IGET_WIDE:
        case DEX_INS_IGET_OBJECT:
        case DEX_INS_IGET_BOOLEAN:
        case DEX_INS_IGET_BYTE:
        case DEX_INS_IGET_CHAR:
        case DEX_INS_IGET_SHORT:
            dex_instance_get_expression(exp, ins);
            break;
        case DEX_INS_IPUT:
        case DEX_INS_IPUT_WIDE:
        case DEX_INS_IPUT_OBJECT:
        case DEX_INS_IPUT_BOOLEAN:
        case DEX_INS_IPUT_BYTE:
        case DEX_INS_IPUT_CHAR:
        case DEX_INS_IPUT_SHORT:
            dex_instance_put_expression(exp, ins);
            break;
        case DEX_INS_SGET:
        case DEX_INS_SGET_WIDE:
        case DEX_INS_SGET_OBJECT:
        case DEX_INS_SGET_BOOLEAN:
        case DEX_INS_SGET_BYTE:
        case DEX_INS_SGET_CHAR:
        case DEX_INS_SGET_SHORT:
            dex_static_get_expression(exp, ins);
            break;
        case DEX_INS_SPUT:
        case DEX_INS_SPUT_WIDE:
        case DEX_INS_SPUT_OBJECT:
        case DEX_INS_SPUT_BOOLEAN:
        case DEX_INS_SPUT_BYTE:
        case DEX_INS_SPUT_CHAR:
        case DEX_INS_SPUT_SHORT:
            dex_static_put_expression(exp, ins);
            break;
        case DEX_INS_INVOKE_VIRTUAL:
        case DEX_INS_INVOKE_SUPER:
        case DEX_INS_INVOKE_DIRECT:
        case DEX_INS_INVOKE_STATIC:
        case DEX_INS_INVOKE_INTERFACE:
            dex_invoke_expression(exp, ins);
            break;
        case DEX_INS_INVOKE_VIRTUAL_RANGE:
        case DEX_INS_INVOKE_SUPER_RANGE:
        case DEX_INS_INVOKE_DIRECT_RANGE:
        case DEX_INS_INVOKE_STATIC_RANGE:
        case DEX_INS_INVOKE_INTERFACE_RANGE:
            dex_invoke_range_expression(exp, ins);
            break;
        case DEX_INS_NEG_INT:
        case DEX_INS_NOT_INT:
        case DEX_INS_NEG_LONG:
        case DEX_INS_NOT_LONG:
        case DEX_INS_NEG_FLOAT:
        case DEX_INS_NEG_DOUBLE:
            dex_single_operator_expression(exp, ins);
            break;
        case DEX_INS_INT_TO_LONG:
        case DEX_INS_INT_TO_FLOAT:
        case DEX_INS_INT_TO_DOUBLE:
        case DEX_INS_LONG_TO_INT:
        case DEX_INS_LONG_TO_FLOAT:
        case DEX_INS_LONG_TO_DOUBLE:
        case DEX_INS_FLOAT_TO_INT:
        case DEX_INS_FLOAT_TO_LONG:
        case DEX_INS_FLOAT_TO_DOUBLE:
        case DEX_INS_DOUBLE_TO_INT:
        case DEX_INS_DOUBLE_TO_LONG:
        case DEX_INS_DOUBLE_TO_FLOAT:
        case DEX_INS_INT_TO_BYTE:
        case DEX_INS_INT_TO_CHAR:
        case DEX_INS_INT_TO_SHORT:
            dex_cast_expression(exp, ins);
            break;
        case DEX_INS_ADD_INT_2ADDR:
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
        case DEX_INS_USHR_LONG_2ADDR:
        case DEX_INS_ADD_FLOAT_2ADDR:
        case DEX_INS_SUB_FLOAT_2ADDR:
        case DEX_INS_MUL_FLOAT_2ADDR:
        case DEX_INS_DIV_FLOAT_2ADDR:
        case DEX_INS_REM_FLOAT_2ADDR:
        case DEX_INS_ADD_DOUBLE_2ADDR:
        case DEX_INS_SUB_DOUBLE_2ADDR:
        case DEX_INS_MUL_DOUBLE_2ADDR:
        case DEX_INS_DIV_DOUBLE_2ADDR:
        case DEX_INS_REM_DOUBLE_2ADDR:
            dex_binop_2addr_expression(exp, ins);
            break;
        case DEX_INS_ADD_INT_LIT16:
        case DEX_INS_RSUB_INT:
        case DEX_INS_MUL_INT_LIT16:
        case DEX_INS_DIV_INT_LIT16:
        case DEX_INS_REM_INT_LIT16:
        case DEX_INS_AND_INT_LIT16:
        case DEX_INS_OR_INT_LIT16:
        case DEX_INS_XOR_INT_LIT16:
        case DEX_INS_ADD_INT_LIT8:
        case DEX_INS_RSUB_INT_LIT8:
        case DEX_INS_MUL_INT_LIT8:
        case DEX_INS_DIV_INT_LIT8:
        case DEX_INS_REM_INT_LIT8:
        case DEX_INS_AND_INT_LIT8:
        case DEX_INS_OR_INT_LIT8:
        case DEX_INS_XOR_INT_LIT8:
            dex_binop_lit_expression(exp, ins);
            break;
        case DEX_INS_INVOKE_POLYMORPHIC:
            dex_invoke_polymorphic_expression(exp, ins);
            break;
        case DEX_INS_INVOKE_POLYMORPHIC_RANGE:
            dex_invoke_polymorphic_range_expression(exp, ins);
            break;
        case DEX_INS_INVOKE_CUSTOM:
            dex_invoke_custom_expression(exp, ins);
            break;
        case DEX_INS_INVOKE_CUSTOM_RANGE:
            dex_invoke_custom_range_expression(exp, ins);
            break;
        case DEX_INS_COPY_BASIC_BLOCK:
        case DEX_INS_COPY_BASIC_BLOCK_GOTO:
            dex_copy_block(ins);
            break;
        default:
            build_empty_expression(exp, ins);
            break;
    }
}

static void dex_copy_block(jd_dex_ins *copy)
{
    list_object *list = copy->extra;
    jd_method *m = copy->method;
    for (int i = 0; i < list->size; ++i) {
        jd_dex_ins *ins = lget_obj(list, i);
        jd_exp *exp = make_obj(jd_exp);
        exp->idx = m->expressions->size;
        dex_build_expression(exp, ins);
        exp_mark_copy(exp);
        ins->expression = exp;
        exp->ins = ins;
        exp->block = copy->block;
        ladd_obj(m->expressions, exp);
    }
}

void dex_instruction_to_expression(jd_method *m)
{
    DEBUG_PRINT("m: %s(%s)\n", m->name,
                lstring_join(m->desc->list, ","));
    m->expressions = linit_object();
    m->lambdas = linit_object();
    m->declarations = bitset_create();

    for (int i = 0; i < m->instructions->size; ++i) {
        jd_dex_ins *ins = get_ins(m, i);

        if (ins->expression != NULL)
            continue;

        if (ins_is_copy_block(ins) ) {
            // dex_copy_block(ins);
        }

        jd_exp *exp = make_obj(jd_exp);
        exp->ins = ins;
        exp->idx = m->expressions->size;
        ins->expression = exp;
        exp->block = ins->block;
        dex_build_expression(exp, ins);
        ladd_obj(m->expressions, exp);
    }
}

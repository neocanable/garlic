#include "jvm/jvm_ins.h"
#include "jvm/jvm_simulator.h"
#include "jvm/jvm_decompile.h"
#include "jvm/jvm_lambda.h"
#include "jvm/jvm_descriptor.h"

#include "decompiler/method.h"
#include "decompiler/expression_helper.h"
#include "decompiler/expression_node.h"
#include "decompiler/klass.h"
#include "decompiler/ssa.h"
#include "decompiler/stack.h"
#include "common/str_tools.h"
#include "jar.h"

jd_exp* build_string_exp(jd_exp *exp, string str)
{
    exp->type = JD_EXPRESSION_CONST;
    jd_exp_const *const_exp = make_obj(jd_exp_const);
    jd_val *val = stack_create_empty_val();
    val->type = JD_VAR_REFERENCE_T;
    val->data->cname = (string)g_str_String;
    val->data->val = str_dup(str);
    const_exp->val = val;
    exp->data = const_exp;
    return exp;
}

static void build_stack_var_exp(jd_exp *exp, jd_val *val)
{
    jd_var *var = val->stack_var;
    var->use_count ++;
    exp->type = JD_EXPRESSION_STACK_VAR;
    exp->data = var;
}

static void build_stack_var_exp_for_dup(jd_exp *exp, jd_val *val)
{
    jd_var *var = val->stack_var;
    var->dupped_count ++;
    exp->type = JD_EXPRESSION_STACK_VAR;
    exp->data = var;
}

static void build_stack_var_exp_of_store(jd_exp *exp, jd_val *val)
{
    jd_var *var = val->stack_var;
    var->use_count ++;
    var->store_count ++;
    exp->type = JD_EXPRESSION_STACK_VAR;
    exp->data = var;
}

static void build_assignment_expression(jd_exp *exp, jd_ins *ins)
{
    jd_exp_assignment *assignment = make_obj(jd_exp_assignment);
    jd_exp *left = make_obj(jd_exp);
    jd_exp *right = make_obj(jd_exp);
    right->ins = ins;
    assignment->right = right;
    assignment->assign_operator = JD_OP_ASSIGN;
    assignment->def_count ++;

    left->type = JD_EXPRESSION_LVALUE;
    jd_exp_lvalue *lvalue = make_obj(jd_exp_lvalue);
    left->data = lvalue;
    assignment->left = left;

    jd_val *out_val = ins->stack_out->vals[0];
    jd_var *stack_var = out_val->stack_var;

    assert(stack_var != NULL);
    lvalue->stack_var = stack_var;
    stack_var->def_count ++;

    exp->type = JD_EXPRESSION_ASSIGNMENT;
    exp->data = assignment;
    exp->ins = ins;
}

static void build_load_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;

    jd_val *val = ins->stack_out->vals[0];
    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_LOCAL_VARIABLE;
    right->ins = ins;
    right->data = val;
}

static void build_store_expression(jd_exp *exp, jd_ins *ins)
{
    exp->type = JD_EXPRESSION_STORE;
    exp->data = make_obj(jd_exp_store);
    jd_exp_store *store = exp->data;
    store->list = make_exp_list(2);
    jd_exp *left = &store->list->args[0];
    jd_exp *right = &store->list->args[1];

    left->type = JD_EXPRESSION_LOCAL_VARIABLE;
    int slot = jvm_ins_store_slot(ins);
    jd_val *local_var = find_local_variable(ins->stack_out, slot);
    left->data = local_var;
    jd_val *val0 = ins->stack_in->vals[0];
    build_stack_var_exp_of_store(right, val0);
}

static void build_const_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;
    jd_exp *right = assignment->right;

    right->type = JD_EXPRESSION_CONST;
    jd_exp_const *const_exp = make_obj(jd_exp_const);
    const_exp->val = ins->stack_out->vals[0];
    right->data = const_exp;
}

static void build_instance_of_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;
    jclass_file *jc = ins->method->meta;

    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_INSTANCEOF;
    jd_exp_instanceof *instanceof = make_obj(jd_exp_instanceof);

    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *info = pool_item(jc, index);
    // TODO: add to import
    string name = get_class_name(jc, info);
    instanceof->class_name = class_simple_name(name);

    instanceof->list = make_exp_list(1);
    jd_exp *first = &instanceof->list->args[0];
    build_stack_var_exp(first, ins->stack_in->vals[0]);
    right->data = instanceof;
}

static void build_operator_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;
    assignment->assign_operator = JD_OP_ASSIGN;
    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_OPERATOR;

    jd_exp_operator *exp_op = make_obj(jd_exp_operator);
    exp_op->list = make_exp_list(2);
    jd_exp *left_exp = &exp_op->list->args[0];
    jd_exp *right_exp = &exp_op->list->args[1];

    build_stack_var_exp(left_exp, ins->stack_in->vals[1]);
    build_stack_var_exp(right_exp, ins->stack_in->vals[0]);

    exp_op->operator = jvm_ins_operator(ins);
    right->data = exp_op;

    exp->type = JD_EXPRESSION_ASSIGNMENT;
    exp->data = assignment;
}

static void build_cast_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;

    jd_exp *right = assignment->right;
    right->ins = ins;

    jd_exp_cast *cast_exp = make_obj(jd_exp_cast);
    cast_exp->list = make_exp_list(1);
    jd_exp *first = &cast_exp->list->args[0];
    build_stack_var_exp(first, ins->stack_in->vals[0]);
    right->type = JD_EXPRESSION_CAST;
    right->data = cast_exp;

    switch(ins->code) {
        case INS_I2L:
        case INS_F2L:
        case INS_D2L:
            cast_exp->class_name = (string)g_str_long;
            break;
        case INS_I2F:
        case INS_L2F:
        case INS_D2F:
            cast_exp->class_name = (string)g_str_float;
            break;
        case INS_I2D:
        case INS_L2D:
        case INS_F2D:
            cast_exp->class_name = (string)g_str_double;
            break;
        case INS_L2I:
        case INS_F2I:
        case INS_D2I:
            cast_exp->class_name = (string)g_str_int;
            break;
        case INS_I2B:
            cast_exp->class_name = (string)g_str_byte;
            break;
        case INS_I2C:
            cast_exp->class_name = (string)g_str_char;
            break;
        case INS_I2S:
            cast_exp->class_name = (string)g_str_short;
            break;
        case INS_CHECKCAST: {
            u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
            jcp_info *info = pool_item(ins->method->meta, index);
            jclass_file *jc = ins->method->meta;
            string class_name = get_class_name(jc, info);
            string descriptor = descriptor_to_s(class_name);
            cast_exp->class_name = class_simple_name(descriptor);
            break;
        }
        default:
            break;
    }
}

static void build_anonymous_expression(jd_ins *ins, jd_exp_invoke *invoke)
{
    assert(jvm_ins_is_invokespecial(ins));

    jsource_file *jf = ins->method->jfile;
    jclass_file *jc = ins->method->meta;
    jcp_info *methodref_info = jvm_invoke_methodref_info(ins);
    if (methodref_info == NULL || jf->jar_entry == NULL)
        return;

    string fname = get_class_name(jc, methodref_info);
    invoke->class_name = class_simple_name(fname);

    if (!str_start_with(fname, jf->fname) || STR_EQL(fname, jf->fname))
        return; // 不是内部类

    jd_jar_entry *inner_entry = NULL;
    bool is_inner = false;
    for (int i = 0; i < jf->jar_entry->inner_classes->size; ++i) {
        jd_jar_entry *entry = lget_obj(jf->jar_entry->inner_classes, i);
        if (STR_EQL(entry->cname, invoke->class_name)) {
            inner_entry = entry;
            is_inner = true;
            break;
        }
    }

    if (inner_entry == NULL) {
        for (int i = 0; i < jf->jar_entry->anoymous_classes->size; ++i) {
            jd_jar_entry *entry = lget_obj(jf->jar_entry->anoymous_classes, i);
            if (STR_EQL(entry->cname, invoke->class_name)) {
                inner_entry = entry;
                is_inner = false;
                break;
            }
        }
    }

    if (inner_entry == NULL)
        return;

    jsource_file *inner_jf = jar_entry_anonymous_analyse(
            jf->jar, inner_entry, jf);
    jclass_file *inner_jc = inner_jf->jclass;

    if (!class_has_flag(inner_jc, CLASS_ACC_SYNTHETIC)) {
        return;
    }

    jd_exp_anonymous *anonymous_exp = make_obj(jd_exp_anonymous);
    anonymous_exp->list = invoke->list;
    anonymous_exp->jfile = inner_jf;
    string cname = NULL;
    if (!is_list_empty(inner_jf->interfaces)) {
        cname = lget_obj_first(inner_jf->interfaces);
    }
    else {
        cname = inner_jf->super_cname;
    }
    anonymous_exp->cname = cname;
    invoke->anonymous = anonymous_exp;

    if (is_inner) {
        ldel_obj(jf->jar_entry->inner_classes, inner_entry);
        ladd_obj(jf->jar_entry->anoymous_classes, inner_entry);
    }
}

static void build_invoke_expression(jd_exp *exp, jd_ins *ins)
{
    jd_exp_invoke *invoke = make_obj(jd_exp_invoke);
    jd_method *m = ins->method;
    jclass_file *jc = m->meta;
    jsource_file *jf = jc->jfile;

    jcp_info *nt_info = jvm_invoke_name_and_type_info(ins);
    jconst_name_and_type *nt = nt_info->info->name_and_type;
    invoke->method_name = pool_str(jc, nt->name_index);
    jd_descriptor *descriptor = jvm_descriptor(jf,nt->descriptor_index);

    if (jvm_ins_is_invokestatic(ins)) {
        u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
        jcp_info *info = pool_item(jc, index);
        jconst_methodref *method_ref = info->info->methodref;
        string full = pool_str(jc, method_ref->class_index);
        invoke->class_name = class_simple_name(full);
    }

    invoke->list = make_obj(jd_exp_list);
    invoke->list->len = descriptor->list->size;
    invoke->descriptor = descriptor;
    if (jvm_ins_is_invokevirtual(ins) ||
        jvm_ins_is_invokeinterface(ins) ||
        jvm_ins_is_invokespecial(ins))
        invoke->list->len += 1;
    invoke->list->args = make_obj_arr(jd_exp, invoke->list->len);

    for (int i = 0; i < descriptor->list->size; ++i) {
        jd_exp *e = &invoke->list->args[descriptor->list->size - i - 1];
        jd_val *val = ins->stack_in->vals[i];
        build_stack_var_exp(e, val);
    }
    if (jvm_ins_is_invokevirtual(ins) ||
        jvm_ins_is_invokeinterface(ins) ||
        jvm_ins_is_invokespecial(ins)) {
        /* invokevirtual, invokeinterface, invokespecial */
        jd_exp *e = &invoke->list->args[invoke->list->len - 1];
        jd_val *val = ins->stack_in->vals[invoke->list->len - 1];
        build_stack_var_exp(e, val);
    }

    if (jvm_ins_is_invokespecial(ins))
        build_anonymous_expression(ins, invoke);

    if (STR_EQL(descriptor->str_return, "V")) {
        exp->type = JD_EXPRESSION_INVOKE;
        exp->data = invoke;
        exp->ins = ins;
    }
    else {
        if (ins->next != NULL && jvm_ins_is_pops(ins->next)) {
            exp->type = JD_EXPRESSION_INVOKE;
            exp->data = invoke;
            exp->ins = ins;
        }
        else {
            build_assignment_expression(exp, ins);
            jd_exp_assignment *assignment = exp->data;
            assignment->right->type = JD_EXPRESSION_INVOKE;
            assignment->right->data = invoke;
        }
    }
}

static void build_if_boolean_exp(jd_exp *condition, jd_ins *ins, jd_val *val0)
{
    if (ins->code == INS_IFEQ) {
        condition->type = JD_EXPRESSION_SINGLE_OPERATOR;
        jd_exp_single_operator *s = make_obj(jd_exp_single_operator);
        s->list = make_exp_list(1);
        jd_exp *single = &s->list->args[0];
        build_stack_var_exp(single, val0);
        s->operator = JD_OP_LOGICAL_NOT;
        condition->data = s;
        condition->ins = ins;
        condition->block = ins->block;
    }
    else {
        condition->type = JD_EXPRESSION_SINGLE_LIST;
        jd_exp_single_list *s = make_obj(jd_exp_single_list);
        s->list = make_exp_list(1);
        jd_exp *single = &s->list->args[0];
        build_stack_var_exp(single, val0);
        condition->data = s;
        condition->ins = ins;
        condition->block = ins->block;
    }
}

static void build_if_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_IF;
    exp->data = make_obj(jd_exp_if);

    jd_exp_if *if_exp = exp->data;
    int16_t offset = (int16_t)(ins->param[0] << 8 | ins->param[1]);
    if_exp->offset = ins->offset + offset;
    if_exp->expression = make_obj(jd_exp);

    jd_exp *condition = if_exp->expression;
    condition->ins = ins;
    condition->type = JD_EXPRESSION_OPERATOR;
    condition->data = make_obj(jd_exp_operator);
    jd_exp_operator *exp_operator = condition->data;
    exp_operator->list = make_exp_list(2);

    exp_operator->operator = jvm_ins_operator(ins);
    jd_exp *left = &exp_operator->list->args[0];
    jd_exp *right = &exp_operator->list->args[1];

    switch (ins->code) {
        case INS_IFEQ: // ifeq
        case INS_IFNE: // ifne
        case INS_IFLT: // iflt
        case INS_IFGE: // ifge
        case INS_IFGT: // ifgt
        case INS_IFLE: { // iflt
            jd_val *val0 = ins->stack_in->vals[0];

            if (stack_val_is_boolean(val0)) {
                build_if_boolean_exp(condition, ins, val0);
            }
            else {
                build_stack_var_exp(left, ins->stack_in->vals[0]);
                right->type = JD_EXPRESSION_CONST;
                jd_exp_const *const_exp = make_obj(jd_exp_const);
                jd_val *var = stack_make_primitive_val(JD_VAR_INT_T);
                var->data->primitive->int_val = 0;
                const_exp->val = var;
                right->data = const_exp;
            }
            break;
        }
        case INS_IF_ICMPEQ: // if_icmpeq
        case INS_IF_ICMPNE: // if_icmpne
        case INS_IF_ICMPLT: // if_icmplt
        case INS_IF_ICMPGE: // if_icmpge
        case INS_IF_ICMPGT: // if_icmpgt
        case INS_IF_ICMPLE: // if_icmple
        case INS_IF_ACMPEQ: // if_acmpeq
        case INS_IF_ACMPNE: { // if_acmpne
            build_stack_var_exp(left, ins->stack_in->vals[1]);
            build_stack_var_exp(right, ins->stack_in->vals[0]);
            break;
        }
        case INS_IFNULL: // ifnull
        case INS_IFNONNULL: { // ifnonnull
            build_stack_var_exp(left, ins->stack_in->vals[0]);
            right->type = JD_EXPRESSION_CONST;
            jd_exp_const *const_exp = make_obj(jd_exp_const);
            jd_val *val = stack_make_primitive_val(JD_VAR_NULL_T);
            val->data->cname = (string)g_str_null;
            val->data->primitive->int_val = 0;
            const_exp->val = val;
            right->data = const_exp;
            break;
        }
    }

    exp->data = if_exp;
}

static void build_cmp_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_ASSIGNMENT;

    jd_exp *left = make_obj(jd_exp);
    left->type = JD_EXPRESSION_LVALUE;
    jd_exp_lvalue *lvalue = make_obj(jd_exp_lvalue);
    jd_var *stack_var = stack_find_var(ins->method, ins, 0);
    lvalue->stack_var = stack_var;
//    lvalue->var = &ins->stack_out->vars[0];
    left->data = lvalue;

    jd_exp *right = make_obj(jd_exp);
    right->type = JD_EXPRESSION_OPERATOR;
    right->data = make_obj(jd_exp_compare);
    jd_exp_compare *compare = right->data;

    compare->list = make_exp_list(2);
    compare->operator = jvm_ins_operator(ins);
    jd_exp *left_cmp = &compare->list->args[0];
    jd_exp *right_cmp = &compare->list->args[1];

    build_stack_var_exp(left_cmp, ins->stack_in->vals[0]);
    build_stack_var_exp(right_cmp, ins->stack_in->vals[1]);

    jd_exp_assignment *assignment = make_obj(jd_exp_assignment);
    assignment->left = left;
    assignment->right = right;
    assignment->assign_operator = JD_OP_ASSIGN;
    exp->data = assignment;
}

static void build_goto_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_GOTO;
    exp->data = make_obj(jd_exp_goto);
    jd_exp_goto *goto_exp = exp->data;
    if (jvm_ins_is_goto_w(ins)) {
        // goto_w
        int32_t offset = (int32_t)ins->param[0] << 24 | 
                                  ins->param[1] << 16 | 
                                  ins->param[2] << 8  | 
                                  ins->param[3];
        goto_exp->goto_offset = ins->offset + offset;
    }
    else {
        int16_t offset = (int16_t)(ins->param[0] << 8) | ins->param[1];
        goto_exp->goto_offset = ins->offset + offset;
    }
}

static void build_switch_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_SWITCH;

    jd_exp_switch *switch_exp = make_obj(jd_exp_switch);
    switch_exp->list = make_exp_list(1);
    jd_exp *first = &switch_exp->list->args[0];

    switch_exp->default_offset = jvm_switch_default_offset(ins);
    switch_exp->targets = linit_object();

    for (int i = 0; i < ins->targets->size; ++i) {
        jd_ins *target_ins = lget_obj(ins->targets, i);
        jd_switch_param *param = make_obj(jd_switch_param);
        param->offset = target_ins->offset;
        param->ikey = jvm_switch_key(ins, target_ins->offset);
        ladd_obj(switch_exp->targets, param);
    }

    build_stack_var_exp(first, ins->stack_in->vals[0]);
    exp->data = switch_exp;
}

static void build_new_array_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;

    string class_name = NULL;
    if (jvm_ins_is_newarray(ins)) {
        switch (ins->param[0]) {
            case 4:
                class_name = (string)g_str_boolean;
                break;
            case 5:
                class_name = (string)g_str_char;
                break;
            case 6:
                class_name = (string)g_str_float;
                break;
            case 7:
                class_name = (string)g_str_double;
                break;
            case 8:
                class_name = (string)g_str_byte;
                break;
            case 9:
                class_name = (string)g_str_short;
                break;
            case 10:
                class_name = (string)g_str_int;
                break;
            case 11:
                class_name = (string)g_str_long;
                break;
            default:
                class_name = (string)g_str_unknown;
                break;
        }
    }
    else {
        u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
        jcp_info *info = pool_item(ins->method->meta, index);
        class_name = get_class_name(ins->method->meta, info);
        // TODO: should imports?
        class_name = class_simple_name(class_name);
    }

    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_NEW_ARRAY;
    jd_exp_new_array *new_array = make_obj(jd_exp_new_array);
    jd_val *push0 = ins->stack_in->vals[0];
    new_array->class_name = class_name;
    new_array->list = make_exp_list(1);
    jd_exp *count_exp = &new_array->list->args[0];
    count_exp->type = JD_EXPRESSION_CONST;
    jd_exp_const *const_exp = make_obj(jd_exp_const);
    const_exp->val = push0;
    count_exp->data = const_exp;

    build_stack_var_exp(count_exp, ins->stack_in->vals[0]);
    right->data = new_array;
}

static void build_arraylength_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;

    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_ARRAYLENGTH;
    jd_exp_arraylength *arraylength = make_obj(jd_exp_arraylength);
    arraylength->list = make_exp_list(1);
    jd_exp *arrayref = &arraylength->list->args[0];

    build_stack_var_exp(arrayref, ins->stack_in->vals[0]);
    right->data = arraylength;
}

static void build_array_load_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;
    jd_exp *right = assignment->right;
    right->data = make_obj(jd_exp_array_load);
    right->type = JD_EXPRESSION_ARRAY_LOAD;
    jd_exp_array_load *array_load = right->data;
    array_load->list = make_exp_list(2);
    jd_exp *index = &array_load->list->args[0];
    jd_exp *array = &array_load->list->args[1];
    build_stack_var_exp(index, ins->stack_in->vals[0]);
    build_stack_var_exp(array, ins->stack_in->vals[1]);
}

static void build_array_store_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_ARRAY_STORE;

    jd_exp_array_store *array_store = make_obj(jd_exp_array_store);
    array_store->list = make_exp_list(3);
    jd_exp *value = &array_store->list->args[0];
    jd_exp *index = &array_store->list->args[1];
    jd_exp *array = &array_store->list->args[2];

    build_stack_var_exp_of_store(value, ins->stack_in->vals[0]);
    build_stack_var_exp(index, ins->stack_in->vals[1]);
    build_stack_var_exp(array, ins->stack_in->vals[2]);
    exp->data = array_store;
}

static void build_uninitialize_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;
    jd_exp *right = assignment->right;

    jd_exp_uninitialize *uninit = make_obj(jd_exp_uninitialize);
    uninit->val = ins->stack_out->vals[0];
    right->ins = ins;
    right->type = JD_EXPRESSION_UNINITIALIZE;
    right->data = uninit;
}

static void build_return_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_RETURN;
    jd_exp_return *exp_return = make_obj(jd_exp_return);
    exp->data = exp_return;
    if (!jvm_ins_is_voidreturn(ins)) {
        exp_return->list = make_exp_list(1);
        jd_exp *first = &exp_return->list->args[0];
        first->ins = ins;
        build_stack_var_exp(first, ins->stack_in->vals[0]);
    }
    else
        exp_return->list = make_obj(jd_exp_list);
}

static void build_last_void_return(jd_exp *exp, jd_ins *ins)
{
    jd_method *m = ins->method;
    jd_ins *last_ins = lget_obj_last(m->instructions);
    if (last_ins == ins && jvm_ins_is_voidreturn(ins))
        exp_mark_nopped(exp);
}

static void build_athrow_expression(jd_exp *exp, jd_ins *ins)
{
    exp->ins = ins;
    exp->type = JD_EXPRESSION_ATHROW;
    jd_exp_athrow *throw_exp = make_obj(jd_exp_athrow);
    throw_exp->list = make_exp_list(1);
    jd_exp *first = &throw_exp->list->args[0];

    build_stack_var_exp(first, ins->stack_in->vals[0]);
    exp->data = throw_exp;
}

static void build_get_static_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;
    jd_exp *right = assignment->right;
    right->ins = ins;

    jd_exp_get_static *get_static = make_obj(jd_exp_get_static);
    jd_val *val = ins->stack_out->vals[0];
    get_static->class_name = val->data->cname;

    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *info = pool_item(ins->method->meta, index);
    string owner_name = get_field_class(ins->method->meta, info);
    get_static->owner_class_name = class_simple_name(owner_name);

    get_static->name = val->data->val;
    get_static->list = make_exp_list(1);
    right->type = JD_EXPRESSION_GET_STATIC;
    right->data = get_static;
}

static void build_put_static_expression(jd_exp *exp, jd_ins *ins)
{
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jclass_file *jc = ins->method->meta;
    jcp_info *info = pool_item(ins->method->meta, index);

    jd_exp_put_static *put_static = make_obj(jd_exp_put_static);
    string full_name = get_field_class(jc, info);
    put_static->class_name = class_simple_name(full_name);
    put_static->name = get_field_name(jc, info);
    put_static->list = make_exp_list(1);
    string owner_class_name = get_field_class(jc, info);
    put_static->owner_class_name = class_simple_name(owner_class_name);

    jd_exp *value_exp = &put_static->list->args[0];
    build_stack_var_exp_of_store(value_exp, ins->stack_in->vals[0]);

    exp->type = JD_EXPRESSION_PUT_STATIC;
    exp->data = put_static;
}

static void build_put_field_expression(jd_exp *exp, jd_ins *ins)
{
    jclass_file *jc = ins->method->meta;
    exp->type = JD_EXPRESSION_PUT_FIELD;
    jd_exp_put_field *putfield = make_obj(jd_exp_put_field);
    exp->data = putfield;

    uint16_t field_index = ins->param[0] << 8 | ins->param[1];
    jcp_info *info = pool_item(jc, be16toh(field_index));
    putfield->name = get_field_name(jc, info);
    putfield->class_name = get_field_descriptor(jc, info);
    putfield->list = make_exp_list(2);
    jd_exp *value_exp = &putfield->list->args[0];
    jd_exp *objref_exp = &putfield->list->args[1];
    objref_exp->type = JD_EXPRESSION_GET_FIELD;
    jd_exp_get_field *exp_get_field = make_obj(jd_exp_get_field);
    exp_get_field->class_name = putfield->class_name;
    exp_get_field->name = putfield->name;
    exp_get_field->list = make_exp_list(1);
    objref_exp->data = exp_get_field;

    build_stack_var_exp_of_store(value_exp, ins->stack_in->vals[0]);
    build_stack_var_exp(&exp_get_field->list->args[0],
                        ins->stack_in->vals[1]);
}

static void build_get_field_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;

    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_GET_FIELD;
    jd_exp_get_field *getfield = make_obj(jd_exp_get_field);
    jd_val *val = ins->stack_out->vals[0];
    getfield->class_name = val->data->cname;
    getfield->name = val->data->val;
    getfield->list = make_exp_list(1);
    build_stack_var_exp(&getfield->list->args[0],
                        ins->stack_in->vals[0]);

    right->ins = ins;
    right->data = getfield;
}

static void build_iinc_expression(jd_exp *exp, jd_ins *ins)
{
    jd_val *local_var = NULL;
    uint16_t slot;
    if (jvm_ins_is_wide(ins))
        slot = ins->param[1] << 8 | ins->param[2];
    else
        slot = ins->param[0];

    local_var = find_local_variable(ins->stack_out, slot);
    assert(local_var != NULL);

    jd_exp_iinc *iinc = make_obj(jd_exp_iinc);
    iinc->list = make_exp_list(2);
    jd_exp *local_variable_exp = &iinc->list->args[0];
    jd_exp *const_expression = &iinc->list->args[1];

    const_expression->type = JD_EXPRESSION_CONST;
    const_expression->data = make_obj(jd_exp_const);
    jd_exp_const *const_exp = const_expression->data;

    local_variable_exp->type = JD_EXPRESSION_LOCAL_VARIABLE;
    local_variable_exp->data = local_var;

    jd_val *val = stack_make_primitive_val(JD_VAR_INT_T);
    val->data->primitive->int_val = ins->param[1];
    const_exp->val = val;

    exp->type = JD_EXPRESSION_IINC;
    exp->data = iinc;
}

static void build_neg_expression(jd_exp *exp, jd_ins *ins)
{
    build_assignment_expression(exp, ins);
    jd_exp_assignment *assignment = exp->data;

    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_SINGLE_OPERATOR;
    jd_exp_single_operator *op = make_obj(jd_exp_single_operator);
    op->operator = JD_OP_NEG;
    op->list = make_exp_list(1);
    jd_exp *first = &op->list->args[0];

    build_stack_var_exp(first, ins->stack_in->vals[0]);
    right->data = op;
}

static void build_monitorenter_expression(jd_exp *exp, jd_ins *ins)
{
    exp->type = JD_EXPRESSION_MONITOR_ENTER;
    exp->data = make_obj(jd_exp_monitorenter);
    jd_exp_monitorenter *enter = exp->data;
    enter->list = make_exp_list(1);
    jd_exp *first = &enter->list->args[0];
    build_stack_var_exp(first, ins->stack_in->vals[0]);
}

static void build_monitorexit_expression(jd_exp *exp, jd_ins *ins)
{
    exp->type = JD_EXPRESSION_MONITOR_EXIT;
    exp->data = make_obj(jd_exp_monitorexit);
    jd_exp_monitorexit *exit = exp->data;
    exit->list = make_exp_list(1);
    jd_exp *first = &exit->list->args[0];
    build_stack_var_exp(first, ins->stack_in->vals[0]);
}

static void increase_assignment_expression_dupped_count(jd_val *val)
{
    jd_ins *ins = val->ins;
    jd_exp *exp = ins->expression;
    if (exp != NULL) {
        assert(exp_is_assignment(exp));
        jd_exp_assignment *assignment = exp->data;
        assignment->dupped_count++;
        assignment->def_count++;
    }

    val->stack_var->def_count ++;
}

static void dup_a_stack_var_expression(jd_exp *exp, jd_ins *ins, jd_var *var)
{
    jd_exp_assignment *assignment = make_obj(jd_exp_assignment);
    jd_exp *left = make_obj(jd_exp);
    jd_exp *right = make_obj(jd_exp);
    right->ins = ins;
    assignment->right = right;
    assignment->assign_operator = JD_OP_ASSIGN;

    left->type = JD_EXPRESSION_LVALUE;
    jd_exp_lvalue *lvalue = make_obj(jd_exp_lvalue);
    left->data = lvalue;
    assignment->left = left;
    lvalue->stack_var = var;

    right->type = JD_EXPRESSION_STACK_VAR;
    right->data = var;
    var->def_count ++;

    exp->type = JD_EXPRESSION_ASSIGNMENT;
    exp->data = assignment;
}

static void build_stack_var_dup_expression(jd_ins *ins, jd_var *var)
{
    jd_method *m = ins->method;
    jd_exp *expression = make_obj(jd_exp);
    expression->ins = ins;
    expression->idx = m->expressions->size;
    build_assignment_expression(expression, ins);

    jd_exp_assignment *assignment = expression->data;
    jd_exp *right = assignment->right;
    right->type = JD_EXPRESSION_STACK_VAR;
    right->data = var;
    var->use_count ++;

    expression->block = ins->block;
    ins->expression = expression;

    ladd_obj(m->expressions, expression);
}

static void build_dup_expression(jd_exp *exp, jd_ins *ins)
{
//    exp->type = JD_EXPRESSION_EMPTY;
//    exp->ins = ins;

    switch (ins->code) {
        case INS_DUP:
        case INS_DUP_X1:
        case INS_DUP_X2: {
            jd_val *item0 = ins->stack_in->vals[0];
            jd_var *var0 = item0->stack_var;
            increase_assignment_expression_dupped_count(item0);
            break;
        }
        case INS_DUP2: {
            jd_val *item0 = ins->stack_in->vals[0];
            if (jd_stack_val_is_compute_category2(item0)) {
                jd_var *var0 = item0->stack_var;
                increase_assignment_expression_dupped_count(item0);
            }
            else {
                jd_var *var0 = item0->stack_var;
                jd_val *item1 = ins->stack_in->vals[1];
                jd_var *var1 = item1->stack_var;
                increase_assignment_expression_dupped_count(item0);
                increase_assignment_expression_dupped_count(item1);
            }
            break;
        }
        case INS_DUP2_X1: {
            jd_val *item0 = ins->stack_in->vals[0];
            jd_val *item1 = ins->stack_in->vals[1];
            jd_var *var0 = item0->stack_var;
            jd_var *var1 = item1->stack_var;
            if (jd_stack_val_is_compute_category2(item0) &&
                jd_stack_val_is_compute_category1(item1)) {
                // ..., value2, value1 →
                // ..., value1, value2, value1
                increase_assignment_expression_dupped_count(item0);
            }
            else {
                // ..., value3, value2, value1 →
                // ..., value2, value1, value3, value2, value1
                increase_assignment_expression_dupped_count(item0);
                increase_assignment_expression_dupped_count(item1);
            }
            break;
        }
        case INS_DUP2_X2: {
            jd_val *item0 = ins->stack_in->vals[0];
            jd_val *item1 = ins->stack_in->vals[1];
            jd_var *var0 = item0->stack_var;
            jd_var *var1 = item1->stack_var;

            if (jd_stack_val_is_compute_category2(item0) &&
                jd_stack_val_is_compute_category2(item1)) {
                // ..., value2, value1 →
                //..., value1, value2, value1
                increase_assignment_expression_dupped_count(item0);
                increase_assignment_expression_dupped_count(item1);
                break;
            }
            jd_val *item2 = ins->stack_in->vals[2];

            if (jd_stack_val_is_compute_category2(item0) &&
                jd_stack_val_is_compute_category1(item1) &&
                jd_stack_val_is_compute_category1(item2)) {
                // ..., value3, value2, value1 →
                // ..., value1, value3, value2, value1
                increase_assignment_expression_dupped_count(item0);
                break;
            }

            if (jd_stack_val_is_compute_category1(item0) &&
                jd_stack_val_is_compute_category1(item1) &&
                jd_stack_val_is_compute_category2(item2)) {
                // ..., value3, value2, value1 →
                // ..., value2, value1, value3, value2, value1
                increase_assignment_expression_dupped_count(item0);
                increase_assignment_expression_dupped_count(item1);
                break;
            }
            // ..., value4, value3, value2, value1 →
            // ..., value2, value1, value4, value3, value2, value1
            increase_assignment_expression_dupped_count(item0);
            increase_assignment_expression_dupped_count(item1);
        }
    }

}

static void build_pop_expression(jd_exp *exp, jd_ins *ins)
{
    exp->type = JD_EXPRESSION_EMPTY;
    exp->ins = ins;
    jd_stack *stack_in = ins->stack_in;
    jd_val *val = stack_in->vals[0];
    jd_var *var = val->stack_var;
    var->def_count --;
    if (ins->prev != NULL && jvm_ins_is_load(ins->prev)) {
        // fix some bug: pop a load
        jd_exp *prev_exp = ins->prev->expression;
        if (prev_exp)
            exp_mark_nopped(prev_exp);
    }
}

static void build_stack_var_copy(jd_method *m, jd_exp *exp, jd_ins *ins)
{
    hashmap *map = m->stack_phi_node_copies;
    list_object *list = hashmap_get_int_to_object(map, ins->offset);
    if (list == NULL)
        return;

    for (int i = 0; i < list->size; ++i) {
        jd_var_copy *copy = lget_obj(list, i);
        jd_exp_def_var *def_stack_var = make_obj(jd_exp_def_var);
        def_stack_var->list = make_exp_list(2);
        jd_exp *left = &def_stack_var->list->args[0];
        jd_exp *right = &def_stack_var->list->args[1];
        left->type = JD_EXPRESSION_STACK_VAR;
        left->data = copy->left;
        copy->left->def_count ++;
        right->type = JD_EXPRESSION_STACK_VAR;
        right->data = copy->right;
        copy->right->use_count ++;
        copy->right->redef_count ++;

        jd_exp *stack_var_exp = make_obj(jd_exp);
        stack_var_exp->ins = ins;
        stack_var_exp->idx = m->expressions->size;
        stack_var_exp->type = JD_EXPRESSION_DEFINE_STACK_VAR;
        stack_var_exp->data = def_stack_var;
        stack_var_exp->block = ins->block;

        ladd_obj(m->expressions, stack_var_exp);
    }
}

static void build_lambda_expression(jd_exp *exp, jd_lambda *lda, jd_method *tm)
{
    jd_exp_invoke *invoke = NULL;
    jd_exp *lambda_expression = NULL;
    if (exp_is_assignment(exp)) {
        jd_exp_assignment *assignment = exp->data;
        invoke = assignment->right->data;
        lambda_expression = assignment->right;
    }
    else {
        invoke = exp->data;
        lambda_expression = exp;
    }

    jd_exp_lambda *exp_lambda = make_obj(jd_exp_lambda);
    exp_lambda->list = make_obj(jd_exp_list);
    exp_lambda->list->len = invoke->list->len;
    if (exp_lambda->list->len > 0) {
        exp_lambda->list->args = make_obj_arr(jd_exp, exp_lambda->list->len);
        memcpy(exp_lambda->list->args, invoke->list->args,
               sizeof(jd_exp) * invoke->list->len);
    }
    exp_lambda->method = tm;
    exp_lambda->lambda = lda;

    exp_lambda->descriptor = lda->target_method->descriptor;
    exp_lambda->class_name = lda->target_method->class_name;
    exp_lambda->method_name = lda->target_method->name;
    exp_lambda->is_static = lda->target_method->kind == 6;

    lambda_expression->type = JD_EXPRESSION_LAMBDA;
    lambda_expression->data = exp_lambda;
}

void build_expression(jd_exp *exp, jd_ins *ins)
{
    if (ins_is_unreached(ins)) {
        build_empty_expression(exp, ins);
        return;
    }

    switch (ins->code) {
        case INS_ACONST_NULL: //aconst_null
        case INS_ICONST_M1: //iconst_m1
        case INS_ICONST_0: //iconst_0
        case INS_ICONST_1: //iconst_1
        case INS_ICONST_2: //iconst_2
        case INS_ICONST_3: //iconst_3
        case INS_ICONST_4: //iconst_4
        case INS_ICONST_5: //iconst_5
        case INS_LCONST_0: //lconst_0
        case INS_LCONST_1: //lconst_1
        case INS_FCONST_0: //fconst_0
        case INS_FCONST_1: //fconst_1
        case INS_FCONST_2: //fconst_2
        case INS_DCONST_0: //dconst_0
        case INS_DCONST_1: //dconst_1
        case INS_BIPUSH: //bipush
        case INS_SIPUSH: //sipush
        case INS_LDC: //ldc
        case INS_LDC_W: //ldc_w
        case INS_LDC2_W: //ldc2_w
            build_const_expression(exp, ins);
            break;
        case INS_ILOAD: //iload
        case INS_LLOAD: //lload
        case INS_FLOAD: //fload
        case INS_DLOAD: //dload
        case INS_ALOAD: //aload
        case INS_ILOAD_0: //iload_0
        case INS_ILOAD_1: //iload_1
        case INS_ILOAD_2: //iload_2
        case INS_ILOAD_3: //iload_3
        case INS_LLOAD_0: //lload_0
        case INS_LLOAD_1: //lload_1
        case INS_LLOAD_2: //lload_2
        case INS_LLOAD_3: //lload_3
        case INS_FLOAD_0: //fload_0
        case INS_FLOAD_1: //fload_1
        case INS_FLOAD_2: //fload_2
        case INS_FLOAD_3: //fload_3
        case INS_DLOAD_0: //dload_0
        case INS_DLOAD_1: //dload_1
        case INS_DLOAD_2: //dload_2
        case INS_DLOAD_3: //dload_3
        case INS_ALOAD_0: //aload_0
        case INS_ALOAD_1: //aload_1
        case INS_ALOAD_2: //aload_2
        case INS_ALOAD_3: //aload_3
            build_load_expression(exp, ins);
            break;
        case INS_IALOAD: //iaload
        case INS_LALOAD: //laload
        case INS_FALOAD: //faload
        case INS_DALOAD: //daload
        case INS_AALOAD: //aaload
        case INS_BALOAD: //baload
        case INS_CALOAD: //caload
        case INS_SALOAD: //saload
            build_array_load_expression(exp, ins);
            break;
        case INS_ISTORE: //istore
        case INS_LSTORE: //lstore
        case INS_FSTORE: //fstore
        case INS_DSTORE: //dstore
        case INS_ASTORE: //astore
        case INS_ISTORE_0: //istore_0
        case INS_ISTORE_1: //istore_1
        case INS_ISTORE_2: //istore_2
        case INS_ISTORE_3: //istore_3
        case INS_LSTORE_0: //lstore_0
        case INS_LSTORE_1: //lstore_1
        case INS_LSTORE_2: //lstore_2
        case INS_LSTORE_3: //lstore_3
        case INS_FSTORE_0: //fstore_0
        case INS_FSTORE_1: //fstore_1
        case INS_FSTORE_2: //fstore_2
        case INS_FSTORE_3: //fstore_3
        case INS_DSTORE_0: //dstore_0
        case INS_DSTORE_1: //dstore_1
        case INS_DSTORE_2: //dstore_2
        case INS_DSTORE_3: //dstore_3
        case INS_ASTORE_0: //astore_0
        case INS_ASTORE_1: //astore_1
        case INS_ASTORE_2: //astore_2
        case INS_ASTORE_3: //astore_3
            build_store_expression(exp, ins);
            break;
        case INS_IASTORE: //iastore
        case INS_LASTORE: //lastore
        case INS_FASTORE: //fastore
        case INS_DASTORE: //dastore
        case INS_AASTORE: //aastore
        case INS_BASTORE: //bastore
        case INS_CASTORE: //castore
        case INS_SASTORE: //sastore
            build_array_store_expression(exp, ins);
            break;
        case INS_POP: //pop
        case INS_POP2: //pop2
            build_pop_expression(exp, ins);
            break;
        case INS_DUP: //dup
        case INS_DUP_X1: //dup_x1
        case INS_DUP_X2: //dup_x2
        case INS_DUP2: //dup2
        case INS_DUP2_X1: //dup2_x1
        case INS_DUP2_X2: //dup2_x2
             build_dup_expression(exp, ins);
             break;
        case INS_SWAP: //swap
            build_empty_expression(exp, ins);
            break;
        case INS_IADD: //iadd
        case INS_LADD: //ladd
        case INS_FADD: //fadd
        case INS_DADD: //dadd
        case INS_ISUB: //isub
        case INS_LSUB: //lsub
        case INS_FSUB: //fsub
        case INS_DSUB: //dsub
        case INS_IMUL: //imul
        case INS_LMUL: //lmul
        case INS_FMUL: //fmul
        case INS_DMUL: //dmul
        case INS_IDIV: //idiv
        case INS_LDIV: //ldiv
        case INS_FDIV: //fdiv
        case INS_DDIV: //ddiv
        case INS_IREM: //irem
        case INS_LREM: //lrem
        case INS_FREM: //frem
        case INS_DREM: //drem
        case INS_ISHL: //ishl
        case INS_LSHL: //lshl
        case INS_ISHR: //ishr
        case INS_LSHR: //lshr
        case INS_IUSHR: //iushr
        case INS_LUSHR: //lushr
        case INS_IAND: //iand
        case INS_LAND: //land
        case INS_IOR: //ior
        case INS_LOR: //lor
        case INS_IXOR: //ixor
        case INS_LXOR: //lxor
            build_operator_expression(exp, ins);
            break;
        case INS_INEG: //ineg
        case INS_LNEG: //lneg
        case INS_FNEG: //fneg
        case INS_DNEG: //dneg
            build_neg_expression(exp, ins);
            break;
        case INS_INSTANCEOF: //instanceof
            build_instance_of_expression(exp, ins);
            break;
        case INS_IINC: //iinc
            build_iinc_expression(exp, ins);
            break;
        case INS_I2L: //i2l
        case INS_I2F: //i2f
        case INS_I2D: //i2d
        case INS_L2I: //l2i
        case INS_L2F: //l2f
        case INS_L2D: //l2d
        case INS_F2I: //f2i
        case INS_F2L: //f2l
        case INS_F2D: //f2d
        case INS_D2I: //d2i
        case INS_D2L: //d2l
        case INS_D2F: //d2f
        case INS_I2B: //i2b
        case INS_I2C: //i2c
        case INS_I2S: //i2s
        case INS_CHECKCAST: //checkcast
            build_cast_expression(exp, ins);
            break;
        case INS_LCMP: //lcmp
        case INS_FCMPL: //fcmpl
        case INS_FCMPG: //fcmpg
        case INS_DCMPL: //dcmpl
        case INS_DCMPG: //dcmpg
            build_cmp_expression(exp, ins);
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
        case INS_IFNULL: //ifnull
        case INS_IFNONNULL: //ifnonnull
            build_if_expression(exp, ins);
            break;
        case INS_TABLESWITCH: //tableswitch
        case INS_LOOKUPSWITCH: //lookupswitch
            build_switch_expression(exp, ins);
            break;
        case INS_IRETURN: //ireturn
        case INS_LRETURN: //lreturn
        case INS_FRETURN: //freturn
        case INS_DRETURN: //dreturn
        case INS_ARETURN: //areturn
        case INS_RETURN: //return
        {
            build_return_expression(exp, ins);
            build_last_void_return(exp, ins);
            break;
        }
        case INS_GOTO: //goto
        case INS_GOTO_W: //goto_w
            build_goto_expression(exp, ins);
            break;
        case INS_GETSTATIC: //getstatic
            build_get_static_expression(exp, ins);
            break;
        case INS_PUTSTATIC: //putstatic
            build_put_static_expression(exp, ins);
            break;
        case INS_GETFIELD: //getfield
            build_get_field_expression(exp, ins);
            break;
        case INS_PUTFIELD: //putfield
            build_put_field_expression(exp, ins);
            break;
        case INS_INVOKEVIRTUAL: //invokevirtual
        case INS_INVOKESPECIAL: //invokespecial
        case INS_INVOKESTATIC: //invokestatic
        case INS_INVOKEINTERFACE: //invokeinterface
        case INS_INVOKEDYNAMIC: //invokedynamic
            build_invoke_expression(exp, ins);
            break;
        case INS_NEW: //new
            build_uninitialize_expression(exp, ins);
            break;
        case INS_ARRAYLENGTH:
            build_arraylength_expression(exp, ins);
            break;
        case INS_ATHROW:
            build_athrow_expression(exp, ins);
            break;
        case INS_NEWARRAY: //newarray
        case INS_ANEWARRAY: //anewarray
            build_new_array_expression(exp, ins);
            break;
        case INS_MULTIANEWARRAY: //multianewarray
            build_new_array_expression(exp, ins);
            break;
        case INS_MONITORENTER:
            build_monitorenter_expression(exp, ins);
            break;
        case INS_MONITOREXIT:
            build_monitorexit_expression(exp, ins);
            break;
        case INS_WIDE: //wide
        {
            switch (ins->param[0]) {
                case INS_ILOAD: //iload
                case INS_LLOAD: //lload
                case INS_FLOAD: //fload
                case INS_DLOAD: //dload
                case INS_ALOAD: //aload
                    build_load_expression(exp, ins);
                    break;
                case INS_ISTORE: //istore
                case INS_LSTORE: //lstore
                case INS_FSTORE: //fstore
                case INS_DSTORE: //dstore
                case INS_ASTORE: //astore
                    build_store_expression(exp, ins);
                    break;
                case INS_IINC: //iinc
                    build_iinc_expression(exp, ins);
                    break;
            }
        }
        default:
            break;
    }
}

static void follow_lambda(jd_method *m, jd_exp *exp)
{
    jd_lambda *lambda = identify_lambda_expression(m, exp);
    if (lambda == NULL)
        return;
    ladd_obj(m->lambdas, lambda);
    int in_current_class = 0;
    jclass_file *jc = m->meta;

    for (int i = 0; i < be16toh(jc->methods_count); ++i) {
        jmethod *jm = &jc->methods[i];
        string name = pool_str(m->meta, jm->name_index);
        u2 target_index = lambda->target_method->descriptor->index;

        if (STR_EQL(lambda->target_method->name, name) &&
            !STR_EQL(name, m->name) &&
            target_index == jm->descriptor_index) {
            jd_method *target_method = make_obj(jd_method);
            ladd_obj(jc->jfile->methods, target_method);

            jvm_method(m->meta, target_method, jm);
            build_lambda_expression(exp, lambda, target_method);
            method_mark_lambda(target_method);
            in_current_class = 1;
        }
    }

    if (!in_current_class)
        build_lambda_expression(exp, lambda, NULL);
}

void instruction_to_expression(jd_method *m)
{
    DEBUG_PRINT("m: %s(%s)\n", m->name,
                lstring_join(m->desc->list, ","));

    m->expressions = linit_object();
    m->lambdas = linit_object();

    for (int i = 0; i < m->instructions->size; ++i) {
        jd_ins *ins = get_ins(m, i);
        jd_exp *expression = make_obj(jd_exp);
        expression->ins = ins;
        expression->idx = m->expressions->size;
        ins->expression = expression;
        build_expression(expression, ins);
        expression->block = ins->block;
        ladd_obj(m->expressions, expression);

        if (exp_is_invokedynamic(expression)) {
            follow_lambda(m, expression);
            identify_string_concat_expression(m, expression);
        }
    }
}
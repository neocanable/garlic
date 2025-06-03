#include "decompiler/structure.h"
#include "decompiler/expression.h"
#include "decompiler/klass.h"
#include "jvm_type_analyse.h"
#include "decompiler/signature.h"
#include "jvm/jvm_simulator.h"
#include "common/str_tools.h"
#include "parser/class/class_tools.h"

static inline string get_array_item_type(jd_ins *ins)
{
    switch(ins->code) {
        case INS_BASTORE:
            return str_dup(g_str_byte);
        case INS_CASTORE:
            return str_dup(g_str_char);
        case INS_SASTORE:
            return str_dup(g_str_short);
        default: {
            fprintf(stderr, "get_array_item_type: unknown ins %s\n",
                    ins->name);
            abort();
        }
    }
}

static inline bool is_hide_int_type(string descriptor)
{
    return STR_EQL(descriptor, "Z") ||
           STR_EQL(descriptor, "B") ||
           STR_EQL(descriptor, "C") ||
           STR_EQL(descriptor, "S");
}

static inline bool is_hide_int_class(string class_name)
{
    return STR_EQL(class_name, g_str_boolean) ||
           STR_EQL(class_name, g_str_byte) ||
           STR_EQL(class_name, g_str_char) ||
           STR_EQL(class_name, g_str_short);
}

static void add_stack_val_type(jd_ins *ins,
                               jd_var *var,
                               string type,
                               int slot)
{
    jd_method *m = ins->method;
    for (int i = 0; i < m->types->size; ++i) {
        jd_val_type *item = lget_obj(m->types, i);
        if (slot > 0 && item->slot == slot &&
            STR_EQL(item->simple_class_name, type))
            return;
    }
    DEBUG_STACK_PRINT("[add stack type]: type:%s ins:%s(%d) slot: %d\n",
            type, ins->name, ins->offset, slot);
    jd_val_type *item = make_obj(jd_val_type);
    item->slot = slot;
    item->ins  = ins;
    item->var  = var;
    item->simple_class_name = str_dup(type);
    ladd_obj(m->types, item);
}


/**
 *  ins_getfield is boolean/char/short
 *  ins_getstatic is boolean/char/short
 *  ins_getfield is boolean/char/short
 *  ins_putstatic is boolean/char/short
 *  ins_instanceof is boolean
 **/
void jvm_int_type_analyze(jd_ins *ins)
{
    jd_stack *stack_in = ins->stack_in;
    jd_val *val = stack_in->depth == 0 ? NULL : stack_in->vals[0];
    jd_ins *vins = val == NULL ? NULL : val->ins;
    switch(ins->code) {
        case INS_IRETURN: {
            jd_descriptor *descriptor = ins->method->desc;
            string type = class_simple_name(descriptor->str_return);
            string val_class_name = val->data->cname;
            if (vins == NULL)
                break;
            int slot = jvm_ins_is_xiload(vins) ? jvm_ins_load_slot(vins) : -1;
            // m return type is not int
            // stack_in->vals[0] must be return type
            if (STR_EQL(type, val_class_name))
                break;

            if (jvm_ins_is_xiload(vins) || jvm_ins_is_boolean_const(vins))
                add_stack_val_type(vins, val->stack_var, type, slot);

            break;
        }
        case INS_BASTORE:
        case INS_CASTORE:
        case INS_SASTORE: {
            if (vins != NULL &&
                    jvm_ins_is_xiload(vins))
                add_stack_val_type(vins,
                                   val->stack_var,
                                   get_array_item_type(ins),
                                   jvm_ins_load_slot(vins));
            break;
        }
        case INS_INVOKEDYNAMIC:
        case INS_INVOKEINTERFACE:
        case INS_INVOKESPECIAL:
        case INS_INVOKESTATIC:
        case INS_INVOKEVIRTUAL: {
            jd_descriptor *descriptor = jvm_invoke_descriptor(ins);

            for (int i = 0; i < descriptor->list->size; ++i) {
                string type = lget_string(descriptor->list, i);
                jd_val *val = stack_in->vals[i];
                jd_ins *vins = val->ins;
                jd_var *var = val->stack_var;
                string val_class_name = val->data->cname;

                if (!is_hide_int_type(type) || vins == NULL)
                    continue;

                string simple_name = class_simple_name(type);
                if (STR_EQL(simple_name, val_class_name))
                    continue;

                int slot = -1;
                if (jvm_ins_is_xiload(vins))
                    slot = jvm_ins_load_slot(vins);

                if (jvm_ins_is_xiload(vins) || jvm_ins_is_boolean_const(vins))
                    add_stack_val_type(vins, var, simple_name, slot);

            }
            break;
        }
        case INS_PUTFIELD: {
            string descriptor = field_descriptor_of_ins(ins);
            if (!is_hide_int_type(descriptor))
                break;
            string class_name = class_simple_name(descriptor);
            string val_class_name = val->data->cname;
            jd_var *var = val->stack_var;
            if (STR_EQL(val_class_name, class_name))
                break;
            add_stack_val_type(vins, var, class_name, -1);
            break;
        }
        case INS_PUTSTATIC: {
            string descriptor = field_descriptor_of_ins(ins);
            if (!is_hide_int_type(descriptor))
                break;
            string class_name = class_simple_name(descriptor);
            jd_var *var = val->stack_var;
            if (STR_EQL(class_name, val->data->cname))
                break;
            add_stack_val_type(vins, var, class_name, -1);
            break;
        }
        case INS_ISTORE:
        case INS_ISTORE_0:
        case INS_ISTORE_1:
        case INS_ISTORE_2:
        case INS_ISTORE_3: {
            int slot = jvm_ins_store_slot(ins);
            jd_val *local_var = find_local_variable(ins->stack_in, slot);
            if (local_var == NULL || val->data->primitive == NULL)
                break;

            string lclass_name = local_var->data->cname;
            if ((is_hide_int_class(lclass_name)) &&
                STR_EQL(val->data->cname, g_str_int)) {
                val->data->cname = lclass_name;
                jd_var *var = val->stack_var;
                add_stack_val_type(vins, var, lclass_name , slot);
            }

            break;
        }
        default:
            break;
    }
}

void jvm_fix_type(jd_method *m)
{
    for (int i = 0; i < m->types->size; ++i) {
        jd_val_type *item = lget_obj(m->types, i);

        for (int j = 0; j < m->stack_variables->size; ++j) {
            jd_var *v = lget_obj(m->stack_variables, j);
            if (v != item->var)
                continue;
            jd_stack *out = v->ins->stack_out;
            jd_val *val = out->vals[0];
            val->data->cname = item->simple_class_name;
        }

        if (item->slot > 0) {
            // means that is local variable
            for (int j = 0; j < m->instructions->size; ++j) {
                jd_ins *ins = get_ins(m, j);
                if (jvm_ins_is_xistore(ins) &&
                        jvm_ins_store_slot(ins) == item->slot &&
                    item->ins->offset > ins->offset) {
                    if (ins->stack_in == NULL) // unreachable
                        continue;
                    jd_stack *stack_in = ins->stack_in;
                    jd_val *val = stack_in->vals[0];
                    val->data->cname = item->simple_class_name;
                }
            }
        }
        else {
            jd_ins *ins = item->ins;
            jd_stack *stack_out = ins->stack_out;
            jd_val *val = stack_out->vals[0];
            val->data->cname = item->simple_class_name;
        }
    }
}

static jattr* attribute_of(jattr_code *code, string name)
{
    for (int i = 0; i < be16toh(code->attributes_count); ++i) {
        jattr *attr = &code->attributes[i];
        if (STR_EQL(attr->name, name))
            return attr;
    }
    return NULL;
}

jattr_local_variable* match_local_variable(jd_method *m, jd_ins *ins, int slot)
{
    jmethod *meta_method = m->meta_method;
    jattr_code *code = meta_method->code_attribute;
    jattr_local_variable *result = NULL;
    jattr *local_vt = attribute_of(code, "LocalVariableTable");

    jattr_lvt *lvt = NULL;
    if (local_vt != NULL)
        lvt = (jattr_lvt *) local_vt->info;
    else
        return result;

    for (int i = 0; i < be16toh(lvt->local_variable_table_length); ++i) {
        jattr_local_variable *local = &lvt->local_variable_table[i];

        jd_ins *next = ins == NULL ? NULL : ins->next;
        if (next != NULL && next->offset == be16toh(local->start_pc) &&
            slot == (int)be16toh(local->index)) {
            result = local;
            break;
        }
        else if (ins == NULL && 0 == be16toh(local->start_pc) &&
                 slot == (int)be16toh(local->index)) {
            result = local;
            break;
        }
    }

    return result;
}

jattr_ltv* match_local_type_variable(jd_method *m, jd_ins *ins, int slot)
{
    jmethod *meta_method = m->meta_method;
    jattr_code *code = meta_method->code_attribute;
    jattr_ltv *result = NULL;
    jattr *local_vtt = attribute_of(code, "LocalVariableTypeTable");

    jattr_lvtt *lvtt = NULL;
    if (local_vtt != NULL)
        lvtt = (jattr_lvtt *) local_vtt->info;
    else
        return result;

    for (int i = 0; i < be16toh(lvtt->local_variable_type_table_length); ++i) {
        jattr_ltv *local = &lvtt->local_variable_type_table[i];

        jd_ins *next = ins == NULL ? NULL : ins->next;
        if (next != NULL && next->offset == be16toh(local->start_pc) &&
            slot == (int)be16toh(local->index)) {
            result = local;
            break;
        }
        else if (ins == NULL && 0 == be16toh(local->start_pc) &&
                 slot == (int)be16toh(local->index)) {
            result = local;
            break;
        }
    }

    return result;
}

bool jvm_has_debug(jd_method *m, jd_ins *ins, int slot)
{
    jattr_local_variable *lv = match_local_variable(m, ins, slot);
    jattr_local_type_variable *ltv = match_local_type_variable(m, ins, slot);
    return ltv != NULL || lv != NULL;
}

jd_matched_debug *matched_local_variable(jd_method *m, jd_ins *ins, int slot)
{
    jattr_local_variable *lv = match_local_variable(m, ins, slot);
    jattr_local_type_variable *ltv = match_local_type_variable(m, ins, slot);
    string name = NULL;
    string signature = NULL;
    uint16_t offset = -1;

    if (ltv != NULL) {
        name = pool_str(m->meta, ltv->name_index);
        signature = pool_str(m->meta, ltv->signature_index);
        offset = be16toh(ltv->start_pc);
    }
    else if (lv != NULL) {
        name = pool_str(m->meta, lv->name_index);
        signature = pool_str(m->meta, lv->descriptor_index);
        offset = be16toh(lv->start_pc);
    }

    if (ltv != NULL || lv != NULL) {
        jd_matched_debug *r = make_obj(jd_matched_debug);
        r->name = name;
        r->signature = signature;
        type_sig *ts = parse_field_signature(signature);
        string full = field_type_sig_to_s(ts);
        r->fname = full;
        r->sname = class_simple_name(full);
        return r;
    }
    return NULL;
}

void jvm_debug(jd_method *m, jd_ins *ins, jd_val *val, int slot)
{
    jattr_local_variable *lv = match_local_variable(m, ins, slot);
    jattr_local_type_variable *ltv = match_local_type_variable(m, ins, slot);
    string name = NULL;
    string signature = NULL;
    uint16_t offset = -1;

    if (ltv != NULL) {
        name = pool_str(m->meta, ltv->name_index);
        signature = pool_str(m->meta, ltv->signature_index);
        offset = be16toh(ltv->start_pc);
    }
    else if (lv != NULL) {
        name = pool_str(m->meta, lv->name_index);
        signature = pool_str(m->meta, lv->descriptor_index);
        offset = be16toh(lv->start_pc);
    }

    if (ltv != NULL || lv != NULL) {
        val->name = name;
        val->name_type = JD_VAR_NAME_DEBUG;
        type_sig *ts = parse_field_signature(signature);
        string full = field_type_sig_to_s(ts);
    }
}

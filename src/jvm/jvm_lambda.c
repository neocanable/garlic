#include "parser/class/class_tools.h"
#include "common/str_tools.h"
#include "jvm_lambda.h"
#include "decompiler/expression.h"
#include "decompiler/structure.h"
#include "jvm/jvm_expression_builder.h"
#include "jvm/jvm_descriptor.h"

#define LAMBDA_CLASS_NAME "java/lang/invoke/LambdaMetafactory"
#define STRING_CONCAT_CLASS_NAME "java/lang/invoke/StringConcatFactory"

static jd_method_sig* get_method_sig(jclass_file *jc,
                                     jconst_method_handle *handle)
{
    switch (handle->reference_kind) {
        case 5: // REF_invokeVirtual
        case 6: // REF_invokeStatic
        case 7: // REF_invokeSpecial
        case 8: // REF_newInvokeSpecial
        case 9: // REF_invokeInterface -> CONSTANT_InterfaceMethodref_info
        {
            u2 reference_index = handle->reference_index;
            jcp_info *method_ref_info = pool_item(jc, reference_index);
            jconst_methodref *method_ref = method_ref_info->info->methodref;

            u2 class_index = method_ref->class_index;
            u2 nt_index = method_ref->name_and_type_index;
            jcp_info *class_info = pool_item(jc, class_index);
            jcp_info *name_info = pool_item(jc, nt_index);
            jconst_name_and_type *name_type = name_info->info->name_and_type;
            jcp_info *name = pool_item(jc, name_type->name_index);
            jconst_utf8 *name_utf8 = name->info->utf8;

            u2 d = name_type->descriptor_index;
            jd_descriptor *descriptor = jvm_descriptor(jc->jfile, d);

            jd_method_sig *method_sig = make_obj(jd_method_sig);
            size_t len = be16toh(name_utf8->length);
            method_sig->name = x_alloc(len + 1);
            memcpy(method_sig->name, name_utf8->bytes, len);
            method_sig->name[len] = '\0';
            method_sig->kind = handle->reference_kind;
            method_sig->class_name = get_class_name(jc, class_info);
            method_sig->descriptor = descriptor;

            return method_sig;
        }
        default:
            return NULL;
    }
}

jclass_bootstrap_method* get_bootstrap_method_attr(jd_ins *ins)
{
    jclass_file *jc = ins->method->meta;
    jattr_bootstrap_methods *bootstrap_methods_attr = NULL;
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jcp_info *info = pool_item(ins->method->meta, index);
    jconst_invoke_dynamic *dynamic = info->info->invoke_dynamic;

    for (int i = 0; i < be16toh(jc->attributes_count); ++i) {
        jattr *attr = &jc->attributes[i];
        if (!STR_EQL(attr->name, "BootstrapMethods"))
            continue;
        bootstrap_methods_attr = (jattr_bootstrap_methods*)attr->info;
    }
    if (bootstrap_methods_attr == NULL)
        return NULL;
    u2 bootstrap_method_id = dynamic->bootstrap_method_attr_index;
    int method_index = be16toh(bootstrap_method_id);
    return &bootstrap_methods_attr->bootstrap_methods[method_index];
}

jd_method_sig* get_method_sig_of_ins(jd_ins *ins)
{
    jd_method *m = ins->method;
    jclass_bootstrap_method *attr = get_bootstrap_method_attr(ins);
    if (attr == NULL)
        return NULL;
    u2 index = attr->bootstrap_method_ref;
    jcp_info *method_handle = pool_item(m->meta, index);
    jconst_method_handle *handle = method_handle->info->method_handle;
    return get_method_sig(ins->method->meta, handle);
}

bool method_sig_is_lambda(jd_method_sig *method_sig)
{
    if (method_sig == NULL)
        return false;
    if ((STR_EQL(method_sig->name, "metafactory") ||
         STR_EQL(method_sig->name, "altMetafactory")) &&
        STR_EQL(method_sig->class_name, LAMBDA_CLASS_NAME))
        return true;
    return false;
}

bool method_sig_is_str_concat(jd_method_sig *method_sig)
{
    if (method_sig == NULL)
        return false;
    return (STR_EQL(method_sig->name, "makeConcatWithConstants") ||
            STR_EQL(method_sig->name, "makeConcat")) &&
           STR_EQL(method_sig->class_name, STRING_CONCAT_CLASS_NAME);
}

static inline int next_marker(string str, int start) {
    int i = start;
    while (i < strlen(str)) {
        if (str[i] == '\x01' || str[i] == '\x02')
            return i;
        ++i;
    }
    return -1;
}

static int get_concat_exp_size(string pattern, 
                            int constants_num, int args_num)
{
    if (constants_num > 1)
        return strlen(pattern);

    int size = 0;
    int i = 0;
    while (i < strlen(pattern)) {
        int next_marker_index = next_marker(pattern, i);
        if (next_marker_index == -1) {
            size ++;
            break;
        }
        if (i < next_marker_index)
            size ++;
        i = next_marker_index + 1;
    }
    return size + args_num;
}

void identify_string_concat_expression(jd_method *m, jd_exp *exp)
{
    jd_ins *ins = exp->ins;

    jclass_bootstrap_method *b_method = get_bootstrap_method_attr(ins);
    jd_method_sig *method_sig = get_method_sig_of_ins(ins);

    if (!method_sig_is_str_concat(method_sig))
        return;

    jd_exp *invoke = NULL;
    if (exp_is_assignment(exp)) {
        jd_exp_assignment *assignment = exp->data;
        invoke = assignment->right;
    }
    else {
        invoke = exp;
    }
    jd_exp_reader *reader = invoke->data;
    jd_exp_list *args_list = reader->list;
    int constants_num = be16toh(b_method->num_bootstrap_arguments);
    int args_num = args_list->len;
    u2 arg_index = b_method->bootstrap_arguments[0];
    jcp_info *arg_info = pool_item(m->meta, arg_index);
    string pattern = arg_info->readable;
    size_t argument_len = strlen(pattern);

    jd_exp_str_concat *exp_concat = make_obj(jd_exp_str_concat);
    int list_size = get_concat_exp_size(pattern, constants_num, args_num);
    exp_concat->list = make_exp_list(list_size);
    int exp_index = 0;

    if (constants_num == 1) {
        // constants_num == 1
        // means that the first argument is a constant string with patten
        // and marker always be '\x01'
        int i = 0;
        int argument_index = 0;
        while (i < argument_len) {
            int next_marker_index = next_marker(pattern, i);

            if (next_marker_index == -1 &&
                argument_index >= args_list->len) {
                string tmp = sub_str(pattern, i, argument_len);

                jd_exp *concat_item = &exp_concat->list->args[exp_index];
                build_string_exp(concat_item, tmp);

                DEBUG_PRINT("[break concat] %s arg %d: %s\n",
                            m->name, i, tmp);
                break;
            }
            size_t len = next_marker_index - i + 1;
            if (len > 1) {
                string tmp = sub_str(pattern, i, next_marker_index - 1);
                jd_exp *concat_item = &exp_concat->list->args[exp_index];
                build_string_exp(concat_item, tmp);
                exp_index++;
                DEBUG_PRINT("[concat] %s arg %d: %s\n", m->name, i, tmp);
            }

            jd_exp *arg = &args_list->args[argument_index];
            memcpy(&exp_concat->list->args[exp_index], arg, sizeof(jd_exp));

            exp_index++;
            argument_index ++;
            i = next_marker_index + 1;
        }
    }
    else {
        int i = 0;
        int argument_index = 0;
        int constant_index = 1;

        while (i < argument_len) {
            int next_marker_index = next_marker(pattern, i);
            char marker = pattern[next_marker_index];
            if (marker == '\x01') {
                jd_exp *arg = &args_list->args[argument_index];
                jd_exp *concat_item = &exp_concat->list->args[exp_index];
                memcpy(concat_item, arg, sizeof(jd_exp));
                argument_index++;
            } else {
                u2 _index = b_method->bootstrap_arguments[constant_index];
                jcp_info *_arg_info = pool_item(m->meta, _index);
                string str = _arg_info->readable;
                jd_exp *concat_item = &exp_concat->list->args[exp_index];
                build_string_exp(concat_item, str);
                constant_index++;
            }
            exp_index++;
            i = next_marker_index + 1;
        }
    }

    invoke->type = JD_EXPRESSION_STRING_CONCAT;
    invoke->data = exp_concat;

}

jd_lambda* identify_lambda_expression(jd_method *m, jd_exp *exp)
{
    if (!exp_is_invokedynamic(exp))
        return NULL;
    jd_ins *ins = exp->ins;

    jclass_bootstrap_method *b_method = get_bootstrap_method_attr(ins);
    jd_method_sig *b_method_sig = get_method_sig_of_ins(ins);

    if (!method_sig_is_lambda(b_method_sig))
        return NULL;

    if (be16toh(b_method->num_bootstrap_arguments) < 3)
        return NULL;

    u2 arg_index = b_method->bootstrap_arguments[1];
    jcp_info *arg_info = pool_item(m->meta, arg_index);
    if (arg_info->tag != CONST_METHODHANDLE_TAG)
        return NULL;
    jconst_method_handle *handle = arg_info->info->method_handle;
    jd_method_sig *target_method = get_method_sig(m->meta, handle);

    if (target_method == NULL)
        return NULL;

    DEBUG_PRINT("[lambda] target m: "
                "cname:%s  class:%s desc:%s\n",
                target_method->name,
                target_method->class_name,
                target_method->descriptor->str);

    jd_lambda *lambda = make_obj(jd_lambda);
    lambda->exp = exp;
    lambda->target_method = target_method;

    return lambda;
}

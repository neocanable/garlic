#include "decompiler/method.h"
#include "decompiler/signature.h"

static void create_method_access_flag(jd_method *m, str_list *list)
{
    m->fn->access_flags_fn(m, list);
}

static jd_val* method_parameter_val(jd_method *m, int index)
{
    return m->fn->param_val_fn(m, index);
}

static string method_parameter_annotation(jd_method *m, int index)
{
    return m->fn->param_annotation_fn(m, index);
}

static void create_method_defination_with_signature(jd_method *m,
                                                    method_sig *sig,
                                                    str_list *list)
{
    if (STR_EQL(m->name, g_str_clinit))
        return;

    jd_stack *stack = m->enter;
    list_object *ftps = sig->formal_type_parameters;
    list_object *exception_types = sig->exception_types;
    list_object *parameter_types = sig->parameter_types;
    if (!is_list_empty(ftps)) {
        string ftp = formal_type_parameters_to_s(ftps);
        strs_concat(list, 2, ftp, " ");
    }
    if (sig->return_type != NULL) {
        string ret = field_type_sig_to_s(sig->return_type);
        strs_concat(list, 2, ret, " ");
    }

    if (method_is_init(m)) {
        string class_name = m->jfile->sname;
        str_concat(list, class_name);
    }
    else
        str_concat(list, m->name);
    str_concat(list, ("("));

    if (parameter_types->size != m->desc->list->size) {
        jd_descriptor *desc = m->desc;
        int index;
        for (int i = 0; i < desc->list->size; ++i) {
            string parameter = lget_string(desc->list, i);
            parameter = class_simple_name(parameter);
            string param_name = NULL;
            if (m->enter != NULL) {
//                index = method_is_member(m) ? i+1 : i;
//                jd_val *val = m->enter->local_vars[index];
                jd_val *val = method_parameter_val(m, i);
                param_name = val->name;
            }
            else
                param_name = str_create("p%d", i);

            string annotation = method_parameter_annotation(m, i);
            if (annotation != NULL)
                strs_concat(list, 2, annotation, (" "));

            strs_concat(list, 3, parameter, (" "), param_name);
            if (!is_list_last(desc->list, i))
                str_concat(list, (", "));
        }
    } else {
        for (int i = 0; i < parameter_types->size; ++i) {
            field_type_sig *fts = lget_obj(parameter_types, i);
            string parameter_type = field_type_sig_to_s(fts);
            string param_name = NULL;
            if (m->enter != NULL) {
                jd_val *val = method_parameter_val(m, i);
                param_name = val->name;
            } else
                param_name = str_create("p%d", i);

            string annotation = method_parameter_annotation(m, i);
            if (annotation != NULL) {
                strs_concat(list, 2, annotation, (" "));
            }
            strs_concat(list, 3, parameter_type, (" "), param_name);
            if (!is_list_last(parameter_types, i))
                str_concat(list, (", "));
        }
    }
    str_concat(list, (")"));

    if (!is_list_empty(exception_types)) {
        str_concat(list, (" throws "));
        for (int i = 0; i < exception_types->size; ++i) {
            field_type_sig *fts = lget_obj(exception_types, i);
            string exception_type = field_type_sig_to_s(fts);
            str_concat(list, exception_type);
            if (!is_list_last(exception_types, i))
                str_concat(list, (", "));
        }
    }
}

static void create_method_defination_without_signature(jd_method *m,
                                                       str_list *list)
{
    if (method_is_clinit(m)) {
        return;
    }
    string name = method_is_init(m) ? m->jfile->sname : m->name;
    jd_descriptor *desc = m->desc;
    string method_return_type = class_simple_name(desc->str_return);

    if (!method_is_init(m) && !method_is_clinit(m))
        str_concat(list, method_return_type);

    strs_concat(list, 3, " ", name, "(");

    for (int i = 0; i < desc->list->size; ++i) {
        string parameter = lget_string(desc->list, i);
        parameter = class_simple_name(parameter);
        string param_name = NULL;
        if (m->enter != NULL) {
            jd_val *val = method_parameter_val(m, i);
            param_name = val->name;
        }
        else
            param_name = str_create("p%d", i);

        string annotation = method_parameter_annotation(m, i);
        if (annotation != NULL) {
            strs_concat(list, 2, annotation, (" "));
        }
        strs_concat(list, 3, parameter, (" "), param_name);
        if (!is_list_last(desc->list, i))
            str_concat(list, (", "));
    }
    str_concat(list, (")"));
}

string create_method_defination(jd_method *m)
{
    str_list *list = str_list_init();
    create_method_access_flag(m, list);
    method_sig *sig = NULL;

    if (m->signature != NULL) {
        sig = parse_method_signature(m->signature);
    }

    jd_descriptor *desc = m->desc;

    if (method_is_enum_constructor(m)) {
        if (sig != NULL &&
            sig->parameter_types->size == desc->list->size - 2) {
            create_method_defination_with_signature(m, sig, list);
        }
        else
            create_method_defination_without_signature(m, list);
    } else {
        if (sig != NULL || 
                (sig != NULL && 
                 sig->parameter_types->size == desc->list->size))
            create_method_defination_with_signature(m, sig, list);
        else
            create_method_defination_without_signature(m, list);
    }

    string defination = str_join(list);
    m->defination = defination;
    DEBUG_PRINT("[m defination]: %s \n",defination);
    return defination;
}

string create_lambda_defination(jd_method *m)
{
    str_list *list = str_list_init();
    str_concat(list, "(");
    jd_descriptor *desc = m->desc;
    for (int i = 0; i < desc->list->size; ++i) {
        string param_name = NULL;
        if (m->enter != NULL) {
            jd_val *val = m->parameters[i];
            param_name = val->name;
        }
        else
            param_name = str_create("p%d", i);

        str_concat(list, param_name);
        if (!is_list_last(desc->list, i))
            str_concat(list, (", "));
    }
    str_concat(list, (") ->"));
    return str_join(list);
}

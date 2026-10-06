#include "decompiler/transformer/transformer.h"
#include "decompiler/method.h"

static int ctor_delegation_first_arg(jd_exp_invoke *invoke,
                                     jd_exp *expression,
                                     string object_ref_str)
{
    if (!STR_EQL(invoke->method_name, g_str_init) ||
            !STR_EQL(object_ref_str, g_str_this))
        return 0;

    return method_synthetic_parameter_count(expression->ins->method);
}

static string new_expression_class_name(jd_exp_invoke *invoke,
                                        string object_ref_str)
{
    return invoke->class_name != NULL ? invoke->class_name : object_ref_str;
}

string exp_invokespecial_to_s(jd_exp *expression)
{
    jd_exp_invoke *invoke = expression->data;
    string method_name = invoke->method_name;
    string current_method_name = expression->ins->method->name;

    string result;
    jd_exp *ref_exp = &invoke->list->args[invoke->list->len - 1];
    string object_ref_str = exp_to_s(ref_exp);
    size_t len;
    size_t new_len;
    if (STR_EQL(object_ref_str, g_str_this) &&
        (!STR_EQL(method_name, g_str_init) &&
         STR_EQL(current_method_name, method_name))) {
        len = snprintf(NULL, 0, "super.%s", method_name) + 3;
        result = x_alloc(len);
        snprintf(result, len, "super.%s", method_name);
        strcat(result, "(");
    }
    else if (STR_EQL(object_ref_str, g_str_this) &&
             STR_EQL(method_name, g_str_init) &&
             invoke_is_this_constructor(invoke, expression)) {
        len = snprintf(NULL, 0, "this") + 3;
        result = x_alloc(len);
        snprintf(result, len, "this");
        strcat(result, "(");
    }
    else if (STR_EQL(object_ref_str, g_str_this) &&
             STR_EQL(method_name, g_str_init)) {
        len = snprintf(NULL, 0, "super") + 3;
        result = x_alloc(len);
        snprintf(result, len, "super");
        strcat(result, "(");
    }
    else if (!STR_EQL(method_name, g_str_init)) {
        // invoke private m
        len = snprintf(NULL, 0, "%s.%s", object_ref_str, method_name) + 3;
        result = x_alloc(len);
        snprintf(result, len, "%s.%s", object_ref_str, method_name);
        strcat(result, "(");
    }
    else {
        string cname = new_expression_class_name(invoke, object_ref_str);
        len = snprintf(NULL, 0, "new %s", cname) + 3;
        result = x_alloc(len);
        snprintf(result, len, "new %s", cname);
        strcat(result, "(");
    }

    for (int j = ctor_delegation_first_arg(invoke, expression, object_ref_str);
         j <= invoke->list->len - 2; ++j) {
        string arg_name = exp_to_s(&invoke->list->args[j]);
        new_len = len + strlen(arg_name) + 2;

        result = x_realloc(result, len, new_len);
        strcat(result, arg_name);
        if (j != invoke->list->len - 2)
            strcat(result, ", ");
        len = new_len;
    }
    strcat(result, ")");
    result[len-1] = '\0';
    return result;
}

void exp_invokespecial_to_stream(FILE *stream,
                                 jd_node *node,
                                 jd_exp *expression)
{
    jd_exp_invoke *invoke = expression->data;
    string method_name = invoke->method_name;
    string current_method_name = expression->ins->method->name;

    jd_exp *ref_exp = &invoke->list->args[invoke->list->len - 1];
    string object_ref_str = exp_to_s(ref_exp);

    if (STR_EQL(object_ref_str, g_str_this) &&
        (!STR_EQL(method_name, g_str_init) &&
         STR_EQL(current_method_name, method_name))) {
        fprintf(stream, "super.%s(", method_name);
    }
    else if (STR_EQL(object_ref_str, g_str_this) &&
             STR_EQL(method_name, g_str_init) &&
             invoke_is_this_constructor(invoke, expression)) {
        fprintf(stream, "this(");
    }
    else if (STR_EQL(object_ref_str, g_str_this) &&
             STR_EQL(method_name, g_str_init)) {
        fprintf(stream, "super(");
    }
    else if (!STR_EQL(method_name, g_str_init)) {
        // invoke private m
        fprintf(stream, "%s.%s(", object_ref_str, method_name);
    }
    else {
        fprintf(stream, "new %s(",
                new_expression_class_name(invoke, object_ref_str));
    }

    for (int j = ctor_delegation_first_arg(invoke, expression, object_ref_str);
         j <= invoke->list->len - 2; ++j) {
        expression_to_stream(stream, node, &invoke->list->args[j]);
        if (j != invoke->list->len - 2)
            fprintf(stream, ", ");
    }
    fprintf(stream, ")");
}

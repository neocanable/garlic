#include "decompiler/transformer/transformer.h"
#include "jvm/jvm_ins.h"

string exp_invokevirtual_to_s(jd_exp *expression)
{
    jd_exp_invoke *invoke = expression->data;
    string method_name = invoke->method_name;

    jd_exp *ref_exp = &invoke->list->args[invoke->list->len - 1];
    string object_ref_str = exp_to_s(ref_exp);
    size_t len = snprintf(NULL, 0, "%s.%s", object_ref_str, method_name) + 3;
    string result = x_alloc(len);
    snprintf(result, len, "%s.%s", object_ref_str, method_name);
    size_t new_len = len;
    strcat(result, "(");

    for (int j = 0; j <= invoke->list->len - 2; ++j) {
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

void exp_invokevirtual_to_stream(FILE *stream,
                                 jd_node *node,
                                 jd_exp *expression)
{
    jd_exp_invoke *invoke = expression->data;
    string method_name = invoke->method_name;

    jd_exp *ref_exp = &invoke->list->args[invoke->list->len - 1];
    expression_to_stream(stream, node, ref_exp);
    fprintf(stream, ".%s(", method_name);

    for (int j = 0; j <= invoke->list->len - 2; ++j) {
        expression_to_stream(stream, node, &invoke->list->args[j]);
        if (j != invoke->list->len - 2)
            fprintf(stream, ", ");
    }
    fprintf(stream, ")");
}

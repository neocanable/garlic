#include "decompiler/transformer/transformer.h"
#include "jvm/jvm_ins.h"

string exp_invokestatic_to_s(jd_exp *expression)
{
    jd_exp_invoke *invoke = expression->data;
    string method_name = invoke->method_name;
    string class_name = invoke->class_name;
    size_t len = strlen(class_name) + 1 + strlen(method_name) + 3;
    size_t new_len = len;
    string s = x_alloc(len);
    memset(s, 0, len);
    strcat(s, class_name);
    strcat(s, ".");
    strcat(s, method_name);
    strcat(s, "(");
    for (int j = 0; j <= invoke->list->len - 1; ++j) {
        string arg_name = exp_to_s(&invoke->list->args[j]);
        new_len = len + strlen(arg_name) + 2;
        s = x_realloc(s, len, new_len);
        strcat(s, arg_name);
        if (j != invoke->list->len - 1)
            strcat(s, ", ");
        len = new_len;
    }
    strcat(s, ")");
    s[len] = '\0';
    return s;
}

void exp_invokestatic_to_stream(FILE *stream,
                                jd_node *node,
                                jd_exp *expression)
{
    jd_exp_invoke *invoke = expression->data;
    string method_name = invoke->method_name;
    string class_name = invoke->class_name;
    fprintf(stream, "%s.%s(", class_name, method_name);
    for (int j = 0; j <= invoke->list->len - 1; ++j) {
        expression_to_stream(stream, node, &invoke->list->args[j]);
        if (j != invoke->list->len - 1)
            fprintf(stream, ", ");
    }
    fprintf(stream, ")");
}

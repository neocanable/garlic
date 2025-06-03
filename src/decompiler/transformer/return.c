#include "decompiler/transformer/transformer.h"
#include "parser/class/class_tools.h"
#include "jvm/jvm_ins_helper.h"

string exp_return_to_s(jd_exp *expression)
{
    jd_exp_return *exp_return = expression->data;
    if (exp_return->list->len == 0)
        return str_dup("return");
    else {
        string return_str = exp_to_s(&exp_return->list->args[0]);
        return str_create("return %s", return_str);
    }
}

void exp_return_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_return *exp_return = expression->data;
    if (exp_return->list->len == 0)
        fprintf(stream, "return");
    else {
        fprintf(stream, "return ");
        expression_to_stream(stream, node, &exp_return->list->args[0]);
    }
}

#include "decompiler/transformer/transformer.h"

string exp_ternary_to_s(jd_exp *expression)
{
    jd_exp_ternary *ternary = expression->data;
    string condition = exp_to_s(&ternary->list->args[0]);
    string true_exp = exp_to_s(&ternary->list->args[1]);
    string false_exp = exp_to_s(&ternary->list->args[2]);

    return str_create("%s ? %s : %s", condition, true_exp, false_exp);
}

void exp_ternary_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_ternary *ternary = expression->data;
    expression_to_stream(stream, node, &ternary->list->args[0]);
    fprintf(stream, " ? ");
    expression_to_stream(stream, node, &ternary->list->args[1]);
    fprintf(stream, " : ");
    expression_to_stream(stream, node, &ternary->list->args[2]);
}

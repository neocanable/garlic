#include "decompiler/transformer/transformer.h"

string exp_logic_not_to_s(jd_exp *expression)
{
    jd_exp_logic_not *logic_not = expression->data;
    string condition = exp_to_s(&logic_not->list->args[0]);

    return str_create("!(%s)", condition);
}

void exp_logic_not_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_logic_not *logic_not = expression->data;
    fprintf(stream, "!(");
    expression_to_stream(stream, node, &logic_not->list->args[0]);
    fprintf(stream, ")");
}

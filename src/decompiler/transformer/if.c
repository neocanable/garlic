#include "decompiler/transformer/transformer.h"
#include "debug.h"

string exp_if_to_s(jd_exp *expression)
{
    jd_exp_if *if_exp = expression->data;
    jd_exp *condition = if_exp->expression;
    string condition_str = exp_to_s(condition);

    return str_create("%s /* target: %d */",
                      condition_str,
                      if_exp->offset);
}

string exp_if_break_to_s(jd_exp *expression)
{
    jd_exp_if *if_exp = expression->data;
    jd_exp *condition = if_exp->expression;
    string condition_str = exp_to_s(condition);

    return str_create("if (%s) break;/* target: %d */",
                      condition_str,
                      if_exp->offset);
}

void exp_if_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_if *if_exp = expression->data;
    jd_exp *condition = if_exp->expression;
    expression_to_stream(stream, node, condition);
    if (DEBUG_INS_AND_NODE_INFO)
        fprintf(stream, " /* target: %d */", if_exp->offset);
}

void exp_if_break_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_if *if_exp = expression->data;
    jd_exp *condition = if_exp->expression;
    fprintf(stream, "if (");
    expression_to_stream(stream, node, condition);
    fprintf(stream, " ) break; /* target: %d */", if_exp->offset);
}
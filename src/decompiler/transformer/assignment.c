#include "decompiler/transformer/transformer.h"

string exp_assignment_to_s(jd_exp *expression)
{
    jd_exp_assignment *assignment = expression->data;
    string op_name = get_operator_name(assignment->assign_operator);

    string lstr = exp_to_s(assignment->left);
    string rstr = exp_to_s(assignment->right);

    return str_create("%s %s %s", lstr, op_name, rstr);
}

void exp_assignment_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_assignment *assignment = expression->data;
    string op_name = get_operator_name(assignment->assign_operator);
    jd_exp *right = assignment->right;
    jd_exp *left = assignment->left;

    expression_to_stream(stream, node, left);
    fprintf(stream, " %s ", op_name);
    expression_to_stream(stream, node, right);
}

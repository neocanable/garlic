#include "decompiler/transformer/transformer.h"

string exp_assignment_chain_to_s(jd_exp *expression)
{
    jd_exp_assignment_chain *assignment_chain = expression->data;

    size_t len = 1;
    size_t old_len = len;
    string result = x_alloc(len);
    for (int i = 0; i < assignment_chain->left->size; ++i) {
        jd_exp *l = lget_obj(assignment_chain->left, i);
        string s = exp_to_s(l);
        old_len = len;
        len = len + strlen(s) + 3;
        result = x_realloc(result, old_len, len);
        strcat(result, s);
        strcat(result, " = ");
    }

    jd_exp *right = assignment_chain->right;
    string right_str = exp_to_s(right);
    old_len = len;
    len = len + strlen(right_str) + 1;
    result = x_realloc(result, old_len, len);
    strcat(result, right_str);

    result[len] = '\0';
    return result;
}

void exp_assignment_chain_to_stream(FILE *stream,
                                    jd_node *node,
                                    jd_exp *expression)
{
    jd_exp_assignment_chain *assignment_chain = expression->data;

    for (int i = 0; i < assignment_chain->left->size; ++i) {
        jd_exp *l = lget_obj(assignment_chain->left, i);
        expression_to_stream(stream, node, l);
        fprintf(stream, " = ");
    }
    jd_exp *right = assignment_chain->right;
    expression_to_stream(stream, node, right);
}

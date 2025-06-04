#include "decompiler/transformer/transformer.h"

string exp_athrow_to_s(jd_exp *expression)
{
    jd_exp_athrow *athrow = expression->data;
    string exception = exp_to_s(&athrow->list->args[0]);

    return str_create("throw %s", exception);
}

void exp_athrow_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    if (stream == NULL) {
        fprintf(stderr, "ERROR: stream is NULL in exp_athrow_to_stream\n");
        return;
    }
    
    if (expression == NULL) {
        fprintf(stderr, "ERROR: expression is NULL in exp_athrow_to_stream\n");
        return;
    }
    
    jd_exp_athrow *athrow = expression->data;
    if (athrow == NULL) {
        fprintf(stderr, "ERROR: athrow data is NULL in exp_athrow_to_stream\n");
        return;
    }
    
    if (athrow->list == NULL) {
        fprintf(stderr, "ERROR: athrow list is NULL in exp_athrow_to_stream\n");
        return;
    }
    
    if (athrow->list->len <= 0 || athrow->list->args == NULL) {
        fprintf(stderr, "ERROR: athrow list has no arguments in exp_athrow_to_stream\n");
        return;
    }
    
    fprintf(stream, "throw ");
    expression_to_stream(stream, node, &athrow->list->args[0]);
}

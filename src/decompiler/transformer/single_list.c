#include "transformer.h"
string exp_single_list_to_s(jd_exp *expression)
{
    jd_exp_single_list *single_list = expression->data;
    jd_exp_list *list = single_list->list;
    jd_exp *first = &list->args[0];
    return exp_to_s(first);
}

void exp_single_list_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression)
{
    jd_exp_single_list *single_list = expression->data;
    jd_exp_list *list = single_list->list;
    jd_exp *first = &list->args[0];
    expression_to_stream(stream, node, first);
}
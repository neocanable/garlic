#include "decompiler/transformer/transformer.h"
#include "str.h"

string exp_str_concat_to_s(jd_exp *expression)
{
    jd_exp_str_concat *str_concat_exp = expression->data;
    jd_exp_list *list = str_concat_exp->list;
    str_list *str_list = str_list_init();
    for (int i = 0; i < list->len; i++)
    {
        jd_exp *exp = &list->args[i];
        string str = exp_to_s(exp);
        str_concat(str_list, str);
        if (i < list->len - 1) {
            str_concat(str_list, " + ");
        }
    }
    return str_join(str_list);
}

void exp_str_concat_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_str_concat *str_concat = expression->data;
    jd_exp_list *list = str_concat->list;
    for (int i = 0; i < list->len; i++)
    {
        expression_to_stream(stream, node, &list->args[i]);
        if (i < list->len - 1) {
            fprintf(stream, " + ");
        }
    }
}

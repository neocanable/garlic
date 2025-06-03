#include "decompiler/transformer/transformer.h"

string exp_array_store_to_s(jd_exp *expression)
{
    jd_exp_array_store *exp = expression->data;
    string array_name = exp_to_s(&exp->list->args[2]);
    string index_name = exp_to_s(&exp->list->args[1]);
    string value_name = exp_to_s(&exp->list->args[0]);

    return str_create("%s[%s] = %s", array_name, index_name, value_name);
}

void exp_array_store_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_array_store *array_store = expression->data;
    jd_exp *array = &array_store->list->args[2];
    jd_exp *index = &array_store->list->args[1];
    jd_exp *value = &array_store->list->args[0];

    expression_to_stream(stream, node, array);
    fprintf(stream, "[");
    expression_to_stream(stream, node, index);
    fprintf(stream, "] = ");
    expression_to_stream(stream, node, value);
}

#include "decompiler/transformer/transformer.h"

string exp_store_to_s(jd_exp *expression)
{
    jd_exp_store *store = expression->data;
    jd_exp *left = &store->list->args[0];
    jd_exp *right = &store->list->args[1];
    jd_val *val   = left->data;
    string left_str     = exp_to_s(left);
    string right_str    = exp_to_s(right);

//    jd_ins *ins = val->ins;
//    if (ins == NULL)
//        ins = expression->ins;
    jd_ins *ins = expression->ins;
    jd_method *m = ins->method;

    if (m->declarations != NULL &&
        bitset_get(m->declarations, expression->idx)) {
//        string cname = NULL;
//        if (val->data->cname == NULL)
//            cname = val->data->cname;
//        else
        string class_name = val->data->cname;
        return str_create("%s %s = %s", class_name, left_str, right_str);
    }
    else {
        return str_create("%s = %s", left_str, right_str);
    }
}

void exp_store_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_store *store = expression->data;
    jd_exp *left = &store->list->args[0];
    jd_exp *right = &store->list->args[1];
    jd_val *val = left->data;
    jd_ins *ins = expression->ins;

    jd_method *m = ins->method;
    if (bitset_get(m->declarations, expression->idx)) {
//        string cname = NULL;
//        if (val->data->cname == NULL)
//            cname = val->data->cname;
//        else

        string class_name = val->data->cname;

        fprintf(stream, "%s ", class_name);
        expression_to_stream(stream, node, left);
        fprintf(stream, " = ");
        expression_to_stream(stream, node, right);
    }
    else {
        expression_to_stream(stream, node, left);
        fprintf(stream, " = ");
        expression_to_stream(stream, node, right);
    }
}

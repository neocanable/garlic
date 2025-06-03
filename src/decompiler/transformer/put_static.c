#include "decompiler/transformer/transformer.h"

string exp_put_static_to_s(jd_exp *expression)
{
    jd_exp_put_static *pc = expression->data;

    string exp_val = exp_to_s(&pc->list->args[0]);

    return str_create("%s.%s = %s", pc->class_name, pc->name, exp_val);
}

void exp_put_static_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_put_static *put_static = expression->data;
    jd_exp *val_exp = &put_static->list->args[0];

    fprintf(stream, "%s.%s = ", put_static->class_name, put_static->name);
    expression_to_stream(stream, node, val_exp);

}


#include "decompiler/transformer/transformer.h"

#include "decompiler/transformer/transformer.h"

string exp_monitorexit_to_s(jd_exp *expression)
{
    jd_exp_monitorenter *enter = expression->data;
    jd_exp *exp = &enter->list->args[0];
    return exp_to_s(exp);
}

void exp_monitorexit_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_monitorenter *enter = expression->data;
    jd_exp *exp = &enter->list->args[0];
    expression_to_stream(stream, node, exp);
}

#include "decompiler/transformer/transformer.h"

string exp_declaration_to_s(jd_exp *expression)
{
    jd_variable_scope *v = expression->data;
    return str_create("%s %s", v->cname, v->name);

}

void exp_declaration_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_variable_scope *scope = expression->data;
    fprintf(stream, "%s", scope->cname);
    fprintf(stream, " ");
    fprintf(stream, "%s", scope->name);
}

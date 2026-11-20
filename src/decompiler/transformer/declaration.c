#include "decompiler/transformer/transformer.h"

static string declaration_suffix(jd_variable_scope *scope)
{
    string c = scope->cname;
    if (STR_EQL(c, "int") || STR_EQL(c, "byte") || STR_EQL(c, "short") ||
        STR_EQL(c, "long") || STR_EQL(c, "char") || STR_EQL(c, "float") ||
        STR_EQL(c, "double"))
        return (string) " = 0";
    if (STR_EQL(c, "boolean"))
        return (string) " = false";
    return (string) " = null";
}

string exp_declaration_to_s(jd_exp *expression)
{
    jd_variable_scope *v = expression->data;
    if (v->declaration_idx == -2)
        return str_create("%s %s%s", v->cname, v->name,
                          declaration_suffix(v));
    return str_create("%s %s", v->cname, v->name);
}

void exp_declaration_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_variable_scope *scope = expression->data;
    fprintf(stream, "%s", scope->cname);
    fprintf(stream, " ");
    fprintf(stream, "%s", scope->name);
    if (scope->declaration_idx == -2)
        fprintf(stream, "%s", declaration_suffix(scope));
}

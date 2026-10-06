#include "decompiler/transformer/transformer.h"
#include "parser/class/class_tools.h"
#include "jvm/jvm_ins_helper.h"
#include "dalvik/dex_ins.h"

static bool return_object_writes_null(jd_exp *expression)
{
    jd_ins *ins = expression->ins;
    if (ins == NULL || ins->type != JD_TYPE_DALVIK)
        return false;
    if (!dex_ins_is_return_object((jd_dex_ins *) ins))
        return false;

    jd_exp_return *exp_return = expression->data;
    if (exp_return->list == NULL || exp_return->list->len == 0)
        return false;

    jd_exp *arg = &exp_return->list->args[0];
    if (arg->type != JD_EXPRESSION_CONST)
        return false;

    jd_exp_const *const_exp = arg->data;
    if (const_exp == NULL)
        return false;
    jd_val *val = const_exp->val;
    if (val == NULL || val->type != JD_VAR_INT_T)
        return false;
    if (val->data == NULL || val->data->primitive == NULL)
        return false;

    return val->data->primitive->int_val == 0;
}

string exp_return_to_s(jd_exp *expression)
{
    jd_exp_return *exp_return = expression->data;
    if (exp_return->list->len == 0)
        return str_dup("return");
    else if (return_object_writes_null(expression))
        return str_dup("return null");
    else {
        string return_str = exp_to_s(&exp_return->list->args[0]);
        return str_create("return %s", return_str);
    }
}

void exp_return_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_return *exp_return = expression->data;
    if (exp_return->list->len == 0)
        fprintf(stream, "return");
    else {
        fprintf(stream, "return ");
        if (return_object_writes_null(expression))
            fprintf(stream, "null");
        else
            expression_to_stream(stream, node, &exp_return->list->args[0]);
    }
}

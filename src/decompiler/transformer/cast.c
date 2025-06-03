#include "decompiler/transformer/transformer.h"
#include "decompiler/expression_helper.h"

string exp_cast_to_s(jd_exp *expression)
{
    jd_exp_cast *cast = expression->data;
    string original = exp_to_s(&cast->list->args[0]);

    return str_create("(%s)%s", cast->class_name, original);
}

void exp_cast_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    // https://bugs.java.com/bugdatabase/view_bug?bug_id=6246854
    // DK-6246854 : Unnecessary checkcast in generated code
    jd_exp_cast *cast = expression->data;
    jd_exp *arg = &cast->list->args[0];
    if (exp_is_cast(arg)) {
        jd_exp_cast *inner = arg->data;
        if (STR_EQL(cast->class_name, inner->class_name)) {
            expression_to_stream(stream, node, arg);
            return;
        }
    }
    else {
        fprintf(stream, "(%s)", cast->class_name);
        expression_to_stream(stream, node, arg);
    }

}

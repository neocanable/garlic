#include "decompiler/transformer/transformer.h"
#include "decompiler/expression_helper.h"

static bool cast_operand_needs_parentheses(jd_exp *arg)
{
    switch (arg->type) {
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_ASSIGNMENT:
        case JD_EXPRESSION_COMPARE:
            return true;
        default:
            return false;
    }
}

string exp_cast_to_s(jd_exp *expression)
{
    jd_exp_cast *cast = expression->data;
    jd_exp *arg = &cast->list->args[0];
    string original = exp_to_s(arg);

    if (cast_operand_needs_parentheses(arg))
        return str_create("(%s)(%s)", cast->class_name, original);
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

    bool paren = cast_operand_needs_parentheses(arg);
    fprintf(stream, "(%s)", cast->class_name);
    if (paren)
        fprintf(stream, "(");
    expression_to_stream(stream, node, arg);
    if (paren)
        fprintf(stream, ")");
}

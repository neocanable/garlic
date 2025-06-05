#include "decompiler/transformer/transformer.h"

string get_operator_name(jd_operator op)
{
    switch (op) {
        case JD_OP_ADD:
            return "+";
        case JD_OP_SUB:
            return "-";
        case JD_OP_MUL:
            return "*";
        case JD_OP_DIV:
            return "/";
        case JD_OP_REM:
            return "%";
        case JD_OP_NEG:
            return "-";
        case JD_OP_SHL:
            return "<<";
        case JD_OP_SHR:
            return ">>";
        case JD_OP_USHR:
            return ">>>";
        case JD_OP_AND:
            return "&";
        case JD_OP_OR:
            return "|";
        case JD_OP_XOR:
            return "^";
        case JD_OP_INSTANCEOF:
            return "instanceof";
        case JD_OP_EQ:
            return "==";
        case JD_OP_NE:
            return "!=";
        case JD_OP_LT:
            return "<";
        case JD_OP_LE:
            return "<=";
        case JD_OP_GT:
            return ">";
        case JD_OP_GE:
            return ">=";
        case JD_OP_LOGICAL_AND:
            return "&&";
        case JD_OP_LOGICAL_OR:
            return "||";
        case JD_OP_LOGICAL_NOT:
            return "!";
        case JD_OP_CMP:
            return "cmp";
        case JD_OP_ASSIGN:
            return "=";
        case JD_OP_ADD_ASSIGN:
            return "+=";
        case JD_OP_SUB_ASSIGN:
            return "-=";
        case JD_OP_MUL_ASSIGN:
            return "*=";
        case JD_OP_DIV_ASSIGN:
            return "/=";
        case JD_OP_REM_ASSIGN:
            return "%=";
        case JD_OP_LSHIFT_ASSIGN:
            return "<<=";
        case JD_OP_RSHIFT_ASSIGN:
            return ">>=";
        case JD_OP_USHIFT_ASSIGN:
            return ">>>=";
        case JD_OP_AND_ASSIGN:
            return "&=";
        case JD_OP_OR_ASSIGN:
            return "|=";
        case JD_OP_XOR_ASSIGN:
            return "^=";
        default:
            return (string)g_str_unknown;
    }
}

string exp_operator_to_s(jd_exp *expression)
{
    jd_exp_operator *op = expression->data;
    string left = exp_to_s(&op->list->args[0]);
    string right = exp_to_s(&op->list->args[1]);
    string op_name = get_operator_name(op->operator);

    return str_create("%s %s %s", left, op_name, right);
}

void exp_operator_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_operator *operator   = expression->data;
    jd_exp *exp_left = &operator->list->args[0];
    jd_exp *exp_right = &operator->list->args[1];
    string op = get_operator_name(operator->operator);

    expression_to_stream(stream, node, exp_left);
    fprintf(stream, " %s ", op);
    expression_to_stream(stream, node, exp_right);
}

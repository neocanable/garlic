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
        case JD_OP_NOT:
            return "~";
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

static int operator_precedence(jd_operator op)
{
    switch (op) {
        case JD_OP_ASSIGN:
        case JD_OP_ADD_ASSIGN:
        case JD_OP_SUB_ASSIGN:
        case JD_OP_MUL_ASSIGN:
        case JD_OP_DIV_ASSIGN:
        case JD_OP_REM_ASSIGN:
        case JD_OP_LSHIFT_ASSIGN:
        case JD_OP_RSHIFT_ASSIGN:
        case JD_OP_USHIFT_ASSIGN:
        case JD_OP_AND_ASSIGN:
        case JD_OP_OR_ASSIGN:
        case JD_OP_XOR_ASSIGN:
            return 1;
        case JD_OP_LOGICAL_OR:
            return 2;
        case JD_OP_LOGICAL_AND:
            return 3;
        case JD_OP_OR:
            return 4;
        case JD_OP_XOR:
            return 5;
        case JD_OP_AND:
            return 6;
        case JD_OP_EQ:
        case JD_OP_NE:
            return 7;
        case JD_OP_LT:
        case JD_OP_LE:
        case JD_OP_GT:
        case JD_OP_GE:
        case JD_OP_INSTANCEOF:
            return 8;
        case JD_OP_SHL:
        case JD_OP_SHR:
        case JD_OP_USHR:
            return 9;
        case JD_OP_ADD:
        case JD_OP_SUB:
            return 10;
        case JD_OP_MUL:
        case JD_OP_DIV:
        case JD_OP_REM:
            return 11;
        default:
            // unary and anything unknown binds tightest
            return 12;
    }
}

static bool operator_is_associative(jd_operator op)
{
    switch (op) {
        case JD_OP_ADD:
        case JD_OP_MUL:
        case JD_OP_AND:
        case JD_OP_OR:
        case JD_OP_XOR:
        case JD_OP_LOGICAL_AND:
        case JD_OP_LOGICAL_OR:
            return true;
        default:
            return false;
    }
}

static bool operator_is_right_associative(jd_operator op)
{
    return operator_precedence(op) == 1;
}

static bool needs_parentheses(jd_exp *child, jd_operator parent, bool is_right)
{
    if (child == NULL || child->type != JD_EXPRESSION_OPERATOR)
        return false;

    jd_exp_operator *child_op = child->data;
    int child_precedence = operator_precedence(child_op->operator);
    int parent_precedence = operator_precedence(parent);

    if (child_precedence != parent_precedence)
        return child_precedence < parent_precedence;

    if (is_right) {
        if (operator_is_right_associative(parent))
            return false;
        return !(child_op->operator == parent &&
                 operator_is_associative(parent));
    }

    return operator_is_right_associative(parent);
}

string exp_operator_to_s(jd_exp *expression)
{
    jd_exp_operator *op = expression->data;
    jd_exp *left_exp = &op->list->args[0];
    jd_exp *right_exp = &op->list->args[1];
    string left = exp_to_s(left_exp);
    string right = exp_to_s(right_exp);
    string op_name = get_operator_name(op->operator);

    if (needs_parentheses(left_exp, op->operator, false))
        left = str_create("(%s)", left);
    if (needs_parentheses(right_exp, op->operator, true))
        right = str_create("(%s)", right);

    return str_create("%s %s %s", left, op_name, right);
}

void exp_operator_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_operator *operator   = expression->data;
    jd_exp *exp_left = &operator->list->args[0];
    jd_exp *exp_right = &operator->list->args[1];
    string op = get_operator_name(operator->operator);
    bool left_paren = needs_parentheses(exp_left, operator->operator, false);
    bool right_paren = needs_parentheses(exp_right, operator->operator, true);

    if (left_paren)
        fprintf(stream, "(");
    expression_to_stream(stream, node, exp_left);
    if (left_paren)
        fprintf(stream, ")");
    fprintf(stream, " %s ", op);
    if (right_paren)
        fprintf(stream, "(");
    expression_to_stream(stream, node, exp_right);
    if (right_paren)
        fprintf(stream, ")");
}

#include "decompiler/transformer/transformer.h"
#include "parser/class/class_tools.h"


string exp_break_to_s(jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    if (DEBUG_INS_AND_NODE_INFO) {
        string str = str_create("break; // %u", exp_goto->goto_offset);
        return str;
    }
    else {
        return str_create("break;");
    }
}

void exp_break_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    if (DEBUG_INS_AND_NODE_INFO)
        fprintf(stream, "break; // %u", exp_goto->goto_offset);
    else
        fprintf(stream, "break;");
}

string exp_continue_to_s(jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    if (DEBUG_INS_AND_NODE_INFO) {
        string str = str_create("continue; // %u", exp_goto->goto_offset);
        return str;
    }
    else {
        return str_create("continue;");
    }
}

void exp_continue_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    if (DEBUG_INS_AND_NODE_INFO)
        fprintf(stream, "continue; // %u", exp_goto->goto_offset);
    else
        fprintf(stream, "continue;");
}

static string exp_loop_to_s(jd_exp *expression, string loop_name)
{
    jd_exp_loop *exp_loop = expression->data;
    string s = exp_to_s(&exp_loop->list->args[0]);

    switch(expression->type)
    {
        case JD_EXPRESSION_DO_WHILE:
            return str_create("dowhile(%s)[%d -> %d]",
                              s,
                              exp_loop->start_offset,
                              exp_loop->end_offset);
        case JD_EXPRESSION_WHILE:
            return str_create("while(%s)[%d -> %d]",
                              s, exp_loop->start_offset, exp_loop->end_offset);
        case JD_EXPRESSION_FOR:
            return str_create("for(%s)[%d -> %d]",
                              s, exp_loop->start_offset, exp_loop->end_offset);
        default:
            return NULL;
    }

}

string exp_while_to_s(jd_exp *expression)
{
    return exp_loop_to_s(expression, "while");
}

void exp_while_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_loop *exp_loop = expression->data;
    expression_to_stream(stream, node, &exp_loop->list->args[0]);
}

string exp_do_while_to_s(jd_exp *expression)
{
    return exp_loop_to_s(expression, "do_while");
}

void exp_do_while_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_loop *exp_loop = expression->data;
    expression_to_stream(stream, node, &exp_loop->list->args[0]);
}

string exp_for_to_s(jd_exp *expression)
{
    jd_exp_for *for_exp = expression->data;
    string s1 = exp_to_s(&for_exp->list->args[0]);
    string s2 = exp_to_s(&for_exp->list->args[1]);
    string s3 = exp_to_s(&for_exp->list->args[2]);

    return str_create("for(%s; %s; %s)", s1, s2, s3);
}

void exp_for_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_for *for_exp = expression->data;

    expression_to_stream(stream, node, &for_exp->list->args[0]);
    fprintf(stream, "; ");
    expression_to_stream(stream, node, &for_exp->list->args[1]);
    fprintf(stream, "; ");
    expression_to_stream(stream, node, &for_exp->list->args[2]);
}

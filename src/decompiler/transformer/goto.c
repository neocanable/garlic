#include "decompiler/transformer/transformer.h"
#include "decompiler/instruction.h"

string exp_goto_to_s(jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    return str_create("goto %u (%d)", exp_goto->goto_offset);
}

void exp_goto_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    fprintf(stream, "goto %u", exp_goto->goto_offset);
}

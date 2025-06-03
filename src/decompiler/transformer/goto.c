#include "decompiler/transformer/transformer.h"
#include "decompiler/instruction.h"

string exp_goto_to_s(jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    jd_ins *ins = expression->ins;
    if (ins_is_copy_block(ins))
        return str_create("goto %u [copy block]", exp_goto->goto_offset);
    else
        return str_create("goto %u (%d)", exp_goto->goto_offset);
}

void exp_goto_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_goto *exp_goto = expression->data;
    jd_ins *ins = expression->ins;
    if (ins_is_copy_block(ins))
        fprintf(stream, "goto %u [copy block]", exp_goto->goto_offset);
    else
        fprintf(stream, "goto %u", exp_goto->goto_offset);
}

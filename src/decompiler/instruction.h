#ifndef GARLIC_INSTRUCTION_H
#define GARLIC_INSTRUCTION_H

#include "decompiler/structure.h"
#include "common/debug.h"


jd_bblock* dup_basic_block_and_ins(jd_method *m, jd_bblock *src_block);

static inline jd_ins* get_ins(jd_method *m, uint32_t id)
{
    return lget_obj(m->instructions, id);
}

static inline bool ins_is_jump_destination(jd_ins *ins)
{
    return !is_list_empty(ins->comings);
}

static inline jd_exp* get_exp(jd_method *m, uint32_t id)
{
    if (id >= m->expressions->size) {
        DEBUG_ERROR_PRINT("expression id %d out of range\n", id);
        return NULL;
    }
    return lget_obj(m->expressions, id);
}

static inline jd_ins* ins_of_offset(jd_method *m, uint32_t offset)
{
    int idx = hget_i2i(m->offset2id_map, offset);
    return get_ins(m, idx);
}

#define INSTRUCTION_STATE_TOOL(name, flag)          \
    static inline bool ins_is_##name(jd_ins *ins)   \
    {                                               \
        return (ins->state_flag & flag) != 0;       \
    }                                               \
    static inline void ins_mark_##name(jd_ins *ins) \
    {                                               \
        ins->state_flag |= flag;                    \
    }                                               \

INSTRUCTION_STATE_TOOL(nopped,              INS_STATE_NOPPED);
INSTRUCTION_STATE_TOOL(unreached,           INS_STATE_UNREACHED);
INSTRUCTION_STATE_TOOL(copy_block,          INS_STATE_COPY_BLOCK);
INSTRUCTION_STATE_TOOL(copy_if_true_block,  INS_STATE_COPY_IF_TRUE_BLOCK);
INSTRUCTION_STATE_TOOL(duplicate,           INS_STATE_DUPLICATE);

#endif //GARLIC_INSTRUCTION_H

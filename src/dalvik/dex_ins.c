#include "dalvik/dex_ins.h"
#include "decompiler/instruction.h"
#include "jvm/jvm_ins.h"
#include "decompiler/control_flow.h"

jd_ins* make_goto_ins(jd_method *m, jd_support_type type, uint32_t offset)
{
    jd_ins *last = lget_obj_last(m->instructions);
    jd_ins *ins = make_obj(jd_ins);
    ins->type = type;
    ins->offset = last->offset + 4;
    ins->idx = m->instructions->size;
    ins->fn = last->fn;
    if (ins->type == JD_TYPE_DALVIK)
        ins->code = DEX_INS_GOTO;
    else
        ins->code = INS_GOTO;
    ins->name = strdup("goto");

    ins->param = x_alloc(sizeof(u2) * 2);
    dex_setup_goto_offset(ins, offset);

    ladd_obj(m->instructions, ins);
    hset_i2i(m->offset2id_map, ins->offset, ins->idx);

    return ins;
}

jd_bblock* make_goto_basic_block(jd_method *m,
                                 jd_support_type type,
                                 uint32_t offset)
{
    jd_ins *goto_ins = make_goto_ins(m, type, offset);
    jd_bblock *block = make_obj(jd_bblock);
    block->method = m;
    block->block_id = m->basic_blocks->size;
    block->type = JD_BB_NORMAL;
    block->live = JD_STATUS_BUSY;

    block->in = linit_object_with_capacity(2);
    block->out = linit_object_with_capacity(2);
    block->dom_children = linit_object();
    block->frontier = linit_object_with_capacity(2);
    block->frontier->cmp_fn = (list_cmp_fn) basic_block_id_comparator;
    block->dominates = linit_object();

    block->ub = make_obj(jd_union_basic_block);
    block->ub->nblock = make_obj(jd_nblock);
    block->is_dup = true;

    jd_nblock *nb = block->ub->nblock;
    nb->start_idx = goto_ins->idx;
    nb->end_idx = goto_ins->idx;
    nb->start_offset = goto_ins->offset;
    nb->end_offset = goto_ins->offset;
    nb->start_ins = goto_ins;
    nb->end_ins = goto_ins;
    ladd_obj(m->basic_blocks, block);
    return block;
}

jd_bblock* make_dalvik_goto_block(jd_method *m, uint32_t offset)
{
    return make_goto_basic_block(m, JD_TYPE_DALVIK, offset);
}

jd_bblock* dup_basic_block_and_ins(jd_method *m, jd_bblock *src_block)
{
    jd_ins *last = lget_obj_last(m->instructions);
    jd_nblock *src_nb = src_block->ub->nblock;
    jd_ins *prev = NULL;
    jd_ins *start = NULL;
    jd_ins *end = NULL;
    for (int i = src_nb->start_idx; i <= src_nb->end_idx; ++i) {
        jd_ins *src = get_ins(m, i);
        if (dex_ins_is_goto_jump(src))
            continue;
        jd_ins *copy = make_obj(jd_ins);
        memcpy(copy, src, sizeof(jd_ins));

        copy->prev = NULL;
        copy->next = NULL;

        copy->comings = linit_object();
        copy->jumps = linit_object();
        copy->targets = linit_object();
        copy->code = src->code;
        copy->name = src->name;

        copy->offset = last->offset + copy->param_length;
        copy->idx = m->instructions->size;
        ladd_obj(m->instructions, copy);
        hset_i2i(m->offset2id_map, copy->offset, copy->idx);

        if (i == src_nb->start_idx)
            start = copy;

        end = copy;

        if (prev != NULL) {
            prev->next = copy;
            copy->prev = prev;
        }
        last = copy;
        prev = copy;
    }

    jd_bblock *new_b = make_obj(jd_bblock);
    new_b->method = m;
    new_b->block_id = m->basic_blocks->size;
    new_b->type = src_block->type;
    new_b->live = JD_STATUS_BUSY;
    new_b->is_dup = true;

    new_b->in = linit_object_with_capacity(2);
    new_b->out = linit_object_with_capacity(2);
    new_b->dom_children = linit_object();
    new_b->frontier = linit_object_with_capacity(2);
    new_b->frontier->cmp_fn = (list_cmp_fn) basic_block_id_comparator;
    new_b->dominates = linit_object();

    new_b->ub = make_obj(jd_union_basic_block);
    new_b->ub->nblock = make_obj(jd_nblock);
    jd_nblock *new_nb = new_b->ub->nblock;
    new_nb->start_idx = start->idx;
    new_nb->end_idx = end->idx;
    new_nb->start_offset = start->offset;
    new_nb->end_offset = end->offset;
    new_nb->start_ins = start;
    new_nb->end_ins = end;
    ladd_obj(m->basic_blocks, new_b);
    printf("dup basic block and instruction: %zu is_dup: %d\n",
           src_block->block_id, src_block->is_dup);

    return new_b;
}


u8 dex_ins_parameter(jd_dex_ins *ins, int number);

static inline u8 dex_ins_parameter_10x(jd_dex_ins *ins, int number)
{
    return 0;
}

static inline void dex_ins_use_def_10x(jd_dex_ins *ins)
{
}

static inline u8 dex_ins_parameter_12x(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8) & 0x0F;
    u1 v_b = item >> 12;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_12x(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_b = dex_ins_parameter(ins, 1);

    bitset_set(ins->uses, v_b);
    bitset_set(ins->defs, v_a);
    if (ins->code >= DEX_INS_ADD_INT_2ADDR &&
        ins->code <= DEX_INS_REM_DOUBLE_2ADDR) {
        bitset_set(ins->uses, v_a);
    }
}

static inline u8 dex_ins_parameter_11n(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8) & 0x0F;
    s1 v_b = (s1)(item >> 12);
    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_11n(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);

    bitset_set(ins->defs, v_a);
}

static inline u8 dex_ins_parameter_11x(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    if (number == 0)
        return v_a;
    else
        abort();
}

static inline void dex_ins_use_def_11x(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);

    switch (ins->code) {
        case DEX_INS_MOVE_RESULT:
        case DEX_INS_MOVE_RESULT_WIDE:
        case DEX_INS_MOVE_RESULT_OBJECT:
        case DEX_INS_MOVE_EXCEPTION:
            bitset_set(ins->defs, v_a);
            break;
        default:
            bitset_set(ins->uses, v_a);
            break;
    }
}

static inline u8 dex_ins_parameter_10t(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    s1 v_a = item >> 8;
    if (number == 0)
        return v_a;
    else
        abort();
}

static inline void dex_ins_use_def_10t(jd_dex_ins *ins)
{
    s1 v_a = (s1)dex_ins_parameter(ins, 0);
}

static inline u8 dex_ins_parameter_20t(jd_dex_ins *ins, int number)
{
    s2 v_a = (s2)(ins->param[1]);
    if (number == 0)
        return v_a;
    else
        abort();
}

static inline void dex_ins_use_def_20t(jd_dex_ins *ins)
{
    s2 v_a = (s2)dex_ins_parameter(ins, 0);
}

static inline u8 dex_ins_parameter_22x(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 v_b = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_22x(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u2 v_b = dex_ins_parameter(ins, 1);
    bitset_set(ins->defs, v_a);
    bitset_set(ins->uses, v_b);
}

static inline u8 dex_ins_parameter_21t(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    s2 v_b = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_21t(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    s2 v_b = (s2)dex_ins_parameter(ins, 1);
    bitset_set(ins->uses, v_a);
}

static inline u8 dex_ins_parameter_21s(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    s2 v_b = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_21s(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    s2 v_b = (s2)dex_ins_parameter(ins, 1);
    bitset_set(ins->defs, v_a);
}

static inline u8 dex_ins_parameter_21h(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 v_b = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_21h(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    s2 v_b = (s2)dex_ins_parameter(ins, 1);

    bitset_set(ins->defs, v_a);
}

static inline u8 dex_ins_parameter_21c(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 v_b = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_21c(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);

    switch (ins->code) {
        case DEX_INS_CHECK_CAST:
        case DEX_INS_SPUT:
        case DEX_INS_SPUT_WIDE:
        case DEX_INS_SPUT_OBJECT:
        case DEX_INS_SPUT_BOOLEAN:
        case DEX_INS_SPUT_BYTE:
        case DEX_INS_SPUT_CHAR:
        case DEX_INS_SPUT_SHORT:
            bitset_set(ins->uses, v_a);
            break;
        default:
            bitset_set(ins->defs, v_a);
            break;
    }
}

static inline u8 dex_ins_parameter_23x(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 second = ins->param[1];
    u1 v_b = second >> 8;
    u1 v_c = second & 0xFF;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_23x(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_b = dex_ins_parameter(ins, 1);
    u1 v_c = dex_ins_parameter(ins, 2);

    switch (ins->code) {
        case DEX_INS_APUT:
        case DEX_INS_APUT_WIDE:
        case DEX_INS_APUT_OBJECT:
        case DEX_INS_APUT_BOOLEAN:
        case DEX_INS_APUT_BYTE:
        case DEX_INS_APUT_CHAR:
        case DEX_INS_APUT_SHORT: {
            bitset_set(ins->uses, v_a);
            bitset_set(ins->uses, v_b);
            bitset_set(ins->uses, v_c);
            break;
        }
        default: {
            bitset_set(ins->defs, v_a);
            bitset_set(ins->uses, v_b);
            bitset_set(ins->uses, v_c);
            break;
        }
    }

}

static inline u8 dex_ins_parameter_22b(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 second = ins->param[1];
    u1 v_c = second >> 8;
    u1 v_b = second & 0xFF;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_22b(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_b = dex_ins_parameter(ins, 1);
    u1 v_c = dex_ins_parameter(ins, 2);

    bitset_set(ins->defs, v_a);
    bitset_set(ins->uses, v_b);
}

static inline u8 dex_ins_parameter_22t(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = item >> 12;
    u1 v_b = (item >> 8) & 0x0F;
    s2 v_c = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_22t(jd_dex_ins *ins)
{
    // 22t
    // B|A|op CCCC
    // vA, vB, +CCCC
    //    u2 item = ins->param[0];
    //    u1 v_a = item >> 12;
    //    u1 v_b = (item >> 8) & 0x0F;
    //    s2 v_c = ins->param[1];

    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_b = dex_ins_parameter(ins, 1);
    s2 v_c = (s2)dex_ins_parameter(ins, 2);

    bitset_set(ins->uses, v_a);
    bitset_set(ins->uses, v_b);
}

static inline u8 dex_ins_parameter_22s(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8) & 0x0F;
    u1 v_b = item >> 12;
    s2 v_c = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_22s(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_b = dex_ins_parameter(ins, 1);
    s2 v_c = (s2)dex_ins_parameter(ins, 2);

    bitset_set(ins->defs, v_a);
    bitset_set(ins->uses, v_b);
}

static inline u8 dex_ins_parameter_22c(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8) & 0x0F;
    u1 v_b = item >> 12;
    u2 v_c = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_22c(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_b = dex_ins_parameter(ins, 1);
    u2 type_index = dex_ins_parameter(ins, 2);

    switch (ins->code) {
        case DEX_INS_IPUT:
        case DEX_INS_IPUT_WIDE:
        case DEX_INS_IPUT_OBJECT:
        case DEX_INS_IPUT_BOOLEAN:
        case DEX_INS_IPUT_BYTE:
        case DEX_INS_IPUT_CHAR:
        case DEX_INS_IPUT_SHORT:
            bitset_set(ins->uses, v_a);
            bitset_set(ins->uses, v_b);
            break;
        default:
            bitset_set(ins->defs, v_a);
            bitset_set(ins->uses, v_b);
            break;
    }
}

static inline u8 dex_ins_parameter_30t(jd_dex_ins *ins, int number)
{
    u2 i1 = ins->param[1];
    u2 i2 = ins->param[2];
    s4 v_a = i2 << 16 | i1;
    if (number == 0)
        return v_a;
    else
        abort();
}

static inline void dex_ins_use_def_30t(jd_dex_ins *ins)
{
    s4 v_a = (s4)dex_ins_parameter(ins, 0);
}

static inline u8 dex_ins_parameter_32x(jd_dex_ins *ins, int number)
{
    u2 v_a = ins->param[1];
    u2 v_b = ins->param[2];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_32x(jd_dex_ins *ins)
{
    u2 v_a = dex_ins_parameter(ins, 0);
    u2 v_b = dex_ins_parameter(ins, 1);

    bitset_set(ins->defs, v_a);
    bitset_set(ins->uses, v_b);
}

static inline u8 dex_ins_parameter_31i(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 low = ins->param[1];
    u2 high = ins->param[2];
    s4 v_b = high << 16 | low;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_31i(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    s4 v_b = (s4)dex_ins_parameter(ins, 1);

    bitset_set(ins->defs, v_a);
}

static inline u8 dex_ins_parameter_31t(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 low = ins->param[1];
    u2 high = ins->param[2];
    s4 v_b = high << 16 | low;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_31t(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    s4 v_b = (s4)dex_ins_parameter(ins, 1);

    bitset_set(ins->uses, v_a);
}

static inline u8 dex_ins_parameter_31c(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 low = ins->param[1];
    u2 high = ins->param[2];
    s4 v_b = high << 16 | low;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_31c(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    s4 v_b = (s4)dex_ins_parameter(ins, 1);
    bitset_set(ins->defs, v_a);
}

static inline u8 dex_ins_parameter_35c(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = item >> 12;
    u1 v_g = (item >> 8) & 0x0F;
    u2 second = ins->param[2];
    u1 v_c = second & 0x0F;
    u1 v_d = (second >> 4) & 0x0F;
    u1 v_e = (second >> 8) & 0x0F;
    u1 v_f = second >> 12;
    u2 v_b = ins->param[1];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        case 3:
            return v_d;
        case 4:
            return v_e;
        case 5:
            return v_f;
        case 6:
            return v_g;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_35c(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u2 v_b = dex_ins_parameter(ins, 1);
    u1 v_c = dex_ins_parameter(ins, 2);
    u1 v_d = dex_ins_parameter(ins, 3);
    u1 v_e = dex_ins_parameter(ins, 4);
    u1 v_f = dex_ins_parameter(ins, 5);
    u1 v_g = dex_ins_parameter(ins, 6);

    switch (v_a) {
        case 0:
            break;
        case 1:
            bitset_set(ins->uses, v_c);
            break;
        case 2:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            break;
        case 3:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            bitset_set(ins->uses, v_e);
            break;
        case 4:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            bitset_set(ins->uses, v_e);
            bitset_set(ins->uses, v_f);
            break;
        case 5:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            bitset_set(ins->uses, v_e);
            bitset_set(ins->uses, v_f);
            bitset_set(ins->uses, v_g);
            break;
        default:
            break;
    }
}

static inline u8 dex_ins_parameter_3rc(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 v_b = ins->param[1];
    u2 v_c = ins->param[2];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        case 2:
            return v_c;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_3rc(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    u2 start_index = dex_ins_parameter(ins, 2);
    u2 count = start_index + v_a - 1;
    for (int i = start_index; i <= count; ++i) {
        bitset_set(ins->uses, i);
    }
}

static inline u8 dex_ins_parameter_45cc(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = item >> 12;
    u1 v_g = (item >> 8) & 0x0F;
    u2 v_bbbb = ins->param[1];
    u2 second = ins->param[2];
    u2 v_hhhh = ins->param[3];
    u1 v_c = second >> 12;
    u1 v_d = (second >> 8) & 0x0F;
    u1 v_e = (second >> 4) & 0x0F;
    u1 v_f = second & 0x0F;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_bbbb;
        case 2:
            return v_c;
        case 3:
            return v_d;
        case 4:
            return v_e;
        case 5:
            return v_f;
        case 6:
            return v_g;
        case 7:
            return v_hhhh;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_45cc(jd_dex_ins *ins)
{
    u2 item = ins->param[0];
    u1 v_a = dex_ins_parameter(ins, 0);
    u1 v_g = dex_ins_parameter(ins, 6);
    u2 v_bbbb = dex_ins_parameter(ins, 1);
    u2 v_hhhh = dex_ins_parameter(ins, 7);
    u1 v_c = dex_ins_parameter(ins, 2);
    u1 v_d = dex_ins_parameter(ins, 3);
    u1 v_e = dex_ins_parameter(ins, 4);
    u1 v_f = dex_ins_parameter(ins, 5);


    switch (v_a) {
        case 0:
            break;
        case 1:
            bitset_set(ins->uses, v_c);
            break;
        case 2:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            break;
        case 3:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            bitset_set(ins->uses, v_e);
            break;
        case 4:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            bitset_set(ins->uses, v_e);
            bitset_set(ins->uses, v_f);
            break;
        case 5:
            bitset_set(ins->uses, v_c);
            bitset_set(ins->uses, v_d);
            bitset_set(ins->uses, v_e);
            bitset_set(ins->uses, v_f);
            bitset_set(ins->uses, v_g);
            break;
        default:
            break;
    }
}

static inline u8 dex_ins_parameter_4rcc(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    u2 v_bbbb = ins->param[1];
    u2 v_cccc = ins->param[2];
    u2 v_hhhh = ins->param[3];

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_bbbb;
        case 2:
            return v_cccc;
        case 3:
            return v_hhhh;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_4rcc(jd_dex_ins *ins)
{
    u1 v_a = dex_ins_parameter(ins, 0);
    u2 v_bbbb = dex_ins_parameter(ins, 1);
    u2 v_cccc = dex_ins_parameter(ins, 2);
    u2 v_hhhh = dex_ins_parameter(ins, 3);
    u2 count = v_cccc + v_a - 1;

    for (int j = v_cccc; j <= count; ++j) {
        bitset_set(ins->uses, j);
    }
}

static inline u8 dex_ins_parameter_51l(jd_dex_ins *ins, int number)
{
    u2 item = ins->param[0];
    u1 v_a = (item >> 8);
    s4 b1 = ins->param[1];
    s4 b2 = ins->param[2];
    s4 b3 = ins->param[3];
    s4 b4 = ins->param[4];
    s8 v_b = (s8)b1 << 48 | (s8)b2 << 32 | (s8)b3 << 16 | b4;

    switch (number) {
        case 0:
            return v_a;
        case 1:
            return v_b;
        default:
            abort();
    }
}

static inline void dex_ins_use_def_51l(jd_dex_ins *ins)
{
    u2 item = ins->param[0];
    u1 v_a = dex_ins_parameter(ins, 0);
    s8 v_b = (s8)dex_ins_parameter(ins, 1);

    bitset_set(ins->defs, v_a);
}

void dex_ins_use_def_init(jd_dex_ins *ins) {
    switch (ins->format) {
        case kFmt10x:
            dex_ins_use_def_10x(ins);
            break;
        case kFmt12x:
            dex_ins_use_def_12x(ins);
            break;
        case kFmt11n:
            dex_ins_use_def_11n(ins);
            break;
        case kFmt11x:
            dex_ins_use_def_11x(ins);
            break;
        case kFmt10t:
            dex_ins_use_def_10t(ins);
            break;
        case kFmt20t:
            dex_ins_use_def_20t(ins);
            break;
        case kFmt22x:
            dex_ins_use_def_22x(ins);
            break;
        case kFmt21t:
            dex_ins_use_def_21t(ins);
            break;
        case kFmt21s:
            dex_ins_use_def_21s(ins);
            break;
        case kFmt21h:
            dex_ins_use_def_21h(ins);
            break;
        case kFmt21c:
            dex_ins_use_def_21c(ins);
            break;
        case kFmt23x:
            dex_ins_use_def_23x(ins);
            break;
        case kFmt22b:
            dex_ins_use_def_22b(ins);
            break;
        case kFmt22t:
            dex_ins_use_def_22t(ins);
            break;
        case kFmt22s:
            dex_ins_use_def_22s(ins);
            break;
        case kFmt22c:
            dex_ins_use_def_22c(ins);
            break;
        case kFmt30t:
            dex_ins_use_def_30t(ins);
            break;
        case kFmt32x:
            dex_ins_use_def_32x(ins);
            break;
        case kFmt31i:
            dex_ins_use_def_31i(ins);
            break;
        case kFmt31t:
            dex_ins_use_def_31t(ins);
            break;
        case kFmt31c:
            dex_ins_use_def_31c(ins);
            break;
        case kFmt35c:
            dex_ins_use_def_35c(ins);
            break;
        case kFmt3rc:
            dex_ins_use_def_3rc(ins);
            break;
        case kFmt51l:
            dex_ins_use_def_51l(ins);
            break;
        case kFmt45cc:
            dex_ins_use_def_45cc(ins);
            break;
        case kFmt4rcc:
            dex_ins_use_def_4rcc(ins);
            break;
        default:
            break;
    }
}

u8 dex_ins_parameter(jd_dex_ins *ins, int number)
{
    switch (ins->format) {
        case kFmt10x:
            return dex_ins_parameter_10x(ins, number);
        case kFmt12x:
            return dex_ins_parameter_12x(ins, number);
        case kFmt11n:
            return dex_ins_parameter_11n(ins, number);
        case kFmt11x:
            return dex_ins_parameter_11x(ins, number);
        case kFmt10t:
            return dex_ins_parameter_10t(ins, number);
        case kFmt20t:
            return dex_ins_parameter_20t(ins, number);
        case kFmt22x:
            return dex_ins_parameter_22x(ins, number);
        case kFmt21t:
            return dex_ins_parameter_21t(ins, number);
        case kFmt21s:
            return dex_ins_parameter_21s(ins, number);
        case kFmt21h:
            return dex_ins_parameter_21h(ins, number);
        case kFmt21c:
            return dex_ins_parameter_21c(ins, number);
        case kFmt23x:
            return dex_ins_parameter_23x(ins, number);
        case kFmt22b:
            return dex_ins_parameter_22b(ins, number);
        case kFmt22t:
            return dex_ins_parameter_22t(ins, number);
        case kFmt22s:
            return dex_ins_parameter_22s(ins, number);
        case kFmt22c:
            return dex_ins_parameter_22c(ins, number);
        case kFmt30t:
            return dex_ins_parameter_30t(ins, number);
        case kFmt32x:
            return dex_ins_parameter_32x(ins, number);
        case kFmt31i:
            return dex_ins_parameter_31i(ins, number);
        case kFmt31t:
            return dex_ins_parameter_31t(ins, number);
        case kFmt31c:
            return dex_ins_parameter_31c(ins, number);
        case kFmt35c:
            return dex_ins_parameter_35c(ins, number);
        case kFmt3rc:
            return dex_ins_parameter_3rc(ins, number);
        case kFmt51l:
            return dex_ins_parameter_51l(ins, number);
        case kFmt45cc:
            return dex_ins_parameter_45cc(ins, number);
        case kFmt4rcc:
            return dex_ins_parameter_4rcc(ins, number);
        default:
            abort();
    }
}

int dex_switch_key(jd_dex_ins *ins, uint32_t target_offset)
{
    jd_method *m = ins->method;
    s4 offset = (s4)(ins->param[2] << 16 | ins->param[1]);
    hashmap *map = m->offset2id_map;
    int payload_idx = hget_i2i(map, ins->offset + offset);
    jd_dex_ins *pins = lget_obj(m->instructions, payload_idx);

    if (dex_ins_is_packed_switch(ins)) {
        int size = pins->param[1];
        int first_key = pins->param[3] << 16 | pins->param[2];

        for (int i = 0; i < size; ++i) {
            u4 toff = pins->param[5+i*2] << 16 | pins->param[4+i*2];
            if (ins->offset + toff == target_offset)
                return first_key + i;
        }
    }
    else {
        int size = pins->param[1];

        u2 *params = pins->param;
        for (int i = 0; i < size; ++i) {
            int key = params[3+i*2] << 16 | params[2+i*2];
            u4 val = params[3+size*2+i*2] | params[2+size*2+i*2];
            if (ins->offset + val == target_offset)
                return key;
        }
    }
    return 0;
}

bool dex_ins_type_changed(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_NEG_DOUBLE:
        case DEX_INS_NEG_FLOAT:
        case DEX_INS_NEG_INT:
        case DEX_INS_NEG_LONG:
        case DEX_INS_NOT_INT:
        case DEX_INS_NOT_LONG: {
            u1 slot_a = dex_ins_parameter(ins, 0);
            u1 slot_b = dex_ins_parameter(ins, 1);
            return slot_a != slot_b;
        }
        case DEX_INS_ADD_INT_2ADDR:
        case DEX_INS_SUB_INT_2ADDR:
        case DEX_INS_MUL_INT_2ADDR:
        case DEX_INS_DIV_INT_2ADDR:
        case DEX_INS_REM_INT_2ADDR:
        case DEX_INS_AND_INT_2ADDR:
        case DEX_INS_OR_INT_2ADDR:
        case DEX_INS_XOR_INT_2ADDR:
        case DEX_INS_SHL_INT_2ADDR:
        case DEX_INS_SHR_INT_2ADDR:
        case DEX_INS_USHR_INT_2ADDR:
        case DEX_INS_ADD_LONG_2ADDR:
        case DEX_INS_SUB_LONG_2ADDR:
        case DEX_INS_MUL_LONG_2ADDR:
        case DEX_INS_DIV_LONG_2ADDR:
        case DEX_INS_REM_LONG_2ADDR:
        case DEX_INS_AND_LONG_2ADDR:
        case DEX_INS_OR_LONG_2ADDR:
        case DEX_INS_XOR_LONG_2ADDR:
        case DEX_INS_SHL_LONG_2ADDR:
        case DEX_INS_SHR_LONG_2ADDR:
        case DEX_INS_USHR_LONG_2ADDR:
        case DEX_INS_ADD_FLOAT_2ADDR:
        case DEX_INS_SUB_FLOAT_2ADDR:
        case DEX_INS_MUL_FLOAT_2ADDR:
        case DEX_INS_DIV_FLOAT_2ADDR:
        case DEX_INS_REM_FLOAT_2ADDR:
        case DEX_INS_ADD_DOUBLE_2ADDR:
        case DEX_INS_SUB_DOUBLE_2ADDR:
        case DEX_INS_MUL_DOUBLE_2ADDR:
        case DEX_INS_DIV_DOUBLE_2ADDR:
        case DEX_INS_REM_DOUBLE_2ADDR: {
            u1 slot_a = dex_ins_parameter(ins, 0);
            u1 slot_b = dex_ins_parameter(ins, 1);
            return slot_a != slot_b;
        }
        case DEX_INS_ADD_INT_LIT16:
        case DEX_INS_RSUB_INT:
        case DEX_INS_MUL_INT_LIT16:
        case DEX_INS_DIV_INT_LIT16:
        case DEX_INS_REM_INT_LIT16:
        case DEX_INS_AND_INT_LIT16:
        case DEX_INS_OR_INT_LIT16:
        case DEX_INS_XOR_INT_LIT16:
        case DEX_INS_ADD_INT_LIT8:
        case DEX_INS_RSUB_INT_LIT8:
        case DEX_INS_MUL_INT_LIT8:
        case DEX_INS_DIV_INT_LIT8:
        case DEX_INS_REM_INT_LIT8:
        case DEX_INS_AND_INT_LIT8:
        case DEX_INS_OR_INT_LIT8:
        case DEX_INS_XOR_INT_LIT8:
        case DEX_INS_SHL_INT_LIT8:
        case DEX_INS_SHR_INT_LIT8:
        case DEX_INS_USHR_INT_LIT8: {
            u1 slot_a = dex_ins_parameter(ins, 0);
            u1 slot_b = dex_ins_parameter(ins, 1);
            return slot_a != slot_b;
        }
        default: return true;
    }
}

jd_dex_ins* dex_ins_clone(jd_dex_ins *ins)
{
    jd_dex_ins *clone = make_obj(jd_dex_ins);
    memcpy(clone, ins, sizeof(jd_dex_ins));
    return clone;
}

jd_dex_ins* make_goto_dex_ins(uint32_t offset, uint32_t target_offset)
{
    jd_dex_ins *ins = make_obj(jd_dex_ins);
    ins->code = DEX_INS_GOTO_32;
    ins->format = kFmt30t;
    ins->offset = offset;
    ins->param[0] = 0;
    return ins;
}

jd_dex_ins* dup_dex_ins(jd_dex_ins *src)
{
    jd_method *m = src->method;
    jd_dex_ins *last = lget_obj_last(m->instructions);
    jd_dex_ins *ins = make_obj(jd_dex_ins);
    memcpy(ins, src, sizeof(jd_dex_ins));
    ins->offset = last->offset + ins->param_length;
    ins->idx = m->instructions->size;
    ins->expression = NULL;
    ins->block = src->block;
    ins->old_offset = src->offset;
    ins_mark_duplicate(ins);

    ladd_obj(m->instructions, ins);
    hset_i2i(m->offset2id_map, ins->offset, ins->idx);
    return ins;
}
#ifndef GARLIC_DEX_INS_H
#define GARLIC_DEX_INS_H

#include "dalvik/dex_structure.h"
#include "dalvik/dex_ins_helper.h"
#include "decompiler/instruction.h"


jd_ins* make_goto_ins(jd_method *m, jd_support_type type, uint32_t offset);

void dex_ins_use_def_init(jd_dex_ins *ins);

jd_dex_ins* dup_dex_ins(jd_dex_ins *src);

u8 dex_ins_parameter(jd_dex_ins *ins, int number);

int dex_switch_key(jd_dex_ins *ins, uint32_t target_offset);

bool dex_ins_type_changed(jd_dex_ins *ins);

static inline bool dex_ins_is_unconditional_jump(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_GOTO:
        case DEX_INS_GOTO_16:
        case DEX_INS_GOTO_32:
            return true;
        default: return false;
    }
}

static inline bool dex_ins_is_conditional_jump(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_NE:
        case DEX_INS_IF_LT:
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GT:
        case DEX_INS_IF_LE:
        case DEX_INS_IF_EQZ:
        case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ:
        case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ:
        case DEX_INS_IF_LEZ:
            return true;
        default: return false;

    }
}

static inline bool dex_ins_is_if(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_NE:
        case DEX_INS_IF_LT:
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GT:
        case DEX_INS_IF_LE:
        case DEX_INS_IF_EQZ:
        case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ:
        case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ:
        case DEX_INS_IF_LEZ:
            return true;
        default: return false;
    }
}

static inline bool dex_ins_is_goto_jump(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_GOTO:
        case DEX_INS_GOTO_16:
        case DEX_INS_GOTO_32:
            return true;
        default: return false;
    }
}

static inline bool dex_ins_is_switch(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_SPARSE_SWITCH:
        case DEX_INS_PACKED_SWITCH:
            return true;
        default: return false;
    }
}

static inline bool dex_ins_is_return_op(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_RETURN:
        case DEX_INS_RETURN_OBJECT:
        case DEX_INS_RETURN_VOID:
        case DEX_INS_RETURN_WIDE:
            return true;
        default: return false;
    }
}

static inline bool dex_ins_is_compare(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_CMPL_FLOAT:
        case DEX_INS_CMP_LONG:
        case DEX_INS_CMPG_DOUBLE:
        case DEX_INS_CMPL_DOUBLE:
        case DEX_INS_CMPG_FLOAT:
            return true;
        default: return false;
    }
}

static inline u4 dex_ins_if_jump_offset(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_NE:
        case DEX_INS_IF_LT:
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GT:
        case DEX_INS_IF_LE: {
            s2 u_c = (s2)ins->param[1];
            return u_c + ins->offset;
        }
        case DEX_INS_IF_EQZ:
        case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ:
        case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ:
        case DEX_INS_IF_LEZ: {
            s2 u_b = (s2)ins->param[1];
            return ins->offset + u_b;
        }
        default: {
            fprintf(stderr, "get if goto_offset error\n");
            return 0;
        }
    }
}

static inline u4 dex_original_goto_offset(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_GOTO: {
            s1 u_a = (s1)dex_ins_parameter(ins, 0);
            return ins->offset + u_a;
        }
        case DEX_INS_GOTO_16: {
            s2 u_a = (s2)dex_ins_parameter(ins, 0);
            return ins->offset + u_a;
        }
        case DEX_INS_GOTO_32: {
            s4 u_a = (s4)dex_ins_parameter(ins, 0);
            return ins->offset + u_a;
        }
        default: {
            fprintf(stderr, "error at get_goto_offset\n");
            return 0;
        }
    }
}

static inline void dex_setup_goto_offset(jd_dex_ins *ins, uint32_t offset)
{
    ins->param[0] = offset >> 16;
    ins->param[1] = offset;
}

static inline void dex_setup_if_offset(jd_dex_ins *ins, uint32_t offset)
{
    s2 u_a = offset - ins->offset;
    ins->param[1] = u_a;
}

static inline u4 dex_goto_offset(jd_dex_ins *ins)
{
    return ins->param[0] << 16 | ins->param[1];
}

static inline bool dex_ins_is_goto_back(jd_dex_ins *ins)
{
    if (dex_ins_is_goto_jump(ins))
        return dex_goto_offset(ins) < ins->offset;
    else if (dex_ins_is_if(ins))
        return dex_ins_if_jump_offset(ins) < ins->offset;
    else
        return false;
}

static inline bool dex_ins_is_copy_basic_block(jd_dex_ins *ins)
{
    return ins->code == DEX_INS_COPY_BASIC_BLOCK ||
           ins->code == DEX_INS_COPY_BASIC_BLOCK_GOTO;
}

static inline bool dex_ins_is_handler_start(jd_dex_ins *ins)
{
    jd_method *m = ins->method;
    for (int i = 0; i < m->cfg_exceptions->size; ++i) {
        jd_exc *e = lget_obj(m->cfg_exceptions, i);
        if (e->handler_start_idx == ins->idx)
            return true;
    }
    return false;
}

static inline bool dex_ins_is_block_end(jd_dex_ins *ins)
{
    return dex_ins_is_conditional_jump(ins) ||
           dex_ins_is_switch(ins) ||
           dex_ins_is_goto_jump(ins) ||
           dex_ins_is_return_op(ins) ||
           dex_ins_is_copy_basic_block(ins);
}

static inline bool dex_ins_is_block_start(jd_dex_ins *ins)
{
    return dex_ins_is_move_exception(ins) ||
           dex_ins_is_handler_start(ins) ||
//           ins_is_handler_start(ins) ||
           ins_is_jump_destination(ins);
}

static inline bool dex_ins_is_branch(jd_dex_ins *ins)
{
    return dex_ins_is_if(ins) ||
        dex_ins_is_goto_jump(ins) ||
        dex_ins_is_switch(ins);

}

static inline bool dex_ins_is_store(jd_dex_ins *ins)
{
    return bitset_count(ins->defs) > 0;
}

static inline int dex_ins_store_slot(jd_dex_ins *ins)
{
    size_t k = 0;
    bitset_next_set_bit(ins->defs, &k);
    return k;
}

static inline bool dex_ins_is_load(jd_dex_ins *ins)
{
    return bitset_count(ins->uses) > 0;
}

static inline bool dex_ins_is_invokedymamic(jd_dex_ins *ins)
{
    return ins->code == DEX_INS_INVOKE_POLYMORPHIC ||
        ins->code == DEX_INS_INVOKE_CUSTOM ||
        ins->code == DEX_INS_INVOKE_POLYMORPHIC_RANGE ||
        ins->code == DEX_INS_INVOKE_CUSTOM_RANGE;
}

static inline bool dex_ins_is_invokevirtual(jd_dex_ins *ins)
{
    return dex_ins_is_invoke_virtual(ins) ||
        dex_ins_is_invoke_virtual_range(ins) ||
        dex_ins_is_invoke_super(ins) ||
        dex_ins_is_invoke_super_range(ins);
}

static inline bool dex_ins_is_invokeinterface(jd_dex_ins *ins)
{
    return dex_ins_is_invoke_interface(ins) ||
        dex_ins_is_invoke_interface_range(ins);
}

static inline bool dex_ins_is_invokestatic(jd_dex_ins *ins)
{
    return dex_ins_is_invoke_static(ins) ||
        dex_ins_is_invoke_static_range(ins);
}

static inline bool dex_ins_is_invokespecial(jd_dex_ins *ins)
{
    return dex_ins_is_invoke_direct(ins) ||
            dex_ins_is_invoke_direct_range(ins) ||
            dex_ins_is_invoke_super(ins) ||
            dex_ins_is_invoke_super_range(ins);
}

static inline bool dex_ins_is_move_to(jd_dex_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_MOVE:
        case DEX_INS_MOVE_FROM16:
        case DEX_INS_MOVE_16:
        case DEX_INS_MOVE_WIDE:
        case DEX_INS_MOVE_WIDE_FROM16:
        case DEX_INS_MOVE_WIDE_16:
        case DEX_INS_MOVE_OBJECT:
        case DEX_INS_MOVE_OBJECT_FROM16:
        case DEX_INS_MOVE_OBJECT_16:
            return true;
        default: return false;
    }
}

static inline bool dex_is_is_move_after_invoke(jd_dex_ins *ins)
{
    return dex_ins_is_move_result(ins) ||
        dex_ins_is_move_result_object(ins) ||
        dex_ins_is_move_result_wide(ins);
}

static inline bool dex_ins_is_jump_destination(jd_dex_ins *ins)
{
    return !is_list_empty(ins->comings);
}

static inline jd_dex_ins* dex_ins_of_offset(jd_method *m, u4 offset)
{
    int idx = hget_i2i(m->offset2id_map, offset);
    return lget_obj(m->instructions, idx);
}

static inline jd_dex_ins* get_dex_ins(jd_method *m, int idx)
{
    return lget_obj(m->instructions, idx);
}

static inline int move_exception_reg_num(jd_dex_ins *ins)
{
    u1 u_a = (ins->param[0] >> 8);
    return u_a;
}

#define dex_ins_meta(ins) (((jd_dex*)ins->method->meta)->meta)

static inline jd_operator dex_ins_operator(jd_ins *ins)
{
    switch (ins->code) {
        case DEX_INS_ADD_INT:
        case DEX_INS_ADD_LONG:
        case DEX_INS_ADD_FLOAT:
        case DEX_INS_ADD_DOUBLE:
        case DEX_INS_ADD_INT_2ADDR:
        case DEX_INS_ADD_LONG_2ADDR:
        case DEX_INS_ADD_FLOAT_2ADDR:
        case DEX_INS_ADD_DOUBLE_2ADDR:
        case DEX_INS_ADD_INT_LIT16:
        case DEX_INS_ADD_INT_LIT8:
            return JD_OP_ADD;
        case DEX_INS_SUB_INT:
        case DEX_INS_SUB_LONG:
        case DEX_INS_SUB_FLOAT:
        case DEX_INS_SUB_DOUBLE:
        case DEX_INS_SUB_INT_2ADDR:
        case DEX_INS_SUB_LONG_2ADDR:
        case DEX_INS_SUB_FLOAT_2ADDR:
        case DEX_INS_SUB_DOUBLE_2ADDR:
        case DEX_INS_RSUB_INT_LIT8:
        case DEX_INS_RSUB_INT:
            return JD_OP_SUB;
        case DEX_INS_MUL_INT:
        case DEX_INS_MUL_LONG:
        case DEX_INS_MUL_FLOAT:
        case DEX_INS_MUL_DOUBLE:
        case DEX_INS_MUL_INT_2ADDR:
        case DEX_INS_MUL_LONG_2ADDR:
        case DEX_INS_MUL_FLOAT_2ADDR:
        case DEX_INS_MUL_INT_LIT16:
        case DEX_INS_MUL_INT_LIT8:
            return JD_OP_MUL;
        case DEX_INS_DIV_INT:
        case DEX_INS_DIV_LONG:
        case DEX_INS_DIV_FLOAT:
        case DEX_INS_DIV_DOUBLE:
        case DEX_INS_DIV_INT_2ADDR:
        case DEX_INS_DIV_LONG_2ADDR:
        case DEX_INS_DIV_FLOAT_2ADDR:
        case DEX_INS_DIV_DOUBLE_2ADDR:
        case DEX_INS_DIV_INT_LIT16:
        case DEX_INS_DIV_INT_LIT8:
            return JD_OP_DIV;
        case DEX_INS_REM_INT:
        case DEX_INS_REM_LONG:
        case DEX_INS_REM_FLOAT:
        case DEX_INS_REM_DOUBLE:
        case DEX_INS_REM_INT_2ADDR:
        case DEX_INS_REM_LONG_2ADDR:
        case DEX_INS_REM_FLOAT_2ADDR:
        case DEX_INS_REM_DOUBLE_2ADDR:
        case DEX_INS_REM_INT_LIT16:
        case DEX_INS_REM_INT_LIT8:
            return JD_OP_REM;
        case DEX_INS_NEG_INT:
        case DEX_INS_NEG_LONG:
        case DEX_INS_NEG_FLOAT:
        case DEX_INS_NEG_DOUBLE:
            return JD_OP_NEG;
        case DEX_INS_SHL_INT:
        case DEX_INS_SHL_LONG:
        case DEX_INS_SHL_INT_2ADDR:
        case DEX_INS_SHL_LONG_2ADDR:
        case DEX_INS_SHL_INT_LIT8:
            return JD_OP_SHL;
        case DEX_INS_SHR_INT:
        case DEX_INS_SHR_LONG:
        case DEX_INS_SHR_INT_2ADDR:
        case DEX_INS_SHR_LONG_2ADDR:
        case DEX_INS_SHR_INT_LIT8:
            return JD_OP_SHR;
        case DEX_INS_USHR_INT:
        case DEX_INS_USHR_LONG:
        case DEX_INS_USHR_INT_2ADDR:
        case DEX_INS_USHR_LONG_2ADDR:
        case DEX_INS_USHR_INT_LIT8:
            return JD_OP_USHR;
        case DEX_INS_AND_INT:
        case DEX_INS_AND_LONG:
        case DEX_INS_AND_INT_2ADDR:
        case DEX_INS_AND_LONG_2ADDR:
        case DEX_INS_AND_INT_LIT16:
        case DEX_INS_AND_INT_LIT8:
            return JD_OP_AND;
        case DEX_INS_OR_INT:
        case DEX_INS_OR_LONG:
        case DEX_INS_OR_INT_2ADDR:
        case DEX_INS_OR_LONG_2ADDR:
        case DEX_INS_OR_INT_LIT16:
        case DEX_INS_OR_INT_LIT8:
            return JD_OP_OR;
        case DEX_INS_XOR_INT:
        case DEX_INS_XOR_LONG:
        case DEX_INS_XOR_INT_2ADDR:
        case DEX_INS_XOR_LONG_2ADDR:
        case DEX_INS_XOR_INT_LIT16:
        case DEX_INS_XOR_INT_LIT8:
            return JD_OP_XOR;
        case DEX_INS_INSTANCE_OF:
            return JD_OP_INSTANCEOF;
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_EQZ:
            return JD_OP_EQ;
        case DEX_INS_IF_NE:
        case DEX_INS_IF_NEZ:
            return JD_OP_NE;
        case DEX_INS_IF_LT:
        case DEX_INS_IF_LTZ:
            return JD_OP_LT;
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GEZ:
            return JD_OP_GE;
        case DEX_INS_IF_GT:
        case DEX_INS_IF_GTZ:
            return JD_OP_GT;
        case DEX_INS_IF_LE:
        case DEX_INS_IF_LEZ:
            return JD_OP_LE;
        case DEX_INS_CMP_LONG:
        case DEX_INS_CMPL_FLOAT:
        case DEX_INS_CMPG_FLOAT:
        case DEX_INS_CMPL_DOUBLE:
        case DEX_INS_CMPG_DOUBLE:
            return JD_OP_CMP;


        default: return JD_OP_UNKNOWN;

    }
}


#endif //GARLIC_DEX_INS_H

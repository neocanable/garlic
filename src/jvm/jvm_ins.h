#ifndef GARLIC_JVM_INS_H
#define GARLIC_JVM_INS_H

#include "jvm/jvm_ins_helper.h"
#include "decompiler/instruction.h"
#include "common/debug.h"

bool match_instruction_patten(jd_exp *expression, int num, ...);

void jvm_ins_setup_opcode(jd_ins *ins);

jd_descriptor* jvm_invoke_descriptor(jd_ins *ins);

jcp_info* jvm_invoke_name_and_type_info(jd_ins *ins);

jcp_info* jvm_invoke_methodref_info(jd_ins *ins);

static inline bool jvm_ins_is_pops(jd_ins *ins)
{
	return ins->code == INS_POP || ins->code == INS_POP2;
}

static inline bool jvm_ins_is_branch(jd_ins *ins)
{
    switch (ins->code) {
        case INS_IFEQ: // ifeq
        case INS_IFNE: // ifne
        case INS_IFLT: // iflt
        case INS_IFGE: // ifge
        case INS_IFGT: // ifgt
        case INS_IFLE: // ifle
        case INS_IF_ICMPEQ: // if_icmpeq
        case INS_IF_ICMPNE: // if_icmpne
        case INS_IF_ICMPLT: // if_icmplt
        case INS_IF_ICMPGE: // if_icmpge
        case INS_IF_ICMPGT: // if_icmpgt
        case INS_IF_ICMPLE: // if_icmple
        case INS_IF_ACMPEQ: // if_acmpeq
        case INS_IF_ACMPNE: // if_acmpne
        case INS_GOTO: // goto
        case INS_JSR: // jsr
        case INS_TABLESWITCH: // tableswitch
        case INS_LOOKUPSWITCH: // lookupswitch
        case INS_IFNULL: // ifnull
        case INS_IFNONNULL: // ifnonnull
        case INS_GOTO_W: // goto_w
        case INS_JSR_W: // jsr_w
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_jump(jd_ins *ins)
{
    switch (ins->code) {
        case INS_IFEQ: // ifeq
        case INS_IFNE: // ifne
        case INS_IFLT: // iflt
        case INS_IFGE: // ifge
        case INS_IFGT: // ifgt
        case INS_IFLE: // ifle
        case INS_IF_ICMPEQ: // if_icmpeq
        case INS_IF_ICMPNE: // if_icmpne
        case INS_IF_ICMPLT: // if_icmplt
        case INS_IF_ICMPGE: // if_icmpge
        case INS_IF_ICMPGT: // if_icmpgt
        case INS_IF_ICMPLE: // if_icmple
        case INS_IF_ACMPEQ: // if_acmpeq
        case INS_IF_ACMPNE: // if_acmpne
        case INS_GOTO: // goto
        case INS_JSR: // jsr
        case INS_TABLESWITCH: // tableswitch
        case INS_LOOKUPSWITCH: // lookupswitch
        case INS_IFNULL: // ifnull
        case INS_IFNONNULL: // ifnonnull
        case INS_GOTO_W: // goto_w
        case INS_JSR_W: // jsr_w
        case INS_ATHROW: // athrow
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_conditional_jump(jd_ins *ins)
{
    switch (ins->code) {
        case INS_IFEQ:
        case INS_IFNE:
        case INS_IFLT:
        case INS_IFGE:
        case INS_IFGT:
        case INS_IFLE:
        case INS_IF_ICMPEQ:
        case INS_IF_ICMPNE:
        case INS_IF_ICMPLT:
        case INS_IF_ICMPGE:
        case INS_IF_ICMPGT:
        case INS_IF_ICMPLE:
        case INS_IF_ACMPEQ:
        case INS_IF_ACMPNE:
        case INS_IFNULL:
        case INS_IFNONNULL:
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_unconditional_jump(jd_ins *ins)
{
    switch (ins->code) {
        case INS_GOTO: // goto
        case INS_JSR: // jsr
        case INS_GOTO_W: // goto_w
        case INS_JSR_W: // jsr_w
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_nonvoid_return(jd_ins *ins)
{
    switch (ins->code) {
        case INS_IRETURN: // ireturn
        case INS_LRETURN: // lreturn
        case INS_FRETURN: // freturn
        case INS_DRETURN: // dreturn
        case INS_ARETURN: // areturn
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_return(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ARETURN: // areturn
        case INS_IRETURN: // ireturn
        case INS_LRETURN: // lreturn
        case INS_FRETURN: // freturn
        case INS_DRETURN: // dreturn
        case INS_RETURN: // return
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_block_start(jd_ins *ins)
{
    return ins_is_jump_destination(ins);
}

static inline bool jvm_ins_is_block_end(jd_ins *ins)
{
    return jvm_ins_is_jump(ins) || jvm_ins_is_return(ins);
}

static inline int jvm_ins_store_slot(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ISTORE: // istore
        case INS_LSTORE: // lstore
        case INS_FSTORE: // fstore
        case INS_DSTORE: // dstore
        case INS_ASTORE: // astore
            return ins->param[0];
        case INS_ISTORE_0: // istore_0
        case INS_FSTORE_0: // fstore_0
        case INS_LSTORE_0: // lstore_0
        case INS_DSTORE_0: // dstore_0
        case INS_ASTORE_0: // astore_0
            return 0;
        case INS_ISTORE_1: // istore_1
        case INS_FSTORE_1: // fstore_1
        case INS_LSTORE_1: // lstore_1
        case INS_DSTORE_1: // dstore_1
        case INS_ASTORE_1: // astore_1
            return 1;
        case INS_ISTORE_2: // istore_2
        case INS_FSTORE_2: // fstore_2
        case INS_LSTORE_2: // lstore_2
        case INS_DSTORE_2: // dstore_2
        case INS_ASTORE_2: // astore_2
            return 2;
        case INS_ISTORE_3: // istore_3
        case INS_FSTORE_3: // fstore_3
        case INS_LSTORE_3: // lstore_3
        case INS_DSTORE_3: // dstore_3
        case INS_ASTORE_3: // astore_3
            return 3;
        case 0xc4: {// wide
            switch (ins->param[0]) {
                case INS_ISTORE: // istore
                case INS_LSTORE: // lstore
                case INS_FSTORE: // fstore
                case INS_DSTORE: // dstore
                case INS_ASTORE: // astore
                    return (ins->param[1] << 8) | ins->param[2];
                default:
                    return -1;
            }
        }
        default:
            return -1;
    }
}

static inline int jvm_ins_load_slot(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ILOAD: // iload
        case INS_LLOAD: // lload
        case INS_FLOAD: // fload
        case INS_DLOAD: // dload
        case INS_ALOAD: // aload
            return ins->param[0];
        case INS_ILOAD_0: // iload_0
        case INS_FLOAD_0: // fload_0
        case INS_LLOAD_0: // lload_0
        case INS_DLOAD_0: // dload_0
        case INS_ALOAD_0: // aload_0
            return 0;
        case INS_ILOAD_1: // iload_1
        case INS_FLOAD_1: // fload_1
        case INS_LLOAD_1: // lload_1
        case INS_DLOAD_1: // dload_1
        case INS_ALOAD_1: // aload_1
            return 1;
        case INS_ILOAD_2: // iload_2
        case INS_FLOAD_2: // fload_2
        case INS_LLOAD_2: // lload_2
        case INS_DLOAD_2: // dload_2
        case INS_ALOAD_2: // aload_2
            return 2;
        case INS_ILOAD_3: // iload_3
        case INS_FLOAD_3: // fload_3
        case INS_LLOAD_3: // lload_3
        case INS_DLOAD_3: // dload_3
        case INS_ALOAD_3: // aload_3
            return 3;
        case INS_WIDE: {// wide
            switch (ins->param[0]) {
                case INS_ILOAD: // iload
                case INS_LLOAD: // lload
                case INS_FLOAD: // fload
                case INS_DLOAD: // dload
                case INS_ALOAD: // aload
                    return (ins->param[1] << 8) | ins->param[2];
                default:
                    return -1;
            }
        }
        default:
            return -1;
    }
}

static inline bool jvm_ins_is_store(jd_ins *ins)
{
    int slot = jvm_ins_store_slot(ins);
    return slot >= 0;
}

static inline bool jvm_ins_is_load(jd_ins *ins)
{
    int slot = jvm_ins_load_slot(ins);
    return slot >= 0;
}

static inline bool jvm_ins_is_xistore(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ISTORE:
        case INS_ISTORE_0:
        case INS_ISTORE_1:
        case INS_ISTORE_2:
        case INS_ISTORE_3:
            return true;
        default:return false;
    }
}

static inline bool jvm_ins_is_xiload(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ILOAD: // iload
        case INS_ILOAD_0: // iload_0
        case INS_ILOAD_1: // iload_1
        case INS_ILOAD_2: // iload_2
        case INS_ILOAD_3: // iload_3
            return true;
        case INS_WIDE: {
            switch (ins->param[0]) {
                case INS_ILOAD: // iload
                    return true;
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

static inline bool jvm_ins_is_array_load(jd_ins *ins)
{
    switch (ins->code) {
        case INS_IALOAD: // iaload
        case INS_LALOAD: // laload
        case INS_FALOAD: // faload
        case INS_DALOAD: // daload
        case INS_AALOAD: // aaload
        case INS_BALOAD: // baload
        case INS_CALOAD: // caload
        case INS_SALOAD: // saload
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_invoke(jd_ins *ins)
{
    switch (ins->code) {
        case INS_INVOKEVIRTUAL: // invokevirtual
        case INS_INVOKESPECIAL: // invokespecial
        case INS_INVOKESTATIC: // invokestatic
        case INS_INVOKEINTERFACE: // invokeinterface
        case INS_INVOKEDYNAMIC: // invokedynamic
            return true;
        default:
            return false;
    }
}

static inline int16_t jvm_if_jump_offset(jd_ins *ins)
{
    return (ins->param[0] << 8 | ins->param[1]) + ins->offset;
}

static inline bool jvm_ins_is_goto(jd_ins *ins)
{
    return (ins->code == INS_GOTO || ins->code == INS_GOTO_W);
}

static inline jd_var_types jvm_ins_load_type(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ILOAD: // iload
        case INS_ILOAD_0: // iload_0
        case INS_ILOAD_1: // iload_1
        case INS_ILOAD_2: // iload_2
        case INS_ILOAD_3: // iload_3
            return JD_VAR_INT_T;
        case INS_LLOAD: // lload
        case INS_LLOAD_0: // lload_0
        case INS_LLOAD_1: // lload_1
        case INS_LLOAD_2: // lload_2
        case INS_LLOAD_3: // lload_3
            return JD_VAR_LONG_T;
        case INS_FLOAD: // fload
        case INS_FLOAD_0: // fload_0
        case INS_FLOAD_1: // fload_1
        case INS_FLOAD_2: // fload_2
        case INS_FLOAD_3: // fload_3
            return JD_VAR_FLOAT_T;
        case INS_DLOAD: // dload
        case INS_DLOAD_0: // dload_0
        case INS_DLOAD_1: // dload_1
        case INS_DLOAD_2: // dload_2
        case INS_DLOAD_3: // dload_3
            return JD_VAR_DOUBLE_T;
        case INS_ALOAD: // aload
        case INS_ALOAD_0: // aload_0
        case INS_ALOAD_1: // aload_1
        case INS_ALOAD_2: // aload_2
        case INS_ALOAD_3: // aload_3
            return JD_VAR_REFERENCE_T;
        case INS_WIDE: {
            switch (ins->param[0]) {
                case INS_ILOAD: // iload
                    return JD_VAR_INT_T;
                case INS_LLOAD: // lload
                    return JD_VAR_LONG_T;
                case INS_FLOAD: // fload
                    return JD_VAR_FLOAT_T;
                case INS_DLOAD: // dload
                    return JD_VAR_DOUBLE_T;
                case INS_ALOAD: // aload
                    return JD_VAR_REFERENCE_T;
                default:
                    return -1;
            }
        }
        default:
            return -1;
    }
}

static inline jd_var_types jvm_ins_store_type(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ISTORE: // istore
        case INS_ISTORE_0: // istore_0
        case INS_ISTORE_1: // istore_1
        case INS_ISTORE_2: // istore_2
        case INS_ISTORE_3: // istore_3
            return JD_VAR_INT_T;
        case INS_LSTORE: // lstore
        case INS_LSTORE_0: // lstore_0
        case INS_LSTORE_1: // lstore_1
        case INS_LSTORE_2: // lstore_2
        case INS_LSTORE_3: // lstore_3
            return JD_VAR_LONG_T;
        case INS_FSTORE: // fstore
        case INS_FSTORE_0: // fstore_0
        case INS_FSTORE_1: // fstore_1
        case INS_FSTORE_2: // fstore_2
        case INS_FSTORE_3: // fstore_3
            return JD_VAR_FLOAT_T;
        case INS_DSTORE: // dstore
        case INS_DSTORE_0: // dstore_0
        case INS_DSTORE_1: // dstore_1
        case INS_DSTORE_2: // dstore_2
        case INS_DSTORE_3: // dstore_3
            return JD_VAR_DOUBLE_T;
        case INS_ASTORE: // astore
        case INS_ASTORE_0: // astore_0
        case INS_ASTORE_1: // astore_1
        case INS_ASTORE_2: // astore_2
        case INS_ASTORE_3: // astore_3
            return JD_VAR_REFERENCE_T;
        case INS_WIDE: {
            switch (ins->param[0]) {
                case INS_ISTORE: // istore
                    return JD_VAR_INT_T;
                case INS_LSTORE: // lstore
                    return JD_VAR_LONG_T;
                case INS_FSTORE: // fstore
                    return JD_VAR_FLOAT_T;
                case INS_DSTORE: // dstore
                    return JD_VAR_DOUBLE_T;
                case INS_ASTORE: // astore
                    return JD_VAR_REFERENCE_T;
                default:
                    return -1;
            }
        }
        default:
            return -1;
    }
}

static inline bool jvm_ins_same_load_type(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_load(ins1) && jvm_ins_is_load(ins2)) {
        if (jvm_ins_load_type(ins1) != -1 &&
                jvm_ins_load_type(ins2) != -1)
            return jvm_ins_load_type(ins1) == jvm_ins_load_type(ins2);
    }
    return false;
}

static inline bool jvm_ins_same_store_type(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_store(ins1) && jvm_ins_is_store(ins2)) {
        if (jvm_ins_store_type(ins1) != -1 &&
                jvm_ins_store_type(ins2) != -1)
            return jvm_ins_store_type(ins1) ==
                    jvm_ins_store_type(ins2);
    }
    return false;
}

static inline bool jvm_ins_same_for_field(jd_ins *ins1, jd_ins *ins2)
{
    if ((jvm_ins_is_getfield(ins1) && jvm_ins_is_getfield(ins2)) ||
        (jvm_ins_is_putfield(ins1) && jvm_ins_is_putfield(ins2)))
        return (ins1->param[0] == ins2->param[0] &&
                ins1->param[1] == ins2->param[1]);
    return false;
}

static inline bool jvm_ins_same_for_static(jd_ins *ins1, jd_ins *ins2)
{
    if ((jvm_ins_is_getstatic(ins1) && jvm_ins_is_getstatic(ins2)) ||
        (jvm_ins_is_putstatic(ins1) && jvm_ins_is_putstatic(ins2)))
        return (ins1->param[0] == ins2->param[0] &&
                ins1->param[1] == ins2->param[1]);
    return false;
}

static inline bool ins_same_for_fetch(jd_ins *ins1, jd_ins *ins2)
{
    return jvm_ins_same_for_field(ins1, ins2) ||
            jvm_ins_same_for_static(ins1, ins2);
}

static inline bool jvm_ins_same_for_int_const(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_bipush(ins1) && jvm_ins_is_bipush(ins2))
        return ins1->param[0] == ins2->param[0];
    if (jvm_ins_is_sipush(ins1) && jvm_ins_is_sipush(ins2))
        return (ins1->param[0] << 8 | ins1->param[1]) ==
               (ins2->param[0] << 8 | ins2->param[1]);
    return false;
}

static inline bool jvm_ins_same_for_invoke(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_invoke(ins1) && jvm_ins_is_invoke(ins2)) {
        if (ins1->code != ins2->code)
            return false;
        return (ins1->param[0] == ins2->param[0] &&
                ins1->param[1] == ins2->param[1]);
    }
    return false;
}

static inline bool jvm_ins_same_for_ldc(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_ldc(ins1) && jvm_ins_is_ldc(ins2))
        return ins1->param[0] == ins2->param[0];
    if (jvm_ins_is_ldc_w(ins1) && jvm_ins_is_ldc_w(ins2))
        return (ins1->param[0] << 8 | ins1->param[1]) ==
               (ins2->param[0] << 8 | ins2->param[1]);
    if (jvm_ins_is_ldc2_w(ins1) && jvm_ins_is_ldc2_w(ins2))
        return (ins1->param[0] << 8 | ins1->param[1]) ==
               (ins2->param[0] << 8 | ins2->param[1]);
    return false;
}

static inline bool jvm_ins_same_for_store(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_store(ins1) && jvm_ins_is_store(ins2))
        return jvm_ins_same_store_type(ins1, ins2);
    return false;
}

static inline bool jvm_ins_same_for_load(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_load(ins1) && jvm_ins_is_load(ins2))
        return jvm_ins_same_load_type(ins1, ins2);
    return false;
}

static inline bool jvm_ins_compares(jd_ins *ins1, jd_ins *ins2)
{
    if (jvm_ins_is_store(ins1))
        return jvm_ins_same_for_store(ins1, ins2);
    if (jvm_ins_is_load(ins1))
        return jvm_ins_same_for_load(ins1, ins2);
    if (jvm_ins_is_getfield(ins1) ||
        jvm_ins_is_putfield(ins1) ||
        jvm_ins_is_getstatic(ins1) ||
        jvm_ins_is_putstatic(ins1))
        return ins_same_for_fetch(ins1, ins2);
    if (jvm_ins_is_invoke(ins1))
        return jvm_ins_same_for_invoke(ins1, ins2);
    if (jvm_ins_is_bipush(ins1) || jvm_ins_is_sipush(ins1))
        return jvm_ins_same_for_int_const(ins1, ins2);
    if (jvm_ins_is_ldc(ins1) ||
            jvm_ins_is_ldc_w(ins1) ||
            jvm_ins_is_ldc2_w(ins1))
        return jvm_ins_same_for_ldc(ins1, ins2);

    return ins1->code == ins2->code;
}

static inline int jvm_ins_iconst_value(jd_ins *ins)
{
    switch (ins->code) {
        case INS_ICONST_0: /*iconst_0*/ return 0;
        case INS_ICONST_1: /*iconst_1*/ return 1;
        case INS_ICONST_2: /*iconst_2*/ return 2;
        case INS_ICONST_3: /*iconst_3*/ return 3;
        case INS_ICONST_4: /*iconst_4*/ return 4;
        case INS_ICONST_5: /*iconst_5*/ return 5;
        case INS_ICONST_M1: /*iconst_m1*/ return -1;
        case INS_BIPUSH: /*bipush*/ return ins->param[0];
        case INS_SIPUSH: /*sipush*/ return  ins->param[0] << 8 | ins->param[1];
        default: return -100;
    }
}

static inline long jvm_ins_lconst_value(jd_ins *ins)
{
    switch (ins->code) {
        case INS_LCONST_0: /*lconst_0*/ return 0;
        case INS_LCONST_1: /*lconst_1*/ return 1;
        default: return -100;
    }
}

static inline float jvm_ins_fconst_value(jd_ins *ins)
{
    switch (ins->code) {
        case INS_FCONST_0: /*fconst_0*/ return 0;
        case INS_FCONST_1: /*fconst_1*/ return 1;
        case INS_FCONST_2: /*fconst_2*/ return 2;
        default: return -100;
    }
}

static inline double jvm_ins_dconst_value(jd_ins *ins)
{
    switch (ins->code) {
        case INS_DCONST_0: /*dconst_0*/ return 0;
        case INS_DCONST_1: /*dconst_1*/ return 1;
        default: return -100;
    }
}

static inline bool jvm_ins_is_if(jd_ins *ins)
{
    switch(ins->code) {
        case INS_IFEQ: // ifeq
        case INS_IFNE: // ifne
        case INS_IFLT: // iflt
        case INS_IFGE: // ifge
        case INS_IFGT: // ifgt
        case INS_IFLE: // ifle
        case INS_IF_ICMPEQ: // if_icmpeq
        case INS_IF_ICMPNE: // if_icmpne
        case INS_IF_ICMPLT: // if_icmplt
        case INS_IF_ICMPGE: // if_icmpge
        case INS_IF_ICMPGT: // if_icmpgt
        case INS_IF_ICMPLE: // if_icmple
        case INS_IF_ACMPEQ: // if_acmpeq
        case INS_IF_ACMPNE: // if_acmpne
        case INS_IFNULL: // ifnull
        case INS_IFNONNULL: // ifnonnull
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_switch(jd_ins *ins)
{
    return ins->code == INS_TABLESWITCH || ins->code == INS_LOOKUPSWITCH;
}

static inline bool jvm_ins_is_goto_back(jd_ins *ins)
{
    if (ins->code == INS_GOTO) {
        u1 param0 = ins->param[0];
        u1 param1 = ins->param[1];
        int16_t offset = (int16_t) param0 << 8 | param1;
        return ins->offset > (ins->offset + offset);
    }
    else if (ins->code == INS_GOTO_W) {
        u1 param0 = ins->param[0];
        u1 param1 = ins->param[1];
        u1 param2 = ins->param[2];
        u1 param3 = ins->param[3];
        int32_t offset = (int32_t) param0 << 24 |
                                   param1 << 16 |
                                   param2 << 8 |
                                   param3;
        return ins->offset > (ins->offset + offset);
    }
    else if (jvm_ins_is_if(ins))
        return jvm_if_jump_offset(ins) < ins->offset;

    return false;
}

static inline bool jvm_ins_is_boolean_const(jd_ins *ins)
{
    return ins->code == INS_ICONST_0 || ins->code == INS_ICONST_1;
}

static inline bool jvm_ins_is_compare(jd_ins *ins)
{
    switch (ins->code) {
        case INS_LCMP:
        case INS_FCMPL:
        case INS_FCMPG:
        case INS_DCMPL:
        case INS_DCMPG:
            return true;
        default:
            return false;
    }
}

static inline bool jvm_ins_is_any(jd_ins *ins)
{
    return true;
}

static inline uint32_t jvm_switch_padding(uint32_t i)
{
    return (4 - (i + 1) % 4) >= 4 ? 0 : (4 - (i + 1) % 4);
}

int32_t jvm_switch_default_offset(jd_ins *ins);

int32_t jvm_switch_key(jd_ins *ins, uint32_t jump_offset);

void jvm_rename_goto2return(jd_method *m);

static inline jd_operator jvm_ins_operator(jd_ins *ins)
{
    switch (ins->code) {
        case INS_IADD:
        case INS_LADD:
        case INS_FADD:
        case INS_DADD:
            return JD_OP_ADD;
        case INS_ISUB:
        case INS_LSUB:
        case INS_FSUB:
        case INS_DSUB:
            return JD_OP_SUB;
        case INS_IMUL:
        case INS_LMUL:
        case INS_FMUL:
        case INS_DMUL:
            return JD_OP_MUL;
        case INS_IDIV:
        case INS_LDIV:
        case INS_FDIV:
        case INS_DDIV:
            return JD_OP_DIV;
        case INS_IREM:
        case INS_LREM:
        case INS_FREM:
        case INS_DREM:
            return JD_OP_REM;
        case INS_INEG:
        case INS_LNEG:
        case INS_FNEG:
        case INS_DNEG:
            return JD_OP_NEG;
        case INS_ISHL:
        case INS_LSHL:
            return JD_OP_SHL;
        case INS_ISHR:
        case INS_LSHR:
            return JD_OP_SHR;
        case INS_IUSHR:
        case INS_LUSHR:
            return JD_OP_USHR;
        case INS_IAND:
        case INS_LAND:
            return JD_OP_AND;
        case INS_IOR:
        case INS_LOR:
            return JD_OP_OR;
        case INS_IXOR:
        case INS_LXOR:
            return JD_OP_XOR;
        case INS_INSTANCEOF:
            return JD_OP_INSTANCEOF;
        case INS_IFEQ:
        case INS_IF_ICMPEQ:
        case INS_IF_ACMPEQ:
            return JD_OP_EQ;
        case INS_IFNE: // ifne
        case INS_IF_ICMPNE: // if_icmpne
        case INS_IF_ACMPNE: // if_acmpne
            return JD_OP_NE;
        case INS_IFLT: // iflt
        case INS_IF_ICMPLT: // if_icmplt
            return JD_OP_LT;
        case INS_IFGE: // ifge
        case INS_IF_ICMPGE: // if_icmpge
            return JD_OP_GE;
        case INS_IFGT: // ifgt
        case INS_IF_ICMPGT: // if_icmpgt
            return JD_OP_GT;
        case INS_IFLE: // ifle
        case INS_IF_ICMPLE: // if_icmple
            return JD_OP_LE;
        case INS_IFNULL: // ifnull
            return JD_OP_EQ;
        case INS_IFNONNULL:// ifnonnull
            return JD_OP_NE;
        case INS_LCMP:
        case INS_FCMPG:
        case INS_FCMPL:
        case INS_DCMPL:
        case INS_DCMPG:
            return JD_OP_CMP;

        default: return JD_OP_UNKNOWN;

    }
}

static inline bool jvm_ins_is_dup_action(jd_ins *ins)
{
    switch(ins->code) {
        case INS_DUP:
        case INS_DUP_X1:
        case INS_DUP_X2:
        case INS_DUP2:
        case INS_DUP2_X1:
        case INS_DUP2_X2:
            return true;
        default:
            return false;
    }
}

#endif //GARLIC_JVM_INS_H

#ifndef GARLIC_JVM_INS_HELPER_H
#define GARLIC_JVM_INS_HELPER_H

#include "decompiler/structure.h"


#define INS_NOP                 0x00
#define INS_ACONST_NULL         0x01
#define INS_ICONST_M1           0x02
#define INS_ICONST_0            0x03
#define INS_ICONST_1            0x04
#define INS_ICONST_2            0x05
#define INS_ICONST_3            0x06
#define INS_ICONST_4            0x07
#define INS_ICONST_5            0x08
#define INS_LCONST_0            0x09
#define INS_LCONST_1            0x0a
#define INS_FCONST_0            0x0b
#define INS_FCONST_1            0x0c
#define INS_FCONST_2            0x0d
#define INS_DCONST_0            0x0e
#define INS_DCONST_1            0x0f
#define INS_BIPUSH              0x10
#define INS_SIPUSH              0x11
#define INS_LDC                 0x12
#define INS_LDC_W               0x13
#define INS_LDC2_W              0x14
#define INS_ILOAD               0x15
#define INS_LLOAD               0x16
#define INS_FLOAD               0x17
#define INS_DLOAD               0x18
#define INS_ALOAD               0x19
#define INS_ILOAD_0             0x1a
#define INS_ILOAD_1             0x1b
#define INS_ILOAD_2             0x1c
#define INS_ILOAD_3             0x1d
#define INS_LLOAD_0             0x1e
#define INS_LLOAD_1             0x1f
#define INS_LLOAD_2             0x20
#define INS_LLOAD_3             0x21
#define INS_FLOAD_0             0x22
#define INS_FLOAD_1             0x23
#define INS_FLOAD_2             0x24
#define INS_FLOAD_3             0x25
#define INS_DLOAD_0             0x26
#define INS_DLOAD_1             0x27
#define INS_DLOAD_2             0x28
#define INS_DLOAD_3             0x29
#define INS_ALOAD_0             0x2a
#define INS_ALOAD_1             0x2b
#define INS_ALOAD_2             0x2c
#define INS_ALOAD_3             0x2d
#define INS_IALOAD              0x2e
#define INS_LALOAD              0x2f
#define INS_FALOAD              0x30
#define INS_DALOAD              0x31
#define INS_AALOAD              0x32
#define INS_BALOAD              0x33
#define INS_CALOAD              0x34
#define INS_SALOAD              0x35
#define INS_ISTORE              0x36
#define INS_LSTORE              0x37
#define INS_FSTORE              0x38
#define INS_DSTORE              0x39
#define INS_ASTORE              0x3a
#define INS_ISTORE_0            0x3b
#define INS_ISTORE_1            0x3c
#define INS_ISTORE_2            0x3d
#define INS_ISTORE_3            0x3e
#define INS_LSTORE_0            0x3f
#define INS_LSTORE_1            0x40
#define INS_LSTORE_2            0x41
#define INS_LSTORE_3            0x42
#define INS_FSTORE_0            0x43
#define INS_FSTORE_1            0x44
#define INS_FSTORE_2            0x45
#define INS_FSTORE_3            0x46
#define INS_DSTORE_0            0x47
#define INS_DSTORE_1            0x48
#define INS_DSTORE_2            0x49
#define INS_DSTORE_3            0x4a
#define INS_ASTORE_0            0x4b
#define INS_ASTORE_1            0x4c
#define INS_ASTORE_2            0x4d
#define INS_ASTORE_3            0x4e
#define INS_IASTORE             0x4f
#define INS_LASTORE             0x50
#define INS_FASTORE             0x51
#define INS_DASTORE             0x52
#define INS_AASTORE             0x53
#define INS_BASTORE             0x54
#define INS_CASTORE             0x55
#define INS_SASTORE             0x56
#define INS_POP                 0x57
#define INS_POP2                0x58
#define INS_DUP                 0x59
#define INS_DUP_X1              0x5a
#define INS_DUP_X2              0x5b
#define INS_DUP2                0x5c
#define INS_DUP2_X1             0x5d
#define INS_DUP2_X2             0x5e
#define INS_SWAP                0x5f
#define INS_IADD                0x60
#define INS_LADD                0x61
#define INS_FADD                0x62
#define INS_DADD                0x63
#define INS_ISUB                0x64
#define INS_LSUB                0x65
#define INS_FSUB                0x66
#define INS_DSUB                0x67
#define INS_IMUL                0x68
#define INS_LMUL                0x69
#define INS_FMUL                0x6a
#define INS_DMUL                0x6b
#define INS_IDIV                0x6c
#define INS_LDIV                0x6d
#define INS_FDIV                0x6e
#define INS_DDIV                0x6f
#define INS_IREM                0x70
#define INS_LREM                0x71
#define INS_FREM                0x72
#define INS_DREM                0x73
#define INS_INEG                0x74
#define INS_LNEG                0x75
#define INS_FNEG                0x76
#define INS_DNEG                0x77
#define INS_ISHL                0x78
#define INS_LSHL                0x79
#define INS_ISHR                0x7a
#define INS_LSHR                0x7b
#define INS_IUSHR               0x7c
#define INS_LUSHR               0x7d
#define INS_IAND                0x7e
#define INS_LAND                0x7f
#define INS_IOR                 0x80
#define INS_LOR                 0x81
#define INS_IXOR                0x82
#define INS_LXOR                0x83
#define INS_IINC                0x84
#define INS_I2L                 0x85
#define INS_I2F                 0x86
#define INS_I2D                 0x87
#define INS_L2I                 0x88
#define INS_L2F                 0x89
#define INS_L2D                 0x8a
#define INS_F2I                 0x8b
#define INS_F2L                 0x8c
#define INS_F2D                 0x8d
#define INS_D2I                 0x8e
#define INS_D2L                 0x8f
#define INS_D2F                 0x90
#define INS_I2B                 0x91
#define INS_I2C                 0x92
#define INS_I2S                 0x93
#define INS_LCMP                0x94
#define INS_FCMPL               0x95
#define INS_FCMPG               0x96
#define INS_DCMPL               0x97
#define INS_DCMPG               0x98
#define INS_IFEQ                0x99
#define INS_IFNE                0x9a
#define INS_IFLT                0x9b
#define INS_IFGE                0x9c
#define INS_IFGT                0x9d
#define INS_IFLE                0x9e
#define INS_IF_ICMPEQ           0x9f
#define INS_IF_ICMPNE           0xa0
#define INS_IF_ICMPLT           0xa1
#define INS_IF_ICMPGE           0xa2
#define INS_IF_ICMPGT           0xa3
#define INS_IF_ICMPLE           0xa4
#define INS_IF_ACMPEQ           0xa5
#define INS_IF_ACMPNE           0xa6
#define INS_GOTO                0xa7
#define INS_JSR                 0xa8
#define INS_RET                 0xa9
#define INS_TABLESWITCH         0xaa
#define INS_LOOKUPSWITCH        0xab
#define INS_IRETURN             0xac
#define INS_LRETURN             0xad
#define INS_FRETURN             0xae
#define INS_DRETURN             0xaf
#define INS_ARETURN             0xb0
#define INS_RETURN              0xb1
#define INS_GETSTATIC           0xb2
#define INS_PUTSTATIC           0xb3
#define INS_GETFIELD            0xb4
#define INS_PUTFIELD            0xb5
#define INS_INVOKEVIRTUAL       0xb6
#define INS_INVOKESPECIAL       0xb7
#define INS_INVOKESTATIC        0xb8
#define INS_INVOKEINTERFACE     0xb9
#define INS_INVOKEDYNAMIC       0xba
#define INS_NEW                 0xbb
#define INS_NEWARRAY            0xbc
#define INS_ANEWARRAY           0xbd
#define INS_ARRAYLENGTH         0xbe
#define INS_ATHROW              0xbf
#define INS_CHECKCAST           0xc0
#define INS_INSTANCEOF          0xc1
#define INS_MONITORENTER        0xc2
#define INS_MONITOREXIT         0xc3
#define INS_WIDE                0xc4
#define INS_MULTIANEWARRAY      0xc5
#define INS_IFNULL              0xc6
#define INS_IFNONNULL           0xc7
#define INS_GOTO_W              0xc8
#define INS_JSR_W               0xc9
#define INS_BREAKPOINT          0xca
#define INS_IMPDEP1             0xfe
#define INS_LEAVEFINALLY        0xff

#define DEFINE_INS_HELPER_METHOD(opcode, code_name) \
    static inline bool                              \
    jvm_ins_is_##code_name(jd_ins *ins)             \
    {                                               \
        return ins->code == opcode;                 \
    }

DEFINE_INS_HELPER_METHOD(INS_NOP, nop)
DEFINE_INS_HELPER_METHOD(INS_ACONST_NULL, aconst_null)
DEFINE_INS_HELPER_METHOD(INS_ICONST_M1, iconst_m1)
DEFINE_INS_HELPER_METHOD(INS_ICONST_0, iconst_0)
DEFINE_INS_HELPER_METHOD(INS_ICONST_1, iconst_1)
DEFINE_INS_HELPER_METHOD(INS_ICONST_2, iconst_2)
DEFINE_INS_HELPER_METHOD(INS_ICONST_3, iconst_3)
DEFINE_INS_HELPER_METHOD(INS_ICONST_4, iconst_4)
DEFINE_INS_HELPER_METHOD(INS_ICONST_5, iconst_5)
DEFINE_INS_HELPER_METHOD(INS_LCONST_0, lconst_0)
DEFINE_INS_HELPER_METHOD(INS_LCONST_1, lconst_1)
DEFINE_INS_HELPER_METHOD(INS_FCONST_0, fconst_0)
DEFINE_INS_HELPER_METHOD(INS_FCONST_1, fconst_1)
DEFINE_INS_HELPER_METHOD(INS_FCONST_2, fconst_2)
DEFINE_INS_HELPER_METHOD(INS_DCONST_0, dconst_0)
DEFINE_INS_HELPER_METHOD(INS_DCONST_1, dconst_1)
DEFINE_INS_HELPER_METHOD(INS_BIPUSH, bipush)
DEFINE_INS_HELPER_METHOD(INS_SIPUSH, sipush)
DEFINE_INS_HELPER_METHOD(INS_LDC, ldc)
DEFINE_INS_HELPER_METHOD(INS_LDC_W, ldc_w)
DEFINE_INS_HELPER_METHOD(INS_LDC2_W, ldc2_w)
DEFINE_INS_HELPER_METHOD(INS_ILOAD, iload)
DEFINE_INS_HELPER_METHOD(INS_LLOAD, lload)
DEFINE_INS_HELPER_METHOD(INS_FLOAD, fload)
DEFINE_INS_HELPER_METHOD(INS_DLOAD, dload)
DEFINE_INS_HELPER_METHOD(INS_ALOAD, aload)
DEFINE_INS_HELPER_METHOD(INS_ILOAD_0, iload_0)
DEFINE_INS_HELPER_METHOD(INS_ILOAD_1, iload_1)
DEFINE_INS_HELPER_METHOD(INS_ILOAD_2, iload_2)
DEFINE_INS_HELPER_METHOD(INS_ILOAD_3, iload_3)
DEFINE_INS_HELPER_METHOD(INS_LLOAD_0, lload_0)
DEFINE_INS_HELPER_METHOD(INS_LLOAD_1, lload_1)
DEFINE_INS_HELPER_METHOD(INS_LLOAD_2, lload_2)
DEFINE_INS_HELPER_METHOD(INS_LLOAD_3, lload_3)
DEFINE_INS_HELPER_METHOD(INS_FLOAD_0, fload_0)
DEFINE_INS_HELPER_METHOD(INS_FLOAD_1, fload_1)
DEFINE_INS_HELPER_METHOD(INS_FLOAD_2, fload_2)
DEFINE_INS_HELPER_METHOD(INS_FLOAD_3, fload_3)
DEFINE_INS_HELPER_METHOD(INS_DLOAD_0, dload_0)
DEFINE_INS_HELPER_METHOD(INS_DLOAD_1, dload_1)
DEFINE_INS_HELPER_METHOD(INS_DLOAD_2, dload_2)
DEFINE_INS_HELPER_METHOD(INS_DLOAD_3, dload_3)
DEFINE_INS_HELPER_METHOD(INS_ALOAD_0, aload_0)
DEFINE_INS_HELPER_METHOD(INS_ALOAD_1, aload_1)
DEFINE_INS_HELPER_METHOD(INS_ALOAD_2, aload_2)
DEFINE_INS_HELPER_METHOD(INS_ALOAD_3, aload_3)
DEFINE_INS_HELPER_METHOD(INS_IALOAD, iaload)
DEFINE_INS_HELPER_METHOD(INS_LALOAD, laload)
DEFINE_INS_HELPER_METHOD(INS_FALOAD, faload)
DEFINE_INS_HELPER_METHOD(INS_DALOAD, daload)
DEFINE_INS_HELPER_METHOD(INS_AALOAD, aaload)
DEFINE_INS_HELPER_METHOD(INS_BALOAD, baload)
DEFINE_INS_HELPER_METHOD(INS_CALOAD, caload)
DEFINE_INS_HELPER_METHOD(INS_SALOAD, saload)
DEFINE_INS_HELPER_METHOD(INS_ISTORE, istore)
DEFINE_INS_HELPER_METHOD(INS_LSTORE, lstore)
DEFINE_INS_HELPER_METHOD(INS_FSTORE, fstore)
DEFINE_INS_HELPER_METHOD(INS_DSTORE, dstore)
DEFINE_INS_HELPER_METHOD(INS_ASTORE, astore)
DEFINE_INS_HELPER_METHOD(INS_ISTORE_0, istore_0)
DEFINE_INS_HELPER_METHOD(INS_ISTORE_1, istore_1)
DEFINE_INS_HELPER_METHOD(INS_ISTORE_2, istore_2)
DEFINE_INS_HELPER_METHOD(INS_ISTORE_3, istore_3)
DEFINE_INS_HELPER_METHOD(INS_LSTORE_0, lstore_0)
DEFINE_INS_HELPER_METHOD(INS_LSTORE_1, lstore_1)
DEFINE_INS_HELPER_METHOD(INS_LSTORE_2, lstore_2)
DEFINE_INS_HELPER_METHOD(INS_LSTORE_3, lstore_3)
DEFINE_INS_HELPER_METHOD(INS_FSTORE_0, fstore_0)
DEFINE_INS_HELPER_METHOD(INS_FSTORE_1, fstore_1)
DEFINE_INS_HELPER_METHOD(INS_FSTORE_2, fstore_2)
DEFINE_INS_HELPER_METHOD(INS_FSTORE_3, fstore_3)
DEFINE_INS_HELPER_METHOD(INS_DSTORE_0, dstore_0)
DEFINE_INS_HELPER_METHOD(INS_DSTORE_1, dstore_1)
DEFINE_INS_HELPER_METHOD(INS_DSTORE_2, dstore_2)
DEFINE_INS_HELPER_METHOD(INS_DSTORE_3, dstore_3)
DEFINE_INS_HELPER_METHOD(INS_ASTORE_0, astore_0)
DEFINE_INS_HELPER_METHOD(INS_ASTORE_1, astore_1)
DEFINE_INS_HELPER_METHOD(INS_ASTORE_2, astore_2)
DEFINE_INS_HELPER_METHOD(INS_ASTORE_3, astore_3)
DEFINE_INS_HELPER_METHOD(INS_IASTORE, iastore)
DEFINE_INS_HELPER_METHOD(INS_LASTORE, lastore)
DEFINE_INS_HELPER_METHOD(INS_FASTORE, fastore)
DEFINE_INS_HELPER_METHOD(INS_DASTORE, dastore)
DEFINE_INS_HELPER_METHOD(INS_AASTORE, aastore)
DEFINE_INS_HELPER_METHOD(INS_BASTORE, bastore)
DEFINE_INS_HELPER_METHOD(INS_CASTORE, castore)
DEFINE_INS_HELPER_METHOD(INS_SASTORE, sastore)
DEFINE_INS_HELPER_METHOD(INS_POP, pop)
DEFINE_INS_HELPER_METHOD(INS_POP2, pop2)
DEFINE_INS_HELPER_METHOD(INS_DUP, dup)
DEFINE_INS_HELPER_METHOD(INS_DUP_X1, dup_x1)
DEFINE_INS_HELPER_METHOD(INS_DUP_X2, dup_x2)
DEFINE_INS_HELPER_METHOD(INS_DUP2, dup2)
DEFINE_INS_HELPER_METHOD(INS_DUP2_X1, dup2_x1)
DEFINE_INS_HELPER_METHOD(INS_DUP2_X2, dup2_x2)
DEFINE_INS_HELPER_METHOD(INS_SWAP, swap)
DEFINE_INS_HELPER_METHOD(INS_IADD, iadd)
DEFINE_INS_HELPER_METHOD(INS_LADD, ladd)
DEFINE_INS_HELPER_METHOD(INS_FADD, fadd)
DEFINE_INS_HELPER_METHOD(INS_DADD, dadd)
DEFINE_INS_HELPER_METHOD(INS_ISUB, isub)
DEFINE_INS_HELPER_METHOD(INS_LSUB, lsub)
DEFINE_INS_HELPER_METHOD(INS_FSUB, fsub)
DEFINE_INS_HELPER_METHOD(INS_DSUB, dsub)
DEFINE_INS_HELPER_METHOD(INS_IMUL, imul)
DEFINE_INS_HELPER_METHOD(INS_LMUL, lmul)
DEFINE_INS_HELPER_METHOD(INS_FMUL, fmul)
DEFINE_INS_HELPER_METHOD(INS_DMUL, dmul)
DEFINE_INS_HELPER_METHOD(INS_IDIV, idiv)
DEFINE_INS_HELPER_METHOD(INS_LDIV, ldiv)
DEFINE_INS_HELPER_METHOD(INS_FDIV, fdiv)
DEFINE_INS_HELPER_METHOD(INS_DDIV, ddiv)
DEFINE_INS_HELPER_METHOD(INS_IREM, irem)
DEFINE_INS_HELPER_METHOD(INS_LREM, lrem)
DEFINE_INS_HELPER_METHOD(INS_FREM, frem)
DEFINE_INS_HELPER_METHOD(INS_DREM, drem)
DEFINE_INS_HELPER_METHOD(INS_INEG, ineg)
DEFINE_INS_HELPER_METHOD(INS_LNEG, lneg)
DEFINE_INS_HELPER_METHOD(INS_FNEG, fneg)
DEFINE_INS_HELPER_METHOD(INS_DNEG, dneg)
DEFINE_INS_HELPER_METHOD(INS_ISHL, ishl)
DEFINE_INS_HELPER_METHOD(INS_LSHL, lshl)
DEFINE_INS_HELPER_METHOD(INS_ISHR, ishr)
DEFINE_INS_HELPER_METHOD(INS_LSHR, lshr)
DEFINE_INS_HELPER_METHOD(INS_IUSHR, iushr)
DEFINE_INS_HELPER_METHOD(INS_LUSHR, lushr)
DEFINE_INS_HELPER_METHOD(INS_IAND, iand)
DEFINE_INS_HELPER_METHOD(INS_LAND, land)
DEFINE_INS_HELPER_METHOD(INS_IOR, ior)
DEFINE_INS_HELPER_METHOD(INS_LOR, lor)
DEFINE_INS_HELPER_METHOD(INS_IXOR, ixor)
DEFINE_INS_HELPER_METHOD(INS_LXOR, lxor)
DEFINE_INS_HELPER_METHOD(INS_IINC, iinc)
DEFINE_INS_HELPER_METHOD(INS_I2L, i2l)
DEFINE_INS_HELPER_METHOD(INS_I2F, i2f)
DEFINE_INS_HELPER_METHOD(INS_I2D, i2d)
DEFINE_INS_HELPER_METHOD(INS_L2I, l2i)
DEFINE_INS_HELPER_METHOD(INS_L2F, l2f)
DEFINE_INS_HELPER_METHOD(INS_L2D, l2d)
DEFINE_INS_HELPER_METHOD(INS_F2I, f2i)
DEFINE_INS_HELPER_METHOD(INS_F2L, f2l)
DEFINE_INS_HELPER_METHOD(INS_F2D, f2d)
DEFINE_INS_HELPER_METHOD(INS_D2I, d2i)
DEFINE_INS_HELPER_METHOD(INS_D2L, d2l)
DEFINE_INS_HELPER_METHOD(INS_D2F, d2f)
DEFINE_INS_HELPER_METHOD(INS_I2B, i2b)
DEFINE_INS_HELPER_METHOD(INS_I2C, i2c)
DEFINE_INS_HELPER_METHOD(INS_I2S, i2s)
DEFINE_INS_HELPER_METHOD(INS_LCMP, lcmp)
DEFINE_INS_HELPER_METHOD(INS_FCMPL, fcmpl)
DEFINE_INS_HELPER_METHOD(INS_FCMPG, fcmpg)
DEFINE_INS_HELPER_METHOD(INS_DCMPL, dcmpl)
DEFINE_INS_HELPER_METHOD(INS_DCMPG, dcmpg)
DEFINE_INS_HELPER_METHOD(INS_IFEQ, ifeq)
DEFINE_INS_HELPER_METHOD(INS_IFNE, ifne)
DEFINE_INS_HELPER_METHOD(INS_IFLT, iflt)
DEFINE_INS_HELPER_METHOD(INS_IFGE, ifge)
DEFINE_INS_HELPER_METHOD(INS_IFGT, ifgt)
DEFINE_INS_HELPER_METHOD(INS_IFLE, ifle)
DEFINE_INS_HELPER_METHOD(INS_IF_ICMPEQ, if_icmpeq)
DEFINE_INS_HELPER_METHOD(INS_IF_ICMPNE, if_icmpne)
DEFINE_INS_HELPER_METHOD(INS_IF_ICMPLT, if_icmplt)
DEFINE_INS_HELPER_METHOD(INS_IF_ICMPGE, if_icmpge)
DEFINE_INS_HELPER_METHOD(INS_IF_ICMPGT, if_icmpgt)
DEFINE_INS_HELPER_METHOD(INS_IF_ICMPLE, if_icmple)
DEFINE_INS_HELPER_METHOD(INS_IF_ACMPEQ, if_acmpeq)
DEFINE_INS_HELPER_METHOD(INS_IF_ACMPNE, if_acmpne)
DEFINE_INS_HELPER_METHOD(INS_GOTO, gotojump)
DEFINE_INS_HELPER_METHOD(INS_JSR, jsr)
DEFINE_INS_HELPER_METHOD(INS_RET, ret)
DEFINE_INS_HELPER_METHOD(INS_TABLESWITCH, tableswitch)
DEFINE_INS_HELPER_METHOD(INS_LOOKUPSWITCH, lookupswitch)
DEFINE_INS_HELPER_METHOD(INS_IRETURN, ireturn)
DEFINE_INS_HELPER_METHOD(INS_LRETURN, lreturn)
DEFINE_INS_HELPER_METHOD(INS_FRETURN, freturn)
DEFINE_INS_HELPER_METHOD(INS_DRETURN, dreturn)
DEFINE_INS_HELPER_METHOD(INS_ARETURN, areturn)
DEFINE_INS_HELPER_METHOD(INS_RETURN, voidreturn)
DEFINE_INS_HELPER_METHOD(INS_GETSTATIC, getstatic)
DEFINE_INS_HELPER_METHOD(INS_PUTSTATIC, putstatic)
DEFINE_INS_HELPER_METHOD(INS_GETFIELD, getfield)
DEFINE_INS_HELPER_METHOD(INS_PUTFIELD, putfield)
DEFINE_INS_HELPER_METHOD(INS_INVOKEVIRTUAL, invokevirtual)
DEFINE_INS_HELPER_METHOD(INS_INVOKESPECIAL, invokespecial)
DEFINE_INS_HELPER_METHOD(INS_INVOKESTATIC, invokestatic)
DEFINE_INS_HELPER_METHOD(INS_INVOKEINTERFACE, invokeinterface)
DEFINE_INS_HELPER_METHOD(INS_INVOKEDYNAMIC, invokedynamic)
DEFINE_INS_HELPER_METHOD(INS_NEW, newobj)
DEFINE_INS_HELPER_METHOD(INS_NEWARRAY, newarray)
DEFINE_INS_HELPER_METHOD(INS_ANEWARRAY, anewarray)
DEFINE_INS_HELPER_METHOD(INS_ARRAYLENGTH, arraylength)
DEFINE_INS_HELPER_METHOD(INS_ATHROW, athrow)
DEFINE_INS_HELPER_METHOD(INS_CHECKCAST, checkcast)
DEFINE_INS_HELPER_METHOD(INS_INSTANCEOF, instanceof)
DEFINE_INS_HELPER_METHOD(INS_MONITORENTER, monitorenter)
DEFINE_INS_HELPER_METHOD(INS_MONITOREXIT, monitorexit)
DEFINE_INS_HELPER_METHOD(INS_WIDE, wide)
DEFINE_INS_HELPER_METHOD(INS_MULTIANEWARRAY, multianewarray)
DEFINE_INS_HELPER_METHOD(INS_IFNULL, ifnull)
DEFINE_INS_HELPER_METHOD(INS_IFNONNULL, ifnonnull)
DEFINE_INS_HELPER_METHOD(INS_GOTO_W, goto_w)
DEFINE_INS_HELPER_METHOD(INS_JSR_W, jsr_w)
DEFINE_INS_HELPER_METHOD(INS_BREAKPOINT, breakpoint)
DEFINE_INS_HELPER_METHOD(INS_IMPDEP1, impdep1)
DEFINE_INS_HELPER_METHOD(INS_LEAVEFINALLY, leavefinally)

#endif //GARLIC_JVM_INS_HELPER_H
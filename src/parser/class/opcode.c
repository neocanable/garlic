#include "hashmap_tools.h"
#include "metadata.h"

#include "jvm/jvm_ins_helper.h"

string jvm_opcode_name(u1 code)
{
    switch(code) {
        case INS_NOP: return "nop";
        case INS_ACONST_NULL: return "aconst_null";
        case INS_ICONST_M1: return "iconst_m1";
        case INS_ICONST_0: return "iconst_0";
        case INS_ICONST_1: return "iconst_1";
        case INS_ICONST_2: return "iconst_2";
        case INS_ICONST_3: return "iconst_3";
        case INS_ICONST_4: return "iconst_4";
        case INS_ICONST_5: return "iconst_5";
        case INS_LCONST_0: return "lconst_0";
        case INS_LCONST_1: return "lconst_1";
        case INS_FCONST_0: return "fconst_0";
        case INS_FCONST_1: return "fconst_1";
        case INS_FCONST_2: return "fconst_2";
        case INS_DCONST_0: return "dconst_0";
        case INS_DCONST_1: return "dconst_1";
        case INS_BIPUSH: return "bipush";
        case INS_SIPUSH: return "sipush";
        case INS_LDC: return "ldc";
        case INS_LDC_W: return "ldc_w";
        case INS_LDC2_W: return "ldc2_w";
        case INS_ILOAD: return "iload";
        case INS_LLOAD: return "lload";
        case INS_FLOAD: return "fload";
        case INS_DLOAD: return "dload";
        case INS_ALOAD: return "aload";
        case INS_ILOAD_0: return "iload_0";
        case INS_ILOAD_1: return "iload_1";
        case INS_ILOAD_2: return "iload_2";
        case INS_ILOAD_3: return "iload_3";
        case INS_LLOAD_0: return "lload_0";
        case INS_LLOAD_1: return "lload_1";
        case INS_LLOAD_2: return "lload_2";
        case INS_LLOAD_3: return "lload_3";
        case INS_FLOAD_0: return "fload_0";
        case INS_FLOAD_1: return "fload_1";
        case INS_FLOAD_2: return "fload_2";
        case INS_FLOAD_3: return "fload_3";
        case INS_DLOAD_0: return "dload_0";
        case INS_DLOAD_1: return "dload_1";
        case INS_DLOAD_2: return "dload_2";
        case INS_DLOAD_3: return "dload_3";
        case INS_ALOAD_0: return "aload_0";
        case INS_ALOAD_1: return "aload_1";
        case INS_ALOAD_2: return "aload_2";
        case INS_ALOAD_3: return "aload_3";
        case INS_IALOAD: return "iaload";
        case INS_LALOAD: return "laload";
        case INS_FALOAD: return "faload";
        case INS_DALOAD: return "daload";
        case INS_AALOAD: return "aaload";
        case INS_BALOAD: return "baload";
        case INS_CALOAD: return "caload";
        case INS_SALOAD: return "saload";
        case INS_ISTORE: return "istore";
        case INS_LSTORE: return "lstore";
        case INS_FSTORE: return "fstore";
        case INS_DSTORE: return "dstore";
        case INS_ASTORE: return "astore";
        case INS_ISTORE_0: return "istore_0";
        case INS_ISTORE_1: return "istore_1";
        case INS_ISTORE_2: return "istore_2";
        case INS_ISTORE_3: return "istore_3";
        case INS_LSTORE_0: return "lstore_0";
        case INS_LSTORE_1: return "lstore_1";
        case INS_LSTORE_2: return "lstore_2";
        case INS_LSTORE_3: return "lstore_3";
        case INS_FSTORE_0: return "fstore_0";
        case INS_FSTORE_1: return "fstore_1";
        case INS_FSTORE_2: return "fstore_2";
        case INS_FSTORE_3: return "fstore_3";
        case INS_DSTORE_0: return "dstore_0";
        case INS_DSTORE_1: return "dstore_1";
        case INS_DSTORE_2: return "dstore_2";
        case INS_DSTORE_3: return "dstore_3";
        case INS_ASTORE_0: return "astore_0";
        case INS_ASTORE_1: return "astore_1";
        case INS_ASTORE_2: return "astore_2";
        case INS_ASTORE_3: return "astore_3";
        case INS_IASTORE: return "iastore";
        case INS_LASTORE: return "lastore";
        case INS_FASTORE: return "fastore";
        case INS_DASTORE: return "dastore";
        case INS_AASTORE: return "aastore";
        case INS_BASTORE: return "bastore";
        case INS_CASTORE: return "castore";
        case INS_SASTORE: return "sastore";
        case INS_POP: return "pop";
        case INS_POP2: return "pop2";
        case INS_DUP: return "dup";
        case INS_DUP_X1: return "dup_x1";
        case INS_DUP_X2: return "dup_x2";
        case INS_DUP2: return "dup2";
        case INS_DUP2_X1: return "dup2_x1";
        case INS_DUP2_X2: return "dup2_x2";
        case INS_SWAP: return "swap";
        case INS_IADD: return "iadd";
        case INS_LADD: return "ladd";
        case INS_FADD: return "fadd";
        case INS_DADD: return "dadd";
        case INS_ISUB: return "isub";
        case INS_LSUB: return "lsub";
        case INS_FSUB: return "fsub";
        case INS_DSUB: return "dsub";
        case INS_IMUL: return "imul";
        case INS_LMUL: return "lmul";
        case INS_FMUL: return "fmul";
        case INS_DMUL: return "dmul";
        case INS_IDIV: return "idiv";
        case INS_LDIV: return "ldiv";
        case INS_FDIV: return "fdiv";
        case INS_DDIV: return "ddiv";
        case INS_IREM: return "irem";
        case INS_LREM: return "lrem";
        case INS_FREM: return "frem";
        case INS_DREM: return "drem";
        case INS_INEG: return "ineg";
        case INS_LNEG: return "lneg";
        case INS_FNEG: return "fneg";
        case INS_DNEG: return "dneg";
        case INS_ISHL: return "ishl";
        case INS_LSHL: return "lshl";
        case INS_ISHR: return "ishr";
        case INS_LSHR: return "lshr";
        case INS_IUSHR: return "iushr";
        case INS_LUSHR: return "lushr";
        case INS_IAND: return "iand";
        case INS_LAND: return "land";
        case INS_IOR: return "ior";
        case INS_LOR: return "lor";
        case INS_IXOR: return "ixor";
        case INS_LXOR: return "lxor";
        case INS_IINC: return "iinc";
        case INS_I2L: return "i2l";
        case INS_I2F: return "i2f";
        case INS_I2D: return "i2d";
        case INS_L2I: return "l2i";
        case INS_L2F: return "l2f";
        case INS_L2D: return "l2d";
        case INS_F2I: return "f2i";
        case INS_F2L: return "f2l";
        case INS_F2D: return "f2d";
        case INS_D2I: return "d2i";
        case INS_D2L: return "d2l";
        case INS_D2F: return "d2f";
        case INS_I2B: return "i2b";
        case INS_I2C: return "i2c";
        case INS_I2S: return "i2s";
        case INS_LCMP: return "lcmp";
        case INS_FCMPL: return "fcmpl";
        case INS_FCMPG: return "fcmpg";
        case INS_DCMPL: return "dcmpl";
        case INS_DCMPG: return "dcmpg";
        case INS_IFEQ: return "ifeq";
        case INS_IFNE: return "ifne";
        case INS_IFLT: return "iflt";
        case INS_IFGE: return "ifge";
        case INS_IFGT: return "ifgt";
        case INS_IFLE: return "ifle";
        case INS_IF_ICMPEQ: return "if_icmpeq";
        case INS_IF_ICMPNE: return "if_icmpne";
        case INS_IF_ICMPLT: return "if_icmplt";
        case INS_IF_ICMPGE: return "if_icmpge";
        case INS_IF_ICMPGT: return "if_icmpgt";
        case INS_IF_ICMPLE: return "if_icmple";
        case INS_IF_ACMPEQ: return "if_acmpeq";
        case INS_IF_ACMPNE: return "if_acmpne";
        case INS_GOTO: return "goto";
        case INS_JSR: return "jsr";
        case INS_RET: return "ret";
        case INS_TABLESWITCH: return "tableswitch";
        case INS_LOOKUPSWITCH: return "lookupswitch";
        case INS_IRETURN: return "ireturn";
        case INS_LRETURN: return "lreturn";
        case INS_FRETURN: return "freturn";
        case INS_DRETURN: return "dreturn";
        case INS_ARETURN: return "areturn";
        case INS_RETURN: return "return";
        case INS_GETSTATIC: return "getstatic";
        case INS_PUTSTATIC: return "putstatic";
        case INS_GETFIELD: return "getfield";
        case INS_PUTFIELD: return "putfield";
        case INS_INVOKEVIRTUAL: return "invokevirtual";
        case INS_INVOKESPECIAL: return "invokespecial";
        case INS_INVOKESTATIC: return "invokestatic";
        case INS_INVOKEINTERFACE: return "invokeinterface";
        case INS_INVOKEDYNAMIC: return "invokedynamic";
        case INS_NEW: return "new";
        case INS_NEWARRAY: return "newarray";
        case INS_ANEWARRAY: return "anewarray";
        case INS_ARRAYLENGTH: return "arraylength";
        case INS_ATHROW: return "athrow";
        case INS_CHECKCAST: return "checkcast";
        case INS_INSTANCEOF: return "instanceof";
        case INS_MONITORENTER: return "monitorenter";
        case INS_MONITOREXIT: return "monitorexit";
        case INS_WIDE: return "wide";
        case INS_MULTIANEWARRAY: return "multianewarray";
        case INS_IFNULL: return "ifnull";
        case INS_IFNONNULL: return "ifnonnull";
        case INS_GOTO_W: return "goto_w";
        case INS_JSR_W: return "jsr_w";
        case INS_BREAKPOINT: return "breakpoint";
        case INS_IMPDEP1: return "impdep1";
        case INS_LEAVEFINALLY: return "leavefinally";
        default: return (string)g_str_unknown;
    }
}

int jvm_opcode_popped(u1 code)
{
    switch(code) {
        case INS_ISTORE:
        case INS_LSTORE:
        case INS_FSTORE:
        case INS_DSTORE:
        case INS_ASTORE:
        case INS_ISTORE_0:
        case INS_ISTORE_1:
        case INS_ISTORE_2:
        case INS_ISTORE_3:
        case INS_LSTORE_0:
        case INS_LSTORE_1:
        case INS_LSTORE_2:
        case INS_LSTORE_3:
        case INS_FSTORE_0:
        case INS_FSTORE_1:
        case INS_FSTORE_2:
        case INS_FSTORE_3:
        case INS_DSTORE_0:
        case INS_DSTORE_1:
        case INS_DSTORE_2:
        case INS_DSTORE_3:
        case INS_ASTORE_0:
        case INS_ASTORE_1:
        case INS_ASTORE_2:
        case INS_ASTORE_3:
        case INS_INEG:
        case INS_LNEG:
        case INS_FNEG:
        case INS_DNEG:
        case INS_I2L:
        case INS_I2F:
        case INS_I2D:
        case INS_L2I:
        case INS_L2F:
        case INS_L2D:
        case INS_F2I:
        case INS_F2L:
        case INS_F2D:
        case INS_D2I:
        case INS_D2L:
        case INS_D2F:
        case INS_I2B:
        case INS_I2C:
        case INS_I2S:
        case INS_IFEQ:
        case INS_IFNE:
        case INS_IFLT:
        case INS_IFGE:
        case INS_IFGT:
        case INS_IFLE:
        case INS_TABLESWITCH:
        case INS_LOOKUPSWITCH:
        case INS_IRETURN:
        case INS_LRETURN:
        case INS_FRETURN:
        case INS_DRETURN:
        case INS_ARETURN:
        case INS_PUTSTATIC:
        case INS_GETFIELD:
        case INS_NEWARRAY:
        case INS_ANEWARRAY:
        case INS_ARRAYLENGTH:
        case INS_ATHROW:
        case INS_CHECKCAST:
        case INS_INSTANCEOF:
        case INS_MONITORENTER:
        case INS_MONITOREXIT:
        case INS_IFNULL:
        case INS_IFNONNULL:
            return 1;
        case INS_IALOAD:
        case INS_LALOAD:
        case INS_FALOAD:
        case INS_DALOAD:
        case INS_AALOAD:
        case INS_BALOAD:
        case INS_CALOAD:
        case INS_SALOAD:
        case INS_IADD:
        case INS_LADD:
        case INS_FADD:
        case INS_DADD:
        case INS_ISUB:
        case INS_LSUB:
        case INS_FSUB:
        case INS_DSUB:
        case INS_IMUL:
        case INS_LMUL:
        case INS_FMUL:
        case INS_DMUL:
        case INS_IDIV:
        case INS_LDIV:
        case INS_FDIV:
        case INS_DDIV:
        case INS_IREM:
        case INS_LREM:
        case INS_FREM:
        case INS_DREM:
        case INS_ISHL:
        case INS_LSHL:
        case INS_ISHR:
        case INS_LSHR:
        case INS_IUSHR:
        case INS_LUSHR:
        case INS_IAND:
        case INS_LAND:
        case INS_IOR:
        case INS_LOR:
        case INS_IXOR:
        case INS_LXOR:
        case INS_LCMP:
        case INS_FCMPL:
        case INS_FCMPG:
        case INS_DCMPL:
        case INS_DCMPG:
        case INS_IF_ICMPEQ:
        case INS_IF_ICMPNE:
        case INS_IF_ICMPLT:
        case INS_IF_ICMPGE:
        case INS_IF_ICMPGT:
        case INS_IF_ICMPLE:
        case INS_IF_ACMPEQ:
        case INS_IF_ACMPNE:
        case INS_PUTFIELD:
            return 2;
        case INS_IASTORE:
        case INS_LASTORE:
        case INS_FASTORE:
        case INS_DASTORE:
        case INS_AASTORE:
        case INS_BASTORE:
        case INS_CASTORE:
        case INS_SASTORE:
            return 3;
        default: return 0;
    }
}

int jvm_opcode_pushed(u1 code)
{
    switch(code) {
        case INS_ACONST_NULL:
        case INS_ICONST_M1:
        case INS_ICONST_0:
        case INS_ICONST_1:
        case INS_ICONST_2:
        case INS_ICONST_3:
        case INS_ICONST_4:
        case INS_ICONST_5:
        case INS_LCONST_0:
        case INS_LCONST_1:
        case INS_FCONST_0:
        case INS_FCONST_1:
        case INS_FCONST_2:
        case INS_DCONST_0:
        case INS_DCONST_1:
        case INS_BIPUSH:
        case INS_SIPUSH:
        case INS_LDC:
        case INS_LDC_W:
        case INS_LDC2_W:
        case INS_ILOAD:
        case INS_LLOAD:
        case INS_FLOAD:
        case INS_DLOAD:
        case INS_ALOAD:
        case INS_ILOAD_0:
        case INS_ILOAD_1:
        case INS_ILOAD_2:
        case INS_ILOAD_3:
        case INS_LLOAD_0:
        case INS_LLOAD_1:
        case INS_LLOAD_2:
        case INS_LLOAD_3:
        case INS_FLOAD_0:
        case INS_FLOAD_1:
        case INS_FLOAD_2:
        case INS_FLOAD_3:
        case INS_DLOAD_0:
        case INS_DLOAD_1:
        case INS_DLOAD_2:
        case INS_DLOAD_3:
        case INS_ALOAD_0:
        case INS_ALOAD_1:
        case INS_ALOAD_2:
        case INS_ALOAD_3:
        case INS_IALOAD:
        case INS_LALOAD:
        case INS_FALOAD:
        case INS_DALOAD:
        case INS_AALOAD:
        case INS_BALOAD:
        case INS_CALOAD:
        case INS_SALOAD:
        case INS_IADD:
        case INS_LADD:
        case INS_FADD:
        case INS_DADD:
        case INS_ISUB:
        case INS_LSUB:
        case INS_FSUB:
        case INS_DSUB:
        case INS_IMUL:
        case INS_LMUL:
        case INS_FMUL:
        case INS_DMUL:
        case INS_IDIV:
        case INS_LDIV:
        case INS_FDIV:
        case INS_DDIV:
        case INS_IREM:
        case INS_LREM:
        case INS_FREM:
        case INS_DREM:
        case INS_INEG:
        case INS_LNEG:
        case INS_FNEG:
        case INS_DNEG:
        case INS_ISHL:
        case INS_LSHL:
        case INS_ISHR:
        case INS_LSHR:
        case INS_IUSHR:
        case INS_LUSHR:
        case INS_IAND:
        case INS_LAND:
        case INS_IOR:
        case INS_LOR:
        case INS_IXOR:
        case INS_LXOR:
        case INS_I2L:
        case INS_I2F:
        case INS_I2D:
        case INS_L2I:
        case INS_L2F:
        case INS_L2D:
        case INS_F2I:
        case INS_F2L:
        case INS_F2D:
        case INS_D2I:
        case INS_D2L:
        case INS_D2F:
        case INS_I2B:
        case INS_I2C:
        case INS_I2S:
        case INS_LCMP:
        case INS_FCMPL:
        case INS_FCMPG:
        case INS_DCMPL:
        case INS_DCMPG:
        case INS_GETSTATIC:
        case INS_GETFIELD:
        case INS_NEW:
        case INS_NEWARRAY:
        case INS_ANEWARRAY:
        case INS_ARRAYLENGTH:
        case INS_ATHROW:
        case INS_CHECKCAST:
        case INS_INSTANCEOF:
        case INS_MULTIANEWARRAY:
            return 1;
        default: return 0;
    }
}

int jvm_opcode_param_len(u1 code)
{
    switch(code) {
        case INS_BIPUSH:
        case INS_LDC:
        case INS_ILOAD:
        case INS_LLOAD:
        case INS_FLOAD:
        case INS_DLOAD:
        case INS_ALOAD:
        case INS_ISTORE:
        case INS_LSTORE:
        case INS_FSTORE:
        case INS_DSTORE:
        case INS_ASTORE:
        case INS_RET:
        case INS_NEWARRAY:
            return 1;
        case INS_SIPUSH:
        case INS_LDC_W:
        case INS_LDC2_W:
        case INS_IINC:
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
        case INS_GOTO:
        case INS_JSR:
        case INS_GETSTATIC:
        case INS_PUTSTATIC:
        case INS_GETFIELD:
        case INS_PUTFIELD:
        case INS_INVOKEVIRTUAL:
        case INS_INVOKESPECIAL:
        case INS_INVOKESTATIC:
        case INS_NEW:
        case INS_ANEWARRAY:
        case INS_CHECKCAST:
        case INS_INSTANCEOF:
        case INS_IFNULL:
        case INS_IFNONNULL:
            return 2;
        case INS_WIDE:
        case INS_MULTIANEWARRAY:
            return 3;
        case INS_INVOKEINTERFACE:
        case INS_INVOKEDYNAMIC:
        case INS_GOTO_W:
        case INS_JSR_W:
            return 4;
        case INS_LOOKUPSWITCH:
            return 8;
        case INS_TABLESWITCH:
            return 16;
        default: return 0;
    }
}

int get_opcode_popped(jclass_file *jc, u1 code)
{
    return jvm_opcode_popped(code);
}

int get_opcode_pushed(jclass_file *jc, u1 code)
{
    return jvm_opcode_pushed(code);
}

int get_opcode_param_length(jclass_file *jc, u1 code)
{
    return jvm_opcode_param_len(code);
}

char* get_opcode_name(jclass_file *jc, u1 code)
{
    return jvm_opcode_name(code);
}
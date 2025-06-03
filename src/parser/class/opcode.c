#include "hashmap_tools.h"
#include "metadata.h"

string jvm_opcode_name(u1 code)
{
    switch(code) {
        case  0x00:
            return  "nop";
        case  0x01:
            return  "aconst_null";
        case  0x02:
            return  "iconst_m1";
        case  0x03:
            return  "iconst_0";
        case  0x04:
            return  "iconst_1";
        case  0x05:
            return  "iconst_2";
        case  0x06:
            return  "iconst_3";
        case  0x07:
            return  "iconst_4";
        case  0x08:
            return  "iconst_5";
        case  0x09:
            return  "lconst_0";
        case  0x0a:
            return  "lconst_1";
        case  0x0b:
            return  "fconst_0";
        case  0x0c:
            return  "fconst_1";
        case  0x0d:
            return  "fconst_2";
        case  0x0e:
            return  "dconst_0";
        case  0x0f:
            return  "dconst_1";
        case  0x10:
            return  "bipush";
        case  0x11:
            return  "sipush";
        case  0x12:
            return  "ldc";
        case  0x13:
            return  "ldc_w";
        case  0x14:
            return  "ldc2_w";
        case  0x15:
            return  "iload";
        case  0x16:
            return  "lload";
        case  0x17:
            return  "fload";
        case  0x18:
            return  "dload";
        case  0x19:
            return  "aload";
        case  0x1a:
            return  "iload_0";
        case  0x1b:
            return  "iload_1";
        case  0x1c:
            return  "iload_2";
        case  0x1d:
            return  "iload_3";
        case  0x1e:
            return  "lload_0";
        case  0x1f:
            return  "lload_1";
        case  0x20:
            return  "lload_2";
        case  0x21:
            return  "lload_3";
        case  0x22:
            return  "fload_0";
        case  0x23:
            return  "fload_1";
        case  0x24:
            return  "fload_2";
        case  0x25:
            return  "fload_3";
        case  0x26:
            return  "dload_0";
        case  0x27:
            return  "dload_1";
        case  0x28:
            return  "dload_2";
        case  0x29:
            return  "dload_3";
        case  0x2a:
            return  "aload_0";
        case  0x2b:
            return  "aload_1";
        case  0x2c:
            return  "aload_2";
        case  0x2d:
            return  "aload_3";
        case  0x2e:
            return  "iaload";
        case  0x2f:
            return  "laload";
        case  0x30:
            return  "faload";
        case  0x31:
            return  "daload";
        case  0x32:
            return  "aaload";
        case  0x33:
            return  "baload";
        case  0x34:
            return  "caload";
        case  0x35:
            return  "saload";
        case  0x36:
            return  "istore";
        case  0x37:
            return  "lstore";
        case  0x38:
            return  "fstore";
        case  0x39:
            return  "dstore";
        case  0x3a:
            return  "astore";
        case  0x3b:
            return  "istore_0";
        case  0x3c:
            return  "istore_1";
        case  0x3d:
            return  "istore_2";
        case  0x3e:
            return  "istore_3";
        case  0x3f:
            return  "lstore_0";
        case  0x40:
            return  "lstore_1";
        case  0x41:
            return  "lstore_2";
        case  0x42:
            return  "lstore_3";
        case  0x43:
            return  "fstore_0";
        case  0x44:
            return  "fstore_1";
        case  0x45:
            return  "fstore_2";
        case  0x46:
            return  "fstore_3";
        case  0x47:
            return  "dstore_0";
        case  0x48:
            return  "dstore_1";
        case  0x49:
            return  "dstore_2";
        case  0x4a:
            return  "dstore_3";
        case  0x4b:
            return  "astore_0";
        case  0x4c:
            return  "astore_1";
        case  0x4d:
            return  "astore_2";
        case  0x4e:
            return  "astore_3";
        case  0x4f:
            return  "iastore";
        case  0x50:
            return  "lastore";
        case  0x51:
            return  "fastore";
        case  0x52:
            return  "dastore";
        case  0x53:
            return  "aastore";
        case  0x54:
            return  "bastore";
        case  0x55:
            return  "castore";
        case  0x56:
            return  "sastore";
        case  0x57:
            return  "pop";
        case  0x58:
            return  "pop2";
        case  0x59:
            return  "dup";
        case  0x5a:
            return  "dup_x1";
        case  0x5b:
            return  "dup_x2";
        case  0x5c:
            return  "dup2";
        case  0x5d:
            return  "dup2_x1";
        case  0x5e:
            return  "dup2_x2";
        case  0x5f:
            return  "swap";
        case  0x60:
            return  "iadd";
        case  0x61:
            return  "ladd";
        case  0x62:
            return  "fadd";
        case  0x63:
            return  "dadd";
        case  0x64:
            return  "isub";
        case  0x65:
            return  "lsub";
        case  0x66:
            return  "fsub";
        case  0x67:
            return  "dsub";
        case  0x68:
            return  "imul";
        case  0x69:
            return  "lmul";
        case  0x6a:
            return  "fmul";
        case  0x6b:
            return  "dmul";
        case  0x6c:
            return  "idiv";
        case  0x6d:
            return  "ldiv";
        case  0x6e:
            return  "fdiv";
        case  0x6f:
            return  "ddiv";
        case  0x70:
            return  "irem";
        case  0x71:
            return  "lrem";
        case  0x72:
            return  "frem";
        case  0x73:
            return  "drem";
        case  0x74:
            return  "ine";
        case  0x75:
            return  "lne";
        case  0x76:
            return  "fne";
        case  0x77:
            return  "dne";
        case  0x78:
            return  "ishl";
        case  0x79:
            return  "lshl";
        case  0x7a:
            return  "ishr";
        case  0x7b:
            return  "lshr";
        case  0x7c:
            return  "iushr";
        case  0x7d:
            return  "lushr";
        case  0x7e:
            return  "iand";
        case  0x7f:
            return  "land";
        case  0x80:
            return  "ior";
        case  0x81:
            return  "lor";
        case  0x82:
            return  "ixor";
        case  0x83:
            return  "lxor";
        case  0x84:
            return  "iinc";
        case  0x85:
            return  "i2l";
        case  0x86:
            return  "i2f";
        case  0x87:
            return  "i2d";
        case  0x88:
            return  "l2i";
        case  0x89:
            return  "l2f";
        case  0x8a:
            return  "l2d";
        case  0x8b:
            return  "f2i";
        case  0x8c:
            return  "f2l";
        case  0x8d:
            return  "f2d";
        case  0x8e:
            return  "d2i";
        case  0x8f:
            return  "d2l";
        case  0x90:
            return  "d2f";
        case  0x91:
            return  "i2b";
        case  0x92:
            return  "i2c";
        case  0x93:
            return  "i2s";
        case  0x94:
            return  "lcmp";
        case  0x95:
            return  "fcmpl";
        case  0x96:
            return  "fcmp";
        case  0x97:
            return  "dcmpl";
        case  0x98:
            return  "dcmp";
        case  0x99:
            return  "ifeq";
        case  0x9a:
            return  "ifne";
        case  0x9b:
            return  "iflt";
        case  0x9c:
            return  "ifge";
        case  0x9d:
            return  "ifgt";
        case  0x9e:
            return  "ifle";
        case  0x9f:
            return  "if_icmpeq";
        case  0xa0:
            return  "if_icmpne";
        case  0xa1:
            return  "if_icmplt";
        case  0xa2:
            return  "if_icmpge";
        case  0xa3:
            return  "if_icmpgt";
        case  0xa4:
            return  "if_icmple";
        case  0xa5:
            return  "if_acmpeq";
        case  0xa6:
            return  "if_acmpne";
        case  0xa7:
            return  "goto";
        case  0xa8:
            return  "jsr";
        case  0xa9:
            return  "ret";
        case  0xaa:
            return  "tableswitch";
        case  0xab:
            return  "lookupswitch";
        case  0xac:
            return  "ireturn";
        case  0xad:
            return  "lreturn";
        case  0xae:
            return  "freturn";
        case  0xaf:
            return  "dreturn";
        case  0xb0:
            return  "areturn";
        case  0xb1:
            return  "return";
        case  0xb2:
            return  "getstatic";
        case  0xb3:
            return  "putstatic";
        case  0xb4:
            return  "getfield";
        case  0xb5:
            return  "putfield";
        case  0xb6:
            return  "invokevirtual";
        case  0xb7:
            return  "invokespecial";
        case  0xb8:
            return  "invokestatic";
        case  0xb9:
            return  "invokeinterface";
        case  0xba:
            return  "invokedynamic";
        case  0xbb:
            return  "new";
        case  0xbc:
            return  "newarray";
        case  0xbd:
            return  "anewarray";
        case  0xbe:
            return  "arraylength";
        case  0xbf:
            return  "athrow";
        case  0xc0:
            return  "checkcast";
        case  0xc1:
            return  "instanceof";
        case  0xc2:
            return  "monitorenter";
        case  0xc3:
            return  "monitorexit";
        case  0xc4:
            return  "wide";
        case  0xc5:
            return  "multianewarray";
        case  0xc6:
            return  "ifnull";
        case  0xc7:
            return  "ifnonnull";
        case  0xc8:
            return  "goto_w";
        case  0xc9:
            return  "jsr_w";
        case  0xca:
            return  "breakpoint";
        case  0xfe:
            return  "impdep1";
        case  0xff:
            return  "leavefinally";
        default:
            return "unknown";
    }
}

int jvm_opcode_popped(u1 code)
{
    switch(code) {
        case  0x00:
            return  0;
        case  0x01:
            return  0;
        case  0x02:
            return  0;
        case  0x03:
            return  0;
        case  0x04:
            return  0;
        case  0x05:
            return  0;
        case  0x06:
            return  0;
        case  0x07:
            return  0;
        case  0x08:
            return  0;
        case  0x09:
            return  0;
        case  0x0a:
            return  0;
        case  0x0b:
            return  0;
        case  0x0c:
            return  0;
        case  0x0d:
            return  0;
        case  0x0e:
            return  0;
        case  0x0f:
            return  0;
        case  0x10:
            return  0;
        case  0x11:
            return  0;
        case  0x12:
            return  0;
        case  0x13:
            return  0;
        case  0x14:
            return  0;
        case  0x15:
            return  0;
        case  0x16:
            return  0;
        case  0x17:
            return  0;
        case  0x18:
            return  0;
        case  0x19:
            return  0;
        case  0x1a:
            return  0;
        case  0x1b:
            return  0;
        case  0x1c:
            return  0;
        case  0x1d:
            return  0;
        case  0x1e:
            return  0;
        case  0x1f:
            return  0;
        case  0x20:
            return  0;
        case  0x21:
            return  0;
        case  0x22:
            return  0;
        case  0x23:
            return  0;
        case  0x24:
            return  0;
        case  0x25:
            return  0;
        case  0x26:
            return  0;
        case  0x27:
            return  0;
        case  0x28:
            return  0;
        case  0x29:
            return  0;
        case  0x2a:
            return  0;
        case  0x2b:
            return  0;
        case  0x2c:
            return  0;
        case  0x2d:
            return  0;
        case  0x2e:
            return  2;
        case  0x2f:
            return  2;
        case  0x30:
            return  2;
        case  0x31:
            return  2;
        case  0x32:
            return  2;
        case  0x33:
            return  2;
        case  0x34:
            return  2;
        case  0x35:
            return  2;
        case  0x36:
            return  1;
        case  0x37:
            return  1;
        case  0x38:
            return  1;
        case  0x39:
            return  1;
        case  0x3a:
            return  1;
        case  0x3b:
            return  1;
        case  0x3c:
            return  1;
        case  0x3d:
            return  1;
        case  0x3e:
            return  1;
        case  0x3f:
            return  1;
        case  0x40:
            return  1;
        case  0x41:
            return  1;
        case  0x42:
            return  1;
        case  0x43:
            return  1;
        case  0x44:
            return  1;
        case  0x45:
            return  1;
        case  0x46:
            return  1;
        case  0x47:
            return  1;
        case  0x48:
            return  1;
        case  0x49:
            return  1;
        case  0x4a:
            return  1;
        case  0x4b:
            return  1;
        case  0x4c:
            return  1;
        case  0x4d:
            return  1;
        case  0x4e:
            return  1;
        case  0x4f:
            return  3;
        case  0x50:
            return  3;
        case  0x51:
            return  3;
        case  0x52:
            return  3;
        case  0x53:
            return  3;
        case  0x54:
            return  3;
        case  0x55:
            return  3;
        case  0x56:
            return  3;
        case  0x57:
            return  0;
        case  0x58:
            return  0;
        case  0x59:
            return  0;
        case  0x5a:
            return  0;
        case  0x5b:
            return  0;
        case  0x5c:
            return  0;
        case  0x5d:
            return  0;
        case  0x5e:
            return  0;
        case  0x5f:
            return  0;
        case  0x60:
            return  2;
        case  0x61:
            return  2;
        case  0x62:
            return  2;
        case  0x63:
            return  2;
        case  0x64:
            return  2;
        case  0x65:
            return  2;
        case  0x66:
            return  2;
        case  0x67:
            return  2;
        case  0x68:
            return  2;
        case  0x69:
            return  2;
        case  0x6a:
            return  2;
        case  0x6b:
            return  2;
        case  0x6c:
            return  2;
        case  0x6d:
            return  2;
        case  0x6e:
            return  2;
        case  0x6f:
            return  2;
        case  0x70:
            return  2;
        case  0x71:
            return  2;
        case  0x72:
            return  2;
        case  0x73:
            return  2;
        case  0x74:
            return  1;
        case  0x75:
            return  1;
        case  0x76:
            return  1;
        case  0x77:
            return  1;
        case  0x78:
            return  2;
        case  0x79:
            return  2;
        case  0x7a:
            return  2;
        case  0x7b:
            return  2;
        case  0x7c:
            return  2;
        case  0x7d:
            return  2;
        case  0x7e:
            return  2;
        case  0x7f:
            return  2;
        case  0x80:
            return  2;
        case  0x81:
            return  2;
        case  0x82:
            return  2;
        case  0x83:
            return  2;
        case  0x84:
            return  0;
        case  0x85:
            return  1;
        case  0x86:
            return  1;
        case  0x87:
            return  1;
        case  0x88:
            return  1;
        case  0x89:
            return  1;
        case  0x8a:
            return  1;
        case  0x8b:
            return  1;
        case  0x8c:
            return  1;
        case  0x8d:
            return  1;
        case  0x8e:
            return  1;
        case  0x8f:
            return  1;
        case  0x90:
            return  1;
        case  0x91:
            return  1;
        case  0x92:
            return  1;
        case  0x93:
            return  1;
        case  0x94:
            return  2;
        case  0x95:
            return  2;
        case  0x96:
            return  2;
        case  0x97:
            return  2;
        case  0x98:
            return  2;
        case  0x99:
            return  1;
        case  0x9a:
            return  1;
        case  0x9b:
            return  1;
        case  0x9c:
            return  1;
        case  0x9d:
            return  1;
        case  0x9e:
            return  1;
        case  0x9f:
            return  2;
        case  0xa0:
            return  2;
        case  0xa1:
            return  2;
        case  0xa2:
            return  2;
        case  0xa3:
            return  2;
        case  0xa4:
            return  2;
        case  0xa5:
            return  2;
        case  0xa6:
            return  2;
        case  0xa7:
            return  0;
        case  0xa8:
            return  0;
        case  0xa9:
            return  0;
        case  0xaa:
            return  1;
        case  0xab:
            return  1;
        case  0xac:
            return  1;
        case  0xad:
            return  1;
        case  0xae:
            return  1;
        case  0xaf:
            return  1;
        case  0xb0:
            return  1;
        case  0xb1:
            return  0;
        case  0xb2:
            return  0;
        case  0xb3:
            return  1;
        case  0xb4:
            return  1;
        case  0xb5:
            return  2;
        case  0xb6:
            return  0;
        case  0xb7:
            return  0;
        case  0xb8:
            return  0;
        case  0xb9:
            return  0;
        case  0xba:
            return  0;
        case  0xbb:
            return  0;
        case  0xbc:
            return  1;
        case  0xbd:
            return  1;
        case  0xbe:
            return  1;
        case  0xbf:
            return  1;
        case  0xc0:
            return  1;
        case  0xc1:
            return  1;
        case  0xc2:
            return  1;
        case  0xc3:
            return  1;
        case  0xc4:
            return  0;
        case  0xc5:
            return  0;
        case  0xc6:
            return  1;
        case  0xc7:
            return  1;
        case  0xc8:
            return  0;
        case  0xc9:
            return  0;
        case  0xca:
            return  0;
        case  0xfe:
            return  0;
        case  0xff:
            return  0;
        default:
            return 0;
    }

}

int jvm_opcode_pushed(u1 code)
{
    switch(code) {
        case  0x00:
            return 0;
        case  0x01:
            return 1;
        case  0x02:
            return 1;
        case  0x03:
            return 1;
        case  0x04:
            return 1;
        case  0x05:
            return 1;
        case  0x06:
            return 1;
        case  0x07:
            return 1;
        case  0x08:
            return 1;
        case  0x09:
            return 1;
        case  0x0a:
            return 1;
        case  0x0b:
            return 1;
        case  0x0c:
            return 1;
        case  0x0d:
            return 1;
        case  0x0e:
            return 1;
        case  0x0f:
            return 1;
        case  0x10:
            return 1;
        case  0x11:
            return 1;
        case  0x12:
            return 1;
        case  0x13:
            return 1;
        case  0x14:
            return 1;
        case  0x15:
            return 1;
        case  0x16:
            return 1;
        case  0x17:
            return 1;
        case  0x18:
            return 1;
        case  0x19:
            return 1;
        case  0x1a:
            return 1;
        case  0x1b:
            return 1;
        case  0x1c:
            return 1;
        case  0x1d:
            return 1;
        case  0x1e:
            return 1;
        case  0x1f:
            return 1;
        case  0x20:
            return 1;
        case  0x21:
            return 1;
        case  0x22:
            return 1;
        case  0x23:
            return 1;
        case  0x24:
            return 1;
        case  0x25:
            return 1;
        case  0x26:
            return 1;
        case  0x27:
            return 1;
        case  0x28:
            return 1;
        case  0x29:
            return 1;
        case  0x2a:
            return 1;
        case  0x2b:
            return 1;
        case  0x2c:
            return 1;
        case  0x2d:
            return 1;
        case  0x2e:
            return 1;
        case  0x2f:
            return 1;
        case  0x30:
            return 1;
        case  0x31:
            return 1;
        case  0x32:
            return 1;
        case  0x33:
            return 1;
        case  0x34:
            return 1;
        case  0x35:
            return 1;
        case  0x36:
            return 0;
        case  0x37:
            return 0;
        case  0x38:
            return 0;
        case  0x39:
            return 0;
        case  0x3a:
            return 0;
        case  0x3b:
            return 0;
        case  0x3c:
            return 0;
        case  0x3d:
            return 0;
        case  0x3e:
            return 0;
        case  0x3f:
            return 0;
        case  0x40:
            return 0;
        case  0x41:
            return 0;
        case  0x42:
            return 0;
        case  0x43:
            return 0;
        case  0x44:
            return 0;
        case  0x45:
            return 0;
        case  0x46:
            return 0;
        case  0x47:
            return 0;
        case  0x48:
            return 0;
        case  0x49:
            return 0;
        case  0x4a:
            return 0;
        case  0x4b:
            return 0;
        case  0x4c:
            return 0;
        case  0x4d:
            return 0;
        case  0x4e:
            return 0;
        case  0x4f:
            return 0;
        case  0x50:
            return 0;
        case  0x51:
            return 0;
        case  0x52:
            return 0;
        case  0x53:
            return 0;
        case  0x54:
            return 0;
        case  0x55:
            return 0;
        case  0x56:
            return 0;
        case  0x57:
            return 0;
        case  0x58:
            return 0;
        case  0x59:
            return 0;
        case  0x5a:
            return 0;
        case  0x5b:
            return 0;
        case  0x5c:
            return 0;
        case  0x5d:
            return 0;
        case  0x5e:
            return 0;
        case  0x5f:
            return 0;
        case  0x60:
            return 1;
        case  0x61:
            return 1;
        case  0x62:
            return 1;
        case  0x63:
            return 1;
        case  0x64:
            return 1;
        case  0x65:
            return 1;
        case  0x66:
            return 1;
        case  0x67:
            return 1;
        case  0x68:
            return 1;
        case  0x69:
            return 1;
        case  0x6a:
            return 1;
        case  0x6b:
            return 1;
        case  0x6c:
            return 1;
        case  0x6d:
            return 1;
        case  0x6e:
            return 1;
        case  0x6f:
            return 1;
        case  0x70:
            return 1;
        case  0x71:
            return 1;
        case  0x72:
            return 1;
        case  0x73:
            return 1;
        case  0x74:
            return 1;
        case  0x75:
            return 1;
        case  0x76:
            return 1;
        case  0x77:
            return 1;
        case  0x78:
            return 1;
        case  0x79:
            return 1;
        case  0x7a:
            return 1;
        case  0x7b:
            return 1;
        case  0x7c:
            return 1;
        case  0x7d:
            return 1;
        case  0x7e:
            return 1;
        case  0x7f:
            return 1;
        case  0x80:
            return 1;
        case  0x81:
            return 1;
        case  0x82:
            return 1;
        case  0x83:
            return 1;
        case  0x84:
            return 0;
        case  0x85:
            return 1;
        case  0x86:
            return 1;
        case  0x87:
            return 1;
        case  0x88:
            return 1;
        case  0x89:
            return 1;
        case  0x8a:
            return 1;
        case  0x8b:
            return 1;
        case  0x8c:
            return 1;
        case  0x8d:
            return 1;
        case  0x8e:
            return 1;
        case  0x8f:
            return 1;
        case  0x90:
            return 1;
        case  0x91:
            return 1;
        case  0x92:
            return 1;
        case  0x93:
            return 1;
        case  0x94:
            return 1;
        case  0x95:
            return 1;
        case  0x96:
            return 1;
        case  0x97:
            return 1;
        case  0x98:
            return 1;
        case  0x99:
            return 0;
        case  0x9a:
            return 0;
        case  0x9b:
            return 0;
        case  0x9c:
            return 0;
        case  0x9d:
            return 0;
        case  0x9e:
            return 0;
        case  0x9f:
            return 0;
        case  0xa0:
            return 0;
        case  0xa1:
            return 0;
        case  0xa2:
            return 0;
        case  0xa3:
            return 0;
        case  0xa4:
            return 0;
        case  0xa5:
            return 0;
        case  0xa6:
            return 0;
        case  0xa7:
            return 0;
        case  0xa8:
            return 0;
        case  0xa9:
            return 0;
        case  0xaa:
            return 0;
        case  0xab:
            return 0;
        case  0xac:
            return 0;
        case  0xad:
            return 0;
        case  0xae:
            return 0;
        case  0xaf:
            return 0;
        case  0xb0:
            return 0;
        case  0xb1:
            return 0;
        case  0xb2:
            return 1;
        case  0xb3:
            return 0;
        case  0xb4:
            return 1;
        case  0xb5:
            return 0;
        case  0xb6:
            return 0;
        case  0xb7:
            return 0;
        case  0xb8:
            return 0;
        case  0xb9:
            return 0;
        case  0xba:
            return 0;
        case  0xbb:
            return 1;
        case  0xbc:
            return 1;
        case  0xbd:
            return 1;
        case  0xbe:
            return 1;
        case  0xbf:
            return 1;
        case  0xc0:
            return 1;
        case  0xc1:
            return 1;
        case  0xc2:
            return 0;
        case  0xc3:
            return 0;
        case  0xc4:
            return 0;
        case  0xc5:
            return 1;
        case  0xc6:
            return 0;
        case  0xc7:
            return 0;
        case  0xc8:
            return 0;
        case  0xc9:
            return 0;
        case  0xca:
            return 0;
        case  0xfe:
            return 0;
        case  0xff:
            return 0;
        default:
            return 0;
    }
}

int jvm_opcode_param_len(u1 code)
{
    switch(code) {
        case  0x00:
        case  0x01:
        case  0x02:
        case  0x03:
        case  0x04:
        case  0x05:
        case  0x06:
        case  0x07:
        case  0x08:
        case  0x09:
        case  0x0a:
        case  0x0b:
        case  0x0c:
        case  0x0d:
        case  0x0e:
        case  0x0f:
            return  0;
        case  0x10:
            return  1;
        case  0x11:
            return  2;
        case  0x12:
            return  1;
        case  0x13:
        case  0x14:
            return  2;
        case  0x15:
        case  0x16:
        case  0x17:
        case  0x18:
        case  0x19:
            return  1;
        case  0x1a:
        case  0x1b:
        case  0x1c:
        case  0x1d:
        case  0x1e:
        case  0x1f:
        case  0x20:
        case  0x21:
        case  0x22:
        case  0x23:
        case  0x24:
        case  0x25:
        case  0x26:
        case  0x27:
        case  0x28:
        case  0x29:
        case  0x2a:
        case  0x2b:
        case  0x2c:
        case  0x2d:
        case  0x2e:
        case  0x2f:
        case  0x30:
        case  0x31:
        case  0x32:
        case  0x33:
        case  0x34:
        case  0x35:
            return  0;
        case  0x36:
        case  0x37:
        case  0x38:
        case  0x39:
        case  0x3a:
            return  1;
        case  0x3b:
        case  0x3c:
        case  0x3d:
        case  0x3e:
        case  0x3f:
        case  0x40:
        case  0x41:
        case  0x42:
        case  0x43:
        case  0x44:
        case  0x45:
        case  0x46:
        case  0x47:
        case  0x48:
        case  0x49:
        case  0x4a:
        case  0x4b:
        case  0x4c:
        case  0x4d:
        case  0x4e:
        case  0x4f:
        case  0x50:
        case  0x51:
        case  0x52:
        case  0x53:
        case  0x54:
        case  0x55:
        case  0x56:
        case  0x57:
        case  0x58:
        case  0x59:
        case  0x5a:
        case  0x5b:
        case  0x5c:
        case  0x5d:
        case  0x5e:
        case  0x5f:
        case  0x60:
        case  0x61:
        case  0x62:
        case  0x63:
        case  0x64:
        case  0x65:
        case  0x66:
        case  0x67:
        case  0x68:
        case  0x69:
        case  0x6a:
        case  0x6b:
        case  0x6c:
        case  0x6d:
        case  0x6e:
        case  0x6f:
        case  0x70:
        case  0x71:
        case  0x72:
        case  0x73:
        case  0x74:
        case  0x75:
        case  0x76:
        case  0x77:
        case  0x78:
        case  0x79:
        case  0x7a:
        case  0x7b:
        case  0x7c:
        case  0x7d:
        case  0x7e:
        case  0x7f:
        case  0x80:
        case  0x81:
        case  0x82:
        case  0x83:
            return  0;
        case  0x84:
            return  2;
        case  0x85:
        case  0x86:
        case  0x87:
        case  0x88:
        case  0x89:
        case  0x8a:
        case  0x8b:
        case  0x8c:
        case  0x8d:
        case  0x8e:
        case  0x8f:
        case  0x90:
        case  0x91:
        case  0x92:
        case  0x93:
        case  0x94:
        case  0x95:
        case  0x96:
        case  0x97:
        case  0x98:
            return  0;
        case  0x99:
        case  0x9a:
        case  0x9b:
        case  0x9c:
        case  0x9d:
        case  0x9e:
        case  0x9f:
        case  0xa0:
        case  0xa1:
        case  0xa2:
        case  0xa3:
        case  0xa4:
        case  0xa5:
        case  0xa6:
        case  0xa7:
        case  0xa8:
            return  2;
        case  0xa9:
            return  1;
        case  0xaa:
            return  16;
        case  0xab:
            return  8;
        case  0xac:
        case  0xad:
        case  0xae:
        case  0xaf:
        case  0xb0:
        case  0xb1:
            return  0;
        case  0xb2:
        case  0xb3:
        case  0xb4:
        case  0xb5:
        case  0xb6:
        case  0xb7:
        case  0xb8:
            return  2;
        case  0xb9:
        case  0xba:
            return  4;
        case  0xbb:
            return  2;
        case  0xbc:
            return  1;
        case  0xbd:
            return  2;
        case  0xbe:
        case  0xbf:
            return  0;
        case  0xc0:
        case  0xc1:
            return  2;
        case  0xc2:
        case  0xc3:
            return  0;
        case  0xc4:
        case  0xc5:
            return  3;
        case  0xc6:
        case  0xc7:
            return  2;
        case  0xc8:
        case  0xc9:
            return  4;
        case  0xca:
        case  0xfe:
        case  0xff:
            return  0;
        default:
            return 0;
    }
}

int get_opcode_popped(jclass_file *jc, u1 code)
{
//    u1_to_object *e = find_u1_to_object_entry(jc->class_opcode, code);
//    jd_class_opcode *opcode = e->value;
//    return opcode->popped;
    return jvm_opcode_popped(code);
}

int get_opcode_pushed(jclass_file *jc, u1 code)
{
//    u1_to_object *e = find_u1_to_object_entry(jc->class_opcode, code);
//    jd_class_opcode *opcode = e->value;
//    return opcode->pushed;
    return jvm_opcode_pushed(code);
}

int get_opcode_param_length(jclass_file *jc, u1 code)
{
//    u1_to_object *e = find_u1_to_object_entry(jc->class_opcode, code);
//    jd_class_opcode *opcode = e->value;
//    return opcode->parameter_length;
    return jvm_opcode_param_len(code);
}

char* get_opcode_name(jclass_file *jc, u1 code)
{
//    u1_to_object *e = find_u1_to_object_entry(jc->class_opcode, code);
//    jd_class_opcode *opcode = e->value;
//    return opcode->name;
    return jvm_opcode_name(code);
}

#if false
static void inline add_opcode_to_map(hashmap *map,
                              u1 code,
                              int parameter_length,
                              char *name,
                              int is_jump,
                              int popped,
                              int pushed)
{
    jd_class_opcode *code_item = make_obj(jd_class_opcode);
    code_item->name = str_dup(name);
    code_item->code = code;
    code_item->parameter_length = parameter_length;
    code_item->is_jump = is_jump;
    code_item->popped = popped;
    code_item->pushed = pushed;
    hashmap_set_u1_to_object(map, code_item->code, code_item);
}

jd_jvm_opcode jvm_opcode_nop = { 0x00, 0,  0,  0,  0,  "nop"};
jd_jvm_opcode jvm_opcode_aconst_null = { 0x01, 1,  0,  0,  0,  "aconst_null"};
jd_jvm_opcode jvm_opcode_iconst_m1 = { 0x02, 1,  0,  0,  0,  "iconst_m1"};
jd_jvm_opcode jvm_opcode_iconst_0 = { 0x03, 1,  0,  0,  0,  "iconst_0"};
jd_jvm_opcode jvm_opcode_iconst_1 = { 0x04, 1,  0,  0,  0,  "iconst_1"};
jd_jvm_opcode jvm_opcode_iconst_2 = { 0x05, 1,  0,  0,  0,  "iconst_2"};
jd_jvm_opcode jvm_opcode_iconst_3 = { 0x06, 1,  0,  0,  0,  "iconst_3"};
jd_jvm_opcode jvm_opcode_iconst_4 = { 0x07, 1,  0,  0,  0,  "iconst_4"};
jd_jvm_opcode jvm_opcode_iconst_5 = { 0x08, 1,  0,  0,  0,  "iconst_5"};
jd_jvm_opcode jvm_opcode_lconst_0 = { 0x09, 1,  0,  0,  0,  "lconst_0"};
jd_jvm_opcode jvm_opcode_lconst_1 = { 0x0a, 1,  0,  0,  0,  "lconst_1"};
jd_jvm_opcode jvm_opcode_fconst_0 = { 0x0b, 1,  0,  0,  0,  "fconst_0"};
jd_jvm_opcode jvm_opcode_fconst_1 = { 0x0c, 1,  0,  0,  0,  "fconst_1"};
jd_jvm_opcode jvm_opcode_fconst_2 = { 0x0d, 1,  0,  0,  0,  "fconst_2"};
jd_jvm_opcode jvm_opcode_dconst_0 = { 0x0e, 1,  0,  0,  0,  "dconst_0"};
jd_jvm_opcode jvm_opcode_dconst_1 = { 0x0f, 1,  0,  0,  0,  "dconst_1"};
jd_jvm_opcode jvm_opcode_bipush = { 0x10, 1,  0,  1,  0,  "bipush"};
jd_jvm_opcode jvm_opcode_sipush = { 0x11, 1,  0,  2,  0,  "sipush"};
jd_jvm_opcode jvm_opcode_ldc = { 0x12, 1,  0,  1,  0,  "ldc"};
jd_jvm_opcode jvm_opcode_ldc_w = { 0x13, 1,  0,  2,  0,  "ldc_w"};
jd_jvm_opcode jvm_opcode_ldc2_w = { 0x14, 1,  0,  2,  0,  "ldc2_w"};
jd_jvm_opcode jvm_opcode_iload = { 0x15, 1,  0,  1,  0,  "iload"};
jd_jvm_opcode jvm_opcode_lload = { 0x16, 1,  0,  1,  0,  "lload"};
jd_jvm_opcode jvm_opcode_fload = { 0x17, 1,  0,  1,  0,  "fload"};
jd_jvm_opcode jvm_opcode_dload = { 0x18, 1,  0,  1,  0,  "dload"};
jd_jvm_opcode jvm_opcode_aload = { 0x19, 1,  0,  1,  0,  "aload"};
jd_jvm_opcode jvm_opcode_iload_0 = { 0x1a, 1,  0,  0,  0,  "iload_0"};
jd_jvm_opcode jvm_opcode_iload_1 = { 0x1b, 1,  0,  0,  0,  "iload_1"};
jd_jvm_opcode jvm_opcode_iload_2 = { 0x1c, 1,  0,  0,  0,  "iload_2"};
jd_jvm_opcode jvm_opcode_iload_3 = { 0x1d, 1,  0,  0,  0,  "iload_3"};
jd_jvm_opcode jvm_opcode_lload_0 = { 0x1e, 1,  0,  0,  0,  "lload_0"};
jd_jvm_opcode jvm_opcode_lload_1 = { 0x1f, 1,  0,  0,  0,  "lload_1"};
jd_jvm_opcode jvm_opcode_lload_2 = { 0x20, 1,  0,  0,  0,  "lload_2"};
jd_jvm_opcode jvm_opcode_lload_3 = { 0x21, 1,  0,  0,  0,  "lload_3"};
jd_jvm_opcode jvm_opcode_fload_0 = { 0x22, 1,  0,  0,  0,  "fload_0"};
jd_jvm_opcode jvm_opcode_fload_1 = { 0x23, 1,  0,  0,  0,  "fload_1"};
jd_jvm_opcode jvm_opcode_fload_2 = { 0x24, 1,  0,  0,  0,  "fload_2"};
jd_jvm_opcode jvm_opcode_fload_3 = { 0x25, 1,  0,  0,  0,  "fload_3"};
jd_jvm_opcode jvm_opcode_dload_0 = { 0x26, 1,  0,  0,  0,  "dload_0"};
jd_jvm_opcode jvm_opcode_dload_1 = { 0x27, 1,  0,  0,  0,  "dload_1"};
jd_jvm_opcode jvm_opcode_dload_2 = { 0x28, 1,  0,  0,  0,  "dload_2"};
jd_jvm_opcode jvm_opcode_dload_3 = { 0x29, 1,  0,  0,  0,  "dload_3"};
jd_jvm_opcode jvm_opcode_aload_0 = { 0x2a, 1,  0,  0,  0,  "aload_0"};
jd_jvm_opcode jvm_opcode_aload_1 = { 0x2b, 1,  0,  0,  0,  "aload_1"};
jd_jvm_opcode jvm_opcode_aload_2 = { 0x2c, 1,  0,  0,  0,  "aload_2"};
jd_jvm_opcode jvm_opcode_aload_3 = { 0x2d, 1,  0,  0,  0,  "aload_3"};
jd_jvm_opcode jvm_opcode_iaload = { 0x2e, 1,  2,  0,  0,  "iaload"};
jd_jvm_opcode jvm_opcode_laload = { 0x2f, 1,  2,  0,  0,  "laload"};
jd_jvm_opcode jvm_opcode_faload = { 0x30, 1,  2,  0,  0,  "faload"};
jd_jvm_opcode jvm_opcode_daload = { 0x31, 1,  2,  0,  0,  "daload"};
jd_jvm_opcode jvm_opcode_aaload = { 0x32, 1,  2,  0,  0,  "aaload"};
jd_jvm_opcode jvm_opcode_baload = { 0x33, 1,  2,  0,  0,  "baload"};
jd_jvm_opcode jvm_opcode_caload = { 0x34, 1,  2,  0,  0,  "caload"};
jd_jvm_opcode jvm_opcode_saload = { 0x35, 1,  2,  0,  0,  "saload"};
jd_jvm_opcode jvm_opcode_istore = { 0x36, 0,  1,  1,  0,  "istore"};
jd_jvm_opcode jvm_opcode_lstore = { 0x37, 0,  1,  1,  0,  "lstore"};
jd_jvm_opcode jvm_opcode_fstore = { 0x38, 0,  1,  1,  0,  "fstore"};
jd_jvm_opcode jvm_opcode_dstore = { 0x39, 0,  1,  1,  0,  "dstore"};
jd_jvm_opcode jvm_opcode_astore = { 0x3a, 0,  1,  1,  0,  "astore"};
jd_jvm_opcode jvm_opcode_istore_0 = { 0x3b, 0,  1,  0,  0,  "istore_0"};
jd_jvm_opcode jvm_opcode_istore_1 = { 0x3c, 0,  1,  0,  0,  "istore_1"};
jd_jvm_opcode jvm_opcode_istore_2 = { 0x3d, 0,  1,  0,  0,  "istore_2"};
jd_jvm_opcode jvm_opcode_istore_3 = { 0x3e, 0,  1,  0,  0,  "istore_3"};
jd_jvm_opcode jvm_opcode_lstore_0 = { 0x3f, 0,  1,  0,  0,  "lstore_0"};
jd_jvm_opcode jvm_opcode_lstore_1 = { 0x40, 0,  1,  0,  0,  "lstore_1"};
jd_jvm_opcode jvm_opcode_lstore_2 = { 0x41, 0,  1,  0,  0,  "lstore_2"};
jd_jvm_opcode jvm_opcode_lstore_3 = { 0x42, 0,  1,  0,  0,  "lstore_3"};
jd_jvm_opcode jvm_opcode_fstore_0 = { 0x43, 0,  1,  0,  0,  "fstore_0"};
jd_jvm_opcode jvm_opcode_fstore_1 = { 0x44, 0,  1,  0,  0,  "fstore_1"};
jd_jvm_opcode jvm_opcode_fstore_2 = { 0x45, 0,  1,  0,  0,  "fstore_2"};
jd_jvm_opcode jvm_opcode_fstore_3 = { 0x46, 0,  1,  0,  0,  "fstore_3"};
jd_jvm_opcode jvm_opcode_dstore_0 = { 0x47, 0,  1,  0,  0,  "dstore_0"};
jd_jvm_opcode jvm_opcode_dstore_1 = { 0x48, 0,  1,  0,  0,  "dstore_1"};
jd_jvm_opcode jvm_opcode_dstore_2 = { 0x49, 0,  1,  0,  0,  "dstore_2"};
jd_jvm_opcode jvm_opcode_dstore_3 = { 0x4a, 0,  1,  0,  0,  "dstore_3"};
jd_jvm_opcode jvm_opcode_astore_0 = { 0x4b, 0,  1,  0,  0,  "astore_0"};
jd_jvm_opcode jvm_opcode_astore_1 = { 0x4c, 0,  1,  0,  0,  "astore_1"};
jd_jvm_opcode jvm_opcode_astore_2 = { 0x4d, 0,  1,  0,  0,  "astore_2"};
jd_jvm_opcode jvm_opcode_astore_3 = { 0x4e, 0,  1,  0,  0,  "astore_3"};
jd_jvm_opcode jvm_opcode_iastore = { 0x4f, 0,  3,  0,  0,  "iastore"};
jd_jvm_opcode jvm_opcode_lastore = { 0x50, 0,  3,  0,  0,  "lastore"};
jd_jvm_opcode jvm_opcode_fastore = { 0x51, 0,  3,  0,  0,  "fastore"};
jd_jvm_opcode jvm_opcode_dastore = { 0x52, 0,  3,  0,  0,  "dastore"};
jd_jvm_opcode jvm_opcode_aastore = { 0x53, 0,  3,  0,  0,  "aastore"};
jd_jvm_opcode jvm_opcode_bastore = { 0x54, 0,  3,  0,  0,  "bastore"};
jd_jvm_opcode jvm_opcode_castore = { 0x55, 0,  3,  0,  0,  "castore"};
jd_jvm_opcode jvm_opcode_sastore = { 0x56, 0,  3,  0,  0,  "sastore"};
jd_jvm_opcode jvm_opcode_pop = { 0x57, 0,  0,  0,  0,  "pop"};
jd_jvm_opcode jvm_opcode_pop2 = { 0x58, 0,  0,  0,  0,  "pop2"};
jd_jvm_opcode jvm_opcode_dup = { 0x59, 0,  0,  0,  0,  "dup"};
jd_jvm_opcode jvm_opcode_dup_x1 = { 0x5a, 0,  0,  0,  0,  "dup_x1"};
jd_jvm_opcode jvm_opcode_dup_x2 = { 0x5b, 0,  0,  0,  0,  "dup_x2"};
jd_jvm_opcode jvm_opcode_dup2 = { 0x5c, 0,  0,  0,  0,  "dup2"};
jd_jvm_opcode jvm_opcode_dup2_x1 = { 0x5d, 0,  0,  0,  0,  "dup2_x1"};
jd_jvm_opcode jvm_opcode_dup2_x2 = { 0x5e, 0,  0,  0,  0,  "dup2_x2"};
jd_jvm_opcode jvm_opcode_swap = { 0x5f, 0,  0,  0,  0,  "swap"};
jd_jvm_opcode jvm_opcode_iadd = { 0x60, 1,  2,  0,  0,  "iadd"};
jd_jvm_opcode jvm_opcode_ladd = { 0x61, 1,  2,  0,  0,  "ladd"};
jd_jvm_opcode jvm_opcode_fadd = { 0x62, 1,  2,  0,  0,  "fadd"};
jd_jvm_opcode jvm_opcode_dadd = { 0x63, 1,  2,  0,  0,  "dadd"};
jd_jvm_opcode jvm_opcode_isub = { 0x64, 1,  2,  0,  0,  "isub"};
jd_jvm_opcode jvm_opcode_lsub = { 0x65, 1,  2,  0,  0,  "lsub"};
jd_jvm_opcode jvm_opcode_fsub = { 0x66, 1,  2,  0,  0,  "fsub"};
jd_jvm_opcode jvm_opcode_dsub = { 0x67, 1,  2,  0,  0,  "dsub"};
jd_jvm_opcode jvm_opcode_imul = { 0x68, 1,  2,  0,  0,  "imul"};
jd_jvm_opcode jvm_opcode_lmul = { 0x69, 1,  2,  0,  0,  "lmul"};
jd_jvm_opcode jvm_opcode_fmul = { 0x6a, 1,  2,  0,  0,  "fmul"};
jd_jvm_opcode jvm_opcode_dmul = { 0x6b, 1,  2,  0,  0,  "dmul"};
jd_jvm_opcode jvm_opcode_idiv = { 0x6c, 1,  2,  0,  0,  "idiv"};
jd_jvm_opcode jvm_opcode_ldiv = { 0x6d, 1,  2,  0,  0,  "ldiv"};
jd_jvm_opcode jvm_opcode_fdiv = { 0x6e, 1,  2,  0,  0,  "fdiv"};
jd_jvm_opcode jvm_opcode_ddiv = { 0x6f, 1,  2,  0,  0,  "ddiv"};
jd_jvm_opcode jvm_opcode_irem = { 0x70, 1,  2,  0,  0,  "irem"};
jd_jvm_opcode jvm_opcode_lrem = { 0x71, 1,  2,  0,  0,  "lrem"};
jd_jvm_opcode jvm_opcode_frem = { 0x72, 1,  2,  0,  0,  "frem"};
jd_jvm_opcode jvm_opcode_drem = { 0x73, 1,  2,  0,  0,  "drem"};
jd_jvm_opcode jvm_opcode_ine = { 0x74, 1,  1,  0,  0,  "ine"};
jd_jvm_opcode jvm_opcode_lne = { 0x75, 1,  1,  0,  0,  "lne"};
jd_jvm_opcode jvm_opcode_fne = { 0x76, 1,  1,  0,  0,  "fne"};
jd_jvm_opcode jvm_opcode_dne = { 0x77, 1,  1,  0,  0,  "dne"};
jd_jvm_opcode jvm_opcode_ishl = { 0x78, 1,  2,  0,  0,  "ishl"};
jd_jvm_opcode jvm_opcode_lshl = { 0x79, 1,  2,  0,  0,  "lshl"};
jd_jvm_opcode jvm_opcode_ishr = { 0x7a, 1,  2,  0,  0,  "ishr"};
jd_jvm_opcode jvm_opcode_lshr = { 0x7b, 1,  2,  0,  0,  "lshr"};
jd_jvm_opcode jvm_opcode_iushr = { 0x7c, 1,  2,  0,  0,  "iushr"};
jd_jvm_opcode jvm_opcode_lushr = { 0x7d, 1,  2,  0,  0,  "lushr"};
jd_jvm_opcode jvm_opcode_iand = { 0x7e, 1,  2,  0,  0,  "iand"};
jd_jvm_opcode jvm_opcode_land = { 0x7f, 1,  2,  0,  0,  "land"};
jd_jvm_opcode jvm_opcode_ior = { 0x80, 1,  2,  0,  0,  "ior"};
jd_jvm_opcode jvm_opcode_lor = { 0x81, 1,  2,  0,  0,  "lor"};
jd_jvm_opcode jvm_opcode_ixor = { 0x82, 1,  2,  0,  0,  "ixor"};
jd_jvm_opcode jvm_opcode_lxor = { 0x83, 1,  2,  0,  0,  "lxor"};
jd_jvm_opcode jvm_opcode_iinc = { 0x84, 0,  0,  2,  0,  "iinc"};
jd_jvm_opcode jvm_opcode_i2l = { 0x85, 1,  1,  0,  0,  "i2l"};
jd_jvm_opcode jvm_opcode_i2f = { 0x86, 1,  1,  0,  0,  "i2f"};
jd_jvm_opcode jvm_opcode_i2d = { 0x87, 1,  1,  0,  0,  "i2d"};
jd_jvm_opcode jvm_opcode_l2i = { 0x88, 1,  1,  0,  0,  "l2i"};
jd_jvm_opcode jvm_opcode_l2f = { 0x89, 1,  1,  0,  0,  "l2f"};
jd_jvm_opcode jvm_opcode_l2d = { 0x8a, 1,  1,  0,  0,  "l2d"};
jd_jvm_opcode jvm_opcode_f2i = { 0x8b, 1,  1,  0,  0,  "f2i"};
jd_jvm_opcode jvm_opcode_f2l = { 0x8c, 1,  1,  0,  0,  "f2l"};
jd_jvm_opcode jvm_opcode_f2d = { 0x8d, 1,  1,  0,  0,  "f2d"};
jd_jvm_opcode jvm_opcode_d2i = { 0x8e, 1,  1,  0,  0,  "d2i"};
jd_jvm_opcode jvm_opcode_d2l = { 0x8f, 1,  1,  0,  0,  "d2l"};
jd_jvm_opcode jvm_opcode_d2f = { 0x90, 1,  1,  0,  0,  "d2f"};
jd_jvm_opcode jvm_opcode_i2b = { 0x91, 1,  1,  0,  0,  "i2b"};
jd_jvm_opcode jvm_opcode_i2c = { 0x92, 1,  1,  0,  0,  "i2c"};
jd_jvm_opcode jvm_opcode_i2s = { 0x93, 1,  1,  0,  0,  "i2s"};
jd_jvm_opcode jvm_opcode_lcmp = { 0x94, 1,  2,  0,  0,  "lcmp"};
jd_jvm_opcode jvm_opcode_fcmpl = { 0x95, 1,  2,  0,  0,  "fcmpl"};
jd_jvm_opcode jvm_opcode_fcmp = { 0x96, 1,  2,  0,  0,  "fcmp"};
jd_jvm_opcode jvm_opcode_dcmpl = { 0x97, 1,  2,  0,  0,  "dcmpl"};
jd_jvm_opcode jvm_opcode_dcmp = { 0x98, 1,  2,  0,  0,  "dcmp"};
jd_jvm_opcode jvm_opcode_ifeq = { 0x99, 0,  1,  2,  1,  "ifeq"};
jd_jvm_opcode jvm_opcode_ifne = { 0x9a, 0,  1,  2,  1,  "ifne"};
jd_jvm_opcode jvm_opcode_iflt = { 0x9b, 0,  1,  2,  1,  "iflt"};
jd_jvm_opcode jvm_opcode_ifge = { 0x9c, 0,  1,  2,  1,  "ifge"};
jd_jvm_opcode jvm_opcode_ifgt = { 0x9d, 0,  1,  2,  1,  "ifgt"};
jd_jvm_opcode jvm_opcode_ifle = { 0x9e, 0,  1,  2,  1,  "ifle"};
jd_jvm_opcode jvm_opcode_if_icmpeq = { 0x9f, 0,  2,  2,  1,  "if_icmpeq"};
jd_jvm_opcode jvm_opcode_if_icmpne = { 0xa0, 0,  2,  2,  1,  "if_icmpne"};
jd_jvm_opcode jvm_opcode_if_icmplt = { 0xa1, 0,  2,  2,  1,  "if_icmplt"};
jd_jvm_opcode jvm_opcode_if_icmpge = { 0xa2, 0,  2,  2,  1,  "if_icmpge"};
jd_jvm_opcode jvm_opcode_if_icmpgt = { 0xa3, 0,  2,  2,  1,  "if_icmpgt"};
jd_jvm_opcode jvm_opcode_if_icmple = { 0xa4, 0,  2,  2,  1,  "if_icmple"};
jd_jvm_opcode jvm_opcode_if_acmpeq = { 0xa5, 0,  2,  2,  1,  "if_acmpeq"};
jd_jvm_opcode jvm_opcode_if_acmpne = { 0xa6, 0,  2,  2,  1,  "if_acmpne"};
jd_jvm_opcode jvm_opcode_goto = { 0xa7, 0,  0,  2,  1,  "goto"};
jd_jvm_opcode jvm_opcode_jsr = { 0xa8, 0,  0,  2,  1,  "jsr"};
jd_jvm_opcode jvm_opcode_ret = { 0xa9, 0,  0,  1,  1,  "ret"};
jd_jvm_opcode jvm_opcode_tableswitch = { 0xaa, 0,  1,  16,  1,  "tableswitch"};
jd_jvm_opcode jvm_opcode_lookupswitch = { 0xab, 0,  1,  8,  1,  "lookupswitch"};
jd_jvm_opcode jvm_opcode_ireturn = { 0xac, 0,  1,  0,  0,  "ireturn"};
jd_jvm_opcode jvm_opcode_lreturn = { 0xad, 0,  1,  0,  0,  "lreturn"};
jd_jvm_opcode jvm_opcode_freturn = { 0xae, 0,  1,  0,  0,  "freturn"};
jd_jvm_opcode jvm_opcode_dreturn = { 0xaf, 0,  1,  0,  0,  "dreturn"};
jd_jvm_opcode jvm_opcode_areturn = { 0xb0, 0,  1,  0,  0,  "areturn"};
jd_jvm_opcode jvm_opcode_return = { 0xb1, 0,  0,  0,  0,  "return"};
jd_jvm_opcode jvm_opcode_getstatic = { 0xb2, 1,  0,  2,  0,  "getstatic"};
jd_jvm_opcode jvm_opcode_putstatic = { 0xb3, 0,  1,  2,  0,  "putstatic"};
jd_jvm_opcode jvm_opcode_getfield = { 0xb4, 1,  1,  2,  0,  "getfield"};
jd_jvm_opcode jvm_opcode_putfield = { 0xb5, 0,  2,  2,  0,  "putfield"};
jd_jvm_opcode jvm_opcode_invokevirtual = { 0xb6, 0,  0,  2,  0,  "invokevirtual"};
jd_jvm_opcode jvm_opcode_invokespecial = { 0xb7, 0,  0,  2,  0,  "invokespecial"};
jd_jvm_opcode jvm_opcode_invokestatic = { 0xb8, 0,  0,  2,  0,  "invokestatic"};
jd_jvm_opcode jvm_opcode_invokeinterface = { 0xb9, 0,  0,  4,  0,  "invokeinterface"};
jd_jvm_opcode jvm_opcode_invokedynamic = { 0xba, 0,  0,  4,  0,  "invokedynamic"};
jd_jvm_opcode jvm_opcode_new = { 0xbb, 1,  0,  2,  0,  "new"};
jd_jvm_opcode jvm_opcode_newarray = { 0xbc, 1,  1,  1,  0,  "newarray"};
jd_jvm_opcode jvm_opcode_anewarray = { 0xbd, 1,  1,  2,  0,  "anewarray"};
jd_jvm_opcode jvm_opcode_arraylength = { 0xbe, 1,  1,  0,  0,  "arraylength"};
jd_jvm_opcode jvm_opcode_athrow = { 0xbf, 1,  1,  0,  0,  "athrow"};
jd_jvm_opcode jvm_opcode_checkcast = { 0xc0, 1,  1,  2,  0,  "checkcast"};
jd_jvm_opcode jvm_opcode_instanceof = { 0xc1, 1,  1,  2,  0,  "instanceof"};
jd_jvm_opcode jvm_opcode_monitorenter = { 0xc2, 0,  1,  0,  0,  "monitorenter"};
jd_jvm_opcode jvm_opcode_monitorexit = { 0xc3, 0,  1,  0,  0,  "monitorexit"};
jd_jvm_opcode jvm_opcode_wide = { 0xc4, 0,  0,  3,  0,  "wide"};
jd_jvm_opcode jvm_opcode_multianewarray = { 0xc5, 1,  0,  3,  0,  "multianewarray"};
jd_jvm_opcode jvm_opcode_ifnull = { 0xc6, 0,  1,  2,  1,  "ifnull"};
jd_jvm_opcode jvm_opcode_ifnonnull = { 0xc7, 0,  1,  2,  1,  "ifnonnull"};
jd_jvm_opcode jvm_opcode_goto_w = { 0xc8, 0,  0,  4,  1,  "goto_w"};
jd_jvm_opcode jvm_opcode_jsr_w = { 0xc9, 0,  0,  4,  1,  "jsr_w"};
jd_jvm_opcode jvm_opcode_breakpoint = { 0xca, 0,  0,  0,  0,  "breakpoint"};
jd_jvm_opcode jvm_opcode_impdep1 = { 0xfe, 0,  0,  0,  0,  "impdep1"};
jd_jvm_opcode jvm_opcode_leavefinally = { 0xff, 0,  0,  0,  0,  "leavefinally"};

void jvm_ins_setup_opcode(jd_ins *ins)
{
    switch(ins->code) {
        case  0x00: ins->op = &jvm_opcode_nop; break;
        case  0x01: ins->op = &jvm_opcode_aconst_null; break;
        case  0x02: ins->op = &jvm_opcode_iconst_m1; break;
        case  0x03: ins->op = &jvm_opcode_iconst_0; break;
        case  0x04: ins->op = &jvm_opcode_iconst_1; break;
        case  0x05: ins->op = &jvm_opcode_iconst_2; break;
        case  0x06: ins->op = &jvm_opcode_iconst_3; break;
        case  0x07: ins->op = &jvm_opcode_iconst_4; break;
        case  0x08: ins->op = &jvm_opcode_iconst_5; break;
        case  0x09: ins->op = &jvm_opcode_lconst_0; break;
        case  0x0a: ins->op = &jvm_opcode_lconst_1; break;
        case  0x0b: ins->op = &jvm_opcode_fconst_0; break;
        case  0x0c: ins->op = &jvm_opcode_fconst_1; break;
        case  0x0d: ins->op = &jvm_opcode_fconst_2; break;
        case  0x0e: ins->op = &jvm_opcode_dconst_0; break;
        case  0x0f: ins->op = &jvm_opcode_dconst_1; break;
        case  0x10: ins->op = &jvm_opcode_bipush; break;
        case  0x11: ins->op = &jvm_opcode_sipush; break;
        case  0x12: ins->op = &jvm_opcode_ldc; break;
        case  0x13: ins->op = &jvm_opcode_ldc_w; break;
        case  0x14: ins->op = &jvm_opcode_ldc2_w; break;
        case  0x15: ins->op = &jvm_opcode_iload; break;
        case  0x16: ins->op = &jvm_opcode_lload; break;
        case  0x17: ins->op = &jvm_opcode_fload; break;
        case  0x18: ins->op = &jvm_opcode_dload; break;
        case  0x19: ins->op = &jvm_opcode_aload; break;
        case  0x1a: ins->op = &jvm_opcode_iload_0; break;
        case  0x1b: ins->op = &jvm_opcode_iload_1; break;
        case  0x1c: ins->op = &jvm_opcode_iload_2; break;
        case  0x1d: ins->op = &jvm_opcode_iload_3; break;
        case  0x1e: ins->op = &jvm_opcode_lload_0; break;
        case  0x1f: ins->op = &jvm_opcode_lload_1; break;
        case  0x20: ins->op = &jvm_opcode_lload_2; break;
        case  0x21: ins->op = &jvm_opcode_lload_3; break;
        case  0x22: ins->op = &jvm_opcode_fload_0; break;
        case  0x23: ins->op = &jvm_opcode_fload_1; break;
        case  0x24: ins->op = &jvm_opcode_fload_2; break;
        case  0x25: ins->op = &jvm_opcode_fload_3; break;
        case  0x26: ins->op = &jvm_opcode_dload_0; break;
        case  0x27: ins->op = &jvm_opcode_dload_1; break;
        case  0x28: ins->op = &jvm_opcode_dload_2; break;
        case  0x29: ins->op = &jvm_opcode_dload_3; break;
        case  0x2a: ins->op = &jvm_opcode_aload_0; break;
        case  0x2b: ins->op = &jvm_opcode_aload_1; break;
        case  0x2c: ins->op = &jvm_opcode_aload_2; break;
        case  0x2d: ins->op = &jvm_opcode_aload_3; break;
        case  0x2e: ins->op = &jvm_opcode_iaload; break;
        case  0x2f: ins->op = &jvm_opcode_laload; break;
        case  0x30: ins->op = &jvm_opcode_faload; break;
        case  0x31: ins->op = &jvm_opcode_daload; break;
        case  0x32: ins->op = &jvm_opcode_aaload; break;
        case  0x33: ins->op = &jvm_opcode_baload; break;
        case  0x34: ins->op = &jvm_opcode_caload; break;
        case  0x35: ins->op = &jvm_opcode_saload; break;
        case  0x36: ins->op = &jvm_opcode_istore; break;
        case  0x37: ins->op = &jvm_opcode_lstore; break;
        case  0x38: ins->op = &jvm_opcode_fstore; break;
        case  0x39: ins->op = &jvm_opcode_dstore; break;
        case  0x3a: ins->op = &jvm_opcode_astore; break;
        case  0x3b: ins->op = &jvm_opcode_istore_0; break;
        case  0x3c: ins->op = &jvm_opcode_istore_1; break;
        case  0x3d: ins->op = &jvm_opcode_istore_2; break;
        case  0x3e: ins->op = &jvm_opcode_istore_3; break;
        case  0x3f: ins->op = &jvm_opcode_lstore_0; break;
        case  0x40: ins->op = &jvm_opcode_lstore_1; break;
        case  0x41: ins->op = &jvm_opcode_lstore_2; break;
        case  0x42: ins->op = &jvm_opcode_lstore_3; break;
        case  0x43: ins->op = &jvm_opcode_fstore_0; break;
        case  0x44: ins->op = &jvm_opcode_fstore_1; break;
        case  0x45: ins->op = &jvm_opcode_fstore_2; break;
        case  0x46: ins->op = &jvm_opcode_fstore_3; break;
        case  0x47: ins->op = &jvm_opcode_dstore_0; break;
        case  0x48: ins->op = &jvm_opcode_dstore_1; break;
        case  0x49: ins->op = &jvm_opcode_dstore_2; break;
        case  0x4a: ins->op = &jvm_opcode_dstore_3; break;
        case  0x4b: ins->op = &jvm_opcode_astore_0; break;
        case  0x4c: ins->op = &jvm_opcode_astore_1; break;
        case  0x4d: ins->op = &jvm_opcode_astore_2; break;
        case  0x4e: ins->op = &jvm_opcode_astore_3; break;
        case  0x4f: ins->op = &jvm_opcode_iastore; break;
        case  0x50: ins->op = &jvm_opcode_lastore; break;
        case  0x51: ins->op = &jvm_opcode_fastore; break;
        case  0x52: ins->op = &jvm_opcode_dastore; break;
        case  0x53: ins->op = &jvm_opcode_aastore; break;
        case  0x54: ins->op = &jvm_opcode_bastore; break;
        case  0x55: ins->op = &jvm_opcode_castore; break;
        case  0x56: ins->op = &jvm_opcode_sastore; break;
        case  0x57: ins->op = &jvm_opcode_pop; break;
        case  0x58: ins->op = &jvm_opcode_pop2; break;
        case  0x59: ins->op = &jvm_opcode_dup; break;
        case  0x5a: ins->op = &jvm_opcode_dup_x1; break;
        case  0x5b: ins->op = &jvm_opcode_dup_x2; break;
        case  0x5c: ins->op = &jvm_opcode_dup2; break;
        case  0x5d: ins->op = &jvm_opcode_dup2_x1; break;
        case  0x5e: ins->op = &jvm_opcode_dup2_x2; break;
        case  0x5f: ins->op = &jvm_opcode_swap; break;
        case  0x60: ins->op = &jvm_opcode_iadd; break;
        case  0x61: ins->op = &jvm_opcode_ladd; break;
        case  0x62: ins->op = &jvm_opcode_fadd; break;
        case  0x63: ins->op = &jvm_opcode_dadd; break;
        case  0x64: ins->op = &jvm_opcode_isub; break;
        case  0x65: ins->op = &jvm_opcode_lsub; break;
        case  0x66: ins->op = &jvm_opcode_fsub; break;
        case  0x67: ins->op = &jvm_opcode_dsub; break;
        case  0x68: ins->op = &jvm_opcode_imul; break;
        case  0x69: ins->op = &jvm_opcode_lmul; break;
        case  0x6a: ins->op = &jvm_opcode_fmul; break;
        case  0x6b: ins->op = &jvm_opcode_dmul; break;
        case  0x6c: ins->op = &jvm_opcode_idiv; break;
        case  0x6d: ins->op = &jvm_opcode_ldiv; break;
        case  0x6e: ins->op = &jvm_opcode_fdiv; break;
        case  0x6f: ins->op = &jvm_opcode_ddiv; break;
        case  0x70: ins->op = &jvm_opcode_irem; break;
        case  0x71: ins->op = &jvm_opcode_lrem; break;
        case  0x72: ins->op = &jvm_opcode_frem; break;
        case  0x73: ins->op = &jvm_opcode_drem; break;
        case  0x74: ins->op = &jvm_opcode_ine; break;
        case  0x75: ins->op = &jvm_opcode_lne; break;
        case  0x76: ins->op = &jvm_opcode_fne; break;
        case  0x77: ins->op = &jvm_opcode_dne; break;
        case  0x78: ins->op = &jvm_opcode_ishl; break;
        case  0x79: ins->op = &jvm_opcode_lshl; break;
        case  0x7a: ins->op = &jvm_opcode_ishr; break;
        case  0x7b: ins->op = &jvm_opcode_lshr; break;
        case  0x7c: ins->op = &jvm_opcode_iushr; break;
        case  0x7d: ins->op = &jvm_opcode_lushr; break;
        case  0x7e: ins->op = &jvm_opcode_iand; break;
        case  0x7f: ins->op = &jvm_opcode_land; break;
        case  0x80: ins->op = &jvm_opcode_ior; break;
        case  0x81: ins->op = &jvm_opcode_lor; break;
        case  0x82: ins->op = &jvm_opcode_ixor; break;
        case  0x83: ins->op = &jvm_opcode_lxor; break;
        case  0x84: ins->op = &jvm_opcode_iinc; break;
        case  0x85: ins->op = &jvm_opcode_i2l; break;
        case  0x86: ins->op = &jvm_opcode_i2f; break;
        case  0x87: ins->op = &jvm_opcode_i2d; break;
        case  0x88: ins->op = &jvm_opcode_l2i; break;
        case  0x89: ins->op = &jvm_opcode_l2f; break;
        case  0x8a: ins->op = &jvm_opcode_l2d; break;
        case  0x8b: ins->op = &jvm_opcode_f2i; break;
        case  0x8c: ins->op = &jvm_opcode_f2l; break;
        case  0x8d: ins->op = &jvm_opcode_f2d; break;
        case  0x8e: ins->op = &jvm_opcode_d2i; break;
        case  0x8f: ins->op = &jvm_opcode_d2l; break;
        case  0x90: ins->op = &jvm_opcode_d2f; break;
        case  0x91: ins->op = &jvm_opcode_i2b; break;
        case  0x92: ins->op = &jvm_opcode_i2c; break;
        case  0x93: ins->op = &jvm_opcode_i2s; break;
        case  0x94: ins->op = &jvm_opcode_lcmp; break;
        case  0x95: ins->op = &jvm_opcode_fcmpl; break;
        case  0x96: ins->op = &jvm_opcode_fcmp; break;
        case  0x97: ins->op = &jvm_opcode_dcmpl; break;
        case  0x98: ins->op = &jvm_opcode_dcmp; break;
        case  0x99: ins->op = &jvm_opcode_ifeq; break;
        case  0x9a: ins->op = &jvm_opcode_ifne; break;
        case  0x9b: ins->op = &jvm_opcode_iflt; break;
        case  0x9c: ins->op = &jvm_opcode_ifge; break;
        case  0x9d: ins->op = &jvm_opcode_ifgt; break;
        case  0x9e: ins->op = &jvm_opcode_ifle; break;
        case  0x9f: ins->op = &jvm_opcode_if_icmpeq; break;
        case  0xa0: ins->op = &jvm_opcode_if_icmpne; break;
        case  0xa1: ins->op = &jvm_opcode_if_icmplt; break;
        case  0xa2: ins->op = &jvm_opcode_if_icmpge; break;
        case  0xa3: ins->op = &jvm_opcode_if_icmpgt; break;
        case  0xa4: ins->op = &jvm_opcode_if_icmple; break;
        case  0xa5: ins->op = &jvm_opcode_if_acmpeq; break;
        case  0xa6: ins->op = &jvm_opcode_if_acmpne; break;
        case  0xa7: ins->op = &jvm_opcode_goto; break;
        case  0xa8: ins->op = &jvm_opcode_jsr; break;
        case  0xa9: ins->op = &jvm_opcode_ret; break;
        case  0xaa: ins->op = &jvm_opcode_tableswitch; break;
        case  0xab: ins->op = &jvm_opcode_lookupswitch; break;
        case  0xac: ins->op = &jvm_opcode_ireturn; break;
        case  0xad: ins->op = &jvm_opcode_lreturn; break;
        case  0xae: ins->op = &jvm_opcode_freturn; break;
        case  0xaf: ins->op = &jvm_opcode_dreturn; break;
        case  0xb0: ins->op = &jvm_opcode_areturn; break;
        case  0xb1: ins->op = &jvm_opcode_return; break;
        case  0xb2: ins->op = &jvm_opcode_getstatic; break;
        case  0xb3: ins->op = &jvm_opcode_putstatic; break;
        case  0xb4: ins->op = &jvm_opcode_getfield; break;
        case  0xb5: ins->op = &jvm_opcode_putfield; break;
        case  0xb6: ins->op = &jvm_opcode_invokevirtual; break;
        case  0xb7: ins->op = &jvm_opcode_invokespecial; break;
        case  0xb8: ins->op = &jvm_opcode_invokestatic; break;
        case  0xb9: ins->op = &jvm_opcode_invokeinterface; break;
        case  0xba: ins->op = &jvm_opcode_invokedynamic; break;
        case  0xbb: ins->op = &jvm_opcode_new; break;
        case  0xbc: ins->op = &jvm_opcode_newarray; break;
        case  0xbd: ins->op = &jvm_opcode_anewarray; break;
        case  0xbe: ins->op = &jvm_opcode_arraylength; break;
        case  0xbf: ins->op = &jvm_opcode_athrow; break;
        case  0xc0: ins->op = &jvm_opcode_checkcast; break;
        case  0xc1: ins->op = &jvm_opcode_instanceof; break;
        case  0xc2: ins->op = &jvm_opcode_monitorenter; break;
        case  0xc3: ins->op = &jvm_opcode_monitorexit; break;
        case  0xc4: ins->op = &jvm_opcode_wide; break;
        case  0xc5: ins->op = &jvm_opcode_multianewarray; break;
        case  0xc6: ins->op = &jvm_opcode_ifnull; break;
        case  0xc7: ins->op = &jvm_opcode_ifnonnull; break;
        case  0xc8: ins->op = &jvm_opcode_goto_w; break;
        case  0xc9: ins->op = &jvm_opcode_jsr_w; break;
        case  0xca: ins->op = &jvm_opcode_breakpoint; break;
        case  0xfe: ins->op = &jvm_opcode_impdep1; break;
        case  0xff: ins->op = &jvm_opcode_leavefinally; break;
    }
}

void init_class_opcode_hashmap(jclass_file *jc)
{
    return;
    jc->class_opcode = hashmap_init((hcmp_fn) u1_to_object_cmp, 0);
    hashmap *class_opcode = jc->class_opcode;

    add_opcode_to_map(class_opcode, 0x00, 0, "nop", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x01, 0, "aconst_null", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x02, 0, "iconst_m1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x03, 0, "iconst_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x04, 0, "iconst_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x05, 0, "iconst_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x06, 0, "iconst_3", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x07, 0, "iconst_4", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x08, 0, "iconst_5", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x09, 0, "lconst_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x0a, 0, "lconst_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x0b, 0, "fconst_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x0c, 0, "fconst_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x0d, 0, "fconst_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x0e, 0, "dconst_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x0f, 0, "dconst_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x10, 1, "bipush", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x11, 2, "sipush", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x12, 1, "ldc", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x13, 2, "ldc_w", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x14, 2, "ldc2_w", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x15, 1, "iload", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x16, 1, "lload", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x17, 1, "fload", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x18, 1, "dload", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x19, 1, "aload", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x1a, 0, "iload_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x1b, 0, "iload_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x1c, 0, "iload_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x1d, 0, "iload_3", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x1e, 0, "lload_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x1f, 0, "lload_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x20, 0, "lload_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x21, 0, "lload_3", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x22, 0, "fload_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x23, 0, "fload_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x24, 0, "fload_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x25, 0, "fload_3", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x26, 0, "dload_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x27, 0, "dload_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x28, 0, "dload_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x29, 0, "dload_3", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x2a, 0, "aload_0", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x2b, 0, "aload_1", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x2c, 0, "aload_2", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x2d, 0, "aload_3", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0x2e, 0, "iaload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x2f, 0, "laload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x30, 0, "faload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x31, 0, "daload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x32, 0, "aaload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x33, 0, "baload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x34, 0, "caload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x35, 0, "saload", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x36, 1, "istore", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x37, 1, "lstore", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x38, 1, "fstore", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x39, 1, "dstore", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x3a, 1, "astore", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x3b, 0, "istore_0", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x3c, 0, "istore_1", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x3d, 0, "istore_2", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x3e, 0, "istore_3", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x3f, 0, "lstore_0", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x40, 0, "lstore_1", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x41, 0, "lstore_2", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x42, 0, "lstore_3", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x43, 0, "fstore_0", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x44, 0, "fstore_1", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x45, 0, "fstore_2", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x46, 0, "fstore_3", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x47, 0, "dstore_0", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x48, 0, "dstore_1", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x49, 0, "dstore_2", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x4a, 0, "dstore_3", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x4b, 0, "astore_0", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x4c, 0, "astore_1", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x4d, 0, "astore_2", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x4e, 0, "astore_3", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0x4f, 0, "iastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x50, 0, "lastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x51, 0, "fastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x52, 0, "dastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x53, 0, "aastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x54, 0, "bastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x55, 0, "castore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x56, 0, "sastore", 0, 3, 0);
    add_opcode_to_map(class_opcode, 0x57, 0, "pop", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x58, 0, "pop2", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x59, 0, "dup", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x5a, 0, "dup_x1", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x5b, 0, "dup_x2", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x5c, 0, "dup2", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x5d, 0, "dup2_x1", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x5e, 0, "dup2_x2", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x5f, 0, "swap", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x60, 0, "iadd", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x61, 0, "ladd", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x62, 0, "fadd", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x63, 0, "dadd", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x64, 0, "isub", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x65, 0, "lsub", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x66, 0, "fsub", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x67, 0, "dsub", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x68, 0, "imul", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x69, 0, "lmul", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x6a, 0, "fmul", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x6b, 0, "dmul", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x6c, 0, "idiv", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x6d, 0, "ldiv", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x6e, 0, "fdiv", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x6f, 0, "ddiv", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x70, 0, "irem", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x71, 0, "lrem", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x72, 0, "frem", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x73, 0, "drem", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x74, 0, "ine", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x75, 0, "lne", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x76, 0, "fne", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x77, 0, "dne", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x78, 0, "ishl", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x79, 0, "lshl", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x7a, 0, "ishr", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x7b, 0, "lshr", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x7c, 0, "iushr", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x7d, 0, "lushr", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x7e, 0, "iand", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x7f, 0, "land", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x80, 0, "ior", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x81, 0, "lor", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x82, 0, "ixor", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x83, 0, "lxor", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x84, 2, "iinc", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0x85, 0, "i2l", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x86, 0, "i2f", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x87, 0, "i2d", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x88, 0, "l2i", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x89, 0, "l2f", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x8a, 0, "l2d", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x8b, 0, "f2i", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x8c, 0, "f2l", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x8d, 0, "f2d", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x8e, 0, "d2i", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x8f, 0, "d2l", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x90, 0, "d2f", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x91, 0, "i2b", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x92, 0, "i2c", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x93, 0, "i2s", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0x94, 0, "lcmp", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x95, 0, "fcmpl", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x96, 0, "fcmp", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x97, 0, "dcmpl", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x98, 0, "dcmp", 0, 2, 1);
    add_opcode_to_map(class_opcode, 0x99, 2, "ifeq", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0x9a, 2, "ifne", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0x9b, 2, "iflt", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0x9c, 2, "ifge", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0x9d, 2, "ifgt", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0x9e, 2, "ifle", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0x9f, 2, "if_icmpeq", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa0, 2, "if_icmpne", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa1, 2, "if_icmplt", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa2, 2, "if_icmpge", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa3, 2, "if_icmpgt", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa4, 2, "if_icmple", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa5, 2, "if_acmpeq", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa6, 2, "if_acmpne", 1, 2, 0);
    add_opcode_to_map(class_opcode, 0xa7, 2, "goto", 1, 0, 0);
    add_opcode_to_map(class_opcode, 0xa8, 2, "jsr", 1, 0, 0);
    add_opcode_to_map(class_opcode, 0xa9, 1, "ret", 1, 0, 0);
    add_opcode_to_map(class_opcode, 0xaa, 16, "tableswitch", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0xab, 8, "lookupswitch", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0xac, 0, "ireturn", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xad, 0, "lreturn", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xae, 0, "freturn", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xaf, 0, "dreturn", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xb0, 0, "areturn", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xb1, 0, "return", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xb2, 2, "getstatic", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0xb3, 2, "putstatic", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xb4, 2, "getfield", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xb5, 2, "putfield", 0, 2, 0);
    add_opcode_to_map(class_opcode, 0xb6, 2, "invokevirtual", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xb7, 2, "invokespecial", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xb8, 2, "invokestatic", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xb9, 4, "invokeinterface", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xba, 4, "invokedynamic", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xbb, 2, "new", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0xbc, 1, "newarray", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xbd, 2, "anewarray", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xbe, 0, "arraylength", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xbf, 0, "athrow", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xc0, 2, "checkcast", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xc1, 2, "instanceof", 0, 1, 1);
    add_opcode_to_map(class_opcode, 0xc2, 0, "monitorenter", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xc3, 0, "monitorexit", 0, 1, 0);
    add_opcode_to_map(class_opcode, 0xc4, 3, "wide", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xc5, 3, "multianewarray", 0, 0, 1);
    add_opcode_to_map(class_opcode, 0xc6, 2, "ifnull", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0xc7, 2, "ifnonnull", 1, 1, 0);
    add_opcode_to_map(class_opcode, 0xc8, 4, "goto_w", 1, 0, 0);
    add_opcode_to_map(class_opcode, 0xc9, 4, "jsr_w", 1, 0, 0);
    add_opcode_to_map(class_opcode, 0xca, 0, "breakpoint", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xfe, 0, "impdep1", 0, 0, 0);
    add_opcode_to_map(class_opcode, 0xff, 0, "leavefinally", 0, 0, 0);
}
#endif


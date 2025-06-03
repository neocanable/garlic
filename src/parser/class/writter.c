#include "metadata.h"

static void print_local_variable_table_attribute(jclass_file* jc, jattr* attribute)
{
    jattr_local_variable_table *attr = (jattr_local_variable_table *) attribute->info;

    fprintf(stdout, "  LocalVariableTable:\n");
    fprintf(stdout, "\t  %s %5s %5s %10s\t%s\n", 
            "Start", "Length", "Slots", "Name", "Signature");

    for (int l = 0; l < be16toh(attr->local_variable_table_length); ++l)
    {
        jattr_local_variable *item = &attr->local_variable_table[l];
        fprintf(stdout, "\t  %d\t%5d\t%5d\t%10s\t%s\n",
                be16toh(item->start_pc),
                be16toh(item->length),
                be16toh(item->index),
                pool_str(jc, item->name_index),
                pool_str(jc, item->descriptor_index)
        );
    }

}

static void print_line_number_table_attribute(jclass_file* jc, jattr* attribute)
{
    jattr_line_number_table *attr = (jattr_line_number_table*) attribute->info;
    uint16_t _length = be16toh(attr->line_number_table_length);
    if (_length == 0) return;
    fprintf(stdout, "  LineNumberTable:\n");
    for (int j = 0; j < _length; ++j)
    {
        jattr_line_number* item = &attr->line_number_table[j];
        fprintf(stdout, "\t  line %d: %d\n", 
                be16toh(item->line_number), be16toh(item->start_pc));
    }
}

static void print_stack_map_table_attribute(jclass_file* jclass, jattr* attribute)
{
    jattr_stack_map_table *attr = (jattr_stack_map_table*) attribute->info;
    uint16_t _length = be16toh(attr->number_of_entries);
    if (_length == 0) return;
    fprintf(stdout, "  StackMapTable: number_of_entries = %d\n", _length);
    for (int j = 0; j < _length; ++j)
    {
        stack_map_frame *item = &attr->entries[j];
        u1 frame_type = item->same_frame->frame_type;
        fprintf(stdout, "\t  frame_type = %d \n", frame_type);
    }
}

void print_code_section(jclass_file* jclass, jattr_code *code_attr)
{
    u1 *code = code_attr->code;
    fprintf(stdout, "  Code:\n");
    for (int j = 0; j < be32toh(code_attr->code_length); )
    {
        u1 opcode = code_attr->code[j];
//        fprintf(stdout, "the code is: %02x\n", opcode);
        int param_length = get_opcode_param_length(jclass, opcode);
        switch (opcode) {
            case 0x00: {
                fprintf(stdout, "\t%4d: %-15s %10s \t// nop\n", j, "nop", "");
                break;
            }
            case 0x01:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push null\n", 
                        j, "aconst_null", "");
                break;
            case 0x02:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int -1\n", 
                        j, "iconst_m1", "");
                break;
            case 0x03:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int 0\n", j, "iconst_0", "");
                break;
            case 0x04:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int 1\n", j, "iconst_1", "");
                break;
            case 0x05:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int 2\n", j, "iconst_2", "");
                break;
            case 0x06:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int 3\n", j, "iconst_3", "");
                break;
            case 0x07:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int 4\n", j, "iconst_4", "");
                break;
            case 0x08:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push int 5\n", j, "iconst_5", "");
                break;
            case 0x09:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push long 0\n", j, "lconst_0", "");
                break;
            case 0x0a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push long 1\n", j, "lconst_1", "");
                break;
            case 0x0b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push float 0\n", j, "fconst_0", "");
                break;
            case 0x0c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push float 1\n", j, "fconst_1", "");
                break;
            case 0x0d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push float 2\n", j, "fconst_2", "");
                break;
            case 0x0e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push double 0\n", j, "dconst_0", "");
                break;
            case 0x0f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push double 1\n", j, "dconst_1", "");
                break;
            case 0x10: {
                u1 param = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// push 1 byte int\n", j, "bipush", param);
                break;
            }
            case 0x11: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t num = ((uint16_t) param0 << 8) | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// push 2 byte int\n", j, "sipush", num);
                break;
            }
            case 0x12: {
                u1 param0 = code[j + 1]; // it's u1 not u2
                fprintf(stdout, "\t%4d: %-15s %10d \t// load index is: %d \"%s\" from const pool\n", j, "ldc",
                        param0, param0, pool_u1_str(jclass, param0));
                break;
            }
            case 0x13: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t num = ((uint16_t) param0 << 8) | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// load index is: %d \"%s\" from const pool\n", j, "ldc_w",
                        num, num,
                        pool_str(jclass, be16toh(num)));
                // TODO: fix the param
                break;
            }
            case 0x14: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t num = ((uint16_t) param0 << 8) | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// load index is: %d \"%s\" from const pool\n", j, "ldc2_w", num,
                        num, pool_str(jclass, be16toh(num)));
                break;
            }
            case 0x15: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// load int LocalVariablesTable[%d]\n", j, "iload", param0,
                        param0);
                break;
            }
            case 0x16: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// load long LocalVariableTable[%d]\n", j, "lload", param0,
                        param0);
                break;
            }
            case 0x17: {
                u1 params0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// load float LocalVariableTable[%d]\n", j, "fload",
                        params0, params0);
                break;
            }
            case 0x18: {
                u1 params0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// load double LocalVariableTable[%d]\n", j, "dload",
                        params0, params0);
                break;
            }
            case 0x19: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// load object LocalVariableTable[%d]\n", j,
                        "aload", param0, param0);
                break;
            }
            case 0x1a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load int LocalVariableTable[0]\n", j, "iload_0",
                        "");
                break;
            case 0x1b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load int LocalVariableTable[1]\n", j, "iload_1",
                        "");
                break;
            case 0x1c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load int LocalVariableTable[2]\n", j, "iload_2",
                        "");
                break;
            case 0x1d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load int LocalVariableTable[3]\n", j, "iload_3",
                        "");
                break;
            case 0x1e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load long LocalVariableTable[0]\n", j, "lload_0",
                        "");
                break;
            case 0x1f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load long LocalVariableTable[1]\n", j, "lload_1",
                        "");
                break;
            case 0x20:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load long LocalVariableTable[2]\n", j, "lload_2",
                        "");
                break;
            case 0x21:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load long LocalVariableTable[3]\n", j, "lload_3",
                        "");
                break;
            case 0x22:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load float LocalVariableTable[0]\n", j, "fload_0",
                        "");
                break;
            case 0x23:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load float LocalVariableTable[1]\n", j, "fload_1",
                        "");
                break;
            case 0x24:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load float LocalVariableTable[2]\n", j, "fload_2",
                        "");
                break;
            case 0x25:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load float LocalVariableTable[3]\n", j, "fload_3",
                        "");
                break;
            case 0x26:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load double LocalVariableTable[0]\n", j, "dload_0",
                        "");
                break;
            case 0x27:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load double LocalVariableTable[1]\n", j, "dload_1",
                        "");
                break;
            case 0x28:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load double LocalVariableTable[2]\n", j, "dload_2",
                        "");
                break;
            case 0x29:
                fprintf(stdout, "\t%4d: %-15s %10s \t// load double LocalVariableTable[3]\n", j, "dload_3",
                        "");
                break;
            case 0x2a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push object LocalVariableTable[0]\n", j,
                        "aload_0", "");
                break;
            case 0x2b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push object LocalVariableTable[1]\n", j,
                        "aload_1", "");
                break;
            case 0x2c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push object LocalVariableTable[2]\n", j,
                        "aload_2", "");
                break;
            case 0x2d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push object LocalVariableTable[3]\n", j,
                        "aload_3", "");
                break;
            case 0x2e:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load int from array\n",
                        j, "iaload", "");
                break;
            case 0x2f:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load long from array\n",
                        j, "laload", "");
                break;
            case 0x30:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load float from array\n",
                        j, "faload", "");
                break;
            case 0x31:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load double from array\n",
                        j, "daload", "");
                break;
            case 0x32:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load object from array\n",
                        j, "aaload", "");
                break;
            case 0x33:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load byte from array\n",
                        j, "baload", "");
                break;
            case 0x34:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load char from array\n",
                        j, "caload", "");
                break;
            case 0x35:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// load short from array\n",
                        j, "saload", "");
                break;
            case 0x36: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int store to LocalVariableTable[%d]\n", j, "istore",
                        param0, param0);
                break;
            }
            case 0x37: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop long store to LocalVariableTable[%d]\n", j,
                        "lstore", param0, param0);
                break;
            }
            case 0x38: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop float store to LocalVariableTable[%d]\n", j,
                        "fstore", param0, param0);
                break;
            }
            case 0x39: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop double store to LocalVariableTable[%d]\n", j,
                        "dstore", param0, param0);
                break;
            }
            case 0x3a: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop object store to LocalVariableTable[%d]\n", j,
                        "astore", param0, param0);
                break;
            }
            case 0x3b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int store to LocalVariableTable[0]\n", j,
                        "istore_0", "");
                break;
            case 0x3c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int store to LocalVariableTable[1]\n", j,
                        "istore_1", "");
                break;
            case 0x3d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int store to LocalVariableTable[2]\n", j,
                        "istore_2", "");
                break;
            case 0x3e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int store to LocalVariableTable[3]\n", j,
                        "istore_3", "");
                break;
            case 0x3f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long store to LocalVariableTable[0]\n", j,
                        "lstore_0", "");
                break;
            case 0x40:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long store to LocalVariableTable[1]\n", j,
                        "lstore_1", "");
                break;
            case 0x41:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long store to LocalVariableTable[2]\n", j,
                        "lstore_2", "");
                break;
            case 0x42:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long store to LocalVariableTable[3]\n", j,
                        "lstore_3", "");
                break;
            case 0x43:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float store to LocalVariableTable[0]\n", j,
                        "fstore_0", "");
                break;
            case 0x44:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float store to LocalVariableTable[1]\n", j,
                        "fstore_1", "");
                break;
            case 0x45:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float store to LocalVariableTable[2]\n", j,
                        "fstore_2", "");
                break;
            case 0x46:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float store to LocalVariableTable[3]\n", j,
                        "fstore_3", "");
                break;
            case 0x47:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double store to LocalVariableTable[0]\n", j,
                        "dstore_0", "");
                break;
            case 0x48:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double store to LocalVariableTable[1]\n", j,
                        "dstore_1", "");
                break;
            case 0x49:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double store to LocalVariableTable[2]\n", j,
                        "dstore_2", "");
                break;
            case 0x4a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double store to LocalVariableTable[3]\n", j,
                        "dstore_3", "");
                break;
            case 0x4b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop object store to LocalVariableTable[0]\n", j,
                        "astore_0", "");
                break;
            case 0x4c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop object store to LocalVariableTable[1]\n", j,
                        "astore_1", "");
                break;
            case 0x4d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop object store to LocalVariableTable[2]\n", j,
                        "astore_2", "");
                break;
            case 0x4e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop object store to LocalVariableTable[3]\n", j,
                        "astore_3", "");
                break;
            case 0x4f:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store int to array\n",
                        j, "iastore", "");
                break;
            case 0x50:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store long to array\n",
                        j, "lastore", "");
                break;
            case 0x51:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store float to array\n",
                        j, "fastore", "");
                break;
            case 0x52:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store double to array\n",
                        j, "dastore", "");
                break;
            case 0x53:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store object to array\n",
                        j, "aastore", "");
                break;
            case 0x54:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store byte to array\n",
                        j, "bastore", "");
                break;
            case 0x55:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store char to array\n",
                        j, "castore", "");
                break;
            case 0x56:
                fprintf(stdout,
                        "\t%4d: %-15s %10s \t// store short to array\n",
                        j, "sastore", "");
                break;
            case 0x57:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop\n", j, "pop", "");
                break;
            case 0x58:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop one(double, float) or two\n", j, "pop2", "");
                break;
            case 0x59:
                fprintf(stdout, "\t%4d: %-15s %10s \t// dup stack top\n", j, "dup", "");
                break;
            case 0x5a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// dup_x1\n", j, "dup_x1", "");
                break;
            case 0x5b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// dup_x2\n", j, "dup_x2",
                        "");
                break;
            case 0x5c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// dup2\n", j, "dup2", "");
                break;
            case 0x5d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// todo\n", j, "dup2_x1", "");
                break;
            case 0x5e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// todo\n", j, "dup2_x2", "");
                break;
            case 0x5f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// swap stack top with top+1\n", j, "swap", "");
                break;
            case 0x60:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v1, v2, push v1+v2\n", j, "iadd", "");
                break;
            case 0x61:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v1, v2, push v1+v2\n", j, "ladd", "");
                break;
            case 0x62:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v1, v2, push v1+v2\n", j, "fadd", "");
                break;
            case 0x63:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v1, v2, push v1+v2\n", j, "dadd", "");
                break;
            case 0x64:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v2,v1, push v1-v2, (v2 is stack top)\n", j, "isub", "");
                break;
            case 0x65:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v2, v1, push v1-v2\n", j, "lsub", "");
                break;
            case 0x66:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v2, v1, push v1-v2\n", j, "fsub", "");
                break;
            case 0x67:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v2, v1, push v1-v2\n", j, "dsub", "");
                break;
            case 0x68:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v1, v2, push v1*v2\n", j, "imul", "");
                break;
            case 0x69:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v1, v2, push v1*v2\n", j, "lmul", "");
                break;
            case 0x6a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v1, v2, push v1*v2\n", j, "fmul", "");
                break;
            case 0x6b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v1, v2, push v1*v2\n", j, "dmul", "");
                break;
            case 0x6c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v2, v1, push v1/v2\n", j, "idiv", "");
                break;
            case 0x6d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v2, v1, push v1/v2\n", j, "ldiv", "");
                break;
            case 0x6e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v2, v1, push v1/v2\n", j, "fdiv", "");
                break;
            case 0x6f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v2, v1, push v1/v2\n", j, "ddiv", "");
                break;
            case 0x70:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v2, v1, push v1 rem v2\n", j, "irem", "");
                break;
            case 0x71:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v2, v1, push v1 rem v2\n", j, "lrem", "");
                break;
            case 0x72:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v2, v1, push v1 rem v2\n", j, "frem", "");
                break;
            case 0x73:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v2, v1, push v1 rem v2\n", j, "drem", "");
                break;
            case 0x74:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v, push ~v\n", j, "ineg", "");
                break;
            case 0x75:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v, push ~v\n", j, "lneg", "");
                break;
            case 0x76:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v, push ~v\n", j, "fneg", "");
                break;
            case 0x77:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v, push ~v\n", j, "dneg", "");
                break;
            case 0x78:
                fprintf(stdout,"\t%4d: %-15s %10s \t// int shift left\n", j,"ishl", "");
                break;
            case 0x79:
                fprintf(stdout,"\t%4d: %-15s %10s \t// long shift left\n", j, "lshl", "");
                break;
            case 0x7a:
                fprintf(stdout,"\t%4d: %-15s %10s \t// int shift right\n", j, "ishr", "");
                break;
            case 0x7b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// long shift right\n", j, "lshr", "");
                break;
            case 0x7c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// unsigned int shift right\n", j, "iushr", "");
                break;
            case 0x7d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// unsigned long shift right\n", j, "lushr", "");
                break;
            case 0x7e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop boolean or int v2, v1, push v1 & v2\n", j, "iand", "");
                break;
            case 0x7f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop boolean long v2, v1, push v1 & v2\n", j, "land", "");
                break;
            case 0x80:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int or boolean v2, v1, push v1 | v2\n", j, "ior", "");
                break;
            case 0x81:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long or boolean v2, v1, push v1 | v2\n", j, "lor", "");
                break;
            case 0x82:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int or boolean v2, v1, push v1^v2\n", j, "ixor", "");
                break;
            case 0x83:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long or boolean v2, v1, push v1^v2\n", j, "lxor", "");
                break;
            case 0x84: {
                u1 p0 = code[j + 1];
                u2 p1 = code[j + 2];
                fprintf(stdout, "\t%4d: %-15s %d, %d \t// LocalVariableTable[%d] += %d\n", j, "iinc", p0, p1, p0, p1);
                break;
            }
            case 0x85:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v, push long v\n", j, "i2l", "");
                break;
            case 0x86:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v push float v\n", j, "i2f", "");
                break;
            case 0x87:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int v push double v\n", j, "i2d", "");
                break;
            case 0x88:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v, push int v\n", j, "l2i", "");
                break;
            case 0x89:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v, push float v\n", j, "l2f", "");
                break;
            case 0x8a:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v, push double v\n", j, "l2d", "");
                break;
            case 0x8b:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v, push int v\n", j, "f2i", "");
                break;
            case 0x8c:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v, push long v\n", j, "f2l", "");
                break;
            case 0x8d:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v, push double v\n", j, "f2d", "");
                break;
            case 0x8e:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v, push int v\n", j, "d2i", "");
                break;
            case 0x8f:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v, push long v\n", j, "d2l", "");
                break;
            case 0x90:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v, push float v\n", j, "d2f", "");
                break;
            case 0x91:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int, push byte\n", j, "i2b", "");
                break;
            case 0x92:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int, push char\n", j, "i2c", "");
                break;
            case 0x93:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop int, push char\n", j, "i2s", "");
                break;
            case 0x94:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop long v2, v1, v1==v2 push 0, v1 > v2 push 1, v1 < v2 push -1\n",
                        j, "lcmp", "");
                break;
            case 0x95:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v2, v1, v1 == v2 push 0, "
                                "v1 > v2 push 1, v1 < v2 or (v1 == NaN || v2 == NaN) push -1\n", j, "fcmpl", "");
                break;
            case 0x96:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop float v2, v1, v1 == v2 push 0, v1 > v2 push -1, "
                                "v1 < v2 or (v1 == NaN || v2 == NaN) push 1\n", j, "fcmpg", "");
                break;
            case 0x97:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v2, v1, v1 == v2 push 0, v1 > v2 push 1, "
                                "v1 < v2 or (v1 == NaN || v2 == NaN) push -1\n", j, "dcmpl", "");
                break;
            case 0x98:
                fprintf(stdout, "\t%4d: %-15s %10s \t// pop double v2, v1, v1 == v2 push 0, v1 > v2 push -1, "
                                "v1 < v2 or (v1 == NaN || v2 == NaN) push 1\n", j, "dcmpg", "");
                break;
            case 0x99: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v, v == 0 jump to: %d\n", j, "ifeq", offset, j + offset);
                break;
            }
            case 0x9a: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v, v != 0 jump to: %d\n", j, "ifne", offset, j + offset);
                break;
            }
            case 0x9b: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-10s %10d \t// pop int v, v < 0 jump to : %d\n", j, "iflt", offset, j + offset);
                break;
            }
            case 0x9c: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v, v >= 0 jump to: %d\n", j, "ifge", offset, j + offset);
                break;
            }
            case 0x9d: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v, v > 0 jump to: %d\n", j, "ifgt",
                        offset, j + offset);
                break;
            }
            case 0x9e: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v, v < 0 jump to: %d\n", j, "iflt", offset, j + offset);
                break;
            }
            case 0x9f: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v2, v1, v1 == v2 jump to: %d\n", j, "if_icmpeq", offset, j + offset);
                break;
            }
            case 0xa0: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v2, v1, v1 != v2 jump to: %d\n", j, "if_icmpne", offset, j + offset);
                break;
            }
            case 0xa1: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v2, v1, v1 < v2 jump to: %d\n", j, "if_icmplt", offset, j + offset);
                break;
            }
            case 0xa2: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v2, v1, v1 >= v2: %d\n", j, "if_icmpge", offset, j + offset);
                break;
            }
            case 0xa3: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v2, v1, v1 > v2 jump to: %d\n", j, "if_icmpgt", offset, j + offset);
                break;
            }
            case 0xa4: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop int v2, v1, v1 <= v2 jump to: %d\n", j, "if_icmple", offset, j + offset);
                break;
            }
            case 0xa5: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop object v2, v1, v1 == v2 jump to: %d\n", j, "if_acmpeq", offset, j + offset);
                break;
            }
            case 0xa6: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop object v2, v1, v1 != v2 jump to: %d\n", j, "if_acmpne", offset, j + offset);
                break;
            }
            case 0xa7: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                int16_t offset = (int16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// goto goto_offset: %d\n", j, "goto", offset,
                        j + offset);
                break;
            }
            case 0xa8: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// 跳转到offset，下一条指令地址压栈到栈顶，跳到位置为: %d\n", j,
                        "jsr", offset, j + offset);
                break;
            }
            case 0xa9: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10d \t// 返回到offset\n", j, "ret", param0);
                break;
            }
            case 0xaa: {
                int padding_len = (4 - (j + 1) % 4) >= 4 ? 0 : (4 - (j + 1) % 4);
                u1 param_default1 = code[j + padding_len + 1];
                u1 param_default2 = code[j + padding_len + 2];
                u1 param_default3 = code[j + padding_len + 3];
                u1 param_default4 = code[j + padding_len + 4];
                int32_t default_offset_byte =
                        (int32_t) param_default1 << 24 | param_default2 << 16 | param_default3 << 8 | param_default4;

                u1 param_low1 = code[j + padding_len + 5];
                u1 param_low2 = code[j + padding_len + 6];
                u1 param_low3 = code[j + padding_len + 7];
                u1 param_low4 = code[j + padding_len + 8];
                int32_t low_byte = (int32_t) param_low1 << 24 | param_low2 << 16 | param_low3 << 8 | param_low4;

                u1 param_high1 = code[j + padding_len + 9];
                u1 param_high2 = code[j + padding_len + 10];
                u1 param_high3 = code[j + padding_len + 11];
                u1 param_high4 = code[j + padding_len + 12];
                int32_t high_byte = (int32_t) param_high1 << 24 | param_high2 << 16 | param_high3 << 8 | param_high4;

                uint32_t jump_size = high_byte - low_byte + 1;
                uint32_t jump_arr[jump_size];
                int start_jump = padding_len + 12;
                fprintf(stdout,
                        "\t%4d: tableswitch:  default_offset: %d , low - high (%d - %d)\n\t\tarray size: %d\n", j,
                        default_offset_byte + j, low_byte, high_byte, jump_size);
                for (uint32_t k = 0; k < jump_size; k++) {
                    u1 p1 = code[j + start_jump + k * 4 + 1];
                    u1 p2 = code[j + start_jump + k * 4 + 2];
                    u1 p3 = code[j + start_jump + k * 4 + 3];
                    u1 p4 = code[j + start_jump + k * 4 + 4];
                    uint32_t offset = (uint32_t) p1 << 24 | p2 << 16 | p3 << 8 | p4;
                    jump_arr[k] = offset;
                    fprintf(stdout, "\t\t\tgoto_offset: %d\n", offset + j);
                }
                fprintf(stdout, "\t%4s}\n", "");
                param_length = jump_size * 4 + padding_len + 12;
                break;
            }
            case 0xab: {
                // padding 3
//            int padding_len = 4 - (j+1)%4;
//            if (padding_len >= 4) padding_len = 0;
                int padding_len = (4 - (j + 1) % 4) >= 4 ? 0 : (4 - (j + 1) % 4);

                u1 param_default1 = code[j + padding_len + 1];
                u1 param_default2 = code[j + padding_len + 2];
                u1 param_default3 = code[j + padding_len + 3];
                u1 param_default4 = code[j + padding_len + 4];
                uint32_t default_offset_byte =
                        (uint32_t) param_default1 << 24 | param_default2 << 16 | param_default3 << 8 | param_default4;

                u1 param_np1 = code[j + padding_len + 5];
                u1 param_np2 = code[j + padding_len + 6];
                u1 param_np3 = code[j + padding_len + 7];
                u1 param_np4 = code[j + padding_len + 8];
                uint32_t npair = (uint32_t) param_np1 << 24 | param_np2 << 16 | param_np3 << 8 | param_np4;

                DEBUG_PRINT("npairs: order is: %02x %02x %02x %02x\n", param_np1, param_np2, param_np3, param_np4);
                int start_npair = padding_len + 8;
                int32_t jump_arr[npair * 2];
                for (uint32_t k = 0; k < npair; ++k) {
                    u1 p1 = code[j + start_npair + k * 8 + 1];
                    u1 p2 = code[j + start_npair + k * 8 + 2];
                    u1 p3 = code[j + start_npair + k * 8 + 3];
                    u1 p4 = code[j + start_npair + k * 8 + 4];
                    int32_t key = (int32_t) p1 << 24 | p2 << 16 | p3 << 8 | p4;
                    u1 p5 = code[j + start_npair + k * 8 + 5];
                    u1 p6 = code[j + start_npair + k * 8 + 6];
                    u1 p7 = code[j + start_npair + k * 8 + 7];
                    u1 p8 = code[j + start_npair + k * 8 + 8];
                    int32_t val = (int32_t) p5 << 24 | p6 << 16 | p7 << 8 | p8;

                    jump_arr[k * 2] = key;
                    jump_arr[k * 2 + 1] = val;
                }

                param_length = 8 + padding_len + npair * 8;
                fprintf(stdout, "\t%4d: lookupswitch:  %d npair size: %d\n", j, default_offset_byte + j, npair);
                for (int k = 0; k < npair; ++k) {
                    fprintf(stdout, "\t\t\tkey: %d, goto_offset: %d\n", jump_arr[k * 2], jump_arr[k * 2 + 1] + j);
                }
                fprintf(stdout, "\t%4s}\n", "");
                break;
            }
            case 0xac:
                fprintf(stdout, "\t%4d: %-15s %10s \t// return int\n", j, "ireturn", "");
                break;
            case 0xad:
                fprintf(stdout, "\t%4d: %-15s %10s \t// return long\n", j, "lreturn", "");
                break;
            case 0xae:
                fprintf(stdout, "\t%4d: %-15s %10s \t// return float\n", j, "freturn", "");
                break;
            case 0xaf:
                fprintf(stdout, "\t%4d: %-15s %10s \t// return double\n", j, "dreturn", "");
                break;
            case 0xb0:
                fprintf(stdout, "\t%4d: %-15s %10s \t// return object\n", j, "areturn", "");
                break;
            case 0xb1:
                fprintf(stdout, "\t%4d: %-15s %10s \t// return\n", j, "return", "");
                break;
            case 0xb2: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "getstatic", index, pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb3: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "putstatic", index, pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb4: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "getfield", index, pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb5: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "putfield", index, pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb6: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// call member m: %s\n", j, "invokevirtual", index,
                        pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb7: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// call parent constructor: %s\n", j, "invokespecial", index,
                        pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb8: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// call static m: %s\n", j, "invokestatic", index,
                        pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xb9: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-10s %10d \t// call interface m: %s\n", j, "invokeinterface", index,
                        pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xba: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-10s %10d \t// dynamic call: %s\n", j, "invokedynamic", index,
                        pool_str(jclass, be16toh(index)));
            }
            case 0xbb: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10s \t// %s\n", j, "new", "", pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xbc: {
                u1 param0 = code[j + 1];
                fprintf(stdout, "\t%4d: %-15s %10s \t// %d new an array\n", j, "newarray", "", param0);
                break;
            }
            case 0xbd: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "anewarray", index, pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xbe:
                fprintf(stdout, "\t%4d: %-15s %10s \t// push array length\n", j, "arraylength", "");
                break;
            case 0xbf:
                fprintf(stdout, "\t%4d: %-15s %10s \t// throw exception\n", j, "athrow", "");
                break;
            case 0xc0: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "checkcast", index, pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xc1: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// %s\n", j, "instanceof", index,
                        pool_str(jclass, be16toh(index)));
                break;
            }
            case 0xc2:
                fprintf(stdout, "\t%4d: %-15s %10s \t// lock\n", j, "monitorenter", "");
                break;
            case 0xc3:
                fprintf(stdout, "\t%4d: %-15s %10s \t// unlock\n", j, "monitorexit", "");
                break;
            case 0xc4: {
                u1 modify_opcode = code[j + 1];
                if (modify_opcode == 0x15) { // iload
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "iload_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x16) { // lload
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "lload_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x17) { // fload
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "fload_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x18) { // dload
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "dload_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x19) { // aload
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "aload_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x36) { // istore
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "istore_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x37) { // lstore
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "lstore_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x38) { // fstore
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "fstore_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x39) { // dstore
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "dstore_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x3a) { // astore
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "astore_w", index);
                    param_length = 3;
                } else if (modify_opcode == 0x84) { // iinc
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    u1 p2 = code[j + 4];
                    u1 p3 = code[j + 5];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    int16_t const_value = (int16_t) p2 << 8 | p3;
                    fprintf(stdout, "\t%4d: %-15s %10d %d\n", j, "iinc_w", index, const_value);
                    param_length = 5;
                } else if (modify_opcode == 0xa9) { // ret
                    u1 p0 = code[j + 2];
                    u1 p1 = code[j + 3];
                    uint16_t index = (uint16_t) p0 << 8 | p1;
                    fprintf(stdout, "\t%4d: %-15s %10d\n", j, "ret_w", index);
                    param_length = 3;
                }
                break;
            }
            case 0xc5: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                u1 param2 = code[j + 3];
                uint16_t index = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10s \t// 创建 %d 多维数组\n", j, "multidimensional array",
                        pool_str(jclass, be16toh(index)), param2);
                break;
            }
            case 0xc6: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop object v, v == null jump to: %d\n", j, "ifnull", offset,
                        j + offset);
                break;
            }
            case 0xc7: {
                u1 param0 = code[j + 1];
                u1 param1 = code[j + 2];
                uint16_t offset = (uint16_t) param0 << 8 | param1;
                fprintf(stdout, "\t%4d: %-15s %10d \t// pop object v, v != null jump to: %d\n", j, "ifnonnull", offset,
                        j + offset);
                break;
            }
            case 0xc8: {
                u1 p1 = code[j + 1];
                u1 p2 = code[j + 2];
                u1 p3 = code[j + 3];
                u1 p4 = code[j + 4];
                int32_t offset = (uint32_t) p1 << 24 | p2 << 16 | p3 << 8 | p4;
                fprintf(stdout, "\t%4d: %-15s %10d \t// jump to: %d\n", j, "w_w", offset, j + offset);
                break;
            }
            case 0xc9: {
                u1 p1 = code[j + 1];
                u1 p2 = code[j + 2];
                u1 p3 = code[j + 3];
                u1 p4 = code[j + 4];
                uint32_t offset = (uint32_t) p1 << 24 | p2 << 16 | p3 << 8 | p4;
                fprintf(stdout, "\t%4d: %-15s %10d\t//无条件跳转，跳到位置为: %d\n", j, "jsr_w", offset, j + offset);
                break;
            }
            case 0xff:
                fprintf(stdout, "\t%4d: %-15s %10s \t// %s\n", j, "finallyleave", "", "");
                break;
            case 0xfe:
                fprintf(stdout, "\t%4d: %-15s %10s \t// %s\n", j, "impdep2", "", "");
                break;
            default:
                fprintf(stdout, "\t%4d: %-15s %10s \t// %s\n", j, "unknown", "", "");
                break;
        }

        j+= param_length + 1;

    }

}

void print_methods_section(jclass_file *jc)
{
    for (int i = 0; i < be16toh(jc->methods_count); ++i)
    {
        jmethod *method = &jc->methods[i];
        fprintf(stdout, "%s%s\n",
                get_method_access_flags_str(method->access_flags),
                pool_str(jc, method->name_index));


        for (int j = 0; j < be16toh(method->attributes_count); ++j) {
            jattr *_p_attr = &method->attributes[j];
            if (STR_EQL(_p_attr->name, "Code")) {
                print_code_section(jc, (jattr_code*)_p_attr->info);

                jattr_code *codeAttribute = (jattr_code *) _p_attr->info;
                for (int k = 0; k < be16toh(codeAttribute->attributes_count); ++k) {
                    jattr *_attr = &codeAttribute->attributes[k];
                    if (STR_EQL(_attr->name, "LocalVariableTable"))
                        print_local_variable_table_attribute(jc, _attr);
                    /*
                    if (STR_EQL(_attr->name, "LineNumberTable"))
                        print_line_number_table_attribute(jc, _attr);
                    if (STR_EQL(_attr->name, "StackMapTable"))
                        print_stack_map_table_attribute(jc, _attr);
                    */
                }

//                Code_attribute_exception_table *exception_table = codeAttribute->exception_table;
                for (int k = 0; k < be16toh(codeAttribute->exception_table_length); ++k) 
                {
                    jattr_code_exception_table *exception = &codeAttribute->exception_table[k];
                    fprintf(stdout, "\t\t%d: start_pc: %d, end_pc: %d, "
                                    "handler_pc: %d, catch_type: %d\n", k,
                            be16toh(exception->start_pc),
                            be16toh(exception->end_pc),
                            be16toh(exception->handler_pc),
                            be16toh(exception->catch_type));
                }
            }

        }
    }
}

void print_bootstrap_methods(jclass_file *jc)
{
    fprintf(stdout, "BootstrapMethods:\n");
    jattr_bootstrap_methods  *bootstrap_methods_attr = NULL;
    for (int i = 0; i < be16toh(jc->attributes_count); ++i) {
        jattr info = jc->attributes[i];
        char *name = pool_str(jc, info.name_index);
        if (strcmp(name, "BootstrapMethods") != 0)
            continue;
        bootstrap_methods_attr = (jattr_bootstrap_methods*)info.info;
    }
    if (bootstrap_methods_attr == NULL)
        return;

    for (int i = 0; i < be16toh(bootstrap_methods_attr->num_bootstrap_methods); i++) {
        jclass_bootstrap_method b_method = bootstrap_methods_attr->bootstrap_methods[i];
        jcp_info *_item = pool_item(jc, b_method.bootstrap_method_ref);
        printf("%d: #%d\t// %s\n", i,
               be16toh(b_method.bootstrap_method_ref), _item->readable);
        fprintf(stdout, "\tMethod arguments:\n");
        for (int k = 0; k < be16toh(b_method.num_bootstrap_arguments); ++k) {
            u2 arg_idx = b_method.bootstrap_arguments[k];
            jcp_info _info = jc->constant_pool[be16toh(arg_idx)];
            fprintf(stdout, "\t  #%d cp_info_tag: %d %s\n",
                    be16toh(arg_idx), _info.tag, _info.readable);
        }
    }

}

void print_constant_pool_section(jclass_file* jc)
{
    fprintf(stdout, "Constant pool: \n");
    for (int i = 0; i < be16toh(jc->constant_pool_count) - 1; i++) {
        jcp_info *_info = &jc->constant_pool[i];
        fprintf(stdout, "  #%d = %s %s\n", i+1, _info->name, _info->readable);
        if (_info->tag == CONST_DOUBLE_TAG || _info->tag == CONST_LONG_TAG)
            i++;
    }
}

void print_source_file_attribute(jclass_file* jclass)
{
    jattr_source_file* source_file_attr = NULL;
    for (int i = 0; i < be16toh(jclass->attributes_count); ++i) {
        jattr* _attr = &jclass->attributes[i];
        if (strcmp(_attr->name, "SourceFile") != 0)
            continue;
        source_file_attr = (jattr_source_file*)_attr->info;
    }
    if (source_file_attr == NULL)
        return;
    fprintf(stdout, "SourceFile: %s\n",
            pool_str(jclass, source_file_attr->sourcefile_index));

}

void print_java_class_file_info(jclass_file *jc)
{
    print_constant_pool_section(jc);
    print_methods_section(jc);
    print_bootstrap_methods(jc);
    print_source_file_attribute(jc);
    fprintf(stdout, "%s %s\n", 
            get_class_access_flags_str(jc->access_flags),
            pool_str(jc, jc->this_class));
}

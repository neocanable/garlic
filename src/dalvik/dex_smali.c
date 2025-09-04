#include "parser/dex/metadata.h"
#include "dex_ins.h"
#include "dex_meta_helper.h"
#include "dex_class.h"
#include "dex_method.h"

static FILE* _smali_stream(FILE *out) {
    return out == NULL ? stdout : out;
}

static void smali_method_defination(jd_meta_dex *dex,
                                      encoded_method *m,
                                      dex_code_item *code,
                                      int type,
                                      FILE *stream)
{
    dex_method_id *method_id = &dex->method_ids[m->method_id];
    dex_proto_id *proto_id = &dex->proto_ids[method_id->proto_idx];

    string method_name = dex_str_of_idx(dex, method_id->name_idx);
    string str_return = dex_str_of_type_id(dex, proto_id->return_type_idx);
    string method_type = type == 0 ? "[direct-method]" : "[virtual-method]";

    str_list *list = str_list_init();
    dex_method_access_flag_with_flags(m->access_flags, list);
    string access_flags = str_join(list);

    if (proto_id->parameters_off == 0) {
        fprintf(_smali_stream(stream),
                ".method %s %s()%s\n",
                access_flags,
                method_name,
                str_return);
    } else {
        fprintf(_smali_stream(stream), 
                ".method %s %s(", 
                access_flags, 
                method_name);
        for (int i = 0; i < proto_id->type_list->size; ++i) {
            dex_type_item *type_item = &proto_id->type_list->list[i];
            string type = dex_str_of_type_id(dex, type_item->type_idx);
            fprintf(_smali_stream(stream), "%s", type);
        }
        fprintf(_smali_stream(stream), ")%s\n", str_return);
    }

    fprintf(_smali_stream(stream),
            "\t.registers %d\n\n",
            code->registers_size);
}

static void smali_instruction_header(encoded_method *m,
                                       u1 opcode,
                                       int i,
                                       FILE *stream)
{
    if (opcode == DEX_INS_MOVE ||
        opcode == DEX_INS_MOVE_FROM16 ||
        opcode == DEX_INS_MOVE_16 ||
        opcode == DEX_INS_MOVE_WIDE ||
        opcode == DEX_INS_MOVE_WIDE_FROM16 ||
        opcode == DEX_INS_MOVE_WIDE_16 ||
        opcode == DEX_INS_MOVE_OBJECT ||
        opcode == DEX_INS_MOVE_OBJECT_FROM16 ||
        opcode == DEX_INS_MOVE_OBJECT_16)
        return;

    fprintf(_smali_stream(stream), "\t%s ", dex_opcode_name(opcode));
}

static void smali_write_method(jd_meta_dex *dex,
                                 encoded_method *m,
                                 dex_code_item *code,
                                 int type,
                                 FILE *stream)
{
    smali_method_defination(dex, m, code, type, stream);

    for (int i = 0; i < code->insns_size; ++i) {
        u2 *item = &code->insns[i];
        u1 opcode = *item & 0xFF;


        int len = dex_opcode_len(opcode);
        smali_instruction_header(m, opcode, i, stream);
        switch(opcode) {
            case DEX_INS_NOP: { // nop
                if (*item == 0x0100) {
                    u2 size = code->insns[i+1];
                    u4 first_key = code->insns[i+2] | (code->insns[i+3] << 16);
                    int *targets = x_alloc_in(dex->pool, sizeof(int)*size);
                    for (int j = 0; j < size; ++j) {
                        targets[j] = code->insns[i+4+j] |
                                     (code->insns[i+5+j] << 16);
                    }
                    len = size * 2 + 4;
                    fprintf(_smali_stream(stream),
                            "packed-switch-payload: size=%d, first_key=%d\n",
                            size, first_key);
                }
                else if (*item == 0x0200) {
                    u2 size = code->insns[i+1];
                    int *keys = x_alloc_in(dex->pool, sizeof(int)*size);
                    int *targets = x_alloc_in(dex->pool, sizeof(int)*size);
                    for (int j = 0; j < size; ++j) {
                        keys[j] = code->insns[i+2+j] |
                                  (code->insns[i+3+j] << 16);
                    }
                    for (int j = 0; j < size; ++j) {
                        targets[j] = code->insns[i+2+size+j] |
                                     (code->insns[i+3+size+j] << 16);
                    }
                    len = size * 4 + 2;
                    fprintf(_smali_stream(stream),
                            "sparse-switch-payload: size=%d\n",
                            size);
                }
                else if (*item == 0x0300) {
                    u2 element_size = code->insns[i+1];
                    u2 size = code->insns[i+2];
                    u2 *data = x_alloc_in(dex->pool, sizeof(u2)*size);
                    for (int j = 0; j < size; ++j) {
                        data[j] = code->insns[i+3+j];
                    }
                    fprintf(_smali_stream(stream),
                            "fill-array-data-payload: size=%d",
                            size);

                    len = (size * element_size + 1) / 2 + 4;
                }
                break;
            }
            case DEX_INS_MOVE_RESULT: // move-result
            case DEX_INS_MOVE_RESULT_WIDE: // move-result-wide
            case DEX_INS_MOVE_RESULT_OBJECT: // move-result-object
            case DEX_INS_MOVE_EXCEPTION: { // move-result-exception
                // move-result <T> vAA
                u1 v_a = (*item >> 8);
                fprintf(_smali_stream(stream), "v%d\n", v_a);
                break;
            }
            case DEX_INS_RETURN_VOID: { // return-void
                fprintf(_smali_stream(stream), "\n");
                break;
            }
            case DEX_INS_RETURN: // return vAA
            case DEX_INS_RETURN_WIDE: // return-wide vAA
            case DEX_INS_RETURN_OBJECT: { // return-object vAA
                // return <vAA>
                u1 v_a = (*item >> 8);
                if (opcode == DEX_INS_RETURN) {
                    fprintf(_smali_stream(stream), "v%d\n", v_a);
                } else if (opcode == DEX_INS_RETURN_WIDE) {
                    fprintf(_smali_stream(stream), "v%d\n", v_a);
                } else {
                    fprintf(_smali_stream(stream), "v%d\n", v_a);
                }
                break;
            }
            case DEX_INS_CONST_4: { // const/4
                // const/4 vA, #+B
                s1 v_a = ((s4)*item >> 8) & 0x0F;
                s1 v_b = (s4)*item >> 12;
                fprintf(_smali_stream(stream), "v%d, %d\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_16: { // const/16
                // const/16 vAA, #+BBBB
                u1 v_a = (*item >> 8);
                s2 v_b = code->insns[i+1];
                fprintf(_smali_stream(stream), "v%d, %d\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST: { // const vAA, #+BBBBBBBB
                // const vAA, #+BBBBBBBB
                u1 v_a = (*item >> 8);
                u4 low = code->insns[i+1];
                u4 high = code->insns[i+2];
                u8 v_b = (u8)high << 32 | low;
                fprintf(_smali_stream(stream), "v%d, %lu\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_HIGH16: { // const/high16
                // const/high16 vAA, #+BBBB0000
                u1 v_a = (*item >> 8);
                s2 v_b = code->insns[i + 1];
                v_b = v_b << 16;
                fprintf(_smali_stream(stream), "v%d, %d\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_WIDE_16: { // const-wide/16
                // const-wide/16 vAA, #+BBBB
                u1 v_a = (*item >> 8);
                s2 v_b = code->insns[i+1];
                fprintf(_smali_stream(stream), "v%d, %d\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_WIDE_32: { // const-wide/32
                // const-wide/32 vAA, #+BBBBBBBB
                u1 v_a = (*item >> 8);
                s4 b1 = code->insns[i+1];
                s4 b2 = code->insns[i+2];
                s8 v_b = (s8)b1 << 32 | b2;
                fprintf(_smali_stream(stream), "v%d, %ld\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_WIDE: { // const-wide vAA, #+BBBBBBBBBBBBBBBB
                // const-wide vAA, #+BBBBBBBBBBBBBBBB
                u1 v_a = (*item >> 8);
                s4 b1 = code->insns[i+1];
                s4 b2 = code->insns[i+2];
                s4 b3 = code->insns[i+3];
                s4 b4 = code->insns[i+4];
                s8 v_b = (s8)b1 << 48 | (s8)b2 << 32 | (s8)b3 << 16 | b4;
                fprintf(_smali_stream(stream), "v%d, %ld\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_WIDE_HIGH16: { // const-wide/high16
                // const-wide/high16 vAA, #+BBBB000000000000
                u1 v_a = (*item >> 8);
                s2 b1 = code->insns[i+1];
                s8 v_b = (s8)b1 << 48;
                fprintf(_smali_stream(stream), "v%d, %ld\n", v_a, v_b);
                break;
            }
            case DEX_INS_CONST_STRING: { // const-string
                // const-string vAA, string@BBBB
                u1 v_a = (*item >> 8);
                u2 string_index = code->insns[i+1];
                string str = dex->strings[string_index].data;
                fprintf(_smali_stream(stream), "v%d, \"%s\"\n", v_a, str);
                break;
            }
            case DEX_INS_CONST_STRING_JUMBO: { // const-string/jumbo
                // const-string vAA, string@BBBBBBBB
                u1 v_a = (*item >> 8);
                u2 index1 = code->insns[i+1];
                u2 index2 = code->insns[i+2];
                u4 string_index = ((u4)index2 << 16) | index1;
                string str = dex->strings[string_index].data;
                fprintf(_smali_stream(stream), "v%d, \"%s\"\n", v_a, str);
                break;
            }
            case DEX_INS_CONST_CLASS: { // const-class
                // const-class vAA, type@BBBB
                u1 v_a = (*item >> 8);
                u2 type_index = code->insns[i+1];
                string type_name = dex_str_of_type_id(dex, type_index);
                fprintf(_smali_stream(stream), "v%d, %s\n", v_a, type_name);
                break;
            }
            case DEX_INS_MONITOR_ENTER: // monitor-enter
            case DEX_INS_MONITOR_EXIT: { // monitor-exit
                // monitor-enter vAA
                u1 v_a = (*item >> 8);
                fprintf(_smali_stream(stream), "v%d\n", v_a);
                break;
            }
            case DEX_INS_CHECK_CAST: { // check-cast
                u1 v_a = (*item >> 8);
                u2 type_index = code->insns[i+1];
                string type_name = dex_str_of_type_id(dex, type_index);
                fprintf(_smali_stream(stream), "v%d, %s\n",
                       v_a, type_name);
                break;
            }
            case DEX_INS_INSTANCE_OF: { // instance-of
                // instance-of vA, vB, type@CCCC
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                u2 type_index = code->insns[i+1];
                dex_type_id *type_id = &dex->type_ids[type_index];
                string type_name = dex->strings[type_id->descriptor_idx].data;
                fprintf(_smali_stream(stream), "v%d, v%d, %s\n",
                       v_a, v_b, type_name);
                break;
            }
            case DEX_INS_ARRAY_LENGTH: { // array-length
                // array-length vA, vB
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                fprintf(_smali_stream(stream), "v%d, v%d\n", v_a, v_b);
                break;
            }
            case DEX_INS_NEW_INSTANCE: { // new-instance
                // new-instance vAA, type@BBBB
                u1 v_a = (*item >> 8);
                u2 type_index = code->insns[i+1];
                string tname = dex_str_of_type_id(dex, type_index);
                fprintf(_smali_stream(stream), "v%d, %s\n",
                       v_a, tname);
                break;
            }
            case DEX_INS_NEW_ARRAY: { // new-array
                // new-array vA, vB, type@CCCC
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                u2 type_index = code->insns[i+1];
                fprintf(_smali_stream(stream),
                        "v%d, v%d %d\n",
                        v_a, v_b, type_index);
                break;
            }
            case DEX_INS_FILLED_NEW_ARRAY: { // filled-new-array
                // filled-new-array {vD, vE, vF, vG, vA}, type@CCCC
                u1 v_a = *item >> 12;
                u1 v_g = (*item >> 8) & 0x0F;
                u2 second = code->insns[i+2];
                u1 v_c = second & 0x0F;
                u1 v_d = (second >> 4) & 0x0F;
                u1 v_e = (second >> 8) & 0x0F;
                u1 v_f = second >> 12;
                u2 type_index = code->insns[i+1];
                switch (v_a) {
                    case 0: {
                        fprintf(_smali_stream(stream),
                                "type@%02x\n",
                                type_index);
                        break;
                    }
                    case 1: {
                        fprintf(_smali_stream(stream),
                                "{v%d} type@%02x\n",
                               v_c,
                               type_index);
                        break;
                    }
                    case 2: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d} type@%02x\n",
                               v_c,
                               v_d,
                               type_index);
                        break;
                    }
                    case 3: {
                        fprintf(_smali_stream(stream),
                               "{v%d, v%d, v%d} type@%02x\n",
                               v_c,
                               v_d,
                               v_e,
                               type_index);
                        break;
                    }
                    case 4: {
                        fprintf(_smali_stream(stream),
                                " {v%d, v%d, v%d, v%d} type@%02x\n",
                               v_c,
                               v_d,
                               v_e,
                               v_f,
                               type_index);
                        break;
                    }
                    case 5: {
                        fprintf(_smali_stream(stream),
                                " {v%d, v%d, v%d, v%d, v%d} type@%02x\n",
                               v_c,
                               v_d,
                               v_e,
                               v_f,
                               v_g,
                               type_index);
                        break;
                    }
                    default: {
                        fprintf(_smali_stream(stream),
                                "[instruction] error \n");
                        break;
                    }

                }
                break;
            }
            case DEX_INS_FILLED_NEW_ARRAY_RANGE: { // filled-new-array/range
                u1 v_a = *item >> 8;
                u2 counter = code->insns[i+2];
                u2 type_index = code->insns[i+1];
                u2 count = counter + v_a - 1;

                fprintf(_smali_stream(stream), "{\n");
                for (int j = counter; j < count; ++j)
                    fprintf(_smali_stream(stream), "v%d, ", j);
                fprintf(_smali_stream(stream), "} @%d\n", type_index);
                break;
            }
            case DEX_INS_FILL_ARRAY_DATA: { // fill-array-data
                // fill-array-data vAA, +BBBBBBBB, 31t
                u1 v_a = (*item >> 8);
                u2 array_data0 = code->insns[i+1];
                u2 array_data1 = code->insns[i+2];
                u4 array_data = (u4)array_data0 << 16 | array_data1;
                fprintf(_smali_stream(stream),
                        "v%d, %d\n",
                        v_a,
                        array_data);
                break;
            }
            case DEX_INS_THROW: { // throw
                // throw vAA
                u1 v_a = (*item >> 8);
                fprintf(_smali_stream(stream), "v%d\n", v_a);
                break;
            }
            case DEX_INS_GOTO: { // goto
                // goto +AA
                s1 v_a = *item >> 8;
                fprintf(_smali_stream(stream), "%d\n", v_a);
                break;
            }
            case DEX_INS_GOTO_16: { // goto/16
                // goto/16 +AAAA
                s2 v_a = code->insns[i+1];
                fprintf(_smali_stream(stream), "%d\n", v_a);
                break;
            }
            case DEX_INS_GOTO_32: { // goto/32
                // goto/32 +AAAAAAAA
                s2 jump_index1 = code->insns[i+1];
                s2 jump_index2 = code->insns[i+2];
                s4 v_a = (s4)jump_index1 << 16 | jump_index2;
                fprintf(_smali_stream(stream), "%d\n", v_a);
                break;
            }
            case DEX_INS_PACKED_SWITCH: // packed-switch
            case DEX_INS_SPARSE_SWITCH: { // sparse-switch
                // packed-switch vAA, +BBBBBBBB
                u1 v_a = (*item >> 8);
                s2 low = code->insns[i+1];
                s2 high = code->insns[i+2];
                s4 v_b = (s4)high << 16 | low;
                fprintf(_smali_stream(stream), "v%d %d\n", v_a, v_b);
                break;
            }
            case DEX_INS_CMPL_FLOAT: // cmpl-float
            case DEX_INS_CMPG_FLOAT: // cmpg-float
            case DEX_INS_CMPL_DOUBLE: // cmpl-double
            case DEX_INS_CMPG_DOUBLE: // cmpg-double
            case DEX_INS_CMP_LONG: { // cmpl-long
                // cmpl-float vAA, vBB, vCC
                u1 v_a = (*item >> 8);
                u2 second = code->insns[i+1];
                u1 v_b = second >> 8;
                u1 v_c = second & 0x0F;
                fprintf(_smali_stream(stream),
                        "v%d v%d v%d\n",
                        v_a,
                        v_b,
                        v_c);
                break;
            }
            case DEX_INS_IF_EQ: // if-eq
            case DEX_INS_IF_NE: // if-ne
            case DEX_INS_IF_LT: // if-lt
            case DEX_INS_IF_GE: // if-ge
            case DEX_INS_IF_GT: // if-gt
            case DEX_INS_IF_LE: { // if-le
                // if-eq vA, vB, +CCCC
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                s2 v_c = code->insns[i+1];
                fprintf(_smali_stream(stream),
                        "v%d, v%d => %d\n",
                        v_a,
                        v_b,
                        v_c + i);
                break;
            }
            case DEX_INS_IF_EQZ: // if-eqz
            case DEX_INS_IF_NEZ: // if-nez
            case DEX_INS_IF_LTZ: // if-ltz
            case DEX_INS_IF_GEZ: // if-gez
            case DEX_INS_IF_GTZ: // if-gtz
            case DEX_INS_IF_LEZ: { // if-lez
                u1 v_a = (*item >> 8);
                s2 v_b = code->insns[i+1];
                fprintf(_smali_stream(stream),
                        "v%d => %d\n",
                        v_a,
                        v_b + i);
                break;
            }
            case 0x3E:
            case 0x3F:
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43: {
                break;
            }
            case DEX_INS_AGET:
            case DEX_INS_AGET_WIDE:
            case DEX_INS_AGET_OBJECT:
            case DEX_INS_AGET_BOOLEAN:
            case DEX_INS_AGET_BYTE:
            case DEX_INS_AGET_CHAR:
            case DEX_INS_AGET_SHORT:
            case DEX_INS_APUT:
            case DEX_INS_APUT_WIDE:
            case DEX_INS_APUT_OBJECT:
            case DEX_INS_APUT_BOOLEAN:
            case DEX_INS_APUT_BYTE:
            case DEX_INS_APUT_CHAR:
            case DEX_INS_APUT_SHORT: {
                u1 v_a = (*item >> 8);
                u2 second = code->insns[i+1];
                u1 v_b = second >> 8;
                u1 v_c = second & 0x0F;
                fprintf(_smali_stream(stream),
                        "v%d, v%d, %d\n",
                        v_a,
                        v_b,
                        v_c);
                break;
            }
            case DEX_INS_IGET: // iget
            case DEX_INS_IGET_WIDE: // iget-wide
            case DEX_INS_IGET_OBJECT: // iget-object
            case DEX_INS_IGET_BOOLEAN: // iget-boolean
            case DEX_INS_IGET_BYTE: // iget-byte
            case DEX_INS_IGET_CHAR: // iget-char
            case DEX_INS_IGET_SHORT: // iget-short
            case DEX_INS_IPUT: // iput
            case DEX_INS_IPUT_WIDE: // iput-wide
            case DEX_INS_IPUT_OBJECT: // iput-object
            case DEX_INS_IPUT_BOOLEAN: // iput-boolean
            case DEX_INS_IPUT_BYTE: // iput-byte
            case DEX_INS_IPUT_CHAR: // iput-char
            case DEX_INS_IPUT_SHORT: { // iput-short
                // instance op vA, vB, field@CCCC, 22c
                u1 v_a = (*item >> 8) & 0x0F;
                u1 v_b = *item >> 12;
                u2 field_index = code->insns[i+1];
                dex_field_id *field_id = &dex->field_ids[field_index];
                u2 class_idx = field_id->class_idx;
                u4 name_idx = field_id->name_idx;
                u2 type_idx = field_id->type_idx;
                string field_name = dex_str_of_idx(dex, name_idx);
                string class_name = dex_str_of_type_id(dex, class_idx);
                string type_name = dex_str_of_type_id(dex, type_idx);
                fprintf(_smali_stream(stream),
                        "v%d, v%d, %s->%s:%s\n",
                       v_a,
                       v_b,
                       class_name,
                       field_name,
                       type_name);
                break;
            }
            case DEX_INS_SGET: // sget
            case DEX_INS_SGET_WIDE: // sget-wide
            case DEX_INS_SGET_OBJECT: // sget-object
            case DEX_INS_SGET_BOOLEAN: // sget-boolean
            case DEX_INS_SGET_BYTE: // sget-byte
            case DEX_INS_SGET_CHAR: // sget-char
            case DEX_INS_SGET_SHORT: // sget-short
            case DEX_INS_SPUT: // sput
            case DEX_INS_SPUT_WIDE: // sput-wide
            case DEX_INS_SPUT_OBJECT: // sput-object
            case DEX_INS_SPUT_BOOLEAN: // sput-boolean
            case DEX_INS_SPUT_BYTE: // sput-byte
            case DEX_INS_SPUT_CHAR: // sput-char
            case DEX_INS_SPUT_SHORT: { // sput-short
                // sstatic op vAA, field@BBBB, 21c
                u1 v_a = (*item >> 8) ;
                u2 field_index = code->insns[i+1];
                dex_field_id *field_id = &dex->field_ids[field_index];
                u2 class_idx = field_id->class_idx;
                u4 name_idx = field_id->name_idx;
                u2 type_idx = field_id->type_idx;
                string field_name = dex_str_of_idx(dex, name_idx);
                string class_name = dex_str_of_type_id(dex, class_idx);
                string type_name = dex_str_of_type_id(dex, type_idx);
                fprintf(_smali_stream(stream),
                        "v%d, %s->%s:%s\n",
                        v_a,
                        class_name,
                        field_name,
                        type_name);
                break;
            }
            case DEX_INS_INVOKE_VIRTUAL: // invoke-virtual
            case DEX_INS_INVOKE_SUPER: // invoke-super
            case DEX_INS_INVOKE_DIRECT: // invoke-direct
            case DEX_INS_INVOKE_STATIC: // invoke-static
            case DEX_INS_INVOKE_INTERFACE: { // invoke-interface
                u1 v_a = *item >> 12;
                u1 v_g = (*item >> 8) & 0x0F;
                u2 second = code->insns[i+2]; // F|E|D|C // 0x0010
                u1 v_c = second & 0x0F;
                u1 v_d = (second >> 4) & 0x0F;
                u1 v_e = (second >> 8) & 0x0F;
                u1 v_f = second >> 12;

                u2 method_index = code->insns[i+1];
                u2 proto_idx = dex->method_ids[method_index].proto_idx;
                u2 class_idx = dex->method_ids[method_index].class_idx;
                u4 name_idx = dex->method_ids[method_index].name_idx;

                string cname = dex_str_of_type_id(dex, class_idx);
                string name = dex_str_of_idx(dex, name_idx);

                dex_proto_id *proto = &dex->proto_ids[proto_idx];
                u4 ret_idx = proto->return_type_idx;
                string return_str = dex_str_of_type_id(dex, ret_idx);
                dex_type_list *type_list = proto->type_list;
                switch (v_a) {
                    case 0: {
                        fprintf(_smali_stream(stream), "{}, ");
                        break;
                    }
                    case 1: {
                        fprintf(_smali_stream(stream),
                                "{v%d}, ",
                                v_c);
                        break;
                    }
                    case 2: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d}, ",
                                v_c,
                                v_d);
                        break;
                    }
                    case 3: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d, v%d}, ",
                                v_c,
                                v_d,
                                v_e);
                        break;
                    }
                    case 4: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d, v%d, v%d}, ",
                                v_c,
                                v_d,
                                v_e,
                                v_f);
                        break;
                    }
                    case 5: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d, v%d, v%d, v%d}, ",
                                v_c,
                                v_d,
                                v_e,
                                v_f,
                                v_g);
                        break;
                    }
                    default: {
                        fprintf(_smali_stream(stream),
                                "[instruction] error at invoke-kind\n");
                        break;
                    }

                }

                fprintf(_smali_stream(stream), "%s->%s(", cname, name);
                if (type_list != NULL) {
                    for (int j = 0; j < type_list->size; ++j) {
                        dex_type_item *item = &type_list->list[j];
                        string desc = dex_str_of_type_id(dex, item->type_idx);
                        fprintf(_smali_stream(stream), "%s", desc);
                    }
                }
                fprintf(_smali_stream(stream), ")%s\n", return_str);

                break;
            }
            case 0x73: {
                break;
            }
            case DEX_INS_INVOKE_VIRTUAL_RANGE:
            case DEX_INS_INVOKE_SUPER_RANGE:
            case DEX_INS_INVOKE_DIRECT_RANGE:
            case DEX_INS_INVOKE_STATIC_RANGE:
            case DEX_INS_INVOKE_INTERFACE_RANGE: {
                u1 v_a = (*item >> 8);
                u2 method_index = code->insns[i+1];
                u2 proto_idx = dex->method_ids[method_index].proto_idx;
                u2 class_idx = dex->method_ids[method_index].class_idx;
                u4 name_idx = dex->method_ids[method_index].name_idx;

                string cname = dex_str_of_type_id(dex, class_idx);
                string name = dex_str_of_idx(dex, name_idx);

                dex_proto_id *proto = &dex->proto_ids[proto_idx];
                string rstr = dex_str_of_type_id(dex, proto->return_type_idx);
                dex_type_list *type_list = proto->type_list;


                u2 start_index = code->insns[i+2];
                u2 count = start_index + v_a - 1;
                fprintf(_smali_stream(stream), "{");
                for (int j = start_index; j <= count; ++j) {
                    fprintf(_smali_stream(stream), "v%d", j);
                    if (j < count)
                        fprintf(_smali_stream(stream), ", ");

                }
                fprintf(_smali_stream(stream), "},");
                fprintf(_smali_stream(stream), "%s->%s(", cname, name);
                if (type_list != NULL) {
                    for (int j = 0; j < type_list->size; ++j) {
                        u2 type_idx = type_list->list[j].type_idx;
                        string desc = dex_str_of_type_id(dex,type_idx);
                        fprintf(_smali_stream(stream), "%s", desc);
                    }
                }
                fprintf(_smali_stream(stream),
                        ")%s # method@%02x\n",
                        rstr,
                        method_index);
                break;
            }
            case 0x79:
            case 0x7A: {
                fprintf(_smali_stream(stream),
                        "[instruction opcode] not used\n");
                break;
            }
            case DEX_INS_NEG_INT:
            case DEX_INS_NOT_INT:
            case DEX_INS_NEG_LONG:
            case DEX_INS_NOT_LONG:
            case DEX_INS_NEG_FLOAT:
            case DEX_INS_NEG_DOUBLE:
            case DEX_INS_INT_TO_LONG:
            case DEX_INS_INT_TO_FLOAT:
            case DEX_INS_INT_TO_DOUBLE:
            case DEX_INS_LONG_TO_INT:
            case DEX_INS_LONG_TO_FLOAT:
            case DEX_INS_LONG_TO_DOUBLE:
            case DEX_INS_FLOAT_TO_INT:
            case DEX_INS_FLOAT_TO_LONG:
            case DEX_INS_FLOAT_TO_DOUBLE:
            case DEX_INS_DOUBLE_TO_INT:
            case DEX_INS_DOUBLE_TO_LONG:
            case DEX_INS_DOUBLE_TO_FLOAT:
            case DEX_INS_INT_TO_BYTE:
            case DEX_INS_INT_TO_CHAR:
            case DEX_INS_INT_TO_SHORT: {
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                fprintf(_smali_stream(stream), "v%d, v%d\n", v_a, v_b);
                break;
            }
            case DEX_INS_ADD_INT:
            case DEX_INS_SUB_INT:
            case DEX_INS_MUL_INT:
            case DEX_INS_DIV_INT:
            case DEX_INS_REM_INT:
            case DEX_INS_AND_INT:
            case DEX_INS_OR_INT:
            case DEX_INS_XOR_INT:
            case DEX_INS_SHL_INT:
            case DEX_INS_SHR_INT:
            case DEX_INS_USHR_INT:
            case DEX_INS_ADD_LONG:
            case DEX_INS_SUB_LONG:
            case DEX_INS_MUL_LONG:
            case DEX_INS_DIV_LONG:
            case DEX_INS_REM_LONG:
            case DEX_INS_AND_LONG:
            case DEX_INS_OR_LONG:
            case DEX_INS_XOR_LONG:
            case DEX_INS_SHL_LONG:
            case DEX_INS_SHR_LONG:
            case DEX_INS_USHR_LONG:
            case DEX_INS_ADD_FLOAT:
            case DEX_INS_SUB_FLOAT:
            case DEX_INS_MUL_FLOAT:
            case DEX_INS_DIV_FLOAT:
            case DEX_INS_REM_FLOAT:
            case DEX_INS_ADD_DOUBLE:
            case DEX_INS_SUB_DOUBLE:
            case DEX_INS_MUL_DOUBLE:
            case DEX_INS_DIV_DOUBLE:
            case DEX_INS_REM_DOUBLE: {
                u1 v_a = (*item >> 8);
                u2 second = code->insns[i+1];
                u1 v_b = second & 0xFF;
                u1 v_c = second >> 8;
                fprintf(_smali_stream(stream),
                        "v%d, v%d, v%d\n",
                        v_a,
                        v_b,
                        v_c);
                break;
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
                // 12x
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                fprintf(_smali_stream(stream), "v%d, v%d\n", v_a, v_b);
                break;
            }
            case DEX_INS_ADD_INT_LIT16:
            case DEX_INS_RSUB_INT:
            case DEX_INS_MUL_INT_LIT16:
            case DEX_INS_DIV_INT_LIT16:
            case DEX_INS_REM_INT_LIT16:
            case DEX_INS_AND_INT_LIT16:
            case DEX_INS_OR_INT_LIT16:
            case DEX_INS_XOR_INT_LIT16: {
                u1 v_a = *item >> 12;
                u1 v_b = (*item >> 8) & 0x0F;
                s2 v_c = code->insns[i+1];
                fprintf(_smali_stream(stream),
                        "v%d, v%d, %d\n",
                        v_a,
                        v_b,
                        v_c);
                break;
            }
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
                u1 v_a = (*item >> 8);
                s2 second = code->insns[i+1];
                s1 v_c = second >> 8;
                s1 v_b = second & 0x0F;
                fprintf(_smali_stream(stream),
                        "v%d v%d %d\n",
                        v_a,
                        v_b,
                        v_c);
                break;
            }
            case DEX_INS_INVOKE_POLYMORPHIC:
            {
                // TODO: invoke-polymorphic
                u1 v_a = *item >> 12;
                u1 v_g = (*item >> 8) & 0x0F;
                u2 v_bbbb = code->insns[i+1];
                u2 v_hhhh = code->insns[i+3];
                u2 second = code->insns[i+2];
                u1 v_c = second >> 12;
                u1 v_d = (second >> 8) & 0x0F;
                u1 v_e = (second >> 4) & 0x0F;
                u1 v_f = second & 0x0F;

                switch (v_a) {
                    case 1: {
                        fprintf(_smali_stream(stream),
                                "{v%d} %d, %d\n", v_c, v_bbbb, v_hhhh);
                        break;
                    }
                    case 2: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d} %d, %d\n",
                                v_c, v_d, v_bbbb, v_hhhh);
                        break;
                    }
                    case 3: {
                        fprintf(_smali_stream(stream),
                               "{v%d, v%d, v%d} %d, %d\n",
                               v_c, v_d, v_e, v_bbbb, v_hhhh);
                        break;
                    }
                    case 4: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d, v%d, v%d} %d, %d\n",
                               v_c, v_d, v_e, v_f, v_bbbb, v_hhhh);
                        break;
                    }
                    case 5: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d, v%d, v%d, v%d} %d, %d\n",
                                v_c, v_d, v_e, v_f, v_g, v_bbbb, v_hhhh);
                        break;
                    }
                    default: {
                        fprintf(_smali_stream(stream),
                                "error at invoke-kind\n");
                        break;
                    }
                }
                break;
            }
            case DEX_INS_INVOKE_POLYMORPHIC_RANGE: {
                u1 v_a = (*item >> 8);
                u2 v_bbbb = code->insns[i+1];
                u2 v_hhhh = code->insns[i+3];
                u2 v_cccc = code->insns[i+2];
                u2 count = v_cccc + v_a - 1;
                fprintf(_smali_stream(stream), "{");
                for (int j = v_cccc; j <= count; ++j)
                    fprintf(_smali_stream(stream), "v%d, ", j);
                
                fprintf(_smali_stream(stream),
                        "} %d, %d\n",
                        v_bbbb,
                        v_hhhh);
                break;
            }
            case DEX_INS_INVOKE_CUSTOM: {
                u1 v_a = *item >> 12;
                u1 v_g = (*item >> 8) & 0x0F;
                u2 second = code->insns[i+2];
                u1 v_c = second & 0x0F;
                u1 v_d = (second >> 4) & 0x0F;
                u1 v_e = (second >> 8) & 0x0F;
                u1 v_f = second >> 12;

                u2 method_index = code->insns[i+1];
                switch (v_a) {
                    case 0: {
                        fprintf(_smali_stream(stream),
                                "call_site@%02x\n",
                               method_index);
                        break;
                    }
                    case 1: {
                        fprintf(_smali_stream(stream),
                                "{v%d} call_site@%02x\n",
                                v_c,
                                method_index);
                        break;
                    }
                    case 2: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d}, call_site@%02x\n",
                                v_c,
                                v_d,
                                method_index);
                        break;
                    }
                    case 3: {
                        fprintf(_smali_stream(stream),
                                "{v%d, v%d, v%d}, call_site@%02x\n",
                                v_c,
                                v_d,
                                v_e,
                                method_index);
                        break;
                    }
                    case 4: {
                        fprintf(_smali_stream(stream),
                               "{v%d, v%d, v%d, v%d}, call_site@%02x\n",
                               v_c, v_d, v_e, v_f, method_index);
                        break;
                    }
                    case 5: {
                        fprintf(_smali_stream(stream),
                               "{v%d, v%d, v%d, v%d, v%d}, call_site@%02x\n",
                               v_c, v_d, v_e, v_f, v_g, method_index);
                        break;
                    }
                    default: {
                        fprintf(_smali_stream(stream),
                                "[instruction] error at invoke\n");
                        break;
                    }

                }
                break;
            }
            case DEX_INS_INVOKE_CUSTOM_RANGE: {
                // 3rc
                u1 v_a = *item >> 8;
                u2 v_bbbb = code->insns[i+1];
                u2 v_cccc = code->insns[i+2];
                u2 count = v_cccc + v_a - 1;
                fprintf(_smali_stream(stream), "{");
                for (int j = v_cccc; j <= count; ++j) {
                    fprintf(_smali_stream(stream), "v%d, ", j);
                }
                fprintf(_smali_stream(stream),
                        "}, call_site@%d\n",
                        v_bbbb);
                break;
            }
            case DEX_INS_CONST_METHOD_HANDLE:
            case DEX_INS_CONST_METHOD_TYPE: {
                // TODO: const-m-type
                u1 v_a = *item >> 8;
                u2 v_bbbb = code->insns[i+1];
                fprintf(_smali_stream(stream), "v%d, %d\n", v_a, v_bbbb);
                break;
            }
        }
        i += (len - 1);
    }
    fprintf(_smali_stream(stream), ".end method\n\n");
}


static void smali_write_class_fields(jd_meta_dex *dex,
                                       dex_class *cf,
                                       FILE *stream)
{
    dex_class_data_item *item = cf->class_data;
    if (item == NULL) return;
    encoded_field *efield;
    if (item->static_fields_size > 0) {
        fprintf(_smali_stream(stream), "#static fields\n");
        for (int i = 0; i < item->static_fields_size; ++i) {
            efield = &item->static_fields[i];
            string field_name = dex_field_name(dex, efield);
            string desc = dex_field_desc(dex, efield);
            str_list *list = str_list_init();
            dex_field_access_flag_with_flags(efield->access_flags, list);
            string flags = str_join(list);
            fprintf(_smali_stream(stream),
                    ".field %s %s:%s\n",
                    flags,
                    field_name,
                    desc);

        }
    }

    if (item->instance_fields_size > 0) {
        fprintf(_smali_stream(stream), "#instance-fields\n");
        for (int i = 0; i < item->instance_fields_size; ++i) {
            efield = &item->instance_fields[i];
            string field_name = dex_field_name(dex, efield);
            string desc = dex_field_desc(dex, efield);
            str_list *list = str_list_init();
            dex_field_access_flag_with_flags(efield->access_flags, list);
            string flags = str_join(list);
            fprintf(_smali_stream(stream),
                    ".field %s %s:%s\n", flags, field_name, desc);

        }
    }
    fprintf(_smali_stream(stream), "\n");
}


static void smali_write_class_def(jd_meta_dex *dex,
                                  dex_class *cf,
                                  FILE *stream)
{
    string class_name = dex_str_of_type_id(dex, cf->class_idx);
    string super_name = dex_str_of_type_id(dex, cf->superclass_idx);

    str_list *list = str_list_init();
    dex_class_access_flag_with_flags(cf->access_flags, list);
    string cf_access_flags = str_join(list);
    fprintf(_smali_stream(stream),
            ".class %s %s\n", cf_access_flags, class_name);
    fprintf(_smali_stream(stream), ".super %s\n", super_name);
    fprintf(_smali_stream(stream), ".source \"%s\"\n", "SourceFile");

    if (cf->interfaces != NULL) {
        fprintf(_smali_stream(stream), "implements ");
        for (int i = 0; i < cf->interfaces->size; ++i) {
            dex_type_item *item = &cf->interfaces->list[i];
            string name = dex_str_of_type_id(dex, item->type_idx);
            fprintf(_smali_stream(stream), "%s ",name);
        }
    }
    fprintf(_smali_stream(stream), "\n");

    smali_write_class_fields(dex, cf, stream);
}

void dex_class_def_to_smali(jd_meta_dex *dex, dex_class_def *cf, FILE *stream)
{
    dex_class_data_item *class_data = cf->class_data;
    smali_write_class_def(dex, cf, stream);
    if (class_data == NULL)
        return;

    for (int j = 0; j < class_data->direct_methods_size; ++j) {
        encoded_method *m = &class_data->direct_methods[j];
        dex_code_item *code = m->code;
        if (code == NULL)
            continue;
        smali_write_method(dex, m, code, 0, stream);
    }

    for (int j = 0; j < class_data->virtual_methods_size; ++j) {
        encoded_method *m = &class_data->virtual_methods[j];
        dex_code_item *code = m->code;
        if (code == NULL)
            continue;
        smali_write_method(dex, m, code, 1, stream);
    }
}

void dex_to_smali(string path)
{
    mem_init_pool();
    jd_meta_dex *dex = parse_dex_file(path);

    dex_header *header = dex->header;
    for (int i = 0; i < header->class_defs_size; ++i) {
        dex_class *cf = &dex->class_defs[i];
        dex_class_def_to_smali(dex, cf, NULL);
    }
    mem_free_pool();
}

#ifndef GARLIC_DEX_INS_HELPER_H
#define GARLIC_DEX_INS_HELPER_H

#include "dex_structure.h"

#define DEX_INS_NOP                              0x00
#define DEX_INS_MOVE                             0x01
#define DEX_INS_MOVE_FROM16                      0x02
#define DEX_INS_MOVE_16                          0x03
#define DEX_INS_MOVE_WIDE                        0x04
#define DEX_INS_MOVE_WIDE_FROM16                 0x05
#define DEX_INS_MOVE_WIDE_16                     0x06
#define DEX_INS_MOVE_OBJECT                      0x07
#define DEX_INS_MOVE_OBJECT_FROM16               0x08
#define DEX_INS_MOVE_OBJECT_16                   0x09
#define DEX_INS_MOVE_RESULT                      0x0A
#define DEX_INS_MOVE_RESULT_WIDE                 0x0B
#define DEX_INS_MOVE_RESULT_OBJECT               0x0C
#define DEX_INS_MOVE_EXCEPTION                   0x0D
#define DEX_INS_RETURN_VOID                      0x0E
#define DEX_INS_RETURN                           0x0F
#define DEX_INS_RETURN_WIDE                      0x10
#define DEX_INS_RETURN_OBJECT                    0x11
#define DEX_INS_CONST_4                          0x12
#define DEX_INS_CONST_16                         0x13
#define DEX_INS_CONST                            0x14
#define DEX_INS_CONST_HIGH16                     0x15
#define DEX_INS_CONST_WIDE_16                    0x16
#define DEX_INS_CONST_WIDE_32                    0x17
#define DEX_INS_CONST_WIDE                       0x18
#define DEX_INS_CONST_WIDE_HIGH16                0x19
#define DEX_INS_CONST_STRING                     0x1A
#define DEX_INS_CONST_STRING_JUMBO               0x1B
#define DEX_INS_CONST_CLASS                      0x1C
#define DEX_INS_MONITOR_ENTER                    0x1D
#define DEX_INS_MONITOR_EXIT                     0x1E
#define DEX_INS_CHECK_CAST                       0x1F
#define DEX_INS_INSTANCE_OF                      0x20
#define DEX_INS_ARRAY_LENGTH                     0x21
#define DEX_INS_NEW_INSTANCE                     0x22
#define DEX_INS_NEW_ARRAY                        0x23
#define DEX_INS_FILLED_NEW_ARRAY                 0x24
#define DEX_INS_FILLED_NEW_ARRAY_RANGE           0x25
#define DEX_INS_FILL_ARRAY_DATA                  0x26
#define DEX_INS_THROW                            0x27
#define DEX_INS_GOTO                             0x28
#define DEX_INS_GOTO_16                          0x29
#define DEX_INS_GOTO_32                          0x2A
#define DEX_INS_PACKED_SWITCH                    0x2B
#define DEX_INS_SPARSE_SWITCH                    0x2C
#define DEX_INS_CMPL_FLOAT                       0x2D
#define DEX_INS_CMPG_FLOAT                       0x2E
#define DEX_INS_CMPL_DOUBLE                      0x2F
#define DEX_INS_CMPG_DOUBLE                      0x30
#define DEX_INS_CMP_LONG                         0x31
#define DEX_INS_IF_EQ                            0x32
#define DEX_INS_IF_NE                            0x33
#define DEX_INS_IF_LT                            0x34
#define DEX_INS_IF_GE                            0x35
#define DEX_INS_IF_GT                            0x36
#define DEX_INS_IF_LE                            0x37
#define DEX_INS_IF_EQZ                           0x38
#define DEX_INS_IF_NEZ                           0x39
#define DEX_INS_IF_LTZ                           0x3A
#define DEX_INS_IF_GEZ                           0x3B
#define DEX_INS_IF_GTZ                           0x3C
#define DEX_INS_IF_LEZ                           0x3D
#define DEX_INS_AGET                             0x44
#define DEX_INS_AGET_WIDE                        0x45
#define DEX_INS_AGET_OBJECT                      0x46
#define DEX_INS_AGET_BOOLEAN                     0x47
#define DEX_INS_AGET_BYTE                        0x48
#define DEX_INS_AGET_CHAR                        0x49
#define DEX_INS_AGET_SHORT                       0x4A
#define DEX_INS_APUT                             0x4B
#define DEX_INS_APUT_WIDE                        0x4C
#define DEX_INS_APUT_OBJECT                      0x4D
#define DEX_INS_APUT_BOOLEAN                     0x4E
#define DEX_INS_APUT_BYTE                        0x4F
#define DEX_INS_APUT_CHAR                        0x50
#define DEX_INS_APUT_SHORT                       0x51
#define DEX_INS_IGET                             0x52
#define DEX_INS_IGET_WIDE                        0x53
#define DEX_INS_IGET_OBJECT                      0x54
#define DEX_INS_IGET_BOOLEAN                     0x55
#define DEX_INS_IGET_BYTE                        0x56
#define DEX_INS_IGET_CHAR                        0x57
#define DEX_INS_IGET_SHORT                       0x58
#define DEX_INS_IPUT                             0x59
#define DEX_INS_IPUT_WIDE                        0x5A
#define DEX_INS_IPUT_OBJECT                      0x5B
#define DEX_INS_IPUT_BOOLEAN                     0x5C
#define DEX_INS_IPUT_BYTE                        0x5D
#define DEX_INS_IPUT_CHAR                        0x5E
#define DEX_INS_IPUT_SHORT                       0x5F
#define DEX_INS_SGET                             0x60
#define DEX_INS_SGET_WIDE                        0x61
#define DEX_INS_SGET_OBJECT                      0x62
#define DEX_INS_SGET_BOOLEAN                     0x63
#define DEX_INS_SGET_BYTE                        0x64
#define DEX_INS_SGET_CHAR                        0x65
#define DEX_INS_SGET_SHORT                       0x66
#define DEX_INS_SPUT                             0x67
#define DEX_INS_SPUT_WIDE                        0x68
#define DEX_INS_SPUT_OBJECT                      0x69
#define DEX_INS_SPUT_BOOLEAN                     0x6A
#define DEX_INS_SPUT_BYTE                        0x6B
#define DEX_INS_SPUT_CHAR                        0x6C
#define DEX_INS_SPUT_SHORT                       0x6D
#define DEX_INS_INVOKE_VIRTUAL                   0x6E
#define DEX_INS_INVOKE_SUPER                     0x6F
#define DEX_INS_INVOKE_DIRECT                    0x70
#define DEX_INS_INVOKE_STATIC                    0x71
#define DEX_INS_INVOKE_INTERFACE                 0x72
#define DEX_INS_INVOKE_VIRTUAL_RANGE             0x74
#define DEX_INS_INVOKE_SUPER_RANGE               0x75
#define DEX_INS_INVOKE_DIRECT_RANGE              0x76
#define DEX_INS_INVOKE_STATIC_RANGE              0x77
#define DEX_INS_INVOKE_INTERFACE_RANGE           0x78
#define DEX_INS_NEG_INT                          0x7B
#define DEX_INS_NOT_INT                          0x7C
#define DEX_INS_NEG_LONG                         0x7D
#define DEX_INS_NOT_LONG                         0x7E
#define DEX_INS_NEG_FLOAT                        0x7F
#define DEX_INS_NEG_DOUBLE                       0x80
#define DEX_INS_INT_TO_LONG                      0x81
#define DEX_INS_INT_TO_FLOAT                     0x82
#define DEX_INS_INT_TO_DOUBLE                    0x83
#define DEX_INS_LONG_TO_INT                      0x84
#define DEX_INS_LONG_TO_FLOAT                    0x85
#define DEX_INS_LONG_TO_DOUBLE                   0x86
#define DEX_INS_FLOAT_TO_INT                     0x87
#define DEX_INS_FLOAT_TO_LONG                    0x88
#define DEX_INS_FLOAT_TO_DOUBLE                  0x89
#define DEX_INS_DOUBLE_TO_INT                    0x8A
#define DEX_INS_DOUBLE_TO_LONG                   0x8B
#define DEX_INS_DOUBLE_TO_FLOAT                  0x8C
#define DEX_INS_INT_TO_BYTE                      0x8D
#define DEX_INS_INT_TO_CHAR                      0x8E
#define DEX_INS_INT_TO_SHORT                     0x8F
#define DEX_INS_ADD_INT                          0x90
#define DEX_INS_SUB_INT                          0x91
#define DEX_INS_MUL_INT                          0x92
#define DEX_INS_DIV_INT                          0x93
#define DEX_INS_REM_INT                          0x94
#define DEX_INS_AND_INT                          0x95
#define DEX_INS_OR_INT                           0x96
#define DEX_INS_XOR_INT                          0x97
#define DEX_INS_SHL_INT                          0x98
#define DEX_INS_SHR_INT                          0x99
#define DEX_INS_USHR_INT                         0x9A
#define DEX_INS_ADD_LONG                         0x9B
#define DEX_INS_SUB_LONG                         0x9C
#define DEX_INS_MUL_LONG                         0x9D
#define DEX_INS_DIV_LONG                         0x9E
#define DEX_INS_REM_LONG                         0x9F
#define DEX_INS_AND_LONG                         0xA0
#define DEX_INS_OR_LONG                          0xA1
#define DEX_INS_XOR_LONG                         0xA2
#define DEX_INS_SHL_LONG                         0xA3
#define DEX_INS_SHR_LONG                         0xA4
#define DEX_INS_USHR_LONG                        0xA5
#define DEX_INS_ADD_FLOAT                        0xA6
#define DEX_INS_SUB_FLOAT                        0xA7
#define DEX_INS_MUL_FLOAT                        0xA8
#define DEX_INS_DIV_FLOAT                        0xA9
#define DEX_INS_REM_FLOAT                        0xAA
#define DEX_INS_ADD_DOUBLE                       0xAB
#define DEX_INS_SUB_DOUBLE                       0xAC
#define DEX_INS_MUL_DOUBLE                       0xAD
#define DEX_INS_DIV_DOUBLE                       0xAE
#define DEX_INS_REM_DOUBLE                       0xAF
#define DEX_INS_ADD_INT_2ADDR                    0xB0
#define DEX_INS_SUB_INT_2ADDR                    0xB1
#define DEX_INS_MUL_INT_2ADDR                    0xB2
#define DEX_INS_DIV_INT_2ADDR                    0xB3
#define DEX_INS_REM_INT_2ADDR                    0xB4
#define DEX_INS_AND_INT_2ADDR                    0xB5
#define DEX_INS_OR_INT_2ADDR                     0xB6
#define DEX_INS_XOR_INT_2ADDR                    0xB7
#define DEX_INS_SHL_INT_2ADDR                    0xB8
#define DEX_INS_SHR_INT_2ADDR                    0xB9
#define DEX_INS_USHR_INT_2ADDR                   0xBA
#define DEX_INS_ADD_LONG_2ADDR                   0xBB
#define DEX_INS_SUB_LONG_2ADDR                   0xBC
#define DEX_INS_MUL_LONG_2ADDR                   0xBD
#define DEX_INS_DIV_LONG_2ADDR                   0xBE
#define DEX_INS_REM_LONG_2ADDR                   0xBF
#define DEX_INS_AND_LONG_2ADDR                   0xC0
#define DEX_INS_OR_LONG_2ADDR                    0xC1
#define DEX_INS_XOR_LONG_2ADDR                   0xC2
#define DEX_INS_SHL_LONG_2ADDR                   0xC3
#define DEX_INS_SHR_LONG_2ADDR                   0xC4
#define DEX_INS_USHR_LONG_2ADDR                  0xC5
#define DEX_INS_ADD_FLOAT_2ADDR                  0xC6
#define DEX_INS_SUB_FLOAT_2ADDR                  0xC7
#define DEX_INS_MUL_FLOAT_2ADDR                  0xC8
#define DEX_INS_DIV_FLOAT_2ADDR                  0xC9
#define DEX_INS_REM_FLOAT_2ADDR                  0xCA
#define DEX_INS_ADD_DOUBLE_2ADDR                 0xCB
#define DEX_INS_SUB_DOUBLE_2ADDR                 0xCC
#define DEX_INS_MUL_DOUBLE_2ADDR                 0xCD
#define DEX_INS_DIV_DOUBLE_2ADDR                 0xCE
#define DEX_INS_REM_DOUBLE_2ADDR                 0xCF
#define DEX_INS_ADD_INT_LIT16                    0xD0
#define DEX_INS_RSUB_INT                         0xD1
#define DEX_INS_MUL_INT_LIT16                    0xD2
#define DEX_INS_DIV_INT_LIT16                    0xD3
#define DEX_INS_REM_INT_LIT16                    0xD4
#define DEX_INS_AND_INT_LIT16                    0xD5
#define DEX_INS_OR_INT_LIT16                     0xD6
#define DEX_INS_XOR_INT_LIT16                    0xD7
#define DEX_INS_ADD_INT_LIT8                     0xD8
#define DEX_INS_RSUB_INT_LIT8                    0xD9
#define DEX_INS_MUL_INT_LIT8                     0xDA
#define DEX_INS_DIV_INT_LIT8                     0xDB
#define DEX_INS_REM_INT_LIT8                     0xDC
#define DEX_INS_AND_INT_LIT8                     0xDD
#define DEX_INS_OR_INT_LIT8                      0xDE
#define DEX_INS_XOR_INT_LIT8                     0xDF
#define DEX_INS_SHL_INT_LIT8                     0xE0
#define DEX_INS_SHR_INT_LIT8                     0xE1
#define DEX_INS_USHR_INT_LIT8                    0xE2
#define DEX_INS_INVOKE_POLYMORPHIC               0xFA
#define DEX_INS_INVOKE_POLYMORPHIC_RANGE         0xFB
#define DEX_INS_INVOKE_CUSTOM                    0xFC
#define DEX_INS_INVOKE_CUSTOM_RANGE              0xFD
#define DEX_INS_CONST_METHOD_HANDLE              0xFE
#define DEX_INS_CONST_METHOD_TYPE                0xFF

// define custome code
#define DEX_INS_COPY_BASIC_BLOCK                 0xE3
#define DEX_INS_COPY_BASIC_BLOCK_GOTO            0xE4


#define DEF_DEX_INS_METHOD(opcode, code_name)           \
    static inline bool                                  \
    dex_ins_is_##code_name(jd_dex_ins *ins)             \
    {                                                   \
        return ins->code == opcode;                     \
    }

DEF_DEX_INS_METHOD(DEX_INS_NOP,                     nop)
DEF_DEX_INS_METHOD(DEX_INS_MOVE,                    move)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_FROM16,             move_from16)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_16,                 move_16)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_WIDE,               move_wide)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_WIDE_FROM16,        move_wide_from16)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_WIDE_16,            move_wide_16)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_OBJECT,             move_object)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_OBJECT_FROM16,      move_object_from16)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_OBJECT_16,          move_object_16)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_RESULT,             move_result)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_RESULT_WIDE,        move_result_wide)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_RESULT_OBJECT,      move_result_object)
DEF_DEX_INS_METHOD(DEX_INS_MOVE_EXCEPTION,          move_exception)
DEF_DEX_INS_METHOD(DEX_INS_RETURN_VOID,             return_void)
DEF_DEX_INS_METHOD(DEX_INS_RETURN,                  return)
DEF_DEX_INS_METHOD(DEX_INS_RETURN_WIDE,             return_wide)
DEF_DEX_INS_METHOD(DEX_INS_RETURN_OBJECT,           return_object)
DEF_DEX_INS_METHOD(DEX_INS_CONST_4, const_4)
DEF_DEX_INS_METHOD(DEX_INS_CONST_16, const_16)
DEF_DEX_INS_METHOD(DEX_INS_CONST,                   const)
DEF_DEX_INS_METHOD(DEX_INS_CONST_HIGH16,            const_high16)
DEF_DEX_INS_METHOD(DEX_INS_CONST_WIDE_16,           const_wide_16)
DEF_DEX_INS_METHOD(DEX_INS_CONST_WIDE_32,           const_wide_32)
DEF_DEX_INS_METHOD(DEX_INS_CONST_WIDE,              const_wide)
DEF_DEX_INS_METHOD(DEX_INS_CONST_WIDE_HIGH16,       const_wide_high16)
DEF_DEX_INS_METHOD(DEX_INS_CONST_STRING,            const_string)
DEF_DEX_INS_METHOD(DEX_INS_CONST_STRING_JUMBO,      const_string_jumbo)
DEF_DEX_INS_METHOD(DEX_INS_CONST_CLASS,             const_class)
DEF_DEX_INS_METHOD(DEX_INS_MONITOR_ENTER,           monitor_enter)
DEF_DEX_INS_METHOD(DEX_INS_MONITOR_EXIT,            monitor_exit)
DEF_DEX_INS_METHOD(DEX_INS_CHECK_CAST,              check_cast)
DEF_DEX_INS_METHOD(DEX_INS_INSTANCE_OF,             instance_of)
DEF_DEX_INS_METHOD(DEX_INS_ARRAY_LENGTH,            array_length)
DEF_DEX_INS_METHOD(DEX_INS_NEW_INSTANCE,            new_instance)
DEF_DEX_INS_METHOD(DEX_INS_NEW_ARRAY,               new_array)
DEF_DEX_INS_METHOD(DEX_INS_FILLED_NEW_ARRAY,        filled_new_array)
DEF_DEX_INS_METHOD(DEX_INS_FILLED_NEW_ARRAY_RANGE,  filled_new_array_range)
DEF_DEX_INS_METHOD(DEX_INS_FILL_ARRAY_DATA,         fill_array_data)
DEF_DEX_INS_METHOD(DEX_INS_THROW,                   throw)
DEF_DEX_INS_METHOD(DEX_INS_GOTO,                    goto)
DEF_DEX_INS_METHOD(DEX_INS_GOTO_16, goto_16)
DEF_DEX_INS_METHOD(DEX_INS_GOTO_32, goto_32)
DEF_DEX_INS_METHOD(DEX_INS_PACKED_SWITCH,           packed_switch)
DEF_DEX_INS_METHOD(DEX_INS_SPARSE_SWITCH,           sparse_switch)
DEF_DEX_INS_METHOD(DEX_INS_CMPL_FLOAT,              cmpl_float)
DEF_DEX_INS_METHOD(DEX_INS_CMPG_FLOAT,              cmpg_float)
DEF_DEX_INS_METHOD(DEX_INS_CMPL_DOUBLE,             cmpl_double)
DEF_DEX_INS_METHOD(DEX_INS_CMPG_DOUBLE,             cmpg_double)
DEF_DEX_INS_METHOD(DEX_INS_CMP_LONG,                cmp_long)
DEF_DEX_INS_METHOD(DEX_INS_IF_EQ,                   if_eq)
DEF_DEX_INS_METHOD(DEX_INS_IF_NE,                   if_ne)
DEF_DEX_INS_METHOD(DEX_INS_IF_LT,                   if_lt)
DEF_DEX_INS_METHOD(DEX_INS_IF_GE,                   if_ge)
DEF_DEX_INS_METHOD(DEX_INS_IF_GT,                   if_gt)
DEF_DEX_INS_METHOD(DEX_INS_IF_LE,                   if_le)
DEF_DEX_INS_METHOD(DEX_INS_IF_EQZ,                  if_eqz)
DEF_DEX_INS_METHOD(DEX_INS_IF_NEZ,                  if_nez)
DEF_DEX_INS_METHOD(DEX_INS_IF_LTZ,                  if_ltz)
DEF_DEX_INS_METHOD(DEX_INS_IF_GEZ,                  if_gez)
DEF_DEX_INS_METHOD(DEX_INS_IF_GTZ,                  if_gtz)
DEF_DEX_INS_METHOD(DEX_INS_IF_LEZ,                  if_lez)
DEF_DEX_INS_METHOD(DEX_INS_AGET,                    aget)
DEF_DEX_INS_METHOD(DEX_INS_AGET_WIDE,               aget_wide)
DEF_DEX_INS_METHOD(DEX_INS_AGET_OBJECT,             aget_object)
DEF_DEX_INS_METHOD(DEX_INS_AGET_BOOLEAN,            aget_boolean)
DEF_DEX_INS_METHOD(DEX_INS_AGET_BYTE,               aget_byte)
DEF_DEX_INS_METHOD(DEX_INS_AGET_CHAR,               aget_char)
DEF_DEX_INS_METHOD(DEX_INS_AGET_SHORT,              aget_short)
DEF_DEX_INS_METHOD(DEX_INS_APUT,                    aput)
DEF_DEX_INS_METHOD(DEX_INS_APUT_WIDE,               aput_wide)
DEF_DEX_INS_METHOD(DEX_INS_APUT_OBJECT,             aput_object)
DEF_DEX_INS_METHOD(DEX_INS_APUT_BOOLEAN,            aput_boolean)
DEF_DEX_INS_METHOD(DEX_INS_APUT_BYTE,               aput_byte)
DEF_DEX_INS_METHOD(DEX_INS_APUT_CHAR,               aput_char)
DEF_DEX_INS_METHOD(DEX_INS_APUT_SHORT,              aput_short)
DEF_DEX_INS_METHOD(DEX_INS_IGET,                    iget)
DEF_DEX_INS_METHOD(DEX_INS_IGET_WIDE,               iget_wide)
DEF_DEX_INS_METHOD(DEX_INS_IGET_OBJECT,             iget_object)
DEF_DEX_INS_METHOD(DEX_INS_IGET_BOOLEAN,            iget_boolean)
DEF_DEX_INS_METHOD(DEX_INS_IGET_BYTE,               iget_byte)
DEF_DEX_INS_METHOD(DEX_INS_IGET_CHAR,               iget_char)
DEF_DEX_INS_METHOD(DEX_INS_IGET_SHORT,              iget_short)
DEF_DEX_INS_METHOD(DEX_INS_IPUT,                    iput)
DEF_DEX_INS_METHOD(DEX_INS_IPUT_WIDE,               iput_wide)
DEF_DEX_INS_METHOD(DEX_INS_IPUT_OBJECT,             iput_object)
DEF_DEX_INS_METHOD(DEX_INS_IPUT_BOOLEAN,            iput_boolean)
DEF_DEX_INS_METHOD(DEX_INS_IPUT_BYTE,               iput_byte)
DEF_DEX_INS_METHOD(DEX_INS_IPUT_CHAR,               iput_char)
DEF_DEX_INS_METHOD(DEX_INS_IPUT_SHORT,              iput_short)
DEF_DEX_INS_METHOD(DEX_INS_SGET,                    sget)
DEF_DEX_INS_METHOD(DEX_INS_SGET_WIDE,               sget_wide)
DEF_DEX_INS_METHOD(DEX_INS_SGET_OBJECT,             sget_object)
DEF_DEX_INS_METHOD(DEX_INS_SGET_BOOLEAN,            sget_boolean)
DEF_DEX_INS_METHOD(DEX_INS_SGET_BYTE,               sget_byte)
DEF_DEX_INS_METHOD(DEX_INS_SGET_CHAR,               sget_char)
DEF_DEX_INS_METHOD(DEX_INS_SGET_SHORT,              sget_short)
DEF_DEX_INS_METHOD(DEX_INS_SPUT,                    sput)
DEF_DEX_INS_METHOD(DEX_INS_SPUT_WIDE,               sput_wide)
DEF_DEX_INS_METHOD(DEX_INS_SPUT_OBJECT,             sput_object)
DEF_DEX_INS_METHOD(DEX_INS_SPUT_BOOLEAN,            sput_boolean)
DEF_DEX_INS_METHOD(DEX_INS_SPUT_BYTE,               sput_byte)
DEF_DEX_INS_METHOD(DEX_INS_SPUT_CHAR,               sput_char)
DEF_DEX_INS_METHOD(DEX_INS_SPUT_SHORT,              sput_short)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_VIRTUAL,          invoke_virtual)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_SUPER,            invoke_super)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_DIRECT,           invoke_direct)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_STATIC,           invoke_static)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_INTERFACE,        invoke_interface)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_VIRTUAL_RANGE,    invoke_virtual_range)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_SUPER_RANGE,      invoke_super_range)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_DIRECT_RANGE,     invoke_direct_range)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_STATIC_RANGE,     invoke_static_range)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_INTERFACE_RANGE,  invoke_interface_range)
DEF_DEX_INS_METHOD(DEX_INS_NEG_INT,                 neg_int)
DEF_DEX_INS_METHOD(DEX_INS_NOT_INT,                 not_int)
DEF_DEX_INS_METHOD(DEX_INS_NEG_LONG,                neg_long)
DEF_DEX_INS_METHOD(DEX_INS_NOT_LONG,                not_long)
DEF_DEX_INS_METHOD(DEX_INS_NEG_FLOAT,               neg_float)
DEF_DEX_INS_METHOD(DEX_INS_NEG_DOUBLE,              neg_double)
DEF_DEX_INS_METHOD(DEX_INS_INT_TO_LONG,             int_to_long)
DEF_DEX_INS_METHOD(DEX_INS_INT_TO_FLOAT,            int_to_float)
DEF_DEX_INS_METHOD(DEX_INS_INT_TO_DOUBLE,           int_to_double)
DEF_DEX_INS_METHOD(DEX_INS_LONG_TO_INT,             long_to_int)
DEF_DEX_INS_METHOD(DEX_INS_LONG_TO_FLOAT,           long_to_float)
DEF_DEX_INS_METHOD(DEX_INS_LONG_TO_DOUBLE,          long_to_double)
DEF_DEX_INS_METHOD(DEX_INS_FLOAT_TO_INT,            float_to_int)
DEF_DEX_INS_METHOD(DEX_INS_FLOAT_TO_LONG,           float_to_long)
DEF_DEX_INS_METHOD(DEX_INS_FLOAT_TO_DOUBLE,         float_to_double)
DEF_DEX_INS_METHOD(DEX_INS_DOUBLE_TO_INT,           double_to_int)
DEF_DEX_INS_METHOD(DEX_INS_DOUBLE_TO_LONG,          double_to_long)
DEF_DEX_INS_METHOD(DEX_INS_DOUBLE_TO_FLOAT,         double_to_float)
DEF_DEX_INS_METHOD(DEX_INS_INT_TO_BYTE,             int_to_byte)
DEF_DEX_INS_METHOD(DEX_INS_INT_TO_CHAR,             int_to_char)
DEF_DEX_INS_METHOD(DEX_INS_INT_TO_SHORT,            int_to_short)
DEF_DEX_INS_METHOD(DEX_INS_ADD_INT,                 add_int)
DEF_DEX_INS_METHOD(DEX_INS_SUB_INT,                 sub_int)
DEF_DEX_INS_METHOD(DEX_INS_MUL_INT,                 mul_int)
DEF_DEX_INS_METHOD(DEX_INS_DIV_INT,                 div_int)
DEF_DEX_INS_METHOD(DEX_INS_REM_INT,                 rem_int)
DEF_DEX_INS_METHOD(DEX_INS_AND_INT,                 and_int)
DEF_DEX_INS_METHOD(DEX_INS_OR_INT,                  or_int)
DEF_DEX_INS_METHOD(DEX_INS_XOR_INT,                 xor_int)
DEF_DEX_INS_METHOD(DEX_INS_SHL_INT,                 shl_int)
DEF_DEX_INS_METHOD(DEX_INS_SHR_INT,                 shr_int)
DEF_DEX_INS_METHOD(DEX_INS_USHR_INT,                ushr_int)
DEF_DEX_INS_METHOD(DEX_INS_ADD_LONG,                add_long)
DEF_DEX_INS_METHOD(DEX_INS_SUB_LONG,                sub_long)
DEF_DEX_INS_METHOD(DEX_INS_MUL_LONG,                mul_long)
DEF_DEX_INS_METHOD(DEX_INS_DIV_LONG,                div_long)
DEF_DEX_INS_METHOD(DEX_INS_REM_LONG,                rem_long)
DEF_DEX_INS_METHOD(DEX_INS_AND_LONG,                and_long)
DEF_DEX_INS_METHOD(DEX_INS_OR_LONG,                 or_long)
DEF_DEX_INS_METHOD(DEX_INS_XOR_LONG,                xor_long)
DEF_DEX_INS_METHOD(DEX_INS_SHL_LONG,                shl_long)
DEF_DEX_INS_METHOD(DEX_INS_SHR_LONG,                shr_long)
DEF_DEX_INS_METHOD(DEX_INS_USHR_LONG,               ushr_long)
DEF_DEX_INS_METHOD(DEX_INS_ADD_FLOAT,               add_float)
DEF_DEX_INS_METHOD(DEX_INS_SUB_FLOAT,               sub_float)
DEF_DEX_INS_METHOD(DEX_INS_MUL_FLOAT,               mul_float)
DEF_DEX_INS_METHOD(DEX_INS_DIV_FLOAT,               div_float)
DEF_DEX_INS_METHOD(DEX_INS_REM_FLOAT,               rem_float)
DEF_DEX_INS_METHOD(DEX_INS_ADD_DOUBLE,              add_double)
DEF_DEX_INS_METHOD(DEX_INS_SUB_DOUBLE,              sub_double)
DEF_DEX_INS_METHOD(DEX_INS_MUL_DOUBLE,              mul_double)
DEF_DEX_INS_METHOD(DEX_INS_DIV_DOUBLE,              div_double)
DEF_DEX_INS_METHOD(DEX_INS_REM_DOUBLE,              rem_double)
DEF_DEX_INS_METHOD(DEX_INS_ADD_INT_2ADDR,           add_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SUB_INT_2ADDR,           sub_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_MUL_INT_2ADDR,           mul_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_DIV_INT_2ADDR,           div_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_REM_INT_2ADDR,           rem_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_AND_INT_2ADDR,           and_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_OR_INT_2ADDR,            or_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_XOR_INT_2ADDR,           xor_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SHL_INT_2ADDR,           shl_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SHR_INT_2ADDR,           shr_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_USHR_INT_2ADDR,          ushr_int_2addr)
DEF_DEX_INS_METHOD(DEX_INS_ADD_LONG_2ADDR,          add_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SUB_LONG_2ADDR,          sub_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_MUL_LONG_2ADDR,          mul_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_DIV_LONG_2ADDR,          div_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_REM_LONG_2ADDR,          rem_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_AND_LONG_2ADDR,          and_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_OR_LONG_2ADDR,           or_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_XOR_LONG_2ADDR,          xor_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SHL_LONG_2ADDR,          shl_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SHR_LONG_2ADDR,          shr_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_USHR_LONG_2ADDR,         ushr_long_2addr)
DEF_DEX_INS_METHOD(DEX_INS_ADD_FLOAT_2ADDR,         add_float_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SUB_FLOAT_2ADDR,         sub_float_2addr)
DEF_DEX_INS_METHOD(DEX_INS_MUL_FLOAT_2ADDR,         mul_float_2addr)
DEF_DEX_INS_METHOD(DEX_INS_DIV_FLOAT_2ADDR,         div_float_2addr)
DEF_DEX_INS_METHOD(DEX_INS_REM_FLOAT_2ADDR,         rem_float_2addr)
DEF_DEX_INS_METHOD(DEX_INS_ADD_DOUBLE_2ADDR,        add_double_2addr)
DEF_DEX_INS_METHOD(DEX_INS_SUB_DOUBLE_2ADDR,        sub_double_2addr)
DEF_DEX_INS_METHOD(DEX_INS_MUL_DOUBLE_2ADDR,        mul_double_2addr)
DEF_DEX_INS_METHOD(DEX_INS_DIV_DOUBLE_2ADDR,        div_double_2addr)
DEF_DEX_INS_METHOD(DEX_INS_REM_DOUBLE_2ADDR,        rem_double_2addr)
DEF_DEX_INS_METHOD(DEX_INS_ADD_INT_LIT16,           add_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_RSUB_INT,                rsub_int)
DEF_DEX_INS_METHOD(DEX_INS_MUL_INT_LIT16,           mul_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_DIV_INT_LIT16,           div_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_REM_INT_LIT16,           rem_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_AND_INT_LIT16,           and_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_OR_INT_LIT16,            or_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_XOR_INT_LIT16,           xor_int_lit16)
DEF_DEX_INS_METHOD(DEX_INS_ADD_INT_LIT8,            add_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_RSUB_INT_LIT8,           rsub_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_MUL_INT_LIT8,            mul_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_DIV_INT_LIT8,            div_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_REM_INT_LIT8,            rem_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_AND_INT_LIT8,            and_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_OR_INT_LIT8,             or_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_XOR_INT_LIT8,            xor_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_SHL_INT_LIT8,            shl_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_SHR_INT_LIT8,            shr_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_USHR_INT_LIT8,           ushr_int_lit8)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_POLYMORPHIC,      invoke_polymorphic)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_POLYMORPHIC_RANGE, invoke_polymorphic_range)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_CUSTOM,           invoke_custom)
DEF_DEX_INS_METHOD(DEX_INS_INVOKE_CUSTOM_RANGE,     invoke_custom_range)
DEF_DEX_INS_METHOD(DEX_INS_CONST_METHOD_HANDLE,     const_method_handle)
DEF_DEX_INS_METHOD(DEX_INS_CONST_METHOD_TYPE,       const_method_type)

#endif //GARLIC_DEX_INS_HELPER_H

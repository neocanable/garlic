#include "dex_ins_helper.h"

extern string dex_opcode_name(u1 code)
{
    switch(code) {
        case DEX_INS_NOP: return "nop";
        case DEX_INS_MOVE: return "move";
        case DEX_INS_MOVE_FROM16: return "move/from16";
        case DEX_INS_MOVE_16: return "move/16";
        case DEX_INS_MOVE_WIDE: return "move-wide";
        case DEX_INS_MOVE_WIDE_FROM16: return "move-wide/from16";
        case DEX_INS_MOVE_WIDE_16: return "move-wide/16";
        case DEX_INS_MOVE_OBJECT: return "move-object";
        case DEX_INS_MOVE_OBJECT_FROM16: return "move-object/from16";
        case DEX_INS_MOVE_OBJECT_16: return "move-object/16";
        case DEX_INS_MOVE_RESULT: return "move-result";
        case DEX_INS_MOVE_RESULT_WIDE: return "move-result-wide";
        case DEX_INS_MOVE_RESULT_OBJECT: return "move-result-object";
        case DEX_INS_MOVE_EXCEPTION: return "move-exception";
        case DEX_INS_RETURN_VOID: return "return-void";
        case DEX_INS_RETURN: return "return";
        case DEX_INS_RETURN_WIDE: return "return-wide";
        case DEX_INS_RETURN_OBJECT: return "return-object";
        case DEX_INS_CONST_4: return "const/4";
        case DEX_INS_CONST_16: return "const/16";
        case DEX_INS_CONST: return "const";
        case DEX_INS_CONST_HIGH16: return "const/high16";
        case DEX_INS_CONST_WIDE_16: return "const-wide/16";
        case DEX_INS_CONST_WIDE_32: return "const-wide/32";
        case DEX_INS_CONST_WIDE: return "const-wide";
        case DEX_INS_CONST_WIDE_HIGH16: return "const-wide/high16";
        case DEX_INS_CONST_STRING: return "const-string";
        case DEX_INS_CONST_STRING_JUMBO: return "const-string/jumbo";
        case DEX_INS_CONST_CLASS: return "const-class";
        case DEX_INS_MONITOR_ENTER: return "monitor-enter";
        case DEX_INS_MONITOR_EXIT: return "monitor-exit";
        case DEX_INS_CHECK_CAST: return "check-cast";
        case DEX_INS_INSTANCE_OF: return "instance-of";
        case DEX_INS_ARRAY_LENGTH: return "array-length";
        case DEX_INS_NEW_INSTANCE: return "new-instance";
        case DEX_INS_NEW_ARRAY: return "new-array";
        case DEX_INS_FILLED_NEW_ARRAY: return "filled-new-array";
        case DEX_INS_FILLED_NEW_ARRAY_RANGE: return "filled-new-array/range";
        case DEX_INS_FILL_ARRAY_DATA: return "fill-array-data";
        case DEX_INS_THROW: return "throw";
        case DEX_INS_GOTO: return "goto";
        case DEX_INS_GOTO_16: return "goto/16";
        case DEX_INS_GOTO_32: return "goto/32";
        case DEX_INS_PACKED_SWITCH: return "packed-switch";
        case DEX_INS_SPARSE_SWITCH: return "sparse-switch";
        case DEX_INS_CMPL_FLOAT: return "cmpl-float";
        case DEX_INS_CMPG_FLOAT: return "cmpg-float";
        case DEX_INS_CMPL_DOUBLE: return "cmpl-double";
        case DEX_INS_CMPG_DOUBLE: return "cmpg-double";
        case DEX_INS_CMP_LONG: return "cmp-long";
        case DEX_INS_IF_EQ: return "if-eq";
        case DEX_INS_IF_NE: return "if-ne";
        case DEX_INS_IF_LT: return "if-lt";
        case DEX_INS_IF_GE: return "if-ge";
        case DEX_INS_IF_GT: return "if-gt";
        case DEX_INS_IF_LE: return "if-le";
        case DEX_INS_IF_EQZ: return "if-eqz";
        case DEX_INS_IF_NEZ: return "if-nez";
        case DEX_INS_IF_LTZ: return "if-ltz";
        case DEX_INS_IF_GEZ: return "if-gez";
        case DEX_INS_IF_GTZ: return "if-gtz";
        case DEX_INS_IF_LEZ: return "if-lez";
        case DEX_INS_AGET: return "aget";
        case DEX_INS_AGET_WIDE: return "aget-wide";
        case DEX_INS_AGET_OBJECT: return "aget-object";
        case DEX_INS_AGET_BOOLEAN: return "aget-boolean";
        case DEX_INS_AGET_BYTE: return "aget-byte";
        case DEX_INS_AGET_CHAR: return "aget-char";
        case DEX_INS_AGET_SHORT: return "aget-short";
        case DEX_INS_APUT: return "aput";
        case DEX_INS_APUT_WIDE: return "aput-wide";
        case DEX_INS_APUT_OBJECT: return "aput-object";
        case DEX_INS_APUT_BOOLEAN: return "aput-boolean";
        case DEX_INS_APUT_BYTE: return "aput-byte";
        case DEX_INS_APUT_CHAR: return "aput-char";
        case DEX_INS_APUT_SHORT: return "aput-short";
        case DEX_INS_IGET: return "iget";
        case DEX_INS_IGET_WIDE: return "iget-wide";
        case DEX_INS_IGET_OBJECT: return "iget-object";
        case DEX_INS_IGET_BOOLEAN: return "iget-boolean";
        case DEX_INS_IGET_BYTE: return "iget-byte";
        case DEX_INS_IGET_CHAR: return "iget-char";
        case DEX_INS_IGET_SHORT: return "iget-short";
        case DEX_INS_IPUT: return "iput";
        case DEX_INS_IPUT_WIDE: return "iput-wide";
        case DEX_INS_IPUT_OBJECT: return "iput-object";
        case DEX_INS_IPUT_BOOLEAN: return "iput-boolean";
        case DEX_INS_IPUT_BYTE: return "iput-byte";
        case DEX_INS_IPUT_CHAR: return "iput-char";
        case DEX_INS_IPUT_SHORT: return "iput-short";
        case DEX_INS_SGET: return "sget";
        case DEX_INS_SGET_WIDE: return "sget-wide";
        case DEX_INS_SGET_OBJECT: return "sget-object";
        case DEX_INS_SGET_BOOLEAN: return "sget-boolean";
        case DEX_INS_SGET_BYTE: return "sget-byte";
        case DEX_INS_SGET_CHAR: return "sget-char";
        case DEX_INS_SGET_SHORT: return "sget-short";
        case DEX_INS_SPUT: return "sput";
        case DEX_INS_SPUT_WIDE: return "sput-wide";
        case DEX_INS_SPUT_OBJECT: return "sput-object";
        case DEX_INS_SPUT_BOOLEAN: return "sput-boolean";
        case DEX_INS_SPUT_BYTE: return "sput-byte";
        case DEX_INS_SPUT_CHAR: return "sput-char";
        case DEX_INS_SPUT_SHORT: return "sput-short";
        case DEX_INS_INVOKE_VIRTUAL: return "invoke-virtual";
        case DEX_INS_INVOKE_SUPER: return "invoke-super";
        case DEX_INS_INVOKE_DIRECT: return "invoke-direct";
        case DEX_INS_INVOKE_STATIC: return "invoke-static";
        case DEX_INS_INVOKE_INTERFACE: return "invoke-interface";
        case DEX_INS_INVOKE_VIRTUAL_RANGE: return "invoke-virtual/range";
        case DEX_INS_INVOKE_SUPER_RANGE: return "invoke-super/range";
        case DEX_INS_INVOKE_DIRECT_RANGE: return "invoke-direct/range";
        case DEX_INS_INVOKE_STATIC_RANGE: return "invoke-static/range";
        case DEX_INS_INVOKE_INTERFACE_RANGE: return "invoke-interface/range";
        case DEX_INS_NEG_INT: return "neg-int";
        case DEX_INS_NOT_INT: return "not-int";
        case DEX_INS_NEG_LONG: return "neg-long";
        case DEX_INS_NOT_LONG: return "not-long";
        case DEX_INS_NEG_FLOAT: return "neg-float";
        case DEX_INS_NEG_DOUBLE: return "neg-double";
        case DEX_INS_INT_TO_LONG: return "int-to-long";
        case DEX_INS_INT_TO_FLOAT: return "int-to-float";
        case DEX_INS_INT_TO_DOUBLE: return "int-to-double";
        case DEX_INS_LONG_TO_INT: return "long-to-int";
        case DEX_INS_LONG_TO_FLOAT: return "long-to-float";
        case DEX_INS_LONG_TO_DOUBLE: return "long-to-double";
        case DEX_INS_FLOAT_TO_INT: return "float-to-int";
        case DEX_INS_FLOAT_TO_LONG: return "float-to-long";
        case DEX_INS_FLOAT_TO_DOUBLE: return "float-to-double";
        case DEX_INS_DOUBLE_TO_INT: return "double-to-int";
        case DEX_INS_DOUBLE_TO_LONG: return "double-to-long";
        case DEX_INS_DOUBLE_TO_FLOAT: return "double-to-float";
        case DEX_INS_INT_TO_BYTE: return "int-to-byte";
        case DEX_INS_INT_TO_CHAR: return "int-to-char";
        case DEX_INS_INT_TO_SHORT: return "int-to-short";
        case DEX_INS_ADD_INT: return "add-int";
        case DEX_INS_SUB_INT: return "sub-int";
        case DEX_INS_MUL_INT: return "mul-int";
        case DEX_INS_DIV_INT: return "div-int";
        case DEX_INS_REM_INT: return "rem-int";
        case DEX_INS_AND_INT: return "and-int";
        case DEX_INS_OR_INT: return "or-int";
        case DEX_INS_XOR_INT: return "xor-int";
        case DEX_INS_SHL_INT: return "shl-int";
        case DEX_INS_SHR_INT: return "shr-int";
        case DEX_INS_USHR_INT: return "ushr-int";
        case DEX_INS_ADD_LONG: return "add-long";
        case DEX_INS_SUB_LONG: return "sub-long";
        case DEX_INS_MUL_LONG: return "mul-long";
        case DEX_INS_DIV_LONG: return "div-long";
        case DEX_INS_REM_LONG: return "rem-long";
        case DEX_INS_AND_LONG: return "and-long";
        case DEX_INS_OR_LONG: return "or-long";
        case DEX_INS_XOR_LONG: return "xor-long";
        case DEX_INS_SHL_LONG: return "shl-long";
        case DEX_INS_SHR_LONG: return "shr-long";
        case DEX_INS_USHR_LONG: return "ushr-long";
        case DEX_INS_ADD_FLOAT: return "add-float";
        case DEX_INS_SUB_FLOAT: return "sub-float";
        case DEX_INS_MUL_FLOAT: return "mul-float";
        case DEX_INS_DIV_FLOAT: return "div-float";
        case DEX_INS_REM_FLOAT: return "rem-float";
        case DEX_INS_ADD_DOUBLE: return "add-double";
        case DEX_INS_SUB_DOUBLE: return "sub-double";
        case DEX_INS_MUL_DOUBLE: return "mul-double";
        case DEX_INS_DIV_DOUBLE: return "div-double";
        case DEX_INS_REM_DOUBLE: return "rem-double";
        case DEX_INS_ADD_INT_2ADDR: return "add-int/2addr";
        case DEX_INS_SUB_INT_2ADDR: return "sub-int/2addr";
        case DEX_INS_MUL_INT_2ADDR: return "mul-int/2addr";
        case DEX_INS_DIV_INT_2ADDR: return "div-int/2addr";
        case DEX_INS_REM_INT_2ADDR: return "rem-int/2addr";
        case DEX_INS_AND_INT_2ADDR: return "and-int/2addr";
        case DEX_INS_OR_INT_2ADDR: return "or-int/2addr";
        case DEX_INS_XOR_INT_2ADDR: return "xor-int/2addr";
        case DEX_INS_SHL_INT_2ADDR: return "shl-int/2addr";
        case DEX_INS_SHR_INT_2ADDR: return "shr-int/2addr";
        case DEX_INS_USHR_INT_2ADDR: return "ushr-int/2addr";
        case DEX_INS_ADD_LONG_2ADDR: return "add-long/2addr";
        case DEX_INS_SUB_LONG_2ADDR: return "sub-long/2addr";
        case DEX_INS_MUL_LONG_2ADDR: return "mul-long/2addr";
        case DEX_INS_DIV_LONG_2ADDR: return "div-long/2addr";
        case DEX_INS_REM_LONG_2ADDR: return "rem-long/2addr";
        case DEX_INS_AND_LONG_2ADDR: return "and-long/2addr";
        case DEX_INS_OR_LONG_2ADDR: return "or-long/2addr";
        case DEX_INS_XOR_LONG_2ADDR: return "xor-long/2addr";
        case DEX_INS_SHL_LONG_2ADDR: return "shl-long/2addr";
        case DEX_INS_SHR_LONG_2ADDR: return "shr-long/2addr";
        case DEX_INS_USHR_LONG_2ADDR: return "ushr-long/2addr";
        case DEX_INS_ADD_FLOAT_2ADDR: return "add-float/2addr";
        case DEX_INS_SUB_FLOAT_2ADDR: return "sub-float/2addr";
        case DEX_INS_MUL_FLOAT_2ADDR: return "mul-float/2addr";
        case DEX_INS_DIV_FLOAT_2ADDR: return "div-float/2addr";
        case DEX_INS_REM_FLOAT_2ADDR: return "rem-float/2addr";
        case DEX_INS_ADD_DOUBLE_2ADDR: return "add-double/2addr";
        case DEX_INS_SUB_DOUBLE_2ADDR: return "sub-double/2addr";
        case DEX_INS_MUL_DOUBLE_2ADDR: return "mul-double/2addr";
        case DEX_INS_DIV_DOUBLE_2ADDR: return "div-double/2addr";
        case DEX_INS_REM_DOUBLE_2ADDR: return "rem-double/2addr";
        case DEX_INS_ADD_INT_LIT16: return "add-int/lit16";
        case DEX_INS_RSUB_INT: return "rsub-int";
        case DEX_INS_MUL_INT_LIT16: return "mul-int/lit16";
        case DEX_INS_DIV_INT_LIT16: return "div-int/lit16";
        case DEX_INS_REM_INT_LIT16: return "rem-int/lit16";
        case DEX_INS_AND_INT_LIT16: return "and-int/lit16";
        case DEX_INS_OR_INT_LIT16: return "or-int/lit16";
        case DEX_INS_XOR_INT_LIT16: return "xor-int/lit16";
        case DEX_INS_ADD_INT_LIT8: return "add-int/lit8";
        case DEX_INS_RSUB_INT_LIT8: return "rsub-int/lit8";
        case DEX_INS_MUL_INT_LIT8: return "mul-int/lit8";
        case DEX_INS_DIV_INT_LIT8: return "div-int/lit8";
        case DEX_INS_REM_INT_LIT8: return "rem-int/lit8";
        case DEX_INS_AND_INT_LIT8: return "and-int/lit8";
        case DEX_INS_OR_INT_LIT8: return "or-int/lit8";
        case DEX_INS_XOR_INT_LIT8: return "xor-int/lit8";
        case DEX_INS_SHL_INT_LIT8: return "shl-int/lit8";
        case DEX_INS_SHR_INT_LIT8: return "shr-int/lit8";
        case DEX_INS_USHR_INT_LIT8: return "ushr-int/lit8";
        case DEX_INS_COPY_BASIC_BLOCK: return "copy-basic-block";
        case DEX_INS_INVOKE_POLYMORPHIC: return "invoke-polymorphic";
        case DEX_INS_INVOKE_POLYMORPHIC_RANGE: return "invoke-polymorphic/range";
        case DEX_INS_INVOKE_CUSTOM: return "invoke-custom";
        case DEX_INS_INVOKE_CUSTOM_RANGE: return "invoke-custom/range";
        case DEX_INS_CONST_METHOD_HANDLE: return "const-method-handle";
        case DEX_INS_CONST_METHOD_TYPE: return "const-method-type";
        default: return (string)g_str_unknown;
    }
}

extern int dex_opcode_len(u1 code)
{
    switch(code) {
        case DEX_INS_NOP:
        case DEX_INS_MOVE:
        case DEX_INS_MOVE_WIDE:
        case DEX_INS_MOVE_OBJECT:
        case DEX_INS_MOVE_RESULT:
        case DEX_INS_MOVE_RESULT_WIDE:
        case DEX_INS_MOVE_RESULT_OBJECT:
        case DEX_INS_MOVE_EXCEPTION:
        case DEX_INS_RETURN_VOID:
        case DEX_INS_RETURN:
        case DEX_INS_RETURN_WIDE:
        case DEX_INS_RETURN_OBJECT:
        case DEX_INS_CONST_4:
        case DEX_INS_MONITOR_ENTER:
        case DEX_INS_MONITOR_EXIT:
        case DEX_INS_ARRAY_LENGTH:
        case DEX_INS_THROW:
        case DEX_INS_GOTO:
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
        case DEX_INS_INT_TO_SHORT:
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
        case DEX_INS_REM_DOUBLE_2ADDR:
        case DEX_INS_COPY_BASIC_BLOCK:
            return 1;
        case DEX_INS_MOVE_FROM16:
        case DEX_INS_MOVE_WIDE_FROM16:
        case DEX_INS_MOVE_OBJECT_FROM16:
        case DEX_INS_CONST_16:
        case DEX_INS_CONST_HIGH16:
        case DEX_INS_CONST_WIDE_16:
        case DEX_INS_CONST_WIDE_HIGH16:
        case DEX_INS_CONST_STRING:
        case DEX_INS_CONST_CLASS:
        case DEX_INS_CHECK_CAST:
        case DEX_INS_INSTANCE_OF:
        case DEX_INS_NEW_INSTANCE:
        case DEX_INS_NEW_ARRAY:
        case DEX_INS_GOTO_16:
        case DEX_INS_CMPL_FLOAT:
        case DEX_INS_CMPG_FLOAT:
        case DEX_INS_CMPL_DOUBLE:
        case DEX_INS_CMPG_DOUBLE:
        case DEX_INS_CMP_LONG:
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
        case DEX_INS_APUT_SHORT:
        case DEX_INS_IGET:
        case DEX_INS_IGET_WIDE:
        case DEX_INS_IGET_OBJECT:
        case DEX_INS_IGET_BOOLEAN:
        case DEX_INS_IGET_BYTE:
        case DEX_INS_IGET_CHAR:
        case DEX_INS_IGET_SHORT:
        case DEX_INS_IPUT:
        case DEX_INS_IPUT_WIDE:
        case DEX_INS_IPUT_OBJECT:
        case DEX_INS_IPUT_BOOLEAN:
        case DEX_INS_IPUT_BYTE:
        case DEX_INS_IPUT_CHAR:
        case DEX_INS_IPUT_SHORT:
        case DEX_INS_SGET:
        case DEX_INS_SGET_WIDE:
        case DEX_INS_SGET_OBJECT:
        case DEX_INS_SGET_BOOLEAN:
        case DEX_INS_SGET_BYTE:
        case DEX_INS_SGET_CHAR:
        case DEX_INS_SGET_SHORT:
        case DEX_INS_SPUT:
        case DEX_INS_SPUT_WIDE:
        case DEX_INS_SPUT_OBJECT:
        case DEX_INS_SPUT_BOOLEAN:
        case DEX_INS_SPUT_BYTE:
        case DEX_INS_SPUT_CHAR:
        case DEX_INS_SPUT_SHORT:
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
        case DEX_INS_REM_DOUBLE:
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
        case DEX_INS_USHR_INT_LIT8:
        case DEX_INS_CONST_METHOD_HANDLE:
        case DEX_INS_CONST_METHOD_TYPE:
            return 2;
        case DEX_INS_MOVE_16:
        case DEX_INS_MOVE_WIDE_16:
        case DEX_INS_MOVE_OBJECT_16:
        case DEX_INS_CONST:
        case DEX_INS_CONST_WIDE_32:
        case DEX_INS_CONST_STRING_JUMBO:
        case DEX_INS_FILLED_NEW_ARRAY:
        case DEX_INS_FILLED_NEW_ARRAY_RANGE:
        case DEX_INS_FILL_ARRAY_DATA:
        case DEX_INS_GOTO_32:
        case DEX_INS_PACKED_SWITCH:
        case DEX_INS_SPARSE_SWITCH:
        case DEX_INS_INVOKE_VIRTUAL:
        case DEX_INS_INVOKE_SUPER:
        case DEX_INS_INVOKE_DIRECT:
        case DEX_INS_INVOKE_STATIC:
        case DEX_INS_INVOKE_INTERFACE:
        case DEX_INS_INVOKE_VIRTUAL_RANGE:
        case DEX_INS_INVOKE_SUPER_RANGE:
        case DEX_INS_INVOKE_DIRECT_RANGE:
        case DEX_INS_INVOKE_STATIC_RANGE:
        case DEX_INS_INVOKE_INTERFACE_RANGE:
        case DEX_INS_INVOKE_CUSTOM:
        case DEX_INS_INVOKE_CUSTOM_RANGE:
            return 3;
        case DEX_INS_CONST_WIDE:
            return 5;
        case DEX_INS_INVOKE_POLYMORPHIC:
        case DEX_INS_INVOKE_POLYMORPHIC_RANGE:
            return 4;
        default: return 0;
    }
}

extern dex_instruction_format dex_opcode_fmt(u1 code)
{
    switch(code) {
        case DEX_INS_NOP:
        case DEX_INS_RETURN_VOID:
        case DEX_INS_COPY_BASIC_BLOCK:
            return kFmt10x;
        case DEX_INS_MOVE:
        case DEX_INS_MOVE_WIDE:
        case DEX_INS_MOVE_OBJECT:
        case DEX_INS_ARRAY_LENGTH:
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
        case DEX_INS_INT_TO_SHORT:
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
        case DEX_INS_REM_DOUBLE_2ADDR:
            return kFmt12x;
        case DEX_INS_MOVE_FROM16:
        case DEX_INS_MOVE_WIDE_FROM16:
        case DEX_INS_MOVE_OBJECT_FROM16:
            return kFmt22x;
        case DEX_INS_MOVE_16:
        case DEX_INS_MOVE_WIDE_16:
        case DEX_INS_MOVE_OBJECT_16:
            return kFmt32x;
        case DEX_INS_MOVE_RESULT:
        case DEX_INS_MOVE_RESULT_WIDE:
        case DEX_INS_MOVE_RESULT_OBJECT:
        case DEX_INS_MOVE_EXCEPTION:
        case DEX_INS_RETURN:
        case DEX_INS_RETURN_WIDE:
        case DEX_INS_RETURN_OBJECT:
        case DEX_INS_MONITOR_ENTER:
        case DEX_INS_MONITOR_EXIT:
        case DEX_INS_THROW:
            return kFmt11x;
        case DEX_INS_CONST_4:
            return kFmt11n;
        case DEX_INS_CONST_16:
        case DEX_INS_CONST_WIDE_16:
            return kFmt21s;
        case DEX_INS_CONST:
        case DEX_INS_CONST_WIDE_32:
            return kFmt31i;
        case DEX_INS_CONST_HIGH16:
        case DEX_INS_CONST_WIDE_HIGH16:
            return kFmt21h;
        case DEX_INS_CONST_WIDE:
            return kFmt51l;
        case DEX_INS_CONST_STRING:
        case DEX_INS_CONST_CLASS:
        case DEX_INS_CHECK_CAST:
        case DEX_INS_NEW_INSTANCE:
        case DEX_INS_SGET:
        case DEX_INS_SGET_WIDE:
        case DEX_INS_SGET_OBJECT:
        case DEX_INS_SGET_BOOLEAN:
        case DEX_INS_SGET_BYTE:
        case DEX_INS_SGET_CHAR:
        case DEX_INS_SGET_SHORT:
        case DEX_INS_SPUT:
        case DEX_INS_SPUT_WIDE:
        case DEX_INS_SPUT_OBJECT:
        case DEX_INS_SPUT_BOOLEAN:
        case DEX_INS_SPUT_BYTE:
        case DEX_INS_SPUT_CHAR:
        case DEX_INS_SPUT_SHORT:
        case DEX_INS_CONST_METHOD_HANDLE:
        case DEX_INS_CONST_METHOD_TYPE:
            return kFmt21c;
        case DEX_INS_CONST_STRING_JUMBO:
            return kFmt31c;
        case DEX_INS_INSTANCE_OF:
        case DEX_INS_NEW_ARRAY:
        case DEX_INS_IGET:
        case DEX_INS_IGET_WIDE:
        case DEX_INS_IGET_OBJECT:
        case DEX_INS_IGET_BOOLEAN:
        case DEX_INS_IGET_BYTE:
        case DEX_INS_IGET_CHAR:
        case DEX_INS_IGET_SHORT:
        case DEX_INS_IPUT:
        case DEX_INS_IPUT_WIDE:
        case DEX_INS_IPUT_OBJECT:
        case DEX_INS_IPUT_BOOLEAN:
        case DEX_INS_IPUT_BYTE:
        case DEX_INS_IPUT_CHAR:
        case DEX_INS_IPUT_SHORT:
            return kFmt22c;
        case DEX_INS_FILLED_NEW_ARRAY:
        case DEX_INS_INVOKE_VIRTUAL:
        case DEX_INS_INVOKE_SUPER:
        case DEX_INS_INVOKE_DIRECT:
        case DEX_INS_INVOKE_STATIC:
        case DEX_INS_INVOKE_INTERFACE:
        case DEX_INS_INVOKE_CUSTOM:
            return kFmt35c;
        case DEX_INS_FILLED_NEW_ARRAY_RANGE:
        case DEX_INS_INVOKE_VIRTUAL_RANGE:
        case DEX_INS_INVOKE_SUPER_RANGE:
        case DEX_INS_INVOKE_DIRECT_RANGE:
        case DEX_INS_INVOKE_STATIC_RANGE:
        case DEX_INS_INVOKE_INTERFACE_RANGE:
        case DEX_INS_INVOKE_CUSTOM_RANGE:
            return kFmt3rc;
        case DEX_INS_FILL_ARRAY_DATA:
        case DEX_INS_PACKED_SWITCH:
        case DEX_INS_SPARSE_SWITCH:
            return kFmt31t;
        case DEX_INS_GOTO:
            return kFmt10t;
        case DEX_INS_GOTO_16:
            return kFmt20t;
        case DEX_INS_GOTO_32:
            return kFmt30t;
        case DEX_INS_CMPL_FLOAT:
        case DEX_INS_CMPG_FLOAT:
        case DEX_INS_CMPL_DOUBLE:
        case DEX_INS_CMPG_DOUBLE:
        case DEX_INS_CMP_LONG:
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
        case DEX_INS_APUT_SHORT:
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
        case DEX_INS_REM_DOUBLE:
            return kFmt23x;
        case DEX_INS_IF_EQ:
        case DEX_INS_IF_NE:
        case DEX_INS_IF_LT:
        case DEX_INS_IF_GE:
        case DEX_INS_IF_GT:
        case DEX_INS_IF_LE:
            return kFmt22t;
        case DEX_INS_IF_EQZ:
        case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ:
        case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ:
        case DEX_INS_IF_LEZ:
            return kFmt21t;
        case DEX_INS_ADD_INT_LIT16:
        case DEX_INS_RSUB_INT:
        case DEX_INS_MUL_INT_LIT16:
        case DEX_INS_DIV_INT_LIT16:
        case DEX_INS_REM_INT_LIT16:
        case DEX_INS_AND_INT_LIT16:
        case DEX_INS_OR_INT_LIT16:
        case DEX_INS_XOR_INT_LIT16:
            return kFmt22s;
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
        case DEX_INS_USHR_INT_LIT8:
            return kFmt22b;
        case DEX_INS_INVOKE_POLYMORPHIC:
            return kFmt45cc;
        case DEX_INS_INVOKE_POLYMORPHIC_RANGE:
            return kFmt4rcc;
        default: return 0;
    }
}
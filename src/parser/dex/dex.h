#ifndef GARLIC_DEX_H
#define GARLIC_DEX_H

#include "types.h"
#include "list.h"
#include "hashmap.h"
#include "decompiler/structure.h"

// https://source.android.com/docs/core/runtime/dex-format?hl=en

typedef uint8_t uleb128;

#define NO_INDEX 0xffffffff

/*
 * 160-bit SHA-1 digest.
 */
enum { kSHA1DigestLen = 20,
    kSHA1DigestOutputLen = kSHA1DigestLen*2 +1 };

/* general constants */
enum {
    kDexEndianConstant = 0x12345678,    /* the endianness indicator */
    kDexNoIndex = 0xffffffff,           /* not a valid index value */
};

/*
 * Enumeration of all the primitive types.
 */
enum PrimitiveType {
    PRIM_NOT        = 0, /* value is a reference type, not a primitive type */
    PRIM_VOID       = 1,
    PRIM_BOOLEAN    = 2,
    PRIM_BYTE       = 3,
    PRIM_SHORT      = 4,
    PRIM_CHAR       = 5,
    PRIM_INT        = 6,
    PRIM_LONG       = 7,
    PRIM_FLOAT      = 8,
    PRIM_DOUBLE     = 9,
};

/*
 * access flags and masks; the "standard" ones are all <= 0x4000
 *
 * Note: There are related declarations in vm/oo/Object.h in the ClassFlags
 * enum.
 */
enum {
    ACC_PUBLIC       = 0x00000001,       // class, field, m, ic
    ACC_PRIVATE      = 0x00000002,       // field, m, ic
    ACC_PROTECTED    = 0x00000004,       // field, m, ic
    ACC_STATIC       = 0x00000008,       // field, m, ic
    ACC_FINAL        = 0x00000010,       // class, field, m, ic
    ACC_SYNCHRONIZED = 0x00000020,       // m (only allowed on natives)
    ACC_SUPER        = 0x00000020,       // class (not used in Dalvik)
    ACC_VOLATILE     = 0x00000040,       // field
    ACC_BRIDGE       = 0x00000040,       // m (1.5)
    ACC_TRANSIENT    = 0x00000080,       // field
    ACC_VARARGS      = 0x00000080,       // m (1.5)
    ACC_NATIVE       = 0x00000100,       // m
    ACC_INTERFACE    = 0x00000200,       // class, ic
    ACC_ABSTRACT     = 0x00000400,       // class, m, ic
    ACC_STRICT       = 0x00000800,       // m
    ACC_SYNTHETIC    = 0x00001000,       // field, m, ic
    ACC_ANNOTATION   = 0x00002000,       // class, ic (1.5)
    ACC_ENUM         = 0x00004000,       // class, field, ic (1.5)
    ACC_CONSTRUCTOR  = 0x00010000,       // m (Dalvik only)
    ACC_DECLARED_SYNCHRONIZED =
    0x00020000,       // m (Dalvik only)
    ACC_CLASS_MASK =
    (ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT
     | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM),
    ACC_INNER_CLASS_MASK =
    (ACC_CLASS_MASK | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC),
    ACC_FIELD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM),
    ACC_METHOD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE
     | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR
     | ACC_DECLARED_SYNCHRONIZED),
};

typedef enum jd_dex_access_flags {
    ACC_DEX_PUBLIC                  = 0x0001,
    ACC_DEX_PRIVATE                 = 0x0002,
    ACC_DEX_PROTECTED               = 0x0004,
    ACC_DEX_STATIC                  = 0x0008,
    ACC_DEX_FINAL                   = 0x0010,
    ACC_DEX_SYNCHRONIZED            = 0x0020,
    ACC_DEX_VOLATILE                = 0x0040,
    ACC_DEX_VARARGS                 = 0x0080,
    ACC_DEX_NATIVE                  = 0x0100,
    ACC_DEX_INTERFACE               = 0x0200,
    ACC_DEX_ABSTRACT                = 0x0400,
    ACC_DEX_STRICT                  = 0x0800,
    ACC_DEX_SYNTHETIC               = 0x1000,
    ACC_DEX_ANNOTATION              = 0x2000,
    ACC_DEX_ENUM                    = 0x4000,
    ACC_DEX_CONSTRUCTOR             = 0x10000,
    ACC_DEX_DECLARED_SYNCHRONIZED   = 0x20000,
} jd_dex_access_flags;

/* annotation constants */
enum {
    kDexVisibilityBuild         = 0x00,     /* annotation visibility */
    kDexVisibilityRuntime       = 0x01,
    kDexVisibilitySystem        = 0x02,

    kDexAnnotationByte          = 0x00,
    kDexAnnotationShort         = 0x02,
    kDexAnnotationChar          = 0x03,
    kDexAnnotationInt           = 0x04,
    kDexAnnotationLong          = 0x06,
    kDexAnnotationFloat         = 0x10,
    kDexAnnotationDouble        = 0x11,
    kDexAnnotationMethodType    = 0x15,
    kDexAnnotationMethodHandle  = 0x16,
    kDexAnnotationString        = 0x17,
    kDexAnnotationType          = 0x18,
    kDexAnnotationField         = 0x19,
    kDexAnnotationMethod        = 0x1a,
    kDexAnnotationEnum          = 0x1b,
    kDexAnnotationArray         = 0x1c,
    kDexAnnotationAnnotation    = 0x1d,
    kDexAnnotationNull          = 0x1e,
    kDexAnnotationBoolean       = 0x1f,

    kDexAnnotationValueTypeMask = 0x1f,     /* low 5 bits */
    kDexAnnotationValueArgShift = 5,
};

/* map item type codes */
enum {
    kDexTypeHeaderItem               = 0x0000,
    kDexTypeStringIdItem             = 0x0001,
    kDexTypeTypeIdItem               = 0x0002,
    kDexTypeProtoIdItem              = 0x0003,
    kDexTypeFieldIdItem              = 0x0004,
    kDexTypeMethodIdItem             = 0x0005,
    kDexTypeClassDefItem             = 0x0006,
    kDexTypeCallSiteIdItem           = 0x0007,
    kDexTypeMethodHandleItem         = 0x0008,
    kDexTypeMapList                  = 0x1000,
    kDexTypeTypeList                 = 0x1001,
    kDexTypeAnnotationSetRefList     = 0x1002,
    kDexTypeAnnotationSetItem        = 0x1003,
    kDexTypeClassDataItem            = 0x2000,
    kDexTypeCodeItem                 = 0x2001,
    kDexTypeStringDataItem           = 0x2002,
    kDexTypeDebugInfoItem            = 0x2003,
    kDexTypeAnnotationItem           = 0x2004,
    kDexTypeEncodedArrayItem         = 0x2005,
    kDexTypeAnnotationsDirectoryItem = 0x2006,
};

/* auxillary data section chunk codes */
enum {
    kDexChunkClassLookup            = 0x434c4b50,   /* CLKP */
    kDexChunkRegisterMaps           = 0x524d4150,   /* RMAP */
    kDexChunkEnd                    = 0x41454e44,   /* AEND */
};

typedef enum dex_instruction_format {
    kFmt00x = 0,    // unknown format (also used for "breakpoint" opcode)
    kFmt10x,        // op
    kFmt12x,        // op vA, vB
    kFmt11n,        // op vA, #+B
    kFmt11x,        // op vAA
    kFmt10t,        // op +AA
    kFmt20bc,       // [opt] op AA, thing@BBBB
    kFmt20t,        // op +AAAA
    kFmt22x,        // op vAA, vBBBB
    kFmt21t,        // op vAA, +BBBB
    kFmt21s,        // op vAA, #+BBBB
    kFmt21h,        // op vAA, #+BBBB00000[00000000]
    kFmt21c,        // op vAA, thing@BBBB
    kFmt23x,        // op vAA, vBB, vCC
    kFmt22b,        // op vAA, vBB, #+CC
    kFmt22t,        // op vA, vB, +CCCC
    kFmt22s,        // op vA, vB, #+CCCC
    kFmt22c,        // op vA, vB, thing@CCCC
    kFmt22cs,       // [opt] op vA, vB, field goto_offset CCCC
    kFmt30t,        // op +AAAAAAAA
    kFmt32x,        // op vAAAA, vBBBB
    kFmt31i,        // op vAA, #+BBBBBBBB
    kFmt31t,        // op vAA, +BBBBBBBB
    kFmt31c,        // op vAA, string@BBBBBBBB
    kFmt35c,        // op {vC,vD,vE,vF,vG}, thing@BBBB
    kFmt35ms,       // [opt] invoke-virtual+super
    kFmt3rc,        // op {vCCCC .. v(CCCC+AA-1)}, thing@BBBB
    kFmt3rms,       // [opt] invoke-virtual+super/range
    kFmt51l,        // op vAA, #+BBBBBBBBBBBBBBBB
    kFmt35mi,       // [opt] inline invoke
    kFmt3rmi,       // [opt] inline invoke/range
    kFmt45cc,       // op {vC, vD, vE, vF, vG}, meth@BBBB, proto@HHHH
    kFmt4rcc,       // op {VCCCC .. v(CCCC+AA-1)}, meth@BBBB, proto@HHHH
} dex_instruction_format;


/* debug info opcodes and constants */
enum {
    DBG_END_SEQUENCE         = 0x00,
    DBG_ADVANCE_PC           = 0x01,
    DBG_ADVANCE_LINE         = 0x02,
    DBG_START_LOCAL          = 0x03,
    DBG_START_LOCAL_EXTENDED = 0x04,
    DBG_END_LOCAL            = 0x05,
    DBG_RESTART_LOCAL        = 0x06,
    DBG_SET_PROLOGUE_END     = 0x07,
    DBG_SET_EPILOGUE_BEGIN   = 0x08,
    DBG_SET_FILE             = 0x09,
    DBG_FIRST_SPECIAL        = 0x0a,
    DBG_LINE_BASE            = -4,
    DBG_LINE_RANGE           = 15,
};

/*
 * Direct-mapped "header_item" struct.
 */
typedef struct dex_header {
    u1  magic[8];           /* includes version number */
    u4  checksum;           /* adler32 checksum */
    u1  signature[kSHA1DigestLen]; /* SHA-1 hash */
    u4  file_size;           /* length of entire file */
    u4  header_size;         /* goto_offset to start of next section */
    u4  endian_tag;
    u4  link_size;
    u4  link_off;
    u4  map_off;
    u4  string_ids_size;
    u4  string_ids_off;
    u4  type_ids_size;
    u4  type_ids_off;
    u4  proto_ids_size;
    u4  proto_ids_off;
    u4  field_ids_size;
    u4  field_ids_off;
    u4  method_ids_size;
    u4  method_ids_off;
    u4  class_defs_size;
    u4  class_defs_off;
    u4  data_size;
    u4  data_off;
} dex_header;

/*
 * Direct-mapped "map_item".
 */
typedef struct {
    u2 type;              /* type code (see kDexType* above) */
    u2 unused;
    u4 size;              /* count of items of the indicated type */
    u4 offset;            /* file goto_offset to the start of data */
} dex_map_item;

/*
 * Direct-mapped "map_list".
 */
typedef struct {
    u4  size;               /* #of entries in list */
    dex_map_item *list;     /* entries */
} dex_map_list;

/*
 * Direct-mapped "string_id_item".
 */
typedef struct {
    u4 string_data_off;      /* file goto_offset to string_data_item */
} dex_string_id;

/*
 * Direct-mapped "type_id_item".
 */
typedef struct {
    u4  descriptor_idx; /* index into stringIds list for type desc */
} dex_type_id;

/*
 * Direct-mapped "field_id_item".
 */
typedef struct {
    u2  class_idx;           /* index into typeIds list for defining class */
    u2  type_idx;            /* index into typeIds for field type */
    u4  name_idx;            /* index into stringIds for field cname */
} dex_field_id;

/*
 * Direct-mapped "method_id_item".
 */
typedef struct {
    u2  class_idx;           /* index into typeIds list for defining class */
    u2  proto_idx;           /* index into protoIds for m prototype */
    u4  name_idx;            /* index into stringIds for m cname */
} dex_method_id;

/*
 * Direct-mapped "proto_id_item".
 */

typedef struct dex_type_list dex_type_list;
typedef struct dex_annotations_directory_item dex_annotations_directory_item;
typedef struct annotation_set_item annotation_set_item;
typedef struct dex_code_item dex_code_item;

typedef struct {
    string name;
    u2 code;
    int len;
	enum dex_instruction_format fmt;
} jd_dex_opcode;

typedef struct dex_proto_id {
    u4  shorty_idx;          /* index into stringIds for shorty desc */
    u4  return_type_idx;      /* index into typeIds list for return type */
    u4  parameters_off;      /* file goto_offset to type_list for parameter types */

    dex_type_list *type_list;
} dex_proto_id;

typedef struct {
    u1 strict_value;
    int value_type;
    int value_arg;
    size_t value_length;
    u1 *value;
} encoded_value;

typedef struct {
    u4 size;
    encoded_value *values;
} encoded_array;

typedef struct {
    u4 name_idx;
    encoded_value *value;
} annotation_element;

typedef struct {
    u4 type_idx;
    u4 size;
    annotation_element *elements;
} encoded_annotation;

typedef struct {
    u1 visibility;
    encoded_annotation *encoded_annotation;
} annotation_item;

typedef struct {
    u4 annotations_off;
    annotation_item *annotation_item;
} annotation_off_item;

typedef struct annotation_set_ref_item {
    u4 annotations_off;

    annotation_set_item *annotation;
} annotation_set_ref_item;

typedef struct annotation_set_ref_list {
    u4 size;
    annotation_set_ref_item *list;
} annotation_set_ref_list;

typedef struct annotation_set_ref_item ano_ref_item;
typedef struct annotation_set_ref_list ano_ref_list;

struct annotation_set_item{
    u4 size;
    annotation_off_item *entries;
};

typedef struct {
    u4 field_idx_diff;
    u4 access_flags;
    u4 field_id;
} encoded_field;

typedef struct {
    u4 method_idx_diff;
    u4 access_flags;
    u4 code_off;
    u4 method_id;
    dex_code_item *code;
} encoded_method;

typedef struct {
    u4 static_fields_size;
    u4 instance_fields_size;
    u4 direct_methods_size;
    u4 virtual_methods_size;

    encoded_field *static_fields;
    encoded_field *instance_fields;
    encoded_method *direct_methods;
    encoded_method *virtual_methods;
} dex_class_data_item;

/*
 * Direct-mapped "class_def_item".
 */
typedef struct {
    u4  class_idx;           /* index into typeIds for this class */
    u4  access_flags;
    u4  superclass_idx;      /* index into typeIds for superclass */
    u4  interfaces_off;      /* file goto_offset to DexTypeList */
    u4  source_file_idx;      /* index into stringIds for source file cname */
    u4  annotations_off;     /* file goto_offset to annotations_directory_item */
    u4  class_data_off;       /* file goto_offset to class_data_item */
    u4  static_values_off;    /* file goto_offset to DexEncodedArray */

    dex_type_list *interfaces;
    dex_annotations_directory_item *annotations;
    dex_class_data_item *class_data;
    encoded_array *static_values;

    bool is_inner;
    bool is_anonymous;

    list_object *inner_classes;
    list_object *anonymous_classes;
} dex_class_def;

/*
 * Direct-mapped "call_site_id_item"
 */
typedef struct {
    u4  callSiteOff;        /* file goto_offset to DexEncodedArray */
} dex_call_site_id;

/*
 * Enumeration of m handle type codes.
 */
enum method_handle_type {
    STATIC_PUT = 0x00,
    STATIC_GET = 0x01,
    INSTANCE_PUT = 0x02,
    INSTANCE_GET = 0x03,
    INVOKE_STATIC = 0x04,
    INVOKE_INSTANCE = 0x05,
    INVOKE_CONSTRUCTOR = 0x06,
    INVOKE_DIRECT = 0x07,
    INVOKE_INTERFACE = 0x08
};

/*
 * Direct-mapped "method_handle_item"
 */
typedef struct {
    u2 method_handle_type;    /* type of m handle */
    u2 reserved1;           /* reserved for future use */
    u2 field_or_method_idx;    /* index of associated field or m */
    u2 reserved2;           /* reserved for future use */
} dex_method_handle_item;

/*
 * Direct-mapped "type_item".
 */
typedef struct {
    u2  type_idx;            /* index into typeIds */
} dex_type_item;

/*
 * Direct-mapped "type_list".
 */
struct dex_type_list {
    u4  size;                     /* #of entries in list */
    dex_type_item *list;    /* entries */
};

typedef struct encoded_type_addr_pair {
    u4 type_idx;
    u4 addr;
} encoded_type_addr_pair;

typedef struct encoded_type_addr_pair encoded_tp;

typedef struct encoded_catch_handler {
    int size;
    encoded_type_addr_pair *handlers;
    u4 catch_all_addr;
    u2 handler_off;
} encoded_catch_handler;

typedef struct {
    u4 size;
    encoded_catch_handler *list;
} encoded_catch_handler_list;

typedef struct dex_try_item {
    u4 start_addr;
    u2 insn_count;
    u2 handler_off;

//    encoded_catch_handler_list *handler_list;
} dex_try_item;

typedef struct dex_try_item dex_try;

typedef struct {
    u1 code;
    u4 offset;
    u4 reg_num;
    u4 name_idx;
    u4 type_idx;
    u4 sig_idx;
} dex_dbg_item;

typedef struct {
    u4 line_start;
    u4 parameters_size;
    u4 *parameter_name;

    u4 dbg_size;
    dex_dbg_item *items;
} dex_debug_info_item;

/*
 * Direct-mapped "code_item".
 *
 * The "catches" table is used when throwing an exception,
 * "debugInfo" is used when displaying an exception stack trace or
 * debugging. An goto_offset of zero indicates that there are no entries.
 */
struct dex_code_item {
    u2  registers_size;
    u2  ins_size;
    u2  outs_size;
    u2  tries_size;
    u4  debug_info_off;       /* file goto_offset to debug info stream */
    u4  insns_size;          /* size of the insns array, in u2 units */
    u2  *insns;

    u2 padding;
    dex_try_item *tries;
    encoded_catch_handler_list *handlers;

    dex_debug_info_item *debug_info;
};

/*
 * Link table.  Currently undefined.
 */
typedef struct {
    u1  bleargh;
} dex_link;

typedef struct {
    u4 field_idx;
    u4 annotations_off;
    annotation_set_item *annotation;
} field_annotation;

typedef struct method_annotation {
    u4 method_idx;
    u4 annotations_off;
    annotation_set_item *annotation;
} method_annotation;

typedef struct method_annotation meth_ano;

typedef struct parameter_annotation {
    u4 method_idx;
    u4 annotations_off;
    annotation_set_ref_list *annotation;
} parameter_annotation;

typedef struct parameter_annotation param_ano;

/*
 * Direct-mapped "annotations_directory_item".
 */
struct dex_annotations_directory_item {
    u4  class_annotations_off;
    u4  fields_size;
    u4  methods_size;
    u4  parameters_size;

    annotation_set_item *class_annotation;
    field_annotation *field_annotations;
    method_annotation *method_annotations;
    parameter_annotation *parameter_annotations;
};

typedef dex_annotations_directory_item dex_ano_dict_item;


/*
 * Direct-mapped "encoded_array".
 *
 * NOTE: this structure is byte-aligned.
 */
struct DexEncodedArray {
    u1  *array;                   /* data in encoded_array format */
};

typedef struct {
    uint32_t utf16_size;
    char *data;
} dex_string_item;

typedef struct jd_meta_dex {
    int fd;

    string path;

    dex_header *header;

    dex_link *links;

    dex_map_list *maps;

    dex_string_id *string_ids;

    dex_type_id *type_ids;

    dex_proto_id *proto_ids;

    dex_field_id *field_ids;

    dex_method_id *method_ids;

    dex_class_def *class_defs;

    dex_string_item *strings;

    dex_method_handle_item *method_handles;

    jd_bin *bin;

    hashmap *class_type_id_map;
    hashmap *class_name_map;
    hashmap *synthetic_classes_map;
    hashmap *lambda_method_map;
    mem_pool *pool;

    string source_dir;
} jd_meta_dex;


#endif //GARLIC_DEX_H

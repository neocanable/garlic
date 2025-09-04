#ifndef GARLIC_DEX_H
#define GARLIC_DEX_H

#include "types.h"
#include "list.h"
#include "hashmap.h"
#include "decompiler/structure.h"

// https://source.android.com/docs/core/runtime/dex-format?hl=en

typedef uint8_t uleb128;

#define NO_INDEX 0xffffffff

enum { kSHA1DigestLen = 20,
    kSHA1DigestOutputLen = kSHA1DigestLen*2 +1 };

enum {
    kDexEndianConstant = 0x12345678,
    kDexNoIndex = 0xffffffff,
};

enum PrimitiveType {
    PRIM_NOT        = 0,
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

enum {
    ACC_PUBLIC       = 0x00000001,
    ACC_PRIVATE      = 0x00000002,
    ACC_PROTECTED    = 0x00000004,
    ACC_STATIC       = 0x00000008,
    ACC_FINAL        = 0x00000010,
    ACC_SYNCHRONIZED = 0x00000020,
    ACC_SUPER        = 0x00000020,
    ACC_VOLATILE     = 0x00000040,
    ACC_BRIDGE       = 0x00000040,
    ACC_TRANSIENT    = 0x00000080,
    ACC_VARARGS      = 0x00000080,
    ACC_NATIVE       = 0x00000100,
    ACC_INTERFACE    = 0x00000200,
    ACC_ABSTRACT     = 0x00000400,
    ACC_STRICT       = 0x00000800,
    ACC_SYNTHETIC    = 0x00001000,
    ACC_ANNOTATION   = 0x00002000,
    ACC_ENUM         = 0x00004000,
    ACC_CONSTRUCTOR  = 0x00010000,
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

enum {
    kDexVisibilityBuild         = 0x00,
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

    kDexAnnotationValueTypeMask = 0x1f,
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

enum {
    kDexChunkClassLookup            = 0x434c4b50,
    kDexChunkRegisterMaps           = 0x524d4150,
    kDexChunkEnd                    = 0x41454e44,
};

typedef enum dex_instruction_format {
    kFmt00x = 0,
    kFmt10x,
    kFmt12x,
    kFmt11n,
    kFmt11x,
    kFmt10t,
    kFmt20bc,
    kFmt20t,
    kFmt22x,
    kFmt21t,
    kFmt21s,
    kFmt21h,
    kFmt21c,
    kFmt23x,
    kFmt22b,
    kFmt22t,
    kFmt22s,
    kFmt22c,
    kFmt22cs,
    kFmt30t,
    kFmt32x,
    kFmt31i,
    kFmt31t,
    kFmt31c,
    kFmt35c,
    kFmt35ms,
    kFmt3rc,
    kFmt3rms,
    kFmt51l,
    kFmt35mi,
    kFmt3rmi,
    kFmt45cc,
    kFmt4rcc,
} dex_instruction_format;

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

typedef struct dex_header {
    u1  magic[8];
    u4  checksum;
    u1  signature[kSHA1DigestLen];
    u4  file_size;
    u4  header_size;
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

typedef struct {
    u2 type;
    u2 unused;
    u4 size;
    u4 offset;
} dex_map_item;

typedef struct {
    u4  size;
    dex_map_item *list;
} dex_map_list;

typedef struct {
    u4 string_data_off;
} dex_string_id;

typedef struct {
    u4  descriptor_idx;
} dex_type_id;

typedef struct {
    u2  class_idx;
    u2  type_idx;
    u4  name_idx;
} dex_field_id;

typedef struct {
    u2  class_idx;
    u2  proto_idx;
    u4  name_idx;
} dex_method_id;

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
    u4  shorty_idx;
    u4  return_type_idx;
    u4  parameters_off;

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

typedef struct dex_class_def {
    u4  class_idx;
    u4  access_flags;
    u4  superclass_idx;
    u4  interfaces_off;
    u4  source_file_idx;
    u4  annotations_off;
    u4  class_data_off;
    u4  static_values_off;

    dex_type_list *interfaces;
    dex_annotations_directory_item *annotations;
    dex_class_data_item *class_data;
    encoded_array *static_values;

    bool is_inner;
    bool is_anonymous;

    list_object *inner_classes;
    list_object *anonymous_classes;
} dex_class_def;

typedef struct dex_class_def dex_class;

typedef struct {
    u4  callSiteOff;
} dex_call_site_id;

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

typedef struct {
    u2 method_handle_type;
    u2 reserved1;
    u2 field_or_method_idx;
    u2 reserved2;
} dex_method_handle_item;

typedef struct {
    u2  type_idx;
} dex_type_item;

struct dex_type_list {
    u4  size;
    dex_type_item *list;
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

struct dex_code_item {
    u2  registers_size;
    u2  ins_size;
    u2  outs_size;
    u2  tries_size;
    u4  debug_info_off;
    u4  insns_size;
    u2  *insns;

    u2 padding;
    dex_try_item *tries;
    encoded_catch_handler_list *handlers;

    dex_debug_info_item *debug_info;
};

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

struct DexEncodedArray {
    u1  *array;
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

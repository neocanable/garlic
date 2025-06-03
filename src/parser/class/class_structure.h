#ifndef GARLIC_CLASS_STRUCTURE_H
#define GARLIC_CLASS_STRUCTURE_H

#include "libs/list/list.h"
#include "libs/hashmap/hashmap.h"
#include "parser/reader/reader.h"

enum field_access_flag_enum {
    FIELD_ACC_PUBLIC	    = 0x0001,
    FIELD_ACC_PRIVATE	    = 0x0002,
    FIELD_ACC_PROTECTED	    = 0x0004,
    FIELD_ACC_STATIC	    = 0x0008,
    FIELD_ACC_FINAL	        = 0x0010,
    FIELD_ACC_VOLATILE	    = 0x0040,
    FIELD_ACC_TRANSIENT	    = 0x0080,
    FIELD_ACC_SYNTHETIC	    = 0x1000,
    FIELD_ACC_ENUM	        = 0x4000
};

enum method_access_flag_enum {
    METHOD_ACC_PUBLIC        = 0x0001,
    METHOD_ACC_PRIVATE       = 0x0002,
    METHOD_ACC_PROTECTED     = 0x0004,
    METHOD_ACC_STATIC        = 0x0008,
    METHOD_ACC_FINAL         = 0x0010,
    METHOD_ACC_SYNCHRONIZED  = 0x0020,
    METHOD_ACC_BRIDGE        = 0x0040,
    METHOD_ACC_VARARGS       = 0x0080,
    METHOD_ACC_NATIVE        = 0x0100,
    METHOD_ACC_ABSTRACT      = 0x0400,
    METHOD_ACC_STRICT        = 0x0800,
    METHOD_ACC_SYNTHETIC     = 0x1000
};

enum class_access_flag_enum {
    CLASS_ACC_PUBLIC        = 0x0001,
    CLASS_ACC_FINAL         = 0x0010,
    CLASS_ACC_SUPER         = 0x0020,
    CLASS_ACC_INTERFACE     = 0x0200,
    CLASS_ACC_ABSTRACT      = 0x0400,
    CLASS_ACC_SYNTHETIC     = 0x1000,
    CLASS_ACC_ANNOTATION    = 0x2000,
    CLASS_ACC_ENUM          = 0x4000,
    CLASS_ACC_MODULE        = 0x8000
};

typedef enum {
    CONST_UTF8_TAG               = 1,
    CONST_INTEGER_TAG            = 3,
    CONST_FLOAT_TAG              = 4,
    CONST_LONG_TAG               = 5,
    CONST_DOUBLE_TAG             = 6,
    CONST_CLASS_TAG              = 7,
    CONST_STRING_TAG             = 8,
    CONST_FIELDREF_TAG           = 9,
    CONST_METHODREF_TAG          = 10,
    CONST_INTERFACEMETHODREF_TAG = 11,
    CONST_NAMEANDTYPE_TAG        = 12,
    CONST_METHODHANDLE_TAG       = 15,
    CONST_METHODTYPE_TAG         = 16,
    CONST_DYNAMIC_TAG            = 17,
    CONST_INVOKEDYNAMIC_TAG      = 18,
    CONST_MODULE_TAG             = 19,
    CONST_PACKAGE_TAG            = 20,
} constant_pool_tag;

typedef struct {
    u2 name_index;
} jconst_class;

typedef struct {
    u2 class_index;
    u2 name_and_type_index;
} jconst_fieldref;

typedef struct {
    u2 class_index;
    u2 name_and_type_index;
} jconst_methodref;

typedef struct {
    u2 class_index;
    u2 name_and_type_index;
} jconst_interface_methodref;

typedef struct {
    u2 string_index;
} jconst_string;

typedef struct {
    u4 bytes;
} jconst_integer;

typedef struct {
    u4 bytes;
} jconst_float;

typedef struct {
    u4 high_bytes;
    u4 low_bytes;
} jconst_long;

typedef struct {
    u4 high_bytes;
    u4 low_bytes;
} jconst_double;

typedef struct {
    u2 name_index;
    u2 descriptor_index;
} jconst_name_and_type;

typedef struct {
    u2 length;
    u1 *bytes;
} jconst_utf8;

typedef struct {
    u1 reference_kind;
    u2 reference_index;
} jconst_method_handle;

typedef struct {
    u2 descriptor_index;
} jconst_method_type;

typedef struct {
    u2 bootstrap_method_attr_index;
    u2 name_and_type_index;
} jconst_dynamic;

typedef struct {
    u2 bootstrap_method_attr_index;
    u2 name_and_type_index;
} jconst_invoke_dynamic;

typedef struct {
    u2 name_index;
} jconst_module;

typedef struct {
    u2 name_index;
} jconst_package;

typedef union {
    jconst_utf8                 *utf8;
    jconst_class                *class;
    jconst_integer              *integer;
    jconst_float                *float_info;
    jconst_double               *double_info;
    jconst_long                 *long_info;
    jconst_dynamic              *dynamic;
    jconst_fieldref             *fieldref;
    jconst_interface_methodref  *interface_methodref;
    jconst_invoke_dynamic       *invoke_dynamic;
    jconst_method_handle        *method_handle;
    jconst_method_type          *method_type;
    jconst_methodref            *methodref;
    jconst_module               *module;
    jconst_name_and_type        *name_and_type;
    jconst_package              *package;
    jconst_string               *string_info;
} jconst_union_info;

typedef struct {
    u1                          tag;
    char                        *name;
    char                        *readable;
    jconst_union_info           *info;
} jcp_info;


/*
 * attribute defination here
 * */

typedef struct {
    u2      name_index;
    u4      length;
    string  name;
    u1      *info;
} jattr;

typedef struct {
    u2 value_index;
} jattr_constant_value;

typedef struct {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
} jattr_code_exception_table;

typedef struct {
    u2                          max_stack;
    u2                          max_locals;
    u4                          code_length;
    u1                          *code;
    u2                          exception_table_length;
    u2                          attributes_count;
    jattr                       *attributes;
    jattr_code_exception_table  *exception_table;
} jattr_code;

typedef struct {
    u2 number_of_exceptions;
    u2 *exception_index_table;
} jattr_exception;

typedef struct {
    u2 sourcefile_index;
} jattr_source_file;

typedef struct {
    u2 start_pc;
    u2 line_number;
} jattr_line_number;

typedef struct {
    u2                  line_number_table_length;
    jattr_line_number   *line_number_table;
} jattr_line_number_table;

typedef struct jattr_local_variable {
    u2 start_pc;
    u2 length;
    u2 name_index;
    u2 descriptor_index;
    u2 index;
} jattr_local_variable;

typedef struct jattr_local_type_variable {
    u2 start_pc;
    u2 length;
    u2 name_index;
    u2 signature_index;
    u2 index;
} jattr_local_type_variable;

typedef struct jattr_local_variable jattr_lv;
typedef struct jattr_local_type_variable jattr_ltv;

typedef struct jattr_local_variable_table {
    u2                      local_variable_table_length;
    jattr_local_variable    *local_variable_table;
} jattr_local_variable_table;

typedef struct jattr_local_variable_table jattr_lvt;

typedef struct jattr_local_variable_type_table {
    u2                           local_variable_type_table_length;
    jattr_local_type_variable    *local_variable_type_table;
} jattr_local_variable_type_table;

typedef struct jattr_local_variable_type_table jattr_lvtt;

typedef struct {
    u2 inner_class_info_index;
    u2 outer_class_info_index;
    u2 inner_name_index;
    u2 inner_class_access_flags;
} jattr_inner_class;

typedef struct {
    u2                  number_of_classes;
    jattr_inner_class   *classes;
} jattr_inner_classes;

typedef struct {
} jattr_synthetic;

typedef struct {
} jattr_deprecated;

typedef struct {
    u2 class_index;
    u2 method_index;
} jattr_enclosing_method;

typedef struct {
    u2 signature_index;
} jattr_signature;

typedef struct {
    u1 *debug_extension;
} jattr_source_debug_extension;


typedef struct element_value element_value;
typedef struct annotation annotation;

typedef struct {
    u2 type_name_index;
    u2 const_name_index;
} element_value_enum_const_value;

typedef struct {
    u2              num_values;
    element_value   *values;
} element_value_array_value;

typedef union {
    u2                              const_value_index;
    element_value_enum_const_value  *enum_const_value;

    u2                              class_info_index;
    annotation                      *annotation_value;
    element_value_array_value       *array_value;
} element_value_union;

struct element_value {
    u1                              tag;
    element_value_union             *union_value;
};

typedef struct annotation_element_value_pairs {
    u2                              element_name_index;
    element_value                   *value;
} annotation_element_value_pairs;

typedef struct annotation_element_value_pairs anno_evp;


struct annotation {
    u2                              type_index;
    u2                              num_element_value_pairs;
    annotation_element_value_pairs  *element_value_pairs;
};

typedef struct {
    u2                              num_annotations;
    annotation                      *annotations;
} jattr_parameters_annotations;

typedef struct {
    // runtime_visible_annotations
    u2                              num_annotations;
    annotation                      *annotations;
} jattr_runtime_visible_annotations;

typedef struct jattr_runtime_invisible_annotations {
    // runtime_invisible_annotations
    u2                              num_annotations;
    annotation                      *annotations;
} jattr_runtime_invisible_annotations;

typedef struct jattr_runtime_invisible_annotations jattr_ria;


typedef struct jattr_runtime_visible_parameter_annotations {
    u1                              num_parameters;
    jattr_parameters_annotations          *annotations;
} jattr_runtime_visible_parameter_annotations;

typedef struct jattr_runtime_visible_parameter_annotations jattr_rvpa;

typedef struct {
    u1                              num_parameters;
    jattr_parameters_annotations          *annotations;
} jattr_runtime_invisible_parameter_annotations;

typedef struct {
    element_value *default_value;
} jattr_annotation_default;

typedef struct {
    u2 bootstrap_method_ref;
    u2 num_bootstrap_arguments;
    u2 *bootstrap_arguments;
} jclass_bootstrap_method;

typedef struct {
    u2                          num_bootstrap_methods;
    jclass_bootstrap_method     *bootstrap_methods;
} jattr_bootstrap_methods;


typedef struct variable_info {
    u1 tag;
    u2 offset;
} variable_info;

typedef struct variable_info Top_variable_info;
typedef struct variable_info Integer_variable_info;
typedef struct variable_info Float_variable_info;
typedef struct variable_info Long_variable_info;
typedef struct variable_info Double_variable_info;
typedef struct variable_info Null_variable_info;
typedef struct variable_info UninitializedThis_variable_info;
typedef struct variable_info Object_variable_info;
typedef struct variable_info Uninitialized_variable_info;

union verification_type_info {
    Top_variable_info               top_variable_info;
    Integer_variable_info           integer_variable_info;
    Float_variable_info             float_variable_info;
    Long_variable_info              long_variable_info;
    Double_variable_info            double_variable_info;
    Null_variable_info              null_variable_info;
    UninitializedThis_variable_info uninitializedThis_variable_info;
    Object_variable_info            object_variable_info;
    Uninitialized_variable_info     uninitialized_variable_info;
};

typedef struct {
    // = SAME; /* 0-63 */
    u1 frame_type;
} same_frame;

typedef struct {
    // = SAME_LOCALS_1_STACK_ITEM; /* 64-127 */
    u1              frame_type;
    variable_info   *stack;
} same_locals_1_stack_item_frame;

typedef struct {
    // = SAME_LOCALS_1_STACK_ITEM_EXTENDED; /* 247 */
    u1              frame_type;
    u2              offset_delta;
    variable_info   *stack;
} same_locals_1_stack_item_frame_extended;

typedef struct {
    // = CHOP; /* 248-250 */
    u1 frame_type;
    u2 offset_delta;
} chop_frame;

typedef struct {
    // = SAME_FRAME_EXTENDED; /* 251 */
    u1 frame_type;
    u2 offset_delta;
} same_frame_extended;

typedef struct {
    // = APPEND; /* 252-254 */
    u1              frame_type;
    u2              offset_delta;
    variable_info   *locals;
} append_frame;

typedef struct {
    //= FULL_FRAME; /* 255 */
    u1              frame_type;
    u2              offset_delta;
    u2              number_of_locals;
    variable_info   *locals;
    u2              number_of_stack_items;
    variable_info   *stack;
} full_frame;

typedef union {
    same_frame                              *same_frame;
    same_locals_1_stack_item_frame          *same_locals_1_stack_item_frame;
    same_locals_1_stack_item_frame_extended *same_locals_1_stack_item_frame_extended;
    chop_frame                              *chop_frame;
    same_frame_extended                     *same_frame_extended;
    append_frame                            *append_frame;
    full_frame                              *full_frame;
} stack_map_frame;

typedef struct {
    u2              number_of_entries;
    stack_map_frame *entries;
} jattr_stack_map_table;

typedef struct {
    u1 type_path_kind;
    u1 type_argument_index;
} type_path_list;

typedef struct {
    u1              path_length;
    type_path_list *path;
} type_path;

typedef struct {
    u1 type_parameter_index;
} type_parameter_target;

typedef struct {
    u2 supertype_index;
} supertype_target;

typedef struct {
    u1 type_parameter_index;
    u1 bound_index;
} type_parameter_bound_target;

typedef struct empty_target {
} empty_target;

typedef struct {
    u1 formal_parameter_index;
} formal_parameter_target;

typedef struct {
    u2 throws_type_index;
} throws_target;

typedef struct {
    u2 start_pc;
    u2 length;
    u2 index;
} localvar_target_table;

typedef struct {
    u2 table_length;
    localvar_target_table *table;
} localvar_target;

typedef struct {
    u2 exception_table_index;
} catch_target;

typedef struct {
    u2 offset;
} offset_target;

typedef struct {
    u2 offset;
    u1 type_argument_index;
} type_argument_target;

typedef union {
    type_parameter_target       *type_parameter_target;
    supertype_target            *supertype_target;
    type_parameter_bound_target *type_parameter_bound_target;
    empty_target                *empty_target;
    formal_parameter_target     *formal_parameter_target;
    throws_target               *throws_target;
    localvar_target             *localvar_target;
    catch_target                *catch_target;
    offset_target               *offset_target;
    type_argument_target        *type_argument_target;
} type_annotation_target_info;

typedef struct {
    u1                              target_type;
    type_annotation_target_info     *target_info;
    type_path                       *target_path;
    u2                              type_index;
    u2                              num_element_value_pairs;
    annotation_element_value_pairs  *element_value_pairs;
} type_annotation;

typedef struct jattr_runtime_visible_type_annotations{
    // runtime_visible_type_annotations
    u2              num_annotations;
    type_annotation *annotations;
} jattr_runtime_visible_type_annotations;

typedef struct jattr_runtime_visible_type_annotations jattr_rvta;

typedef struct jattr_runtime_invisible_type_annotations {
    // runtime_invisible_type_annotations
    u2              num_annotations;
    type_annotation *annotations;
} jattr_runtime_invisible_type_annotations;

typedef struct jattr_runtime_invisible_type_annotations jattr_rita;

typedef struct {
    u2 name_index;
    u2 access_flags;
} jattr_method_parameter;

typedef struct {
    u1                      parameters_count;
    jattr_method_parameter *parameters;
} jattr_method_parameters;

typedef struct {
    u2 requires_index;
    u2 requires_flags;
    u2 requires_version_index;
} module_attr_requires;

typedef struct {
    u2 exports_index;
    u2 exports_flags;
    u2 exports_to_count;
    u2 *exports_to_index;
} module_attr_exports;

typedef struct {
    u2 opens_index;
    u2 opens_flags;
    u2 opens_to_count;
    u2 *opens_to_index;
} module_attr_opens;

typedef struct {
    u2 provides_index;
    u2 provides_with_count;
    u2 *provides_with_index;
} module_attr_provides;

typedef struct {
    u2                      module_name_index;
    u2                      module_flags;
    u2                      module_version_index;

    u2                      requires_count;
    module_attr_requires    *requires;

    u2                      exports_count;
    module_attr_exports     *exports;

    u2                      opens_count;
    module_attr_opens       *opens;

    u2                      uses_count;
    u2                      *uses_index;

    u2                      provides_count;
    module_attr_provides    *provides;
} jattr_module;

typedef struct {
    u2 package_count;
    u2 *package_index;
} jattr_module_packages;

typedef struct {
    u2 main_class_index;
} jattr_module_main_class;

typedef struct {
    u2 host_class_index;
} jattr_nest_host;

typedef struct {
    u2 number_of_classes;
    u2 *classes;
} jattr_nest_members;

typedef struct {
    u2              name_index;
    u2              descriptor_index;
    u2              attributes_count;
    jattr      *attributes;
} jattr_record_component;

typedef struct {
    u2                      component_count;
    jattr_record_component  *components;
} jattr_record;

typedef struct {
    u2 number_of_classes;
    u2 *classes;
} jattr_permitted_subclasses;

typedef struct {
    string name;
    string class_name;
    jattr *attr;
} string_annotation;

typedef struct jfield {
    u2              access_flags;
    u2              name_index;
    u2              descriptor_index;
    u2              attributes_count;
    jattr      *attributes;
} jfield;

typedef struct jmethod {
    u2              access_flags;
    u2              name_index;
    u2              descriptor_index;
    u2              attributes_count;
    jattr_code      *code_attribute;
    jattr           *attributes;
} jmethod;


/*
 *  Custom struct
 * */
typedef struct jclass_file jclass_file;
typedef struct jsource_file jsource_file;


struct jclass_file {
    int            fd;
    char           *path;
    u4             magic;
    u2             minor_version;
    u2             major_version;
    u2             constant_pool_count;
    jcp_info       *constant_pool;
    u2             access_flags;
    u2             this_class;
    u2             super_class;
    u2             interfaces_count;
    u2             *interfaces;
    u2             fields_count;
    jfield         *fields;
    u2             methods_count;
    jmethod        *methods;
    u2             attributes_count;
    jattr          *attributes;

    jsource_file   *jfile;
    jd_bin         *bin;

    // for opcode hashmap
    hashmap *class_opcode;
};


#endif //GARLIC_CLASS_STRUCTURE_H

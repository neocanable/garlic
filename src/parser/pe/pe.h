#ifndef GARLIC_PE_H
#define GARLIC_PE_H

#include "common/types.h"
#include "libs/memory/mem_pool.h"
#include "parser/pe/pe_const.h"
#include "parser/reader/reader.h"
#include "mem_pool.h"

typedef struct {
    u2 magic;       // MZ
    u2 e_cblp;      // bytes on last page of file
    u2 e_cp;        // pages in file
    u2 e_crlc;      // relocations
    u2 e_cparhdr;   // size of dos_header in paragraphs
    u2 e_minalloc;  // minimum extra paragraphs needed
    u2 e_maxalloc;  // maximum extra paragraphs needed
    u2 e_ss;        // initial (relative) SS value
    u2 e_sp;        // initial SP value
    u2 e_csum;      // checksum
    u2 e_ip;        // initial IP value
    u2 e_cs;        // initial (relative) CS value
    u2 e_lfarlc;    // file address of relocation table
    u2 e_ovno;      // overlay number
    u2 e_res[4];    // reserved words
    u2 e_oemid;     // OEM identifier (for e_oeminfo)
    u2 e_oeminfo;   // OEM information; e_oemid specific
    u2 e_res2[10];      // reserved words
    u4 e_lfanew;    // goto_offset to start of PE dos_header
} pe_dos_header;

typedef struct {
    u4 size;
    u1 *data;
} pe_dos_stub;

typedef struct {
    u4 signature;
    u2 machine;
    u2 number_of_sections;
    u4 time_stamp;
    u4 pointer_to_symbol_table;
    u4 number_of_symbols;
    u2 size_of_optional_header;
    u2 characteristics;
} pe_nt_header;

typedef struct {
    u4 virtual_address; /* 数据目录的 RVA */
    u4 size;            /* 数据目录的大小 */
} pe_data_directory;

/**
 * IMAGE_EXPORT_DIRECTORY
 **/
typedef struct {
    u4 characteristics;
    u4 time_date_stamp;
    u2 major_version;
    u2 minor_version;
    u4 name;
    u4 base;
    u4 number_of_functions;
    u4 number_of_names;
    u4 address_of_functions;
    u4 address_of_names;
    u4 address_of_name_ordinals;

    string str_name;
    u4 *functions;
    u4 *names;
    u2 *name_ordinals;
} pe_export_directory;

/**
 * IMAGE_IMPORT_DESCRIPTOR
 **/
typedef struct {
    u2 hint;
    string name;
} pe_import_by_name;

typedef struct {
    union {
        u8 forwarder_string;
        u8 function;
        u8 ordinal;
        u8 address_of_data;
    } u1;
    pe_import_by_name *name;
} pe_import_trunk_data;

typedef struct {
    u4 original_first_thunk;
    u4 time_date_stamp;
    u4 forwarder_chain;
    u4 name;
    u4 first_thunk;

    u2 trunk_size;
    pe_import_trunk_data *trunks;
} pe_import_directory;

/**
 * IMAGE_RESOURCE_DIRECTORY
 **/

typedef struct {
    u2 length;
    string name_string;
} pe_resource_directory_string;

typedef struct {
    u2 length;
    string name_string; // TODO: wchar_t
} pe_resource_dir_string_u;

typedef struct {
    union {
        struct {
            u4 name_offset:31;
            u4 name_is_string:1;
        } dummystructname;
        u4 name;
        u4 id;
    } dummyunionname;

    union {
        u4 offset_to_data;
        struct {
            u4 offset_to_directory:31;
            u4 data_is_directory:1;
        } dummystructname2;
    } dummyunionname2;
} pe_resource_directory_entry;

typedef struct {
    u4 characteristics;
    u4 time_date_stamp;
    u2 major_version;
    u2 minor_version;
    u2 number_of_named_entries;
    u2 number_of_id_entries;
} pe_resource_directory;

typedef enum ReplacesCorHdrNumericDefines
{
// COM+ Header entry point flags.
    COMIMAGE_FLAGS_ILONLY               = 0x00000001,
    COMIMAGE_FLAGS_32BITREQUIRED        = 0x00000002,
    COMIMAGE_FLAGS_IL_LIBRARY           = 0x00000004,
    COMIMAGE_FLAGS_STRONGNAMESIGNED     = 0x00000008,
// DDBLD - Added Next Line - Still verifying general usage
    COMIMAGE_FLAGS_NATIVE_ENTRYPOINT    = 0x00000010,
// DDBLD - End of Add
    COMIMAGE_FLAGS_TRACKDEBUGDATA       = 0x00010000,

// Other kinds of flags follow

} ReplacesCorHdrNumericDefines;

typedef struct {
    u4 cb;
    u2 major_runtime_version;
    u2 minor_runtime_version;
    pe_data_directory meta_data;
    u4 flags;
    union {
        u4 entry_point_token;
        u4 entry_point_rva;
    } eu;
    pe_data_directory resources;
    pe_data_directory strong_name_signature;
    pe_data_directory code_manager_table;
    pe_data_directory vtable_fixups;
    pe_data_directory export_address_table_jumps;
    pe_data_directory managed_native_header;
} pe_cor20_header;

typedef struct {
    u4 offset;
    u4 size;
    string name; // 4 byte align
} pe_metadata_stream_header;

typedef struct {
    u4 signature;
    u2 major_version;
    u2 minor_version;
    u4 reserved;
    u4 version_length;
    string version;
    u1 padding;
    u2 flags;
    u2 streams_count;
    pe_metadata_stream_header *stream_headers;
} pe_metadata_header;

typedef struct {
    u4 reserved;
    u1 major_version;
    u1 minor_version;
    u1 heap_offset_sizes;
    u1 reserved2;
    u8 valid;
    u8 sorted;
} pe_metadata_tables_header;


//00 - Module                  01 - TypeRef                02 - TypeDef
//04 - Field                   06 - MethodDef              08 - Param
//09 - InterfaceImpl           10 - MemberRef              11 - Constant
//12 - CustomAttribute         13 - FieldMarshal           14 - DeclSecurity
//15 - ClassLayout             16 - FieldLayout            17 - StandAloneSig
//18 - EventMap                20 - Event                  21 - PropertyMap
//23 - Property                24 - MethodSemantics        25 - MethodImpl
//26 - ModuleRef               27 - TypeSpec               28 - ImplMap
//29 - FieldRVA                32 - Assembly               33 - AssemblyProcessor
//34 - AssemblyOS              35 - AssemblyRef            36 - AssemblyRefProcessor
//37 - AssemblyRefOS           38 - File                   39 - ExportedType
//40 - ManifestResource        41 - NestedClass            42 - GenericParam
//44 - GenericParamConstraint

#define PE_METATABLE_MODULE                     0x01
#define PE_METATABLE_TYPE_REF                   0x02
#define PE_METATABLE_TYPE_DEF                   0x04
#define PE_METATABLE_RESERVED1                  0x08
#define PE_METATABLE_FIELD                      0x10
#define PE_METATABLE_RESERVED2                  0x20
#define PE_METATABLE_METHOD                     0x40
#define PE_METATABLE_RESERVED3                  0x80
#define PE_METATABLE_PARAM                      0x100
#define PE_METATABLE_INTERFACE_IMPL             0x200
#define PE_METATABLE_MEMBER_REF                 0x400
#define PE_METATABLE_CONSTANT                   0x800
#define PE_METATABLE_CUSTOM_ATTRIBUTE           0x1000
#define PE_METATABLE_FIELD_MARSHAL              0x2000
#define PE_METATABLE_DECL_SECURITY              0x4000
#define PE_METATABLE_CLASS_LAYOUT               0x8000
#define PE_METATABLE_FIELD_LAYOUT               0x10000
#define PE_METATABLE_STAND_ALONE_SIG            0x20000
#define PE_METATABLE_EVENT_MAP                  0x40000
#define PE_METATABLE_RESERVED4                  0x80000
#define PE_METATABLE_EVENT                      0x100000
#define PE_METATABLE_PROPERTY_MAP               0x200000
#define PE_METATABLE_RESERVED5                  0x400000
#define PE_METATABLE_PROPERTY                   0x800000
#define PE_METATABLE_METHOD_SEMANTICS           0x1000000
#define PE_METATABLE_METHOD_IMPL                0x2000000
#define PE_METATABLE_MODULE_REF                 0x4000000
#define PE_METATABLE_TYPE_SPEC                  0x8000000
#define PE_METATABLE_IMPL_MAP                   0x10000000
#define PE_METATABLE_FIELD_RVA                  0x20000000
#define PE_METATABLE_RESERVED6                  0x40000000
#define PE_METATABLE_RESERVED7                  0x80000000
#define PE_METATABLE_ASSEMBLY                   0x100000000
#define PE_METATABLE_ASSEMBLY_PROCESSOR         0x200000000
#define PE_METATABLE_ASSEMBLY_OS                0x400000000
#define PE_METATABLE_ASSEMBLY_REF               0x800000000
#define PE_METATABLE_ASSEMBLY_REF_PROCESSOR     0x1000000000
#define PE_METATABLE_ASSEMBLY_REF_OS            0x2000000000
#define PE_METATABLE_FILE                       0x4000000000
#define PE_METATABLE_EXPORTED_TYPE              0x8000000000
#define PE_METATABLE_MANIFEST_RESOURCE          0x10000000000
#define PE_METATABLE_NESTED_CLASS               0x20000000000
#define PE_METATABLE_GENERIC_PARAM              0x40000000000
#define PE_METATABLE_METHOD_SPEC                0x80000000000
#define PE_METATABLE_GENERIC_PARAM_CONSTRAINT   0x100000000000

static u8 pe_meta_table_flags[] = {
        PE_METATABLE_MODULE,
        PE_METATABLE_TYPE_REF,
        PE_METATABLE_TYPE_DEF,
        PE_METATABLE_RESERVED1,
        PE_METATABLE_FIELD,
        PE_METATABLE_RESERVED2,
        PE_METATABLE_METHOD,
        PE_METATABLE_RESERVED3,
        PE_METATABLE_PARAM,
        PE_METATABLE_INTERFACE_IMPL,
        PE_METATABLE_MEMBER_REF ,
        PE_METATABLE_CONSTANT ,
        PE_METATABLE_CUSTOM_ATTRIBUTE,
        PE_METATABLE_FIELD_MARSHAL,
        PE_METATABLE_DECL_SECURITY,
        PE_METATABLE_CLASS_LAYOUT,
        PE_METATABLE_FIELD_LAYOUT,
        PE_METATABLE_STAND_ALONE_SIG,
        PE_METATABLE_EVENT_MAP,
        PE_METATABLE_RESERVED4,
        PE_METATABLE_EVENT,
        PE_METATABLE_PROPERTY_MAP,
        PE_METATABLE_RESERVED5,
        PE_METATABLE_PROPERTY,
        PE_METATABLE_METHOD_SEMANTICS,
        PE_METATABLE_METHOD_IMPL,
        PE_METATABLE_MODULE_REF,
        PE_METATABLE_TYPE_SPEC,
        PE_METATABLE_IMPL_MAP,
        PE_METATABLE_FIELD_RVA,
        PE_METATABLE_RESERVED6,
        PE_METATABLE_RESERVED7,
        PE_METATABLE_ASSEMBLY,
        PE_METATABLE_ASSEMBLY_PROCESSOR,
        PE_METATABLE_ASSEMBLY_OS,
        PE_METATABLE_ASSEMBLY_REF,
        PE_METATABLE_ASSEMBLY_REF_PROCESSOR,
        PE_METATABLE_ASSEMBLY_REF_OS,
        PE_METATABLE_FILE,
        PE_METATABLE_EXPORTED_TYPE,
        PE_METATABLE_MANIFEST_RESOURCE,
        PE_METATABLE_NESTED_CLASS,
        PE_METATABLE_GENERIC_PARAM,
        PE_METATABLE_METHOD_SPEC,
        PE_METATABLE_GENERIC_PARAM_CONSTRAINT,
};

typedef struct {
    u8 table_name_mask;
    u4 size;
} pe_metadata_table;

typedef struct pe_module_table {
    u2 generation;
    u4 name; // index to string
    u4 mvid; // index to guid
    u4 enc_id; // index to guid
    u4 enc_base_id; // index to guid
} pe_module_table;

typedef struct pe_type_ref_table {
    u4 resolution_scope;
    u4 name; // index to string
    u4 namespace; // index to string
} pe_type_ref_table;

typedef struct {
    u4 flags;
    u4 type_name; // index to string
    u4 type_namespace; // index to string
    u4 extends; // index to type_def/type_ref/type_spec

    // field_list, method_list都是指向一个field/method的索引
    // field_list和method_list有多少个field/m
    // 需要下一个typedef的field_list和method_list来确定
    // II.22.15 ECMA-335
    u4 field_list; // index to field
    u4 method_list; // index to method_def
} pe_type_def_table;

typedef struct {
    u2 flags;
    u4 name; // index to string
    u4 signature; // index to blob
} pe_field_table;

typedef struct {
    u4 rva;
    u2 impl_flags;
    u4 name; // index to string
    u4 signature; // index to blob
    u4 param_list; // index to param
} pe_method_def_table;

typedef struct {
    u2 flags;
    u2 sequence;
    u4 name; // index to string
} pe_param_table;

typedef struct {
    u4 class; // index to type_def
    u4 interface; // index to type_def
} pe_interface_impl_table;

typedef struct {
    u4 klass;
    u4 name; // index to string
    u4 signature; // index to blob
} pe_member_ref_table;

typedef struct {
    u1 type;
    u1 padding;
    u4 parent;
    u4 value; // index to blob
} pe_constant_table;

typedef struct {
    u4 parent;
    u4 type;
    u4 value; // index to blob
} pe_custom_attribute_table;

typedef struct {
    u4 parent;
    u4 native_type; // index to blob
} pe_field_marshal_table;

typedef struct {
    u2 action;
    u4 parent;
    u4 permission_set; // index to blob
} pe_decl_security_table;

typedef struct {
    u2 packing_size;
    u4 class_size;
    u4 parent; // index to type_def
} pe_class_layout_table;

typedef struct {
    u4 offset;
    u4 field; // index to field
} pe_field_layout_table;

typedef struct {
    u4 signature;
} pe_stand_alone_sig_table;

typedef struct {
    u4 parent; // index to type_def
    u4 event_list; // index to event
} pe_event_map_table;

typedef struct {
    u2 event_flags;
    u4 name; // index to string
    u4 event_type; // index to type_def/type_ref/type_spec
} pe_event_table;

typedef struct {
    u4 parent; // index to type_def
    u4 property_list; // index to property
} pe_property_map_table;

typedef struct {
    u2 flags;
    u4 name; // index to string
    u4 type; // index to blob
} pe_property_table;

typedef struct {
    u2 semantics;
    u4 method; // index to method_def
    u4 association; // index to event/property
} pe_method_semantics_table;

typedef struct {
    u4 klass; // index to type_def
    u4 method_body; // index to method_def
    u4 method_declaration; // index to method_def
} pe_method_impl_table;

typedef struct {
    u4 name; // index to string
} pe_module_ref_table;

typedef struct {
    u4 signature; // index to blob
} pe_type_spec_table;

typedef struct {
    u2 mapping_flags;
    u4 member_forwarded; // index to field/m
    u4 import_name; // index to string
    u4 import_scope; // index to module_ref
} pe_impl_map_table;

typedef struct {
    u4 rva;
    u4 field; // index to field
} pe_field_rva_table;

typedef struct {
    u4 hash_alg_id;
    u2 major_version;
    u2 minor_version;
    u2 build_number;
    u2 revision_number;
    u4 flags;
    u4 public_key;
    u4 name; // index to string
    u4 culture; // index to string
} pe_assembly_table;

typedef struct {
    u4 processor;
} pe_assembly_processor_table;

typedef struct {
    u4 os_platform_id;
    u4 os_major_version;
    u4 os_minor_version;
} pe_assembly_os_table;

typedef struct {
    u2 major_version;
    u2 minor_version;
    u2 build_number;
    u2 revision_number;
    u4 flags;
    u4 public_key_or_token; // blob
    u4 name; // index to string
    u4 culture; // index to string
    u4 hash_value; // blob
} pe_assembly_ref_table;

typedef struct {
    u4 processor;
    u4 assembly_ref; // index to assembly_ref
} pe_assembly_ref_processor_table;

typedef struct {
    u4 os_platform_id;
    u4 os_major_version;
    u4 os_minor_version;
    u4 assembly_ref; // index to assembly_ref
} pe_assembly_ref_os_table;

typedef struct {
    u4 flags;
    u4 name; // index to string
    u4 hash_value; // index to blob
} pe_file_table;

typedef struct {
    u4 flags;
    u4 type_def_id; // index to type_def
    u4 type_name; // index to string
    u4 type_namespace; // index to string
    u4 implementation; // index to implementation
} pe_exported_type_table;

typedef struct {
    u4 offset;
    u4 flags;
    u4 name; // index to string
    u4 implementation;
} pe_manifest_resource_table;

typedef struct {
    u4 nested_class;
    u4 enclosing_class; // index to type_def
} pe_nested_class_table;

typedef struct {
    u2 number;
    u2 flags;
    u4 owner; // index to type_def/method_def
    u4 name; // index to string
} pe_generic_param_table;

typedef struct {
    u4 owner; //
    u4 constraint; // index to type_def/type_ref
} pe_generic_param_constraint_table;

typedef struct {
    pe_metadata_header *header;
    pe_metadata_stream_header *stream_headers;
} pe_metadata;

typedef struct {
    u2 magic;
    u1 major_linker_version;
    u1 minor_linker_version;
    u4 size_of_code;
    u4 size_of_initialized_data;
    u4 size_of_uninitialized_data;
    u4 address_of_entry_point;
    u4 base_of_code;
    u4 base_of_data;
    u4 image_base;
    u4 section_alignment;
    u4 file_alignment;
    u2 major_operating_system_version;
    u2 minor_operating_system_version;
    u2 major_image_version;
    u2 minor_image_version;
    u2 major_subsystem_version;
    u2 minor_subsystem_version;
    u4 win32_version_value;
    u4 size_of_image;
    u4 size_of_headers;
    u4 checksum;
    u2 subsystem;
    u2 dll_characteristics;
    u8 size_of_stack_reserve;
    u8 size_of_stack_commit;
    u8 size_of_heap_reserve;
    u8 size_of_heap_commit;
    u4 loader_flags; // discard
    u4 number_of_rva_and_sizes;
    // data_directory size is number_of_rva_and_sizes, 0x10
    pe_data_directory data_directory[16]; // IMAGE_NUMBEROF_DIRECTORY_ENTRIES
} pe_optional_header;

typedef struct {
    u1 name[8];
    u4 virtual_size; // or physical_address
    u4 virtual_address;
    u4 size_of_raw_data;
    u4 pointer_to_raw_data;
    u4 pointer_to_relocations;
    u4 pointer_to_linenumbers;
    u2 number_of_relocations;
    u2 number_of_linenumbers;
    u4 characteristics;
} pe_section_header;

typedef struct {
    pe_dos_header       *dos_header;
    pe_dos_stub         *stub;
    pe_nt_header        *nt_header;
    pe_optional_header  *optional_header;
    pe_section_header   *section_headers;

    u2 import_table_size;
    pe_import_directory *import_table;

    pe_export_directory *export_table;

    pe_cor20_header     *clr;

    pe_metadata_header *metadata_header;

    pe_metadata_tables_header *metadata_tables_header;

    u2 meta_tables_size;
    pe_metadata_table *meta_tables;

    mem_pool    *pool;
//    size_t      buffer_size;
//    char        *buffer;
//    size_t      cur_off;

    jd_bin      *bin;

} pe_file;


// RAW - PointerToRawData = RVA - ImageBase
// RAW = RVA - ImageBase + PointerToRawData

#endif //GARLIC_PE_H

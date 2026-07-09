#ifndef GARLIC_JD_ANALYZER_H
#define GARLIC_JD_ANALYZER_H

#include "common/types.h"
#include "dex_structure.h"


#define JD_STR_TYPE_NORMAL                  0x00000000
// Class
#define JD_STR_TYPE_CLASS_NAME              0x00000001
#define JD_STR_TYPE_CLASS_DESC              0x00000002
#define JD_STR_TYPE_SOURCE_FILE             0x00000004

// Field
#define JD_STR_TYPE_FIELD_NAME              0x00000008
#define JD_STR_TYPE_FIELD_TYPE              0x00000010

// Method
#define JD_STR_TYPE_METHOD_NAME             0x00000020
#define JD_STR_TYPE_METHOD_RETURN_TYPE      0x00000040
#define JD_STR_TYPE_METHOD_PARAM_TYPE       0x00000080

// Proto
#define JD_STR_TYPE_PROTO_SHORTY            0x00000100

// Annotation / Debug
#define JD_STR_TYPE_ANNOTATION              0x00000200
#define JD_STR_TYPE_SIGNATURE               0x00000400

// Runtime
#define JD_STR_TYPE_REFLECTION              0x00000800
#define JD_STR_TYPE_JNI                     0x00001000
#define JD_STR_TYPE_SO_NAME                 0x00002000
#define JD_STR_TYPE_URL                     0x00004000
#define JD_STR_TYPE_PERMISSION              0x00008000
#define JD_STR_TYPE_INTENT_ACTION           0x00010000
#define JD_STR_TYPE_CRYPTO                  0x00020000
#define JD_STR_TYPE_CONFIG_KEY              0x00040000
#define JD_STR_TYPE_INTERNAL_CLASS_NAME     0x00080000



typedef struct dex_callsite_resolver {
    jd_meta_dex *meta;
    u4 call_site_ids_off;
    u4 call_site_ids_size;
    u4 method_handles_off;
    u4 method_handles_size;
    bool initialized;
} dex_callsite_resolver;

typedef struct jd_graph_node {
    u1 type;
    u8 api_type;
    string klass;
    string method_name;
    string method_desc;
    string ident;
    int id;
} jd_graph_node;



typedef struct jd_dumper_analyzer {
    dex_callsite_resolver *resolver;
    hashmap *method_id_map;
    hashmap *method_edge_map;
    hashmap *string_map;
    hashmap *string_edge_map;

    string out_dir;

    string method_node_path;
    string method_edge_path;

    string string_node_path;
    string string_edge_path;

    FILE *method_node_stream;
    FILE *method_edge_stream;

    FILE *string_node_stream;
    FILE *string_edge_stream;

    int next_string_id;
    int next_method_id;

    mem_pool *pool;
} jd_dumper_analyzer;

typedef struct jd_export_str {
    u8 id;
    string val;
    u8 flags;
} jd_export_str;

void dex_analyzer(jd_dumper_analyzer *analyzer, jd_meta_dex *meta);

void apk_analyzer(string path, string our_dir);

void jd_dex_analyzer_from_file(string path, string save_dir);


#endif //GARLIC_JD_ANALYZER_H

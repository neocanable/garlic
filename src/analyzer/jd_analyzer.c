#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jd_analyzer.h"
#include "parser/dex/metadata.h"
#include "dalvik/dex_ins_helper.h"
#include "dalvik/dex_meta_helper.h"
#include "libs/hashmap/hashmap_tools.h"
#include "common/str_tools.h"
#include "jd_string_analyzer.h"
#include "decompiler/descriptor.h"
#include "jd_api_matcher.h"

static jd_dumper_analyzer *g_dumpper_analyer = NULL;

static void mark_string_to(jd_dumper_analyzer *analyzer, string str, u8 flags)
{
    jd_export_str *obj = hget_s2o(analyzer->string_map, str);
    if (obj)
        obj->flags |= flags;
}

static void mark_field_strings(jd_dumper_analyzer *analyzer, jd_meta_dex *meta)
{
    for (u4 i = 0; i < meta->header->field_ids_size; i++) {
        dex_field_id *field_id = &meta->field_ids[i];

        mark_string_to(analyzer, dex_str_of_type_id(meta, field_id->class_idx), JD_STR_TYPE_CLASS_DESC);
        mark_string_to(analyzer, dex_str_of_idx(meta, field_id->name_idx), JD_STR_TYPE_FIELD_NAME);
        mark_string_to(analyzer, dex_str_of_type_id(meta, field_id->type_idx), JD_STR_TYPE_CLASS_DESC);
    }
}

static void mark_method_strings(jd_dumper_analyzer *analyzer, jd_meta_dex *meta)
{
    for (u4 i = 0; i < meta->header->method_ids_size; i++) {
        dex_method_id *mth_id = &meta->method_ids[i];
        mark_string_to(analyzer, dex_str_of_type_id(meta, mth_id->class_idx), JD_STR_TYPE_CLASS_DESC);
        mark_string_to(analyzer, dex_str_of_idx(meta, mth_id->name_idx), JD_STR_TYPE_METHOD_NAME);

        dex_proto_id *proto = &meta->proto_ids[mth_id->proto_idx];
        mark_string_to(analyzer, dex_str_of_type_id(meta, proto->return_type_idx), JD_STR_TYPE_METHOD_RETURN_TYPE);

        if (proto->parameters_off != 0 && proto->type_list != NULL) {
            for (int j = 0; j < proto->type_list->size; ++j) {
                dex_type_item *item = &proto->type_list->list[j];
                mark_string_to(analyzer, dex_str_of_type_id(meta, item->type_idx), JD_STR_TYPE_METHOD_PARAM_TYPE);
            }
        }

    }
}

static void mark_class_strings(jd_dumper_analyzer *analyzer, jd_meta_dex *meta)
{
    for (u4 i = 0; i < meta->header->class_defs_size; i++) {
        dex_class_def *cls = &meta->class_defs[i];

        mark_string_to(analyzer, dex_str_of_type_id(meta, cls->class_idx), JD_STR_TYPE_CLASS_DESC);
        if (cls->source_file_idx != NO_INDEX) {
            mark_string_to(analyzer, dex_str_of_idx(meta, cls->source_file_idx), JD_STR_TYPE_SOURCE_FILE);
        }

        if (cls->superclass_idx != NO_INDEX) {
            mark_string_to(analyzer, dex_str_of_type_id(meta, cls->superclass_idx), JD_STR_TYPE_CLASS_DESC);
        }

        if (cls->interfaces != NULL) {
            for (int j = 0; j < cls->interfaces->size; ++j) {
                dex_type_item *item = &cls->interfaces->list[j];
                mark_string_to(analyzer, dex_str_of_type_id(meta, item->type_idx), JD_STR_TYPE_CLASS_DESC);
            }
        }
    }
}

static void csv_write_quoted(FILE *stream, const char *text)
{
    fputc('"', stream);

    const unsigned char *p = (const unsigned char *)text;

    while (*p) {

        if (*p == '\n' || *p == '\r' || *p == '\t') {
            fputc(' ', stream);
            p++;
            continue;
        }

        if (*p == '"') {
            fputc('"', stream);
            fputc('"', stream);
            p++;
            continue;
        }

        if (*p > 0 && *p < 0x80) {
            if (*p >= 0x20)
                fputc(*p, stream);
            else
                fputc(' ', stream);
            p++;
            continue;
        }

        if (p[0] == 0xC0 && p[1] == 0x80) {
            fputc(' ', stream);
            p += 2;
            continue;
        }

        if ((*p >> 5) == 0x6 && (p[1] & 0xC0) == 0x80) {
            fputc(p[0], stream);
            fputc(p[1], stream);
            p += 2;
            continue;
        }

        if (p[0] == 0xED && (p[1] & 0xF0) == 0xA0 && (p[2] & 0xC0) == 0x80 &&
            p[3] == 0xED && (p[4] & 0xF0) == 0xB0 && (p[5] & 0xC0) == 0x80) {

            unsigned int high = ((p[1] & 0x0F) << 6) | (p[2] & 0x3F);
            unsigned int low  = ((p[4] & 0x0F) << 6) | (p[5] & 0x3F);
            unsigned int code = 0x10000 + ((high & 0x3FF) << 10) | (low & 0x3FF);

            fputc(0xF0 | ((code >> 18) & 0x07), stream);
            fputc(0x80 | ((code >> 12) & 0x3F), stream);
            fputc(0x80 | ((code >> 6) & 0x3F), stream);
            fputc(0x80 | (code & 0x3F), stream);

            p += 6;
            continue;
        }

        if ((*p >> 4) == 0xE && p[0] != 0xED &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80) {

            fputc(p[0], stream);
            fputc(p[1], stream);
            fputc(p[2], stream);
            p += 3;
            continue;
        }

        if ((*p >> 3) == 0x1E &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80 &&
            (p[3] & 0xC0) == 0x80) {

            fputc(p[0], stream);
            fputc(p[1], stream);
            fputc(p[2], stream);
            fputc(p[3], stream);
            p += 4;
            continue;
        }

        fputc('?', stream);
        p++;
    }

    fputc('"', stream);
}


static inline bool jd_export_str_has_flag(jd_export_str *str, u8 flag)
{
    return ((str->flags & flag) != 0);
}

static void write_all_graph_node(jd_dumper_analyzer *analyzer)
{
    struct hashmap_iter iter;
    hashmap_iter_init(analyzer->method_id_map, &iter);
    struct hashmap_entry *_entry;
    while ((_entry = hashmap_iter_next(&iter))) {
        u8_to_object *entry = _entry;
        jd_graph_node *node = entry->value;
        fprintf(analyzer->method_node_stream, "%d,", node->id);
        csv_write_quoted(analyzer->method_node_stream, node->ident);
        fprintf(analyzer->method_node_stream, ",%d,", node->type);
        csv_write_quoted(analyzer->method_node_stream, node->klass);
        fprintf(analyzer->method_node_stream, ",");
        csv_write_quoted(analyzer->method_node_stream, node->method_name);
        fprintf(analyzer->method_node_stream, ",");
        csv_write_quoted(analyzer->method_node_stream, node->method_desc);
        fprintf(analyzer->method_node_stream, ",%d\n", node->api_type);
    }

}

static void write_all_string(jd_dumper_analyzer *analyzer)
{
    struct hashmap_iter iter;
    hashmap_iter_init(analyzer->string_map, &iter);
    struct hashmap_entry *_entry;
    fprintf(analyzer->string_node_stream, "id,pc,str,is_class_desc,is_field_name,is_method_name,is_return_type,is_method_param_type,is_internal_class_desc,is_url,is_enc_dec,is_uuid,is_pem_key,is_so_name,is_ipv4\n");
    while ((_entry = hashmap_iter_next(&iter))) {
        u8_to_object *entry = _entry;
        jd_export_str *str = entry->value;
        fprintf(analyzer->string_node_stream, "%d,", str->id);
        fprintf(analyzer->string_node_stream, "%d,", 0);
        csv_write_quoted(analyzer->string_node_stream, str->val);
        fprintf(analyzer->string_node_stream, ",%d", jd_export_str_has_flag(str, JD_STR_TYPE_CLASS_DESC));
        fprintf(analyzer->string_node_stream, ",%d", jd_export_str_has_flag(str, JD_STR_TYPE_FIELD_NAME));
        fprintf(analyzer->string_node_stream, ",%d", jd_export_str_has_flag(str, JD_STR_TYPE_METHOD_NAME));
        fprintf(analyzer->string_node_stream, ",%d", jd_export_str_has_flag(str, JD_STR_TYPE_METHOD_RETURN_TYPE));
        fprintf(analyzer->string_node_stream, ",%d", jd_export_str_has_flag(str, JD_STR_TYPE_METHOD_PARAM_TYPE));
        int is_internal = str_contains(str->val, "$");
        fprintf(analyzer->string_node_stream, ",%d", is_internal);
        int is_url = 0;
        if (str->flags == 0) {
            is_url = str_is_url(str->val);
        }
        fprintf(analyzer->string_node_stream, ",%d", is_url);
        int is_enc_dec = 0;
        if (str->flags == 0) {
            is_enc_dec = str_is_enc_dec(str->val);
        }
        fprintf(analyzer->string_node_stream, ",%d", is_enc_dec);

        int is_uuid = 0;
        if (str->flags == 0) {
            is_uuid = str_is_uuid(str->val);
        }
        fprintf(analyzer->string_node_stream, ",%d", is_uuid);

        int is_pem_key = 0;
        if (str->flags == 0) {
            is_pem_key = str_is_pem_key(str) || str_is_der_base64_key(str);
        }
        fprintf(analyzer->string_node_stream, ",%d", is_pem_key);

        int is_so_name = 0;
        if (str->flags == 0) {
            is_so_name = str_is_so_name(str->val);
        }
        fprintf(analyzer->string_node_stream, ",%d", is_so_name);

        int is_ipv4 = 0;
        if (str->flags == 0) {
            is_ipv4 = str_is_ipv4(str->val);
        }
        fprintf(analyzer->string_node_stream, ",%d", is_ipv4);

        fputc('\n', analyzer->string_node_stream);
    }
}


static bool is_noise_callee_class(const char *class_name)
{
    return false;
}

static jd_graph_api_type jd_node_api_type_matcher(jd_graph_node *node)
{
    if (node->klass == NULL || node->method_name == NULL)
        return JD_GRAPH_NODE_API_UNKNOWN;
    for (int i = 0; i < sizeof(api_table) / sizeof(api_table[0]); i++) {
        jd_api_matcher *matcher = &api_table[i];
        if (STR_EQL(matcher->clazz, node->klass) && STR_EQL(matcher->method, node->method_name))
            return matcher->behavior;
    }
    return JD_GRAPH_NODE_API_UNKNOWN;
}

static int register_method(jd_dumper_analyzer *analyzer, string class_name, string method_name, string method_desc, const char *ident, encoded_method *em)
{
    jd_graph_node *n = hget_s2o(analyzer->method_id_map, (char *)ident);
    if (n) {
        if (em && ((em->access_flags & ACC_DEX_NATIVE) != 0))
            n->type = 1;
        return n->id;
    }

    jd_graph_node *node = make_obj_in(jd_graph_node, analyzer->pool);
    node->klass = class_name;
    node->type = 0;
    node->method_name = method_name;
    node->method_desc = method_desc;
    node->ident = ident;
    node->id = analyzer->next_method_id ++;
    if (em != NULL) {
        if ((em->access_flags & ACC_DEX_NATIVE) != 0)
            node->type = 1;
    }
    jd_graph_api_type t = jd_node_api_type_matcher(node);
    if (t != JD_GRAPH_NODE_API_UNKNOWN) {
        node->api_type = t;
    }

    hset_s2o(analyzer->method_id_map, ident, node);
    return node->id;
}

static void register_method_edge(jd_dumper_analyzer *analyzer, int src_id, int dst_id)
{
    string key = str_create_in(analyzer->pool, "%d>%d", src_id, dst_id);
    if (hget_s2i(analyzer->method_edge_map, key) >= 0)
        return;

    hset_s2i(analyzer->method_edge_map, key, 1);
    fprintf(analyzer->method_edge_stream, "%d,%d\n", src_id, dst_id);
}

static int register_string(jd_dumper_analyzer *analyzer, const char *str, encoded_method *em)
{
    jd_export_str *info = hget_s2o(analyzer->string_map, (char *)str);
    if (info != NULL)
        return (int)info->id;

    int id = analyzer->next_string_id++;
    info = make_obj_in(jd_export_str, analyzer->pool);
    info->id = id;
    info->val = str_create_in(analyzer->pool, "%s", str);

    hset_s2o(analyzer->string_map, info->val, info);
    return id;
}

static void register_string_edge(jd_dumper_analyzer *analyzer, int src_id, int dst_id)
{
    string key = str_create_in(analyzer->pool, "%d>%d", src_id, dst_id);
    if (hget_s2i(analyzer->string_edge_map, key) >= 0)
        return;

    hset_s2i(analyzer->string_edge_map, key, 1);
    fprintf(analyzer->string_edge_stream, "%d,%d\n", src_id, dst_id);
}

static string dex_proto_to_descriptor(jd_meta_dex *meta, dex_proto_id *proto)
{
    str_list *list = str_list_init();
    str_concat(list, "(");

    if (proto->parameters_off != 0 && proto->type_list != NULL) {
        for (int i = 0; i < proto->type_list->size; ++i) {
            dex_type_item *item = &proto->type_list->list[i];
            str_concat(list, dex_str_of_type_id(meta, item->type_idx));
        }
    }

    str_concat(list, ")");
    str_concat(list, dex_str_of_type_id(meta, proto->return_type_idx));
    return str_join(list);
}

static bool dex_is_invoke_opcode(u1 opcode)
{
    return opcode == DEX_INS_INVOKE_VIRTUAL ||
           opcode == DEX_INS_INVOKE_SUPER ||
           opcode == DEX_INS_INVOKE_DIRECT ||
           opcode == DEX_INS_INVOKE_STATIC ||
           opcode == DEX_INS_INVOKE_INTERFACE ||
           opcode == DEX_INS_INVOKE_VIRTUAL_RANGE ||
           opcode == DEX_INS_INVOKE_SUPER_RANGE ||
           opcode == DEX_INS_INVOKE_DIRECT_RANGE ||
           opcode == DEX_INS_INVOKE_STATIC_RANGE ||
           opcode == DEX_INS_INVOKE_INTERFACE_RANGE ||
           opcode == DEX_INS_INVOKE_CUSTOM ||
           opcode == DEX_INS_INVOKE_CUSTOM_RANGE ||
           opcode == DEX_INS_INVOKE_POLYMORPHIC ||
           opcode == DEX_INS_INVOKE_POLYMORPHIC_RANGE;
}

static u4 dex_ins_len_for_call_graph(dex_code_item *code, u4 i)
{
    u2 item = code->insns[i];
    u1 opcode = item & 0xFF;

    if (opcode == 0x00) {
        if (item == 0x0100) {
            u2 size = code->insns[i + 1];
            return size * 2 + 4;
        }
        if (item == 0x0200) {
            u2 size = code->insns[i + 1];
            return size * 4 + 2;
        }
        if (item == 0x0300) {
            u2 element_size = code->insns[i + 1];
            u2 size = code->insns[i + 2];
            return (size * element_size + 1) / 2 + 4;
        }
        return 1;
    }

    u4 len = dex_opcode_len(opcode);
    if (len == 0)
        len = 1;
    return len;
}

static inline u2 read_u2(const u1 *p)
{
    u2 v;
    memcpy(&v, p, sizeof(u2));
    return v;
}

static inline u4 read_u4(const u1 *p)
{
    u4 v;
    memcpy(&v, p, sizeof(u4));
    return v;
}

static u4 read_uleb128_raw(const u1 *buf, size_t *pos, size_t size)
{
    u4 result = 0;
    int shift = 0;
    while (*pos < size) {
        u1 b = buf[(*pos)++];
        result |= (u4)(b & 0x7f) << shift;
        if ((b & 0x80) == 0)
            break;
        shift += 7;
    }
    return result;
}

static u4 read_value_unsigned_le(const u1 *p, int len)
{
    u4 v = 0;
    for (int i = 0; i < len; ++i) {
        v |= ((u4)p[i]) << (i * 8);
    }
    return v;
}

static bool dex_find_map_item(jd_meta_dex *meta, u2 type, u4 *out_size, u4 *out_off)
{
    size_t map_off = meta->header->map_off;
    if (map_off == 0 || map_off + 4 > meta->bin->buffer_size)
        return false;

    const u1 *buf = (const u1 *)meta->bin->buffer;
    u4 item_size = read_u4(buf + map_off);
    size_t pos = map_off + 4;
    for (u4 i = 0; i < item_size; ++i) {
        if (pos + 12 > meta->bin->buffer_size)
            break;
        u2 item_type = read_u2(buf + pos);
        u4 size = read_u4(buf + pos + 4);
        u4 off = read_u4(buf + pos + 8);
        if (item_type == type) {
            *out_size = size;
            *out_off = off;
            return true;
        }
        pos += 12;
    }
    return false;
}

static void dex_callsite_resolver_init(jd_dumper_analyzer *analyzer, jd_meta_dex *meta)
{
    dex_callsite_resolver *resolver = make_obj_in(dex_callsite_resolver, analyzer->pool);
    memset(resolver, 0, sizeof(dex_callsite_resolver));
    resolver->meta = meta;
    u4 cs_size = 0, cs_off = 0;
    u4 mh_size = 0, mh_off = 0;
    bool ok_cs = dex_find_map_item(meta, kDexTypeCallSiteIdItem, &cs_size, &cs_off);
    bool ok_mh = dex_find_map_item(meta, kDexTypeMethodHandleItem, &mh_size, &mh_off);
    if (ok_cs && ok_mh) {
        resolver->call_site_ids_size = cs_size;
        resolver->call_site_ids_off = cs_off;
        resolver->method_handles_size = mh_size;
        resolver->method_handles_off = mh_off;
        resolver->initialized = true;
    }
}

static int dex_resolve_method_from_handle(dex_callsite_resolver *resolver, u4 handle_idx)
{
    jd_meta_dex *meta = resolver->meta;
    if (handle_idx >= resolver->method_handles_size)
        return -1;

    size_t off = resolver->method_handles_off + handle_idx * 8;
    if (off + 8 > meta->bin->buffer_size)
        return -1;

    const u1 *buf = (const u1 *)meta->bin->buffer;
    u2 handle_type = read_u2(buf + off);
    u2 field_or_method_idx = read_u2(buf + off + 4);

    if (handle_type < INVOKE_STATIC || handle_type > INVOKE_INTERFACE)
        return -1;
    if (field_or_method_idx >= meta->header->method_ids_size)
        return -1;

    return (int)field_or_method_idx;
}

static int dex_resolve_invoke_custom_method_id(dex_callsite_resolver *resolver, u4 call_site_idx)
{
    if (!resolver->initialized)
        return -1;

    jd_meta_dex *meta = resolver->meta;
    if (call_site_idx >= resolver->call_site_ids_size)
        return -1;

    size_t cs_off = resolver->call_site_ids_off + call_site_idx * 4;
    if (cs_off + 4 > meta->bin->buffer_size)
        return -1;

    const u1 *buf = (const u1 *)meta->bin->buffer;
    u4 call_site_off = read_u4(buf + cs_off);
    if (call_site_off >= meta->bin->buffer_size)
        return -1;

    size_t pos = call_site_off;
    u4 value_count = read_uleb128_raw(buf, &pos, meta->bin->buffer_size);
    int fallback_method_id = -1;

    for (u4 i = 0; i < value_count && pos < meta->bin->buffer_size; ++i) {
        u1 strict = buf[pos++];
        int value_type = strict & 0x1f;
        int value_arg = strict >> 5;
        int value_len = value_arg + 1;

        if (value_type == kDexAnnotationBoolean || value_type == kDexAnnotationNull)
            value_len = 0;

        if (pos + value_len > meta->bin->buffer_size)
            break;

        if (value_type == kDexAnnotationMethodHandle && value_len > 0) {
            u4 handle_idx = read_value_unsigned_le(buf + pos, value_len);
            int method_id = dex_resolve_method_from_handle(resolver, handle_idx);
            if (method_id >= 0) {
                if (hget_u4obj(meta->lambda_method_map, (u4)method_id) != NULL)
                    return method_id;
                if (fallback_method_id < 0)
                    fallback_method_id = method_id;
            }
        }

        pos += value_len;
    }

    return fallback_method_id;
}

static void dex_call_graph_scan_method(jd_meta_dex *meta, jd_dumper_analyzer *analyzer,
                            encoded_method *em,
                            dex_method_id *caller_mid,
                            dex_code_item *code)
{
    dex_callsite_resolver *resolver = analyzer->resolver;
    string caller_class = dex_str_of_type_id(meta, caller_mid->class_idx);
    string caller_name = dex_str_of_idx(meta, caller_mid->name_idx);
    dex_proto_id *caller_proto = &meta->proto_ids[caller_mid->proto_idx];
    string caller_desc = dex_proto_to_descriptor(meta, caller_proto);

    if (caller_class == NULL || caller_name == NULL || caller_desc == NULL)
        return;

    char *caller_ident = str_create_in(analyzer->pool, "%s->%s%s", caller_class, caller_name, caller_desc);

    if (em->code == NULL) {
        register_method(analyzer, caller_class, caller_name, caller_desc,caller_ident,  em);
        return;
    }

    for (u4 i = 0; i < code->insns_size; ) {
        u2 item = code->insns[i];
        u1 opcode = item & 0xFF;

        if (dex_is_invoke_opcode(opcode)) {
            u2 method_index = code->insns[i + 1];
            if (opcode == DEX_INS_INVOKE_CUSTOM ||
                opcode == DEX_INS_INVOKE_CUSTOM_RANGE) {
                int resolved_mid = dex_resolve_invoke_custom_method_id(resolver, method_index);
                if (resolved_mid >= 0) {
                    dex_method_id *callee_mid = &meta->method_ids[resolved_mid];
                    dex_proto_id *callee_proto = &meta->proto_ids[callee_mid->proto_idx];
                    string callee_class = dex_str_of_type_id(meta, callee_mid->class_idx);
                    string callee_name = dex_str_of_idx(meta, callee_mid->name_idx);
                    string callee_desc = dex_proto_to_descriptor(meta, callee_proto);
                    if (callee_class != NULL && callee_name != NULL && callee_desc != NULL) {
                        /**
                         * do not filter system lib
                         * there are lots of reflect call
                         */
                        if (is_noise_callee_class(callee_class)) {
                            i += dex_ins_len_for_call_graph(code, i);
                            continue;
                        }
                        char *callee_ident = str_create_in(analyzer->pool, "%s->%s%s", callee_class, callee_name, callee_desc);
                        int src_id = register_method(analyzer, caller_class, caller_name, caller_desc, caller_ident, em);
                        int dst_id = register_method(analyzer, callee_class, callee_name, callee_desc, callee_ident, NULL);
                        register_method_edge(analyzer, src_id, dst_id);
                    }
                } else {
                    char dynamic_name[64];
                    snprintf(dynamic_name, sizeof(dynamic_name), "call_site@%u", (unsigned int)method_index);
                    char *callee_ident = str_create_in(analyzer->pool, "%s->%s%s", "<dynamic_lambda>", dynamic_name, "()V");
                    int src_id = register_method(analyzer, caller_class, caller_name, caller_desc, caller_ident, em);

                    int dst_id = register_method(analyzer, NULL, dynamic_name, NULL, callee_ident, NULL);
                    register_method_edge(analyzer, src_id, dst_id);
                }
            }
            else if (method_index < meta->header->method_ids_size) {
                dex_method_id *callee_mid = &meta->method_ids[method_index];
                dex_proto_id *callee_proto = &meta->proto_ids[callee_mid->proto_idx];

                string callee_class = dex_str_of_type_id(meta, callee_mid->class_idx);
                string callee_name = dex_str_of_idx(meta, callee_mid->name_idx);
                string callee_desc = dex_proto_to_descriptor(meta, callee_proto);
                if (callee_class != NULL && callee_name != NULL && callee_desc != NULL) {
                    char *callee_ident = str_create_in(analyzer->pool, "%s->%s%s", callee_class, callee_name, callee_desc);
                    int src_id = register_method(analyzer, caller_class, caller_name, caller_desc, caller_ident, em);
                    int dst_id = register_method(analyzer, callee_class, callee_name, callee_desc, callee_ident, NULL);
                    register_method_edge(analyzer, src_id, dst_id);
                }
            }
        }

        string str = NULL;
        if (opcode == DEX_INS_CONST_STRING) {
            u4 idx = code->insns[i + 1];
            str = dex_str_of_idx(meta, idx);
        }
        else if (opcode == DEX_INS_CONST_STRING_JUMBO) {
            u4 idx = (code->insns[i + 2] << 16 | code->insns[i + 1]);
            str = dex_str_of_idx(meta, idx);
        }

        if (str != NULL) {
            int str_id = register_string(analyzer, str, em);
            int mth_id = register_method(analyzer, caller_class, caller_name, caller_desc, caller_ident, em);
            register_string_edge(analyzer, str_id, mth_id);
            // printf("string: %s, class: %s, method: %s\n", str, caller_class, caller_name);
        }

        i += dex_ins_len_for_call_graph(code, i);
    }
}

void dex_call_graph(jd_dumper_analyzer *analyzer, jd_meta_dex *meta)
{
    dex_callsite_resolver_init(analyzer, meta);

    for (u4 i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        dex_class_data_item *data = cf->class_data;
        if (data == NULL)
            continue;

        for (u4 j = 0; j < data->direct_methods_size; ++j) {
            encoded_method *em = &data->direct_methods[j];
            dex_method_id *caller_mid = &meta->method_ids[em->method_id];
            dex_call_graph_scan_method(meta, analyzer, em, caller_mid, em->code);
        }

        for (u4 j = 0; j < data->virtual_methods_size; ++j) {
            encoded_method *em = &data->virtual_methods[j];
            dex_method_id *caller_mid = &meta->method_ids[em->method_id];
            dex_call_graph_scan_method(meta, analyzer, em, caller_mid, em->code);
        }
    }
}


void initialize_analyzer(string out_dir)
{
    mem_pool *pool = mem_create_pool();
    g_dumpper_analyer = make_obj_in(jd_dumper_analyzer, pool);
    g_dumpper_analyer->pool = pool;

    g_dumpper_analyer->out_dir = out_dir;
    g_dumpper_analyer->method_node_path = str_create_in(pool, "%s/call_graph_node.csv", out_dir);
    g_dumpper_analyer->method_edge_path = str_create_in(pool, "%s/call_graph_edge.csv", out_dir);

    g_dumpper_analyer->string_node_path = str_create_in(pool, "%s/string_node.csv", out_dir);
    g_dumpper_analyer->string_edge_path = str_create_in(pool, "%s/string_edge.csv", out_dir);

    g_dumpper_analyer->method_node_stream = fopen(g_dumpper_analyer->method_node_path, "w");
    g_dumpper_analyer->method_edge_stream = fopen(g_dumpper_analyer->method_edge_path, "w");
    g_dumpper_analyer->string_node_stream = fopen(g_dumpper_analyer->string_node_path, "w");
    g_dumpper_analyer->string_edge_stream = fopen(g_dumpper_analyer->string_edge_path, "w");

    g_dumpper_analyer->method_id_map = hashmap_init_in(pool, s2o_cmp, 0);
    g_dumpper_analyer->method_edge_map = hashmap_init_in(pool, s2i_cmp, 0);
    g_dumpper_analyer->next_method_id = 0;

    g_dumpper_analyer->string_map = hashmap_init_in(pool, s2o_cmp, 0);
    g_dumpper_analyer->string_edge_map = hashmap_init_in(pool, s2i_cmp, 0);
    g_dumpper_analyer->next_string_id = 0;

    fprintf(g_dumpper_analyer->method_node_stream, "id,method,type,class_name,method_name,method_desc,api_type\n");
    fprintf(g_dumpper_analyer->method_edge_stream, "src_id,dst_id\n");
    fprintf(g_dumpper_analyer->string_edge_stream, "src_id,dst_id\n");
}

void jd_dex_analyzer_from_file(string path, string save_dir)
{
    initialize_analyzer(save_dir);

    jd_meta_dex *meta = parse_dex_file(path);
    dex_call_graph(g_dumpper_analyer, meta);
    mem_pool_free(meta->pool);
}

void dex_analyzer(jd_dumper_analyzer *analyzer, jd_meta_dex *meta)
{
    dex_call_graph(analyzer, meta);

    mark_method_strings(analyzer, meta);
    mark_class_strings(analyzer, meta);
    mark_field_strings(analyzer, meta);
}

/**
 * TODO: nested apk need support
 **/
void apk_analyzer(string path, string out_dir)
{
    initialize_analyzer(out_dir);

    struct zip_t *zip = zip_open(path, 0, 'r');
    if (zip == NULL)
        return;

    int total = zip_entries_total(zip);
    for (int i = 0; i < total; ++i) {
        zip_entry_openbyindex(zip, i);
        string entry_name = (string)zip_entry_name(zip);

        if (!str_end_with(entry_name, ".dex") ||
            strchr(entry_name, '/') != NULL) {
            zip_entry_close(zip);
            continue;
        }

        size_t buf_size = zip_entry_size(zip);
        char *buf = malloc(buf_size);
        if (buf == NULL) {
            zip_entry_close(zip);
            continue;
        }

        zip_entry_noallocread(zip, (void *)buf, buf_size);
        zip_entry_close(zip);

        jd_meta_dex *meta = parse_dex_from_buffer(buf, buf_size);
        dex_analyzer(g_dumpper_analyer, meta);

        mem_pool_free(meta->pool);
        free(buf);
    }
    write_all_graph_node(g_dumpper_analyer);
    write_all_string(g_dumpper_analyer);

    zip_close(zip);
    mem_pool_free(g_dumpper_analyer->pool);
}

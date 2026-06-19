#include "apk_manifest.h"
#include "file_tools.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int indent = 0;

static inline u2 read_uint16_le(const u1* ptr) {
    return (u2)ptr[0] | ((u2)ptr[1] << 8);
}

static inline u4 read_uint32_le(const u1* ptr) {
    return (u4)ptr[0] | ((u4)ptr[1] << 8) | ((u4)ptr[2] << 16) | ((u4)ptr[3] << 24);
}

void utf16_to_utf8_safe(const u1* src_bytes, size_t src_max_bytes, char* dest, size_t dest_max_len) {
    if (!src_bytes || !dest || dest_max_len == 0) return;

    if (src_max_bytes < 2) {
        dest[0] = '\0';
        return;
    }

    size_t src_idx = 0;
    size_t dest_idx = 0;

    while (src_idx + 1 < src_max_bytes && dest_idx < dest_max_len - 1) {
        u2 u16 = read_uint16_le(src_bytes + src_idx);
        src_idx += 2;

        if (u16 == 0) break;

        if (u16 < 0x80) {
            dest[dest_idx++] = (char)u16;
        } else if (u16 < 0x800) {
            if (dest_idx + 2 >= dest_max_len) break;
            dest[dest_idx++] = (char)(0xC0 | (u16 >> 6));
            dest[dest_idx++] = (char)(0x80 | (u16 & 0x3F));
        } else {
            if (dest_idx + 3 >= dest_max_len) break;
            dest[dest_idx++] = (char)(0xE0 | (u16 >> 12));
            dest[dest_idx++] = (char)(0x80 | ((u16 >> 6) & 0x3F));
            dest[dest_idx++] = (char)(0x80 | (u16 & 0x3F));
        }
    }
    dest[dest_idx] = '\0';
}

const char* get_string(const axml_string_table_t* str_table, u4 idx) {
    if (!str_table || !str_table->table || idx == 0xFFFFFFFF || idx >= str_table->count) {
        return "";
    }
    return str_table->table[idx] ? str_table->table[idx] : "";
}

void parse_manifest_binary(FILE *stream, const u1* buffer, size_t buffer_size) {

    if (stream == NULL)
        stream = stdout;

    if (buffer_size < 8) {
        DEBUG_ERROR_PRINT("file too small.\n");
        return;
    }

    u4 file_magic = read_uint32_le(buffer + 0);
    u4 file_size  = read_uint32_le(buffer + 4);

    if (file_magic != 0x00080003) {
        DEBUG_ERROR_PRINT("magic number mismatch\n");
        return;
    }

    axml_string_table_t str_table = { .table = NULL, .count = 0 };
    size_t offset = 8;
    indent = 0;

    // 字符串池
    if (offset + 8 > buffer_size) {
        DEBUG_ERROR_PRINT("not enough data\n");
        return;
    }

    u2 chunk_type  = read_uint16_le(buffer + offset + 0);
    u2 header_size = read_uint16_le(buffer + offset + 2);
    u4 chunk_size  = read_uint32_le(buffer + offset + 4);

    if (chunk_type != 0x0001) {
        DEBUG_ERROR_PRINT("first is not string pool\n");
        return;
    }

    if (chunk_size < 28 || offset + chunk_size > buffer_size) {
        DEBUG_ERROR_PRINT("invalid string pool chunk\n");
        return;
    }

    {
        u4 string_count = read_uint32_le(buffer + offset + 8);
        u4 flags        = read_uint32_le(buffer + offset + 16);
        u4 string_start = read_uint32_le(buffer + offset + 20);

        if (string_count == 0 || string_count > 100000) {
            DEBUG_ERROR_PRINT("string count invalid\n");
            return;
        }

        str_table.count = string_count;
        str_table.table = (char**)calloc(str_table.count, sizeof(char*));

        if (!str_table.table) {
            fprintf(stderr, "Error: Out of memory\n");
            return;
        }

        const u1* pool_start = buffer + offset;
        const u1* pool_end = buffer + offset + chunk_size;
        const u4* offset_array = (const u4*)(pool_start + 28);
        const u1* string_data_start = pool_start + string_start;

        int is_utf8 = (flags & 0x00000100) != 0;

        for (u4 i = 0; i < string_count; i++) {
            if ((const u1*)&offset_array[i] + 4 > pool_end) {
                str_table.table[i] = strdup("");
                continue;
            }

            u4 str_offset = read_uint32_le((const u1*)&offset_array[i]);
            const u1* str_ptr = string_data_start + str_offset;

            if (str_ptr >= pool_end || str_ptr < string_data_start) {
                str_table.table[i] = strdup("");
                continue;
            }

            size_t remaining = pool_end - str_ptr;

            if (is_utf8) {
                char buf[2048] = {0};
                size_t len = 0;

                if (remaining > 0) {
                    if (str_ptr[0] & 0x80) {
                        if (remaining >= 2) {
                            len = ((str_ptr[0] & 0x7F) << 8) | str_ptr[1];
                            str_ptr += 2;
                            remaining -= 2;
                        } else {
                            str_table.table[i] = strdup("");
                            continue;
                        }
                    } else {
                        len = str_ptr[0];
                        str_ptr += 1;
                        remaining -= 1;
                    }
                }

                if (len > 0 && len <= remaining) {
                    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
                    memcpy(buf, str_ptr, len);
                    buf[len] = '\0';
                    str_table.table[i] = strdup(buf);
                } else {
                    str_table.table[i] = strdup("");
                }
            } else {
                // UTF-16LE
                if (remaining >= 2) {
                    u2 u16_len = read_uint16_le(str_ptr);
                    str_ptr += 2;
                    remaining -= 2;

                    size_t byte_len = u16_len * 2;
                    if (byte_len > remaining) byte_len = remaining;

                    char buf[2048] = {0};
                    utf16_to_utf8_safe(str_ptr, byte_len, buf, sizeof(buf));
                    str_table.table[i] = strdup(buf);
                } else {
                    str_table.table[i] = strdup("");
                }
            }
        }
    }

    offset += chunk_size;

    while (offset + 8 <= buffer_size) {
        chunk_type  = read_uint16_le(buffer + offset + 0);
        header_size = read_uint16_le(buffer + offset + 2);
        chunk_size  = read_uint32_le(buffer + offset + 4);

        if (chunk_size < 8 || offset + chunk_size > buffer_size) {
            break;
        }

        int processed = 0;

        switch (chunk_type) {
            case 0x0001: // string pool
                processed = 1;
                break;

            case 0x0180: // resource map
            case 0x0100: // start namespace
            case 0x0101: // end namespace
                processed = 1;
                break;

            case 0x0102: { // start element
                if (offset + header_size + 20 > buffer_size) break;

                const u1* payload = buffer + offset + header_size;

                u4 ns_idx = read_uint32_le(payload + 0);
                u4 name_idx = read_uint32_le(payload + 4);
                u2 flags = read_uint16_le(payload + 8);
                u2 attr_size = read_uint16_le(payload + 10);
                u2 attr_count = read_uint16_le(payload + 12);

                const char* tag_name = get_string(&str_table, name_idx);
                if (!tag_name[0]) tag_name = "unknown";

                for (int i = 0; i < indent; i++) printf("  ");
                fprintf(stream, "<%s", tag_name);

                if (attr_size == 0) attr_size = 20;
                if (attr_count > 1000) attr_count = 0;
                if (attr_size < 14 || attr_size > 100) attr_count = 0;

                size_t attr_chunk_start = header_size + 20;
                size_t attr_abs_start = offset + attr_chunk_start;
                size_t attr_abs_end = attr_abs_start + (size_t)attr_count * attr_size;

                if (attr_count > 0 && attr_abs_end <= buffer_size) {
                    const u1* attr_base = buffer + attr_abs_start;

                    for (u2 i = 0; i < attr_count; i++) {
                        const u1* attr_ptr = attr_base + (i * attr_size);
                        u4 attr_uri_idx = read_uint32_le(attr_ptr + 0);
                        u4 attr_name_idx = read_uint32_le(attr_ptr + 4);
                        u4 attr_raw_idx = read_uint32_le(attr_ptr + 8);
                        u4 attr_val_size = read_uint32_le(attr_ptr + 12);
                        u1  attr_res0 = attr_ptr[16];
                        u1  attr_val_type = attr_ptr[17];
                        u4 attr_val_data = read_uint32_le(attr_ptr + 18);

                        const char* attr_name = get_string(&str_table, attr_name_idx);
                        if (!attr_name[0]) attr_name = "attr";

                        char full_attr_name[256];
                        if (attr_uri_idx != 0xFFFFFFFF && attr_uri_idx < str_table.count) {
                            const char* uri = get_string(&str_table, attr_uri_idx);
                            if (uri[0] && strstr(uri, "android")) {
                                snprintf(full_attr_name, sizeof(full_attr_name), "android:%s", attr_name);
                            } else {
                                snprintf(full_attr_name, sizeof(full_attr_name), "%s", attr_name);
                            }
                        } else {
                            snprintf(full_attr_name, sizeof(full_attr_name), "%s", attr_name);
                        }

                        fprintf(stream, " %s=", full_attr_name);

                        int handled = 0;

                        // string index
                        if (attr_raw_idx != 0xFFFFFFFF && attr_raw_idx < str_table.count) {
                            const char* val = get_string(&str_table, attr_raw_idx);
                            if (val && val[0]) {
                                fprintf(stream, "\"%s\"", val);
                                handled = 1;
                            }
                        }

                        if (!handled) {
                            switch (attr_val_type) {
                                case 0x00:
                                    // null
                                    fprintf(stream, "\"\"");
                                    break;
                                case 0x01:
                                    // @id
                                    fprintf(stream, "\"@0x%08X\"", attr_val_data);
                                    break;
                                case 0x02:
                                    // resource id
                                    fprintf(stream, "\"@0x%08X\"", attr_val_data);
                                    break;
                                case 0x03: {
                                    // string
                                    const char* val = get_string(&str_table, attr_val_data);
                                    fprintf(stream, "\"%s\"", val);
                                }
                                    break;
                                case 0x04:
                                    // int
                                    fprintf(stream, "\"%d\"", (int32_t)attr_val_data);
                                    break;
                                case 0x05: {
                                    u4 val = attr_val_data;
                                    float fval = (val & 0xFFFFFF) / 256.0f;
                                    fprintf(stream, "\"%.0fdip\"", fval);
                                }
                                    break;
                                case 0x06: {
                                    // FRACTION
                                    u4 val = attr_val_data;
                                    float fval = (int32_t)(val & 0xFFFFFF) / 256.0f;
                                    fprintf(stream, "\"%.1f%%\"", fval);
                                }
                                    break;
                                case 0x08:
                                    // resouce id
                                    fprintf(stream, "\"@0x%08X\"", attr_val_data);
                                    break;
                                case 0x09: {
                                    // asset
                                    const char* val = get_string(&str_table, attr_raw_idx);
                                    fprintf(stream, "\"%s\"", val);
                                    break;
                                }
                                case 0x0A: {
                                    // boolean
                                    const char* val = get_string(&str_table, attr_raw_idx);
                                    if (val && val[0]) {
                                        fprintf(stream, "\"%s\"", val);
                                    } else {
                                        fprintf(stream, "\"%s\"", attr_val_data ? "true" : "false");
                                    }
                                    break;
                                }
                                case 0x10: // INT_DEC
                                    fprintf(stream, "\"%d\"", (int32_t)attr_val_data);
                                    break;
                                case 0x11: // INT_HEX
                                    fprintf(stream, "\"0x%08X\"", attr_val_data);
                                    break;
                                case 0x12: // INT_BOOLEAN
                                    fprintf(stream, "\"%s\"", attr_val_data ? "true" : "false");
                                    break;
                                case 0x1C:
                                case 0x1D:
                                case 0x1E:
                                case 0x1F: // COLOR
                                    fprintf(stream, "\"#%08X\"", attr_val_data);
                                    break;
                                default:
                                    // unknown
                                    if ((attr_val_data >> 24) >= 0x01 && attr_val_data != 0) {
                                        fprintf(stream, "\"@0x%08X\"", attr_val_data);
                                    } else if (attr_val_data != 0) {
                                        fprintf(stream, "\"0x%08X\"", attr_val_data);
                                    } else {
                                        fprintf(stream, "\"\"");
                                    }
                                    break;
                            }
                        }

//                        if (strcmp(full_attr_name, "android:package") == 0 && attr_value) {
//                            g_manifest.package_name = strdup(attr_value);
//                        } else if (strcmp(full_attr_name, "android:name") == 0 && attr_value) {
//                            if (strcmp(tag_name, "permission") == 0) {
//                                add_permission(attr_value);
//                            } else if (strcmp(tag_name, "activity") == 0) {
//                                add_activity(attr_value);
//                            } else if (strcmp(tag_name, "service") == 0) {
//                                add_service(attr_value);
//                            } else if (strcmp(tag_name, "receiver") == 0) {
//                                add_receiver(attr_value);
//                            } else if (strcmp(tag_name, "provider") == 0) {
//                                add_provider(attr_value);
//                            }
//                        }
                    }
                }

                fprintf(stream, ">\n");
                indent++;
                processed = 1;
                break;
            }

            case 0x0103: { // END_ELEMENT
                if (offset + header_size + 8 > buffer_size) break;

                const u1* payload = buffer + offset + header_size;
                u4 name_idx = read_uint32_le(payload + 4);

                const char* tag_name = get_string(&str_table, name_idx);
                if (!tag_name[0]) tag_name = "unknown";

                indent--;
                if (indent < 0) indent = 0;
                for (int i = 0; i < indent; i++) printf("  ");
                fprintf(stream, "</%s>\n", tag_name);
                processed = 1;
                break;
            }

            case 0x0104:
                processed = 1;
                break;

            default:
                processed = 1;
                break;
        }

        if (processed) {
            offset += chunk_size;
        } else {
            break;
        }
    }

    // clear string table
    if (str_table.table) {
        for (u4 i = 0; i < str_table.count; i++) {
            if (str_table.table[i]) free(str_table.table[i]);
        }
        free(str_table.table);
    }
}

static FILE* android_manifest_stream(jd_apk *apk)
{
    string full_dir = str_create("%s", apk->save_dir);
    mkdir_p(full_dir);

    string path = str_create("%s/AndroidManifest.xml", full_dir);
    FILE *stream = fopen(path, "w");
    if (stream == NULL) {
        fprintf(stdout, "[error]: open file %s failed: %d\n", path, errno);
        return NULL;
    }
    return stream;
}

/**
 * extract manifest form zip
 **/
void apk_parse_manifest_from_zip(jd_apk *apk)
{
    struct zip_t *zip = apk->zip;
    char *buf = NULL;
    size_t buf_size;
    buf_size = zip_entry_size(zip);
    buf = x_alloc_in(apk->pool, buf_size * sizeof(unsigned char));
    zip_entry_noallocread(zip, (void *)buf, buf_size);
    FILE *stream = android_manifest_stream(apk);
    parse_manifest_binary(stream, (u1*)buf, buf_size);
}


/**
 * test code
 * path is ab path of android manifest file
 **/
void apk_parse_manifest(string path)
{
    mem_pool *pool = mem_create_pool();
    apk_manifest *manifest = make_obj_in(apk_manifest, pool);
    manifest->pool = pool;

    FILE *file = fopen(path, "rb");
    if (!file) {
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 8) {
        fclose(file);
        return;
    }

    manifest->bin = make_obj_in(jd_bin, pool);
    manifest->bin->buffer = x_alloc_in(pool, file_size);
    manifest->bin->buffer_size = file_size;
    manifest->bin->cur_off = 0;

    size_t read_bytes = fread(manifest->bin->buffer, 1, file_size, file);
    fclose(file);

    if (read_bytes != file_size) {
        return;
    }

    parse_manifest_binary(NULL, (u1*)manifest->bin->buffer, manifest->bin->buffer_size);
}

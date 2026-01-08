#include <stdlib.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include "mem_pool.h"
#include "dex.h"
#include "dex_tools.h"
#include "debug.h"
#include "metadata.h"
#include "dex_meta_helper.h"
#include "decompiler/klass.h"

/*
 *  android 9.0 libdex/Leb128.h
 */

int read_unsigned_leb128(jd_meta_dex *dex)
{
    u1 uleb = 0;
    int result = 0;

    jdex_read1(dex, &uleb);
    result = uleb;
    if (result > 0x7f) {
        jdex_read1(dex, &uleb);
        result = (result & 0x7f) | ((uleb & 0x7f) << 7);
        if (uleb > 0x7f) {
            jdex_read1(dex, &uleb);
            result |= (uleb & 0x7f) << 14;
            if (uleb > 0x7f) {
                jdex_read1(dex, &uleb);
                result |= (uleb & 0x7f) << 21;
                if (uleb > 0x7f) {
                    jdex_read1(dex, &uleb);
                    result |= uleb << 28;
                }
            }
        }
    }
    return result;
}

int read_signed_leb128(jd_meta_dex *dex)
{
    s1 leb = 0;
    jdex_read1(dex, &leb);
    int result = leb;
    if(result <= 0x7f)
        result = (result << 25) >> 25;
    else {
        jdex_read1(dex, &leb);
        result = (result & 0x7f) | ((leb & 0x7f) << 7);
        if(leb <= 0x7f)
            result = (result << 18) >> 18;
        else {
            jdex_read1(dex, &leb);
            result |= (leb & 0x7f) << 14;
            if(leb <= 0x7f)
                result = (result << 11) >> 11;
            else {
                jdex_read1(dex, &leb);
                result |= (leb & 0x7f) << 21;
                if(leb <= 0x7f)
                    result = (result << 4) >> 4;
                else {
                    jdex_read1(dex, &leb);
                    result |= leb << 28;
                }
            }
        }
    }
    return result;
}

static void setup_current_offset(jd_meta_dex *dex, size_t offset)
{
    dex->bin->cur_off = offset;
}

static void parse_dex_header(jd_meta_dex *dex)
{
    dex->header = dex->bin->buffer;
}

static void parse_dex_links(jd_meta_dex *dex)
{
    dex->links = dex->bin->buffer + dex->header->link_off;
}

static void parse_dex_map_list(jd_meta_dex *dex)
{
    dex->maps = dex->bin->buffer + dex->header->map_off;
}

static int dex_string_byte_size(jd_meta_dex *dex, uint32_t utf16_size)
{
    int real, cnt;
    for (real = 0, cnt = 0; cnt < utf16_size; ++real, ++cnt) {
        unsigned char c = dex->bin->buffer[real + dex->bin->cur_off];
        if (c > 0 && c < 127) {
        }
        else if (c >= 0xE0) {
            real += 2;
        }
        else if (c >= 0xC2) {
            real += 1;
        }
        else if (c == 0xC0) {
            unsigned char nc = dex->bin->buffer[real+1 + dex->bin->cur_off];
            if (nc == 0x80)
                real += 1;
        }
    }
    return real;
}

static void parse_dex_string_ids(jd_meta_dex *dex)
{
    dex_header *header = dex->header;
    dex->string_ids = dex->bin->buffer + header->string_ids_off;


    dex->strings = make_obj_arr_in(dex_string_item,
                                   header->string_ids_size,
                                   dex->pool);
    for (int i = 0; i < header->string_ids_size; ++i) {
        dex_string_id *string_id = &dex->string_ids[i];
        dex_string_item *item = &dex->strings[i];

        setup_current_offset(dex, string_id->string_data_off);

        int size = read_unsigned_leb128(dex);
        item->utf16_size = size;

        int real = dex_string_byte_size(dex, size);

        item->data = x_alloc_in(dex->pool, real+1);
        memset(item->data, 0, real+1);
        if (real > 0)
            jdex_read(dex, item->data, real);
        item->data[real] = '\0';
    }
}

static void parse_dex_type_ids(jd_meta_dex *dex)
{
    dex->type_ids = dex->bin->buffer + dex->header->type_ids_off;
}

static void parse_dex_proto_ids(jd_meta_dex *dex)
{
    mem_pool *pool = dex->pool;
    u4 size = 0;
    dex_header *header = dex->header;
    setup_current_offset(dex, header->proto_ids_off);
    size_t dex_type_item_size = sizeof(dex_type_item);
    size = header->proto_ids_size;
    dex->proto_ids = make_obj_arr_in(dex_proto_id, size, pool);
    for (int i = 0; i < header->proto_ids_size; ++i)
    {
        dex_proto_id *proto_id = &dex->proto_ids[i];
        jdex_read4(dex, &proto_id->shorty_idx);
        jdex_read4(dex, &proto_id->return_type_idx);
        jdex_read4(dex, &proto_id->parameters_off);
    }

    for (int i = 0; i < header->proto_ids_size; ++i)
    {
        dex_proto_id *proto_id = &dex->proto_ids[i];
        if (proto_id->parameters_off == 0) {
            proto_id->type_list = NULL;
        }
        else {
            setup_current_offset(dex, proto_id->parameters_off);
            proto_id->type_list = make_obj_in(dex_type_list, dex->pool);
            jdex_read4(dex, &proto_id->type_list->size);

            size = proto_id->type_list->size;
            dex_type_item *list = make_obj_arr_in(dex_type_item, size, pool);
            proto_id->type_list->list = list;
            jdex_read(dex, list, dex_type_item_size*size);
        }
    }
    DEBUG_PRINT("[proto_ids_size]: %d\n", header->proto_ids_size);
}

static void parse_dex_field_ids(jd_meta_dex *dex)
{
    dex->field_ids = dex->bin->buffer + dex->header->field_ids_off;
}

static void parse_dex_method_ids(jd_meta_dex *dex)
{
    dex->method_ids = dex->bin->buffer + dex->header->method_ids_off;
}

static void parse_dex_class_interfaces(jd_meta_dex *dex, dex_class_def *cdef)
{
    if (cdef->interfaces_off == 0)
        return;

    setup_current_offset(dex, cdef->interfaces_off);
    cdef->interfaces = make_obj_in(dex_type_list, dex->pool);
    dex_type_list *interfaces = cdef->interfaces;
    jdex_read4(dex, &interfaces->size);

    u4 size = interfaces->size;
    size_t item_size = sizeof(dex_type_item);
    cdef->interfaces->list = make_obj_arr_in(dex_type_item, size, dex->pool);
    jdex_read(dex, interfaces->list, item_size*size);
}

static void parse_dex_encoded_value(jd_meta_dex *dex, encoded_value *value)
{
    mem_pool *pool = dex->pool;
    u4 size = 0;
    jdex_read1(dex, &value->strict_value);
    value->value_type = value->strict_value & 0x1F;
    value->value_arg = value->strict_value >> 5;
    value->value_length = value->value_arg + 1;
    switch (value->value_type) {
        case kDexAnnotationBoolean:
            value->value_length = 0;
            break;
        case kDexAnnotationByte:
            value->value = make_obj_in(u1, pool);
            jdex_read1(dex, value->value);
            break;
        case kDexAnnotationShort:
        case kDexAnnotationChar:
        case kDexAnnotationInt:
        case kDexAnnotationLong:
        case kDexAnnotationFloat:
        case kDexAnnotationMethod:
        case kDexAnnotationDouble:
        case kDexAnnotationString:
        case kDexAnnotationType:
        case kDexAnnotationMethodType:
        case kDexAnnotationMethodHandle:
        case kDexAnnotationField:
        case kDexAnnotationEnum:
            value->value = x_alloc_in(dex->pool, value->value_length);
            memset(value->value, 0, value->value_length);
            jdex_read(dex, value->value, value->value_length);
            break;
        case kDexAnnotationArray: {
            encoded_array *en_array = make_obj_in(encoded_array, pool);
            size = read_unsigned_leb128(dex);
            en_array->size = size;
            en_array->values = make_obj_arr_in(encoded_value, size, pool);
            for (int i = 0; i < en_array->size; ++i) {
                encoded_value *v = &en_array->values[i];
                parse_dex_encoded_value(dex, v);
            }
            value->value = (u1*)en_array;
            break;
        }
        case kDexAnnotationAnnotation: {
            encoded_annotation *anno = make_obj_in(encoded_annotation, pool);
            anno->type_idx = read_unsigned_leb128(dex);

            size = read_unsigned_leb128(dex);
            anno->size = size;
            anno->elements = make_obj_arr_in(annotation_element, size, pool);
            for (int i = 0; i < anno->size; ++i) {
                annotation_element *element = &anno->elements[i];
                element->name_idx = read_unsigned_leb128(dex);
                element->value = make_obj_in(encoded_value, pool);
                parse_dex_encoded_value(dex, element->value);
            }
            value->value = (u1*)anno;
            break;
        }
        case kDexAnnotationNull:
            value->value_length = 0;
            break;
        default:
            fprintf(stderr, "[annotation value]: value_type: %0x\n",
                    value->value_type);
            abort();
    }
}

static void parse_dex_annotation_off_item(jd_meta_dex *dex,
                                          annotation_off_item *off_item)
{
    mem_pool *pool = dex->pool;
    u4 size = 0;

    off_item->annotation_item = make_obj_in(annotation_item, pool);
    setup_current_offset(dex, off_item->annotations_off);

    annotation_item *item = off_item->annotation_item;
    item->encoded_annotation = make_obj_in(encoded_annotation, pool);
    jdex_read1(dex, &item->visibility);

    encoded_annotation *eanno = item->encoded_annotation;
    eanno->type_idx = read_unsigned_leb128(dex);
    size = read_unsigned_leb128(dex);
    eanno->size = size;
    eanno->elements = make_obj_arr_in(annotation_element, size, pool);
    for (int i = 0; i < eanno->size; ++i) {
        annotation_element *element = &eanno->elements[i];
        element->name_idx = read_unsigned_leb128(dex);
        element->value = make_obj_in(encoded_value, dex->pool);

        parse_dex_encoded_value(dex, element->value);
    }

}

static annotation_set_item* parse_dex_ano_set_item(jd_meta_dex *dex,
                                                   u4 offset)
{
    setup_current_offset(dex, offset);
    annotation_set_item *item = make_obj_in(annotation_set_item, dex->pool);
    jdex_read4(dex, &item->size);
    item->entries = make_obj_arr_in(annotation_off_item, item->size, dex->pool);
    for (int i = 0; i < item->size; ++i) {
        annotation_off_item *off_item = &item->entries[i];
        jdex_read4(dex, &off_item->annotations_off);
    }

    for (int i = 0; i < item->size; ++i) {
        annotation_off_item *off_item = &item->entries[i];
        parse_dex_annotation_off_item(dex, off_item);
    }
    return item;
}

static ano_ref_list* parse_dex_ano_ref_list(jd_meta_dex *dex, u4 offset)
{
    setup_current_offset(dex, offset);
    ano_ref_list *ref_list = make_obj_in(ano_ref_list, dex->pool);

    jdex_read4(dex, &ref_list->size);
    ref_list->list = make_obj_arr_in(ano_ref_item, ref_list->size, dex->pool);
    for (int i = 0; i < ref_list->size; ++i) {
        annotation_set_ref_item *item = &ref_list->list[i];
        jdex_read4(dex, &item->annotations_off);
    }

    for (int i = 0; i < ref_list->size; ++i) {
        annotation_set_ref_item *item = &ref_list->list[i];
        u4 off = item->annotations_off;
        if (off > 0)
            item->annotation = parse_dex_ano_set_item(dex,off);
    }

    return ref_list;
}

static void parse_dex_class_defs_anos(jd_meta_dex *dex, dex_class_def *cf)
{
    if (cf->annotations_off == 0)
        return;
    setup_current_offset(dex, cf->annotations_off);

    mem_pool *pool = dex->pool;
    u4 size = 0;
    u4 off = 0;

    dex_ano_dict_item *dict = make_obj_in(dex_ano_dict_item, pool);
    cf->annotations = dict;
    jdex_read4(dex, &dict->class_annotations_off);
    jdex_read4(dex, &dict->fields_size);
    jdex_read4(dex, &dict->methods_size);
    jdex_read4(dex, &dict->parameters_size);

    size = dict->fields_size;
    if (size > 0) {
        dict->field_annotations = make_obj_arr_in(field_annotation, size, pool);
        for (int j = 0; j < size; ++j) {
            field_annotation *fa = &dict->field_annotations[j];
            jdex_read4(dex, &fa->field_idx);
            jdex_read4(dex, &fa->annotations_off);
        }
    }

    size = dict->methods_size;
    if (size > 0) {
        dict->method_annotations = make_obj_arr_in(meth_ano, size, pool);
        for (int j = 0; j < size; ++j) {
            meth_ano *ma = &dict->method_annotations[j];
            jdex_read4(dex, &ma->method_idx);
            jdex_read4(dex, &ma->annotations_off);
        }
    }

    size = dict->parameters_size;
    if (size > 0) {
        dict->parameter_annotations = make_obj_arr_in(param_ano, size, pool);
        for (int j = 0; j < size; ++j) {
            parameter_annotation *pa = &dict->parameter_annotations[j];
            jdex_read4(dex, &pa->method_idx);
            jdex_read4(dex, &pa->annotations_off);
        }
    }

    off = dict->class_annotations_off;
    if (off > 0)
        dict->class_annotation = parse_dex_ano_set_item(dex,off);

    size = dict->fields_size;
    if (size > 0) {
        for (int j = 0; j < size; ++j) {
            field_annotation *fa = &dict->field_annotations[j];
            fa->annotation = parse_dex_ano_set_item(dex, fa->annotations_off);
        }
    }

    if (dict->methods_size > 0) {
        for (int j = 0; j < dict->methods_size; ++j) {
            method_annotation *ma = &dict->method_annotations[j];
            ma->annotation = parse_dex_ano_set_item(dex, ma->annotations_off);
        }
    }

    if (dict->parameters_size > 0) {
        for (int j = 0; j < dict->parameters_size; ++j) {
            parameter_annotation *pa = &dict->parameter_annotations[j];
            pa->annotation = parse_dex_ano_ref_list(dex, pa->annotations_off);
        }
    }
}

static void parse_dex_encoded_field(jd_meta_dex *dex, encoded_field *field)
{
    field->field_idx_diff = read_unsigned_leb128(dex);
    field->access_flags = read_unsigned_leb128(dex);
}

static void parse_dex_encoded_method(jd_meta_dex *dex, encoded_method *method)
{
    method->method_idx_diff = read_unsigned_leb128(dex);
    method->access_flags = read_unsigned_leb128(dex);
    method->code_off = read_unsigned_leb128(dex);
}

static void parse_dex_code_try_item(jd_meta_dex *dex, dex_code_item *code)
{
    mem_pool *pool = dex->pool;
    if (code->tries_size == 0)
        return;

    if (code->tries_size > 0 && code->insns_size % 2 > 0) {
        jdex_read2(dex, &code->padding);
    }
    if (code->tries_size == 0)
        return;

    code->tries = make_obj_arr_in(dex_try_item, code->tries_size, pool);
    for (int i = 0; i < code->tries_size; ++i) {
        dex_try_item *try_item = &code->tries[i];
        jdex_read4(dex, &try_item->start_addr);
        jdex_read2(dex, &try_item->insn_count);
        jdex_read2(dex, &try_item->handler_off);
        DEBUG_PRINT("[try_item]:"
                    "startAddr: %d "
                    "insnCount: %d, "
                    "range: %d -> %d "
                    "handlerOff: %d\n",
                    try_item->start_addr,
                    try_item->insn_count,
                    try_item->start_addr,
                    try_item->start_addr + try_item->insn_count - 1,
                    try_item->handler_off);
    }

    u4 handler_offset = dex->bin->cur_off;
    code->handlers = make_obj_in(encoded_catch_handler_list, dex->pool);
    u4 size = read_unsigned_leb128(dex);
    code->handlers->size = size;
    code->handlers->list = make_obj_arr_in(encoded_catch_handler, size, pool);
    DEBUG_PRINT("handlers size: %d\n", code->handlers->size);
    for (int j = 0; j < code->handlers->size; ++j) {
        DEBUG_PRINT("handler: %zu\n", dex->bin->cur_off - handler_offset);
        encoded_catch_handler *handler = &code->handlers->list[j];
        handler->handler_off = dex->bin->cur_off - handler_offset;
        handler->size = read_signed_leb128(dex);

        if (handler->size != 0) {
            int sz = abs(handler->size);
            encoded_tp *pairs = make_obj_arr_in(encoded_tp, sz, pool);
            handler->handlers = pairs;
            for (int k = 0; k < abs(handler->size); ++k) {
                encoded_tp *pair = &pairs[k];
                pair->type_idx = read_unsigned_leb128(dex);
                pair->addr = read_unsigned_leb128(dex);
                DEBUG_PRINT("\ttype_idx: %d addr: %d\n",
                            pair->type_idx, pair->addr);
            }
        }
        if (handler->size <= 0) {
            handler->catch_all_addr = read_unsigned_leb128(dex);
            DEBUG_PRINT("\tcatch_all_addr: %d\n", handler->catch_all_addr);
        }
    }
}

static void parse_dex_code_debug_item(jd_meta_dex *dex, dex_code_item *code)
{
    mem_pool *pool = dex->pool;
    if (code->debug_info_off == 0)
        return;

    setup_current_offset(dex, code->debug_info_off);
    code->debug_info = make_obj_in(dex_debug_info_item, pool);
    dex_debug_info_item *debug_info = code->debug_info;
    debug_info->line_start = read_unsigned_leb128(dex);
    debug_info->parameters_size = read_unsigned_leb128(dex);
    debug_info->parameter_name = make_obj_arr_in(u4, debug_info->parameters_size, pool);

    u4 current_offset = dex->bin->cur_off;

    DEBUG_PRINT("[DEBUG_INFO]: %d line_start: %d, parameters_size: %d\n",
           code->debug_info_off,
           debug_info->line_start,
           debug_info->parameters_size);

    u2 dbg_size = 0;

    for (int i = 0; i < debug_info->parameters_size; ++i) {
        read_unsigned_leb128(dex);
    }
    dbg_size += debug_info->parameters_size;
    u1 dbg_opcode;
    while (1) {
        jdex_read1(dex, &dbg_opcode);
        if (dbg_opcode == DBG_END_SEQUENCE)
            break;
        switch (dbg_opcode) {
            case DBG_ADVANCE_LINE: {
                read_signed_leb128(dex);
                break;
            }
            case DBG_START_LOCAL: {
                read_unsigned_leb128(dex);
                read_unsigned_leb128(dex);
                read_unsigned_leb128(dex);
                dbg_size ++;
                break;
            }
            case DBG_START_LOCAL_EXTENDED: {
                s4 reg = read_unsigned_leb128(dex);
                u4 name_idx = read_unsigned_leb128(dex);
                u4 type_idx = read_unsigned_leb128(dex);
                u4 sig_idx = read_unsigned_leb128(dex);
                dbg_size ++;
                break;
            }
            case DBG_SET_FILE:
            case DBG_END_LOCAL:
            case DBG_ADVANCE_PC:
            case DBG_RESTART_LOCAL: {
                read_unsigned_leb128(dex);
                break;
            }
            case DBG_SET_PROLOGUE_END:
            case DBG_SET_EPILOGUE_BEGIN: {
                break;
            }
            default: {
                continue;
            }
        }
    }

    setup_current_offset(dex, current_offset);
    dex_dbg_item *items = make_obj_arr_in(dex_dbg_item, dbg_size, pool);
    debug_info->dbg_size = dbg_size;
    debug_info->items = items;
    u4 itor = 0;
    for (int i = 0; i < debug_info->parameters_size; ++i) {
        u4 name_idx = read_unsigned_leb128(dex) - 1;
        debug_info->parameter_name[i] = name_idx;
        if (name_idx != NO_INDEX) {
            string param_name = dex_str_of_idx(dex, name_idx);
            DEBUG_PRINT("\tparameter_name: %d %s index: %d\n",
                   name_idx, param_name, i);
        }
        dex_dbg_item *dbg_item = &items[itor];
        dbg_item->name_idx = debug_info->parameter_name[i];
        dbg_item->code = DBG_START_LOCAL;
        dbg_item->offset = 0;
        dbg_item->reg_num = i;
        itor++;
    }

    dbg_opcode = 0;
    u4 address = 0;
    u4 line = 0;
    while (1) {
        jdex_read1(dex, &dbg_opcode);
        if (dbg_opcode == DBG_END_SEQUENCE)
            break;
        switch (dbg_opcode) {
            case DBG_ADVANCE_PC: {
                u4 addr_diff = read_unsigned_leb128(dex);
                address += addr_diff;
                DEBUG_PRINT("\t[DBG_ADVANCE_PC]: %d\n", addr_diff);
                break;
            }
            case DBG_ADVANCE_LINE: {
                s4 line_diff = read_signed_leb128(dex);
                DEBUG_PRINT("\t[DBG_ADVANCE_LINE]: %d\n", line_diff);
                break;
            }
            case DBG_START_LOCAL: {
                s4 reg = read_unsigned_leb128(dex);
                u4 name_idx = read_unsigned_leb128(dex) - 1;
                u4 type_idx = read_unsigned_leb128(dex) - 1;

                dex_dbg_item *dbg_item = &items[itor];
                dbg_item->code = DBG_START_LOCAL;
                dbg_item->offset = address;
                dbg_item->reg_num = reg;
                dbg_item->name_idx = name_idx;
                itor++;
                break;
            }
            case DBG_START_LOCAL_EXTENDED: {
                s4 reg = read_unsigned_leb128(dex);
                u4 name_idx = read_unsigned_leb128(dex) - 1;
                u4 type_idx = read_unsigned_leb128(dex) - 1;
                u4 sig_idx = read_unsigned_leb128(dex) -1;

                dex_dbg_item *dbg_item = &items[itor];
                dbg_item->code = DBG_START_LOCAL;
                dbg_item->offset = address;
                dbg_item->reg_num = reg;
                dbg_item->name_idx = name_idx;
                itor++;
                break;
            }
            case DBG_END_LOCAL: {
                s4 reg = read_unsigned_leb128(dex);
                DEBUG_PRINT("\t[DBG_END_LOCAL]: %d\n", reg);
                break;
            }
            case DBG_RESTART_LOCAL: {
                s4 reg = read_unsigned_leb128(dex);
                DEBUG_PRINT("\t[DBG_RESTART_LOCAL]: %d\n", reg);
                break;
            }
            case DBG_SET_PROLOGUE_END: {
                DEBUG_PRINT("\t[DBG_SET_PROLOGUE_END]\n");
                break;
            }
            case DBG_SET_EPILOGUE_BEGIN: {
                DEBUG_PRINT("\t[DBG_SET_EPILOGUE_BEGIN]\n");
                break;
            }
            case DBG_SET_FILE: {
                u4 name_idx = read_unsigned_leb128(dex) - 1;
                DEBUG_PRINT("\t[DBG_SET_FILE]: %d\n", name_idx);
                break;
            }
            default: {
                u4 adjusted_opcode = dbg_opcode - DBG_FIRST_SPECIAL;
                address += (adjusted_opcode / DBG_LINE_RANGE);
                DEBUG_PRINT("\t[DBG_SPEC]: %d addr: %d\n", dbg_opcode, address);
                continue;
            }
        }
    }
}

static void parse_dex_code_item(jd_meta_dex *dex, encoded_method *em)
{
    mem_pool *pool = dex->pool;
    dex_method_id *method_id = &dex->method_ids[em->method_id];
    string method_name = dex->strings[method_id->name_idx].data;
    setup_current_offset(dex, em->code_off);
    em->code = make_obj_in(dex_code_item, pool);
    dex_code_item *code = em->code;
    jdex_read2(dex, &code->registers_size);
    jdex_read2(dex, &code->ins_size);
    jdex_read2(dex, &code->outs_size);
    jdex_read2(dex, &code->tries_size);
    jdex_read4(dex, &code->debug_info_off);
    jdex_read4(dex, &code->insns_size);

    code->insns = dex->bin->buffer + dex->bin->cur_off;
    setup_current_offset(dex, dex->bin->cur_off + sizeof(u2)*code->insns_size);

    parse_dex_code_try_item(dex, code);

    DEBUG_PRINT("[code_item]: %s registers_size: %d "
                "ins_size: %d outs_size: %d "
                "tries_size: %d insns_size: %d\n",
                method_name,
                code->registers_size,
                code->ins_size,
                code->outs_size,
                code->tries_size,
                code->insns_size);
    parse_dex_code_debug_item(dex, code);
}

static void parse_dex_class_data_items(jd_meta_dex *dex, dex_class_def *cdef)
{
    if (cdef->class_data_off == 0)
        return;

    setup_current_offset(dex, cdef->class_data_off);
    cdef->class_data = make_obj_in(dex_class_data_item, dex->pool);
    dex_class_data_item *cdata = cdef->class_data;

    cdata->static_fields_size      = read_unsigned_leb128(dex);
    cdata->instance_fields_size    = read_unsigned_leb128(dex);
    cdata->direct_methods_size     = read_unsigned_leb128(dex);
    cdata->virtual_methods_size    = read_unsigned_leb128(dex);

    u4 size = 0;
    mem_pool *pool = dex->pool;
    if (cdata->static_fields_size > 0) {
        size = cdata->static_fields_size;
        cdata->static_fields = make_obj_arr_in(encoded_field, size, pool);
        u4 start_idx = 0;
        for (int i = 0; i < cdata->static_fields_size; ++i) {
            encoded_field *field = &cdata->static_fields[i];
            parse_dex_encoded_field(dex, field);
            if (i == 0) {
                start_idx = field->field_idx_diff;
                field->field_id = start_idx;
            }
            else {
                field->field_id = start_idx + field->field_idx_diff;
                start_idx = field->field_id;
            }
        }
    }

    if (cdata->instance_fields_size > 0) {
        size = cdata->instance_fields_size;
        cdata->instance_fields = make_obj_arr_in(encoded_field, size, pool);
        u4 start_idx = 0;
        for (int i = 0; i < cdata->instance_fields_size; ++i) {
            encoded_field *field = &cdata->instance_fields[i];
            parse_dex_encoded_field(dex, field);
            if (i == 0) {
                start_idx = field->field_idx_diff;
                field->field_id = start_idx;
            }
            else {
                field->field_id = start_idx + field->field_idx_diff;
                start_idx = field->field_id;
            }
        }
    }

    if (cdata->direct_methods_size > 0) {
        size = cdata->direct_methods_size;
        cdata->direct_methods = make_obj_arr_in(encoded_method, size, pool);
        u4 start_idx = 0;
        for (int i = 0; i < cdata->direct_methods_size; ++i) {
            encoded_method *method = &cdata->direct_methods[i];
            parse_dex_encoded_method(dex, method);
            if (i == 0) {
                start_idx = method->method_idx_diff;
                method->method_id = start_idx;
            }
            else {
                method->method_id = start_idx + method->method_idx_diff;
                start_idx = method->method_id;
            }

            if (dex_encoded_method_is_lambda(dex, method)) {
                hset_u4obj(dex->lambda_method_map, method->method_id, method);
            }
        }
    }

    if (cdata->virtual_methods_size > 0) {
        size = cdata->virtual_methods_size;
        cdata->virtual_methods = make_obj_arr_in(encoded_method, size, pool);
        u4 start_idx = 0;
        for (int i = 0; i < cdata->virtual_methods_size; ++i) {
            encoded_method *method = &cdata->virtual_methods[i];
            parse_dex_encoded_method(dex, method);
            if (i == 0) {
                start_idx = method->method_idx_diff;
                method->method_id = start_idx;
            }
            else {
                method->method_id = start_idx + method->method_idx_diff;
                start_idx = method->method_id;
            }

             if (dex_encoded_method_is_lambda(dex, method)) {
                 hset_u4obj(dex->lambda_method_map, method->method_id, method);
             }
        }
    }

    DEBUG_PRINT("[class_data_item]: static_fields_size: %d, "
           "instance_fields_size: %d, direct_methods_size: %d, "
           "virtual_methods_size: %d\n",
                cdata->static_fields_size,
                cdata->instance_fields_size,
                cdata->direct_methods_size,
                cdata->virtual_methods_size);

    for (int i = 0; i < cdata->direct_methods_size; ++i) {
        encoded_method *method = &cdata->direct_methods[i];
        if (method->code_off == 0)
            continue;
        parse_dex_code_item(dex, method);
    }

    for (int i = 0; i < cdata->virtual_methods_size; ++i) {
        encoded_method *method = &cdata->virtual_methods[i];
        if (method->code_off == 0)
            continue;
        parse_dex_code_item(dex, method);
    }
}

static void parse_dex_class_defs(jd_meta_dex *dex)
{
    dex_header *header = dex->header;
    mem_pool *pool = dex->pool;
    setup_current_offset(dex, header->class_defs_off);

    u4 size = header->class_defs_size;
    dex->class_defs = make_obj_arr_in(dex_class_def, size, pool);
    for (int i = 0; i < header->class_defs_size; ++i) {
        dex_class_def *class_def = &dex->class_defs[i];
        jdex_read4(dex, &class_def->class_idx);
        jdex_read4(dex, &class_def->access_flags);
        jdex_read4(dex, &class_def->superclass_idx);
        jdex_read4(dex, &class_def->interfaces_off);
        jdex_read4(dex, &class_def->source_file_idx);
        jdex_read4(dex, &class_def->annotations_off);
        jdex_read4(dex, &class_def->class_data_off);
        jdex_read4(dex, &class_def->static_values_off);

        class_def->is_inner = false;
        class_def->is_anonymous = false;
        if ((class_def->access_flags & ACC_DEX_SYNTHETIC) != 0 &&
                (class_def->access_flags & ACC_DEX_FINAL) != 0) {
            hset_u4obj(dex->synthetic_classes_map, class_def->class_idx,
                       class_def);
        }
        hset_u4obj(dex->class_type_id_map, class_def->class_idx,
                   class_def);

        string cname = dex_str_of_type_id(dex, class_def->class_idx);
        hset_s2o(dex->class_name_map, cname, class_def);

        class_def->anonymous_classes = linit_object_with_pool(dex->pool);
        class_def->inner_classes = linit_object_with_pool(dex->pool);
//        printf("[class_def]: %d %s %p\n", class_def->class_idx, cname, class_def);
    }

    for (int i = 0; i < header->class_defs_size; ++i) {
        dex_class_def *class_def = &dex->class_defs[i];
        parse_dex_class_interfaces(dex, class_def);
        parse_dex_class_defs_anos(dex, class_def);
        parse_dex_class_data_items(dex, class_def);

        if (dex_class_is_anonymous_class(dex, class_def))
            class_def->is_anonymous = true;
        else if (dex_class_is_inner_class(dex, class_def))
            class_def->is_inner = true;
    }

    DEBUG_PRINT("[class_defs_size]: %d\n", header->class_defs_size);
}

static jd_meta_dex* init_dex_content(mem_pool *pool, string path)
{
    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    jd_meta_dex *dex = make_obj_in(jd_meta_dex, pool);
    dex->pool = pool;
    dex->bin = make_obj_in(jd_bin, pool);
    dex->bin->buffer_size = file_size;
    dex->bin->buffer = x_alloc_in(pool, file_size);
    dex->bin->cur_off = 0;
    fread(dex->bin->buffer, 1, file_size, file);
//    dalvik->buffer_size = file_size;
//    dalvik->buffer = x_alloc_in(pool, file_size);
//    fread(dalvik->buffer, 1, file_size, file);
//    dalvik->cur_off = 0;
    fclose(file);
    return dex;
}

static void init_dex_extract_data(jd_meta_dex *dex)
{
    dex->synthetic_classes_map = hashmap_init_in(dex->pool, u4obj_cmp, 0);
    dex->lambda_method_map = hashmap_init_in(dex->pool, u4obj_cmp, 0);
    dex->class_type_id_map = hashmap_init_in(dex->pool, u4obj_cmp, 0);
    dex->class_name_map = hashmap_init_in(dex->pool, s2o_cmp, 0);
}

jd_meta_dex* parse_dex_file(string path)
{
//    init_dex_opcode_hashmap();

    mem_pool *pool = mem_create_pool();

    jd_meta_dex *dex = init_dex_content(pool, path);

    init_dex_extract_data(dex);

    parse_dex_header(dex);

    parse_dex_links(dex);

    parse_dex_map_list(dex);

    parse_dex_string_ids(dex);

    parse_dex_type_ids(dex);

    parse_dex_proto_ids(dex);

    parse_dex_field_ids(dex);

    parse_dex_method_ids(dex);

    parse_dex_class_defs(dex);

    return dex;
}


jd_meta_dex* parse_dex_from_buffer(char *buffer, size_t size)
{
    mem_pool *pool = mem_create_pool();
    jd_meta_dex *meta = make_obj_in(jd_meta_dex, pool);
    meta->pool = pool;
    meta->bin = make_obj_in(jd_bin, pool);
    meta->bin->buffer_size = size;
    meta->bin->buffer = buffer;
    meta->bin->cur_off = 0;

    init_dex_extract_data(meta);

    parse_dex_header(meta);

    parse_dex_links(meta);

    parse_dex_map_list(meta);

    parse_dex_string_ids(meta);

    parse_dex_type_ids(meta);

    parse_dex_proto_ids(meta);

    parse_dex_field_ids(meta);

    parse_dex_method_ids(meta);

    parse_dex_class_defs(meta);

    return meta;
}
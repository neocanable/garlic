#include "common/output_error.h"
#include "parser/dex/metadata.h"
#include "dex_ins.h"
#include "dex_ins_helper.h"
#include "dex_meta_helper.h"
#include "dex_class.h"
#include "dex_method.h"


#include "klass.h"
#include "common/str_tools.h"
#include "common/file_tools.h"

#include <string.h>

static FILE* _smali_stream(FILE *out) {
    return out == NULL ? stdout : out;
}

static u4 smali_u32_at(const dex_code_item *code, int i)
{
    return ((u4) code->insns[i + 1] << 16) | code->insns[i];
}

static u8 smali_u64_at(const dex_code_item *code, int i)
{
    u8 low = smali_u32_at(code, i);
    u8 high = smali_u32_at(code, i + 2);
    return (high << 32) | low;
}

static void smali_int(FILE *stream, s4 v)
{
    if (v < 0)
        fprintf(stream, "-0x%x", (unsigned) (-(s8) v));
    else
        fprintf(stream, "0x%x", (unsigned) v);
}

static void smali_long(FILE *stream, s8 v)
{
    if (v < 0)
        fprintf(stream, "-0x%llxL", (unsigned long long) (-(u8) v));
    else
        fprintf(stream, "0x%llxL", (unsigned long long) v);
}

static void smali_string(FILE *stream, string str)
{
    fputc('"', stream);

    const unsigned char *p = (const unsigned char *) str;
    while (p != NULL && *p != '\0') {
        unsigned c = *p;
        unsigned cp;
        int extra;

        if (c < 0x80)       { cp = c;         extra = 0; }
        else if (c >= 0xC0 && c < 0xE0) { cp = c & 0x1F; extra = 1; }
        else if (c >= 0xE0 && c < 0xF0) { cp = c & 0x0F; extra = 2; }
        else if (c >= 0xF0 && c < 0xF8) { cp = c & 0x07; extra = 3; }
        else                { cp = c;         extra = 0; }

        for (int k = 1; k <= extra; ++k) {
            if (p[k] == '\0' || (p[k] & 0xC0) != 0x80) {
                cp = c;
                extra = 0;
                break;
            }
            cp = (cp << 6) | (p[k] & 0x3F);
        }
        p += 1 + extra;

        switch (cp) {
            case '"':  fputs("\\\"", stream); continue;
            case '\\': fputs("\\\\", stream); continue;
            case '\n': fputs("\\n", stream); continue;
            case '\r': fputs("\\r", stream); continue;
            case '\t': fputs("\\t", stream); continue;
            default: break;
        }

        if (cp >= 0x20 && cp < 0x7F) {
            fputc((int) cp, stream);
        }
        else if (cp > 0xFFFF) {
            unsigned v = cp - 0x10000;
            fprintf(stream, "\\u%04x\\u%04x",
                    0xD800 + (v >> 10), 0xDC00 + (v & 0x3FF));
        }
        else {
            fprintf(stream, "\\u%04x", cp);
        }
    }
    fputc('"', stream);
}

static void smali_type_ref(FILE *stream, jd_meta_dex *dex, u4 type_idx)
{
    fprintf(stream, "%s", dex_str_of_type_id(dex, (u2) type_idx));
}

static void smali_proto(FILE *stream, jd_meta_dex *dex, u4 proto_idx)
{
    dex_proto_id *proto = &dex->proto_ids[proto_idx];

    fputc('(', stream);
    if (proto->parameters_off != 0 && proto->type_list != NULL) {
        for (int i = 0; i < proto->type_list->size; ++i)
            fprintf(stream, "%s",
                    dex_str_of_type_id(dex, proto->type_list->list[i].type_idx));
    }
    fputc(')', stream);
    fprintf(stream, "%s", dex_str_of_type_id(dex, proto->return_type_idx));
}

static void smali_field_ref(FILE *stream, jd_meta_dex *dex, u4 field_idx)
{
    dex_field_id *field_id = &dex->field_ids[field_idx];
    fprintf(stream, "%s->%s:%s",
            dex_str_of_type_id(dex, field_id->class_idx),
            dex_str_of_idx(dex, field_id->name_idx),
            dex_str_of_type_id(dex, field_id->type_idx));
}

static void smali_method_ref(FILE *stream, jd_meta_dex *dex, u4 method_idx)
{
    dex_method_id *method_id = &dex->method_ids[method_idx];
    fprintf(stream, "%s->%s",
            dex_str_of_type_id(dex, method_id->class_idx),
            dex_str_of_idx(dex, method_id->name_idx));
    smali_proto(stream, dex, method_id->proto_idx);
}

typedef enum {
    REF_NONE = 0,
    REF_TYPE,
    REF_FIELD,      // iget/iput, sget/sput
    REF_METHOD,     // invoke-*
    REF_STRING,     // const-string
    REF_PROTO,      // const-method-type
    REF_HANDLE,     // const-method-handle
    REF_CALLSITE    // invoke-custom
} smali_ref_kind;

static smali_ref_kind smali_ref_of(u1 op)
{
    switch (op) {
        case DEX_INS_CONST_STRING:
        case DEX_INS_CONST_STRING_JUMBO:
            return REF_STRING;

        case DEX_INS_CONST_CLASS:
        case DEX_INS_CHECK_CAST:
        case DEX_INS_INSTANCE_OF:
        case DEX_INS_NEW_INSTANCE:
        case DEX_INS_NEW_ARRAY:
        case DEX_INS_FILLED_NEW_ARRAY:
        case DEX_INS_FILLED_NEW_ARRAY_RANGE:
            return REF_TYPE;

        case DEX_INS_CONST_METHOD_TYPE:
            return REF_PROTO;
        case DEX_INS_CONST_METHOD_HANDLE:
            return REF_HANDLE;
        case DEX_INS_INVOKE_CUSTOM:
        case DEX_INS_INVOKE_CUSTOM_RANGE:
            return REF_CALLSITE;

        default:
            break;
    }

    if (op >= DEX_INS_IGET && op <= DEX_INS_IPUT_SHORT)
        return REF_FIELD;
    if (op >= DEX_INS_SGET && op <= DEX_INS_SPUT_SHORT)
        return REF_FIELD;
    if (op >= DEX_INS_INVOKE_VIRTUAL && op <= DEX_INS_INVOKE_INTERFACE)
        return REF_METHOD;
    if (op >= DEX_INS_INVOKE_VIRTUAL_RANGE && op <= DEX_INS_INVOKE_INTERFACE_RANGE)
        return REF_METHOD;
    if (op == DEX_INS_INVOKE_POLYMORPHIC || op == DEX_INS_INVOKE_POLYMORPHIC_RANGE)
        return REF_METHOD;

    return REF_NONE;
}

static void smali_write_ref(FILE *stream, jd_meta_dex *dex,
                            smali_ref_kind kind, u4 idx)
{
    switch (kind) {
        case REF_TYPE:  smali_type_ref(stream, dex, idx); break;
        case REF_FIELD: smali_field_ref(stream, dex, idx); break;
        case REF_METHOD: smali_method_ref(stream, dex, idx); break;
        case REF_STRING:
            smali_string(stream, dex_str_of_idx(dex, idx));
            break;
        case REF_PROTO:     fprintf(stream, "proto@0x%x", idx); break;
        case REF_HANDLE:    fprintf(stream, "method_handle@0x%x", idx); break;
        case REF_CALLSITE:  fprintf(stream, "call_site@0x%x", idx); break;
        case REF_NONE:      break;
    }
}

#define SMALI_LABEL_MAX 32

typedef struct {
    u4   off;
    char name[SMALI_LABEL_MAX];
} smali_label;

typedef struct {
    smali_label *items;
    int          count;
    int          cap;
} smali_label_table;

static smali_label_table* smali_labels_new(void)
{
    smali_label_table *t = x_alloc(sizeof(smali_label_table));
    memset(t, 0, sizeof(*t));
    return t;
}

static const char* smali_labels_find(smali_label_table *t, u4 off)
{
    for (int i = 0; i < t->count; ++i) {
        if (t->items[i].off == off)
            return t->items[i].name;
    }
    return NULL;
}

static const char* smali_labels_intern(smali_label_table *t, u4 off,
                                       const char *prefix, int *counter)
{
    const char *existing = smali_labels_find(t, off);
    if (existing != NULL)
        return existing;

    if (t->count == t->cap) {
        int cap = t->cap == 0 ? 16 : t->cap * 2;
        smali_label *items = x_alloc(sizeof(smali_label) * cap);
        memset(items, 0, sizeof(smali_label) * cap);
        if (t->items != NULL)
            memcpy(items, t->items, sizeof(smali_label) * t->count);
        t->items = items;
        t->cap = cap;
    }

    smali_label *label = &t->items[t->count++];
    label->off = off;
    snprintf(label->name, SMALI_LABEL_MAX, "%s_%d", prefix, (*counter)++);
    return label->name;
}

typedef struct {
    u4  off;         // where the payload sits, in code units
    u4  base;        // the switch / fill-array-data instruction that reaches it
    u2  ident;       // 0x0100 packed, 0x0200 sparse, 0x0300 array-data
    u2  size;        // element count
    u4  first_key;   // packed-switch only
    u2  elem_width;  // fill-array-data only
} smali_payload;

typedef struct {
    smali_payload *items;
    int            count;
    int            cap;
} smali_payload_table;

static smali_payload_table* smali_payloads_new(void)
{
    smali_payload_table *t = x_alloc(sizeof(smali_payload_table));
    memset(t, 0, sizeof(*t));
    return t;
}

static smali_payload* smali_payloads_add(smali_payload_table *t, u4 off)
{
    if (t->count == t->cap) {
        int cap = t->cap == 0 ? 8 : t->cap * 2;
        smali_payload *items = x_alloc(sizeof(smali_payload) * cap);
        memset(items, 0, sizeof(smali_payload) * cap);
        if (t->items != NULL)
            memcpy(items, t->items, sizeof(smali_payload) * t->count);
        t->items = items;
        t->cap = cap;
    }
    smali_payload *p = &t->items[t->count++];
    memset(p, 0, sizeof(*p));
    p->off = off;
    return p;
}

static smali_payload* smali_payloads_find(smali_payload_table *t, u4 off)
{
    for (int i = 0; i < t->count; ++i) {
        if (t->items[i].off == off)
            return &t->items[i];
    }
    return NULL;
}

typedef struct {
    smali_label_table   *labels;
    smali_payload_table *payloads;
    int n_goto, n_cond;
    int n_pswitch_data, n_pswitch;
    int n_sswitch_data, n_sswitch;
    int n_array;
    int n_try_start, n_try_end, n_catch, n_catchall;
} smali_scan;

static bool smali_is_payload(const dex_code_item *code, int i)
{
    if ((code->insns[i] & 0xFF) != DEX_INS_NOP)
        return false;
    u2 unit = code->insns[i];
    return unit == 0x0100 || unit == 0x0200 || unit == 0x0300;
}

static int smali_payload_len(const dex_code_item *code, int i)
{
    u2 unit = code->insns[i];
    if (unit == 0x0100) {
        u2 size = code->insns[i + 1];
        return 4 + size * 2;
    }
    if (unit == 0x0200) {
        u2 size = code->insns[i + 1];
        return 2 + size * 4;
    }
    u2 element_width = code->insns[i + 1];
    u4 size = smali_u32_at(code, i + 2);
    return 4 + (int) ((size * element_width + 1) / 2);
}

static void smali_scan_payload(smali_scan *scan, const dex_code_item *code,
                               int i, int payload_off, u4 base)
{
    if (payload_off < 0 || payload_off >= (int) code->insns_size)
        return;
    if (!smali_is_payload(code, payload_off))
        return;
    if (smali_payloads_find(scan->payloads, (u4) payload_off) != NULL)
        return;

    smali_payload *p = smali_payloads_add(scan->payloads, (u4) payload_off);
    p->base = base;
    p->ident = code->insns[payload_off];
    p->size = code->insns[payload_off + 1];

    if (p->ident == 0x0100) {
        p->first_key = smali_u32_at(code, payload_off + 2);
        smali_labels_intern(scan->labels, (u4) payload_off,
                            "pswitch_data", &scan->n_pswitch_data);
        for (int j = 0; j < p->size; ++j) {
            u4 rel = smali_u32_at(code, payload_off + 4 + j * 2);
            smali_labels_intern(scan->labels, base + (s4) rel,
                                "pswitch", &scan->n_pswitch);
        }
    }
    else if (p->ident == 0x0200) {
        smali_labels_intern(scan->labels, (u4) payload_off,
                            "sswitch_data", &scan->n_sswitch_data);
        for (int j = 0; j < p->size; ++j) {
            u4 rel = smali_u32_at(code, payload_off + 2 + p->size * 2 + j * 2);
            smali_labels_intern(scan->labels, base + (s4) rel,
                                "sswitch", &scan->n_sswitch);
        }
    }
    else {
        p->elem_width = code->insns[payload_off + 1];
        smali_labels_intern(scan->labels, (u4) payload_off,
                            "array", &scan->n_array);
    }
}

/* The instruction a branch lands on, in code units. */
static u4 smali_branch_target(u1 op, const dex_code_item *code, int i)
{
    switch (op) {
        case DEX_INS_GOTO:
            return (u4) (i + (s1) (code->insns[i] >> 8));
        case DEX_INS_GOTO_16:
        case DEX_INS_IF_EQ: case DEX_INS_IF_NE:
        case DEX_INS_IF_LT: case DEX_INS_IF_GE:
        case DEX_INS_IF_GT: case DEX_INS_IF_LE:
        case DEX_INS_IF_EQZ: case DEX_INS_IF_NEZ:
        case DEX_INS_IF_LTZ: case DEX_INS_IF_GEZ:
        case DEX_INS_IF_GTZ: case DEX_INS_IF_LEZ:
            return (u4) (i + (s2) code->insns[i + 1]);
        case DEX_INS_GOTO_32:
            return (u4) (i + (s4) smali_u32_at(code, i + 1));
        default:
            return (u4) i;
    }
}

static int smali_payload_target(const dex_code_item *code, int i)
{
    return i + (s4) smali_u32_at(code, i + 1);
}

static void smali_scan_tries(smali_scan *scan, const dex_code_item *code)
{
    for (int i = 0; i < (int) code->tries_size; ++i) {
        dex_try_item *t = &code->tries[i];
        smali_labels_intern(scan->labels, t->start_addr,
                            "try_start", &scan->n_try_start);
        smali_labels_intern(scan->labels, t->start_addr + t->insn_count,
                            "try_end", &scan->n_try_end);
    }

    if (code->handlers == NULL)
        return;

    for (int i = 0; i < (int) code->handlers->size; ++i) {
        encoded_catch_handler *h = &code->handlers->list[i];
        int typed = h->size < 0 ? -h->size : h->size;
        for (int j = 0; h->handlers != NULL && j < typed; ++j) {
            smali_labels_intern(scan->labels, h->handlers[j].addr,
                                "catch", &scan->n_catch);
        }
        if (h->size <= 0 && h->catch_all_addr != 0) {
            smali_labels_intern(scan->labels, h->catch_all_addr,
                                "catchall", &scan->n_catchall);
        }
    }
}

static void smali_scan_code(const dex_code_item *code, smali_scan *scan)
{
    int i = 0;
    while (i < (int) code->insns_size) {
        u2 unit = code->insns[i];
        u1 op = unit & 0xFF;

        if (smali_is_payload(code, i)) {
            i += smali_payload_len(code, i);
            continue;
        }

        switch (op) {
            case DEX_INS_GOTO:
            case DEX_INS_GOTO_16:
            case DEX_INS_GOTO_32:
                smali_labels_intern(scan->labels, smali_branch_target(op, code, i),
                                    "goto", &scan->n_goto);
                break;

            case DEX_INS_IF_EQ: case DEX_INS_IF_NE:
            case DEX_INS_IF_LT: case DEX_INS_IF_GE:
            case DEX_INS_IF_GT: case DEX_INS_IF_LE:
            case DEX_INS_IF_EQZ: case DEX_INS_IF_NEZ:
            case DEX_INS_IF_LTZ: case DEX_INS_IF_GEZ:
            case DEX_INS_IF_GTZ: case DEX_INS_IF_LEZ:
                smali_labels_intern(scan->labels, smali_branch_target(op, code, i),
                                    "cond", &scan->n_cond);
                break;

            case DEX_INS_PACKED_SWITCH:
            case DEX_INS_SPARSE_SWITCH:
            case DEX_INS_FILL_ARRAY_DATA:
                smali_scan_payload(scan, code, i, smali_payload_target(code, i),
                                   (u4) i);
                break;

            default:
                break;
        }

        int len = dex_opcode_len(op);
        if (len <= 0)
            len = 1;
        i += len;
    }

    smali_scan_tries(scan, code);
}

static void smali_write_operands(FILE *stream, jd_meta_dex *dex,
                                 const dex_code_item *code, int i,
                                 smali_label_table *labels)
{
    u2 unit = code->insns[i];
    u1 op = unit & 0xFF;
    dex_instruction_format fmt = dex_opcode_fmt(op);
    smali_ref_kind ref = smali_ref_of(op);

    switch (fmt) {
        case kFmt10x:                       // return-void, nop
            break;
        case kFmt12x:                       // vA, vB
            fprintf(stream, " v%d, v%d", (unit >> 8) & 0x0F, (unit >> 12) & 0x0F);
            break;

        case kFmt11n: {                     // vA, #+B  (const/4)
            s4 lit = (s4) (unit >> 12);
            if (lit > 7)
                lit -= 16;
            fprintf(stream, " v%d, ", (unit >> 8) & 0x0F);
            smali_int(stream, lit);
            break;
        }

        case kFmt11x:                       // vAA
            fprintf(stream, " v%d", unit >> 8);
            break;

        case kFmt10t:                       // goto +AA
        case kFmt20t:                       // goto/16 +AAAA
        case kFmt30t: {                     // goto/32 +AAAAAAAA
            const char *label = smali_labels_find(labels, smali_branch_target(op, code, i));
            fprintf(stream, " :%s", label ? label : "unknown");
            break;
        }

        case kFmt22x:                       // vAA, vBBBB
            fprintf(stream, " v%d, v%d", unit >> 8, code->insns[i + 1]);
            break;

        case kFmt21t: {                     // if-*z vAA, +BBBB
            const char *label = smali_labels_find(labels, smali_branch_target(op, code, i));
            fprintf(stream, " v%d, :%s", unit >> 8, label ? label : "unknown");
            break;
        }

        case kFmt21s: {                     // const/16 vAA, #+BBBB
            fprintf(stream, " v%d, ", unit >> 8);
            smali_int(stream, (s2) code->insns[i + 1]);
            break;
        }

        case kFmt21h: {                     // const/high16, const-wide/high16
            fprintf(stream, " v%d, ", unit >> 8);
            if (op == DEX_INS_CONST_WIDE_HIGH16)
                smali_long(stream, (s8) ((u8) code->insns[i + 1] << 48));
            else
                smali_int(stream, (s4) ((u4) code->insns[i + 1] << 16));
            break;
        }

        case kFmt21c:                       // vAA, ref@BBBB
            fprintf(stream, " v%d, ", unit >> 8);
            smali_write_ref(stream, dex, ref, code->insns[i + 1]);
            break;

        case kFmt23x:                       // vAA, vBB, vCC
            fprintf(stream, " v%d, v%d, v%d",
                    unit >> 8,
                    code->insns[i + 1] & 0xFF,
                    code->insns[i + 1] >> 8);
            break;

        case kFmt22b: {                     // binop/lit8 vAA, vBB, #+CC
            fprintf(stream, " v%d, v%d, ", unit >> 8, code->insns[i + 1] & 0xFF);
            smali_int(stream, (s1) (code->insns[i + 1] >> 8));
            break;
        }

        case kFmt22t: {                     // if-* vA, vB, +CCCC
            const char *label = smali_labels_find(labels, smali_branch_target(op, code, i));
            fprintf(stream, " v%d, v%d, :%s",
                    (unit >> 8) & 0x0F, (unit >> 12) & 0x0F,
                    label ? label : "unknown");
            break;
        }

        case kFmt22s: {                     // binop/lit16 vA, vB, #+CCCC
            fprintf(stream, " v%d, v%d, ",
                    (unit >> 8) & 0x0F, (unit >> 12) & 0x0F);
            smali_int(stream, (s2) code->insns[i + 1]);
            break;
        }

        case kFmt22c:                       // vA, vB, ref@CCCC
            fprintf(stream, " v%d, v%d, ",
                    (unit >> 8) & 0x0F, (unit >> 12) & 0x0F);
            smali_write_ref(stream, dex, ref, code->insns[i + 1]);
            break;

        case kFmt32x:                       // vAAAA, vBBBB
            fprintf(stream, " v%d, v%d", code->insns[i + 1], code->insns[i + 2]);
            break;

        case kFmt31i: {                     // const vAA, #+BBBBBBBB
            fprintf(stream, " v%d, ", unit >> 8);
            smali_int(stream, (s4) smali_u32_at(code, i + 1));
            break;
        }

        case kFmt31t: {                     // switch / fill-array-data vAA, +BBBBBBBB
            const char *label = smali_labels_find(labels, (u4) smali_payload_target(code, i));
            fprintf(stream, " v%d, :%s", unit >> 8, label ? label : "unknown");
            break;
        }

        case kFmt31c: {                     // const-string/jumbo vAA, string@BBBBBBBB
            fprintf(stream, " v%d, ", unit >> 8);
            smali_write_ref(stream, dex, ref, smali_u32_at(code, i + 1));
            break;
        }

        case kFmt35c: {                     // invoke-* {vC..vG}, ref@BBBB
            u2 count = unit >> 12;
            u2 regs = code->insns[i + 2];
            int reg[5];
            reg[0] = regs & 0x0F;
            reg[1] = (regs >> 4) & 0x0F;
            reg[2] = (regs >> 8) & 0x0F;
            reg[3] = (regs >> 12) & 0x0F;
            reg[4] = (unit >> 8) & 0x0F;

            fputs(" { ", stream);
            for (int k = 0; k < count && k < 5; ++k)
                fprintf(stream, "%sv%d", k == 0 ? "" : ", ", reg[k]);
            fputs(" }, ", stream);
            smali_write_ref(stream, dex, ref, code->insns[i + 1]);
            break;
        }

        case kFmt3rc: {                     // invoke-*/range {vCCCC .. vNNNN}, ref@BBBB
            u2 count = unit >> 8;
            u2 first = code->insns[i + 2];

            if (count == 0)
                fputs(" { }, ", stream);
            else
                fprintf(stream, " { v%d .. v%d }, ", first, first + count - 1);
            smali_write_ref(stream, dex, ref, code->insns[i + 1]);
            break;
        }

        case kFmt51l: {                     // const-wide vAA, #+BBBB...
            fprintf(stream, " v%d, ", unit >> 8);
            smali_long(stream, (s8) smali_u64_at(code, i + 1));
            break;
        }

        case kFmt45cc: {                    // invoke-polymorphic {vC..vG}, meth@BBBB, proto@HHHH
            u2 count = unit >> 12;
            u2 regs = code->insns[i + 2];
            int reg[5];
            reg[0] = regs & 0x0F;
            reg[1] = (regs >> 4) & 0x0F;
            reg[2] = (regs >> 8) & 0x0F;
            reg[3] = (regs >> 12) & 0x0F;
            reg[4] = (unit >> 8) & 0x0F;

            fputs(" { ", stream);
            for (int k = 0; k < count && k < 5; ++k)
                fprintf(stream, "%sv%d", k == 0 ? "" : ", ", reg[k]);
            fputs(" }, ", stream);
            smali_method_ref(stream, dex, code->insns[i + 1]);
            fputs(", ", stream);
            smali_proto(stream, dex, code->insns[i + 3]);
            break;
        }

        case kFmt4rcc: {                    // invoke-polymorphic/range {vCCCC .. vNNNN}, meth@BBBB, proto@HHHH
            u2 count = unit >> 8;
            u2 first = code->insns[i + 2];

            if (count == 0)
                fputs(" { }, ", stream);
            else
                fprintf(stream, " { v%d .. v%d }, ", first, first + count - 1);
            smali_method_ref(stream, dex, code->insns[i + 1]);
            fputs(", ", stream);
            smali_proto(stream, dex, code->insns[i + 3]);
            break;
        }

        case kFmt00x:
        case kFmt20bc:
        case kFmt22cs:
        case kFmt35ms:
        case kFmt3rms:
        case kFmt35mi:
        case kFmt3rmi:
        default:
            fputs("  # undecoded instruction format", stream);
            break;
    }
}

static void smali_write_instructions(jd_meta_dex *dex, const dex_code_item *code,
                                     smali_label_table *labels, FILE *stream)
{
    int i = 0;
    while (i < (int) code->insns_size) {
        u2 unit = code->insns[i];
        u1 op = unit & 0xFF;

        if (smali_is_payload(code, i)) {
            i += smali_payload_len(code, i);
            continue;
        }

        const char *label = smali_labels_find(labels, (u4) i);
        if (label != NULL)
            fprintf(stream, "  :%s\n", label);

        fprintf(stream, "    %s", dex_opcode_name(op));
        smali_write_operands(stream, dex, code, i, labels);
        fputc('\n', stream);

        int len = dex_opcode_len(op);
        if (len <= 0)
            len = 1;
        i += len;
    }

    const char *end = smali_labels_find(labels, code->insns_size);
    if (end != NULL)
        fprintf(stream, "  :%s\n", end);
}

static void smali_write_payloads(jd_meta_dex *dex, const dex_code_item *code,
                                 smali_scan *scan, FILE *stream)
{
    smali_payload_table *table = scan->payloads;

    for (int i = 0; i < table->count; ++i) {
        smali_payload *p = &table->items[i];
        const char *label = smali_labels_find(scan->labels, p->off);
        if (label == NULL)
            continue;

        fprintf(stream, "\n  :%s\n", label);

        if (p->ident == 0x0100) {
            fputs("  .packed-switch ", stream);
            smali_int(stream, (s4) p->first_key);
            fputc('\n', stream);
            for (int j = 0; j < p->size; ++j) {
                u4 rel = smali_u32_at(code, p->off + 4 + j * 2);
                const char *target = smali_labels_find(scan->labels,
                                                       p->base + (s4) rel);
                fprintf(stream, "    :%s\n", target ? target : "unknown");
            }
            fprintf(stream, "  .end packed-switch\n");
        }
        else if (p->ident == 0x0200) {
            fprintf(stream, "  .sparse-switch\n");
            for (int j = 0; j < p->size; ++j) {
                s4 key = (s4) smali_u32_at(code, p->off + 2 + j * 2);
                u4 rel = smali_u32_at(code, p->off + 2 + p->size * 2 + j * 2);
                const char *target = smali_labels_find(scan->labels,
                                                       p->base + (s4) rel);
                fputs("    ", stream);
                smali_int(stream, key);
                fprintf(stream, " -> :%s\n", target ? target : "unknown");
            }
            fprintf(stream, "  .end sparse-switch\n");
        }
        else {
            u2 width = p->elem_width;
            const u1 *data = (const u1 *) (code->insns + p->off + 4);
            u4 size = smali_u32_at(code, p->off + 2);

            fprintf(stream, "  .array-data %d\n", width);
            for (u4 j = 0; j < size; ++j) {
                u8 raw = 0;
                for (int k = 0; k < width && k < 8; ++k)
                    raw |= (u8) data[j * width + k] << (8 * k);

                s8 value = 0;
                switch (width) {
                    case 1: value = (s8) (s1) (u1) raw; break;
                    case 2: value = (s8) (s2) (u2) raw; break;
                    case 4: value = (s8) (s4) (u4) raw; break;
                    default: value = (s8) raw; break;
                }
                fputs("    ", stream);
                if (width == 8)
                    smali_long(stream, value);
                else
                    smali_int(stream, (s4) value);
                fputc('\n', stream);
            }
            fprintf(stream, "  .end array-data\n");
        }
    }
}

static void smali_write_catches(jd_meta_dex *dex, const dex_code_item *code,
                                smali_label_table *labels, FILE *stream)
{
    for (int i = 0; i < (int) code->tries_size; ++i) {
        dex_try_item *t = &code->tries[i];
        const char *start = smali_labels_find(labels, t->start_addr);
        const char *end = smali_labels_find(labels, t->start_addr + t->insn_count);
        if (start == NULL || end == NULL)
            continue;

        encoded_catch_handler *h = NULL;
        if (code->handlers != NULL) {
            for (int j = 0; j < (int) code->handlers->size; ++j) {
                if (code->handlers->list[j].handler_off == t->handler_off) {
                    h = &code->handlers->list[j];
                    break;
                }
            }
        }
        if (h == NULL)
            continue;

        int typed = h->size < 0 ? -h->size : h->size;
        for (int j = 0; j < typed; ++j) {
            if (h->handlers == NULL)
                break;
            const char *target = smali_labels_find(labels, h->handlers[j].addr);
            if (target == NULL)
                continue;
            fprintf(stream, "  .catch %s { :%s .. :%s } :%s\n",
                    dex_str_of_type_id(dex, h->handlers[j].type_idx),
                    start, end, target);
        }

        if (h->size <= 0 && h->catch_all_addr != 0) {
            const char *target = smali_labels_find(labels, h->catch_all_addr);
            if (target != NULL)
                fprintf(stream, "  .catchall { :%s .. :%s } :%s\n",
                        start, end, target);
        }
    }
}

static void smali_class_flags(FILE *stream, string flags)
{
    int n = (int) strlen(flags);
    while (n > 0 && flags[n - 1] == ' ')
        --n;

    static const char *kKind[] = {"class", "interface", "enum"};
    for (int i = 0; i < 3; ++i) {
        int k = (int) strlen(kKind[i]);
        if (n >= k && strncmp(flags + n - k, kKind[i], k) == 0) {
            n -= k;
            while (n > 0 && flags[n - 1] == ' ')
                --n;
            break;
        }
    }
    fprintf(stream, "%.*s", n, flags);
}

static void smali_flags(FILE *stream, string flags)
{
    int n = (int) strlen(flags);
    while (n > 0 && flags[n - 1] == ' ')
        --n;
    fprintf(stream, "%.*s", n, flags);
}

static void smali_method_defination(jd_meta_dex *dex,
                                    encoded_method *m,
                                    dex_code_item *code,
                                    int type,
                                    FILE *stream)
{
    (void) type;
    (void) code;
    dex_method_id *method_id = &dex->method_ids[m->method_id];
    string method_name = dex_str_of_idx(dex, method_id->name_idx);

    str_list *list = str_list_init();
    dex_method_access_flag_with_flags(m->access_flags, list);
    string access_flags = str_join(list);

    fprintf(_smali_stream(stream), ".method ");
    smali_flags(_smali_stream(stream), access_flags);

    // <init> and <clinit> are marked as constructors, which is how smali
    // tells them apart from a method that merely happens to be called that -
    // and how it knows to set the constructor bit back on the dex.
    if (strcmp(method_name, "<init>") == 0 ||
        strcmp(method_name, "<clinit>") == 0)
        fprintf(_smali_stream(stream), " constructor");

    fprintf(_smali_stream(stream), " %s", method_name);
    smali_proto(_smali_stream(stream), dex, method_id->proto_idx);
    fputc('\n', _smali_stream(stream));
}

static void smali_write_method(jd_meta_dex *dex,
                               encoded_method *m,
                               dex_code_item *code,
                               int type,
                               FILE *stream)
{
    smali_method_defination(dex, m, code, type, stream);

    if (code != NULL) {
        smali_scan scan;
        memset(&scan, 0, sizeof(scan));
        scan.labels = smali_labels_new();
        scan.payloads = smali_payloads_new();

        smali_scan_code(code, &scan);
        smali_write_catches(dex, code, scan.labels, _smali_stream(stream));
        fprintf(_smali_stream(stream), "  .registers %d\n", code->registers_size);
        smali_write_instructions(dex, code, scan.labels, _smali_stream(stream));
        smali_write_payloads(dex, code, &scan, _smali_stream(stream));
    }
    fprintf(_smali_stream(stream), ".end method\n\n");
}

static void smali_write_class_fields(jd_meta_dex *dex,
                                     dex_class *cf,
                                     FILE *stream)
{
    dex_class_data_item *item = cf->class_data;
    if (item == NULL)
        return;

    for (int i = 0; i < (int) item->static_fields_size; ++i) {
        encoded_field *efield = &item->static_fields[i];
        str_list *list = str_list_init();
        dex_field_access_flag_with_flags(efield->access_flags, list);
        string flags = str_join(list);
        fprintf(stream, "\n.field ");
        smali_flags(stream, flags);
        fprintf(stream, " %s:%s", dex_field_name(dex, efield), dex_field_desc(dex, efield));
        fputc('\n', stream);
    }

    for (int i = 0; i < (int) item->instance_fields_size; ++i) {
        encoded_field *efield = &item->instance_fields[i];
        str_list *list = str_list_init();
        dex_field_access_flag_with_flags(efield->access_flags, list);
        string flags = str_join(list);
        fprintf(stream, "\n.field ");
        smali_flags(stream, flags);
        fprintf(stream, " %s:%s", dex_field_name(dex, efield), dex_field_desc(dex, efield));
        fputc('\n', stream);
    }
    fprintf(stream, "\n");
}

static void smali_write_class_def(jd_meta_dex *dex,
                                  dex_class *cf,
                                  FILE *stream)
{
    string class_name = dex_str_of_type_id(dex, cf->class_idx);
    string super_name = dex_str_of_type_id(dex, cf->superclass_idx);

    str_list *list = str_list_init();
    dex_class_access_flag_with_flags(cf->access_flags, list);
    string cf_access_flags = str_join(list);

    fprintf(stream, ".class ");
    smali_class_flags(stream, cf_access_flags);
    fprintf(stream, " %s\n", class_name);
    fprintf(stream, ".super %s\n", super_name);

    const u4 kNoIndex = 0xFFFFFFFF;
    if (cf->source_file_idx != kNoIndex &&
        cf->source_file_idx < dex->header->string_ids_size) {
        fprintf(stream, ".source ");
        smali_string(stream, dex_str_of_idx(dex, cf->source_file_idx));
        fputc('\n', stream);
    }

    if (cf->interfaces != NULL) {
        for (int i = 0; i < cf->interfaces->size; ++i) {
            dex_type_item *type_item = &cf->interfaces->list[i];
            fprintf(stream, ".implements %s\n",
                    dex_str_of_type_id(dex, type_item->type_idx));
        }
    }

    smali_write_class_fields(dex, cf, stream);
}

void dex_class_def_to_smali(jd_meta_dex *dex, dex_class_def *cf, FILE *stream)
{
    dex_class_data_item *class_data = cf->class_data;
    smali_write_class_def(dex, cf, stream);
    if (class_data == NULL)
        return;

    for (int j = 0; j < (int) class_data->direct_methods_size; ++j) {
        encoded_method *m = &class_data->direct_methods[j];
        smali_write_method(dex, m, m->code, 0, stream);
    }

    for (int j = 0; j < (int) class_data->virtual_methods_size; ++j) {
        encoded_method *m = &class_data->virtual_methods[j];
        smali_write_method(dex, m, m->code, 1, stream);
    }
}

void dex_to_smali(string path)
{
    mem_init_pool();
    jd_meta_dex *meta = parse_dex_file(path);
    if (meta == NULL) {
        mem_free_pool();
        return;
    }

    dex_header *header = meta->header;
    for (int i = 0; i < (int) header->class_defs_size; ++i)
        dex_class_def_to_smali(meta, &meta->class_defs[i], NULL);

    mem_free_pool();
}

static int smali_write_class_by_desc(jd_meta_dex *meta, string class_desc, string out_dir)
{
    if (meta == NULL || meta->header == NULL)
        return -1;

    for (int i = 0; i < (int) meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        string desc = dex_str_of_type_id(meta, cf->class_idx);
        if (desc == NULL || strcmp(desc, class_desc) != 0)
            continue;

        string full = class_full_name(desc);
        string pname = class_package_name_of(full);
        if (pname == NULL)
            pname = (string) g_str_default;
        string dir = str_create("%s/%s", out_dir, pname);
        mkdir_p(dir);
        string path = str_create("%s/%s.smali", dir,
                                 class_simple_name_without_primitive(full));

        FILE *stream = fopen(path, "wb");
        if (stream == NULL) {
            output_open_failed(path);
            return -1;
        }
        dex_class_def_to_smali(meta, cf, stream);
        output_close(stream, path);
        return 0;
    }

    return -1;  // not in this dex
}

static int smali_one_from_buffer(char *buf, size_t size, string class_desc, string out_dir)
{
    jd_meta_dex *meta = parse_dex_from_buffer(buf, size);
    int rc = -1;
    if (meta != NULL) {
        rc = smali_write_class_by_desc(meta, class_desc, out_dir);
        if (meta->pool != NULL)
            mem_pool_free(meta->pool);
    }
    free(buf);
    return rc;
}

int dex_smali_class_to_dir(string source_path, string class_desc, string out_dir)
{
    if (source_path == NULL || class_desc == NULL || out_dir == NULL)
        return -1;

    mem_init_pool();
    int rc = -1;

    if (str_end_with(source_path, ".dex")) {
        FILE *f = fopen(source_path, "rb");
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = malloc(size > 0 ? (size_t) size : 1);
            if (buf != NULL && size > 0 && fread(buf, 1, (size_t) size, f) == (size_t) size)
                rc = smali_one_from_buffer(buf, (size_t) size, class_desc, out_dir);
            else
                free(buf);
            fclose(f);
        }
    }
    else {
        struct zip_t *zip = zip_open(source_path, 0, 'r');
        if (zip != NULL) {
            int total = zip_entries_total(zip);
            for (int i = 0; i < total && rc != 0; ++i) {
                zip_entry_openbyindex(zip, i);
                string name = (string) zip_entry_name(zip);
                if (name == NULL || strchr(name, '/') != NULL ||
                    !str_end_with(name, ".dex")) {
                    zip_entry_close(zip);
                    continue;
                }
                size_t size = zip_entry_size(zip);
                char *buf = malloc(size > 0 ? size : 1);
                if (buf != NULL && size > 0) {
                    zip_entry_noallocread(zip, buf, size);
                    rc = smali_one_from_buffer(buf, size, class_desc, out_dir);
                }
                else {
                    free(buf);
                }
                zip_entry_close(zip);
            }
            zip_close(zip);
        }
    }

    mem_free_pool();
    return rc;
}

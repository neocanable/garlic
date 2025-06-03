#include "jvm/jvm_method.h"
#include "decompiler/control_flow.h"
#include "decompiler/method.h"
#include "decompiler/descriptor.h"
#include "decompiler/instruction.h"
#include "jvm_ins.h"
#include "decompiler/exception.h"
#include "jvm_simulator.h"
#include "decompiler/stack.h"
#include "jvm_optimizer.h"
#include "jvm/jvm_exception.h"
#include "jvm/jvm_annotation.h"
#include "jvm/jvm_descriptor.h"

void jvm_method_access_flags(jd_method *m, str_list *list) {
    if (method_has_flag(m, METHOD_ACC_FINAL))
        str_concat(list, ("final"));

    if (method_has_flag(m, METHOD_ACC_PUBLIC)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("public"));
    }

    if (method_has_flag(m, METHOD_ACC_PRIVATE)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("private"));
    }

    if (method_has_flag(m, METHOD_ACC_PROTECTED)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("protected"));
    }

    if (method_has_flag(m, METHOD_ACC_STATIC)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("static"));
    }

    if (method_has_flag(m, METHOD_ACC_NATIVE)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("native"));
    }

    if (method_has_flag(m, METHOD_ACC_ABSTRACT)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("abstract"));
    }

    if (method_has_flag(m, METHOD_ACC_SYNTHETIC)) {
        if (list->len > 0)
            str_concat(list, (" "));
        str_concat(list, ("/* synthetic */"));
    }

    if (!method_is_init(m))
        if (list->count > 0)
            str_concat(list, (" "));
}

jd_val* jvm_method_parameter_val(jd_method *m, int index)
{
    if (m->enter == NULL)
        return NULL;
    int i = method_is_member(m) ? index + 1 : index;
    // i += method_is_enum_constructor(m) ? 2 : 0;
    return m->enter->local_vars[i];
}

static void jvm_fill_multi_array_stack_action(jd_ins *ins)
{
//    u1 *param = ins->param;
    u1 dimensions = ins->param[2];
    ins->pushed_cnt = 1;
    ins->popped_cnt = dimensions;
}

static void jvm_fill_invoke_stack_action(jd_ins *ins)
{
    jd_method *m = ins->method;
    jclass_file *jc = m->meta;
    jsource_file *jf = jc->jfile;
    u2 name_and_type_index = 0;
    jcp_info *info = NULL;
    if (jvm_ins_is_invokedynamic(ins)) {
        // invokedynamic
        u1 *parameters = ins->param;
        uint16_t method_index = parameters[0] << 8 | parameters[1];
        info = &jc->constant_pool[method_index - 1];
        jconst_invoke_dynamic *invoke_dynamic = info->info->invoke_dynamic;
        name_and_type_index = invoke_dynamic->name_and_type_index;
    }
    else {
        u1 *param = ins->param;
        u2 index = be16toh(param[0] << 8 | param[1]);
        info = pool_item(m->meta, index);
        jconst_methodref *methodref = info->info->methodref;
        name_and_type_index = methodref->name_and_type_index;
    }

    jcp_info *nt_info = pool_item(m->meta, name_and_type_index);
    jconst_name_and_type *nt = nt_info->info->name_and_type;
    u2 desc_index = nt->descriptor_index;
    jd_descriptor *descriptor = jvm_descriptor(jf, desc_index);
    ins->popped_cnt = descriptor->list->size;

    if (jvm_ins_is_invokespecial(ins) ||
        jvm_ins_is_invokeinterface(ins) ||
        jvm_ins_is_invokevirtual(ins))
        ins->popped_cnt += 1;

    ins->pushed_cnt = STR_EQL(descriptor->str_return, "V") ? 0 : 1;
}

static void init_tableswitch_jumps(jd_ins *ins) {
    u1 p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12;
    uint32_t padding = jvm_switch_padding(ins->offset);
    p1  = ins->param[padding + 0];
    p2  = ins->param[padding + 1];
    p3  = ins->param[padding + 2];
    p4  = ins->param[padding + 3];
    p5  = ins->param[padding + 4];
    p6  = ins->param[padding + 5];
    p7  = ins->param[padding + 6];
    p8  = ins->param[padding + 7];
    p9  = ins->param[padding + 8];
    p10 = ins->param[padding + 9];
    p11 = ins->param[padding + 10];
    p12 = ins->param[padding + 11];

    uint32_t default_offset = be_32(p1, p2, p3, p4) + ins->offset;
    int default_idx = hget_i2i(ins->method->offset2id_map,
                               default_offset);

    jd_ins *default_ins = get_ins(ins->method, default_idx);
    if (!lcontains_obj(ins->targets, default_ins))
        ladd_obj(ins->targets, default_ins);
    if (!lcontains_obj(ins->jumps, default_ins))
        ladd_obj(ins->jumps, default_ins);

    uint32_t low_byte  = be_32(p5, p6, p7, p8);
    uint32_t high_byte = be_32(p9, p10, p11, p12);

    uint32_t jump_size = high_byte - low_byte + 1;
    uint32_t start_jump = padding + 12;
    for (uint32_t k = 0; k < jump_size; k++) {
        p1 = ins->param[start_jump + k * 4 + 0];
        p2 = ins->param[start_jump + k * 4 + 1];
        p3 = ins->param[start_jump + k * 4 + 2];
        p4 = ins->param[start_jump + k * 4 + 3];
        uint32_t offset      = be_32(p1, p2, p3, p4);
        uint32_t jump_offset = offset + ins->offset;

        jd_ins *jump_ins = ins_of_offset(ins->method, jump_offset);
        ladd_obj_no_dup(ins->targets, jump_ins);
        ladd_obj_no_dup(ins->jumps, jump_ins);
    }
}

static void init_lookupswitch_jumps(jd_ins *ins)
{
    u1 p1, p2, p3, p4, p5, p6, p7, p8;
    uint32_t padding = jvm_switch_padding(ins->offset);
    hashmap *map = ins->method->offset2id_map;
    p1 = ins->param[padding + 0];
    p2 = ins->param[padding + 1];
    p3 = ins->param[padding + 2];
    p4 = ins->param[padding + 3];
    p5 = ins->param[padding + 4];
    p6 = ins->param[padding + 5];
    p7 = ins->param[padding + 6];
    p8 = ins->param[padding + 7];
    uint32_t default_offset = be_32(p1, p2, p3, p4) + ins->offset;

    int default_idx = hget_i2i(map, default_offset);

    jd_ins *default_ins = get_ins(ins->method, default_idx);
    ladd_obj_no_dup(ins->targets, default_ins);
    ladd_obj_no_dup(ins->jumps, default_ins);

    uint32_t npair = be_32(p5, p6, p7, p8);
    uint32_t start_npair = padding + 8;
    for (uint32_t k = 0; k < npair ;++k) {
        p5                   = ins->param[start_npair + k * 8 + 4];
        p6                   = ins->param[start_npair + k * 8 + 5];
        p7                   = ins->param[start_npair + k * 8 + 6];
        p8                   = ins->param[start_npair + k * 8 + 7];
        uint32_t val         = be_32(p5, p6, p7, p8);
        uint32_t jump_offset = val + ins->offset;
        jd_ins *jump_ins = ins_of_offset(ins->method, jump_offset);
        ladd_obj_no_dup(ins->targets, jump_ins);
        ladd_obj_no_dup(ins->jumps, jump_ins);
    }
}

static void init_conditional_jumps(jd_ins *ins)
{
    // if cmp and jump
    u1 p1                = ins->param[0];
    u1 p2                = ins->param[1];
    uint16_t jump_offset = be_16(p1, p2) + ins->offset;
    uint16_t next_offset = ins->offset + ins->param_length + 1;

    jd_ins *jump_to_ins = ins_of_offset(ins->method, jump_offset);
    jd_ins *next_ins = ins_of_offset(ins->method, next_offset);
    ladd_obj(ins->targets, jump_to_ins);
    ladd_obj(ins->targets, next_ins);
    ladd_obj(ins->jumps, jump_to_ins);
}

static void init_unconditional_jumps(jd_ins *ins)
{
    // goto_w
    u1 p1, p2, p3, p4;
    int32_t jump_byte = 0;
    if (jvm_ins_is_goto_w(ins)) {
        p1        = ins->param[0];
        p2        = ins->param[1];
        p3        = ins->param[2];
        p4        = ins->param[3];
        jump_byte = (int32_t)(p1 << 24) | (p2 << 16) | (p3 << 8) | p4;
    }
    else { // goto
        p1          = ins->param[0];
        p2          = ins->param[1];
        jump_byte   = (int16_t)(p1 << 8) | p2;
    }

    uint32_t jump_offset = jump_byte + ins->offset;
    int target_id = hget_i2i(ins->method->offset2id_map, jump_offset);
    jd_ins *target_ins = get_ins(ins->method, target_id);
    ladd_obj(ins->targets, target_ins);
    ladd_obj(ins->jumps, target_ins);
}

static void init_default_jumps(jd_ins *ins)
{
    if (jvm_ins_is_return(ins) || jvm_ins_is_athrow(ins)) return;
    uint32_t next_offset = ins->offset + ins->param_length + 1;
    int idx = hget_i2i(ins->method->offset2id_map, next_offset);

    jd_ins *target_ins = get_ins(ins->method, idx);
    ladd_obj(ins->targets, target_ins);
}

uint32_t caculate_param_length(jd_method *m, uint32_t i)
{
    const u1 *code = m->code;
    u1 opcode = code[i];
    u1 p5, p6, p7, p8, p9, p10, p11, p12;
    switch (opcode) {
        case INS_TABLESWITCH: {
            uint32_t padding = jvm_switch_padding(i);
            p5  = code[i + padding + 5];
            p6  = code[i + padding + 6];
            p7  = code[i + padding + 7];
            p8  = code[i + padding + 8];
            p9  = code[i + padding + 9];
            p10 = code[i + padding + 10];
            p11 = code[i + padding + 11];
            p12 = code[i + padding + 12];
            uint32_t low  = be_32(p5, p6, p7, p8);
            uint32_t high = be_32(p9, p10, p11, p12);
            return (high - low + 1) * 4 + padding + 12;
        }
        case INS_LOOKUPSWITCH: {
            uint32_t padding = jvm_switch_padding(i);
            p5 = code[i + padding + 5];
            p6 = code[i + padding + 6];
            p7 = code[i + padding + 7];
            p8 = code[i + padding + 8];
            uint32_t npair = be_32(p5, p6, p7, p8);
            return padding + 8 + npair * 8;
        }
        case INS_WIDE: {
            u1 wcode = code[i + 1];
            // iinc 5, others is 3
            return wcode == INS_IINC ? 5 : 3;
        }
        default: {
            return get_opcode_param_length(m->meta, opcode);
        }
    }
}

static int method_instruction_count(jd_method *m)
{
    int result = 0;
    for (uint32_t i = 0; i < be32toh(m->code_length); ) {
        uint32_t param_length = caculate_param_length(m, i);
        i += param_length + 1;
        result ++;
    }
    return result;
}

static void init_method_instructions(jd_method *m)
{
    jclass_file *jc = m->meta;
    jsource_file *jf = jc->jfile;
    int ins_count = method_instruction_count(m);
    m->instructions = linit_object_with_capacity(ins_count);
    m->offset2id_map = hashmap_init((hcmp_fn) i2i_cmp, ins_count);

    int idx = 0;
    jd_ins *prev_ins = NULL;
    for (uint32_t i = 0; i < be32toh(m->code_length); ) {
        u1 opcode = m->code[i];
        uint32_t param_length = caculate_param_length(m, i);
        jd_ins *ins = make_obj(jd_ins);
        ins->method = m;
        ins->code = opcode;
        ins->name = get_opcode_name(m->meta, opcode);
        ins->fn = jf->ins_fn;
        if (param_length > 0) {
            ins->param = &m->code[i + 1];
        }
        else
            ins->param = NULL;

        if (jvm_ins_is_wide(ins)) {
            switch (ins->param[0]) {
                case INS_ISTORE: // istore
                case INS_LSTORE: // lstore
                case INS_FSTORE: // fstore
                case INS_DSTORE: // dstore
                case INS_ASTORE: { // astore
                    ins->pushed_cnt = 0;
                    ins->popped_cnt = 1;
                    break;
                }
                case INS_ILOAD: // iload
                case INS_LLOAD: // lload
                case INS_FLOAD: // fload
                case INS_DLOAD: // dload
                case INS_ALOAD: { // aload
                    ins->pushed_cnt = 1;
                    ins->popped_cnt = 0;
                    break;
                }
                default: {
                    ins->pushed_cnt = 0;
                    ins->popped_cnt = 0;
                    break;
                }
            }
        }
        else if (jvm_ins_is_invoke(ins)) {
            jvm_fill_invoke_stack_action(ins);
        }
        else if (jvm_ins_is_multianewarray(ins)) {
            jvm_fill_multi_array_stack_action(ins);
        }
        else {
            ins->pushed_cnt = get_opcode_pushed(m->meta, opcode);
            ins->popped_cnt = get_opcode_popped(m->meta, opcode);
        }

        ins->idx = m->instructions->size;
        ins->offset = i;

        ins->targets = linit_object_with_capacity(1);
        ins->jumps = linit_object_with_capacity(2);
        ins->comings = linit_object_with_capacity(1);

        if (jvm_ins_is_store(ins)) {
            ins->defs = bitset_create();
            int slot = jvm_ins_store_slot(ins);
            bitset_set(ins->defs, slot);
        }

        if (jvm_ins_is_load(ins)) {
            ins->uses = bitset_create();
            int slot = jvm_ins_load_slot(ins);
            bitset_set(ins->uses, slot);
        }

        if (prev_ins == NULL) {
            ins->prev = NULL;
        }
        else {
            ins->prev = prev_ins;
            prev_ins->next = ins;
        }
        hset_i2i(m->offset2id_map, ins->offset, ins->idx);
        ladd_obj(m->instructions, ins);
        prev_ins = ins;
        ins->param_length = param_length;
        i += param_length + 1;
        idx++;
    }

}

static void init_method_exception_table(jd_method *m, jmethod *item)
{
    DEBUG_PRINT("===> start extract m exception table\n");
    jattr_code *code_attr = item->code_attribute;
    if (code_attr == NULL)
        return;
    int _length = be16toh(code_attr->exception_table_length);
    m->cfg_exceptions = linit_object();
    hashmap *offset2id_map = m->offset2id_map;
    for (int i = 0; i < _length; ++i) {
        jattr_code_exception_table *eitem = &code_attr->exception_table[i];
        jd_exc *e = make_obj(jd_exc);
        e->idx        = i;
        e->start_pc   = eitem->start_pc;
        e->end_pc     = eitem->end_pc;
        e->handler_pc = eitem->handler_pc;
        e->catch_type = eitem->catch_type;

        e->try_start = be16toh(eitem->start_pc);
        int try_end_offset = be16toh(eitem->end_pc);
        int try_end_idx = hget_i2i(offset2id_map, try_end_offset);
        int prev_try_end_idx = try_end_idx - 1;
        jd_ins *prev_try_end_ins = get_ins(m, prev_try_end_idx);
        e->try_end = prev_try_end_ins->offset;
        e->try_end_idx = prev_try_end_ins->idx;
        e->handler_start = be16toh(eitem->handler_pc);
        e->catch_type_index = be16toh(eitem->catch_type);
        e->try_start_idx = hget_i2i(offset2id_map, e->try_start);
        e->handler_start_idx = hget_i2i(offset2id_map, e->handler_start);

        ladd_obj(m->cfg_exceptions, e);
    }
}

static void init_jvm_instruction_graph(jd_method *m)
{
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_ins *ins = get_ins(m, i);
        u1 opcode = ins->code;

        if (jvm_ins_is_conditional_jump(ins))
            init_conditional_jumps(ins);
        else if (opcode == INS_JSR ||
                 opcode == INS_RET ||
                 opcode == INS_JSR_W) {
            method_mark_unsupport(m);
            fprintf(stderr, "can not support jsr and ret instruction\n");
        }
        else if (opcode == INS_TABLESWITCH)
            init_tableswitch_jumps(ins);
        else if (opcode == INS_LOOKUPSWITCH)
            init_lookupswitch_jumps(ins);
        else if (opcode == INS_GOTO_W || opcode == INS_GOTO)
            init_unconditional_jumps(ins);
        else
            init_default_jumps(ins);

        for (int j = 0; j < ins->jumps->size; ++j) {
            jd_ins *jump_to_ins = lget_obj(ins->jumps, j);
            ladd_obj(jump_to_ins->comings, ins);
        }

    }
}

void jvm_method_init(jclass_file *jc, jd_method *m, jmethod *item)
{
    jsource_file *jf = jc->jfile;
    string name = pool_str(jc, item->name_index);
    m->meta = jc;
    m->jfile = jc->jfile;
    m->name = name;
    u2 desc_index = item->descriptor_index;
    jd_descriptor *descriptor = jvm_descriptor(jf, desc_index);
    m->desc = descriptor;
    m->meta_method = item;
    m->type = JD_TYPE_JVM;
    m->fn = jf->method_fn;

    jattr_code *code_attribute = item->code_attribute;
    // the abstract and native m doesn't have code attribute
    if (item->code_attribute == NULL)
        return;
    m->max_locals = be16toh(code_attribute->max_locals);
    m->access_flags = be16toh(item->access_flags);
    m->code_length = code_attribute->code_length;
    m->code = code_attribute->code;

    init_method_instructions(m);

    init_jvm_instruction_graph(m);

    init_method_exception_table(m, item);
}

void jvm_method(jclass_file *jc, jd_method *m, jmethod *jm)
{
    jvm_method_init(jc, m, jm);

    if (method_is_unsupport(m) || method_is_empty(m))
        return;

    jvm_rename_goto2return(m);

    jvm_method_exception_edge(m);

    jvm_simulator(m);

    mark_unreachable_instruction(m);

    cfg_remove_exception_block(m);

    optimize_jvm_method(m);
}

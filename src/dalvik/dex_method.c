#include "decompiler/method.h"
#include "dalvik/dex_method.h"
#include "dex_descriptor.h"
#include "decompiler/descriptor.h"
#include "dex_exception.h"
#include "parser/dex/metadata.h"
#include "dex_annotation.h"

void dex_method_access_flag_with_flags(u4 flags, str_list *list)
{
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_PUBLIC, list, "public ")
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_PRIVATE, list, "private ")
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_PROTECTED, list, "protected ")
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_STATIC, list, "static ")
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_FINAL, list, "final ")
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_NATIVE, list, "native ")
    CONCAT_ACCESS_FLAG(flags, ACC_DEX_ABSTRACT, list, "abstract ")
    // CONCAT_ACCESS_FLAG(flags, ACC_DEX_SYNTHETIC, list, "/* synthetic */")
}

void dex_method_access_flags(jd_method *m, str_list *list)
{
    dex_method_access_flag_with_flags(m->access_flags, list);

    u4 sync_flags = ACC_DEX_SYNCHRONIZED | ACC_DEX_DECLARED_SYNCHRONIZED;
    if (access_flags_contains(m->access_flags, sync_flags) &&
        !method_is_sync_block(m))
        str_concat(list, "synchronized ");
}

jd_val* dex_method_parameter_val(jd_method *m, int index)
{
    if (m->enter == NULL)
        return NULL;

    jd_descriptor *desc = m->desc;
    int slot = m->max_locals;
    for (int i = desc->list->size - 1; i >= index; --i)
        slot -= descriptor_type_slot_width(lget_string(desc->list, i));

    return m->enter->local_vars[slot];
}

static void init_dex_ins_unconditional_jump(jd_dex_ins *ins)
{
    u4 jump_offset = dex_goto_offset(ins);
    jd_dex_ins *target_ins = (jd_dex_ins *) ins_of_offset(ins->method, jump_offset);
    ladd_obj(ins_targets((jd_ins *) ins), target_ins);
    ladd_obj(ins_jumps((jd_ins *) ins), target_ins);
}

static void init_dex_ins_conditional_jump(jd_dex_ins *ins)
{
    u4 jump_offset = (s2)(ins->param[1]) + ins->offset;
    jd_dex_ins *jump_to_ins = (jd_dex_ins *) ins_of_offset(ins->method, jump_offset);
    ladd_obj(ins_targets((jd_ins *) ins), jump_to_ins);
    ladd_obj(ins_jumps((jd_ins *) ins), jump_to_ins);
}


static void init_dex_packed_switch_jump(jd_dex_ins *ins)
{
    jd_method *m = ins->method;
    s4 packed_offset = (s4)ins->param[2] << 16 | ins->param[1];
    hashmap *map = m->offset2id_map;
    int payload_idx = hget_i2i(map, ins->offset + packed_offset);
    jd_dex_ins *packed_ins = lget_obj(m->instructions, payload_idx);

    int size = packed_ins->param[1];
    int first_key = packed_ins->param[3] << 16 | packed_ins->param[2];

    for (int i = 0; i < size; ++i) {
        int offset = packed_ins->param[5 + i * 2] << 16 |
                     packed_ins->param[4 + i * 2];
        jd_dex_ins *target_ins = (jd_dex_ins *) ins_of_offset(m, ins->offset+offset);
        ladd_obj(ins_targets((jd_ins *) ins), target_ins);
        ladd_obj(ins_jumps((jd_ins *) ins), target_ins);
        DEBUG_PRINT("key: %d, goto_offset: %d\n",
                first_key+i, offset+ins->offset);
    }
    if (ins->next != NULL)
        ladd_obj_no_dup(ins_targets((jd_ins *) ins), ins->next);
}

static void init_dex_sparse_switch_jump(jd_dex_ins *ins)
{
    jd_method *m = ins->method;
    s4 offset = ins->param[2] << 16 | ins->param[1];
    int payload_idx = hget_i2i(m->offset2id_map, ins->offset + offset);
    jd_dex_ins *packed_ins = lget_obj(m->instructions, payload_idx);
    int size = packed_ins->param[1];

    u2 *params = packed_ins->param;
    for (int i = 0; i < size; ++i) {
        int key = params[3+i*2] << 16 | params[2+i*2];
        int val = params[3+size*2+i*2] << 16 | params[2+size*2+i*2];
        int target_id = hget_i2i(m->offset2id_map, ins->offset + val);
        jd_dex_ins *target_ins = get_dex_ins(m, target_id);
        ladd_obj(ins_targets((jd_ins *) ins), target_ins);
        ladd_obj(ins_jumps((jd_ins *) ins), target_ins);
        DEBUG_PRINT("key is: %d -> val: %d\n", key, val);
    }
    if (ins->next != NULL)
        ladd_obj_no_dup(ins_targets((jd_ins *) ins), ins->next);
}

static void init_dex_instruction_graph(jd_method *m)
{
    list_object *instructions = m->instructions;
    for (int i = 0; i < instructions->size; ++i) {
        jd_dex_ins *ins = lget_obj(instructions, i);
        if (dex_ins_is_unconditional_jump(ins)) {
            init_dex_ins_unconditional_jump(ins);
        }
        else if (dex_ins_is_conditional_jump(ins)) {
            init_dex_ins_conditional_jump(ins);
        }
        else if (dex_ins_is_packed_switch(ins)) {
            init_dex_packed_switch_jump(ins);
        }
        else if (dex_ins_is_sparse_switch(ins)) {
            init_dex_sparse_switch_jump(ins);
        }
        else if (dex_ins_is_return_op(ins) || dex_ins_is_throw(ins)) {

        }

        for (int j = 0; ins->jumps != NULL && j < ins->jumps->size; ++j) {
            jd_dex_ins *jump_to_ins = lget_obj(ins->jumps, j);
            ladd_obj(ins_comings((jd_ins *) jump_to_ins), ins);
        }
    }
}

static void dex_code_item_instruction(jd_method *m, dex_code_item *code)
{
    m->instructions = linit_object();
    m->offset2id_map = hashmap_init((hcmp_fn) i2i_cmp, 0);
    m->ins_fn = ((jd_dex*)(m->meta))->ins_fn;
    jd_dex_ins *prev = NULL;
    uint32_t offset = 0;
    for (int i = 0; i < code->insns_size; ++i) {
        u2 item = code->insns[i];
        u1 opcode = item & 0xFF;

        jd_dex_ins *ins = make_obj(jd_dex_ins);
        ins->code = opcode;
        ins->name = dex_opcode_name(opcode);
        ins->format = dex_opcode_fmt(opcode);
        ins->idx = m->instructions->size;
        ins->offset = offset;
        ins->extra = NULL;

        ins->method = m;
        ins->param = &code->insns[i];
        ins->uses = bitset_create_with_capacity(m->max_locals);
        ins->defs = bitset_create_with_capacity(m->max_locals);
        ladd_obj(m->instructions, ins);
        hset_i2i(m->offset2id_map, offset, ins->idx);
        dex_ins_use_def_init(ins);

        if (dex_ins_is_goto_jump(ins)) {
            uint32_t target = dex_original_goto_offset(ins);
            ins->param = x_alloc(sizeof(u2) * 2);
            ins->param[0] = target >> 16;
            ins->param[1] = target;
        }

        if (dex_ins_is_if(ins)) {
            u2 *new_param = x_alloc(sizeof(u2) * 2);
            memcpy(new_param, ins->param, sizeof(u2) * 2);
            ins->param = new_param;
        }

        if (opcode == 0x00) {
            if (item == 0x0100) {
                u2 size = code->insns[i+1];
                ins->param_length = size * 2 + 4;
            }
            else if (item == 0x0200) {
                u2 size = code->insns[i+1];
                ins->param_length = size * 4 + 2;
            }
            else if (item == 0x0300) {
                u2 element_size = code->insns[i+1];
                u2 size = code->insns[i+2];
                ins->param_length = (size * element_size + 1) / 2 + 4;
            }
            else {
                ins->param_length = 1;
            }
        }
        else {
            ins->param_length = dex_opcode_len(opcode);
        }
        i += (ins->param_length - 1);
        offset += ins->param_length;

        if (prev == NULL)
            ins->prev = NULL;
        else {
            ins->prev = prev;
            prev->next = ins;
        }
        prev = ins;
    }
}

static bool dex_method_signature_equal(jd_meta_dex *meta,
                                       dex_method_id *a,
                                       dex_method_id *b)
{
    if (a->name_idx != b->name_idx)
        return false;
    dex_proto_id *pa = &meta->proto_ids[a->proto_idx];
    dex_proto_id *pb = &meta->proto_ids[b->proto_idx];
    if (pa->return_type_idx != pb->return_type_idx)
        return false;
    u4 sa = pa->type_list == NULL ? 0 : pa->type_list->size;
    u4 sb = pb->type_list == NULL ? 0 : pb->type_list->size;
    if (sa != sb)
        return false;
    for (u4 i = 0; i < sa; ++i) {
        if (pa->type_list->list[i].type_idx !=
            pb->type_list->list[i].type_idx)
            return false;
    }
    return true;
}

static bool dex_ancestor_method_accessible(jd_method *m,
                                           dex_class_def *ancestor,
                                           encoded_method *am)
{
    if ((am->access_flags & (ACC_DEX_PRIVATE | ACC_DEX_STATIC)) != 0)
        return false;
    if (am->access_flags & (ACC_DEX_PUBLIC | ACC_DEX_PROTECTED))
        return true;

    jd_meta_dex *meta = ((jd_dex *) m->meta)->meta;
    string desc = dex_str_of_type_id(meta, ancestor->class_idx);
    string anc_pkg = class_package_name_of(class_full_name(desc));
    if (anc_pkg == NULL)
        anc_pkg = (string) g_str_default;
    string my_pkg = m->jfile->pname;
    return my_pkg != NULL && STR_EQL(anc_pkg, my_pkg);
}

static bool dex_class_declares_override(jd_method *m,
                                        dex_class_def *ancestor,
                                        dex_method_id *mid)
{
    if (ancestor->class_data == NULL)
        return false;
    jd_meta_dex *meta = ((jd_dex *) m->meta)->meta;
    dex_class_data_item *data = ancestor->class_data;
    encoded_method *lists[2] = {data->virtual_methods, data->direct_methods};
    u4 sizes[2] = {data->virtual_methods_size, data->direct_methods_size};
    for (int l = 0; l < 2; ++l) {
        for (u4 i = 0; i < sizes[l]; ++i) {
            encoded_method *am = &lists[l][i];
            dex_method_id *amid = &meta->method_ids[am->method_id];
            if (!dex_method_signature_equal(meta, amid, mid))
                continue;
            if (dex_ancestor_method_accessible(m, ancestor, am))
                return true;
        }
    }
    return false;
}

static void dex_push_ancestors(jd_meta_dex *meta,
                               dex_class_def *cf,
                               dex_class_def **queue,
                               int *tail)
{
    if (cf->superclass_idx != NO_INDEX) {
        string desc = dex_str_of_type_id(meta, cf->superclass_idx);
        dex_class_def *parent = hget_s2o(meta->class_name_map, desc);
        if (parent != NULL && *tail < 32)
            queue[(*tail)++] = parent;
    }
    if (cf->interfaces == NULL)
        return;
    for (u4 i = 0; i < cf->interfaces->size && *tail < 32; ++i) {
        string desc = dex_str_of_type_id(meta,
                                         cf->interfaces->list[i].type_idx);
        dex_class_def *parent = hget_s2o(meta->class_name_map, desc);
        if (parent != NULL)
            queue[(*tail)++] = parent;
    }
}

static bool dex_method_is_object_override(jd_meta_dex *meta,
                                          dex_method_id *mid)
{
    string name = dex_str_of_idx(meta, mid->name_idx);
    dex_proto_id *proto = &meta->proto_ids[mid->proto_idx];
    string ret = dex_str_of_type_id(meta, proto->return_type_idx);
    u4 count = proto->type_list == NULL ? 0 : proto->type_list->size;

    if (STR_EQL(name, "toString"))
        return count == 0 && STR_EQL(ret, "Ljava/lang/String;");
    if (STR_EQL(name, "hashCode"))
        return count == 0 && STR_EQL(ret, "I");
    if (STR_EQL(name, "equals"))
        return count == 1 && STR_EQL(ret, "Z") &&
               STR_EQL(dex_str_of_type_id(meta,
                       proto->type_list->list[0].type_idx),
                       "Ljava/lang/Object;");
    if (STR_EQL(name, "clone"))
        return count == 0 && STR_EQL(ret, "Ljava/lang/Object;");
    if (STR_EQL(name, "finalize"))
        return count == 0 && STR_EQL(ret, "V");
    return false;
}

static bool dex_method_overrides(jd_method *m, encoded_method *em)
{
    if ((em->access_flags & (ACC_DEX_PRIVATE | ACC_DEX_STATIC |
                             ACC_DEX_SYNTHETIC)) != 0)
        return false;

    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex->meta;
    dex_method_id *mid = &meta->method_ids[em->method_id];
    string name = dex_str_of_idx(meta, mid->name_idx);
    if (name == NULL || name[0] == '<')
        return false;

    dex_class_def *queue[32];
    dex_class_def *visited[32];
    int head = 0, tail = 0, visited_count = 0;
    dex_class_def *cf = (dex_class_def *) m->jfile->jclass;
    if ((cf->access_flags & ACC_DEX_INTERFACE) == 0 &&
        dex_method_is_object_override(meta, mid))
        return true;

    dex_push_ancestors(meta, cf, queue, &tail);

    while (head < tail) {
        dex_class_def *ancestor = queue[head++];
        bool seen = false;
        for (int i = 0; i < visited_count; ++i) {
            if (visited[i] == ancestor) {
                seen = true;
                break;
            }
        }
        if (seen)
            continue;
        if (visited_count < 32)
            visited[visited_count++] = ancestor;

        if (dex_class_declares_override(m, ancestor, mid))
            return true;
        dex_push_ancestors(meta, ancestor, queue, &tail);
    }
    return false;
}

void dex_method_init(jsource_file *jf, jd_method *m, encoded_method *em)
{
    jd_dex *dex = jf->meta;
    jd_meta_dex *meta = dex->meta;
    dex_method_id *method_id = &meta->method_ids[em->method_id];
    m->name = dex_str_of_idx(meta, method_id->name_idx);
    m->access_flags = em->access_flags;
    m->meta_method = em;
    m->meta = dex;
    m->jfile = jf;
    m->type = JD_TYPE_DALVIK;
    m->fn = dex->method_fn;

    dex_method_descriptor(m);

    m->is_override = dex_method_overrides(m, em);

    if (em->code == NULL)
        return;

    m->max_locals = em->code->registers_size;

    dex_code_item_instruction(m, em->code);

    init_dex_instruction_graph(m);

    dex_method_exception_init(m, em);
}

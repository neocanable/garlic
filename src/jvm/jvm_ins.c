#include <stdarg.h>
#include "decompiler/structure.h"
#include "jvm_ins.h"
#include "decompiler/descriptor.h"
#include "parser/class/class_tools.h"
#include "common/str_tools.h"
#include "decompiler/method.h"
#include "jvm/jvm_descriptor.h"

bool match_instruction_patten(jd_exp *expression, int num, ...)
{
    jd_ins *ins = expression->ins;
    va_list args;
    va_start(args, num);
    for (int i = 0; i < num; ++i) {
        if (ins == NULL)
            return false;
        ins_filter filter = va_arg(args, ins_filter);
        if (filter(ins)) {
            ins = ins->next;
            continue;
        }
        else
            return false;
    }
    va_end(args);
    return true;
}

int32_t jvm_switch_key(jd_ins *ins, uint32_t jump_offset)
{
    if (jvm_ins_is_tableswitch(ins)) {
        u1 p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11;
        uint32_t padding = jvm_switch_padding(ins->offset);
        p4  = ins->param[padding + 4];
        p5  = ins->param[padding + 5];
        p6  = ins->param[padding + 6];
        p7  = ins->param[padding + 7];
        p8  = ins->param[padding + 8];
        p9 = ins->param[padding + 9];
        p10 = ins->param[padding + 10];
        p11 = ins->param[padding + 11];

        int32_t low_byte  = (int32_t)(p4 << 24) | (p5 << 16) | (p6 << 8) | p7;
        int32_t high_byte = be_32(p8, p9, p10, p11);

        uint32_t jump_size = high_byte - low_byte + 1;
        uint32_t start_jump = padding + 12;
        int32_t key = low_byte;
        for (uint32_t k = 0; k < jump_size; k++) {
            p0 = ins->param[start_jump + k * 4 + 0];
            p1 = ins->param[start_jump + k * 4 + 1];
            p2 = ins->param[start_jump + k * 4 + 2];
            p3 = ins->param[start_jump + k * 4 + 3];
            uint32_t offset = be_32(p0, p1, p2, p3);
            offset = offset + ins->offset;
            if (jump_offset == offset)
                return key;
            key ++;
        }

    }
    else {
        u1 p0, p1, p2, p3, p4, p5, p6, p7;
        uint32_t padding = jvm_switch_padding(ins->offset);
        p4 = ins->param[padding + 4];
        p5 = ins->param[padding + 5];
        p6 = ins->param[padding + 6];
        p7 = ins->param[padding + 7];

        uint32_t npair = be_32(p4, p5, p6, p7);
        uint32_t start_npair = padding + 8;
        for (uint32_t k = 0; k < npair ;++k) {
            p0                   = ins->param[start_npair + k * 8 + 0];
            p1                   = ins->param[start_npair + k * 8 + 1];
            p2                   = ins->param[start_npair + k * 8 + 2];
            p3                   = ins->param[start_npair + k * 8 + 3];
            p4                   = ins->param[start_npair + k * 8 + 4];
            p5                   = ins->param[start_npair + k * 8 + 5];
            p6                   = ins->param[start_npair + k * 8 + 6];
            p7                   = ins->param[start_npair + k * 8 + 7];
            int32_t key          = (p0 << 24) | (p1 << 16) | (p2 << 8) | p3;
            uint32_t val         = be_32(p4, p5, p6, p7);
            uint32_t offset      = val + ins->offset;
            if (offset == jump_offset)
                return key;
        }
    }
    return 0;
}

int32_t jvm_switch_default_offset(jd_ins *ins)
{
    u1 p0, p1, p2, p3;
    uint32_t padding = jvm_switch_padding(ins->offset);
    p0 = ins->param[padding + 0];
    p1 = ins->param[padding + 1];
    p2 = ins->param[padding + 2];
    p3 = ins->param[padding + 3];
    return be_32s(p0, p1, p2, p3) + ins->offset;
}

void jvm_rename_goto2return(jd_method *m)
{
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_ins *ins = lget_obj(m->instructions, i);
        if (jvm_ins_is_goto(ins) || jvm_ins_is_goto_w(ins)) {
            jd_ins *target_ins = lget_obj_first(ins->jumps);

            if (jvm_ins_is_return(target_ins)) {
                ins->name = str_dup(target_ins->name);
                ins->code = target_ins->code;
                ins->popped_cnt = target_ins->popped_cnt;
                ins->pushed_cnt = target_ins->pushed_cnt;
                ins->param = target_ins->param;
                ins->param_length = target_ins->param_length;

                ldel_obj(ins->jumps, target_ins);
                ldel_obj(ins->targets, target_ins);
                ldel_obj(target_ins->comings, ins);
            }
        }
    }
}

jcp_info* jvm_invoke_methodref_info(jd_ins *ins)
{
    jd_method *m = ins->method;
    u1 *param = ins->param;
    u2 index = be16toh(param[0] << 8 | param[1]);
    jcp_info *info = pool_item(m->meta, index);

    if (jvm_ins_is_invokedynamic(ins)) {
        // invokedynamic
        return NULL;
    }
    else {
        jconst_methodref *methodref = info->info->methodref;
        index = methodref->class_index;
    }
    return pool_item(m->meta, index);
}

jcp_info* jvm_invoke_name_and_type_info(jd_ins *ins)
{
    jd_method *m = ins->method;
    u2 name_and_type_index = 0;
    jcp_info *info = NULL;
    if (jvm_ins_is_invokedynamic(ins)) {
        // invokedynamic
        u1 *param = ins->param;
        uint16_t index = be16toh(param[0] << 8 | param[1]);
        info = pool_item(m->meta, index);
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
    return pool_item(m->meta, name_and_type_index);
}

jd_descriptor* jvm_invoke_descriptor(jd_ins *ins)
{
    jd_method *m = ins->method;
    jcp_info *nt_info = jvm_invoke_name_and_type_info(ins);
    jconst_name_and_type *nt = nt_info->info->name_and_type;
    u2 desc_index = nt->descriptor_index;
    jclass_file *jc = m->meta;
    jsource_file *jf = jc->jfile;
    jd_descriptor *descriptor = jvm_descriptor(jf, desc_index);
    return descriptor;
}



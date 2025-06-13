#include "dalvik/dex_type_analyse.h"
#include "dalvik/dex_meta_helper.h"
#include "common/debug.h"
#include "decompiler/method.h"

bool match_dex_debug(jd_method *m, jd_dex_ins *ins, jd_val *val, int slot)
{
//    if (ins == NULL)
//        return false;

    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex->meta;
    encoded_method *em = m->meta_method;
    dex_code_item *code = em->code;
    if (code->debug_info_off == 0)
        return false;

    dex_debug_info_item *debug_info = code->debug_info;
    if (debug_info == NULL)
        return false;

    if (ins == NULL) {
        int max = m->max_locals;
        int start = max - m->desc->list->size;
        for (int i = 0; i < debug_info->parameters_size; ++i) {
            u4 name_idx = debug_info->parameter_name[i];
            if (name_idx == NO_INDEX)
                continue;
            int s = slot - start;
            s = method_is_member(m) ? s - 1 : s;
            if (s == i) {
                string name = dex_str_of_idx(meta, name_idx);
                val->name = name;
                return true;
            }
        }
        return false;
    }

    jd_dex_ins *next = ins->next;
    if (dex_ins_is_new_instance(ins) && next != NULL)
        next = next->next;

    if (next != NULL && dex_ins_is_check_cast(next))
        next = next->next;

    for (int i = 0; i < debug_info->dbg_size; ++i) {
        dex_dbg_item *item = &debug_info->items[i];

        if (item->reg_num == slot &&
            next != NULL &&
            next->offset == item->offset &&
            item->name_idx != NO_INDEX) {
            string name = dex_str_of_idx(meta, item->name_idx);
            DEBUG_PRINT("\t[MATCH_DBG_ITEM]: %d %d %s\n",
                        item->offset, item->reg_num, name);
            val->name = name;
            val->name_type = JD_VAR_NAME_DEBUG;
            return true;
        }
    }
    return false;
}

bool match_dex_parameter(jd_method *m, jd_stack *stack)
{
    // TODO: NEo parameter and dbg info not same
    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex->meta;
    encoded_method *em = m->meta_method;
    dex_code_item *code = em->code;
    dex_debug_info_item *debug_info = code->debug_info;
    if (debug_info == NULL)
        return false;
    int param_itor = m->max_locals - 1;
    for (int i = 0; i < debug_info->parameters_size; ++i) {
        u4 name_idx = debug_info->parameter_name[i];
        if (name_idx == NO_INDEX)
            continue;
        string name = dex_str_of_idx(meta, name_idx);
        jd_val *val = stack->local_vars[param_itor];
        val->name = name;
    }
    return true;
}
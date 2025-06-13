#include "common/str_tools.h"

#include "parser/dex/dex.h"
//#include "str/str.h"
#include "dalvik/dex_ins.h"
#include "dalvik/dex_meta_helper.h"

static inline string dex_ins_header(jd_dex_ins *ins)
{
    return str_create("\t[%08X]: %s ", ins->offset, ins->name);
}

static void write_nop(jd_dex_ins *ins)
{
    u2 item = ins->param[0];
    if (item == 0x0100) {
        printf("\t[%08X]: packed-switch-payload\n",
               ins->offset);
    }
    else if (item == 0x0200) {
        printf("\t[%08X]: sparse-switch-payload\n",
               ins->offset);
    }
    else if (item == 0x0300) {
        printf("\t[%08X]: fill-array-data-payload",
               ins->offset);
    }
    else {
        printf("\t[%08X]: nop\n", ins->offset);
    }
}

static void write_move(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_from16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_16(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_wide(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_wide_from16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_wide_16(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_object(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_object_from16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_object16(jd_dex_ins *ins)
{
    u2 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_move_result(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    printf("%s v%d\n", dex_ins_header(ins), u_a);
}

static void write_return_void(jd_dex_ins *ins)
{
    printf("%s\n", dex_ins_header(ins));
}

static void write_return(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    printf("%s v%d\n", dex_ins_header(ins), u_a);
}

static void write_const4(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s1 u_b = (s1)dex_ins_parameter(ins, 1);
    printf("%s v%d %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s2 u_b = (s2)dex_ins_parameter(ins, 1);
    printf("%s v%d, %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const_high16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 u_b = dex_ins_parameter(ins, 1) << 16;
    printf("%s v%d %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const_wide16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s2 u_b = (s2)dex_ins_parameter(ins, 1);
    printf("%s v%d %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const_wide32(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s4 u_b = (s4)dex_ins_parameter(ins, 1);
    printf("%s v%d, %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const_wide(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s8 u_b = (s8)dex_ins_parameter(ins, 1);
    printf("%s v%d, %ld\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const_wide_high16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 ub = (s2)dex_ins_parameter(ins, 1);
    s8 u_b = (s8)ub << 48;
    printf("%s v%d, %ld\n", dex_ins_header(ins), u_a, u_b);
}

static void write_const_string(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 index = dex_ins_parameter(ins, 1);
    string str = dex_str_of_idx(dex_ins_meta(ins), index);
    printf("%s v%d, \"%s\" // %d\n", dex_ins_header(ins), u_a, str, index);
}

static void write_const_string_jumbo(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u4 idx = dex_ins_parameter(ins, 1);

    jd_meta_dex *meta = dex_ins_meta(ins);
    string str = dex_str_of_idx(meta, idx);
    printf("%s v%d, %s // %d\n", dex_ins_header(ins), u_a, str, idx);
}

static void write_const_class(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    printf("%s v%d, %d\n", dex_ins_header(ins), u_a, type_index);
}

static void write_monitor(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    printf("%s v%d\n", dex_ins_header(ins), u_a);
}

static void write_check_cast(jd_dex_ins *ins)
{
    jd_meta_dex *meta = dex_ins_meta(ins);
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    string type_name = dex_str_of_type_id(meta, type_index);
    printf("%s v%d, %s // %d\n",
           dex_ins_header(ins), u_a, type_name, type_index);
}

static void write_instance_of(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u2 type_index = dex_ins_parameter(ins, 1);
    jd_meta_dex *meta = dex_ins_meta(ins);
    string type_name = dex_str_of_type_id(meta, type_index);
    printf("%s v%d, v%d %s // %d\n",
           dex_ins_header(ins), u_a, u_b, type_name, type_index);
}

static void write_array_length(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_new_instance(jd_dex_ins *ins)
{
    jd_meta_dex *meta = dex_ins_meta(ins);
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    string type_name = dex_str_of_type_id(meta, type_index);
    printf("%s v%d, %s // %d\n",
           dex_ins_header(ins), u_a, type_name, type_index);
}

static void write_new_array(jd_dex_ins *ins)
{
    jd_meta_dex *meta = dex_ins_meta(ins);
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u2 type_index = dex_ins_parameter(ins, 2);
    string tname = dex_str_of_type_id(meta, type_index);
    printf("%s v%d, v%d %s %d\n", dex_ins_header(ins),
           u_a, u_b, tname, type_index);
}

static void write_fill_new_array(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);
    u1 u_d = dex_ins_parameter(ins, 3);
    u1 u_e = dex_ins_parameter(ins, 4);
    u1 u_f = dex_ins_parameter(ins, 5);
    u1 u_g = dex_ins_parameter(ins, 6);

    switch (u_a) {
        case 0: {
            printf("%s %d\n", dex_ins_header(ins), type_index);
            break;
        }
        case 1: {
            printf("%s {v%d} %d\n", dex_ins_header(ins), u_c, type_index);
            break;
        }
        case 2: {
            printf("%s {v%d, v%d} %d\n",
                   dex_ins_header(ins), u_c, u_d, type_index);
            break;
        }
        case 3: {
            printf("%s {v%d, v%d, v%d} %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, type_index);
            break;
        }
        case 4: {
            printf("%s  {v%d, v%d, v%d, v%d} %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, u_f, type_index);
            break;
        }
        case 5: {
            printf("%s  {v%d, v%d, v%d, v%d, v%d} %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, u_f, u_g, type_index);
            break;
        }
        default: {
            fprintf(stderr, "[instruction] error \n");
            break;
        }
    }
}

static void write_fill_new_array_range(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 type_index = dex_ins_parameter(ins, 1);
    u2 counter = dex_ins_parameter(ins, 2);
    u2 count = counter + u_a - 1;

    printf("%s {\n", dex_ins_header(ins));
    for (int j = counter; j < count; ++j)
        printf("v%d, ", j);
    printf("} @%d\n", type_index);
}

static void write_fill_array_data(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s4 array_data = (s4)dex_ins_parameter(ins, 1);
    printf("%s v%d, %d\n", dex_ins_header(ins), u_a, array_data);
}

static void write_throw(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    printf("%s v%d\n", dex_ins_header(ins), u_a);
}

static void write_goto(jd_dex_ins *ins)
{
    printf("%s :%X\n", dex_ins_header(ins), dex_goto_offset(ins));
}

static void write_goto16(jd_dex_ins *ins)
{
    printf("%s :%X\n", dex_ins_header(ins), dex_goto_offset(ins));
}

static void write_goto32(jd_dex_ins *ins)
{
    printf("%s :%X\n", dex_ins_header(ins), dex_goto_offset(ins));
}

static void write_switch(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s4 u_b = (s4)dex_ins_parameter(ins, 1);
    printf("%s v%d %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_cmp(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 0);
    u1 u_c = dex_ins_parameter(ins, 0);

    printf("%s v%d v%d v%d\n", dex_ins_header(ins), u_a, u_b, u_c);
}

static void write_if(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    s2 u_c = (s2)dex_ins_parameter(ins, 2);
    printf("%s v%d, v%d :%X\n", dex_ins_header(ins),
            u_a, u_b, u_c + ins->offset);
}

static void write_ifz(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    s2 u_b = (s2)dex_ins_parameter(ins, 1);
    printf("%s v%d :%X\n", dex_ins_header(ins), u_a, u_b + ins->offset);
}

static void write_arrayop(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);
    printf("%s v%d v%d v%d\n", dex_ins_header(ins), u_a, u_b, u_c);
}

static void write_instanceop(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u2 field_index = dex_ins_parameter(ins, 2);
    jd_meta_dex *meta = dex_ins_meta(ins);
    dex_field_id *field_id = &meta->field_ids[field_index];

    string field_name = dex_str_of_field_name(meta, field_index);
    string class_name = dex_str_of_type_id(meta, field_id->class_idx);
    string type_name = dex_str_of_type_id(meta, field_id->type_idx);

    printf("%s v%d, v%d, %s.%s %s\n", dex_ins_header(ins),
           u_a, u_b, class_name, field_name, type_name);
}

static void write_staticop(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 field_index = dex_ins_parameter(ins, 1);
    jd_meta_dex *meta = dex_ins_meta(ins);
    dex_field_id *field_id = &meta->field_ids[field_index];

    string field_name = dex_str_of_idx(meta, field_id->name_idx);
    string class_name = dex_str_of_type_id(meta, field_id->class_idx);
    string type_name = dex_str_of_type_id(meta, field_id->type_idx);
    printf("%s v%d, %s.%s %s\n", dex_ins_header(ins),
           u_a, class_name, field_name, type_name);
}

static void write_invoke(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 method_index = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);
    u1 u_d = dex_ins_parameter(ins, 3);
    u1 u_e = dex_ins_parameter(ins, 4);
    u1 u_f = dex_ins_parameter(ins, 5);
    u1 u_g = dex_ins_parameter(ins, 6);

    jd_meta_dex *meta = dex_ins_meta(ins);
    dex_method_id *method_id = &meta->method_ids[method_index];
    u2 proto_idx = method_id->proto_idx;

    string class_name = dex_str_of_type_id(meta, method_id->class_idx);
    string name = dex_str_of_idx(meta, method_id->name_idx);

    dex_proto_id *proto = &meta->proto_ids[proto_idx];
    string return_str = dex_str_of_type_id(meta, proto->return_type_idx);
    dex_type_list *type_list = proto->type_list;
    str_list *method_def = str_list_init();
    str_concat(method_def, class_name);
    str_concat(method_def, "->");
    str_concat(method_def, name);
    str_concat(method_def, "(");

    if (type_list != NULL) {
        for (int j = 0; j < type_list->size; ++j) {
            u2 type_idx = type_list->list[j].type_idx;
            string desc = dex_str_of_type_id(meta, type_idx);
            str_concat(method_def, desc);
        }
    }
    str_concat(method_def, ")");
    str_concat(method_def, return_str);
    string method_defination = str_join(method_def);

    switch (u_a) {
        case 0: {
            printf("%s %s\n", dex_ins_header(ins), method_defination);
            break;
        }
        case 1: {
            printf("%s {v%d} %s\n",
                   dex_ins_header(ins), u_c, method_defination);
            break;
        }
        case 2: {
            printf("%s {v%d, v%d}, %s\n",
                   dex_ins_header(ins), u_c, u_d, method_defination);
            break;
        }
        case 3: {
            printf("%s {v%d, v%d, v%d}, %s\n",
                   dex_ins_header(ins), u_c, u_d, u_e, method_defination);
            break;
        }
        case 4: {
            printf("%s {v%d, v%d, v%d, v%d}, %s\n",
                   dex_ins_header(ins), 
                   u_c, u_d, u_e, u_f, method_defination);
            break;
        }
        case 5: {
            printf("%s {v%d, v%d, v%d, v%d, v%d}, %s\n",
                   dex_ins_header(ins), u_c, u_d, u_e, u_f, u_g, 
                   method_defination);
            break;
        }
        default: {
            printf("[instruction] error at invoke-kind\n");
            break;
        }

    }
}

static void write_invoke_range(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 method_index = dex_ins_parameter(ins, 1);
    u2 start_index = dex_ins_parameter(ins, 2);
    jd_meta_dex *meta = dex_ins_meta(ins);

    dex_method_id *method_id = &meta->method_ids[method_index];
    u2 proto_idx = method_id->proto_idx;

    string class_name = dex_str_of_type_id(meta, method_id->class_idx);
    string name = dex_str_of_idx(meta, method_id->name_idx);

    dex_proto_id *proto = &meta->proto_ids[proto_idx];
    u4 return_idx = proto->return_type_idx;
    string return_str = dex_str_of_type_id(meta, return_idx);

    dex_type_list *type_list = proto->type_list;
    str_list *method_def = str_list_init();
    str_concat(method_def, class_name);
    str_concat(method_def, ".");
    str_concat(method_def, name);
    str_concat(method_def, "(");

    if (type_list != NULL) {
        for (int j = 0; j < type_list->size; ++j) {
            u2 type_idx = type_list->list[j].type_idx;
            string desc = dex_str_of_type_id(meta, type_idx);
            str_concat(method_def, desc);
        }
    }
    str_concat(method_def, ")");
    str_concat(method_def, return_str);
    string method_defination = str_join(method_def);

    u2 count = start_index + u_a - 1;
    printf("%s {", dex_ins_header(ins));
    for (int j = start_index; j <= count; ++j) {
        printf("v%d, ", j);
    }
    printf("}, %s\n", method_defination);
}

static void write_unop(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_binop(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);
    printf("%s v%d, v%d, v%d\n", dex_ins_header(ins), u_a, u_b, u_c);
}

static void write_binop_2addr(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, v%d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_binop_lit16(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    s2 u_c = (s2)dex_ins_parameter(ins, 2);
    printf("%s v%d, v%d, %d\n", dex_ins_header(ins), u_a, u_b, u_c);
}

static void write_binop_lit8(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u1 u_b = dex_ins_parameter(ins, 1);
    s1 u_c = (s1)dex_ins_parameter(ins, 2);
    printf("%s v%d v%d %d\n", dex_ins_header(ins), u_a, u_b, u_c);
}

static void write_invoke_polymorphic(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 v_bbbb = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);
    u1 u_d = dex_ins_parameter(ins, 3);
    u1 u_e = dex_ins_parameter(ins, 4);
    u1 u_f = dex_ins_parameter(ins, 5);
    u1 u_g = dex_ins_parameter(ins, 6);
    u2 v_hhhh = dex_ins_parameter(ins, 7);
    switch (u_a) {
        case 1: {
            printf("%s {v%d} %d, %d\n",
                   dex_ins_header(ins), u_c, v_bbbb, v_hhhh);
            break;
        }
        case 2: {
            printf("%s {v%d, v%d} %d, %d\n",
                   dex_ins_header(ins), u_c, u_d, v_bbbb, v_hhhh);
            break;
        }
        case 3: {
            printf("%s {v%d, v%d, v%d} %d, %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, v_bbbb, v_hhhh);
            break;
        }
        case 4: {
            printf("%s {v%d, v%d, v%d, v%d} %d, %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, u_f, v_bbbb, v_hhhh);
            break;
        }
        case 5: {
            printf("%s {v%d, v%d, v%d, v%d, v%d} %d, %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e,
                   u_f, u_g, v_bbbb, v_hhhh);
            break;
        }
        default: {
            printf("%s error at invoke-kind\n", dex_ins_header(ins));
            break;
        }
    }
}

static void write_invoke_polymorphic_range(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 v_bbbb = dex_ins_parameter(ins, 1);
    u2 v_cccc = dex_ins_parameter(ins, 2);
    u2 v_hhhh = dex_ins_parameter(ins, 3);
    u2 count = v_cccc + u_a - 1;
    printf("%s {", dex_ins_header(ins));
    for (int j = v_cccc; j <= count; ++j) {
        printf("v%d, ", j);
    }
    printf("} %d, %d\n", v_bbbb, v_hhhh);
}

static void write_invoke_custom(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 method_index = dex_ins_parameter(ins, 1);
    u1 u_c = dex_ins_parameter(ins, 2);
    u1 u_d = dex_ins_parameter(ins, 3);
    u1 u_e = dex_ins_parameter(ins, 4);
    u1 u_f = dex_ins_parameter(ins, 5);
    u1 u_g = dex_ins_parameter(ins, 6);

    switch (u_a) {
        case 0: {
            printf("%s call_site@%d\n", dex_ins_header(ins), method_index);
            break;
        }
        case 1: {
            printf("%s {v%d} call_site@%d\n", 
                    dex_ins_header(ins), u_c, method_index);
            break;
        }
        case 2: {
            printf("%s {v%d, v%d}, call_site@%d\n",
                   dex_ins_header(ins), u_c, u_d, method_index);
            break;
        }
        case 3: {
            printf("%s {v%d, v%d, v%d}, call_site@%d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, method_index);
            break;
        }
        case 4: {
            printf("%s {v%d, v%d, v%d, v%d}, %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, u_f, method_index);
            break;
        }
        case 5: {
            printf("%s {v%d, v%d, v%d, v%d, v%d}, %d\n",
                   dex_ins_header(ins), u_c, u_d, u_e, u_f, u_g, method_index);
            break;
        }
        default: {
            fprintf(stderr, "[instruction] error at invoke\n");
            break;
        }

    }
}

static void write_invoke_custom_range(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_bbbb = dex_ins_parameter(ins, 1);
    u2 u_cccc = dex_ins_parameter(ins, 2);
    u2 count = u_cccc + u_a - 1;
    printf("%s {", dex_ins_header(ins));
    for (int j = u_cccc; j <= count; ++j) {
        printf("v%d, ", j);
    }
    printf("}, call_site@%d\n", u_bbbb);
}

static void write_const_method(jd_dex_ins *ins)
{
    u1 u_a = dex_ins_parameter(ins, 0);
    u2 u_b = dex_ins_parameter(ins, 1);
    printf("%s v%d, %d\n", dex_ins_header(ins), u_a, u_b);
}

static void write_copy_basic_block(jd_dex_ins *ins)
{
    u2 start = ins->param[0];
    u2 end = ins->param[1];

    printf("%s %d, %d\n", dex_ins_header(ins), start, end);
}


void dexdump_method(jd_method *m)
{
    printf(".m %s(%s)%s\n", m->name,
           m->desc->str,
           m->desc->str_return);
    printf("\t.locals %d\n", m->max_locals);
    for (int i = 0; i < m->instructions->size; ++i) {
        jd_dex_ins *ins = lget_obj(m->instructions, i);
        switch(ins->code) {
            case DEX_INS_NOP:
                write_nop(ins);
                break;
            case DEX_INS_MOVE: // move
                write_move(ins);
                break;
            case DEX_INS_MOVE_FROM16:
                write_move_from16(ins);
                break;
            case DEX_INS_MOVE_16:
                write_move_16(ins);
                break;
            case DEX_INS_MOVE_WIDE:
                write_move_wide(ins);
                break;
            case DEX_INS_MOVE_WIDE_FROM16:
                write_move_wide_from16(ins);
                break;
            case DEX_INS_MOVE_WIDE_16:
                write_move_wide_16(ins);
                break;
            case DEX_INS_MOVE_OBJECT:
                write_move_object(ins);
                break;
            case DEX_INS_MOVE_OBJECT_FROM16:
                write_move_object_from16(ins);
                break;
            case DEX_INS_MOVE_OBJECT_16:
                write_move_object16(ins);
                break;
            case DEX_INS_MOVE_RESULT: // move-result
            case DEX_INS_MOVE_RESULT_WIDE: // move-result-wide
            case DEX_INS_MOVE_RESULT_OBJECT: // move-result-object
            case DEX_INS_MOVE_EXCEPTION: // move-result-exception
                write_move_result(ins);
                break;
            case DEX_INS_RETURN_VOID: // return-void
                write_return_void(ins);
                break;
            case DEX_INS_RETURN: // return vAA
            case DEX_INS_RETURN_WIDE: // return-wide vAA
            case DEX_INS_RETURN_OBJECT: // return-object vAA
                write_return(ins);
                break;
            case DEX_INS_CONST_4: // const/4
                write_const4(ins);
                break;
            case DEX_INS_CONST_16:
                write_const16(ins);
                break;
            case DEX_INS_CONST: // const vAA, #+BBBBBBBB
                write_const(ins);
                break;
            case DEX_INS_CONST_HIGH16: // const/high16
                write_const_high16(ins);
                break;
            case DEX_INS_CONST_WIDE_16: // const-wide/16
                write_const_wide16(ins);
                break;
            case DEX_INS_CONST_WIDE_32: // const-wide/32
                write_const_wide32(ins);
                break;
            case DEX_INS_CONST_WIDE: // const-wide vAA, #+BBBBBBBBBBBBBBBB
                write_const_wide(ins);
                break;
            case DEX_INS_CONST_WIDE_HIGH16: // const-wide/high16
                write_const_wide_high16(ins);
                break;
            case DEX_INS_CONST_STRING: // const-string
                write_const_string(ins);
                break;
            case DEX_INS_CONST_STRING_JUMBO: // const-string/jumbo
                write_const_string_jumbo(ins);
                break;
            case DEX_INS_CONST_CLASS: // const-class
                write_const_class(ins);
                break;
            case DEX_INS_MONITOR_ENTER: // monitor-enter
            case DEX_INS_MONITOR_EXIT:
                write_monitor(ins);
                break;
            case DEX_INS_CHECK_CAST: // check-cast
                write_check_cast(ins);
                break;
            case DEX_INS_INSTANCE_OF: // instance-of
                write_instance_of(ins);
                break;
            case DEX_INS_ARRAY_LENGTH: // array-length
                write_array_length(ins);
                break;
            case DEX_INS_NEW_INSTANCE: // new-instance
                write_new_instance(ins);
                break;
            case DEX_INS_NEW_ARRAY:
                write_new_array(ins);
                break;
            case DEX_INS_FILLED_NEW_ARRAY: // filled-new-array
                write_fill_new_array(ins);
                break;
            case DEX_INS_FILLED_NEW_ARRAY_RANGE: // filled-new-array/range
                write_fill_new_array_range(ins);
                break;
            case DEX_INS_FILL_ARRAY_DATA: // fill-array-data
                write_fill_array_data(ins);
                break;
            case DEX_INS_THROW: // throw
                write_throw(ins);
                break;
            case DEX_INS_GOTO:
                write_goto(ins);
                break;
            case DEX_INS_GOTO_16: // goto/16
                write_goto16(ins);
                break;
            case DEX_INS_GOTO_32: // goto/32
                write_goto32(ins);
                break;
            case DEX_INS_PACKED_SWITCH: // packed-switch
            case DEX_INS_SPARSE_SWITCH: // sparse-switch
                write_switch(ins);
                break;
            case DEX_INS_CMPL_FLOAT: // cmpl-float
            case DEX_INS_CMPG_FLOAT: // cmpg-float
            case DEX_INS_CMPL_DOUBLE: // cmpl-double
            case DEX_INS_CMPG_DOUBLE: // cmpg-double
            case DEX_INS_CMP_LONG: // cmpl-long
                write_cmp(ins);
                break;
            case DEX_INS_IF_EQ: // if-eq
            case DEX_INS_IF_NE: // if-ne
            case DEX_INS_IF_LT: // if-lt
            case DEX_INS_IF_GE: // if-ge
            case DEX_INS_IF_GT: // if-gt
            case DEX_INS_IF_LE: // if-le
                write_if(ins);
                break;
            case DEX_INS_IF_EQZ: // if-eqz
            case DEX_INS_IF_NEZ: // if-nez
            case DEX_INS_IF_LTZ: // if-ltz
            case DEX_INS_IF_GEZ: // if-gez
            case DEX_INS_IF_GTZ: // if-gtz
            case DEX_INS_IF_LEZ: { // if-lez
                write_ifz(ins);
                break;
            }
            case 0x3E:
            case 0x3F:
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43: {
                break;
            }
            case DEX_INS_AGET:
            case DEX_INS_AGET_WIDE:
            case DEX_INS_AGET_OBJECT:
            case DEX_INS_AGET_BOOLEAN:
            case DEX_INS_AGET_BYTE:
            case DEX_INS_AGET_CHAR:
            case DEX_INS_AGET_SHORT:
            case DEX_INS_APUT:
            case DEX_INS_APUT_WIDE:
            case DEX_INS_APUT_OBJECT:
            case DEX_INS_APUT_BOOLEAN:
            case DEX_INS_APUT_BYTE:
            case DEX_INS_APUT_CHAR:
            case DEX_INS_APUT_SHORT: {
                write_arrayop(ins);
                break;
            }
            case DEX_INS_IGET: // iget
            case DEX_INS_IGET_WIDE: // iget-wide
            case DEX_INS_IGET_OBJECT: // iget-object
            case DEX_INS_IGET_BOOLEAN: // iget-boolean
            case DEX_INS_IGET_BYTE: // iget-byte
            case DEX_INS_IGET_CHAR: // iget-char
            case DEX_INS_IGET_SHORT: // iget-short
            case DEX_INS_IPUT: // iput
            case DEX_INS_IPUT_WIDE: // iput-wide
            case DEX_INS_IPUT_OBJECT: // iput-object
            case DEX_INS_IPUT_BOOLEAN: // iput-boolean
            case DEX_INS_IPUT_BYTE: // iput-byte
            case DEX_INS_IPUT_CHAR: // iput-char
            case DEX_INS_IPUT_SHORT: { // iput-short
                write_instanceop(ins);
                break;
            }
            case DEX_INS_SGET: // sget
            case DEX_INS_SGET_WIDE: // sget-wide
            case DEX_INS_SGET_OBJECT: // sget-object
            case DEX_INS_SGET_BOOLEAN: // sget-boolean
            case DEX_INS_SGET_BYTE: // sget-byte
            case DEX_INS_SGET_CHAR: // sget-char
            case DEX_INS_SGET_SHORT: // sget-short
            case DEX_INS_SPUT: // sput
            case DEX_INS_SPUT_WIDE: // sput-wide
            case DEX_INS_SPUT_OBJECT: // sput-object
            case DEX_INS_SPUT_BOOLEAN: // sput-boolean
            case DEX_INS_SPUT_BYTE: // sput-byte
            case DEX_INS_SPUT_CHAR: // sput-char
            case DEX_INS_SPUT_SHORT: { // sput-short
                write_staticop(ins);
                break;
            }
            case DEX_INS_INVOKE_VIRTUAL: // invoke-virtual
            case DEX_INS_INVOKE_SUPER: // invoke-super
            case DEX_INS_INVOKE_DIRECT: // invoke-direct
            case DEX_INS_INVOKE_STATIC: // invoke-static
            case DEX_INS_INVOKE_INTERFACE: { // invoke-interface
                write_invoke(ins);
                break;
            }
            case 0x73: {
                break;
            }
            case DEX_INS_INVOKE_VIRTUAL_RANGE:
            case DEX_INS_INVOKE_SUPER_RANGE:
            case DEX_INS_INVOKE_DIRECT_RANGE:
            case DEX_INS_INVOKE_STATIC_RANGE:
            case DEX_INS_INVOKE_INTERFACE_RANGE: {
                write_invoke_range(ins);
                break;
            }
            case 0x79:
            case 0x7A: {
                fprintf(stdout, "[instruction opcode] not used\n");
                break;
            }
            case DEX_INS_NEG_INT:
            case DEX_INS_NOT_INT:
            case DEX_INS_NEG_LONG:
            case DEX_INS_NOT_LONG:
            case DEX_INS_NEG_FLOAT:
            case DEX_INS_NEG_DOUBLE:
            case DEX_INS_INT_TO_LONG:
            case DEX_INS_INT_TO_FLOAT:
            case DEX_INS_INT_TO_DOUBLE:
            case DEX_INS_LONG_TO_INT:
            case DEX_INS_LONG_TO_FLOAT:
            case DEX_INS_LONG_TO_DOUBLE:
            case DEX_INS_FLOAT_TO_INT:
            case DEX_INS_FLOAT_TO_LONG:
            case DEX_INS_FLOAT_TO_DOUBLE:
            case DEX_INS_DOUBLE_TO_INT:
            case DEX_INS_DOUBLE_TO_LONG:
            case DEX_INS_DOUBLE_TO_FLOAT:
            case DEX_INS_INT_TO_BYTE:
            case DEX_INS_INT_TO_CHAR:
            case DEX_INS_INT_TO_SHORT: {
                write_unop(ins);
                break;
            }
            case DEX_INS_ADD_INT:
            case DEX_INS_SUB_INT:
            case DEX_INS_MUL_INT:
            case DEX_INS_DIV_INT:
            case DEX_INS_REM_INT:
            case DEX_INS_AND_INT:
            case DEX_INS_OR_INT:
            case DEX_INS_XOR_INT:
            case DEX_INS_SHL_INT:
            case DEX_INS_SHR_INT:
            case DEX_INS_USHR_INT:
            case DEX_INS_ADD_LONG:
            case DEX_INS_SUB_LONG:
            case DEX_INS_MUL_LONG:
            case DEX_INS_DIV_LONG:
            case DEX_INS_REM_LONG:
            case DEX_INS_AND_LONG:
            case DEX_INS_OR_LONG:
            case DEX_INS_XOR_LONG:
            case DEX_INS_SHL_LONG:
            case DEX_INS_SHR_LONG:
            case DEX_INS_USHR_LONG:
            case DEX_INS_ADD_FLOAT:
            case DEX_INS_SUB_FLOAT:
            case DEX_INS_MUL_FLOAT:
            case DEX_INS_DIV_FLOAT:
            case DEX_INS_REM_FLOAT:
            case DEX_INS_ADD_DOUBLE:
            case DEX_INS_SUB_DOUBLE:
            case DEX_INS_MUL_DOUBLE:
            case DEX_INS_DIV_DOUBLE:
            case DEX_INS_REM_DOUBLE: {
                write_binop(ins);
                break;
            }
            case DEX_INS_ADD_INT_2ADDR:
            case DEX_INS_SUB_INT_2ADDR:
            case DEX_INS_MUL_INT_2ADDR:
            case DEX_INS_DIV_INT_2ADDR:
            case DEX_INS_REM_INT_2ADDR:
            case DEX_INS_AND_INT_2ADDR:
            case DEX_INS_OR_INT_2ADDR:
            case DEX_INS_XOR_INT_2ADDR:
            case DEX_INS_SHL_INT_2ADDR:
            case DEX_INS_SHR_INT_2ADDR:
            case DEX_INS_USHR_INT_2ADDR:
            case DEX_INS_ADD_LONG_2ADDR:
            case DEX_INS_SUB_LONG_2ADDR:
            case DEX_INS_MUL_LONG_2ADDR:
            case DEX_INS_DIV_LONG_2ADDR:
            case DEX_INS_REM_LONG_2ADDR:
            case DEX_INS_AND_LONG_2ADDR:
            case DEX_INS_OR_LONG_2ADDR:
            case DEX_INS_XOR_LONG_2ADDR:
            case DEX_INS_SHL_LONG_2ADDR:
            case DEX_INS_SHR_LONG_2ADDR:
            case DEX_INS_USHR_LONG_2ADDR:
            case DEX_INS_ADD_FLOAT_2ADDR:
            case DEX_INS_SUB_FLOAT_2ADDR:
            case DEX_INS_MUL_FLOAT_2ADDR:
            case DEX_INS_DIV_FLOAT_2ADDR:
            case DEX_INS_REM_FLOAT_2ADDR:
            case DEX_INS_ADD_DOUBLE_2ADDR:
            case DEX_INS_SUB_DOUBLE_2ADDR:
            case DEX_INS_MUL_DOUBLE_2ADDR:
            case DEX_INS_DIV_DOUBLE_2ADDR:
            case DEX_INS_REM_DOUBLE_2ADDR:{
                write_binop_2addr(ins);
                break;
            }
            case DEX_INS_ADD_INT_LIT16:
            case DEX_INS_RSUB_INT:
            case DEX_INS_MUL_INT_LIT16:
            case DEX_INS_DIV_INT_LIT16:
            case DEX_INS_REM_INT_LIT16:
            case DEX_INS_AND_INT_LIT16:
            case DEX_INS_OR_INT_LIT16:
            case DEX_INS_XOR_INT_LIT16: {
                write_binop_lit16(ins);
                break;
            }
            case DEX_INS_ADD_INT_LIT8:
            case DEX_INS_RSUB_INT_LIT8:
            case DEX_INS_MUL_INT_LIT8:
            case DEX_INS_DIV_INT_LIT8:
            case DEX_INS_REM_INT_LIT8:
            case DEX_INS_AND_INT_LIT8:
            case DEX_INS_OR_INT_LIT8:
            case DEX_INS_XOR_INT_LIT8:
            case DEX_INS_SHL_INT_LIT8:
            case DEX_INS_SHR_INT_LIT8:
            case DEX_INS_USHR_INT_LIT8: {
                write_binop_lit8(ins);
                break;
            }
            case DEX_INS_INVOKE_POLYMORPHIC:
            {
                write_invoke_polymorphic(ins);
                break;
            }
            case DEX_INS_INVOKE_POLYMORPHIC_RANGE: {
                write_invoke_polymorphic_range(ins);
                break;
            }
            case DEX_INS_INVOKE_CUSTOM: {
                write_invoke_custom(ins);
                break;
            }
            case DEX_INS_INVOKE_CUSTOM_RANGE: {
                write_invoke_custom_range(ins);
                break;
            }
            case DEX_INS_CONST_METHOD_HANDLE:
            case DEX_INS_CONST_METHOD_TYPE: {
                write_const_method(ins);
                break;
            }
            case DEX_INS_COPY_BASIC_BLOCK: {
                write_copy_basic_block(ins);
                break;
            }
        }
    }
    printf(".end m\n\n");
}


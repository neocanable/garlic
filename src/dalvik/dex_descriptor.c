#include "dalvik/dex_descriptor.h"
#include "dex_class.h"

void dex_method_descriptor(jd_method *m)
{
    jd_descriptor *desc = make_obj(jd_descriptor);
    m->desc = desc;

    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex->meta;
    encoded_method *em = m->meta_method;
    dex_method_id *method_id = &meta->method_ids[em->method_id];
    dex_proto_id *proto_id = &meta->proto_ids[method_id->proto_idx];

    desc->str_return = dex_str_of_type_id(meta, proto_id->return_type_idx);
    desc->list = linit_string();
    if (proto_id->parameters_off == 0)
        return;

    str_list *str_list = str_list_init();
    for (int i = 0; i < proto_id->type_list->size; ++i) {
        dex_type_item *type_item = &proto_id->type_list->list[i];
        string type = dex_str_of_type_id(meta, type_item->type_idx);
        str_concat(str_list, type);
        ladd_string(desc->list, type);
    }
    desc->str = str_join(str_list);
}

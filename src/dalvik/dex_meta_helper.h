#ifndef GARLIC_DEX_META_HELPER_H
#define GARLIC_DEX_META_HELPER_H

#include "dalvik/dex_structure.h"
#include "dalvik/dex_ins.h"

static inline string dex_str_of_idx(jd_meta_dex *meta, u4 idx)
{
    return meta->strings[idx].data;
}

static inline string dex_str_of_type_id(jd_meta_dex *meta, u2 idx)
{
    return dex_str_of_idx(meta, meta->type_ids[idx].descriptor_idx);
}

static inline string dex_str_of_field_type(jd_meta_dex *meta, u2 idx)
{
    return dex_str_of_type_id(meta, meta->field_ids[idx].type_idx);
}

static inline string dex_str_of_field_name(jd_meta_dex *meta, u2 field_idx)
{
    dex_field_id field_id = meta->field_ids[field_idx];
    return dex_str_of_idx(meta, field_id.name_idx);
}

static inline string dex_str_of_field_class(jd_meta_dex *meta, u2 field_idx)
{
    dex_field_id field_id = meta->field_ids[field_idx];
    return dex_str_of_type_id(meta, field_id.class_idx);
}

static inline string dex_str_of_method_id(jd_meta_dex *meta, u2 idx)
{
    return dex_str_of_idx(meta, meta->method_ids[idx].name_idx);
}

static inline string dex_method_class_descriptor(jd_meta_dex *meta,
                                           encoded_method *em)
{
    // m's class desc
    dex_method_id *method_id = &meta->method_ids[em->method_id];
    dex_type_id *type_id = &meta->type_ids[method_id->class_idx];
    return meta->strings[type_id->descriptor_idx].data;
}

static inline string dex_field_desc(jd_meta_dex *meta, encoded_field *efield)
{
    // field's class desc
    dex_field_id *field_id = &meta->field_ids[efield->field_id];
    dex_type_id *type_id = &meta->type_ids[field_id->class_idx];
    return meta->strings[type_id->descriptor_idx].data;
}

static inline string dex_field_name(jd_meta_dex *meta, encoded_field *efield)
{
    // field's name
    dex_field_id *field_id = &meta->field_ids[efield->field_id];
    return meta->strings[field_id->name_idx].data;
}
#endif //GARLIC_DEX_META_HELPER_H

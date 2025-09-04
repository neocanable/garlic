#ifndef GARLIC_DEX_METHOD_H
#define GARLIC_DEX_METHOD_H

#include "dalvik/dex_structure.h"
#include "dalvik/dex_class.h"
#include "decompiler/method.h"

#define dex_method_meta(method) (((jd_dex*)method->meta)->meta)

void dex_method_init(jsource_file *jf,
                     jd_method *m,
                     encoded_method *em);

jd_val* dex_method_parameter_val(jd_method *m, int index);

void dex_method_access_flag_with_flags(u4 flags, str_list *list);

void dex_method_access_flags(jd_method *m, str_list *list);

static inline bool dex_encoded_method_is_lambda(jd_meta_dex *meta,
                                                encoded_method *em)
{
    dex_method_id method_id = meta->method_ids[em->method_id];
    string name = meta->strings[method_id.name_idx].data;
    return str_contains(name, "lambda$") ||
            (em->access_flags & ACC_DEX_SYNTHETIC) != 0;
}

static inline dex_class_def* dex_cdef_of_method(jd_meta_dex *meta,
                                                encoded_method *em)
{
    dex_method_id *method_id = &meta->method_ids[em->method_id];
    dex_type_id *type_id = &meta->type_ids[method_id->class_idx];
    return &meta->class_defs[type_id->descriptor_idx];
}

static inline bool dex_method_is_native(jd_method *m)
{
    return method_has_flag(m, ACC_DEX_NATIVE);
}

static inline bool dex_method_is_abstract(jd_method *m)
{
    return method_has_flag(m, ACC_DEX_ABSTRACT);
}

static inline bool dex_method_is_init(jd_method *m)
{
    return STR_EQL(m->name, g_str_init);
}

static inline  bool dex_method_is_clinit(jd_method *m)
{
    return STR_EQL(m->name, g_str_clinit);
}

static inline bool dex_method_is_member(jd_method *m)
{
    return !method_has_flag(m, ACC_DEX_STATIC);
}

static inline bool dex_method_is_synthetic(jd_method *m)
{
    return method_has_flag(m, ACC_DEX_SYNTHETIC);
}

static inline bool dex_method_is_varargs(jd_method *m)
{
    return method_has_flag(m, ACC_DEX_VARARGS);
}

#endif //GARLIC_DEX_METHOD_H

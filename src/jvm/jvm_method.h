#ifndef GARLIC_JVM_METHOD_H
#define GARLIC_JVM_METHOD_H

#include "decompiler/structure.h"
#include "decompiler/method.h"

jd_val* jvm_method_parameter_val(jd_method *m, int index);

void jvm_method_access_flags(jd_method *m, str_list *list);

void jvm_method_init(jclass_file *jc, jd_method *m, jmethod *item);

void jvm_method(jclass_file *jc, jd_method *m, jmethod *jm);

static inline bool jvm_method_is_native(jd_method *m)
{
    return method_has_flag(m, METHOD_ACC_NATIVE);
}

static inline bool jvm_method_is_abstract(jd_method *m)
{
    return method_has_flag(m, METHOD_ACC_ABSTRACT);
}

static inline bool jvm_method_is_init(jd_method *m)
{
    return STR_EQL(m->name, g_str_init);
}

static inline bool jvm_method_is_clinit(jd_method *m)
{
    return STR_EQL(m->name, g_str_clinit);
}

static inline bool jvm_method_is_member(jd_method *m)
{
    return !method_has_flag(m, METHOD_ACC_STATIC);
}

static inline bool jvm_method_is_synthetic(jd_method *m)
{
    return method_has_flag(m, METHOD_ACC_SYNTHETIC);
}

static inline bool jvm_method_is_varargs(jd_method *m)
{
    return method_has_flag(m, METHOD_ACC_VARARGS);
}

static inline bool jvm_method_is_empty(jd_method *m)
{
    return m->instructions == NULL || m->instructions->size == 0;
}

#endif //GARLIC_JVM_METHOD_H

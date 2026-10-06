#ifndef GARLIC_METHOD_H
#define GARLIC_METHOD_H

#include "common/debug.h"
#include "common/str_tools.h"
#include "parser/class/class_tools.h"
#include "jvm/jvm_lambda.h"
#include "decompiler/structure.h"
#include "decompiler/klass.h"
#include "decompiler/stack.h"

string create_method_defination(jd_method *m);

string create_lambda_defination(jd_method *m, int captures);

static inline bool method_is_jvm(jd_method *m)
{
    return m->type == JD_TYPE_JVM;
}

static inline bool method_is_daivik(jd_method *m)
{
    return m->type == JD_TYPE_DALVIK;
}

static inline bool method_has_flag(jd_method *m, uint16_t flag)
{
    return access_flags_contains(m->access_flags, flag);
}

static inline bool method_is_native(jd_method *m)
{
    return m->fn->is_native(m);
}

static inline bool method_is_init(jd_method *m)
{
    return m->fn->is_init(m);
}

static inline bool method_is_clinit(jd_method *m)
{
    return m->fn->is_clinit(m);
}

static inline bool method_is_abstract(jd_method *m)
{
    return m->fn->is_abstract(m);
}

static inline bool method_is_empty(jd_method *m)
{
    return m->instructions == NULL || m->instructions->size == 0;
}

static inline bool method_is_member(jd_method *m)
{
    return m->fn->is_member(m);
}

static inline bool method_is_synthetic(jd_method *m)
{
    return m->fn->is_synthetic(m);
}

static inline bool method_is_varargs(jd_method *m)
{
    return m->fn->is_varargs(m);
}

static inline void method_mark_hide(jd_method *m)
{
    m->state_flag |= METHOD_STATE_HIDE;
}

static inline bool method_is_hide(jd_method *m)
{
    return access_flags_contains(m->state_flag, METHOD_STATE_HIDE);
}

static inline void method_mark_sync_block(jd_method *m)
{
    m->state_flag |= METHOD_STATE_SYNC_BLOCK;
}

static inline bool method_is_sync_block(jd_method *m)
{
    return access_flags_contains(m->state_flag, METHOD_STATE_SYNC_BLOCK);
}

static inline void method_mark_lambda(jd_method *m)
{
    m->state_flag |= METHOD_STATE_LAMBDA;
}

static inline void method_mark_unsupport(jd_method *m)
{
    m->state_flag |= METHOD_STATE_UNSUPPORT;
}

static inline bool method_is_unsupport(jd_method *m)
{
    return access_flags_contains(m->state_flag, METHOD_STATE_UNSUPPORT);
}

static inline bool method_is_lambda(jd_method *m)
{
    return access_flags_contains(m->state_flag, METHOD_STATE_LAMBDA);
}

static inline bool method_is_enum_constructor(jd_method *m)
{
    if (!STR_EQL(m->name, g_str_init))
        return false;

    jsource_file *jf = m->jfile;
    if (!class_is_enum(jf))
        return false;

    return jf->super_cname != NULL &&
           STR_EQL(class_simple_name(jf->super_cname), (string) g_str_Enum);
}

static inline int method_synthetic_parameter_count(jd_method *m)
{
    return method_is_enum_constructor(m) ? 2 : 0;
}

static inline bool invoke_is_this_constructor(jd_exp_invoke *invoke,
                                              jd_exp *expression)
{
    if (invoke->method_name == NULL ||
            !STR_EQL(invoke->method_name, g_str_init))
        return false;

    jd_method *m = expression->ins->method;
    if (invoke->owner_class == NULL || m->jfile == NULL)
        return false;

    return STR_EQL(invoke->owner_class, m->jfile->fname);
}

#endif //GARLIC_METHOD_H

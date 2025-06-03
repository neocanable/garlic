
#ifndef GARLIC_DEBUG_POINTER_H
#define GARLIC_DEBUG_POINTER_H

static inline void dex_debug_v1(jd_method *m)
{
    jd_ins *ins = lget_obj_first(m->instructions);
    if (STR_EQL(m->name, "run") &&
        m->instructions->size == 33 &&
        STR_EQL(ins->name, "const-string/jumbo")) {
        DEBUG_PRINT("debug");
    }
}

static inline void debug_pointer_v2(jd_method *m)
{
    if (STR_EQL(m->name, "a") &&
        STR_EQL(m->desc->str_return, "Z")) {
        DEBUG_PRINT("debug");
    }
}

#endif //GARLIC_DEBUG_POINTER_H

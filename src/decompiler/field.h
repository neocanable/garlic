
#ifndef GARLIC_FIELD_H
#define GARLIC_FIELD_H

#include "decompiler/structure.h"
#include "common/endian.h"
#include "klass.h"


static inline void field_mark_hide(jd_field *field)
{
    field->state_flag |= FIELD_STATE_HIDE;
}

static inline bool field_is_hide(jd_field *field)
{
    return (field->state_flag & FIELD_STATE_HIDE) != 0;
}

static inline bool field_is_assert(jd_field *field)
{
    return STR_EQL(field->name, "$assertionsDisabled");
}

static inline bool field_has_flag(jd_field *field, uint16_t flag)
{
    return (field->access_flags & flag) != 0;
}
#endif //GARLIC_FIELD_H

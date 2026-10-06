#ifndef GARLIC_KLASS_H
#define GARLIC_KLASS_H

#include "decompiler/structure.h"
#include "common/endian_x.h"
#include "parser/dex/dex.h"

#define CONCAT_ACCESS_FLAG(access_flag, flag, list, str)        \
    if (access_flags_contains(access_flag, flag)) {             \
        str_concat(list, str);                                  \
    }                                                           \

#define access_flags_contains(access_flags, flag) ((access_flags & flag) != 0)

bool is_inner_class(string class_name);

bool is_anonymous_class(string class_name);

bool is_synthetic_lambda_class(string class_name);

static inline bool class_has_flag(jclass_file *jc, uint16_t flag)
{
    return access_flags_contains(be16toh(jc->access_flags), flag);
}

static inline bool class_is_enum(jsource_file *jf)
{
    if (jf == NULL || jf->jclass == NULL)
        return false;

    if (jf->type == JD_TYPE_DALVIK)
        return access_flags_contains(((dex_class_def *)jf->jclass)->access_flags,
                                     ACC_DEX_ENUM);

    return class_has_flag((jclass_file *)jf->jclass, CLASS_ACC_ENUM);
}

static inline bool class_is_interface(jsource_file *jf)
{
    if (jf == NULL || jf->jclass == NULL)
        return false;

    if (jf->type == JD_TYPE_DALVIK)
        return access_flags_contains(((dex_class_def *)jf->jclass)->access_flags,
                                     ACC_DEX_INTERFACE);

    return class_has_flag((jclass_file *)jf->jclass, CLASS_ACC_INTERFACE);
}

string class_path_to_short(string class_name);

string class_simple_name(string full);

string class_simple_name_without_primitive(string full);

string class_full_name(string descriptor);

string class_package_name(jsource_file *jf);

string class_package_name_of(string path);

void class_create_definations(jsource_file *jf);

void class_create_blocks(jsource_file *jf);

jd_node* class_root_block(jsource_file *jf);

jd_node* class_body_block(jsource_file *jf);

void class_import(jsource_file *jf, string path);

#endif //GARLIC_KLASS_H

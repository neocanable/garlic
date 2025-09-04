#ifndef GARLIC_DEX_CLASS_H
#define GARLIC_DEX_CLASS_H

#include "dalvik/dex_structure.h"
#include "dalvik/dex_meta_helper.h"

void dex_filed_access_flag(jd_field *field, str_list *list);

void dex_class_access_flag_with_flags(u4 flags, str_list *list);

void dex_field_access_flag_with_flags(u4 flags, str_list *list);

void dex_class_access_flag(jsource_file *jf, str_list *list);

bool dex_class_is_inner_class(jd_meta_dex *meta, dex_class_def *cf);

int dex_class_is_anonymous_class(jd_meta_dex *meta, dex_class_def *cf);

void dex_class_annotations(jsource_file *jf);

void dex_class_import(jsource_file *jf);

void dex_fields(jsource_file *jf);

#endif //GARLIC_DEX_CLASS_H

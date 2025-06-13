#ifndef GARLIC_DEX_ANNOTATION_H
#define GARLIC_DEX_ANNOTATION_H

#include "dalvik/dex_structure.h"

void dex_class_annotation(jsource_file *jf);

void dex_field_annotation(jd_meta_dex *meta,
                          jd_field *field,
                          dex_class_def *cf);

void dex_method_annotation(jd_method *m);

string dex_method_parameter_annotation(jd_method *m, int index);

list_object* dex_parameter_annotation(jd_method *m, int index);

void dex_annotations(jd_meta_dex *meta, dex_class_def *cf);

void print_all_class_annotations(jd_meta_dex *meta);

void print_dex_class_annotations(jd_meta_dex *meta, dex_class_def *cf);

#endif //GARLIC_DEX_ANNOTATION_H

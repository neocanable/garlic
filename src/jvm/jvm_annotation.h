#ifndef GARLIC_JVM_ANNOTATION_H
#define GARLIC_JVM_ANNOTATION_H

#include "decompiler/structure.h"

void jvm_method_annotation(jd_method *m);

void jvm_class_annotations(jsource_file *jf);

void jvm_field_annotations(jsource_file *jf);

void jvm_annotations(jsource_file *jf);

void jvm_print_all_signatures(jsource_file *jf);

string annotation_to_s(jclass_file *jc, annotation *ano);

string jvm_method_parameter_annotation(jd_method *m, int index);

#endif //GARLIC_JVM_ANNOTATION_H

#ifndef GARLIC_JVM_CLASS_H
#define GARLIC_JVM_CLASS_H

#include "decompiler/structure.h"
#include "parser/class/class_tools.h"

void jvm_class_access_flag(jsource_file *jf, str_list *list);

void jvm_field_access_flag(jd_field *field, str_list *list);

void jvm_fields(jsource_file *jf);

void jvm_signatures(jsource_file *jf);

#endif //GARLIC_JVM_CLASS_H

#ifndef GARLIC_JVM_DECOMPILE_H
#define GARLIC_JVM_DECOMPILE_H

#include "parser/class/metadata.h"

void jvm_analyse_class_file(jsource_file *jf);

void jvm_analyse_class_file_inside(jsource_file *jf);

void jvm_method(jclass_file *jc, jd_method *m, jmethod *jm);

#endif //GARLIC_JVM_DECOMPILE_H

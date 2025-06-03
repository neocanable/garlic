

#ifndef GARLIC_JVM_DESCRIPTOR_H
#define GARLIC_JVM_DESCRIPTOR_H

#include "decompiler/structure.h"
#include "decompiler/descriptor.h"

void jvm_collect_descriptor(jsource_file *jf);

jd_descriptor* jvm_descriptor(jsource_file *jf, u2 index);

#endif //GARLIC_JVM_DESCRIPTOR_H

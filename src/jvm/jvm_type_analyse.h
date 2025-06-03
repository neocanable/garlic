#ifndef GARLIC_JVM_TYPE_ANALYSE_H
#define GARLIC_JVM_TYPE_ANALYSE_H

#include "decompiler/structure.h"

typedef struct {
    string name;
    string signature;
    string sname;
    string fname;
} jd_matched_debug;

typedef struct {
    int             slot;
    string          simple_class_name;
    jd_ins          *ins;
    jd_var    *var;
} jd_val_type;

bool jvm_has_debug(jd_method *m, jd_ins *ins, int slot);

jd_matched_debug *matched_local_variable(jd_method *m, jd_ins *ins, int slot);

void jvm_int_type_analyze(jd_ins *ins);

void jvm_debug(jd_method *m, jd_ins *ins, jd_val *val, int slot);

void jvm_fix_type(jd_method *m);

#endif //GARLIC_JVM_TYPE_ANALYSE_H

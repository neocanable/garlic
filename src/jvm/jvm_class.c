#include "decompiler/klass.h"
#include "decompiler/descriptor.h"
#include "decompiler/field.h"

#include "jvm/jvm_class.h"
#include "parser/class/class_tools.h"
#include "jvm_descriptor.h"

void jvm_signatures(jsource_file *jf)
{
    jclass_file *jc = jf->jclass;
    for (int i = 0; i < be16toh(jc->attributes_count); ++i) {
        jattr *attr = &jc->attributes[i];
        if (STR_EQL(attr->name, "Signature")) {
            jattr_signature *s = (jattr_signature*)attr->info;
            jf->signature = pool_str(jc, s->signature_index);
            break;
        }
    }

    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];
        jfield *f = field->meta;
        for (int j = 0; j < be16toh(f->attributes_count); ++j) {
             jattr *attr = &f->attributes[j];
             if (STR_EQL(attr->name, "Signature")) {
                jattr_signature *s = (jattr_signature*)attr->info;
                field->signature = pool_str(jc, s->signature_index);
                break;
             }
        }
    }

    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        jmethod *jm = m->meta_method;
        for (int j = 0; j < be16toh(jm->attributes_count); ++j) {
            jattr *attr = &jm->attributes[j];
            if (STR_EQL(attr->name, "Signature")) {
                jattr_signature *s = (jattr_signature*)attr->info;
                m->signature = pool_str(jc, s->signature_index);
                break;
            }
        }
    }
}

void jvm_class_access_flag(jsource_file *jf, str_list *list)
{
    // https://docs.oracle.com/javase/specs/jvms/se15/html/jvms-4.html
    // #jvms-4.1-200-E.1
    jclass_file *jc = jf->jclass;
    if (class_has_flag(jc, CLASS_ACC_PUBLIC))
        str_concat(list, "public ");
    if (class_has_flag(jc, CLASS_ACC_FINAL))
        str_concat(list, "final ");
    if (class_has_flag(jc, CLASS_ACC_ABSTRACT) && !
            class_has_flag(jc, CLASS_ACC_INTERFACE))
        str_concat(list, "abstract ");
    if (class_has_flag(jc, CLASS_ACC_SYNTHETIC))
        str_concat(list, "/* synthetic */ ");

    if (class_has_flag(jc, CLASS_ACC_INTERFACE))
        str_concat(list, "interface ");
    else if (class_has_flag(jc, CLASS_ACC_ENUM))
        str_concat(list, "enum ");
    else
        str_concat(list, "class ");
}

void jvm_field_access_flag(jd_field *field, str_list *list)
{
    if (field_has_flag(field, FIELD_ACC_FINAL))
        str_concat(list, "final ");
    if (field_has_flag(field, FIELD_ACC_PUBLIC))
        str_concat(list, "public ");
    if (field_has_flag(field, FIELD_ACC_PRIVATE))
        str_concat(list, "private ");
    if (field_has_flag(field, FIELD_ACC_PROTECTED))
        str_concat(list, "protected ");
    if (field_has_flag(field, FIELD_ACC_STATIC))
        str_concat(list, "static ");
    if (field_has_flag(field, FIELD_ACC_VOLATILE))
        str_concat(list, "volatile ");
    if (field_has_flag(field, FIELD_ACC_TRANSIENT))
        str_concat(list, "transient ");
    if (field_has_flag(field, FIELD_ACC_SYNTHETIC))
        str_concat(list, "synthetic ");
}

void jvm_fields(jsource_file *jf)
{
    jf->fields = make_obj_arr(jd_field, jf->fields_count);
    jclass_file *jc = jf->jclass;
    u2 desc_index;
    for (int i = 0; i < jf->fields_count; ++i) {
        jfield *j_field = &jc->fields[i];
        jd_field *field = &jf->fields[i];
        field->meta = j_field;
        field->access_flags = be16toh(j_field->access_flags);

        desc_index = j_field->descriptor_index;
        jd_descriptor *descriptor = jvm_descriptor(jf, desc_index);
        field->type = lget_string(descriptor->list, 0);
        field->name = pool_str(jf->jclass, j_field->name_index);
        field->access_flags_fn = jvm_field_access_flag;
    }
}
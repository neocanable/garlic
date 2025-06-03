#include "debug.h"
#include "parser/class/class_tools.h"

jd_descriptor* jvm_descriptor(jsource_file *jf, u2 index)
{
    return hashmap_get_u2_to_object(jf->descs, index);
}

static bool descriptor_index_parsed(jclass_file *jc, u2 index)
{
    jd_descriptor *descriptor = jvm_descriptor(jc->jfile, index);
    return descriptor == NULL ? false : true;
}

static void add_descriptor_index(jclass_file *jc, u2 index)
{
    if (descriptor_index_parsed(jc, index))
        return;
    jd_descriptor *descriptor = make_obj(jd_descriptor);
    descriptor->index = index;
    descriptor->str = pool_str(jc, index);
    DEBUG_PRINT("[add_descriptor_index]: %d -> %s\n",
                index, descriptor->str);

    expand_descriptor(descriptor);
    ladd_obj(jc->jfile->descriptors, descriptor);
    hashmap_set_u2_to_object(jc->jfile->descs, index, descriptor);
}

void jvm_collect_descriptor(jsource_file *jf)
{
    jclass_file *jc = jf->jclass;
    for (int i = 0; i < be16toh(jc->fields_count); ++i) {
        jfield *field = &jc->fields[i];
        add_descriptor_index(jc, field->descriptor_index);
    }

    for (int i = 0; i < be16toh(jc->methods_count); ++i) {
        jmethod *method = &jc->methods[i];
        add_descriptor_index(jc, method->descriptor_index);
    }

    for (int i = 0; i < be16toh(jc->methods_count); ++i) {
        jmethod *method = &jc->methods[i];
        for (int j = 0; j < be16toh(method->attributes_count); ++j) {
            jattr *attr = &method->attributes[j];
            if (!STR_EQL(attr->name, "Code"))
                continue;
            jattr_code *code = (jattr_code*)attr->info;
            for (int k = 0; k < be16toh(code->attributes_count); ++k) {
                jattr *inner = &code->attributes[k];
                if (!STR_EQL(inner->name, "LocalVariableTable"))
                    continue;

                jattr_lvt *table = (jattr_lvt*)inner->info;
                int _length = be16toh(table->local_variable_table_length);
                for (int l = 0; l < _length; ++l) {
                    jattr_lv *variable = &table->local_variable_table[l];
                    add_descriptor_index(jc, variable->descriptor_index);
                }
            }
        }
    }

    for (int i = 0; i < be16toh(jc->constant_pool_count) - 1; ++i) {
        jcp_info *cp = &jc->constant_pool[i];

        switch (cp->tag) {
            case CONST_METHODTYPE_TAG: {
                jconst_method_type *info = cp->info->method_type;
                add_descriptor_index(jc, info->descriptor_index);
                break;
            }
            case CONST_NAMEANDTYPE_TAG: {
                jconst_name_and_type *info = cp->info->name_and_type;
                add_descriptor_index(jc, info->descriptor_index);
                break;
            }
        }
    }
}
#include "common/debug.h"
#include "common/str_tools.h"
#include "parser/class/class_tools.h"
#include "decompiler/klass.h"
#include "decompiler/descriptor.h"
#include "jvm/jvm_annotation.h"
#include <wchar.h>

static string element_value_to_s(jclass_file *jc, element_value *ev)
{
    element_value_union *uval = ev->union_value;
    switch (ev->tag) {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z':
        case 's': {
            /**
             * @FieldNameConstants(onlyExplicitlyIncluded = true)
             *      key: FieldNameConstants
             *      value: onlyExplicitlyIncluded = true
             **/
            jcp_info *cp_info = pool_item(jc, uval->const_value_index);
            string result = NULL;
            if (ev->tag == 's')
                result = str_create("\"%s\"", cp_info->readable);
            else
                result = str_create("%s", cp_info->readable);
            return result;
        }
        case 'e': {
            /**
             * @RolesAllowed(EnumBasedRole.Fields.ADMIN)
             *      key: RolesAllowed
             *      value: EnumBasedRole.Fields.ADMIN
             **/

            element_value_enum_const_value *ev = uval->enum_const_value;
            string _type_name = pool_str(jc, ev->type_name_index);
            string full_class_name = class_full_name(_type_name);
//            import_class(jc->jfile, fname);
            class_import(jc->jfile, full_class_name);
            string type_name = descriptor_to_s(_type_name);
            string const_name = pool_str(jc, ev->const_name_index);
            string result = str_create("%s.%s", type_name, const_name);
            return result;
        }
        case 'c':
            return str_dup(pool_str(jc,
                                    uval->class_info_index));
        case '@':
            return annotation_to_s(jc, uval->annotation_value);
        case '[': {
            // []
            str_list *list = str_list_init();
            element_value_array_value *array_value = uval->array_value;
            str_concat(list, "{");
            for (int i = 0; i < be16toh(array_value->num_values); ++i) {
                struct element_value *_ev = &array_value->values[i];
                string item = element_value_to_s(jc, _ev);

                if (i == be16toh(array_value->num_values) - 1) {
                    str_concat(list, item);
                    str_concat(list, "}");
                }
                else {
                    str_concat(list, item);
                    str_concat(list, ", ");
                }
            }
            return str_join(list);
        }
        default:
            return str_dup(g_str_unknown);
    }
}

static string element_pair_to_s(jclass_file *jc, annotation *ano, anno_evp *p)
{
    if (be16toh(ano->num_element_value_pairs) == 1) {
        string value = element_value_to_s(jc, p->value);
        string result = str_create("%s", value);
        DEBUG_ANNO_PRINT("[annotation p] value: %s %s\n",
                         value,
                         result);
        return result;
    }
    else {
        string key = pool_str(jc, p->element_name_index);
        string value = element_value_to_s(jc, p->value);
        string result = str_create("%s = %s", key, value);

        DEBUG_ANNO_PRINT("[annotation p] key: %s, value: %s %s\n",
                         key,
                         value,
                         result);
        return result;
    }
}

string annotation_to_s(jclass_file *jc, annotation *ano)
{
    string type_index_str = pool_str(jc, ano->type_index);
//    import_class(jc->jfile, class_full_name(type_index_str));
    class_import(jc->jfile, class_full_name(type_index_str));
    string type_name = descriptor_to_annotation_name(type_index_str);
    if (be16toh(ano->num_element_value_pairs) == 0)
        return type_name;

    size_t len = strlen(type_name) + 3; // type()
    size_t old_len = len;
    string result = x_alloc(len);
    snprintf(result, len, "%s(", type_name);
    for (int i = 0; i < be16toh(ano->num_element_value_pairs); ++i) {
        annotation_element_value_pairs *pair = &ano->element_value_pairs[i];
        string pair_str = element_pair_to_s(jc, ano, pair);
        old_len = len;
        len = old_len + strlen(pair_str) + 2; // key = value,[space]

        result = x_realloc(result, old_len, len);
        strcat(result, pair_str);
        if (i < be16toh(ano->num_element_value_pairs) - 1)
            strcat(result, ", ");
    }
    strcat(result, ")");
    result[len-1] = '\0';
    return result;
}

static void jvm_type_annotation_location(type_annotation *ta)
{
    type_annotation_target_info *ti = ta->target_info;
    switch (ta->target_type)
    {
        case 0x00: {
            // ClassFile
            type_parameter_target *t = ti->type_parameter_target;
            printf("[locate]:  %x %d\n",
                   ta->target_type, be16toh(t->type_parameter_index));
            break;
        }
        case 0x01: {
            // method_info
            type_parameter_target *t = ti->type_parameter_target;
            printf("[type_parameter_target]: %x %d\n",
                   ta->target_type, be16toh(t->type_parameter_index));
            break;
        }
        case 0x10: {
            // ClassFile
            supertype_target *s = ti->supertype_target;
            printf("[supertype_target]: %x %d\n",
                   ta->target_type, be16toh(s->supertype_index));
            break;
        }
        case 0x11: {
            // ClassFile
            type_parameter_bound_target *t = ti->type_parameter_bound_target;
            printf("[type_parameter_bound_target]: %x %d %d\n",
                   ta->target_type, t->type_parameter_index,
                   t->bound_index);
            break;
        }
        case 0x12: {
            // method_info
            type_parameter_bound_target *t = ti->type_parameter_bound_target;
            printf("[type_parameter_bound_target]: %x %d %d\n",
                   ta->target_type, t->type_parameter_index,
                   t->bound_index);
            break;
        }
        case 0x13: {
            // field_info
            empty_target *t = ti->empty_target;
            printf("[empty_target]: %x\n", ta->target_type);
            break;
        }
        case 0x14: {
            // method_info
            empty_target *t = ti->empty_target;
            printf("[empty_target]: %x\n", ta->target_type);
            break;
        }
        case 0x15: {
            empty_target *t = ti->empty_target;
            printf("[empty_target]: %x\n", ta->target_type);
            break;
        }
        case 0x16: {
            // method_info
            formal_parameter_target *t = ti->formal_parameter_target;
            printf("[formal_parameter_target]: %x %d\n",
                   ta->target_type, t->formal_parameter_index);
            break;
        }
        case 0x17: {
            // method_info
            throws_target *t = ti->throws_target;
            printf("[throws_target]: %x %d\n",
                   ta->target_type,
                   be16toh(t->throws_type_index));
            break;
        }
        case 0x40: {
            // method_info
            localvar_target *t = ti->localvar_target;
            printf("[localvar_target]: %x %d %d %d\n",
                   ta->target_type,
                   be16toh(t->table_length),
                   be16toh(t->table[0].start_pc),
                   be16toh(t->table[0].length));
            break;
        }
        case 0x41: {
            // method_info
            localvar_target *t = ti->localvar_target;
            printf("[localvar_target]: %x %d %d %d\n",
                   ta->target_type,
                   be16toh(t->table_length),
                   be16toh(t->table[0].start_pc),
                   be16toh(t->table[0].length));
            break;
        }
        case 0x42: {
            // method_info
            catch_target *t = ti->catch_target;
            printf("[catch_target]:  %x %d\n",
                   ta->target_type,
                   be16toh(t->exception_table_index));
            break;
        }
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46: {
            offset_target *t = ti->offset_target;
            printf("[offset_target]: %x %d\n",
                   ta->target_type,
                   be16toh(t->offset));
            break;
        }
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B: {
            type_argument_target *tt = ti->type_argument_target;
            printf("[type_argument_target]: %x %d %d %d\n",
                   ta->target_type,
                   be16toh(tt->offset),
                   be16toh(tt->offset),
                   tt->type_argument_index);
            break;
        }
        default:
            break;
    }
}

static void jvm_read_runtime_type_annotation(jclass_file *jc,
                                             type_annotation *ta)
{
    string type_str = pool_str(jc, ta->type_index);
    string type_name = descriptor_to_annotation_name(type_str);
    if (be16toh(ta->num_element_value_pairs) == 0) {
        printf("[type annotation]: %s\n", type_name);
        jvm_type_annotation_location(ta);
        return;
    }

    size_t len = strlen(type_name) + 3; // type()
    size_t old_len = len;
    string result = x_alloc(len);
    snprintf(result, len, "%s(", type_name);
    for (int i = 0; i < be16toh(ta->num_element_value_pairs); ++i) {
        annotation_element_value_pairs *pair = &ta->element_value_pairs[i];
        string pair_str = element_pair_to_s(jc, ta, pair);
        old_len = len;
        len = old_len + strlen(pair_str) + 2; // key = value,[space]

        result = x_realloc(result, old_len, len);
        strcat(result, pair_str);
        if (i < be16toh(ta->num_element_value_pairs) - 1)
            strcat(result, ", ");
    }
    strcat(result, ")");
    result[len-1] = '\0';
    DEBUG_ANNO_PRINT("[type annotation]: %s\n", result);
    jvm_type_annotation_location(ta);
}

static void jvm_read_annotation_attribute(jclass_file *jc, jattr *attr)
{
    if (STR_EQL(attr->name, "RuntimeVisibleAnnotations")) {
        jattr_ria *annotations = (jattr_ria*)attr->info;
        for (int j = 0; j < be16toh(annotations->num_annotations); ++j) {
            annotation *annotation = &annotations->annotations[j];
            string str = annotation_to_s(jc, annotation);
            DEBUG_ANNO_PRINT("[runtime visiable annotation]: %s\n", str);
        }
    }
    else if (STR_EQL(attr->name, "RuntimeInvisibleAnnotations")) {
        jattr_ria *annotations = (jattr_ria*)attr->info;
        for (int j = 0; j < be16toh(annotations->num_annotations); ++j) {
            annotation *annotation = &annotations->annotations[j];
            string str = annotation_to_s(jc, annotation);
            DEBUG_ANNO_PRINT("[runtime invisiable annotation]: %s\n", str);

        }
    }
    else if (STR_EQL(attr->name, "RuntimeVisibleParameterAnnotations")) {
        jattr_rvpa *annotations = (jattr_rvpa*)attr->info;

        for (int j = 0; j < annotations->num_parameters; ++j) {
            jattr_parameters_annotations *pa = &annotations->annotations[j];
            for (int k = 0; k < be16toh(pa->num_annotations); ++k) {
                annotation *annotation = &pa->annotations[k];
                string str = annotation_to_s(jc, annotation);
                DEBUG_ANNO_PRINT("[annotation]: %s\n", str);
            }
        }

    }
    else if (STR_EQL(attr->name, "RuntimeInvisibleParameterAnnotations")) {
        jattr_rvpa *annotations = (jattr_rvpa*)attr->info;
        for (int j = 0; j < annotations->num_parameters; ++j) {
            jattr_parameters_annotations *pa = &annotations->annotations[j];
            for (int k = 0; k < be16toh(pa->num_annotations); ++k) {
                annotation *annotation = &pa->annotations[k];
                string str = annotation_to_s(jc, annotation);
                DEBUG_ANNO_PRINT("[annotation]: %s\n", str);
            }
        }
    }
    else if (STR_EQL(attr->name, "RuntimeVisibleTypeAnnotations")) {
        jattr_rvta *annotations = (jattr_rvta*)attr->info;
        for (int j = 0; j < be16toh(annotations->num_annotations); ++j) {
            type_annotation *annotation = &annotations->annotations[j];
            jvm_read_runtime_type_annotation(jc, annotation);
        }
    }
    else if (STR_EQL(attr->name, "RuntimeInvisibleTypeAnnotations")) {
        jattr_rita *annotations = (jattr_rita*)attr->info;
        for (int j = 0; j < be16toh(annotations->num_annotations); ++j) {
            type_annotation *annotation = &annotations->annotations[j];
            jvm_read_runtime_type_annotation(jc, annotation);
        }
    }
}

void jvm_annotations(jsource_file *jf)
{
    jvm_class_annotations(jf);
    jvm_field_annotations(jf);

    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        jvm_method_annotation(m);
    }
}

void jvm_print_all_signatures(jsource_file *jf)
{
    jclass_file *jc = jf->jclass;

    for (int i = 0; i < be16toh(jc->attributes_count); ++i) {
        jattr *attr = &jc->attributes[i];
        if (STR_EQL(attr->name, "Signature")) {
            jattr_signature *s = (jattr_signature *) attr->info;
            printf("[class signature]: %s\n",
                   pool_str(jc, s->signature_index));
        }
    }

    for (int i = 0; i < be16toh(jc->fields_count); ++i) {
        jfield *f = &jc->fields[i];
        for (int j = 0; j < be16toh(f->attributes_count); ++j) {
            jattr *attr = &f->attributes[j];
            if (STR_EQL(attr->name, "Signature")) {
                jattr_signature *s = (jattr_signature *) attr->info;
                printf("[field signature]: %s\n",
                       pool_str(jc, s->signature_index));
            }
        }
    }

    for (int i = 0; i < be16toh(jc->methods_count); ++i) {
        jmethod *m = &jc->methods[i];
        for (int j = 0; j < be16toh(m->attributes_count); ++j) {
            jattr *attr = &m->attributes[j];
            if (STR_EQL(attr->name, "Signature")) {
                jattr_signature *s = (jattr_signature *) attr->info;
                printf("[m signature]: %s\n",
                       pool_str(jc, s->signature_index));
            }
        }
    }
}

string jvm_method_parameter_annotation(jd_method *m, int index)
{
    jmethod *j_method = m->meta_method;
    m->annotations = linit_object();
    str_list *list = str_list_init();

    for (int i = 0; i < be16toh(j_method->attributes_count); ++i) {
        jattr *attr = &j_method->attributes[i];

        if (!STR_EQL(attr->name, "RuntimeVisibleParameterAnnotations") &&
            !STR_EQL(attr->name, "RuntimeInvisibleParameterAnnotations"))
            continue;
        jattr_rvpa *annotations = (jattr_rvpa*)attr->info;
        for (int j = 0; j < annotations->num_parameters; ++j) {
            if (index != j)
                continue;
            jattr_parameters_annotations *pa = &annotations->annotations[j];
            for (int k = 0; k < be16toh(pa->num_annotations); ++k) {
                annotation *ano = &pa->annotations[k];
                string ano_str = annotation_to_s(m->meta, ano);
                str_concat(list, ano_str);
            }
        }
    }

    if (list->count > 0) {
        string result = str_join_with(list, " ");
        return result;
    }
    else {
        return NULL;
    }
}

void jvm_method_annotation(jd_method *m)
{
    DEBUG_PRINT("====== [Annotation For Method: %s] =======\n", m->name);

    jmethod *j_method = m->meta_method;
    m->annotations = linit_object();
    for (int i = 0; i < be16toh(j_method->attributes_count); ++i) {
        jattr *attr = &j_method->attributes[i];

        if (!STR_EQL(attr->name, "RuntimeVisibleAnnotations") &&
            !STR_EQL(attr->name, "RuntimeInvisibleAnnotations"))
            continue;

        jattr_ria *annotations = (jattr_ria*)attr->info;
        for (int j = 0; j < be16toh(annotations->num_annotations); ++j) {
            annotation *annotation = &annotations->annotations[j];
            jd_annotation *ano = make_obj(jd_annotation);
            ano->fname = str_dup(attr->name);
            ano->value = annotation;
            ano->str = annotation_to_s(m->meta, annotation);
            ladd_obj(m->annotations, ano);
        }
    }
}

void jvm_field_annotations(jsource_file *jf)
{
    DEBUG_PRINT("============ [Annotation For Field] =============\n");
    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];
        field->annotations = linit_object();
        jfield *meta = field->meta;
        for (int j = 0; j < be16toh(meta->attributes_count); ++j) {
            jattr *attr = &meta->attributes[j];
            if (!STR_EQL(attr->name, "RuntimeVisibleAnnotations") &&
                !STR_EQL(attr->name, "RuntimeInvisibleAnnotations"))
                continue;

            jattr_ria *annotations = (jattr_ria*)attr->info;
            for (int k = 0; k < be16toh(annotations->num_annotations); ++k) {
                annotation *annotation = &annotations->annotations[k];
                jd_annotation *ano = make_obj(jd_annotation);
                ano->fname = str_dup(attr->name);
                ano->value = annotation;
                ano->str = annotation_to_s(jf->jclass, annotation);
                ladd_obj(field->annotations, ano);
            }
        }
    }
}

void jvm_class_annotations(jsource_file *jf)
{
    DEBUG_ANNO_PRINT("========= [Annotation For Class] ==========\n");

    jf->annotations = linit_object();
    jclass_file *jc = jf->jclass;
    for (int i = 0; i < be16toh(jc->attributes_count); ++i) {
        jattr *attr = &jc->attributes[i];
        if (!STR_EQL(attr->name, "RuntimeVisibleAnnotations") &&
            !STR_EQL(attr->name, "RuntimeInvisibleAnnotations"))
            continue;

        jattr_ria *annotations = (jattr_ria*)attr->info;
        for (int j = 0; j < be16toh(annotations->num_annotations); ++j) {
            annotation *annotation = &annotations->annotations[j];
            jd_annotation *ano = make_obj(jd_annotation);
            ano->fname = str_dup(attr->name);
            ano->value = annotation;
            ano->str = annotation_to_s(jf->jclass, annotation);
            ladd_obj(jf->annotations, ano);
        }
    }
}

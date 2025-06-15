#include "dalvik/dex_annotation.h"
#include "dalvik/dex_class.h"
#include "decompiler/klass.h"
#include "debug.h"
#include "dex_method.h"

string encoded_value_to_s(jd_meta_dex *meta, encoded_value *ev)
{
    switch (ev->value_type) {
        case kDexAnnotationByte: {
            return str_create("%d", ev->value[0]);
        }
        case kDexAnnotationShort: {
            u2 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("%d", v);
        }
        case kDexAnnotationChar: {
            u2 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("%c", v);
        }
        case kDexAnnotationInt: {
            u4 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("%d", v);
        }
        case kDexAnnotationLong: {
            u8 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("%lld", v);
        }
        case kDexAnnotationFloat: {
            u4 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("%f", *(float *)&v);
        }
        case kDexAnnotationDouble: {
            u8 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("%lf", *(double *)&v);
        }
        case kDexAnnotationString: {
            u4 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            return str_create("\"%s\"", dex_str_of_idx(meta, v));
        }
        case kDexAnnotationType: {
            u4 v = 0;
            for (int i = 0; i < ev->value_length; ++i) {
                v = v | ev->value[i] << (i * 8);
            }
            string cname = dex_str_of_type_id(meta, v);
            string sname = class_simple_name(cname);
            return str_create("%s", sname);
        }
        case kDexAnnotationArray: {
            encoded_array *ea = (encoded_array *)ev->value;
            str_list *list = str_list_init();
            str_concat(list, "{");
            for (int i = 0; i < ea->size; ++i) {
                encoded_value *v = &ea->values[i];
                string item = encoded_value_to_s(meta, v);
                if (i == ea->size - 1) {
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
        case kDexAnnotationNull: {
            return str_create("null");
        }
        case kDexAnnotationBoolean: {
            return str_create("%s", ev->value_arg == 0 ? "false" : "true");
        }
        default:
            return str_create((string)g_str_unknown);
    }
}

string encoded_signature_to_s(jd_meta_dex *meta, encoded_annotation *en_anno)
{
    if (en_anno->size != 1)
        return NULL;
    encoded_value *ev = en_anno->elements[0].value;

    if (ev->value_type != kDexAnnotationArray)
        return NULL;

    encoded_array *ea = (encoded_array *)ev->value;
    str_list *list = str_list_init();
    for (int i = 0; i < ea->size; ++i) {
        encoded_value *v = &ea->values[i];
        if (v->value_type != kDexAnnotationString)
            return NULL;
        u4 id = 0;
        for (int j = 0; j < v->value_length; ++j) {
            id = id | v->value[j] << (j * 8);
        }
        string item = dex_str_of_idx(meta, id);
        str_concat(list, item);
    }
    return str_join(list);
}

string encoded_annotation_to_s(jd_meta_dex *meta, encoded_annotation *ea)
{
    str_list *list = str_list_init();
    string ano_fname = dex_str_of_type_id(meta, ea->type_idx);
    DEBUG_PRINT("type_idx: %d, %s\n", ea->type_idx, ano_fname);
    string ano_sname = class_simple_name(ano_fname);
    string ano_name = str_create("@%s", ano_sname);
    str_concat(list, ano_name);
    if (ea->size == 0) {
        return str_join(list);
    }
    else if (ea->size == 1) {
        str_concat(list, "(");
        annotation_element *element = &ea->elements[0];
        string name = dex_str_of_idx(meta, element->name_idx);
        if (STR_EQL(name, "value")) {
            string value = encoded_value_to_s(meta, element->value);
            str_concat(list, value);
        }
        else {
            string value = encoded_value_to_s(meta, element->value);
            str_concat(list, str_create("%s = %s", name, value));
        }
        str_concat(list, ")");
        return str_join(list);
    }
    else {
        str_concat(list, "(");
        for (int i = 0; i < ea->size; ++i) {
            annotation_element *element = &ea->elements[i];
            string name = dex_str_of_idx(meta, element->name_idx);
            string value = encoded_value_to_s(meta, element->value);
            if (i == ea->size - 1) {
                str_concat(list, str_create("%s = %s", name, value));
                str_concat(list, ")");
            }
            else
                str_concat(list, str_create("%s = %s, ", name, value));
        }
        return str_join(list);
    }
}

void dex_class_annotation(jsource_file *jf)
{
    dex_class_def *cf = jf->jclass;
    jd_meta_dex *meta = ((jd_dex*)jf->meta)->meta;
    jf->annotations = linit_object();
    dex_ano_dict_item *dict = cf->annotations;
    if (dict == NULL) {
        return;
    }

    annotation_set_item *item = dict->class_annotation;
    if (item == NULL) {
        return;
    }

    for (int i = 0; i < item->size; ++i) {
        annotation_off_item *off_item = &item->entries[i];
        annotation_item *aitem = off_item->annotation_item;
        encoded_annotation *ea = aitem->encoded_annotation;

        if (aitem->visibility == kDexVisibilitySystem) {
            string ea_str = dex_str_of_type_id(meta, ea->type_idx);
            if (STR_EQL(ea_str, "Ldalvik/annotation/Signature;")) {
                string sig = encoded_signature_to_s(meta, ea);
                jf->signature = sig;
                DEBUG_PRINT("class: %s signature: %s\n", jf->fname, sig);
            }
            continue;
        }

        string ano_str = encoded_annotation_to_s(meta, ea);
        jd_annotation *ano = make_obj(jd_annotation);
        ano->value = ea;
        ano->str = ano_str;
        string desc = dex_str_of_type_id(meta, ea->type_idx);
        ano->fname = class_full_name(desc);
        ladd_obj(jf->annotations, ano);
    }
}

void dex_field_annotation(jd_meta_dex *meta,
                          jd_field *field,
                          dex_class_def *cf)
{
    encoded_field *efield = field->meta;
    dex_ano_dict_item *dict = cf->annotations;

    field->annotations = linit_object();
    if (dict == NULL) {
        return;
    }

    if (dict->fields_size == 0) {
        return;
    }

    for (int i = 0; i < dict->fields_size; ++i) {
        field_annotation *fa = &dict->field_annotations[i];
        if (fa->field_idx != efield->field_id)
            continue;

        annotation_set_item *item = fa->annotation;
        if (item == NULL || item->size == 0)
            continue;

        for (int j = 0; j < item->size; ++j) {
            annotation_off_item *off_item = &item->entries[j];
            annotation_item *aitem = off_item->annotation_item;
            encoded_annotation *ea = aitem->encoded_annotation;
            if (aitem->visibility == kDexVisibilitySystem) {
                string ea_str = dex_str_of_type_id(meta, ea->type_idx);
                if (STR_EQL(ea_str, "Ldalvik/annotation/Signature;")) {
                    string sig = encoded_signature_to_s(meta, ea);
                    field->signature = sig;
                    DEBUG_PRINT("field: %s signature: %s\n", field->name, sig);
                }
                continue;
            }
            string ano_str = encoded_annotation_to_s(meta, ea);
            jd_annotation *ano = make_obj(jd_annotation);
            ano->value = ea;
            ano->str = ano_str;
            string desc = dex_str_of_type_id(meta, ea->type_idx);
            ano->fname = class_full_name(desc);
            ladd_obj(field->annotations, ano);
        }
    }

}

void dex_method_annotation(jd_method *m)
{
//    jd_dex *dex = m->meta;
    jd_meta_dex *meta = dex_method_meta(m);
    dex_class_def *cf = m->jfile->jclass;
    m->annotations = linit_object();
    encoded_method *em = m->meta_method;
    dex_ano_dict_item *dict = cf->annotations;
    if (dict == NULL) {
        return;
    }

    if (dict->methods_size == 0) {
        return;
    }

    for (int i = 0; i < dict->methods_size; ++i) {
        method_annotation *ma = &dict->method_annotations[i];
        if (ma->method_idx != em->method_id)
            continue;

        annotation_set_item *item = ma->annotation;
        if (item == NULL || item->size == 0)
            continue;

        for (int j = 0; j < item->size; ++j) {
            annotation_off_item *off_item = &item->entries[j];
            annotation_item *aitem = off_item->annotation_item;
            encoded_annotation *ea = aitem->encoded_annotation;
            if (aitem->visibility == kDexVisibilitySystem) {
                string ea_str = dex_str_of_type_id(meta, ea->type_idx);
                if (STR_EQL(ea_str, "Ldalvik/annotation/Signature;")) {
                    string sig = encoded_signature_to_s(meta, ea);
                    m->signature = sig;
                    DEBUG_PRINT("m: %s sig: %s\n", m->name, sig);
                }
                continue;
            }

            string ano_str = encoded_annotation_to_s(meta, ea);
            jd_annotation *ano = make_obj(jd_annotation);
            ano->value = ea;
            ano->str = ano_str;
            string desc = dex_str_of_type_id(meta, ea->type_idx);
            ano->fname = class_full_name(desc);
            ladd_obj(m->annotations, ano);
        }
    }
}

string dex_method_parameter_annotation(jd_method *m, int index)
{
    list_object *annotations = dex_parameter_annotation(m, index);
    if (is_list_empty(annotations))
        return NULL;
    str_list *list = str_list_init();
    for (int i = 0; i < annotations->size; ++i) {
        jd_annotation *ano = lget_obj(annotations, i);
        str_concat(list, ano->str);
        if (i != annotations->size - 1)
            str_concat(list, " ");
    }
    return str_join(list);
}

list_object* dex_parameter_annotation(jd_method *m, int index)
{
    jd_meta_dex *meta = dex_method_meta(m);
    dex_class_def *cf = m->jfile->jclass;
    encoded_method *em = m->meta_method;
    dex_ano_dict_item *dict = cf->annotations;
    if (dict == NULL || dict->parameters_size == 0)
        return NULL;

    for (int i = 0; i < dict->parameters_size; ++i) {
        parameter_annotation *pa = &dict->parameter_annotations[i];
        if (pa->method_idx != em->method_id)
            continue;

        annotation_set_ref_list *list = pa->annotation;
        if (list == NULL || list->size == 0)
            return NULL;

        for (int j = 0; j < list->size; ++j) {
            if (j != index)
                continue;
            annotation_set_ref_item *item = &list->list[j];
            annotation_set_item *set_item = item->annotation;
            if (set_item == NULL || set_item->size == 0)
                return NULL;

            list_object *annotations = linit_object();
            for (int k = 0; k < set_item->size; ++k) {
                annotation_off_item *off_item = &set_item->entries[k];
                annotation_item *aitem = off_item->annotation_item;
                if (aitem->visibility == kDexVisibilitySystem)
                    continue;

                encoded_annotation *ea = aitem->encoded_annotation;
                string ano_str = encoded_annotation_to_s(meta, ea);
                jd_annotation *ano = make_obj(jd_annotation);
                ano->value = ea;
                ano->str = ano_str;
                string desc = dex_str_of_type_id(meta, ea->type_idx);
                ano->fname = class_full_name(desc);
                ladd_obj(annotations, ano);
            }
            return annotations;
        }
    }
    return NULL;
}

#if DEBUG

void print_all_class_annotations(jd_meta_dex *meta)
{
    dex_header *header = meta->header;
    for (int i = 0; i < header->class_defs_size; ++i) {
        dex_class_def *class_def = &meta->class_defs[i];
        print_dex_class_annotations(meta, class_def);
    }
}

static void print_dex_class_field_annotation(jd_meta_dex *meta,
                                             string cname,
                                             dex_ano_dict_item *dict)
{
    if (dict->fields_size == 0)
        return;

    DEBUG_PRINT("%s annotations field: %d"
                "    m: %d "
                "    parameters: %d\n",
                cname,
                dict->fields_size,
                dict->methods_size,
                dict->parameters_size);

    for (int i = 0; i < dict->fields_size; ++i) {
        field_annotation *fa = &dict->field_annotations[i];
        string field_name = dex_str_of_field_name(meta, fa->field_idx);

        annotation_set_item *item = fa->annotation;
        if (item == NULL)
            continue;

        for (int j = 0; j < item->size; ++j) {
            annotation_off_item *off_item = &item->entries[j];
            annotation_item *aitem = off_item->annotation_item;

            if (aitem->visibility == kDexVisibilitySystem)
                continue;

            encoded_annotation *ea = aitem->encoded_annotation;
            string ano_str = encoded_annotation_to_s(meta, ea);
            printf("[field]: %s %s\n", field_name, ano_str);
        }
    }
}

static void print_dex_class_method_annotation(jd_meta_dex *meta,
                                              string cname,
                                              dex_ano_dict_item *dict)
{

    for (int i = 0; i < dict->methods_size; ++i) {
        method_annotation *ma = &dict->method_annotations[i];
        string method_name = dex_str_of_method_id(meta, ma->method_idx);
        annotation_set_item *item = ma->annotation;
        if (item == NULL)
            continue;

        for (int j = 0; j < item->size; ++j) {
            annotation_off_item *off_item = &item->entries[j];
            annotation_item *aitem = off_item->annotation_item;
            if (aitem->visibility == kDexVisibilitySystem)
                continue;

            encoded_annotation *ea = aitem->encoded_annotation;
            string ano_str = encoded_annotation_to_s(meta, ea);
            printf("[m encoded_annotation]: %s %s\n",
                   method_name, ano_str);
        }
    }
}

static void print_dex_class_parameter_annotation(jd_meta_dex *meta,
                                                 string cname,
                                                 dex_ano_dict_item *dict)
{
    for (int i = 0; i < dict->parameters_size; ++i) {
        parameter_annotation *pa = &dict->parameter_annotations[i];
        dex_method_id *method_id = &meta->method_ids[pa->method_idx];
        dex_proto_id *proto = &meta->proto_ids[method_id->proto_idx];
        dex_type_list *type_list = proto->type_list;
        string method_name = dex_str_of_method_id(meta, pa->method_idx);

        printf("%s(", method_name);
        for (int j = 0; j < type_list->size; ++j) {
            dex_type_item *ti = &type_list->list[j];
            string desc = dex_str_of_type_id(meta, ti->type_idx);
            if (j == type_list->size - 1)
                printf("%s", desc);
            else
                printf("%s,", desc);
        }
        printf(")\n");
        annotation_set_ref_list *list = pa->annotation;
        if (list == NULL)
            continue;

        for (int j = 0; j < list->size; ++j) {
            annotation_set_ref_item *item = &list->list[j];
            annotation_set_item *aitem = item->annotation;
            if (item->annotation == NULL)
                continue;
            for (int k = 0; k < aitem->size; ++k) {
                annotation_off_item *offitem = &aitem->entries[k];
                annotation_item *anno_item = offitem->annotation_item;
                if (anno_item->visibility == kDexVisibilitySystem)
                    continue;
                encoded_annotation *ea = anno_item->encoded_annotation;
                string ano_str = encoded_annotation_to_s(meta, ea);
                printf("\t[parameter encoded_annotation]: %d %s %s\n",
                        j, method_name, ano_str);
            }
        }
    }
}

static void print_dex_class_self_annotation(jd_meta_dex *meta,
                                            string cname,
                                            dex_ano_dict_item *dict)
{
    if (dict->class_annotation == NULL)
        return;

    for (int i = 0; i < dict->class_annotation->size; ++i) {
        annotation_off_item *item = &dict->class_annotation->entries[i];
        annotation_item *aitem = item->annotation_item;
        if (aitem->visibility == kDexVisibilitySystem)
            continue;
        encoded_annotation *ea = aitem->encoded_annotation;
        string ano_str = encoded_annotation_to_s(meta, ea);
        printf("[class encoded_annotation]: %s %s\n", cname, ano_str);
    }
}

void print_dex_class_annotations(jd_meta_dex *meta, dex_class_def *cdf)
{
    string cname = dex_str_of_type_id(meta, cdf->class_idx);
    dex_ano_dict_item *dict = cdf->annotations;

    if (dict == NULL)
        return;

    print_dex_class_field_annotation(meta, cname, dict);

    print_dex_class_method_annotation(meta, cname, dict);

    print_dex_class_parameter_annotation(meta, cname, dict);

    print_dex_class_self_annotation(meta, cname, dict);
}

#endif

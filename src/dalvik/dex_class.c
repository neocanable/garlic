#include "dalvik/dex_class.h"
#include "dalvik/dex_annotation.h"

#include "decompiler/klass.h"
#include "decompiler/method.h"
#include "decompiler/descriptor.h"
#include "decompiler/field.h"

bool dex_class_is_synthetic(jd_meta_dex *meta, dex_class_def *def) {
    if (def->class_data_off == 0) {
        return false;
    }

    dex_class_data_item *data = def->class_data;
    if (data == NULL) {
        return false;
    }

    string name = dex_str_of_type_id(meta, def->class_idx);
    return 
           (def->access_flags & ACC_DEX_SYNTHETIC) != 0 &&
           (def->access_flags & ACC_DEX_FINAL) != 0;
}

void dex_filed_access_flag(jd_field *field, str_list *list)
{
    if (field_has_flag(field, ACC_DEX_FINAL))
        str_concat(list, "final ");
    if (field_has_flag(field, ACC_DEX_PUBLIC))
        str_concat(list, "public ");
    if (field_has_flag(field, ACC_DEX_PRIVATE))
        str_concat(list, "private ");
    if (field_has_flag(field, ACC_DEX_PROTECTED))
        str_concat(list, "protected ");
    if (field_has_flag(field, ACC_DEX_STATIC))
        str_concat(list, "static ");
    if (field_has_flag(field, ACC_DEX_VOLATILE))
        str_concat(list, "volatile ");
    if (field_has_flag(field, ACC_DEX_SYNTHETIC))
        str_concat(list, "synthetic ");
}

void dex_class_access_flag(jsource_file *jf, str_list *list)
{
    dex_class_def *def = jf->jclass;

    if (access_flags_contains(def->access_flags, ACC_DEX_PUBLIC))
        str_concat(list, "public ");
    if (access_flags_contains(def->access_flags, ACC_DEX_FINAL))
        str_concat(list, "final ");
    if (access_flags_contains(def->access_flags, ACC_DEX_ABSTRACT) && !
            access_flags_contains(def->access_flags, ACC_DEX_INTERFACE))
        str_concat(list, "abstract ");
    if (access_flags_contains(def->access_flags, ACC_DEX_SYNTHETIC))
        str_concat(list, "synthetic ");

    if (access_flags_contains(def->access_flags, ACC_DEX_INTERFACE))
        str_concat(list, "interface ");
    else if (access_flags_contains(def->access_flags, ACC_DEX_ENUM))
        str_concat(list, "enum ");
    else
        str_concat(list, "class ");
}

void dex_class_annotations(jsource_file *jf)
{
    dex_class_def *cf = jf->jclass;
    jd_dex *dex = jf->meta;

    dex_class_annotation(jf);

    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];
        dex_field_annotation(dex->meta, field, cf);
    }
    if (is_list_empty(jf->methods))
        return;

    for (int i = 0; i < jf->methods->size; ++i)
        dex_method_annotation(lget_obj(jf->methods, i));
}

void dex_class_import(jsource_file *jf)
{
    for (int i = 0; i < jf->annotations->size; ++i) {
        jd_annotation *annotation = lget_obj(jf->annotations, i);
        class_import(jf, annotation->fname);
    }

    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *f = &jf->fields[i];
        for (int j = 0; j < f->annotations->size; ++j) {
            jd_annotation *annotation = lget_obj(f->annotations, j);
            class_import(jf, annotation->fname);
        }
    }
}

bool dex_class_is_inner_class(jd_meta_dex *meta, dex_class_def *cf) {
    string cname = dex_str_of_type_id(meta, cf->class_idx);
    string class_name = class_simple_name(cname);

    bool result = is_inner_class(class_name);

    if (!result) return result;

    dex_annotations_directory_item *annotations = cf->annotations;
    if (annotations == NULL)
        return result;
    annotation_set_item *set_item = annotations->class_annotation;
    if (set_item == NULL)
        return result;

    for (int i = 0; i < set_item->size; ++i) {
        annotation_item *item = &set_item->entries[i];
        u4 type_idx = item->encoded_annotation->type_idx;
        string type_name = dex_str_of_type_id(meta, type_idx);
        if (STR_EQL(type_name, "Ldalvik/annotation/InnerClass;")) {
            result = true;
            break;
        }
    }
    return result;
}

int dex_class_is_anonymous_class(jd_meta_dex *meta, dex_class_def *cf) {
    string cname = dex_str_of_type_id(meta, cf->class_idx);
    string class_name = class_simple_name(cname);
    bool result = is_anonymous_class(class_name);
    if (!result) return result;

    dex_annotations_directory_item *annotations = cf->annotations;
    if (annotations == NULL)
        return result;
    annotation_set_item *set_item = annotations->class_annotation;
    if (set_item == NULL)
        return result;

    for (int i = 0; i < set_item->size; ++i) {
        annotation_item *item = &set_item->entries[i];
        u4 type_idx = item->encoded_annotation->type_idx;
        string type_name = dex_str_of_type_id(meta, type_idx);
        if (STR_EQL(type_name, "Ldalvik/annotation/EnclosingClass;")) {
            result = true;
            break;
        }
    }

    return result;
}

static void dex_encoded_field_to_field(jd_dex *dex,
                                       encoded_field *efield,
                                       jd_field *field,
                                       bool instance)
{
    jd_meta_dex *meta = dex->meta;
    field->access_flags = efield->access_flags;
    field->meta = efield;
    field->name = dex_field_name(meta, efield);
    field->type = descriptor_to_s(dex_field_desc(meta, efield));
    field->access_flags_fn = dex_filed_access_flag;
    if (instance)
        field->defination = str_create("%s %s",
                                       field->type,
                                       field->name);
    else
        field->defination = str_create("static %s %s",
                                       field->type,
                                       field->name);
}

void dex_fields(jsource_file *jf)
{
    jd_dex *dex = jf->meta;
    dex_class_def *cf = jf->jclass;
    dex_class_data_item *data = cf->class_data;
    uint32_t size = data->static_fields_size + data->instance_fields_size;
    jf->fields_count = size;
    jf->fields = make_obj_arr(jd_field, size);

    encoded_field *efield;
    jd_field *field;

    for (int i = 0; i < data->static_fields_size; ++i) {
        efield = &data->static_fields[i];
        field = &jf->fields[i];
        dex_encoded_field_to_field(dex, efield, field, false);
    }

    for (int i = 0; i < data->instance_fields_size; ++i) {
        efield = &data->instance_fields[i];
        field = &jf->fields[data->static_fields_size + i];
        dex_encoded_field_to_field(dex, efield, field, true);
    }
}

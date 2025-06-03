#include "decompiler/expression_enum.h"
#include "decompiler/method.h"
#include "decompiler/field.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "decompiler/transformer/transformer.h"

static void optimize_enum_statics(jd_method *m)
{
    jclass_file *jc = m->meta;
    jd_node *root = lget_obj(m->nodes, 0);
    string method_class_name = jc->jfile->sname;
    jd_exp_enum *enum_exp = make_obj(jd_exp_enum);
    enum_exp->list = linit_object();
    jd_exp *last = NULL;
    for (int i = 0; i < root->children->size; ++i) {
        jd_node *node = lget_obj(root->children, i);
        if (node_is_not_expression(node))
            continue;

        jd_exp *exp = node->data;
        if (exp_is_nopped(exp) || !exp_is_put_static(exp))
            continue;

        jd_exp_put_static *put_static = exp->data;
        if (STR_EQL(put_static->name, "$VALUES") &&
            STR_EQL(put_static->class_name, method_class_name)) {
            exp_mark_nopped(exp);
            continue;
        }

        jd_exp *val_exp = &put_static->list->args[0];
        if (exp_is_initialize(val_exp)) {
            jd_exp_initialize *initialize = val_exp->data;
            if (STR_EQL(initialize->class_name, method_class_name)) {
                jd_exp_num_item *item = make_obj(jd_exp_num_item);
                item->list = initialize->list;
                item->name = put_static->name;

                ladd_obj(enum_exp->list, item);
                last = exp;
                exp_mark_nopped(exp);
            }
        }
    }
    if (last != NULL) {
        exp_mark_not_nopped(last);
        last->type = JD_EXPRESSION_ENUM;
        last->data = enum_exp;
    }

}

void optimize_enum_constructor(jd_method *m)
{
    if (!method_is_enum_constructor(m))
        return;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_invoke(exp)) {
            jd_exp_invoke *invoke = exp->data;
            if (STR_EQL(invoke->method_name, g_str_init))
                exp_mark_nopped(exp);
        }
    }
}

static void optimize_enum_methods(jd_method *m)
{
    jclass_file *jc = m->meta;
    jsource_file *jf = jc->jfile;

    if (!class_has_flag(jc, CLASS_ACC_ENUM))
        return;

    if (STR_EQL(m->name, "values")) {
        string name = class_simple_name(m->desc->str_return);
        string array_name = str_create("%s[]", jf->sname);

        if (STR_EQL(name, array_name))
            method_mark_hide(m);
    }
    else if (STR_EQL(m->name, "valueOf")) {
        string name = class_simple_name(m->desc->str_return);
        string cname = jf->sname;
        if (STR_EQL(name, cname))
            method_mark_hide(m);
    }
    else if (STR_EQL(m->name, "$values")) {
        string name = class_simple_name(m->desc->str_return);
        string array_name = str_create("%s[]", jf->sname);
        if (STR_EQL(name, array_name))
            method_mark_hide(m);
    }
}

static void optimize_enum_fields(jsource_file *jf)
{
    jclass_file *jc = jf->jclass;

    if (!class_has_flag(jc, CLASS_ACC_ENUM))
        return;

    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];

        if (STR_EQL(field->name, "$VALUES"))
            field_mark_hide(field);
        if (field_has_flag(field, FIELD_ACC_FINAL) &&
            field_has_flag(field, FIELD_ACC_STATIC) &&
            field_has_flag(field, FIELD_ACC_PUBLIC)) {
            string type = class_simple_name(field->type);
            if (STR_EQL(type, jf->sname))
                field_mark_hide(field);
        }
    }
}

void optimize_enum_class(jsource_file *jf)
{
    optimize_enum_fields(jf);

    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (method_is_empty(m) || method_is_unsupport(m))
            continue;
        optimize_enum_methods(m);
        optimize_enum_statics(m);
    }
}


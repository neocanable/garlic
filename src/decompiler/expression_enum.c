#include "decompiler/expression_enum.h"
#include "decompiler/method.h"
#include "decompiler/field.h"
#include "decompiler/descriptor.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "decompiler/expression_visitor.h"
#include "decompiler/transformer/transformer.h"

static jd_exp *clinit_constant_of(jd_method *m, jd_val *val)
{
    if (val == NULL)
        return NULL;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_is_store(exp))
            continue;

        jd_exp_store *store = exp->data;
        jd_exp *left = &store->list->args[0];
        if (!exp_is_local_variable(left))
            continue;
        if (left->data != val)
            continue;

        jd_exp *right = &store->list->args[1];
        if (exp_is_const(right))
            return right;
    }

    return NULL;
}

static bool enum_arguments_are_self_contained(jd_exp_initialize *initialize)
{
    for (int i = 2; i < initialize->list->len; ++i) {
        list_object *refs = linit_object();
        visit_expression_for_loop(&initialize->list->args[i], refs);
        for (int j = 0; j < refs->size; ++j) {
            jd_exp *ref = lget_obj(refs, j);
            if (ref->type == JD_EXPRESSION_STACK_VAR ||
                    ref->type == JD_EXPRESSION_LOCAL_VARIABLE)
                return false;
        }
    }
    return true;
}

static void drop_unused_clinit_locals(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp) || !exp_is_store(exp))
            continue;

        jd_exp_store *store = exp->data;
        jd_exp *left = &store->list->args[0];
        if (!exp_is_local_variable(left))
            continue;

        jd_val *val = left->data;
        bool used = false;
        for (int j = 0; j < m->expressions->size && !used; ++j) {
            if (j == i)
                continue;
            jd_exp *other = lget_obj(m->expressions, j);
            if (exp_is_nopped(other))
                continue;

            list_object *refs = linit_object();
            visit_expression_for_loop(other, refs);
            for (int k = 0; k < refs->size; ++k) {
                jd_exp *ref = lget_obj(refs, k);
                if (ref->type == JD_EXPRESSION_LOCAL_VARIABLE &&
                        ref->data == val) {
                    used = true;
                    break;
                }
            }
        }

        if (!used)
            exp_mark_nopped(exp);
    }
}

static void optimize_enum_statics(jd_method *m)
{
    jd_node *root = lget_obj(m->nodes, 0);
    jsource_file *jf = m->jfile;
    string method_class_name = jf->sname;
    jd_exp_enum *enum_exp = make_obj(jd_exp_enum);
    enum_exp->list = linit_object();
    list_object *dropped = linit_object();
    bool found = false;
    for (int i = 0; i < root->children->size; ++i) {
        jd_node *node = lget_obj(root->children, i);
        if (node_is_basic_block(node)) {
            for (int j = node->start_idx; j < node->end_idx; ++j) {
                jd_exp *exp = lget_obj(m->expressions, j);
                if (exp_is_nopped(exp) || !exp_is_put_static(exp))
                    continue;

                jd_exp_put_static *put_static = exp->data;
                if (STR_EQL(put_static->name, "$VALUES") &&
                    STR_EQL(put_static->class_name, method_class_name)) {
                    ladd_obj(dropped, exp);
                    continue;
                }

                jd_exp *val_exp = &put_static->list->args[0];
                if (!exp_is_initialize(val_exp))
                    continue;

                jd_exp_initialize *initialize = val_exp->data;
                if (!STR_EQL(initialize->class_name, method_class_name))
                    continue;

                for (int q = 2; q < initialize->list->len; ++q) {
                    jd_exp *arg = &initialize->list->args[q];
                    if (!exp_is_local_variable(arg))
                        continue;

                    jd_exp *constant = clinit_constant_of(m, arg->data);
                    if (constant != NULL)
                        *arg = *constant;
                }
                if (!enum_arguments_are_self_contained(initialize))
                    return;

                jd_exp_num_item *item = make_obj(jd_exp_num_item);
                item->list = initialize->list;
                item->name = put_static->name;

                ladd_obj(enum_exp->list, item);
                ladd_obj(dropped, exp);
                found = true;
            }
        }
    }

    if (!found)
        return;

    for (int i = 0; i < dropped->size; ++i)
        exp_mark_nopped(lget_obj(dropped, i));

    drop_unused_clinit_locals(m);

    jd_exp *constants = make_obj(jd_exp);
    constants->type = JD_EXPRESSION_ENUM;
    constants->data = enum_exp;
    jf->enum_constants = constants;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_is_nopped(exp) && !exp_is_empty(exp))
            return;
    }
    method_mark_hide(m);
}

void optimize_enum_constructor(jd_method *m)
{
    if (!method_is_enum_constructor(m))
        return;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_is_invoke(exp))
            continue;

        jd_exp_invoke *invoke = exp->data;
        if (STR_EQL(invoke->method_name, g_str_init) &&
                !invoke_is_this_constructor(invoke, exp))
            exp_mark_nopped(exp);
    }
}

static void optimize_enum_methods(jd_method *m)
{
    jsource_file *jf = m->jfile;

    if (!class_is_enum(jf))
        return;

    string ret = m->desc->str_return != NULL
                 ? class_simple_name_without_primitive(m->desc->str_return)
                 : NULL;

    if (STR_EQL(m->name, "values")) {
        string array_name = str_create("%s[]", jf->sname);

        if (ret != NULL && STR_EQL(ret, array_name))
            method_mark_hide(m);
    }
    else if (STR_EQL(m->name, "valueOf")) {
        if (ret != NULL && STR_EQL(ret, jf->sname))
            method_mark_hide(m);
    }
    else if (STR_EQL(m->name, "$values")) {
        string array_name = str_create("%s[]", jf->sname);
        if (ret != NULL && STR_EQL(ret, array_name))
            method_mark_hide(m);
    }
}

static void optimize_enum_fields(jsource_file *jf)
{
    if (!class_is_enum(jf))
        return;

    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];

        if (STR_EQL(field->name, "$VALUES"))
            field_mark_hide(field);
        if (field_has_flag(field, FIELD_ACC_FINAL) &&
            field_has_flag(field, FIELD_ACC_STATIC) &&
            field_has_flag(field, FIELD_ACC_PUBLIC)) {
            string type = class_simple_name_without_primitive(field->type);
            if (STR_EQL(type, jf->sname))
                field_mark_hide(field);
        }
    }
}

void optimize_enum_class(jsource_file *jf)
{
    if (!class_is_enum(jf))
        return;

    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (method_is_empty(m) || method_is_unsupport(m))
            continue;
        optimize_enum_statics(m);
    }

    if (jf->enum_constants == NULL)
        return;

    optimize_enum_fields(jf);

    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (method_is_empty(m) || method_is_unsupport(m))
            continue;
        optimize_enum_methods(m);
    }
}


#include "common/str_tools.h"
#include "decompiler/expression.h"
#include "jvm/jvm_expression_builder.h"

jd_exp_list* make_exp_list(int size)
{
    size_t exp_size = sizeof(jd_exp);
    jd_exp_list *list = make_obj(jd_exp_list);
    list->len = size;
    list->args = x_alloc(exp_size*list->len);
    return list;
}

void build_empty_expression(jd_exp *exp, jd_ins *ins)
{
    exp->type = JD_EXPRESSION_EMPTY;
    exp->ins = ins;
}

void nop_empty_expression(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        jd_ins *ins = exp->ins;
        jd_ins_fn *fn = NULL;
        if (ins != NULL)
            fn = ins->fn;

//        if (exp_is_empty(exp))
//            exp_mark_nopped(exp);
//        else if (exp->ins != NULL && ins_is_nopped(exp->ins))
//            exp_mark_nopped(exp);

        if (exp_is_empty(exp) || (ins != NULL && ins_is_nopped(ins)))
            exp_mark_nopped(exp);
    }
}

jd_exp* next_valid_exp(jd_method *m, int index)
{
    int i = index;
    while (i < m->expressions->size) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp) || exp_is_empty(exp)) {
            i++;
            continue;
        }
        return exp;
    }
    return NULL;
}

jd_exp* prev_valid_exp(jd_method *m, int index)
{
    int i = index;
    while (i >= 0) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp) || exp_is_empty(exp)) {
            i--;
            continue;
        }
        return exp;
    }
    return NULL;
}

jd_exp* exp_saved_value(jd_exp *e)
{
    jd_exp *value_exp = NULL;
    if (exp_is_store(e)) {
        jd_exp_store *store = e->data;
        value_exp = &store->list->args[1];
    }

    if (exp_is_put_static(e)) {
        jd_exp_put_static *put_static = e->data;
        value_exp = &put_static->list->args[0];
    }

    if (exp_is_put_field(e)) {
        jd_exp_put_field *put_field = e->data;
        value_exp = &put_field->list->args[0];
    }

    if (exp_is_array_store(e)) {
        jd_exp_array_store *array_store = e->data;
        value_exp = &array_store->list->args[0];
    }
    return value_exp;
}

jd_exp* exp_saved_left(jd_exp *e)
{
    jd_exp *left_exp = NULL;
    if (exp_is_store(e)) {
        jd_exp_store *store = e->data;
        left_exp = &store->list->args[0];
    }

    if (exp_is_put_field(e)) {
        jd_exp_put_field *put_field = e->data;
        jd_exp *putfield = &put_field->list->args[1];
        jd_exp_get_field *getfield = putfield->data;

        jd_exp *exp = make_obj(jd_exp);
        exp->type = JD_EXPRESSION_GET_FIELD;
        exp->data = getfield;
        left_exp = exp;
    }

    if (exp_is_array_store(e)) {
        jd_exp_array_store *array_store = e->data;

        jd_exp_array_load *array_load = make_obj(jd_exp_array_load);
        array_load->list = make_exp_list(2);
        jd_exp *index = &array_store->list->args[1];
        jd_exp *objref = &array_store->list->args[2];
        memcpy(&array_load->list->args[0], index, sizeof(jd_exp));
        memcpy(&array_load->list->args[1], objref, sizeof(jd_exp));

        jd_exp *exp = make_obj(jd_exp);
        exp->type = JD_EXPRESSION_ARRAY_LOAD;
        exp->data = array_load;
        left_exp = exp;
    }

    if (exp_is_put_static(e)) {
        jd_exp_put_static *put_static = e->data;

        jd_exp_get_static *getstatic = make_obj(jd_exp_get_static);
        getstatic->list = make_obj(jd_exp_list);
        getstatic->owner_class_name = put_static->owner_class_name;
        getstatic->class_name = put_static->class_name;
        getstatic->name = put_static->name;
        jd_exp *exp = make_obj(jd_exp);
        exp->type = JD_EXPRESSION_GET_STATIC;
        exp->data = getstatic;
        left_exp = exp;
    }

    return left_exp;
}

bool exp_saved_same(jd_exp *e1, jd_exp *e2)
{
    jd_exp *left1 = exp_saved_left(e1);
    jd_exp *left2 = exp_saved_left(e2);
    if (left1 == NULL || left2 == NULL || e1->type != e2->type)
        return false;

    jd_exp *e1_left = exp_saved_left(e1);
    jd_exp *e2_left = exp_saved_left(e2);

    if (exp_is_store(e1)) {
        jd_val *v1 = e1_left->data;
        jd_val *v2 = e2_left->data;
        if (v1->name == NULL || v2->name == NULL)
            return false;
        return STR_EQL(v1->name, v2->name);
    }
    else if (exp_is_put_field(e1)) {
        jd_exp_get_field *g1 = e1_left->data;
        jd_exp_get_field *g2 = e2_left->data;
        if (g1->name == NULL || g2->name == NULL)
            return false;
        return STR_EQL(g1->name, g2->name);
    }
    else if (exp_is_put_static(e1)) {
        jd_exp_get_static *g1 = e1_left->data;
        jd_exp_get_static *g2 = e2_left->data;
        if (g1->name == NULL || g2->name == NULL)
            return false;
        return STR_EQL(g1->name, g2->name);
    }
    else if (exp_is_array_store(e1)) {
        jd_exp_array_load *a1 = e1_left->data;
        jd_exp_array_load *a2 = e2_left->data;

        jd_exp *a1_index = &a1->list->args[1];
        jd_exp *a1_objref = &a1->list->args[2];
        jd_exp *a2_index = &a2->list->args[1];
        jd_exp *a2_objref = &a2->list->args[2];

        if (a1_index == a2_index && a1_objref == a2_objref)
            return true;

        return false;
    }
    return false;
}

extern bool if_expression_is_assert(jd_exp *e)
{
    jd_exp_if *if_exp = e->data;
    jd_exp *if_expression = if_exp->expression;

    if (exp_is_operator(if_expression)) {
        jd_exp_operator *if_operator = if_expression->data;
        jd_exp *if_op_left = &if_operator->list->args[0];
        jd_exp *if_op_right = &if_operator->list->args[1];
        if (exp_is_get_static(if_op_left)) {
            jd_exp_get_static *get_static_exp = if_op_left->data;
            if (STR_EQL(get_static_exp->name, "$assertionsDisabled"))
                return true;
        }
        if (exp_is_get_static(if_op_right)) {
            jd_exp_get_static *get_static_exp = if_op_right->data;
            if (STR_EQL(get_static_exp->name, "$assertionsDisabled"))
                return true;
        }
    }
    else if (exp_is_single_list(if_expression)) {
        jd_exp_single_list *single_list = if_expression->data;
        jd_exp_list *list = single_list->list;
        jd_exp *arg = &list->args[0];
        if (exp_is_get_static(arg)) {
            jd_exp_get_static *get_static_exp = arg->data;
            if (STR_EQL(get_static_exp->name, "$assertionsDisabled"))
                return true;
        }
    }
    else if (exp_is_single_operator(if_expression)) {
        jd_exp_single_operator *single_operator = if_expression->data;
        jd_exp *arg = &single_operator->list->args[0];
        if (exp_is_get_static(arg)) {
            jd_exp_get_static *get_static_exp = arg->data;
            if (STR_EQL(get_static_exp->name, "$assertionsDisabled"))
                return true;
        }
    }

    return false;
}

static bool expression_has_lambda(jd_method *m, jd_exp *expression);
static bool expression_has_lambda_inner(jd_method *m, int len, jd_exp *args)
{
    for (int i = 0; i < len; ++i) {
        jd_exp *arg = &args[i];
        if (exp_is_lambda(arg))
            return true;
        else
            expression_has_lambda(m, arg);
    }
    return false;
}

static bool expression_has_lambda(jd_method *m, jd_exp *expression)
{
    switch(expression->type) {
        case JD_EXPRESSION_ASSIGNMENT: {
            jd_exp_assignment *other_assignment = expression->data;
            jd_exp *other_right = other_assignment->right;
            return expression_has_lambda(m, other_right);
        }
        case JD_EXPRESSION_RETURN: {
            if (expression->data == NULL)
                return false;
            jd_exp_return *ret = expression->data;
            return expression_has_lambda_inner(m,
                                               ret->list->len,
                                               ret->list->args);
        }
        case JD_EXPRESSION_IF: {
            jd_exp_if *if_exp = expression->data;
            jd_exp *condition = if_exp->expression;
            jd_exp_operator *operator_exp = condition->data;
            return expression_has_lambda_inner(m,
                                               operator_exp->list->len,
                                               operator_exp->list->args);
        }
        case JD_EXPRESSION_COMPARE: 
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_GET_FIELD:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH: {
            jd_exp_reader *reader_exp = expression->data;
            jd_exp_list *list = reader_exp->list;
            return expression_has_lambda_inner(m,
                                               list->len,
                                               list->args);
        }
        default:
            return 0;
    }
}

#include "common/str_tools.h"
#include "decompiler/expression_loop_type.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "decompiler/expression_visitor.h"
#include "jvm/jvm_expression_builder.h"

static jd_exp* loop_condition_exp(jd_method *m, jd_loop *loop)
{
    jd_range *range = loop->condition_range;
    jd_exp *loop_exp = get_exp(m, range->end_idx);
    if (!exp_is_if(loop_exp))
        return NULL;
    jd_exp_if *if_exp = loop_exp->data;
    jd_exp *condition = if_exp->expression;
    return condition;
}

static jd_exp* loop_last_valid_exp(jd_method *m, jd_node *loop)
{
    jd_exp *exp = get_exp(m, loop->end_idx);
    if (exp_is_nopped(exp))
        return prev_valid_exp(m, loop->end_idx - 1);
    return exp;
}

static bool loop_condition_contains(list_object *exps, jd_exp *exp)
{
    switch(exp->type) {
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_STORE:
            break;
        default:
            return false;
    }
    for (int i = 0; i < exps->size; ++i) {
        jd_exp *e = lget_obj(exps, i);
        if (exp_is_local_variable(e) &&
            exp_is_store(exp)) {
            jd_val *val = e->data;
            jd_exp_store *store = exp->data;
            jd_exp *l = &store->list->args[0];
            jd_val *store_val = l->data;
            if (STR_EQL(val->name, store_val->name))
                return true;
        }
        else if (exp_is_get_field(e) &&
                 exp_is_put_field(exp)) {
            jd_exp_get_field *get_field = e->data;
            jd_exp_put_field *put_field = exp->data;

            if (STR_EQL(get_field->name, put_field->name) &&
                STR_EQL(get_field->class_name, put_field->class_name))
                return true;
        }
        else if (exp_is_get_static(e) &&
                 exp_is_put_static(exp)) {
            jd_exp_get_static *get = e->data;
            jd_exp_put_static *put = exp->data;

            if (STR_EQL(get->name, put->name) &&
                STR_EQL(get->class_name, put->class_name) &&
                STR_EQL(get->owner_class_name, put->owner_class_name))
                return true;

        }
    }
    return false;
}

static void loop_to_infinite_structure(jd_method *m, jd_loop *loop)
{
    if (loop->condition_range != NULL)
        return;
    loop->type = JD_LOOP_INFINITE;
}

static void loop_to_while_structure(jd_method *m, jd_loop *loop)
{
    if (!loop->can_write_condition || loop->is_post_condition)
        return;

    jd_exp *exp = get_exp(m, loop->condition_range->end_idx);
//    assert(exp_is_if(exp));
    if (!exp_is_if(exp))
        return;

    jd_exp_if *if_exp = exp->data;
    jd_exp *condition = if_exp->expression;

    jd_exp_while *exp_while = make_obj(jd_exp_while);
    exp_while->list = make_exp_list(1);
    memcpy(&exp_while->list->args[0], condition, sizeof(jd_exp));
    exp_while->start_idx = loop->start_idx;
    exp_while->end_idx = loop->end_idx;
    exp_while->offset = if_exp->offset;

    exp->type = JD_EXPRESSION_WHILE;
    exp->data = exp_while;

    loop->type = JD_LOOP_WHILE;
}

static void loop_to_do_while_structure(jd_method *m, jd_loop *loop)
{
    if (!loop->is_post_condition || !loop->can_write_condition)
        return;

    jd_exp *exp = get_exp(m, loop->condition_range->end_idx);
    if(!exp_is_if(exp))
        return;
    jd_exp_if *if_exp = exp->data;
    jd_exp *condition = if_exp->expression;

    jd_exp_do_while *do_while = make_obj(jd_exp_while);
    do_while->list = make_exp_list(1);
    memcpy(&do_while->list->args[0], condition, sizeof(jd_exp));
    do_while->start_idx = loop->start_idx;
    do_while->end_idx = loop->end_idx;
    do_while->offset = if_exp->offset;

    exp->type = JD_EXPRESSION_DO_WHILE;
    exp->data = do_while;

    loop->type = JD_LOOP_DO_WHILE;
}

static void loop_to_for_structure(jd_method *m, jd_loop *loop)
{
    if (!loop->can_write_condition || loop->is_post_condition)
        return;
    jd_node *loop_node = loop->node;

    jd_exp *loop_exp = get_exp(m, loop->condition_range->end_idx);
    jd_exp *condition_exp = loop_condition_exp(m, loop);
    if (condition_exp == NULL)
        return;
    // 在condition_exp里面找对应的store/getfield/getstatic

    list_object *exps = get_expression_for_loop(condition_exp);
    if (exps->size == 0)
        return;

    jd_exp *initial_exp = NULL;
    jd_node *start_node = loop_exp->block->node;

    jd_exp *pre_exp = get_exp(m, start_node->start_idx - 1);
    if (pre_exp == NULL)
        return;
    jd_node *pre_node = pre_exp->block->node;

    for (int i = pre_node->end_idx; i >= pre_node->start_idx ; --i) {
        jd_exp *e = get_exp(m, i);
        DEBUG_PRINT("[loop exp]: id: %d type: %d\n", e->idx, e->type);
        if (exp_is_nopped(e))
            continue;
        if (loop_condition_contains(exps, e)) {
            initial_exp = e;
            break;
        }
    }

    if (initial_exp == NULL)
        return;
    DEBUG_PRINT("[for loop]: found initial exp: %d\n",
                initial_exp->idx);

    jd_exp *last_exp = loop_last_valid_exp(m, loop_node);
    if (last_exp == NULL)
        return;
    jd_exp *last_continue = NULL;

    if (exp_is_continue(last_exp)) {
        last_continue = last_exp;
        last_exp = prev_valid_exp(m, last_exp->idx - 1);
    }

    jd_exp *increament_exp = NULL;
    if (exp_is_iinc(last_exp) ||
        exp_is_operator(last_exp) ||
        exp_is_put_static(last_exp) ||
        exp_is_put_field(last_exp))
        increament_exp = last_exp;

    if (increament_exp == NULL)
        return;

    if (last_continue != NULL)
        exp_mark_nopped(last_continue);

    jd_exp_for *for_exp = make_obj(jd_exp_for);
    for_exp->list = make_exp_list(3);
    memcpy(&for_exp->list->args[0], initial_exp, sizeof(jd_exp));
    memcpy(&for_exp->list->args[1], condition_exp, sizeof(jd_exp));
    memcpy(&for_exp->list->args[2], increament_exp, sizeof(jd_exp));

    loop_exp->type = JD_EXPRESSION_FOR;
    loop_exp->data = for_exp;
    for_exp->start_idx = loop->start_idx;
    for_exp->end_idx = loop->end_idx;

    exp_mark_nopped(initial_exp);
    exp_mark_nopped(increament_exp);

    loop->type = JD_LOOP_FOR;
}

void identify_loop_type(jd_method *m)
{
    for (int i = 0; i < m->loops->size; ++i) {
        jd_loop *loop = lget_obj(m->loops, i);
        loop_to_for_structure(m, loop);

        if (loop->type == JD_LOOP_UNKNOWN)
            loop_to_do_while_structure(m, loop);

        if (loop->type == JD_LOOP_UNKNOWN)
            loop_to_while_structure(m, loop);

        if (loop->type == JD_LOOP_UNKNOWN)
            loop_to_infinite_structure(m, loop);
    }
}
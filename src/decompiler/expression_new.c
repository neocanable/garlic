#include "decompiler/expression_new.h"
#include "decompiler/expression.h"
#include "jvm/jvm_ins.h"
#include "decompiler/klass.h"

static bool exp_contains_new_object(jd_exp *exp)
{
    if (exp_is_nopped(exp))
        return false;
    jd_ins *ins = exp->ins;
    if (ins != NULL && jvm_ins_is_newobj(ins))
        return true;
    if (exp_is_store(exp)) {
        jd_exp_store *store = exp->data;
        jd_exp *right = &store->list->args[1];
        return exp_is_uninitialize(right);
    }
    return false;
}

static bool identify_initialize_of_assignment(jd_method *m, jd_exp *exp, int i)
{
    bool found = false;

    jd_exp_assignment *new_obj_assignment = exp->data;
    jd_exp_lvalue *new_obj_lvalue = new_obj_assignment->left->data;
    jd_var *left_var = new_obj_lvalue->stack_var;

    for (int j = i + 1; j < m->expressions->size; ++j) {
        jd_exp *invoke_exp = lget_obj(m->expressions, j);
        if (exp_is_nopped(invoke_exp))
            continue;
        jd_ins *invoke_ins = invoke_exp->ins;
        if (invoke_ins == NULL)
            continue;
        jd_ins_fn *fn = invoke_ins->fn;
        if (!fn->is_invoke_special(invoke_ins))
            continue;

        if (!exp_is_invoke(invoke_exp))
            continue;

        jd_exp_invoke *invoke = invoke_exp->data;
        jd_exp *first_arg = &invoke->list->args[invoke->list->len - 1];
        if (!exp_is_stack_var(first_arg))
            continue;
        jd_var *first_arg_var = first_arg->data;
        if (first_arg_var != left_var)
            break;

        jd_exp_initialize *initialize = make_obj(jd_exp_initialize);
        initialize->list = make_obj(jd_exp_list);
        initialize->list->len = invoke->list->len - 1;
        initialize->class_name = left_var->cname;
        if (initialize->list->len > 0) {
            size_t size = sizeof(jd_exp) * initialize->list->len;
            initialize->list->args = x_alloc(size);
            memcpy(initialize->list->args,
                   invoke->list->args,
                   size);
        }
        invoke_exp->data = initialize;
        invoke_exp->type = JD_EXPRESSION_INITIALIZE;
        left_var->use_count --;
        left_var->def_count --;
        left_var->dupped_count --;

        jd_exp *right = new_obj_assignment->right;
        right->type = JD_EXPRESSION_INITIALIZE;
        right->data = initialize;

        if (invoke->anonymous != NULL) {
            right->type = JD_EXPRESSION_ANONYMOUS;
            right->data = invoke->anonymous;
        }

        invoke_exp->type = JD_EXPRESSION_ASSIGNMENT;
        invoke_exp->data = new_obj_assignment;

        new_obj_assignment->dupped_count --;
        new_obj_assignment->def_count --;
        exp_mark_nopped(exp);
        found = true;
        break;
    }
    return found;
}

static bool identify_initialize_of_store(jd_method *m, jd_exp *exp, int i)
{
    bool found = false;
    jd_exp_store *store = exp->data;
    jd_exp *left = &store->list->args[0];
    jd_exp *right = &store->list->args[1];
    jd_val *left_val = left->data;

    for (int j = i + 1; j < m->expressions->size; ++j) {
        jd_exp *invoke_exp = lget_obj(m->expressions, j);
        if (exp_is_nopped(invoke_exp) || !exp_is_invoke(invoke_exp))
            continue;
        jd_ins *invoke_ins = invoke_exp->ins;
        if (invoke_ins == NULL)
            continue;
        jd_ins_fn *fn = invoke_ins->fn;
        if (!fn->is_invoke_special(invoke_ins))
            continue;
        jd_exp_invoke *invoke = invoke_exp->data;
        jd_exp *first_arg = &invoke->list->args[invoke->list->len - 1];
        if (!exp_is_local_variable(first_arg))
            continue;
        jd_val *first_arg_val = first_arg->data;
        if (first_arg_val->name != NULL && left_val->name != NULL &&
            !STR_EQL(first_arg_val->name, left_val->name))
            break;

        jd_exp_initialize *initialize = make_obj(jd_exp_initialize);
        initialize->list = make_obj(jd_exp_list);
        initialize->list->len = invoke->list->len - 1;
//        string _name = class_simple_name(left_val->data->cname);
        initialize->class_name = left_val->data->cname;
//        initialize->desc = left_val->data->desc;
        if (initialize->list->len > 0) {
            size_t size = sizeof(jd_exp) * initialize->list->len;
            initialize->list->args = x_alloc(size);
            memcpy(initialize->list->args,
                   invoke->list->args,
                   size);
        }
        invoke_exp->data = initialize;
        invoke_exp->type = JD_EXPRESSION_INITIALIZE;

        // alloc 和 invoke_direct 是原子性的
        jd_val *val = left->data;
        jd_var *var = val->stack_var;
        if (var != NULL)
            var->use_count --;

        if (invoke->lambda != NULL) {
            right->type = JD_EXPRESSION_LAMBDA;
            right->data = invoke->lambda;
        }
        else if (invoke->anonymous != NULL) {
            right->type = JD_EXPRESSION_ANONYMOUS;
            right->data = invoke->anonymous;
        }
        else {
            right->type = JD_EXPRESSION_INITIALIZE;
            right->data = initialize;
        }

        exp_mark_nopped(invoke_exp);
        found = true;
        break;
    }
    return found;
}

bool identify_initialize(jd_method *m)
{
    bool found = false;
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_contains_new_object(exp))
            continue;

        if (exp_is_assignment(exp))
            found = identify_initialize_of_assignment(m, exp, i);
        else if (exp_is_store(exp))
            found = identify_initialize_of_store(m, exp, i);

    }
    return found;
}

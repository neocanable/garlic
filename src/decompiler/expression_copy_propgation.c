#include "expression_copy_propgation.h"
#include "expression.h"
#include "control_flow.h"

static bool do_copy_propagation(jd_exp *expression, jd_exp *other);

static inline bool store_exp_can_copy(jd_exp *exp)
{
    assert(exp_is_store(exp));

    jd_exp_store *store = exp->data;
    jd_exp *left = &store->list->args[0];
    jd_val *val = left->data;
    jd_var *var = val->stack_var;
    return var->def_count == 1 && var->use_count == 1;
}

static inline bool duped_store_exp_can_copy(jd_exp *exp)
{
    assert(exp_is_store(exp));

    jd_exp_store *store = exp->data;
    jd_exp *left = &store->list->args[0];
    jd_val *val = left->data;
    jd_var *var = val->stack_var;
    if (var == NULL)
        return false;

    return var->def_count == var->use_count;
//    return var->def_count == 1 && var->use_count == 1;
}

static bool replace_expression_with_local_variable(jd_exp *expression,
                                           int args_len,
                                           jd_exp *args)
{
    if (expression == NULL) {
        DEBUG_PRINT("[replace_expression_with_local_variable] expression is NULL\n");
        return false;
    }
    
    if (args == NULL) {
        DEBUG_PRINT("[replace_expression_with_local_variable] args is NULL\n");
        return false;
    }
    
    if (expression->data == NULL) {
        DEBUG_PRINT("[replace_expression_with_local_variable] expression->data is NULL\n");
        return false;
    }
    
    bool replaced = false;
    jd_exp_store *store = expression->data;
    if (store->list == NULL) {
        DEBUG_PRINT("[replace_expression_with_local_variable] store->list is NULL\n");
        return false;
    }
    
    jd_exp *left = &store->list->args[0];
    if (left == NULL || left->data == NULL) {
        DEBUG_PRINT("[replace_expression_with_local_variable] left or left->data is NULL\n");
        return false;
    }
    
    jd_val *val = left->data;
    jd_var *var = val->stack_var;
    jd_exp *right = &store->list->args[1];

    for (int i = 0; i < args_len; ++i) {
        jd_exp *arg = &args[i];
        if (arg == NULL) {
            DEBUG_PRINT("[replace_expression_with_local_variable] arg[%d] is NULL\n", i);
            continue;
        }
        
        // Check if the expression is properly initialized
        if (arg->type == 0) {
            DEBUG_PRINT("[replace_expression_with_local_variable] arg[%d] has invalid type: %d\n", i, arg->type);
            continue;
        }
        
        if (exp_is_local_variable(arg)) {
            if (arg->data == NULL) {
                DEBUG_PRINT("[replace_expression_with_local_variable] arg->data is NULL\n");
                continue;
            }
            jd_val *other_val = arg->data;
            jd_var *other_var = other_val->stack_var;
            if (var == other_var) {
                arg->type = right->type;
                arg->ins = right->ins;
                arg->data = right->data;
                if (var != NULL) {
                    var->use_count --;
                    if (var->redef_count >= 0)
                        var->redef_count --;
                    if (var->dupped_count > 0)
                        var->dupped_count --;
                }
                replaced = true;
            }
        }
        else {
            replaced |= do_copy_propagation(expression, arg);
        }
    }

    if (replaced && var != NULL && var->use_count == 0)
        exp_mark_nopped(expression);
    return replaced;
}

static bool do_copy_propagation(jd_exp *expression, jd_exp *other)
{
    if (expression == NULL || other == NULL) {
        DEBUG_PRINT("[do_copy_propagation] expression or other is NULL\n");
        return false;
    }
    
    if (other->data == NULL) {
        DEBUG_PRINT("[do_copy_propagation] other->data is NULL\n");
        return false;
    }
    
    switch(other->type) {
        case JD_EXPRESSION_ASSIGNMENT: {
            jd_exp_assignment *other_assignment = other->data;
            if (other_assignment == NULL) {
                DEBUG_PRINT("[do_copy_propagation] other_assignment is NULL\n");
                return false;
            }
            jd_exp *o = other_assignment->right;
            return do_copy_propagation(expression, o);
        }
        case JD_EXPRESSION_IF: {
            jd_exp_if *if_exp = other->data;
            if (if_exp == NULL) {
                DEBUG_PRINT("[do_copy_propagation] if_exp is NULL\n");
                return false;
            }
            jd_exp *exp = if_exp->expression;
            return do_copy_propagation(expression, exp);
        }
        case JD_EXPRESSION_STORE: {
            jd_exp_store *store = other->data;
            if (store == NULL || store->list == NULL) {
                DEBUG_PRINT("[do_copy_propagation] store or store->list is NULL\n");
                return false;
            }
            jd_exp *right = &store->list->args[1];
            return do_copy_propagation(expression, right);
        }
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_ANONYMOUS:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_GET_FIELD:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_MONITOR_ENTER:
        case JD_EXPRESSION_MONITOR_EXIT:
        case JD_EXPRESSION_STRING_CONCAT: {
            jd_exp_reader *reader_exp = other->data;
            if (reader_exp == NULL) {
                DEBUG_PRINT("[do_copy_propagation] reader_exp is NULL\n");
                return false;
            }
            jd_exp_list *list = reader_exp->list;
            if (list == NULL) {
                DEBUG_PRINT("[do_copy_propagation] list is NULL\n");
                return false;
            }
            if (list->args == NULL) {
                DEBUG_PRINT("[do_copy_propagation] list->args is NULL\n");
                return false;
            }
            return replace_expression_with_local_variable(expression,
                                                  list->len, list->args);
        }
        default:
            return 0;
    }
}

bool copy_propagation_of_expression(jd_method *m)
{
    bool result = false;
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (!node_is_basic_block(node))
            continue;

        for (int k = node->start_idx; k <= node->end_idx; ++k) {
            jd_exp *exp = lget_obj(m->expressions, k);
            if (exp_is_nopped(exp) ||
                !exp_is_store(exp) ||
                !store_exp_can_copy(exp))
                continue;

            for (int l = k + 1; l <= node->end_idx; ++l) {
                jd_exp *other = lget_obj(m->expressions, l);
                if (exp_is_nopped(other))
                    continue;

                result |= do_copy_propagation(exp, other);
            }
        }
    }
    return result;
}


bool copy_propagation_of_dup_local_variable(jd_method *m)
{
    bool result = false;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp) || !exp_is_store(exp))
            continue;
        jd_exp_store *store = exp->data;
        jd_exp *right = &store->list->args[1];
        if (!exp_is_local_variable(right) &&
            !exp_is_get_static(right) &&
            !exp_is_get_field(right) &&
            !exp_is_arraylength(right))
            continue;

        if (!exp_is_local_variable(right) && !duped_store_exp_can_copy(exp))
            continue;

        bool tmp = false;
        for (int j = i+1; j < m->expressions->size; ++j) {
            jd_exp *other = lget_obj(m->expressions, j);
            if (exp_is_nopped(other))
                continue;

            tmp |= do_copy_propagation(exp, other);
        }

        if (tmp) {
            exp_mark_nopped(exp);
        }
    }

    return result;
}
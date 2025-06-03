#include "common/str_tools.h"
#include "decompiler/expression_assign.h"
#include "decompiler/expression.h"

/*
 * usage for example:
 * local variable a = 1;
 *      a = a + 1 => a += 1
 *      a = a - 1 => a -= 1
 * field variable obj.a = 1;
 *      obj.a = obj.a + 1 => obj.a += 1
 * static variable A.a = 1;
 *      A.a = A.a + 1 => A.a += 1
 * */

static bool local_variable_expression_cmp(jd_exp *e1, jd_exp *e2)
{
    if (exp_is_local_variable(e1) && exp_is_local_variable(e2)) {
        jd_val *v1 = e1->data;
        jd_val *v2 = e2->data;
        if (v1->name == NULL || v2->name == NULL)
            return false;
        return STR_EQL(v1->name, v2->name);
    }
    return false;
}

static bool get_field_expression_cmp(jd_exp *e1, jd_exp *e2)
{
    if (!exp_is_get_field(e1) || exp_is_get_field(e2))
        return false;
    jd_exp_get_field *get_field1 = e1->data;
    jd_exp_get_field *get_field2 = e2->data;
    if (get_field1->name == NULL || get_field2->name == NULL ||
        get_field1->class_name == NULL || get_field2->class_name == NULL)
        return false;
    if (!STR_EQL(get_field1->name, get_field2->name) ||
        !STR_EQL(get_field1->class_name, get_field2->class_name))
        return false;
    jd_exp *g1 = &get_field1->list->args[0];
    jd_exp *g2 = &get_field2->list->args[0];
    return local_variable_expression_cmp(g1, g2);
}

static bool static_exp_cmp(jd_exp_put_static *e1, jd_exp_get_static *e2)
{
    if (e1->name == NULL || e2->name == NULL ||
        e1->class_name == NULL || e2->class_name == NULL)
        return false;

    if (!STR_EQL(e1->name, e2->name) ||
        !STR_EQL(e1->class_name, e2->class_name))
        return false;
    return true;
}

static jd_operator to_operator(jd_operator op)
{
    switch (op) {
        case JD_OP_ADD:
            return JD_OP_ADD_ASSIGN;
        case JD_OP_SUB:
            return JD_OP_SUB_ASSIGN;
        case JD_OP_MUL:
            return JD_OP_MUL_ASSIGN;
        case JD_OP_DIV:
            return JD_OP_DIV_ASSIGN;
        case JD_OP_REM:
            return JD_OP_REM_ASSIGN;
        case JD_OP_AND:
            return JD_OP_AND_ASSIGN;
        case JD_OP_OR:
            return JD_OP_OR_ASSIGN;
        case JD_OP_XOR:
            return JD_OP_XOR_ASSIGN;
        case JD_OP_SHL:
            return JD_OP_LSHIFT_ASSIGN;
        case JD_OP_SHR:
            return JD_OP_RSHIFT_ASSIGN;
        case JD_OP_USHR:
            return JD_OP_USHIFT_ASSIGN;
        default:
            return JD_OP_UNKNOWN;
    }
}

void identify_assignment(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp) ||
            !(exp_is_put_static(exp) || 
             exp_is_put_field(exp) || 
             exp_is_store(exp))) 
            continue;

        switch(exp->type) {
            case JD_EXPRESSION_STORE: {
                jd_exp_store *exp_store = exp->data;
                jd_exp *e1 = &exp_store->list->args[0];
                jd_exp *e2 = &exp_store->list->args[1];
                if (!exp_is_operator(e2))
                    continue;
                jd_exp_operator *op_exp = e2->data;
                if (op_exp->list->len != 2)
                    continue;
                jd_exp *exp1 = &op_exp->list->args[0];
                // jd_exp *exp2 = &op_exp->list->args[1];

                if (local_variable_expression_cmp(e1, exp1)) {
                    exp->type = JD_EXPRESSION_OPERATOR;
                    exp->data = op_exp;
                    op_exp->operator = to_operator(op_exp->operator);
                }
                break;
            }
            case JD_EXPRESSION_PUT_FIELD:
            {
                jd_exp_put_field *exp_put_field = exp->data;
                jd_exp *e1 = &exp_put_field->list->args[0];
                jd_exp *e2 = &exp_put_field->list->args[1];

                if (!exp_is_operator(e1))
                    continue;
                jd_exp_operator *op_exp = e1->data;
                if (op_exp->list->len != 2)
                    continue;
                jd_exp *exp1 = &op_exp->list->args[0];
                // jd_exp *exp2 = &op_exp->list->args[1];
                if (exp_is_get_field(e2) &&
                        exp_is_get_field(exp1) &&
                    get_field_expression_cmp(e2, exp1)) {
                    exp->type = JD_EXPRESSION_OPERATOR;
                    exp->data = op_exp;
                    op_exp->operator = to_operator(op_exp->operator);
                }
                break;
            }
            case JD_EXPRESSION_PUT_STATIC:
            {
                jd_exp_put_static *exp_put_static = exp->data;
                jd_exp *e1 = &exp_put_static->list->args[0];

                if (!exp_is_operator(e1))
                    continue;
                jd_exp_operator *op_exp = e1->data;
                if (op_exp->list->len != 2)
                    continue;

                jd_exp *exp1 = &op_exp->list->args[0];

                if (exp_is_get_static(exp1) &&
                    static_exp_cmp(exp_put_static, exp1->data)) {
                    exp->type = JD_EXPRESSION_OPERATOR;
                    exp->data = op_exp;
                    op_exp->operator = to_operator(op_exp->operator);
                }
                break;
            }
            default:
                break;
        }
    }
}

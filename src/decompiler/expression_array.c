#include "decompiler/expression_array.h"
#include "decompiler/expression.h"

bool identify_array_initialize(jd_method *m)
{
    /*
      2: newarray       int
      4: dup
      5: iconst_0
      6: iconst_1
      7: iastore
      8: dup
      9: iconst_1
      10: iconst_2
      11: iastore
      12: dup
    */
    /*
     * can be merge to: array = new int[]{1, 2, 3};
     */
    bool find_array_initialize = false;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);
        if (exp_is_nopped(expression) || !exp_is_assignment(expression))
            continue;

        jd_exp_assignment *assignment = expression->data;
        jd_exp *right = assignment->right;
        if (!exp_is_new_array(right))
            continue;

        jd_exp_new_array *exp_new_array = right->data;
        jd_exp_lvalue *lvalue = assignment->left->data;
        jd_var *array_var = lvalue->stack_var;

        jd_exp *next_exp = next_valid_exp(m, expression->idx + 1);
        while (exp_is_array_store(next_exp)) {
            jd_exp_array_store *store = next_exp->data;
            jd_exp *value = &store->list->args[0];
//            jd_exp *index = &store->args[1];
            jd_exp *arrayref = &store->list->args[2];
            if (!exp_is_stack_var(arrayref))
                break;

            jd_var *_array_var = arrayref->data;
            if (_array_var != array_var)
                break;

            int old_size = exp_new_array->list->len;
            exp_new_array->list->len = old_size + 1;
            exp_new_array->list->args = x_realloc(exp_new_array->list->args,
                                old_size * sizeof(jd_exp),
                                exp_new_array->list->len * sizeof(jd_exp));
            memcpy(&exp_new_array->list->args[exp_new_array->list->len - 1],
                    value, sizeof(jd_exp));
            array_var->use_count --;
            array_var->def_count --;
            array_var->dupped_count --;

            assignment->dupped_count --;
            assignment->def_count --;

            exp_mark_nopped(next_exp);
            next_exp = next_valid_exp(m, next_exp->idx + 1);
            i = next_exp->idx;
            find_array_initialize = true;
        }

    }

    return find_array_initialize;
}

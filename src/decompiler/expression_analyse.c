#include "decompiler/expression_analyse.h"
#include "decompiler/expression.h"

void method_type_analyse(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp))
            continue;

        if (exp_is_if(exp)) {

        }
        else if (exp_is_store(exp)) {

        }
        else if (exp_is_ternary(exp)) {

        }
    }
}
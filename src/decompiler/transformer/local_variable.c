#include "decompiler/transformer/transformer.h"
#include "decompiler/ssa.h"

static jd_ssa_var* get_ssa_var(jd_method *m,
                               jd_ins *ins,
                               jd_val *val)
{
    // this m for debug ssa form, don't use it in other place
    for (int i = 0; i < m->ssa_vars->size; ++i) {
        jd_ssa_var *var = lget_obj(m->ssa_vars, i);
        if (var->ins == ins && var->slot == val->slot) {
            return var;
        }
    }
    return NULL;
}

string exp_local_variable_to_s(jd_exp *expression)
{
    jd_val *val = expression->data;
    if (val->name == NULL) {
        // val->name may not be set yet during intermediate stages
        return str_create("var_%d", val->slot);
    }
    return val->name;
}

void exp_local_variable_to_stream(FILE *stream, 
                                jd_node *node, jd_exp *expression)
{
    jd_val *val = expression->data;
    fprintf(stream, "%s", val->name);
}

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
//    jd_ins *ins = val->ins;
//    if (ins != NULL && ins->m != NULL) {
//        jd_method *m = ins->m;
//        jd_ssa_var *var = get_ssa_var(m, ins, val);
//        if (var == NULL)
//            return val->name;
//        else
//            return str_create("ssa_%d_%d", var->slot, var->version);
//    }
//
//    if (ins == NULL)
//        return val->name;
    return val->name;
}

void exp_local_variable_to_stream(FILE *stream, 
                                jd_node *node, jd_exp *expression)
{
    jd_val *val = expression->data;
    fprintf(stream, "%s", val->name);
}

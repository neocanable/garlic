#ifndef GARLIC_EXPRESSION_H
#define GARLIC_EXPRESSION_H

#include "common/debug.h"
#include "decompiler/expression_helper.h"
#include "jvm/jvm_ins.h"
#include "jvm/jvm_lambda.h"

jd_exp_list* make_exp_list(int size);

void build_empty_expression(jd_exp *exp, jd_ins *ins);

void nop_empty_expression(jd_method *m);

void print_expression(jd_exp *expression, jd_ins *ins);

jd_exp* next_valid_exp(jd_method *m, int index);

jd_exp* prev_valid_exp(jd_method *m, int index);

jd_exp* exp_saved_value(jd_exp *e);

jd_exp* exp_saved_left(jd_exp *e);

bool exp_saved_same(jd_exp *e1, jd_exp *e2);

bool if_expression_is_assert(jd_exp *e);

static inline void exp_mark_copy(jd_exp *e)
{
    e->state_flag |= EXP_STATE_COPIED;
}

static inline bool exp_is_copy(jd_exp *e)
{
    return (e->state_flag & EXP_STATE_COPIED) != 0;
}

static inline void exp_mark_nopped(jd_exp *e)
{
    e->state_flag |= EXP_STATE_NOPPED;
}

static inline void exp_mark_not_nopped(jd_exp *e)
{
    e->state_flag &= ~EXP_STATE_NOPPED;
}

static inline void exp_mark_nopped_of_range(jd_method *m, int from, int to)
{
    for (int i = from; i <= to; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        exp_mark_nopped(exp);
    }
}

static inline bool exp_is_nopped(jd_exp *e)
{
    return (e->state_flag & EXP_STATE_NOPPED) != 0;
}

//#define exp_is_nopped(e) ((e)->state_flag & EXP_STATE_NOPPED)
static inline bool exp_is_save(jd_exp *e)
{
    switch (e->type)
    {
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_PUT_STATIC:
            return true;
        default:
            return false;
    }
}

static inline jd_exp* exp_of_offset(jd_method *m, uint32_t offset)
{
    jd_ins *ins = ins_of_offset(m, offset);
    return ins->expression;
}

static inline void debug_stack_var_info(jd_var *var)
{
    printf("[stack var %d]: %s def: %d use: %d store: %d dupped: %d\n",
            var->idx,
            var->name,
            var->def_count,
            var->use_count,
            var->store_count,
            var->redef_count);
}

static inline bool exp_is_invokedynamic(jd_exp *e)
{
    return e->ins != NULL &&
            jvm_ins_is_invokedynamic(e->ins) &&
            (e->type == JD_EXPRESSION_INVOKE ||
            e->type == JD_EXPRESSION_ASSIGNMENT);
}

static bool exp_is_alive_if(jd_exp *exp)
{
    return exp != NULL && !exp_is_nopped(exp) && exp_is_if(exp);
}

static bool exp_is_alive_goto(jd_exp *exp)
{
    return exp != NULL && !exp_is_nopped(exp) && exp_is_goto(exp);
}

static inline uint32_t exp_goto_offset(jd_exp *e)
{
    jd_exp_goto *g = e->data;
    return g->goto_offset;
}

static inline jd_lambda* exp_is_lambda(jd_exp *e)
{
    jd_ins *ins = e->ins;
    if (ins == NULL || !jvm_ins_is_invokedynamic(ins))
        return NULL;
    jd_method *m = ins->method;
    for (int i = 0; i < m->lambdas->size; ++i)
    {
        jd_lambda *lambda = lget_obj(m->lambdas, i);
        if (lambda->exp == e) return lambda;
    }
    return NULL;
}

#endif //GARLIC_EXPRESSION_H

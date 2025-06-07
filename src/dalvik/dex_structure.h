#ifndef GARLIC_DEX_STRUCTURE_H
#define GARLIC_DEX_STRUCTURE_H

#include "parser/dex/dex.h"

typedef struct jd_dex jd_dex;
typedef struct jd_dex_class jd_dex_class;
typedef struct jd_dex_ins jd_dex_ins;


struct jd_dex {
    jd_meta_dex     *meta;

    list_object     *classes;

    list_object     *descriptors;

    hashmap         *descs;

    jd_method_fn    *method_fn;

    jd_ins_fn       *ins_fn;

    threadpool_t    *threadpool;

    int added;

    int done;
};

struct jd_dex_ins {
    // same as jd_ins
    jd_support_type     type;
    u2                  code;
    string              name;
    u2                  state_flag;
    u2                  param_length;
    u2                  *param;
    int                 idx;
    uint32_t            offset;
    uint32_t            old_offset;

    jd_stack            *stack_out;
    jd_stack            *stack_in;
    int                 pushed_count;
    int                 popped_count;

    list_object         *targets;
    list_object         *jumps;
    list_object         *comings;

    jd_method           *method;
    jd_dex_ins          *prev;
    jd_dex_ins          *next;
    jd_exp              *expression;
    jd_bblock           *block;

    bitset_t            *uses;
    bitset_t            *defs;

    jd_ins_fn           *fn;

    void                *extra;

    dex_instruction_format format;
};

struct jd_dex_class {
    string name;

    list_object *methods;

    jd_dex *dex;

    jd_meta_dex *meta;

    dex_class_def *class_def;
};

#endif //GARLIC_DEX_STRUCTURE_H

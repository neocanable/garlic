#ifndef GARLIC_STRUCTURE_H
#define GARLIC_STRUCTURE_H

#include <assert.h>

#include "parser/class/class_structure.h"
#include "libs/hashmap/hashmap_tools.h"
#include "libs/bitset/bitset.h"
#include "libs/queue/queue.h"
#include "libs/zip/zip.h"
#include "libs/list/list.h"
#include "libs/str/str.h"
#include "libs/trie/trie_tree.h"
#include "libs/threadpool/threadpool.h"
#include "libs/threadpool/threadpool.h"


typedef struct jd_nblock            jd_nblock;
typedef struct jd_eblock            jd_eblock;
typedef struct jd_edge              jd_edge;
typedef struct jd_basic_block       jd_bblock;
typedef struct jd_method            jd_method;
typedef struct jd_ins               jd_ins;
typedef struct jd_exp               jd_exp;
typedef struct jd_node              jd_node;
typedef struct jd_synthetic_class   jd_synthetic_class;
typedef struct jd_var               jd_var;

typedef char* (*jd_access_flag_fn) (void *ptr, str_list *list);

typedef struct jd_ssa_state {
    int version;
} jd_ssa_state;

typedef struct jd_ssa_param {
    int version;
    int slot;
    int def_block_id;
    int block_id;
    jd_var *stack_var;
} jd_ssa_param;

typedef struct jd_ssa_var {
    int idx;
    jd_ins *ins;
    int version;
    int slot;
} jd_ssa_var;

typedef struct jd_local_phi_node {
    int slot;
    int version;
    int block_id;
    int ins_offset;
    list_object *params;
} jd_local_phi_node;

typedef struct jd_stack_phi_node {
    int slot;
    int version;
    int block_id;
    int ins_offset;
    list_object *params;
} jd_stack_phi_node;

typedef struct jd_var_copy {
    jd_ins *ins;
    jd_var *left;
    jd_var *right;
} jd_var_copy;

typedef enum {
    /** 
     * https://docs.oracle.com/javase/specs/jvms/se21/html/jvms-4.html
     * #jvms-4.10.1.2 
     **/
    JD_VAR_DYNAMIC   = -1,
    JD_VAR_UNKNOWN_T = 0,
    JD_VAR_INT_T,
    JD_VAR_FLOAT_T,
    JD_VAR_LONG_T,
    JD_VAR_DOUBLE_T,
    JD_VAR_NULL_T,
    JD_VAR_UNINITIALIZED_THIS_T,
    JD_VAR_UNINITIALIZED_T,
    JD_VAR_REFERENCE_T,
    JD_VAR_TOP_T,
} jd_var_types;

typedef struct jd_jvm_opcode {
    u2 code;
    int pushed_cnt;
    int popped_cnt;
    int param_length;
    int is_jump;
    string name;
} jd_jvm_opcode;

typedef struct jd_class_opcode {
    u1          code;
    u1          parameter_length; // max of parameter 16
    uint16_t    is_jump;
    string      name;
    int         popped;
    int         pushed;
} jd_class_opcode;

typedef union {
    int     int_val;
    long    long_val;
    float   float_val;
    double  double_val;
} jd_primitive_union;

typedef enum jd_name_type {
    JD_VAR_NAME_DEF = 0,
    JD_VAR_NAME_DEBUG,
} jd_name_type;

typedef struct {
    jd_primitive_union  *primitive;
    string              cname; // cname
    string              val;
} jd_val_data;

typedef struct {
    int                 idx;
    int                 slot;
    jd_var_types        type;
    jd_ins              *ins;
    jd_val_data         *data;

    string              name;
    jd_var              *stack_var;
    jd_name_type        name_type;
} jd_val;

struct jd_var {
    int             idx;
    int             slot;
    int             def_count;
    int             use_count;
    int             store_count;
    int             redef_count;
    int             dupped_count;
    jd_ins          *ins;
    jd_val          *def_val;
    string          name;
    string          cname;
};

typedef struct {
    int               depth;
    jd_val            **vals;

    int               local_vars_count;
    jd_val            **local_vars;
} jd_stack;

typedef enum {
    jd_method_descriptor   = 0,
    jd_variable_descriptor = 1,
} jd_descriptor_tag;

typedef struct {
    jd_descriptor_tag   tag;
    u2                  index;
    string              str;
    string              str_return;
    list_string         *list;
} jd_descriptor;

typedef enum {
    FIELD_STATE_HIDE       = 0x0001,
} jd_field_state;

typedef struct {
    u2                  state_flag;

    u4                  access_flags;

    jd_access_flag_fn   access_flags_fn;

    string              type;

    string              name;

    string              signature;

    string              defination;

    list_object         *annotations;

    void                *meta;
} jd_field;

typedef enum {
    INS_STATE_NOPPED                = 0x0001,
    INS_STATE_UNREACHED             = 0x0002,
    INS_STATE_COPY_BLOCK            = 0x0004,
    INS_STATE_DUPLICATE             = 0x0008,
    INS_STATE_COPY_IF_TRUE_BLOCK    = 0x0010,
    INS_STATE_TRY_START             = 0x0020,
    INS_STATE_TRY_END               = 0x0040,
    INS_STATE_HANDLER_START         = 0x0080,
    INS_STATE_HANDLER_END           = 0x0100,
    INS_STATE_FINALLY_START         = 0x0200,
    INS_STATE_FINALLY_END           = 0x0400,
} jd_ins_state;

typedef enum {
    JD_TYPE_JVM,
    JD_TYPE_DALVIK,
} jd_support_type;

typedef bool (*ins_filter)(jd_ins *ins);
typedef bool (*ins_checker)(jd_ins *ins);

typedef struct {
    ins_checker          is_return;
    ins_checker          is_void_return;
    ins_checker          is_goto;
    ins_checker          is_switch;
    ins_checker          is_if;
    ins_checker          is_conditional_jump;
    ins_checker          is_unconditional_jump;
    ins_checker          is_goto_back;
    ins_checker          is_jump_dst;
    ins_checker          is_block_start;
    ins_checker          is_block_end;
    ins_checker          is_compare;
    ins_checker          is_store;
    ins_checker          is_load;
    ins_checker          is_branch;
    ins_checker          is_athrow;
    ins_checker          is_invoke_virtual;
    ins_checker          is_invoke_static;
    ins_checker          is_invoke_interface;
    ins_checker          is_invoke_dynamic;
    ins_checker          is_invoke_special;
    ins_checker          is_invoke;
} jd_ins_fn;

struct jd_ins {
    jd_support_type     type;
    u2                  code;
    string              name;
    u2                  state_flag;
    u2                  param_length;
    u1                  *param;
    int                 idx;    // order of array
    uint32_t            offset; // order of stream
    uint32_t            old_offset;

    jd_stack            *stack_out;
    jd_stack            *stack_in;
    int                 pushed_cnt;
    int                 popped_cnt;

    list_object         *targets;
    list_object         *jumps;
    list_object         *comings;

    jd_method           *method;
    jd_ins              *prev;
    jd_ins              *next;
    jd_exp              *expression;
    jd_bblock           *block;

    bitset_t            *uses;
    bitset_t            *defs;

    jd_ins_fn           *fn;

    void                *extra;
    int                 format;
};

typedef struct {
    int         idx;
    u2          start_pc;
    u2          end_pc;
    u2          handler_pc;
    u2          catch_type;

    uint32_t    try_start;
    uint32_t    try_end;
    uint32_t    handler_start;
    uint32_t    handler_end;

    int         try_start_idx;
    int         try_end_idx;
    int         handler_start_idx;
    int         handler_end_idx;

    int         catch_type_index;
} jd_exc;

typedef struct {
    uint32_t    start_offset;
    uint32_t    end_offset;
    int         end_idx;
    int         start_idx;
} jd_range;

typedef struct {
    jd_range        *try;
    list_object     *catches;
    jd_range        *finally;
} jd_mix_exception;

typedef enum {
    JD_OP_UNKNOWN = -1,
    JD_OP_ADD,
    JD_OP_SUB,
    JD_OP_MUL,
    JD_OP_DIV,
    JD_OP_REM,
    JD_OP_NEG,
    JD_OP_NOT,
    JD_OP_SHL,
    JD_OP_SHR,
    JD_OP_USHR,
    JD_OP_AND,
    JD_OP_OR,
    JD_OP_XOR,
    JD_OP_INSTANCEOF,
    JD_OP_EQ,
    JD_OP_NE,
    JD_OP_LT,
    JD_OP_GT,
    JD_OP_GE,
    JD_OP_LE,
    JD_OP_LOGICAL_AND,
    JD_OP_LOGICAL_OR,
    JD_OP_LOGICAL_NOT,
    JD_OP_CMP,

    JD_OP_ASSIGN    ,
    JD_OP_ADD_ASSIGN,
    JD_OP_SUB_ASSIGN,
    JD_OP_MUL_ASSIGN,
    JD_OP_DIV_ASSIGN,
    JD_OP_REM_ASSIGN,
    JD_OP_AND_ASSIGN,
    JD_OP_OR_ASSIGN,
    JD_OP_XOR_ASSIGN,
    JD_OP_LSHIFT_ASSIGN,
    JD_OP_RSHIFT_ASSIGN,
    JD_OP_USHIFT_ASSIGN,
} jd_operator;


typedef enum {
    JD_EXPRESSION_UNKNOWN           = -1,
    JD_EXPRESSION_EMPTY             ,
    JD_EXPRESSION_LVALUE            ,
    JD_EXPRESSION_STACK_VAR         ,
    JD_EXPRESSION_STACK_VALUE       , // stack val
    JD_EXPRESSION_LITERAL           ,
    JD_EXPRESSION_OPERATOR          ,
    JD_EXPRESSION_SINGLE_OPERATOR   ,
    JD_EXPRESSION_SINGLE_LIST       ,
    JD_EXPRESSION_COMPARE           ,
    JD_EXPRESSION_ASSIGNMENT        ,
    JD_EXPRESSION_INVOKE            ,
    JD_EXPRESSION_LAMBDA            ,
    JD_EXPRESSION_ANONYMOUS         ,
    JD_EXPRESSION_INVOKE_VIRTUAL    ,
    JD_EXPRESSION_INVOKE_STATIC     ,
    JD_EXPRESSION_INVOKE_INTERFACE  ,
    JD_EXPRESSION_INVOKE_DYNAMIC    ,
    JD_EXPRESSION_INVOKE_SPECIAL    , 
//    JD_EXPRESSION_CONDITION         ,
    JD_EXPRESSION_IF                ,
    JD_EXPRESSION_SWITCH            ,
    JD_EXPRESSION_NEW_ARRAY         ,
    JD_EXPRESSION_ARRAY_LOAD        ,
    JD_EXPRESSION_ARRAY_STORE       ,
    JD_EXPRESSION_NEW_OBJ           ,
    JD_EXPRESSION_CAST              ,
    JD_EXPRESSION_ANNOTATION        ,
    JD_EXPRESSION_CONST             ,
    JD_EXPRESSION_GOTO              ,
    JD_EXPRESSION_RETURN            ,
    JD_EXPRESSION_TERNARY           ,
    JD_EXPRESSION_GET_STATIC        ,
    JD_EXPRESSION_PUT_STATIC        ,
    JD_EXPRESSION_GET_FIELD         ,
    JD_EXPRESSION_PUT_FIELD         ,
    JD_EXPRESSION_IINC              ,
    JD_EXPRESSION_ARRAYLENGTH       ,
    JD_EXPRESSION_UNINITIALIZE      ,
    JD_EXPRESSION_INSTANCEOF        ,
    JD_EXPRESSION_ATHROW            ,
    JD_EXPRESSION_SWAP              ,
    JD_EXPRESSION_POP               ,
    JD_EXPRESSION_DUP               ,
    JD_EXPRESSION_LOCAL_VARIABLE    ,
    JD_EXPRESSION_STORE             ,
    JD_EXPRESSION_DEFINE_STACK_VAR  ,
    JD_EXPRESSION_INITIALIZE        ,
    JD_EXPRESSION_ASSIGNMENT_CHAIN  ,
    JD_EXPRESSION_LABEL             ,
    JD_EXPRESSION_BREAK             ,
    JD_EXPRESSION_CONTINUE          ,
    JD_EXPRESSION_LOGIC_NOT         ,
    JD_EXPRESSION_WHILE             ,
    JD_EXPRESSION_DO_WHILE          ,
    JD_EXPRESSION_FOR               ,
    JD_EXPRESSION_DECLARATION       ,
    JD_EXPRESSION_ASSERT            ,
    JD_EXPRESSION_MONITOR_ENTER     ,
    JD_EXPRESSION_MONITOR_EXIT      ,
    JD_EXPRESSION_STRING_CONCAT     ,
    JD_EXPRESSION_ENUM              ,
    JD_EXPRESSION_IF_BREAK          ,
} jd_expression_type;

typedef enum {
    EXP_STATE_NOPPED    = 0x0001,
    EXP_STATE_UNREACHED = 0x0002,
    EXP_STATE_OPTIMIZED = 0x0004,
    EXP_STATE_COPIED    = 0x0008,
} jd_expression_state;

struct jd_exp {
    u4                  idx;
    jd_ins              *ins;
    jd_expression_type  type;
    u2                  state_flag;
    void                *data;
    jd_bblock           *block;
    jd_node             *node;
};

typedef struct jd_exp_list {
    int len;
    jd_exp *args;
} jd_exp_list;

typedef struct jd_exp_reader {
    jd_exp_list *list;
} jd_exp_reader;

typedef struct {
    jd_exp_list *list;
} jd_exp_store;

typedef struct {
    jd_exp_list *list;
    jd_operator operator;
} jd_exp_operator;

typedef struct {
    jd_exp_list *list;
    jd_operator operator;
} jd_exp_single_operator;

typedef struct {
    jd_exp_list *list;
} jd_exp_single_list;

typedef struct {
    jd_exp_list *list;
    jd_operator operator;
} jd_exp_compare;

typedef struct {
    jd_operator assign_operator;
    jd_exp      *left;
    jd_exp      *right;
    int         dupped_count;
    int         def_count;
} jd_exp_assignment;

typedef struct {
    list_object     *left;
    jd_exp          *right;
} jd_exp_assignment_chain;

typedef struct {
    uint32_t offset;
    u4       target_idx;
    jd_exp   *target_exp;
    jd_exp   *expression;
} jd_exp_if;

typedef struct {
    int kind;
    string name;
    string class_name;
    jd_descriptor *descriptor;
} jd_method_sig;

typedef struct {
    jd_exp *exp;
    jd_method_sig *target_method;
} jd_lambda;

typedef struct {
    jd_exp_list *list;

    jd_method       *method;
    jd_lambda       *lambda;
    string          class_name;
    string          method_name;
    jd_descriptor   *descriptor;
    bool            is_static;
} jd_exp_lambda;

typedef struct {
    jd_exp_list *list;
    jsource_file *jfile;
    string method_name;
    string cname;
} jd_exp_anonymous;

typedef struct {
    jd_exp_list *list;

    string          class_name;
    string          method_name;
    jd_descriptor   *descriptor;
    jd_exp_lambda   *lambda;
    jd_exp_anonymous *anonymous;
} jd_exp_invoke;

struct jd_synthetic_class {
    string name;
    jsource_file *jfile;
};

typedef struct jd_switch_param {
    int ikey;
    string skey;
    uint32_t offset;
} jd_switch_param;

typedef struct {
    jd_exp_list *list;
    uint32_t default_offset;
    list_object *targets;
} jd_exp_switch;

typedef struct {
    uint32_t goto_offset;
    u4       target_idx;
    jd_exp   *target_exp;
} jd_exp_goto;

typedef struct {
    jd_exp_list *list;
    string          class_name;
} jd_exp_new_array;

typedef struct {
    jd_exp_list *list;
} jd_exp_array_load;

typedef struct {
    jd_exp_list *list;
} jd_exp_array_store;

typedef struct {
    jd_exp_list *list;
} jd_exp_arraylength;

typedef struct {
    jd_exp_list *list;

    u2              index;
    string          class_name;
    string          name;
} jd_exp_get_field;

typedef struct {
    jd_exp_list *list;
    u2              index;
    string          class_name;
    string          name;
} jd_exp_put_field;

typedef struct {
    jd_exp_list *list;

    u2              index;
    string          class_name;
    string          name;
    string          owner_class_name;
} jd_exp_get_static;

typedef struct {
    jd_exp_list *list;

    u2              index;
    string          class_name;
    string          name;

    string          owner_class_name;
} jd_exp_put_static;

typedef struct {
    jd_exp_list *list;
} jd_exp_new_obj;

typedef struct {
    jd_exp_list *list;
    string      class_name;
} jd_exp_cast;

typedef struct {
    jd_exp_list *list;
} jd_exp_return;

typedef struct {
    jd_exp_list *list;
} jd_exp_ternary;

typedef struct {
    jd_var *stack_var;
} jd_exp_lvalue;

typedef struct {
    jd_exp_list *list;
} jd_exp_athrow;

typedef struct {
    jd_val    *val;
    jd_var_types    type;
    jd_val_data     *data;
} jd_exp_const;

typedef struct {
    jd_exp_list *list;
    int iinc_type; // 0, pre; 1, post
} jd_exp_iinc;

typedef struct {
    jd_val *val;
    jd_synthetic_class *synthetic_class;
    jsource_file *jfile;
} jd_exp_uninitialize;

typedef struct {
    jd_exp_list *list;

    string          class_name;
} jd_exp_initialize;

typedef struct {
    jd_exp_list *list;
    string class_name;
} jd_exp_instanceof;

typedef struct {
    jd_exp_list *list;
} jd_exp_array_assign;

typedef struct {
    jd_exp_list *list;
} jd_exp_logic_not;

typedef struct {
    jd_exp_list     *list;
    uint32_t        start_offset;
    uint32_t        end_offset;
    int             start_idx;
    int             end_idx;
} jd_exp_loop;

typedef struct {
    jd_exp_list     *list;
    uint32_t        start_offset;
    uint32_t        end_offset;
    int             start_idx;
    int             end_idx;
    uint32_t        offset;
} jd_exp_while;

typedef struct {
    jd_exp_list     *list;
    int             start_idx;
    int             end_idx;
    uint32_t        offset;
} jd_exp_do_while;

typedef struct {
    jd_exp_list     *list;
    int             start_idx;
    int             end_idx;
    uint32_t        offset;
} jd_exp_for;

typedef struct {
    jd_exp_list *list;
} jd_exp_def_var;

typedef struct {
    jd_exp_list *list;
}jd_exp_monitorenter;

typedef struct {
    jd_exp_list *list;
} jd_exp_monitorexit;

typedef struct {
    jd_exp_list *list;
} jd_exp_str_concat;

typedef struct {
    string name;
    jd_exp_list *list;
} jd_exp_num_item;

typedef struct {
    string class_name;
    list_object *list;
} jd_exp_enum;

typedef enum {
    JD_NODE_UNKNOWN = -1,
    JD_NODE_METHOD_ROOT = 0,
    JD_NODE_IF,
    JD_NODE_ELSE_IF,
    JD_NODE_ELSE,
    JD_NODE_SWITCH,
    JD_NODE_CASE,
    JD_NODE_TRY,
    JD_NODE_CATCH,
    JD_NODE_FINALLY,
    JD_NODE_SYNCHRONIZED,
    JD_NODE_LOOP,
    JD_NODE_WHILE,
    JD_NODE_FOR,
    JD_NODE_DO_WHILE,
    JD_NODE_IF_TRUE,
    JD_NODE_IF_FALSE,
    JD_NODE_EXCEPTION,
    JD_NODE_BASIC_BLOCK,
    JD_NODE_EXPRESSION,

    JD_NODE_CLASS_ROOT        ,
    JD_NODE_PACKAGE_IMPORT    ,
    JD_NODE_CLASS             ,
    JD_NODE_INNER_CLASS       ,
    JD_NODE_METHOD            ,
    JD_NODE_FIELD             ,

    // special node
    JD_NODE_DELETED           ,
} jd_node_type;

typedef struct jd_case {
    u4 start_idx;
    u4 end_idx;
    int key;
    int is_default;
    jd_bblock  *block;
    list_object *blocks;
} jd_case;

typedef struct {
    jd_bblock       *start_block;
    list_object     *blocks;
    list_object     *true_blocks;
    list_object     *false_blocks;
    jd_node         *node;

    int             if_start_idx;
    int             true_start_idx;
    int             true_end_idx;
    int             false_start_idx;
    int             false_end_idx;
} jd_if_branch;

typedef struct {
    int             start_idx;
    int             end_idx;
    int             break_idx;
    jd_bblock       *switch_block;
    jd_case         *default_case;
    list_object     *cases;
    list_object     *blocks;
} jd_switch;

typedef enum {
    JD_LOOP_UNKNOWN = 0,
    JD_LOOP_WHILE,
    JD_LOOP_DO_WHILE,
    JD_LOOP_FOR,
    JD_LOOP_INFINITE,
} jd_loop_type;

typedef struct {
    int start_idx;
    int end_idx;
    int is_post_condition;
    int can_write_condition;
    bool cross_exception;

    jd_bblock *header;
    jd_bblock *tail;
    jd_bblock *condition;
    jd_range  *header_range;
    jd_range  *condition_range;
    jd_range  *tail_range;

    list_object *condition_blocks;
    list_object *exit_to_blocks;
    jd_bblock   *exit_block;

    list_object *blocks;
    list_object *code_ranges;
    jd_node *node;

    jd_loop_type type;
} jd_loop;

/**
 * 1. exception有断的情况，有些obfuscator会把try,catch,finally分开
 * 2. switch有断的情况，有些obfuscator会把switch, case分开
 **/

struct jd_node {
    int             node_id;
    int             start_idx;
    int             end_idx;
    void            *data;

    jd_node_type    type;
    jd_method       *method;
    jd_node         *parent;
    list_object     *children;

    /**
     * catch/case/if/loop/switch/synchronized/else_if
     * 这些节点是有参数的,param_exp就是存储这个参数的
     **/
    jd_exp          *param_exp;

    int             order;
};

enum method_state_flag {
    METHOD_STATE_LAMBDA        = 0x0001,
    METHOD_STATE_HIDE          = 0x0002,
    METHOD_STATE_UNSUPPORT     = 0x0004,
};

typedef struct {
    string fname;
    void *value;
    string str;
} jd_annotation;

typedef struct jd_variable_scope {
    string name;
    jd_val *val;
    jd_var *var;
    int declaration_idx;
    int start_idx;
    int end_idx;
    string fname;
    string cname;
    bitset_t *stores;
} jd_variable_scope;

typedef bool (*jd_method_filter_fn)(jd_method *m);
typedef char* (*jd_meth_param_annotation_fn)(jd_method *m, int index);
typedef jd_val* (*jd_meth_param_val_fn)(jd_method *m, int index);

typedef struct jd_method_fn {
    jd_method_filter_fn is_native;
    jd_method_filter_fn is_init;
    jd_method_filter_fn is_clinit;
    jd_method_filter_fn is_abstract;
    jd_method_filter_fn is_member;
    jd_method_filter_fn is_synthetic;
    jd_method_filter_fn is_varargs;
    jd_access_flag_fn access_flags_fn;
    jd_meth_param_val_fn param_val_fn;
    jd_meth_param_annotation_fn param_annotation_fn;
} jd_method_fn;

struct jd_method {
    u2              state_flag;

    u4              access_flags;

    jd_method_fn    *fn;

//    jd_access_flag_fn           access_flags_fn;
//    jd_meth_param_val_fn        param_val_fn;
//    jd_meth_param_annotation_fn param_annotation_fn;

    int             max_locals;

    string          name;

    string          signature;

    string          defination;

    jd_descriptor   *desc;

    uint32_t        code_length;

    u1              *code;

    int             variable_counter;

    hashmap         *slot_counter_map;

    hashmap         *offset2id_map;

    hashmap         *class_counter_map;

    hashmap         *var_name_map;

    list_object     *var_scopes;

    uint32_t        ins_size;

    list_object     *instructions;

    list_object     *expressions;

    /** 
     * https://docs.oracle.com/javase/specs/jvms/se21/html/jvms-4.html
     * #jvms-4.10.2.2 
     **/
    queue_object   *ins_visit_queue;

    list_object     *ins_watch_successors;

    list_object     *basic_blocks;

    list_object     *nodes;

    list_object     *cfg_exceptions;

    list_object     *closed_exceptions;

    list_object     *mix_exceptions;

    list_object     *loops;

    list_object     *switches;

    list_object     *branches;

    list_object     *stack_variables;

    // hashmap         *offset2varidx_map;

    hashmap         *offset2var_map;

    list_int        *assignment_chains;

    list_object     *lambdas;

    list_object     *annotations;

    jd_stack        *enter;

    jd_val          **parameters;

    bitset_t        *declarations;

    list_object     *types;

    list_object     *uses;
    list_object     *defs;
    list_object     *live_ins;
    list_object     *live_outs;
    list_object     *assigns;
    list_object     *reaching_in;
    list_object     *reaching_out;
    list_object     *live_intervals;

    list_object     *local_phi_list;

    list_object     *stack_phi_list;

    list_object     *ssa_vars;

    hashmap         *stack_phi_node_copies;

    // void 类型的变量, 表示两种形式
    // java jclass_file
    // dalvik jd_dex

    jd_support_type type;

    jsource_file    *jfile;

    /**
     * class file : jmethod
     * dalvik file: encoded_method
     **/
    void            *meta_method;

    /**
     * class file: jclass_file
     * dalvik file: jd_dex
     **/
    void            *meta;
};

enum jd_bblock_id {
    JD_BB_ENTER_ID          = 0,
    JD_BB_EXIT_ID           = 1,
    JD_BB_EXCEPTION_ID      = 2,
};

typedef enum jd_bblock_type {
    JD_BB_NORMAL         = 0,
    JD_BB_EXCEPTION      = 1,
    JD_BB_ENTER          = 2,
    JD_BB_EXIT           = 3,
    JD_BB_EXCEPTION_EXIT = 4,
} jd_bblock_type;

typedef union {
    jd_nblock       *nblock;
    jd_eblock       *eblock;
} jd_union_basic_block;

struct jd_basic_block {
    size_t                  block_id;
    int                     visited;
    int                     live;
    jd_bblock_type          type;
    jd_node_type            source;
    void                    *data;
    list_object             *in;
    list_object             *out;
    jd_union_basic_block    *ub;

    list_object             *dom_children;
    list_object             *frontier;
    list_object             *dominates;
    jd_bblock               *idom;

    jd_method               *method;
    jd_node                 *node;
    bool                    is_dup;
};

enum jd_edge_type {
    JD_EDGE_NORMAL     = 1,
    JD_EDGE_EXCEPTION  = 2,
};

struct jd_edge {
    int                 source_block_id;
    int                 target_block_id;
    jd_bblock           *source_block;
    jd_bblock           *target_block;
    enum jd_edge_type   type;
};

struct jd_nblock {
    int             start_idx;
    int             end_idx;
    void            *start_ins;
    void            *end_ins;
    uint32_t        start_offset;
    uint32_t        end_offset;

    list_object     *instructions;
    list_object     *expressions;
};

typedef enum jd_exception_type {
    JD_EXCEPTION_CATCH = 0,
    JD_EXCEPTION_FINALLY
} jd_exception_type;

struct jd_eblock {
    int                     idx;
    uint32_t                try_start_offset;
    uint32_t                try_end_offset;
    uint32_t                handler_start_offset;
    uint32_t                handler_end_offset;
    int                     try_start_idx;
    int                     try_end_idx;
    int                     handler_start_idx;
    int                     handler_end_idx;

    jd_exc                  *exception;
    enum jd_exception_type  type;
};

typedef struct jd_jar jd_jar;

typedef struct jd_jar_entry {
    int                     index;
    string                  path;
    string                  cname;
    bool                    is_inner;
    bool                    is_anoymous;
    bool                    parsed;
    list_object             *inner_classes;
    list_object             *anoymous_classes;
    jd_jar                  *jar;
    struct jd_jar_entry     *parent;
    jsource_file            *jf;

    size_t                  buf_size;
    char                    *buf;
} jd_jar_entry;

struct jd_jar {
    string          path;
    string          save;
    mem_pool        *pool;
    hashmap         *inner_class_map;
    hashmap         *anoymous_class_map;
    hashmap         *name_to_index_map;
    hashmap         *index_to_name_map;

    int             entries_size;
    list_object     *class_entries;
    int             added;
    int             done;
    struct zip_t    *zip;

    threadpool_t    *threadpool;
    pthread_mutex_t *lock;
};

struct jsource_file {
    jd_support_type type;

    jd_access_flag_fn access_flags_fn;
    // package cname
    string          pname;

    // simple class cname
    string          sname;

    string          super_cname;

    list_object     *interfaces;

    string          fname; // full class cname

    bool            is_inner;

    bool            is_anonymous;

    // signature of class
    string          signature;

    // class defination string
    string          defination;

    uint32_t        fields_count;

    jd_field        *fields;

    uint32_t        methods_count;

    list_object     *methods;

    jsource_file    *parent;

    void            *meta;

    void            *jclass;

    list_object     *descriptors;

    hashmap         *descs_map;

    hashmap         *descs;

    jd_trie_node    *imports;

    list_object     *annotations;

    list_object     *blocks;

    list_object     *inner_classes;

    struct zip_t    *zip;

    jd_jar          *jar;

    jd_jar_entry    *jar_entry;

    jd_method_fn    *method_fn;

    jd_ins_fn       *ins_fn;

    FILE            *source;
};

#endif //GARLIC_STRUCTURE_H

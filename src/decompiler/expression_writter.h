#ifndef GARLIC_EXPRESSION_WRITTER_H
#define GARLIC_EXPRESSION_WRITTER_H

#include "decompiler/structure.h"
#include "expression_node_helper.h"

#define DEFAULT_WRITE_OUT stdout

void print_code_nodes(jd_method *m, jd_node *node);

void print_code_nodes_v2(jd_method *m, jd_node *node);

void jsource_file_writter_first(jsource_file *jf, jd_node *node);

void writter_for_class(jsource_file *jf, jd_node *node);

void writter_for_anonymous_class(jsource_file *jf, jd_node *node);

string method_block_to_string(jd_method *m, jd_node *node);

string lambada_method_ident(jd_method *m);

void print_expression(jd_exp *expression, jd_ins *ins);

void print_all_expression(jd_method *m);

void print_all_expression_with_ssa(jd_method *m);

void print_full_expression(jd_method *m);

static inline string get_node_ident(jd_node *node)
{
    int tabsize = 4;
    int level = 0;
    jd_node *parent = node->parent;
    while (parent != NULL) {
        if (!node_is_method_root(parent))
            level ++;
        parent = parent->parent;
    }

    if (!node_is_package_import(node))
        level --;

    string ident = NULL;
    if (level > 0) {
        ident = x_alloc(level*tabsize+1);
        for (int i = 0; i < level*tabsize; ++i)
            ident[i] = ' ';
        ident[level*tabsize] = '\0';
    }
    else {
        ident = x_alloc(1);
        ident[0] = '\0';
    }
    return ident;
}

static inline string node_name(jd_node *node) {
    switch (node->type) {
        case JD_NODE_METHOD_ROOT:  return "root";
        case JD_NODE_TRY:          return "try";
        case JD_NODE_CATCH:        return "catch";
        case JD_NODE_FINALLY:      return "finally";
        case JD_NODE_SWITCH:       return "switch";
        case JD_NODE_CASE:         return "case";
        case JD_NODE_SYNCHRONIZED: return "synchronized";
        case JD_NODE_IF:           return "if";
        case JD_NODE_ELSE_IF:      return "else if";
        case JD_NODE_ELSE:         return "else";
        case JD_NODE_LOOP:         return "loop";
        case JD_NODE_WHILE:        return "while";
        case JD_NODE_FOR:          return "for";
        case JD_NODE_DO_WHILE:     return "do_while";
        case JD_NODE_IF_TRUE:      return "if_true_block";
        case JD_NODE_IF_FALSE:     return "if_false_block";
        case JD_NODE_EXCEPTION:    return "exception";
        case JD_NODE_EXPRESSION:   return "expression";
        case JD_NODE_BASIC_BLOCK:  return "basic_block";
        default:                    return "unknown";
    }
}


#endif //GARLIC_EXPRESSION_WRITTER_H

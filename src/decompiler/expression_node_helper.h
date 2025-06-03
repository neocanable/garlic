#ifndef GARLIC_EXPRESSION_NODE_HELPER_H
#define GARLIC_EXPRESSION_NODE_HELPER_H

#include "decompiler/structure.h"

#define DEFINE_NODE_HELPER_METHOD(t, name)  \
    static inline bool                      \
    node_is_##name(jd_node *n)              \
    {                                       \
        return n->type == t;                \
    }                                       \
                                            \
    static inline bool                      \
    node_is_not_##name(jd_node *n)          \
    {                                       \
        return n->type != t;                \
    }                                       \

DEFINE_NODE_HELPER_METHOD(JD_NODE_UNKNOWN,      unknown);
DEFINE_NODE_HELPER_METHOD(JD_NODE_METHOD_ROOT,  method_root);
DEFINE_NODE_HELPER_METHOD(JD_NODE_IF,           if);
DEFINE_NODE_HELPER_METHOD(JD_NODE_ELSE_IF,      else_if);
DEFINE_NODE_HELPER_METHOD(JD_NODE_ELSE,         else);
DEFINE_NODE_HELPER_METHOD(JD_NODE_SWITCH,       switch);
DEFINE_NODE_HELPER_METHOD(JD_NODE_CASE,         case);
DEFINE_NODE_HELPER_METHOD(JD_NODE_EXCEPTION,    exception);
DEFINE_NODE_HELPER_METHOD(JD_NODE_TRY,          try);
DEFINE_NODE_HELPER_METHOD(JD_NODE_CATCH,        catch);
DEFINE_NODE_HELPER_METHOD(JD_NODE_FINALLY,      finally);
DEFINE_NODE_HELPER_METHOD(JD_NODE_SYNCHRONIZED, synchronized);
DEFINE_NODE_HELPER_METHOD(JD_NODE_LOOP,         loop);
DEFINE_NODE_HELPER_METHOD(JD_NODE_WHILE,        while);
DEFINE_NODE_HELPER_METHOD(JD_NODE_FOR,          for);
DEFINE_NODE_HELPER_METHOD(JD_NODE_DO_WHILE,     do_while);
DEFINE_NODE_HELPER_METHOD(JD_NODE_IF_TRUE,      if_true);
DEFINE_NODE_HELPER_METHOD(JD_NODE_IF_FALSE,     if_false);
DEFINE_NODE_HELPER_METHOD(JD_NODE_EXPRESSION,   expression);

DEFINE_NODE_HELPER_METHOD(JD_NODE_BASIC_BLOCK, basic_block);

DEFINE_NODE_HELPER_METHOD(JD_NODE_CLASS_ROOT,       class_root);
DEFINE_NODE_HELPER_METHOD(JD_NODE_PACKAGE_IMPORT,   package_import);
DEFINE_NODE_HELPER_METHOD(JD_NODE_CLASS,            class);
DEFINE_NODE_HELPER_METHOD(JD_NODE_INNER_CLASS,      inner_class);
DEFINE_NODE_HELPER_METHOD(JD_NODE_METHOD,           method);
DEFINE_NODE_HELPER_METHOD(JD_NODE_FIELD,            field);



#endif //GARLIC_EXPRESSION_NODE_HELPER_H

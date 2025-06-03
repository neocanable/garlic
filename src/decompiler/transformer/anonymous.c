#include "decompiler/transformer/transformer.h"
#include "decompiler/method.h"
#include "decompiler/expression_writter.h"
#include "decompiler/klass.h"

string exp_anonymous_to_s(jd_exp *expression)
{
    return NULL;
}

void exp_anonymous_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_anonymous *anaonymous = expression->data;
    fprintf(stream, "new %s(", anaonymous->cname);
    for (int i = 1; i < anaonymous->list->len; ++i) {
        jd_exp *arg = &anaonymous->list->args[i];
        expression_to_stream(stream, node, arg);
        if (i != anaonymous->list->len - 1)
            fprintf(stream, ", ");
    }
    fprintf(stream, ") {\n");
    jsource_file *inner = anaonymous->jfile;
    jd_node *root_node = lget_obj_first(inner->blocks);
    jd_node *class_node = NULL;
    for (int i = 0; i < root_node->children->size; ++i) {
        jd_node *n = lget_obj(root_node->children, i);
        if (node_is_class(n))
            class_node = n;
    }
    assert(class_node != NULL);
    class_node->parent = node;
    writter_for_anonymous_class(anaonymous->jfile, class_node);
    string ident = get_node_ident(node);
    fprintf(stream, "%s}", ident);
}

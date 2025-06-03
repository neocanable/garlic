#include "decompiler/transformer/transformer.h"
#include "decompiler/method.h"
#include "decompiler/expression_writter.h"
#include "decompiler/klass.h"

string exp_lambda_to_s(jd_exp *expression)
{
    jd_exp_lambda *exp_lambda = expression->data;
    jd_method *m = exp_lambda->method;
    if (m == NULL) {
//        jd_lambda *lambda = exp_lambda->lambda;
        // check target m is static
        if (exp_lambda->is_static || exp_lambda->list->len == 0)
            return str_create("%s::%s",
                              class_simple_name(
                                      exp_lambda->class_name),
                              exp_lambda->method_name);
        else
            return str_create("%s::%s",
                              exp_to_s(&exp_lambda->list->args[0]),
                              exp_lambda->method_name);
    }
    else {
        if (method_is_synthetic(m) && exp_lambda->list->len > 0) {
            string defination = create_lambda_defination(m);
            string method_body = method_block_to_string(m, NULL);
            string ident = lambada_method_ident(m);

            return str_create("%s{\n %s%s}",
                              defination, method_body, ident);
        }
        else {
            if (method_is_member(m) && exp_lambda->list->len > 0) {
                jd_exp *first = &exp_lambda->list->args[0];
                return str_create("%s::%s",
                                  first,
                                  m->name);
            }
            else
                return str_create("%s::%s",
                                  exp_lambda->class_name, m->name);
        }
    }
}

void exp_lambda_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_lambda *exp_lambda = expression->data;
    jd_method *m = exp_lambda->method;
    if (m == NULL) {
        if (exp_lambda->is_static && exp_lambda->list->len > 0) {
            jd_exp *first = &exp_lambda->list->args[0];
            expression_to_stream(stream, node, first);
            fprintf(stream, "::%s", exp_lambda->method_name);
        }
        else
            fprintf(stream, "%s::%s",
                    class_simple_name(exp_lambda->class_name),
                    exp_lambda->method_name);
    }
    else {
        if (method_is_synthetic(m)) {
            string defination = create_lambda_defination(m);
            string ident = get_node_ident(node);
            fprintf(stream, "%s {\n", defination);
            jd_node *root = lget_obj(m->nodes, 0);
            root->parent = node;
            writter_for_class(m->jfile, root);
            fprintf(stream, "%s}", ident);
        }
        else {
            if (method_is_member(m) && exp_lambda->list->len > 0) {
                jd_exp *first_exp = &exp_lambda->list->args[0];
                expression_to_stream(stream, node, first_exp);
                fprintf(stream, "::%s", m->name);
            }
            else
                fprintf(stream, "%s::%s",
                        exp_lambda->class_name, m->name);
        }
    }
}

#include "decompiler/transformer/transformer.h"
#include "decompiler/method.h"
#include "decompiler/expression_writter.h"
#include "decompiler/klass.h"

static bool lambda_is_constructor_ref(jd_exp_lambda *exp_lambda)
{
    jd_method *m = exp_lambda->method;
    string name = m == NULL ? exp_lambda->method_name : m->name;
    return exp_lambda->class_name != NULL &&
           name != NULL && STR_EQL(name, g_str_init);
}

static string lambda_constructor_ref(jd_exp_lambda *exp_lambda)
{
    return str_create("%s::%s",
                      class_simple_name_without_primitive(exp_lambda->class_name),
                      "new");
}

static int lambda_body_skip(jd_exp_lambda *exp_lambda, jd_method *m)
{
    if (exp_lambda->body_arity <= 0 || m->desc == NULL ||
            m->desc->list == NULL)
        return exp_lambda->captures;

    int arity = exp_lambda->body_arity - 1;
    int body = m->desc->list->size;
    return body >= arity ? body - arity : exp_lambda->captures;
}

static bool lambda_capture_is_primary(jd_exp *exp)
{
    switch (exp->type) {
        case JD_EXPRESSION_LOCAL_VARIABLE:
        case JD_EXPRESSION_STACK_VAR:
        case JD_EXPRESSION_STACK_VALUE:
        case JD_EXPRESSION_GET_FIELD:
        case JD_EXPRESSION_GET_STATIC:
        case JD_EXPRESSION_ARRAY_LOAD:
            return true;
        default:
            return false;
    }
}

static string lambda_capture_to_s(jd_exp_lambda *exp_lambda)
{
    jd_exp *first = &exp_lambda->list->args[0];
    if (lambda_capture_is_primary(first))
        return exp_to_s(first);

    return str_create("(%s)", exp_to_s(first));
}

static void lambda_capture_to_stream(FILE *stream, jd_node *node,
                                     jd_exp_lambda *exp_lambda)
{
    jd_exp *first = &exp_lambda->list->args[0];
    bool parenthesise = !lambda_capture_is_primary(first);

    if (parenthesise)
        fprintf(stream, "(");
    expression_to_stream(stream, node, first);
    if (parenthesise)
        fprintf(stream, ")");
}

string exp_lambda_to_s(jd_exp *expression)
{
    jd_exp_lambda *exp_lambda = expression->data;
    jd_method *m = exp_lambda->method;
    if (exp_lambda->class_name == NULL &&
            (m == NULL || m->name == NULL))
        return str_dup(g_str_unknown);
    if (lambda_is_constructor_ref(exp_lambda))
        return lambda_constructor_ref(exp_lambda);
    if (m == NULL) {
//        jd_lambda *lambda = exp_lambda->lambda;
        // check target m is static
        if (exp_lambda->is_static || exp_lambda->captures == 0)
            return str_create("%s::%s",
                              class_simple_name_without_primitive(
                                      exp_lambda->class_name),
                              exp_lambda->method_name);
        else
            return str_create("%s::%s",
                              lambda_capture_to_s(exp_lambda),
                              exp_lambda->method_name);
    }
    else {
        if (method_is_synthetic(m) && exp_lambda->captures > 0) {
            string defination = create_lambda_defination(
                    m, lambda_body_skip(exp_lambda, m));
            string method_body = method_block_to_string(m, NULL);
            string ident = lambada_method_ident(m);

            return str_create("%s{\n %s%s}",
                              defination, method_body, ident);
        }
        else {
            if (method_is_member(m) && exp_lambda->captures > 0) {
                return str_create("%s::%s",
                                  lambda_capture_to_s(exp_lambda),
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
    if (exp_lambda->class_name == NULL &&
            (m == NULL || m->name == NULL))
        return;
    if (lambda_is_constructor_ref(exp_lambda)) {
        fprintf(stream, "%s", lambda_constructor_ref(exp_lambda));
        return;
    }
    if (m == NULL) {
        if (!exp_lambda->is_static && exp_lambda->captures > 0) {
            lambda_capture_to_stream(stream, node, exp_lambda);
            fprintf(stream, "::%s", exp_lambda->method_name);
        }
        else
            fprintf(stream, "%s::%s",
                    class_simple_name_without_primitive(exp_lambda->class_name),
                    exp_lambda->method_name);
    }
    else {
        if (method_is_synthetic(m)) {
            string defination = create_lambda_defination(
                    m, lambda_body_skip(exp_lambda, m));
            string ident = get_node_ident(node);
            fprintf(stream, "%s {\n", defination);
            jd_node *root = lget_obj(m->nodes, 0);
            root->parent = node;
            writter_for_class(m->jfile, root);
            fprintf(stream, "%s}", ident);
        }
        else {
            if (method_is_member(m) && exp_lambda->captures > 0) {
                lambda_capture_to_stream(stream, node, exp_lambda);
                fprintf(stream, "::%s", m->name);
            }
            else
                fprintf(stream, "%s::%s",
                        exp_lambda->class_name, m->name);
        }
    }
}

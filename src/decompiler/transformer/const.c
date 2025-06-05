#include "common/str_tools.h"
#include "decompiler/transformer/transformer.h"
#include "decompiler/klass.h"
#include "decompiler/stack.h"

static inline bool const_exp_is_string(jd_exp_const *e)
{
    string full_class_name = e->val->data->cname;
    return STR_EQL(full_class_name, "String");
}

static inline bool const_exp_is_class(jd_exp_const *e)
{
    string full_class_name = e->val->data->cname;
    return STR_EQL(full_class_name, "Class");
}

static inline bool const_exp_is_boolean(jd_exp_const *e)
{
    if (e->val->data->cname == NULL)
        return false;

    return stack_val_is_boolean(e->val);
}

static string get_const_value(jd_exp *expression)
{
    jd_exp_const *const_exp = expression->data;
    jd_val_data *data = const_exp->val->data;
    jd_primitive_union *primitive = data->primitive;

    switch (const_exp->val->type) {
        case JD_VAR_INT_T: {
            if (const_exp_is_boolean(const_exp) &&
                primitive->int_val == 0)
                return "false";
            else if (const_exp_is_boolean(const_exp) &&
                primitive->int_val == 1)
                return "true";
            else
                return str_create("%d", primitive->int_val);
        }
        case JD_VAR_LONG_T:
            return str_create("%ldL", primitive->long_val);
        case JD_VAR_FLOAT_T:
            return str_create("%f", primitive->float_val);
        case JD_VAR_DOUBLE_T:
            return str_create("%lf", primitive->double_val);
        case JD_VAR_NULL_T:
            return str_dup("null");
        case JD_VAR_REFERENCE_T: {
            if (const_exp_is_string(const_exp))
                return str_create("\"%s\"", const_exp->val->data->val);
            else if (const_exp_is_class(const_exp))
                return str_create("%s.class",
                                  class_simple_name(
                                          const_exp->val->data->val));
            else
                return str_create("%s", const_exp->val->data->val);
        }
        default:
            return (string)g_str_unknown;
    }
}

string exp_const_to_s(jd_exp *expression)
{
    return get_const_value(expression);
}

void exp_const_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    fprintf(stream, "%s", get_const_value(expression));
}

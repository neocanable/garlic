#include "decompiler/transformer/transformer.h"
#include "str.h"

static bool new_array_is_initializer(jd_exp_new_array *new_array)
{
    return new_array->list->len > 1;
}

static string new_array_type_name(jd_exp_new_array *new_array)
{
    return new_array->class_name == NULL ? (string) g_str_unknown
                                         : new_array->class_name;
}

static string new_array_length_to_s(string type, string count)
{
    char *brackets = strchr(type, '[');
    if (brackets == NULL)
        return str_create("new %s[%s]", type, count);

    return str_create("new %.*s[%s]%s", (int) (brackets - type), type, count,
                      brackets);
}

string exp_new_array_to_s(jd_exp *expression)
{
    jd_exp_new_array *new_array = expression->data;

    if (!new_array_is_initializer(new_array)) {
        string count = new_array->list->len == 0
                       ? str_dup("0")
                       : exp_to_s(&new_array->list->args[0]);
        return new_array_length_to_s(new_array_type_name(new_array), count);
    }

    str_list *list = str_list_init();
    strs_concat(list, 3, "new ", new_array_type_name(new_array), "[]{");
    for (int i = 1; i < new_array->list->len; ++i) {
        str_concat(list, exp_to_s(&new_array->list->args[i]));
        if (i != new_array->list->len - 1)
            str_concat(list, ", ");
    }
    str_concat(list, "}");
    return str_join(list);
}

void exp_new_array_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_new_array *new_array = expression->data;

    if (!new_array_is_initializer(new_array)) {
        string type = new_array_type_name(new_array);
        char *brackets = strchr(type, '[');
        if (brackets == NULL)
            fprintf(stream, "new %s[", type);
        else
            fprintf(stream, "new %.*s[", (int) (brackets - type), type);

        if (new_array->list->len == 0)
            fprintf(stream, "0");
        else
            expression_to_stream(stream, node, &new_array->list->args[0]);

        fprintf(stream, "]%s", brackets == NULL ? "" : brackets);
        return;
    }

    fprintf(stream, "new %s[]{", new_array_type_name(new_array));
    for (int i = 1; i < new_array->list->len; ++i) {
        expression_to_stream(stream, node, &new_array->list->args[i]);
        if (i != new_array->list->len - 1)
            fprintf(stream, ", ");
    }
    fprintf(stream, "}");
}

#include "decompiler/transformer/transformer.h"
#include "str.h"

static bool new_array_is_initializer(jd_exp_new_array *new_array)
{
    return new_array->dimensions == 0 && new_array->list->len > 1;
}

static string new_array_type_name(jd_exp_new_array *new_array)
{
    return new_array->class_name == NULL ? (string) g_str_unknown
                                         : new_array->class_name;
}

static int new_array_base_length(string type)
{
    char *brackets = strchr(type, '[');
    return brackets == NULL ? (int) strlen(type) : (int) (brackets - type);
}

static string new_array_count_to_s(jd_exp_new_array *new_array, int i)
{
    if (i < new_array->list->len)
        return exp_to_s(&new_array->list->args[i]);
    return str_dup("0");
}

string exp_new_array_to_s(jd_exp *expression)
{
    jd_exp_new_array *new_array = expression->data;

    if (!new_array_is_initializer(new_array)) {
        string type = new_array_type_name(new_array);
        char *brackets = strchr(type, '[');
        int dims = new_array->dimensions > 0 ? new_array->dimensions : 1;

        str_list *list = str_list_init();
        str_concat(list, str_create("new %.*s",
                                    new_array_base_length(type), type));
        for (int i = 0; i < dims; ++i) {
            str_concat(list, str_create("[%s]",
                                        new_array_count_to_s(new_array, i)));
        }
        if (brackets != NULL)
            str_concat(list, brackets);
        return str_join(list);
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
        int dims = new_array->dimensions > 0 ? new_array->dimensions : 1;

        fprintf(stream, "new %.*s", new_array_base_length(type), type);
        for (int i = 0; i < dims; ++i) {
            fprintf(stream, "[");
            if (i < new_array->list->len)
                expression_to_stream(stream, node,
                                     &new_array->list->args[i]);
            else
                fprintf(stream, "0");
            fprintf(stream, "]");
        }
        if (brackets != NULL)
            fprintf(stream, "%s", brackets);
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

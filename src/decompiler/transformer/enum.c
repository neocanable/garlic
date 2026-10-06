#include "decompiler/transformer/transformer.h"

string exp_enum_to_s(jd_exp *expression)
{
    return str_create("unknown %d", expression->type);
}

void exp_enum_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    jd_exp_enum *enum_exp = expression->data;
    bool last_item;
    for (int i = 0; i < enum_exp->list->size; ++i) {
        jd_exp_num_item *item = lget_obj(enum_exp->list, i);
        last_item = i == enum_exp->list->size - 1;

        if (item->list->len <= 2) {
            fprintf(stream, "%s", item->name);
        }
        else {
            fprintf(stream, "%s(", item->name);
            for (int j = 2; j < item->list->len; ++j) {
                expression_to_stream(stream, node, &item->list->args[j]);
                if (j != item->list->len - 1)
                    fprintf(stream, ", ");
            }
            fprintf(stream, ")");
        }

        if (!last_item)
            fprintf(stream, ", ");
    }
}

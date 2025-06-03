#include "transformer.h"
#include "jvm/jvm_ins.h"

string exp_invoke_to_s(jd_exp *expression)
{
    if (jvm_ins_is_invokevirtual(expression->ins) ||
        jvm_ins_is_invokeinterface(expression->ins) ||
        jvm_ins_is_invokespecial(expression->ins)) {
        jd_exp_invoke *invoke = expression->data;
        string method_name = invoke->method_name;
        
        jd_exp *arg_exp = &invoke->list->args[invoke->list->len - 1];
        string object_ref_str = exp_to_s(arg_exp);
        size_t len;
        string result;
        size_t new_len;
        if (jvm_ins_is_invokespecial(expression->ins)) {
            len = snprintf(NULL, 0, "new %s.%s", 
                           object_ref_str, method_name) + 3;
            result = x_alloc(len);
            snprintf(result, len, "new %s.%s", object_ref_str, method_name);
        }
        else {
            len = snprintf(NULL, 0, "%s.%s", object_ref_str, method_name) + 3;
            result = x_alloc(len);
            snprintf(result, len, "%s.%s", object_ref_str, method_name);
        }
        new_len = len;
        strcat(result, "(");

        for (int j = 0; j <= invoke->list->len - 2; ++j) {
            string arg_name = exp_to_s(&invoke->list->args[j]);
            new_len = len + strlen(arg_name) + 2;

            result = x_realloc(result, len, new_len);
            strcat(result, arg_name);
            if (j != invoke->list->len - 2)
                strcat(result, ", ");
            len = new_len;
        }
        strcat(result, ")");
        result[len-1] = '\0';
        return result;
    }
    else if (jvm_ins_is_invokestatic(expression->ins)) {
        jd_exp_invoke *invoke = expression->data;
        string method_name = invoke->method_name;
        string class_name = invoke->class_name;
        size_t len = strlen(class_name) + 1 + strlen(method_name) + 3;
        size_t new_len = len;
        string s = x_alloc(len);
        memset(s, 0, len);
        strcat(s, class_name);
        strcat(s, ".");
        strcat(s, method_name);
        strcat(s, "(");
        for (int j = 0; j <= invoke->list->len - 1; ++j) {
            string arg_name = exp_to_s(&invoke->list->args[j]);
            new_len = len + strlen(arg_name) + 2;
            s = x_realloc(s, len, new_len);
            strcat(s, arg_name);
            if (j != invoke->list->len - 1)
                strcat(s, ", ");
            len = new_len;
        }
        strcat(s, ")");
        s[len] = '\0';
        return s;
    }
    else {
        jd_exp_invoke *invoke = expression->data;
        string method_name = invoke->method_name;
        size_t len = strlen(method_name) + 3;
        size_t new_len = len;
        string s = x_alloc(len);
        memset(s, 0, len);
        strcat(s, method_name);
        strcat(s, "(");
        for (int j = 0; j <= invoke->list->len - 1; ++j) {
            string arg_name = exp_to_s(&invoke->list->args[j]);
            new_len = len + strlen(arg_name) + 2;
            s = x_realloc(s, len, new_len);
            strcat(s, arg_name);
            if (j != invoke->list->len - 1)
                strcat(s, ", ");
            len = new_len;
        }
        strcat(s, ")");
        s[len] = '\0';
        return s;
    }
}


void exp_invoke_to_stream(FILE *stream, jd_node *node, jd_exp *expression)
{
    if (jvm_ins_is_invokevirtual(expression->ins) ||
        jvm_ins_is_invokeinterface(expression->ins) ||
        jvm_ins_is_invokespecial(expression->ins))
    {
        jd_exp_invoke *invoke = expression->data;
        string method_name = invoke->method_name;

        jd_exp *arg_exp = &invoke->list->args[invoke->list->len - 1];
        if (jvm_ins_is_invokespecial(expression->ins)) {
            fprintf(stream, "new ");
            expression_to_stream(stream, node, arg_exp);
            fprintf(stream, ".%s(", method_name);
        }
        else {
            expression_to_stream(stream, node, arg_exp);
            fprintf(stream, ".%s(", method_name);
        }

        for (int j = 0; j <= invoke->list->len - 2; ++j) {
            expression_to_stream(stream, node, &invoke->list->args[j]);
            if (j != invoke->list->len - 2)
                fprintf(stream, ", ");
        }
        fprintf(stream, ")");
    }
    else {
        jd_exp_invoke *invoke = expression->data;
        string method_name = invoke->method_name;
        fprintf(stream, "%s(", method_name);
        for (int j = 0; j <= invoke->list->len - 1; ++j) {
            expression_to_stream(stream, node, &invoke->list->args[j]);
            if (j != invoke->list->len - 1)
                fprintf(stream, ", ");
        }
        fprintf(stream, ")");
    }
}

#include "parser/class/class_tools.h"
#include "common/str_tools.h"
#include "decompiler/klass.h"
#include "descriptor.h"


#define EXTRACT_BASIC_DESCRIPTOR_MARCOS(param_type) do {                \
        if (_from_length == 0) _from_length = i;                        \
        size_t _obj_total_length = i-_from_length+1+1;                  \
        string substring = x_alloc(_obj_total_length);                  \
        memcpy(substring, &str[_from_length], _obj_total_length-1);     \
        substring[_obj_total_length] = '\0';                            \
        ladd_string(descriptor->list, substring);                       \
        cur_parameter += 1;                                             \
        _is_obj = 0;                                                    \
        _is_arr = 0;                                                    \
        _from_length = 0;                                               \
} while(0)

#define EXTRACT_BASIC_STRING_MARCOS(param_type) do {                    \
        size_t _obj_length = strlen(param_type);                        \
        if (_from_length == 0) _from_length = i;                        \
        size_t _obj_total_length = _obj_length + _is_arr*2 + 1;         \
        string substring = x_alloc(_obj_total_length);                  \
        snprintf(substring, _obj_total_length, "%s", param_type);       \
        for (int j = 0; j < _is_arr; ++j)                               \
            strcat(substring, "[]");                                    \
        substring[_obj_total_length] = '\0';                            \
        _is_obj = 0;                                                    \
        _is_arr = 0;                                                    \
        _from_length = 0;                                               \
        return substring;                                               \
} while(0)


string descriptor_to_s(string str)
{
    int _is_arr = 0;
    int _is_obj = 0;
    size_t _from_length = 0;

    if (str[0] != 'L' && str[0] != '[')
        return str_dup(str);

    for (size_t i = 0; i < strlen(str); ++i) {
        char tmp = str[i];
        switch(tmp) {
            case 'L': {
                if (!_is_obj) {
                    _from_length = i;
                    _is_obj = 1;
                }
                break;
            }
            case '[': {
                _is_arr += 1;
                _from_length = i;
                break;
            }
            case ';': {
                for (int j = _from_length; j < i; ++j) {
                    if (str[j] == '/') _from_length = j;
                }
                size_t _obj_length = i - _from_length-1;
                size_t _obj_total_length = _obj_length + _is_arr*2 + 1;
                char *substring = x_alloc(_obj_total_length);
                memcpy(substring, &str[_from_length+1], _obj_length);
                for (int j = 0; j < _is_arr; ++j) {
                    strcat(substring, "[]");
                }
                substring[_obj_total_length] = '\0';
                return substring;
            }
            case 'B': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("byte");
            }
            case 'C': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("char");
            }
            case 'D': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("double");
            }
            case 'I': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("int");
            }
            case 'F': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("float");
            }
            case 'J': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("long");
            }
            case 'S': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("short");
            }
            case 'Z': {
                if (!_is_obj)
                    EXTRACT_BASIC_STRING_MARCOS("boolean");
            }
            default:
                break;
        }
    }
    return NULL;
}

static void descriptor_tokenizer(jd_descriptor *descriptor)
{
    char *str = descriptor->str;
    int _is_arr = 0;
    int _is_obj = 0;
    int _is_generic = 0;
    size_t _from_length = 0;
    size_t cur_parameter = 0;

    descriptor->list = linit_string();
    for (size_t i = 0; i < strlen(str); ++i) {
        char tmp = str[i];
        if (tmp == '[') {
            _is_arr += 1;
            _from_length = i;
        }
        else if (tmp == '<') {
            _is_generic ++;
            if (_is_generic == 1)
                _from_length = i;
        }
        else if (tmp == '>') {
            _is_generic --;
            if (_is_generic == 0)
                _from_length = 0;
        }
        else if (tmp == 'L' && _is_obj == 0) {
            _is_obj = 1;
            _from_length = i;
        }
        else if (tmp == ';') {
            size_t _obj_length = i - _from_length-1;
            size_t _obj_total_length = _obj_length + _is_arr*2 + 1;
            char *substring = x_alloc(_obj_total_length);
            memcpy(substring, &str[_from_length+1], _obj_length);
            for (int j = 0; j < _is_arr; ++j)
                strcat(substring, "[]");
            substring[_obj_total_length] = '\0';
            _is_obj = 0;
            _is_arr = 0;
            _from_length = 0;
            ladd_string(descriptor->list, substring);
            cur_parameter += 1;
        }
        else if (tmp == 'B' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("byte");
        }
        else if (tmp == 'C' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("char");
        }
        else if (tmp == 'D' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("double");
        }
        else if (tmp == 'I' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("int");
        }
        else if (tmp == 'F' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("float");
        }
        else if (tmp == 'J' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("long");
        }
        else if (tmp == 'S' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("short");
        }
        else if (tmp == 'Z' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("boolean");
        }
        else if (tmp == 'V' && _is_obj == 0) {
            EXTRACT_BASIC_DESCRIPTOR_MARCOS("void");
        }
    }
}

void expand_descriptor(jd_descriptor *descriptor)
{
    if (descriptor->str[0] == '(')
        descriptor->tag = jd_method_descriptor;
    else
        descriptor->tag = jd_variable_descriptor;
    descriptor_tokenizer(descriptor);

    if (descriptor->tag == jd_method_descriptor) {
        int index = descriptor->list->size-1;
        descriptor->str_return = lget_string(descriptor->list, index);
        lremove_string(descriptor->list, descriptor->list->size-1);
    }
}

jd_var_types descriptor_data_type(string descriptor)
{
    return descriptor_data_type_of_char(descriptor[0]);
}

jd_var_types descriptor_data_type_of_char(char c) {
    switch (c) {
        case 'B':
        case 'C':
        case 'I':
        case 'S':
        case 'Z':
            return JD_VAR_INT_T;
        case 'D':
            return JD_VAR_DOUBLE_T;
        case 'F':
            return JD_VAR_FLOAT_T;
        case 'J':
            return JD_VAR_LONG_T;
        default:
            return JD_VAR_REFERENCE_T;
    }
}
#include "common/str_tools.h"
#include "decompiler/signature.h"
#include "decompiler/klass.h"
#include "parser/class/class_tools.h"

static inline void increase(int *int_pointer)
{
    (*int_pointer)++;
}

static inline char char_at_index(string sig, int *pint)
{
    return sig[*pint];
}

string simple_class_signature_to_s(simple_class_type_sig *ss)
{
    str_list *list = str_list_init();
    str_concat(list, class_path_to_short(ss->name));
    if (ss->type_arguments != NULL) {
        str_concat(list, ("<"));
        for (int i = 0; i < ss->type_arguments->size; ++i) {
            type_argument *ta = lget_obj(ss->type_arguments, i);
            string fts_str = field_type_sig_to_s(ta);
            str_concat(list, fts_str);
            if (i != ss->type_arguments->size-1)
                str_concat(list, (", "));
        }
        str_concat(list, (">"));
    }
    string result = str_join(list);
    return result;
}

string field_type_sig_to_s(field_type_sig *fts)
{
    switch (fts->tag) {
        case BASE_TYPE_SIGNATURE: {
            switch (fts->type->base_type->tag) {
                case 0:     return str_dup(g_str_byte);
                case 1:     return str_dup(g_str_char);
                case 2:     return str_dup(g_str_double);
                case 3:     return str_dup(g_str_float);
                case 4:     return str_dup(g_str_int);
                case 5:     return str_dup(g_str_long);
                case 6:     return str_dup(g_str_short);
                case 7:     return str_dup(g_str_boolean);
                case 8:     return str_dup(g_str_void);
                default:    return str_dup(g_str_unknown);
            }
        }
        case CLASS_TYPE_SIGNATURE: {
            str_list *list = str_list_init();
            class_type_sig *cts = fts->type->class_type;
            for (int i = 0; i < cts->path->size; ++i) {
                simple_class_type_sig *ss = lget_obj(cts->path, i);
                string s1 = simple_class_signature_to_s(ss);
                str_concat(list, s1);
            }
            return str_join(list);
        }
        case TYPE_VARIABLE_SIGNATURE: {
            type_variable_signature *tvs = fts->type->type_variable;
            return tvs->name;
        }
        case ARRAY_TYPE_SIGNATURE: {
            array_type_signature *ats = fts->type->array_type;
            str_list *list = str_list_init();
            str_concat(list, field_type_sig_to_s(ats->component_type));
            for (int i = 0; i < ats->level; ++i)
                str_concat(list, ("[]"));
            return str_join(list);
        }
        case BOTTOM_SIGNATURE: {
            return ("Object");
        }
        case SIMPLE_CLASS_TYPE_SIGNATURE: {
            simple_class_type_sig *ss = fts->type->simple_class_type;
            return simple_class_signature_to_s(ss);
        }
        case WILDCARD: {
            str_list *list = str_list_init();

            wildcard *w = fts->type->wildcard;
            field_type_sig *fsuper = w->super_bound;
            field_type_sig *fextends = w->extends_bound;
            if (STR_EQL(w->tag, "+")) {
                str_concat(list, ("? extends "));
                if (fsuper->tag != BOTTOM_SIGNATURE) {
                    string super_str = field_type_sig_to_s(fsuper);
                    str_concat(list, super_str);
                    if (fextends->tag != BOTTOM_SIGNATURE)
                        str_concat(list, (" & "));
                }
                if (fextends->tag != BOTTOM_SIGNATURE) {
                    string extends_str = field_type_sig_to_s(fextends);
                    str_concat(list, extends_str);
                }
            }
            else if (STR_EQL(w->tag, "-")) {
                str_concat(list, ("? super "));
                if (fsuper->tag != BOTTOM_SIGNATURE) {
                    string super_str = field_type_sig_to_s(fsuper);
                    str_concat(list, super_str);
//                    if (fextends->tag != BOTTOM_SIGNATURE)
//                        add_to_jd_str_list(list, (" & "));
                }
                if (fextends->tag != BOTTOM_SIGNATURE) {
                    string extends_str = field_type_sig_to_s(fextends);
                    if (!STR_EQL(extends_str, "Object")) {
                        str_concat(list, (" & "));
                        str_concat(list, extends_str);
                    }
                }
            }
            else if (STR_EQL(w->tag, "*")) {
                str_concat(list, ("?"));
                if (fsuper->tag != BOTTOM_SIGNATURE) {
                    string super_str = field_type_sig_to_s(fsuper);
                    if (!STR_EQL(super_str, "Object")) {
                        str_concat(list, (" super "));
                        str_concat(list, super_str);
                    }
                }
                if (fextends->tag != BOTTOM_SIGNATURE) {
                    string extends_str = field_type_sig_to_s(fextends);
                    if (!STR_EQL(extends_str, "Object")) {
                        str_concat(list, (" & "));
                        str_concat(list, extends_str);
                    }
                }
            }

            return str_join(list);
        }
        case FIELD_TYPE_SIGNATURE: {
            return field_type_sig_to_s(fts->type->field_type);
        }
        default:
            return (string)g_str_unknown;
    }
}

string formal_type_parameters_to_s(list_object *formal_type_parameters)
{
    if (is_list_empty(formal_type_parameters))
        return NULL;

    str_list *list = str_list_init();
    if (!is_list_empty(formal_type_parameters))
        str_concat(list, ("<"));

    for (int i = 0; i < formal_type_parameters->size; ++i) {
        formal_type_parameter *ftp = lget_obj(formal_type_parameters, i);
        str_concat(list, ftp->name);

        for (int j = 0; j < ftp->bounds->size; ++j) {
            field_type_sig *fts = lget_obj(ftp->bounds, j);
            string fts_str = field_type_sig_to_s(fts);
            if (j == 0 && !STR_EQL(fts_str, "Object")) {
                str_concat(list, " extends ");
                str_concat(list, fts_str);
            }
            else if (j != ftp->bounds->size - 1 && 
                    !STR_EQL(fts_str, "Object")) {
                str_concat(list, " & ");
                str_concat(list, fts_str);
            }
        }

        if (i != formal_type_parameters->size - 1)
            str_concat(list, (", "));
    }
    if (!is_list_empty(formal_type_parameters))
        str_concat(list, (">"));

    return str_join(list);
}

string interfaces_to_s(class_signature *cs)
{
    if (cs->interfaces == NULL || cs->interfaces->size == 0)
        return NULL;
    str_list *list = str_list_init();

    for (int i = 0; i < cs->interfaces->size; ++i) {
        class_type_sig *cts = lget_obj(cs->interfaces, i);
        for (int j = 0; j < cts->path->size; ++j) {
            simple_class_type_sig *ss = lget_obj(cts->path, j);
            string sss = simple_class_signature_to_s(ss);
            str_concat(list, sss);
            if (j != cts->path->size - 1)
                str_concat(list, (", "));
        }
        if (i != cs->interfaces->size - 1)
            str_concat(list, (", "));
    }
    return str_join(list);
}

static type_argument* parse_type_argument(string sig, int *pint)
{
    type_argument *ta = make_obj(type_argument);
    ta->type = make_obj(type_signature_union);

    char c = char_at_index(sig, pint);
    switch (c) {
        case '+': {
            increase(pint);
            wildcard *w = make_obj(wildcard);
            field_type_sig *fts = make_obj(field_type_sig);
            ta->tag = WILDCARD;
            ta->type->wildcard = w;
            w->tag = str_dup("+");
            w->super_bound = fts;
            fts->tag = BOTTOM_SIGNATURE;
            fts->type = make_obj(type_signature_union);
            fts->type->bottom = make_obj(type_signature_union);

            w->extends_bound = parse_field_type_signature(sig, pint);
            DEBUG_PRINT("BottomSignature\n");
            return ta;
        }
        case '*': {
            increase(pint);
            DEBUG_PRINT("[wild card]extends: java.lang.Object\n");

            simple_class_type_sig *ss = make_obj(simple_class_type_sig);
            ss->dollor = 0;
            ss->name = ("java/lang/Object");
            ss->type_arguments = NULL;

            wildcard *w = make_obj(wildcard);
            ta->tag = WILDCARD;
            ta->type->wildcard = w;
            w->tag = str_dup("*");
            w->super_bound = make_obj(field_type_sig);
            w->super_bound->tag = BOTTOM_SIGNATURE;
            w->super_bound->type = make_obj(type_signature_union);
            w->super_bound->type->bottom = make_obj(bottom_sig);

            w->extends_bound = make_obj(field_type_sig);
            w->extends_bound->tag = SIMPLE_CLASS_TYPE_SIGNATURE;
            w->extends_bound->type = make_obj(field_type_sig);
            w->extends_bound->type->simple_class_type = ss;
            return ta;
        }
        case '-': {
            increase(pint);
//            parse_field_type_signature(sig, pint);
            simple_class_type_sig *ss = make_obj(simple_class_type_sig);
            ss->dollor = 0;
            ss->name = ("java/lang/Object");
            ss->type_arguments = NULL;

            wildcard *w = make_obj(wildcard);
            ta->tag = WILDCARD;
            ta->type->wildcard = w;
            w->tag = str_dup("-");
            w->super_bound = parse_field_type_signature(sig, pint);

            w->extends_bound = make_obj(field_type_sig);
            w->extends_bound->tag = SIMPLE_CLASS_TYPE_SIGNATURE;
            w->extends_bound->type = make_obj(type_signature_union);
            w->extends_bound->type->simple_class_type = ss;
            DEBUG_PRINT("[wild card]super: java.lang.Object\n");
            return ta;
        }
        default: {
            return parse_field_type_signature(sig, pint);
        }
    }
}

static list_object* parse_type_arguments(string sig, int *pint)
{
    list_object *type_arguments = linit_object();

    increase(pint);
    ladd_obj(type_arguments, parse_type_argument(sig, pint));
    char c = char_at_index(sig, pint);
    while (c != '>') {
        ladd_obj(type_arguments, parse_type_argument(sig, pint));
        c = char_at_index(sig, pint);
    }
    increase(pint);

    return type_arguments;
}

static void parse_class_type_sig_suffix(string sig,
                                        int *pint,
                                        list_object *list)
{
    char c = char_at_index(sig, pint);

    while (c == '/' || c == '.') {
        int use_dollor = c == '.';
        increase(pint);
        sst_sig *ss = parse_simple_class_type_sig(sig, pint, use_dollor);
        ladd_obj(list, ss);
        c = char_at_index(sig, pint);
    }
}

static sst_sig* parse_simple_class_type_sig(string sig,
                                            int *pint,
                                            int use_dollor)
{
    simple_class_type_sig *s = make_obj(sst_sig);
    s->name = parse_identifier(sig, pint);
    s->dollor = use_dollor;

    char c = char_at_index(sig, pint);

    switch (c) {
        case ';':
        case '/':
        case '.':
        case '$':
            s->type_arguments = NULL;
            break;
        case '<':
            s->type_arguments = parse_type_arguments(sig, pint);
            break;
        default:
            break;
    }
    return s;
}

static class_type_sig* parse_class_type_signature(string sig, int *pint)
{
    list_object *list = linit_object();
    increase(pint);
    simple_class_type_sig *ss = parse_simple_class_type_sig(sig, pint, 0);
    ladd_obj(list, ss);
    parse_class_type_sig_suffix(sig, pint, list);
    increase(pint);

    class_type_sig *cs = make_obj(class_type_sig);
    cs->path = list;
    return cs;
}

static type_variable_signature* parse_type_variable_sig(string sig, int *pint)
{
    increase(pint);
    type_variable_signature *ts = make_obj(type_variable_signature);
    ts->name = parse_identifier(sig, pint);
    increase(pint);

    return ts;
}

static base_type_signature* parse_base_type(string sig, int *pint)
{
    base_type_signature *bs = make_obj(base_type_signature);
    char c = char_at_index(sig, pint);
    switch (c) {
        case 'B':
            increase(pint);
            bs->tag = 0;
            break;
        case 'C':
            increase(pint);
            bs->tag = 1;
            break;
        case 'D':
            increase(pint);
            bs->tag = 2;
            break;
        case 'F':
            increase(pint);
            bs->tag = 3;
            break;
        case 'I':
            increase(pint);
            bs->tag = 4;
            break;
        case 'J':
            increase(pint);
            bs->tag = 5;
            break;
        case 'S':
            increase(pint);
            bs->tag = 6;
            break;
        case 'Z':
            increase(pint);
            bs->tag = 7;
            break;
        case 'V':
            increase(pint);
            bs->tag = 8;
            break;
        default:
            break;
    }
    return bs;
}

static type_sig* parse_type_signature(string sig, int *pint)
{
    type_sig *ts = make_obj(type_sig);
    ts->type = make_obj(type_signature_union);
    char c = char_at_index(sig, pint);
    switch (c) {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z':
        case 'V':
            ts->tag = BASE_TYPE_SIGNATURE;
            ts->type->base_type = parse_base_type(sig, pint);
            break;
        default:
            ts->tag = FIELD_TYPE_SIGNATURE;
            ts->type->field_type = parse_field_type_signature(sig, pint);
            break;
    }
    return ts;
}

static array_type_signature* parse_array_type_signature(string sig, int *pint)
{
    int level = 0;
//    increase(pint);
    char c = char_at_index(sig, pint);
    while (c == '[') {
        increase(pint);
        level ++;
        c = char_at_index(sig, pint);
    }
    type_sig *ts = parse_type_signature(sig, pint);
    array_type_signature *ats = make_obj(array_type_signature);
    ats->level = level;
    ats->component_type = ts;
    return ats;
}

static field_type_sig* parse_field_type_signature(string sig, int *pint)
{
    field_type_sig *ft = make_obj(field_type_sig);
    ft->type = make_obj(type_signature_union);

    char c = char_at_index(sig, pint);
    switch(c) {
        case 'L':
            ft->tag = CLASS_TYPE_SIGNATURE;
            ft->type->class_type = parse_class_type_signature(sig, pint);
            break;
        case 'T':
            ft->tag = TYPE_VARIABLE_SIGNATURE;
            ft->type->type_variable = parse_type_variable_sig(sig, pint);
            break;
        case '[':
            ft->tag = ARRAY_TYPE_SIGNATURE;
            ft->type->array_type = parse_array_type_signature(sig, pint);
            break;
        default: {
            DEBUG_PRINT("[parse_field_type_signature] error\n");
            break;
        }
    }
    return ft;
}

static list_object* parse_bounds(string sig, int *pint)
{
    list_object *fts = linit_object();
    char c = char_at_index(sig, pint);
    if (c == ':') {
        increase(pint);
        c = char_at_index(sig, pint);
        if (c == ':') {
            bottom_sig *bs = make_obj(bottom_sig);
            type_sig *ts = make_obj(type_sig);
            ts->type = make_obj(type_signature_union);
            ts->tag = BOTTOM_SIGNATURE;
            ts->type->bottom = bs;
            ladd_obj(fts, ts);
            DEBUG_PRINT("[Bottom Signature]\n");
        }
        else
            ladd_obj(fts, parse_field_type_signature(sig, pint));

        c = char_at_index(sig, pint);
        while (c == ':') {
            increase(pint);
            ladd_obj(fts, parse_field_type_signature(sig, pint));
            c = char_at_index(sig, pint);
        }
    }
    return fts;
}

static string parse_identifier(string sig, int *pint)
{
    int start = *pint;
    char c = char_at_index(sig, pint);
    while (c != ' ') {
        switch (c) {
            case ';':
            case '.':
            case ':':
            case '>':
            case '<': {
                int end = *pint;
                int len = end - start + 1;
                string identifier = x_alloc(len);
                memcpy(identifier, sig + start, len-1);
                identifier[len] = '\0';
                DEBUG_PRINT("[identifier]: %d %s\n", len, identifier);
                return identifier;
            }
            default: {
                increase(pint);
                c = char_at_index(sig, pint);
            }
        }
    }
    return NULL;
}

static formal_type_parameter* parse_formal_type_parameter(string sig, 
                                                        int *pint)
{
    formal_type_parameter *ftp = make_obj(formal_type_parameter);

    ftp->name = parse_identifier(sig, pint);

    ftp->bounds = parse_bounds(sig, pint);

    return ftp;
}

static list_object* parse_formal_type_parameters(string sig, int *pint)
{
    if (sig[*pint] != '<') {
        DEBUG_PRINT("[formal type param]: EMPTY FORMAL TYPE PARAM\n");
        return NULL;
    }
    list_object *params = linit_object();

    increase(pint);
    ladd_obj(params, parse_formal_type_parameter(sig, pint));

    char c = char_at_index(sig, pint);
    while (c != '>') {
        ladd_obj(params, parse_formal_type_parameter(sig, pint));
        c = char_at_index(sig, pint);
    }
    increase(pint);
    return params;
}

static list_object* parse_interfaces(string sig, int *pint)
{
    list_object *list = linit_object();
    char c = char_at_index(sig, pint);
    while (c == 'L') {
        ladd_obj(list, parse_class_type_signature(sig, pint));
        c = char_at_index(sig, pint);
    }
    return list;
}

static list_object* parse_type_signatures(string sig, int *pint)
{
    list_object *list = linit_object();
    int stop = 0;
    char c = char_at_index(sig, pint);
    while (!stop) {
        switch (c) {
            case 'B':
            case 'C':
            case 'D':
            case 'F':
            case 'I':
            case 'J':
            case 'S':
            case 'Z':
            case 'L':
            case 'T':
            case '[': {
                ladd_obj(list, parse_type_signature(sig, pint));
                c = char_at_index(sig, pint);
                break;
            }
            default:
                stop = 1;
        }
    }
    return list;
}

static list_object* parse_formal_parameters(string sig, int *pint)
{
    char c = char_at_index(sig, pint);
    if (c != '(') {
        DEBUG_PRINT("[parse_formal_parameters] empty signature\n");
        return NULL;
    }
    else {
        increase(pint);
        list_object *list = parse_type_signatures(sig, pint);
        increase(pint);
        return list;
    }
}

static type_sig* parse_return_type(string sig, int *pint)
{
    type_sig *ts = make_obj(type_sig);
    ts->type = make_obj(type_signature_union);
    char c = char_at_index(sig, pint);
    if (c == 'V') {
        increase(pint);
        DEBUG_PRINT("[return type] void\n");
        ts->tag = BASE_TYPE_SIGNATURE;
        ts->type->base_type = make_obj(base_type_signature);
        ts->type->base_type->tag = 8;
        return ts;
    }
    else {
        return parse_type_signature(sig, pint);
    }
}

static class_type_sig* parse_throws_signature(string sig, int *pint)
{
    increase(pint);
    return parse_class_type_signature(sig, pint);
}

static list_object* parse_throws_signatures(string sig, int *pint)
{
    list_object *list = linit_object();
    char c = char_at_index(sig, pint);
    while (c == '^') {
        ladd_obj(list, parse_throws_signature(sig, pint));
        c = char_at_index(sig, pint);
    }
    return list;
}

class_signature* parse_class_signature(string sig)
{
    int *pint = make_obj(int);
    *pint = 0;

    class_signature *cs = make_obj(class_signature);

    cs->formal_type_parameters = parse_formal_type_parameters(sig, pint);
    cs->base_class = parse_class_type_signature(sig, pint);
    cs->interfaces = parse_interfaces(sig, pint);
    return cs;
}

method_sig* parse_method_signature(string sig)
{
    int *pint = make_obj(int);
    *pint = 0;
    method_sig *ms = make_obj(method_sig);
    ms->formal_type_parameters = parse_formal_type_parameters(sig, pint);
    ms->parameter_types = parse_formal_parameters(sig, pint);
    ms->return_type = parse_return_type(sig, pint);
    ms->exception_types = parse_throws_signatures(sig, pint);
    return ms;
}

type_sig* parse_field_signature(string sig)
{
    int *pint = make_obj(int);
    *pint = 0;
    type_sig *ts = parse_type_signature(sig, pint);
    return ts;
}

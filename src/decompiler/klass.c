#include "decompiler/klass.h"
#include "decompiler/signature.h"
#include "decompiler/method.h"
#include "decompiler/expression_node.h"
#include "common/str_tools.h"

#include "libs/str/str.h"

// 检查字符是否是合法的 Java 标识符首字符
static bool is_java_identifier_start(char c)
{
    return (isalpha(c) || c == '_' || c == '$');
}

// 检查字符是否是合法的 Java 标识符后续字符
static bool is_java_identifier_part(char c)
{
    return (isalnum(c) || c == '_' || c == '$');
}

// 检查字符串是否是合法的 Java 标识符
static bool is_valid_java_identifier(const char *str,
                                     int start_pos,
                                     int end_pos)
{
    if (start_pos >= end_pos) return false;

    // 检查首字符
    if (!is_java_identifier_start(str[start_pos])) return false;

    // 检查后续字符
    for (int i = start_pos + 1; i < end_pos; i++) {
        if (!is_java_identifier_part(str[i])) return false;
    }

    return true;
}

bool is_inner_class(string class_name)
{
    int result = false;

    char *dollar_pos = strchr(class_name, '$');
    if (dollar_pos == NULL) return result;

    int dollar_idx = dollar_pos - class_name;
    int len = strlen(class_name);

    // $ 不能在开头或结尾
    if (dollar_idx == 0 || dollar_idx == len - 1) return 0;

    // 检查 $ 前后的子串是否合法
    int valid_outer = is_valid_java_identifier(class_name, 0, dollar_idx);
    int valid_inner = is_valid_java_identifier(class_name, dollar_idx+1, len);

    result = (valid_outer && valid_inner);
    return result;
}

bool is_anonymous_class(string class_name)
{
    bool result = false;

    // 找到最后一个 $ 的位置
    char *last_dollar = strrchr(class_name, '$');
    if (last_dollar == NULL)
        return result;
    int dollar_idx = last_dollar - class_name;
    int len = strlen(class_name);

    // $ 后必须全部是数字
    for (int i = dollar_idx + 1; i < len; i++) {
        if (!isdigit(class_name[i])) return false;
    }

    // 匿名类至少有一个数字（如 Outer$1）
    result = (len - dollar_idx > 1);
    return result;
}

static string class_type_array_name(string class_name, int depth)
{
    size_t len = strlen(class_name);
    size_t new_len = len + depth*2 + 1;
    string new_class_name = x_alloc(new_len);
    memcpy(new_class_name, class_name, strlen(class_name));
    for (int i = 0; i < depth; ++i)
        strcat(new_class_name, "[]");
    new_class_name[new_len - 1] = '\0';
    return new_class_name;
}

string class_path_to_short(string class_name)
{
    // not primitive and array
    // [Ljava/lang/String => String[]
    // Ljava/lang/String;[] => String[]
    // Ljava/lang/String; => String
    const char *last_slash = strrchr(class_name, '/');
    const char *start = (last_slash == NULL) ? class_name : last_slash + 1;

    const char *last_char = strstr(start, ";");
    size_t len = (last_char != NULL) ? (last_char - start) : strlen(start);

    if (len == 0)
        return str_dup("");
    string simple_name = x_alloc(len + 1);
    strncpy(simple_name, start, len);
    simple_name[len] = '\0';
    return simple_name;

//    printf("full class name: %s\n", class_name);
//    size_t len = strlen(class_name);
//    size_t start_index = len - 1;
//    if (class_name[start_index] == ';')
//        len --;
//
//    char c = class_name[start_index];
//    while (c != '/' && c > 0) {
//        start_index--;
//        c = class_name[start_index];
//    }
//    size_t simple_len = len - (start_index + 1);
//    string simple_name = x_alloc(simple_len + 1);
//    memcpy(simple_name, class_name + start_index + 1, simple_len);
//    simple_name[simple_len] = '\0';
//    return simple_name;
}

void cut_generic_type_from_class_name(string class_name)
{
    size_t len = strlen(class_name);
    size_t start_index = len - 1;
    char c = class_name[start_index];
    while (c != '<' && start_index > 0) {
        start_index--;
        c = class_name[start_index];
    }
    if (start_index != 0) {
        memset(class_name + start_index, 0, len - start_index);
        class_name[start_index] = '\0';
    }
}

string class_full_name(string descriptor)
{
    size_t len = strlen(descriptor);
    switch (descriptor[0]) {
        case 'I': return (string)g_str_int;
        case 'J': return (string)g_str_long;
        case 'F': return (string)g_str_float;
        case 'D': return (string)g_str_double;
        case 'B': return (string)g_str_byte;
        case 'C': return (string)g_str_char;
        case 'S': return (string)g_str_short;
        case 'Z': return (string)g_str_boolean;
        case '[': {
            char c = descriptor[0];
            int depth = 0;
            while (c == '[') {
                depth++;
                c = descriptor[depth];
            }
            switch (c) {
                case 'L': {
                    size_t class_name_len = len - depth - 1 + 1;
                    string class_name = x_alloc(class_name_len);
                    memcpy(class_name, descriptor + depth + 1, class_name_len);
                    cut_generic_type_from_class_name(class_name);
                    size_t _tmp_len = strlen(class_name);
                    if (class_name[_tmp_len-1] == ';')
                        class_name[_tmp_len-1] = '\0';
                    return class_name;
                }
                case 'I': return class_type_array_name((string)g_str_int, depth);
                case 'J': return class_type_array_name((string)g_str_long, depth);
                case 'F': return class_type_array_name((string)g_str_float, depth);
                case 'D': return class_type_array_name((string)g_str_double, depth);
                case 'B': return class_type_array_name((string)g_str_byte, depth);
                case 'C': return class_type_array_name((string)g_str_char, depth);
                case 'S': return class_type_array_name((string)g_str_short, depth);
                case 'Z': return class_type_array_name((string)g_str_boolean, depth);
                default:
                    return NULL;
            }
        }
        case 'L': {
            string class_name = x_alloc(len);
            memcpy(class_name, descriptor + 1, len - 1);
            class_name[len - 1] = '\0';
            cut_generic_type_from_class_name(class_name);
            size_t _tmp_len = strlen(class_name);
            if (class_name[_tmp_len-1] == ';')
                class_name[_tmp_len-1] = '\0';
            return class_name;
        }
        default:
            return descriptor;
    }
}

string class_simple_name(string full)
{
    if (strlen(full) == 1) {
        switch(full[0]) {
            case 'I': return (string)g_str_int;
            case 'J': return (string)g_str_long;
            case 'F': return (string)g_str_float;
            case 'D': return (string)g_str_double;
            case 'B': return (string)g_str_byte;
            case 'C': return (string)g_str_char;
            case 'S': return (string)g_str_short;
            case 'Z': return (string)g_str_boolean;
            case 'V': return (string)g_str_void;
            default: return full;
        }
    }

    switch(full[0]) {
        case '[': {
            char *last_square = strrchr(full, '[');
            int depth = (int)(last_square - full) + 1;
            char c = full[depth];
            switch (c) {
                case 'I': return class_type_array_name((string)g_str_int, depth);
                case 'J': return class_type_array_name((string)g_str_long, depth);
                case 'F': return class_type_array_name((string)g_str_float, depth);
                case 'D': return class_type_array_name((string)g_str_double, depth);
                case 'B': return class_type_array_name((string)g_str_byte, depth);
                case 'C': return class_type_array_name((string)g_str_char, depth);
                case 'S': return class_type_array_name((string)g_str_short, depth);
                case 'Z': return class_type_array_name((string)g_str_boolean, depth);
                default: {
                    // L
                    string short_name = class_path_to_short(full);
                    return class_type_array_name(short_name, depth);
                }
            }
        }
        default:
            return class_path_to_short(full);
    }
}

string class_package_name(jsource_file *jf)
{
    string path = jf->fname;
    size_t len = strlen(path);
    size_t start = len;
    char p = path[0];
    while (p != '/' && start > 0)
        p = path[--start];

    if (start == 0)
        return NULL;

    string package = x_alloc(start+1);
    memcpy(package, path, start);
    package[start] = '\0';
    // str_replace_char(package, '/', '.');
    return package;
}

void class_import(jsource_file *jf, string path)
{
    if (str_start_with(path, "java/lang") ||
        str_start_with(path, jf->fname) ||
        (jf->pname != NULL && str_start_with(path, jf->pname)) ||
        str_start_with(path, (string)g_str_int) ||
        str_start_with(path, (string)g_str_long) ||
        str_start_with(path, (string)g_str_float) ||
        str_start_with(path, (string)g_str_double) ||
        str_start_with(path, (string)g_str_byte) ||
        str_start_with(path, (string)g_str_char) ||
        str_start_with(path, (string)g_str_short) ||
        str_start_with(path, (string)g_str_boolean) ||
        str_start_with(path, (string)g_str_void) ||
        path[0] == '[')
        return;

    if (trie_search(jf->imports, path))
        return;
    trie_insert(jf->imports, path);
}

static void create_field_access_flag(jsource_file *jf,
                                     jd_field *field,
                                     str_list *list)
{
    field->access_flags_fn(field, list);
}

static void create_class_access_flag(jsource_file *jf, str_list *list)
{
    jf->access_flags_fn(jf, list);
}

static void class_fields_defination(jsource_file *jf)
{
    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];
        str_list *list = str_list_init();

        create_field_access_flag(jf, field, list);

        field_type_sig *fts = NULL;
        string signature = field->signature;
        if (signature != NULL)
            fts = parse_field_signature(signature);
        if (fts != NULL)
            str_concat(list, field_type_sig_to_s(fts));
        else {
            string type = class_simple_name(field->type);
            str_concat(list, type);
        }
        str_concat(list, " ");
        str_concat(list, field->name);
        field->defination = str_join(list);

        DEBUG_PRINT("[field]: %s\n", field->defination);
    }
}

static void class_methods_defination(jsource_file *jf)
{
    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (method_is_lambda(m))
            continue;
        m->defination = create_method_defination(m);
    }
}

static void class_defination_with_signature(jsource_file *jf)
{
    str_list *list = str_list_init();
    jclass_file *jc = jf->jclass;

    create_class_access_flag(jf, list);

    list_object *ftps = NULL;
    class_type_sig *bs = NULL;

    string signature = jf->signature;
    class_signature *cs = NULL;
    if (signature != NULL) {
        cs = parse_class_signature(signature);
        ftps = cs->formal_type_parameters;
        bs = cs->base_class;
    }

    str_concat(list, jf->sname);

    if (cs != NULL && !is_list_empty(ftps)) {
        string buf = formal_type_parameters_to_s(ftps);
        str_concat(list, buf);
    }

    // TODO:
    if (cs != NULL) {
        if (class_has_flag(jc, CLASS_ACC_ENUM)) {
            if (cs->base_class->path->size > 1) {
                str_concat(list, " extends ");
                for (int i = 0; i < cs->base_class->path->size; ++i) {
                    simple_class_type_sig *ss = lget_obj(bs->path, i);
                    string sss = simple_class_signature_to_s(ss);
                    str_concat(list, sss);
                    if (i != cs->base_class->path->size - 1)
                        str_concat(list, ", ");
                }
            }
        }
    }

    if (cs != NULL && !is_list_empty(cs->interfaces)) {
        str_concat(list, " implements ");
        str_concat(list, interfaces_to_s(cs));
    }

    jf->defination = str_join(list);
}

static void class_defination_without_signature(jsource_file *jf)
{
    str_list *list = str_list_init();

    create_class_access_flag(jf, list);

    str_concat(list, jf->sname);

    if (!STR_EQL(jf->super_cname, g_str_Object)) {
        str_concat(list, " extends ");
        str_concat(list, jf->super_cname);
    }

    if (!is_list_empty(jf->interfaces)) {
        str_concat(list, " implements ");
        for (int i = 0; i < jf->interfaces->size; ++i) {
            string interface_name = lget_obj(jf->interfaces, i);
            str_concat(list, interface_name);
            if (i != jf->interfaces->size - 1)
                str_concat(list, ", ");
        }
    }

    jf->defination = str_join(list);
}

static void class_defination(jsource_file *jf)
{
    if (jf->signature == NULL)
        class_defination_without_signature(jf);
    else
        class_defination_with_signature(jf);
}

static void class_build_unsupport_method(jd_method *m)
{
    m->nodes = linit_object();
    jd_node *root = make_obj(jd_node);
    root->node_id = 0;
    root->type = JD_NODE_METHOD_ROOT;
    root->start_idx = 0;
    root->parent = NULL;

    root->children = linit_object();
    ladd_obj(m->nodes, root);
}

jd_node* class_root_block(jsource_file *jf)
{
    return lget_obj_first(jf->blocks);
}

jd_node* class_body_block(jsource_file *jf)
{
    jd_node *root = class_root_block(jf);
    return lget_obj(root->children, 1);
}

void class_create_blocks(jsource_file *jf)
{
    jf->blocks = linit_object();
    jd_node *root = make_obj(jd_node);
    root->type = JD_NODE_CLASS_ROOT;
    root->data = jf;
    root->children = linit_object();
    ladd_obj(jf->blocks, root);

    jd_node *package_import_block = make_obj(jd_node);
    package_import_block->type = JD_NODE_PACKAGE_IMPORT;
    package_import_block->data = jf;
    package_import_block->parent = root;
    ladd_obj(root->children, package_import_block);

    jd_node *class_block = make_obj(jd_node);
    class_block->type = JD_NODE_CLASS;
    class_block->data = jf;
    class_block->parent = root;
    class_block->children = linit_object();
    ladd_obj(root->children, class_block);


    jd_node *field_block = make_obj(jd_node);
    field_block->type = JD_NODE_FIELD;
    field_block->data = jf;
    field_block->parent = class_block;
    ladd_obj(class_block->children, field_block);

    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);

        jd_node *block = make_obj(jd_node);
        block->type = JD_NODE_METHOD;
        block->data = m;
        block->parent = class_block;
        ladd_obj(class_block->children, block);

        if (method_is_empty(m) /*|| method_is_synthetic(m)*/)
            continue;
        if (method_is_unsupport(m)) {
            // TODO: lower than 50's class file print instructions
            class_build_unsupport_method(m);
        }
        jd_node *method_root_block = lget_obj(m->nodes, 0);
        method_root_block->parent = block;
        block->children = method_root_block->children;
    }
}

void class_create_definations(jsource_file *jf)
{
    class_defination(jf);

    class_fields_defination(jf);

    class_methods_defination(jf);
}

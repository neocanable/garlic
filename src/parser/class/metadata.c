#include "common/str_tools.h"
#include "parser/class/metadata.h"
#include "decompiler/klass.h"

// <editor-fold defaultstate="collapsed" desc="class file">

static void setup_string_of_const_pool(jclass_file*);

jsource_file* init_java_source_file(jclass_file *jc)
{
    jsource_file *jf     = make_obj(jsource_file);
    jc->jfile            = jf;
    jf->jclass           = jc;
    jf->descriptors      = linit_object();
    if (jc->super_class > 0)
        jf->super_cname = pool_str(jc, jc->super_class);
    else
        jf->super_cname = (string)g_str_Object;

    jf->imports          = trie_create_node("");
    jf->source           = NULL;
    jf->descs            = hashmap_init((hcmp_fn)u2obj_cmp, 0);
    jf->descs_map        = hashmap_init((hcmp_fn)s2o_cmp, 0);
    jf->type             = JD_TYPE_JVM;
    return jf;
}

void init_java_class_content(jclass_file *jc, const char *path)
{
    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    jd_bin *bin = make_obj(jd_bin);
    bin->buffer_size = file_size;
    bin->buffer = x_alloc(file_size);
    bin->cur_off = 0;
    fread(bin->buffer, 1, file_size, file);
    jc->bin = bin;
    fclose(file);
}

jclass_file* init_java_class_from_file(const char* path)
{
    jclass_file* jc = make_obj(jclass_file);
    // init_class_opcode_hashmap(jc);
    jc->path = str_create("%s", path);
    init_java_class_content(jc, path);
    init_java_source_file(jc);
    return jc;
}

jclass_file* init_class_from_content(string path, string buf, size_t size)
{
    jclass_file* jc = make_obj(jclass_file);
    // init_class_opcode_hashmap(jc);
    jc->path = str_create("%s", path);
    jc->bin = make_obj(jd_bin);
    jc->bin->buffer = buf;
    jc->bin->cur_off = 0;
    jc->bin->buffer_size = size;
    init_java_source_file(jc);
    return jc;
}

void parse_class_finish(jclass_file* jclass)
{
    if (close(jclass->fd) < 0)
        fprintf(stderr, "error close file: \n");
    else
        DEBUG_PRINT("succ close file: \n");
    mem_free_pool();
    DEBUG_PRINT("File %s finished!\n", jclass->path);
}

static void parse_class(jclass_file *jc)
{
    jclass_read4(jc, &jc->magic);
    jclass_read2(jc, &jc->minor_version);
    jclass_read2(jc, &jc->major_version);

    parse_constant_pool_section(jc);
    setup_string_of_const_pool(jc);

    jclass_read2(jc, &jc->access_flags);
    jclass_read2(jc, &jc->this_class);
    jclass_read2(jc, &jc->super_class);

    parse_interfaces_section(jc);
    parse_fields_section(jc);
    parse_methods_section(jc);

    jclass_read2(jc, &jc->attributes_count);
    uint16_t _attr_count = be16toh(jc->attributes_count);
    if (_attr_count > 0) {
        jattr *attributes = parse_attributes_section(jc, _attr_count);
        jc->attributes = attributes;
    }
}

jclass_file* parse_class_content_from_jar_entry(jd_jar_entry *e)
{
    jclass_file *jc = init_class_from_content(e->path, e->buf, e->buf_size);
    parse_class(jc);
    return jc;
}

jclass_file* parse_class_content(string path, string buf, size_t size)
{
    jclass_file *jc = init_class_from_content(path, buf, size);
    parse_class(jc);
    return jc;
}

jclass_file* parse_class_file(const char* path) {
    jclass_file *jc = init_java_class_from_file(path);
    parse_class(jc);
    return jc;
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="constant pool">

static void setup_string_of_const_pool(jclass_file *jc)
{
    uint16_t pool_count = be16toh(jc->constant_pool_count);
    for (int i = 0; i < pool_count - 1; ++i) {
        jcp_info *item = &jc->constant_pool[i];
        jconst_union_info *info = item->info;
        switch (item->tag) {
            case CONST_UTF8_TAG: {
                break;
            }
            case CONST_CLASS_TAG: {
                jcp_info *utf8 = pool_item(jc, info->class->name_index);
                item->readable = str_create("%s", utf8->readable);
                break;
            }
            case CONST_INTEGER_TAG: {
                item->readable = i2a(be32toh(info->integer->bytes));
                break;
            }
            case CONST_LONG_TAG: {
                jconst_long *l = info->long_info;
                int64_t long_value = ((int64_t)(ntohl(l->high_bytes)) << 32) |
                                     (uint32_t)ntohl(l->low_bytes);
                item->readable = l2a(long_value);
                i++;
                break;
            }
            case CONST_DOUBLE_TAG: {
                jconst_double *d = info->double_info;
                int64_t dvalue = ((int64_t) (ntohl(d->high_bytes)) << 32) |
                                 (uint32_t) ntohl(d->low_bytes);
                double dval;
                memcpy(&dval, &dvalue, sizeof(double));
                item->readable = double2a(dval);
                i++;
                break;
            }
            case CONST_FLOAT_TAG: {
                float f;
                uint32_t hex = ntohl(info->float_info->bytes);
                memcpy(&f, &hex, sizeof(float));
                item->readable = double2a(f);
                break;
            }
            case CONST_FIELDREF_TAG:
            case CONST_METHODREF_TAG:
            case CONST_INTERFACEMETHODREF_TAG: {
                // Fieldref && Methodref && InterfaceMethodref are same
                jconst_fieldref *ref = item->info->fieldref;
                jcp_info *cp_class = pool_item(jc, ref->class_index);
                jconst_class *class = cp_class->info->class;

                jcp_info *cp_nt = pool_item(jc, ref->name_and_type_index);
                jconst_name_and_type *nt = cp_nt->info->name_and_type;
                string class_name = pool_str(jc, class->name_index);
                string name = pool_str(jc, nt->name_index);
                string desc = pool_str(jc, nt->descriptor_index);
                item->readable = str_create("%s.%s%s",
                                            class_name,
                                            name,
                                            desc);
                break;
            }
            case CONST_METHODHANDLE_TAG: {
                u2 reference_index = info->method_handle->reference_index;
                jcp_info *field_info = pool_item(jc, reference_index);

                jconst_fieldref *ref = field_info->info->fieldref;
                jcp_info *cp_class = pool_item(jc, ref->class_index);
                jconst_class *class = cp_class->info->class;
                jcp_info *cp_nt = pool_item(jc, ref->name_and_type_index);
                jconst_name_and_type *nt = cp_nt->info->name_and_type;
                string class_name = pool_str(jc, class->name_index);
                string name = pool_str(jc, nt->name_index);
                string desc = pool_str(jc, nt->descriptor_index);
                item->readable = str_create("%s.%s%s", class_name,
                                            name, desc);
                break;
            }
            case CONST_STRING_TAG:
            case CONST_METHODTYPE_TAG:
            case CONST_PACKAGE_TAG:
            case CONST_MODULE_TAG: {
              /*
               * const string && const methodtype && const_package && 
               * const_module same structure, all index point to const_utf8
               **/
                u2 string_index = info->string_info->string_index;
                jcp_info *_utf8 = pool_item(jc, string_index);
                item->readable = str_create("%s", _utf8->readable);
                break;
            }
            case CONST_NAMEANDTYPE_TAG: {
                jconst_name_and_type *nt = item->info->name_and_type;
                char *n1 = pool_str(jc, nt->name_index);
                char *n2 = pool_str(jc, nt->descriptor_index);
                item->readable = str_create("%s%s", n1, n2);
                break;
            }
            case CONST_DYNAMIC_TAG: {
                item->readable = str_create("dynamic");
                break;
            }
            case CONST_INVOKEDYNAMIC_TAG: {
                item->readable = str_create("invokedynamic");
                break;
            }
            default: {
                fprintf(stderr, "[error setup const tag: %d]", item->tag);
                break;
            }
        }
    }
}

void parse_constant_pool_section(jclass_file *jc)
{
    jclass_read2(jc, &jc->constant_pool_count);
    uint16_t const_pool_size = be16toh(jc->constant_pool_count) - 1;
    DEBUG_PRINT("constant_pool_count %02x is: %d\n",
                jc->constant_pool_count,
                const_pool_size);
    jc->constant_pool = make_obj_arr(jcp_info, const_pool_size);

    string str_utf8 = str_create("UTF8");
    string str_int  = str_create("Integer");
    string str_class = str_create("Class");
    string str_float = str_create("Float");
    string str_long = str_create("Long");
    string str_double = str_create("Double");
    string str_string = str_create("String");
    string str_fieldref = str_create("Fieldref");
    string str_methodref = str_create("Methodref");
    string str_interface_methodref = str_create("InterfaceMethodref");
    string str_dynamic = str_create("Dynamic");
    string str_invoke_dynamic = str_create("InvokeDynamic");
    string str_method_handle = str_create("MethodHandle");
    string str_method_type = str_create("MethodType");
    string str_module = str_create("Module");
    string str_name_and_type = str_create("NameAndType");
    string str_package = str_create("Package");


    for (int i = 0; i < const_pool_size; ++i) {
        jcp_info *item = &jc->constant_pool[i];
        item->name = NULL;
        item->readable = NULL;
        item->info = make_obj(jconst_union_info);
        jconst_union_info *info = item->info;
        jclass_read1(jc, &item->tag);

        switch (item->tag) {
            case CONST_UTF8_TAG: {
                info->utf8 = make_obj(jconst_utf8);
                jconst_utf8 *utf8 = info->utf8;

                jclass_read2(jc, &utf8->length);
                uint16_t _utf8_length = be16toh(utf8->length);
                if (_utf8_length == 0) {
                    item->readable = (string)g_str_empty;
                } else {
                    utf8->bytes = x_alloc(_utf8_length);
                    jclass_read(jc, utf8->bytes, _utf8_length);
                    item->readable = x_alloc(_utf8_length+1);
                    memcpy(item->readable, utf8->bytes, _utf8_length);
                    item->readable[_utf8_length] = '\0';
                }
                item->name = str_utf8;
                break;
            }
            case CONST_CLASS_TAG: {
                info->class = make_obj(jconst_class);
                jclass_read2(jc, &info->class->name_index);
                item->name = str_class;
                break;
            }
            case CONST_INTEGER_TAG: {
                info->integer = make_obj(jconst_integer);
                jclass_read4(jc, &info->integer->bytes);
                item->name = str_int;
                break;
            }
            case CONST_FLOAT_TAG: {
                info->float_info = make_obj(jconst_float);
                jclass_read4(jc, &info->float_info->bytes);
                item->name = str_float;
                break;
            }
            case CONST_LONG_TAG: {
                info->long_info = make_obj(jconst_long);
                jclass_read4(jc, &info->long_info->high_bytes);
                jclass_read4(jc, &info->long_info->low_bytes);
                item->name = str_long;
                ++i;
                break;
            }
            case CONST_DOUBLE_TAG: {
                info->double_info = make_obj(jconst_double);
                jclass_read4(jc, &info->double_info->high_bytes);
                jclass_read4(jc, &info->double_info->low_bytes);
                item->name = str_double;
                ++i;
                break;
            }
            case CONST_DYNAMIC_TAG: {
                info->dynamic = make_obj(jconst_dynamic);
                jconst_dynamic *dynamic = make_obj(jconst_dynamic);
                jclass_read2(jc, &dynamic->bootstrap_method_attr_index);
                jclass_read2(jc, &dynamic->name_and_type_index);
                item->name = str_dynamic;
                break;
            }
            case CONST_FIELDREF_TAG: {
                info->fieldref = make_obj(jconst_fieldref);
                jclass_read2(jc, &info->fieldref->class_index);
                jclass_read2(jc, &info->fieldref->name_and_type_index);
                item->name = str_fieldref;
                break;
            }
            case CONST_METHODREF_TAG: {
                info->methodref = make_obj(jconst_methodref);
                jclass_read2(jc, &info->methodref->class_index);
                jclass_read2(jc, &info->methodref->name_and_type_index);
                item->name = str_methodref;
                break;
            }
            case CONST_INTERFACEMETHODREF_TAG: {
                info->interface_methodref = make_obj(jconst_interface_methodref);
                jconst_interface_methodref *mf = info->interface_methodref;
                jclass_read2(jc, &mf->class_index);
                jclass_read2(jc, &mf->name_and_type_index);
                item->name = str_interface_methodref;
                break;
            }
            case CONST_INVOKEDYNAMIC_TAG: {
                info->invoke_dynamic = make_obj(jconst_invoke_dynamic);
                jconst_invoke_dynamic *dy = info->invoke_dynamic;
                jclass_read2(jc, &dy->bootstrap_method_attr_index);
                jclass_read2(jc, &dy->name_and_type_index);
                item->name = str_invoke_dynamic;
                break;
            }
            case CONST_METHODHANDLE_TAG: {
                info->method_handle = make_obj(jconst_method_handle);
                jconst_method_handle *mh = info->method_handle;
                jclass_read1(jc, &mh->reference_kind);
                jclass_read2(jc, &mh->reference_index);
                item->name = str_method_handle;
                break;
            }
            case CONST_METHODTYPE_TAG: {
                info->method_type = make_obj(jconst_method_type);
                jclass_read2(jc, &info->method_type->descriptor_index);
                item->name = str_method_type;
                break;
            }
            case CONST_MODULE_TAG: {
                info->module = make_obj(jconst_module);
                jclass_read2(jc, &info->module->name_index);
                item->name = str_module;
                break;
            }
            case CONST_NAMEANDTYPE_TAG: {
                info->name_and_type = make_obj(jconst_name_and_type);
                jconst_name_and_type *nt = info->name_and_type;
                jclass_read2(jc, &nt->name_index);
                jclass_read2(jc, &nt->descriptor_index);
                item->name = str_name_and_type;
                break;
            }
            case CONST_PACKAGE_TAG: {
                info->package = make_obj(jconst_package);
                jclass_read2(jc, &info->package->name_index);
                item->name = str_package;
                break;
            }
            case CONST_STRING_TAG: {
                info->string_info = make_obj(jconst_string);
                jclass_read2(jc, &info->string_info->string_index);
                item->name = str_string;
                break;
            }
            default: {
                fprintf(stderr, "const poll error: %s\n", jc->path);
                abort();
            }
        }

    }
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="class field">
void parse_fields_section(jclass_file* jc)
{
    jclass_read2(jc, &jc->fields_count);
    uint16_t field_max_count = be16toh(jc->fields_count);
    jc->fields = make_obj_arr(jfield, field_max_count);

    jsource_file *jf = jc->jfile;
    jf->fields_count = field_max_count;
//    jf->fields = make_obj_arr(jd_field, jf->fields_count);

    for (int i = 0; i < field_max_count; ++i) {
        jfield *item = &jc->fields[i];
//        jd_field *field = &jf->fields[i];
//        field->meta = item;
//        field->access_flags = be16toh(item->access_flags);

        jclass_read(jc, item, sizeof(u2) * 4);

        uint16_t _attr_count = be16toh(item->attributes_count);
        if (_attr_count > 0) {
            jattr* attributes = parse_attributes_section(jc, _attr_count);
            item->attributes = attributes;
        }
    }
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="methods">

void parse_methods_section(jclass_file* jc)
{
    jclass_read2(jc, &jc->methods_count);
    uint16_t methods_count = be16toh(jc->methods_count);
    jc->methods = make_obj_arr(jmethod, methods_count);
    jsource_file *jf = jc->jfile;
    jf->methods_count = methods_count;
    jf->methods = linit_object();

    DEBUG_PRINT("Method_count is: %d\n", methods_count);
    for (int i = 0; i < methods_count; ++i) {
        jmethod *item = &jc->methods[i];
        jd_method *m = make_obj(jd_method);
        m->meta_method = item;
        m->jfile = jf;
        ladd_obj(jf->methods, m);
        jclass_read(jc, item, sizeof(u2) * 4);

        uint16_t _attr_count = be16toh(item->attributes_count);
        if (_attr_count > 0) {
            jattr* attributes = parse_attributes_section(jc, _attr_count);
            item->attributes = attributes;
        }
        for (int j = 0; j < _attr_count; ++j) {
            jattr *_attr = &item->attributes[j];
            if (STR_EQL(_attr->name, "Code"))
                item->code_attribute = (jattr_code*)_attr->info;
        }
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="class interface">

void parse_interfaces_section(jclass_file* jc)
{
    jclass_read2(jc, &jc->interfaces_count); // get interface_count

    uint16_t interface_count = be16toh(jc->interfaces_count);
    jc->interfaces = make_obj_arr(u2, interface_count);

    jclass_read(jc, jc->interfaces, interface_count * sizeof(u2));

    jsource_file *jf = jc->jfile;
    jf->interfaces = linit_object();
    for (int i = 0; i < interface_count; ++i) {
        string _name = pool_str(jc, jc->interfaces[i]);
        ladd_obj(jf->interfaces, class_simple_name(_name));
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="attributes">
static void parse_attr_unsupport(jclass_file *jc, jattr *attr);
static void parse_attr_constant_value(jclass_file *jc, jattr *attr);
static void parse_attr_code(jclass_file *jc, jattr *attribute);
static void parse_attr_stack_map_table(jclass_file *jc, jattr *attr);
static void parse_attr_exceptions(jclass_file *jc, jattr* attr);
static void parse_attr_inner_classes(jclass_file *jc, jattr* attr);
static void parse_attr_enclosing_method(jclass_file *jc, jattr* attr);
static void parse_attr_synthetic(jclass_file *jc, jattr* attr);
static void parse_attr_signature(jclass_file *jc, jattr* attr);
static void parse_attr_source_file(jclass_file *jc, jattr* attr);
static void parse_attr_source_debug_extension(jclass_file *jc, jattr* attr);
static void parse_attr_line_number_table(jclass_file *jc, jattr* attr);
static void parse_attr_local_variable_table(jclass_file *jc, jattr* attr);
static void parse_attr_local_variable_type_table(jclass_file *jc, jattr* attr);
static void parse_attr_deprecated(jclass_file *jc, jattr* attr);
static void parse_attr_rv_annotations(jclass_file *jc, jattr* attr);
static void parse_attr_riv_annotations(jclass_file *jc, jattr* attr);
static void parse_attr_rvp_annotations(jclass_file *jc, jattr* attr);
static void parse_attr_rivp_annotations(jclass_file *jc, jattr* attr);
static void parse_attr_rvt_annotations(jclass_file *jc, jattr* attr);
static void parse_attr_rivt_annotations(jclass_file *jc, jattr* attr);
static void parse_attr_annotation_default(jclass_file *jc, jattr* attr);
static void parse_attr_bootstrap_methods(jclass_file *jc, jattr* attr);
static void parse_attr_method_parameters(jclass_file *jc, jattr* attr);
static void parse_attr_module(jclass_file *jc, jattr* attribute);
static void parse_attr_module_packages(jclass_file *jc, jattr* attr);
static void parse_attr_module_main_class(jclass_file *jc, jattr* attr);
static void parse_attr_nest_host(jclass_file *jc, jattr* attr);
static void parse_attr_nest_members(jclass_file *jc, jattr* attr);
static void parse_attr_record(jclass_file *jc, jattr* attr);
static void parse_attr_permitted_subclasses(jclass_file *jc, jattr* attr);
static void parse_attr_empty(jclass_file *jc, jattr *attribute);


static void parse_annotation_element_value(jclass_file *jc, element_value *v);
static void parse_annotation(jclass_file *jc, annotation *annotation);
static void parse_type_annotation(jclass_file *jc, type_annotation *annotation);

typedef void (*attribute_parser)(jclass_file *jc, jattr *attribute);
typedef struct attribute_parser_mapper
{
    char *name;
    attribute_parser parser_fn;
} attribute_parser_mapper;


static struct attribute_parser_mapper parser_mapper[] = {
{"ConstantValue",                        parse_attr_constant_value},
{"Code",                                 parse_attr_code },
{"StackMapTable",                        parse_attr_stack_map_table},
{"Exceptions",                           parse_attr_exceptions},
{"InnerClasses",                         parse_attr_inner_classes},
{"EnclosingMethod",                      parse_attr_enclosing_method},
{"Synthetic",                            parse_attr_synthetic},
{ "Signature",                           parse_attr_signature },
{"SourceFile",                           parse_attr_source_file},
{"SourceDebugExtension",                 parse_attr_source_debug_extension},
{"LineNumberTable",                      parse_attr_line_number_table },
{"LocalVariableTable",                   parse_attr_local_variable_table},
{"LocalVariableTypeTable",               parse_attr_local_variable_type_table},
{"Deprecated",                           parse_attr_deprecated},
{"RuntimeVisibleAnnotations",            parse_attr_rv_annotations},
{"RuntimeInvisibleAnnotations",          parse_attr_riv_annotations},
{"RuntimeVisibleParameterAnnotations",   parse_attr_rvp_annotations},
{"RuntimeInvisibleParameterAnnotations", parse_attr_rivp_annotations},
{"RuntimeVisibleTypeAnnotations",        parse_attr_rvt_annotations},
{"RuntimeInvisibleTypeAnnotations",      parse_attr_rivt_annotations},
{"AnnotationDefault",                    parse_attr_annotation_default},
{"BootstrapMethods",                     parse_attr_bootstrap_methods},
{"MethodParameters",                     parse_attr_method_parameters},
{"Module",                               parse_attr_module},
{"ModulePackages",                       parse_attr_module_packages},
{"ModuleMainClass",                      parse_attr_module_main_class},
{"NestHost",                             parse_attr_nest_host},
{"NestMembers",                          parse_attr_nest_members},
{"Record",                               parse_attr_record},
{"PermittedSubclasses",                  parse_attr_permitted_subclasses},
};


jattr* parse_attributes_section(jclass_file *jc , uint16_t size) {
    // https://docs.oracle.com/javase/specs/jvms/se15/html/jvms-4.html#jvms-4.7
    int jvm_attribute_type_size = 28;

    jattr *attributes = make_obj_arr(jattr, size);
    for (int i = 0; i < size; ++i) {
        jattr *attribute = &attributes[i];
        jclass_read2(jc, &attribute->name_index);
        jclass_read4(jc, &attribute->length);
        uint32_t attribute_length_in_bytes = be32toh(attribute->length);
        if (attribute->name_index > 0)
            attribute->name = pool_str(jc, attribute->name_index);
        else
            attribute->name = str_dup(g_str_unknown);

        int parseable = 0;
        for (int j = 0; j < jvm_attribute_type_size; ++j) {
            attribute_parser_mapper fn_map = parser_mapper[j];
            if(!STR_EQL(attribute->name, fn_map.name))
                continue;
            fn_map.parser_fn(jc, attribute);
            parseable = 1;
        }

        if (parseable == 0) {
            lseek(jc->fd, attribute_length_in_bytes, SEEK_CUR);
            DEBUG_PRINT("unsupportable!!! : %s\n", jc->path);
        }
    }
    return attributes;
}

static void parse_annotation(jclass_file *jc, annotation *annotation)
{
    jclass_read2(jc, &annotation->type_index);
    jclass_read2(jc, &annotation->num_element_value_pairs);
    uint16_t _num_element_value_pairs = be16toh(annotation->num_element_value_pairs);
    if (_num_element_value_pairs == 0)
        return;
    size_t _length = _num_element_value_pairs * sizeof(annotation_element_value_pairs);
    annotation->element_value_pairs = x_alloc(_length);
    for (int k = 0; k < _num_element_value_pairs; ++k) {
        annotation_element_value_pairs *pair = &annotation->element_value_pairs[k];
        jclass_read2(jc, &pair->element_name_index);
        pair->value = make_obj(element_value);
        parse_annotation_element_value(jc, pair->value);
    }
}

static void parse_annotation_element_value(jclass_file *jc, element_value *v)
{
    jclass_read1(jc, &v->tag);
    v->union_value = make_obj(element_value_union);
    element_value_union *uvalue = v->union_value;
    switch (v->tag) {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z':
        case 's': {
            jclass_read2(jc, &v->union_value->const_value_index);
            break;
        }
        case 'e': {
            uvalue->enum_const_value = make_obj(element_value_enum_const_value);
            jclass_read2(jc, &uvalue->enum_const_value->type_name_index);
            jclass_read2(jc, &uvalue->enum_const_value->const_name_index);
            break;
        }
        case 'c': {
            jclass_read2(jc, &uvalue->class_info_index);
            break;
        }
        case '@': {
            uvalue->annotation_value = make_obj(annotation);
            parse_annotation(jc, uvalue->annotation_value);
            break;
        }
        case '[': {
            uvalue->array_value = make_obj(element_value_array_value);
            jclass_read2(jc, &uvalue->array_value->num_values);
            uint16_t _length = be16toh(uvalue->array_value->num_values);
            uvalue->array_value->values = make_obj_arr(element_value, _length);
            for (int i = 0; i < _length; ++i)
                parse_annotation_element_value(jc, &uvalue->array_value->values[i]);
            break;
        }
        default: {
            fprintf(stderr, "error parse annotation element v\n");
            abort();
        }
    }
}

static void parse_type_annotation(jclass_file *jc, type_annotation *annotation) {
    jclass_read1(jc, &annotation->target_type);
    annotation->target_info = make_obj(type_annotation_target_info);
    annotation->target_path = make_obj(type_path);
    type_annotation_target_info *target_info = annotation->target_info;
    type_path *target_path = annotation->target_path;
    switch (annotation->target_type) {
        case 0x00:
        case 0x01: {
            target_info->type_parameter_target = make_obj(type_parameter_target);
            jclass_read1(jc, &target_info->type_parameter_target->type_parameter_index);
            break;
        }
        case 0x10: {
            target_info->supertype_target = make_obj(supertype_target);
            jclass_read2(jc, &target_info->supertype_target->supertype_index);
            break;
        }
        case 0x11:
        case 0x12: {
            target_info->type_parameter_target = make_obj(type_parameter_target);
            jclass_read1(jc, &target_info->type_parameter_bound_target->type_parameter_index);
            jclass_read1(jc, &target_info->type_parameter_bound_target->bound_index);
            break;
        }
        case 0x13:
        case 0x14:
        case 0x15: {
            // no need to read, its empty target
            break;
        }
        case 0x16: {
            target_info->formal_parameter_target = make_obj(formal_parameter_target);
            jclass_read1(jc, &target_info->formal_parameter_target->formal_parameter_index);
            break;
        }
        case 0x17: {
            target_info->throws_target = make_obj(throws_target);
            jclass_read2(jc, &target_info->throws_target->throws_type_index);
            break;
        }
        case 0x40:
        case 0x41: {
            target_info->localvar_target = make_obj(localvar_target);
            jclass_read2(jc, &target_info->localvar_target->table_length);
            uint16_t _table_length = be16toh(target_info->localvar_target->table_length);
            size_t _length = _table_length * sizeof(localvar_target_table);
            target_info->localvar_target->table = x_alloc(_length);
            for (int i = 0; i < _table_length; ++i) {
                localvar_target_table *table = &target_info->localvar_target->table[i];
                jclass_read2(jc, &table->start_pc);
                jclass_read2(jc, &table->length);
                jclass_read2(jc, &table->index);
            }
            break;
        }
        case 0x42: {
            target_info->catch_target = make_obj(catch_target);
            jclass_read2(jc, &target_info->catch_target->exception_table_index);
            break;
        }
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46: {
            target_info->offset_target = make_obj(offset_target);
            jclass_read2(jc, &target_info->offset_target->offset);
            break;
        }
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b: {
            target_info->type_argument_target = make_obj(type_argument_target);
            jclass_read2(jc, &target_info->type_argument_target->offset);
            jclass_read1(jc, &target_info->type_argument_target->type_argument_index);
            break;
        }
        default: {
            fprintf(stderr, "error parse type annotation\n");
            abort();
        }
    }

    jclass_read1(jc, &target_path->path_length);
    uint16_t _path_length = target_path->path_length;
    target_path->path = make_obj_arr(type_path_list, _path_length);
    for (int i = 0; i < _path_length; ++i) {
        jclass_read1(jc, &target_path->path[i].type_path_kind);
        jclass_read1(jc, &target_path->path[i].type_argument_index);
    }
    jclass_read2(jc, &annotation->type_index);
    jclass_read2(jc, &annotation->num_element_value_pairs);
    uint16_t _num_element_value_pairs = be16toh(annotation->num_element_value_pairs);
    size_t _length = _num_element_value_pairs * sizeof(annotation_element_value_pairs);
    annotation->element_value_pairs = x_alloc(_length);
    for (int i = 0; i < _num_element_value_pairs; ++i) {
        annotation_element_value_pairs *pair = &annotation->element_value_pairs[i];
        jclass_read2(jc, &pair->element_name_index);
        pair->value = make_obj(element_value);
        parse_annotation_element_value(jc, pair->value);
    }
}

static void parse_attr_rvt_annotations(jclass_file *jc, jattr* attr)
{
    jattr_runtime_visible_type_annotations *item = make_obj(jattr_runtime_visible_type_annotations);
    jclass_read2(jc, &item->num_annotations);
    uint16_t _num_annotations = be16toh(item->num_annotations);
    attr->info = (u1*)item;
    if (_num_annotations == 0)
        return;
    item->annotations = make_obj_arr(type_annotation, _num_annotations);
    for (int i = 0; i < _num_annotations; ++i)
        parse_type_annotation(jc, &item->annotations[i]);
}

static void parse_attr_rivt_annotations(jclass_file *jc, jattr* attr)
{
    parse_attr_rvt_annotations(jc, attr);
}

static void parse_attr_annotation_default(jclass_file *jc, jattr* attr)
{
    jattr_annotation_default *annotation = make_obj(jattr_annotation_default);
    annotation->default_value = make_obj(element_value);
    parse_annotation_element_value(jc, annotation->default_value);
}

static void parse_attr_riv_annotations(jclass_file *jc, jattr* attr)
{
    // same as parse_runtime_visible_annotations_attribute
    parse_attr_rv_annotations(jc, attr);
}

static void parse_attr_rv_annotations(jclass_file *jc, jattr* attr)
{
    jattr_runtime_visible_annotations *annotation_attr = make_obj(jattr_runtime_visible_annotations);
    jclass_read2(jc, &annotation_attr->num_annotations);
    uint16_t _length = be16toh(annotation_attr->num_annotations);
    attr->info = (u1*)annotation_attr;
    if (_length == 0)
        return;
    annotation_attr->annotations = make_obj_arr(annotation, _length);
    for (int j = 0; j < _length; ++j)
        parse_annotation(jc, &annotation_attr->annotations[j]);
}

static void parse_attr_rvp_annotations(jclass_file *jc, jattr* attr)
{
    jattr_runtime_visible_parameter_annotations *item = make_obj(jattr_runtime_visible_parameter_annotations);
    jclass_read1(jc, &item->num_parameters);
    uint8_t _length = item->num_parameters;
    attr->info = (u1*)item;
    if (_length == 0)
        return;
    item->annotations = make_obj_arr(jattr_parameters_annotations, _length);
    for (int j = 0; j < _length; ++j) {
        jattr_parameters_annotations *parameter_annotations = &item->annotations[j];
        jclass_read2(jc, &parameter_annotations->num_annotations);
        uint16_t _num_annotations = be16toh(parameter_annotations->num_annotations);
        if (_num_annotations == 0)
            continue;
        parameter_annotations->annotations = make_obj_arr(annotation, _num_annotations);
        for (int k = 0; k < _num_annotations; ++k)
            parse_annotation(jc, &parameter_annotations->annotations[k]);
    }
}

static void parse_attr_rivp_annotations(jclass_file *jc, jattr* attr)
{
    parse_attr_rvp_annotations(jc, attr);
}

static void parse_attr_empty(jclass_file *jc, jattr *attribute)
{
    // do nothing
}

static void parse_attr_unsupport(jclass_file *jc, jattr *attr)
{
    // seek;
}

static void parse_attr_line_number_table(jclass_file *jc, jattr* attr)
{
    jattr_line_number_table *lnt = make_obj(jattr_line_number_table);
    jclass_read2(jc, &lnt->line_number_table_length);
    uint16_t _length = be16toh(lnt->line_number_table_length);
    attr->info = (u1*) lnt;
    if (_length == 0)
        return;
    lnt->line_number_table = make_obj_arr(jattr_line_number, _length);
    size_t _size = _length * sizeof(jattr_line_number);
    jclass_read(jc, lnt->line_number_table, _size);
}

static void parse_attr_local_variable_table(jclass_file *jc, jattr* attr)
{
    jattr_local_variable_table *lvt = make_obj(jattr_local_variable_table);
    jclass_read2(jc, &lvt->local_variable_table_length);
    uint16_t _length = be16toh(lvt->local_variable_table_length);
    attr->info = (u1*)lvt;
    if (_length == 0)
        return;

    jattr_local_variable *table = make_obj_arr(jattr_local_variable, _length);

    size_t _size = _length * sizeof(jattr_local_variable);
    jclass_read(jc, table, _size);
    lvt->local_variable_table = table;
}

static void parse_attr_local_variable_type_table(jclass_file *jc, jattr* attr)
{
    jattr_lvtt *lvtt = make_obj(jattr_lvtt);
    jclass_read2(jc, &lvtt->local_variable_type_table_length);
    uint16_t _length = be16toh(lvtt->local_variable_type_table_length);
    attr->info = (u1*)lvtt;
    if (_length == 0)
        return;
    jattr_ltv *table = make_obj_arr(jattr_ltv, _length);
    jclass_read(jc, table, _length * sizeof(jattr_ltv));
    lvtt->local_variable_type_table = table;
}

static void parse_attr_code(jclass_file *jc, jattr* attribute)
{
    jattr_code *code_attr = make_obj(jattr_code);
    jclass_read(jc, code_attr, sizeof(u2) * 2 + sizeof(u4));
    code_attr->code = x_alloc(be32toh(code_attr->code_length));
    jclass_read(jc, code_attr->code, be32toh(code_attr->code_length));

    jclass_read2(jc, &code_attr->exception_table_length);
    uint16_t _length = be16toh(code_attr->exception_table_length);
    if (_length > 0) {
        jattr_code_exception_table *table = make_obj_arr(jattr_code_exception_table, _length);
        jclass_read(jc, table, _length * sizeof(jattr_code_exception_table));
        code_attr->exception_table = table;
    }

    jclass_read2(jc, &code_attr->attributes_count);
    uint16_t _count_inner = be16toh(code_attr->attributes_count);
    if (_count_inner > 0) {
        jattr *code_attributes = parse_attributes_section(jc, _count_inner);
        code_attr->attributes = code_attributes;
    }
    attribute->info = (u1*)code_attr;
}

static void parse_attr_stack_map_table(jclass_file *jc, jattr* attr)
{
    jattr_stack_map_table *smt = make_obj(jattr_stack_map_table);
    jclass_read2(jc, &smt->number_of_entries);
    uint16_t _length = be16toh(smt->number_of_entries);
    attr->info = (u1*)smt;

    if (_length == 0)
        return;

    stack_map_frame *entries = make_obj_arr(stack_map_frame, _length);
    for (int j = 0; j < _length; ++j) {
        stack_map_frame *item = &entries[j];
        // NOTE: all stack map entries first byte are same
        u1 frame_type;
        jclass_read1(jc, &frame_type);
        if (frame_type >= 0 && frame_type <= 63) {
            item->same_frame = make_obj(same_frame);
            item->same_frame->frame_type = frame_type; // same_frame
        }
        else if (frame_type >= 64 && frame_type <= 127) {
            item->same_locals_1_stack_item_frame = make_obj(same_locals_1_stack_item_frame);
            item->same_locals_1_stack_item_frame->frame_type = frame_type;
            item->same_locals_1_stack_item_frame->stack = make_obj(variable_info);
            variable_info *v = &item->same_locals_1_stack_item_frame->stack[0];
            jclass_read1(jc, &v->tag);
            if (v->tag >= 7)
                jclass_read2(jc, &v->offset);
        }
        else if (frame_type == 247) {
            item->same_locals_1_stack_item_frame_extended = make_obj(same_locals_1_stack_item_frame_extended);
            item->same_locals_1_stack_item_frame_extended->frame_type = frame_type;
            jclass_read2(jc, &item->same_locals_1_stack_item_frame_extended->offset_delta);

            item->same_locals_1_stack_item_frame_extended->stack = make_obj(variable_info);
            variable_info *v = &item->same_locals_1_stack_item_frame_extended->stack[0];
            jclass_read1(jc, &v->tag);
            if (v->tag >= 7)
                jclass_read2(jc, &v->offset);
        }
        else if (frame_type >= 248 && frame_type <= 250) {
            item->chop_frame = make_obj(chop_frame);
            item->chop_frame->frame_type = frame_type;
            jclass_read2(jc, &item->chop_frame->offset_delta);
        }
        else if (frame_type == 251) {
            item->same_frame_extended = make_obj(same_frame_extended);
            item->same_frame_extended->frame_type = frame_type;
            jclass_read2(jc, &item->same_frame_extended->offset_delta);
        }
        else if (frame_type >= 252 && frame_type <= 254) {
            item->append_frame = make_obj(append_frame);
            item->append_frame->frame_type = frame_type;
            jclass_read2(jc, &item->append_frame->offset_delta);
            uint16_t _length_variables = frame_type - 251;
            item->append_frame->locals = make_obj_arr(variable_info, _length_variables);
            for (int k = 0; k < _length_variables; ++k) {
                variable_info *v = &item->append_frame->locals[k];
                jclass_read1(jc, &v->tag);
                if (v->tag >= 7)
                    jclass_read2(jc, &v->offset);
            }
        }
        else if (frame_type == 255) {
            item->full_frame = make_obj(full_frame);
            item->full_frame->frame_type = frame_type;
            jclass_read2(jc, &item->full_frame->offset_delta);
            jclass_read2(jc, &item->full_frame->number_of_locals);
            uint16_t locals_length = be16toh(item->full_frame->number_of_locals);
            item->full_frame->locals = make_obj_arr(variable_info, locals_length);
            for (int k = 0; k < locals_length; ++k) {
                variable_info *v = &item->full_frame->locals[k];
                jclass_read1(jc, &v->tag);
                if (v->tag >= 7)
                    jclass_read2(jc, &v->offset);
            }

            jclass_read2(jc, &item->full_frame->number_of_stack_items);
            uint16_t stacks_length = be16toh(item->full_frame->number_of_stack_items);
            item->full_frame->stack = make_obj_arr(variable_info, stacks_length);
            for (int k = 0; k < stacks_length; ++k) {
                variable_info *v = &item->full_frame->stack[k];
                jclass_read1(jc, &v->tag);
                if (v->tag >= 7)
                    jclass_read2(jc, &v->offset);
            }
        }

    }
    smt->entries = entries;
}

static void parse_attr_exceptions(jclass_file *jc, jattr* attr)
{
    jattr_exception *ex = make_obj(jattr_exception);
    jclass_read2(jc, &ex->number_of_exceptions);
    uint16_t _length = be16toh(ex->number_of_exceptions);
    attr->info = (u1*)ex;
    if (_length == 0)
        return;
    u2 *table = make_obj_arr(u2, _length);
    jclass_read(jc, table, _length * sizeof(u2));
    //    for (int j = 0; j < _length; ++j) {
    //        u2 *item = &table[j];
    //        jclass_read2(meta, item);
    //    }
    ex->exception_index_table = table;
}

static void parse_attr_constant_value(jclass_file *jc, jattr* attr)
{
    jattr_constant_value *cv = make_obj(jattr_constant_value);
    jclass_read2(jc, &cv->value_index);
    attr->info = (u1*)cv;
}

static void parse_attr_bootstrap_methods(jclass_file *jc, jattr* attr)
{
    jattr_bootstrap_methods *bm = make_obj(jattr_bootstrap_methods);
    jclass_read2(jc, &bm->num_bootstrap_methods);
    uint16_t size = be16toh(bm->num_bootstrap_methods);
    attr->info = (u1*)bm;
    if (size == 0)
        return;
    jclass_bootstrap_method *list = make_obj_arr(jclass_bootstrap_method, size);
    for (int j = 0; j < size; ++j) {
        jclass_bootstrap_method *item = &list[j];
        jclass_read2(jc, &item->bootstrap_method_ref);
        jclass_read2(jc, &item->num_bootstrap_arguments);
        uint16_t _arg_length = be16toh(item->num_bootstrap_arguments);

        if (_arg_length == 0)
            continue;
        u2 *_args = make_obj_arr(u2, _arg_length);
        jclass_read(jc, _args, _arg_length * sizeof(u2));
        item->bootstrap_arguments = _args;
    }
    bm->bootstrap_methods = list;
}

static void parse_attr_nest_host(jclass_file *jc, jattr* attr)
{
    jattr_nest_host *nest = make_obj(jattr_nest_members);
    jclass_read2(jc, &nest->host_class_index);
    attr->info = (u1*)nest;
}

static void parse_attr_nest_members(jclass_file *jc, jattr* attr)
{
    jattr_nest_members* nest = make_obj(jattr_nest_members);
    jclass_read2(jc, &nest->number_of_classes);
    uint16_t _length = be16toh(nest->number_of_classes);
    attr->info = (u1*)nest;
    if (_length == 0)
        return;
    nest->classes = make_obj_arr(u2, _length);
    jclass_read(jc, nest->classes, _length * sizeof(u2));
}

static void parse_attr_inner_classes(jclass_file *jc, jattr* attr)
{
    jattr_inner_classes* inner = make_obj(jattr_inner_classes);

    jclass_read2(jc, &inner->number_of_classes);
    uint16_t _length = be16toh(inner->number_of_classes);
    attr->info = (u1*) inner;
    if (_length == 0)
        return;
    inner->classes = make_obj_arr(jattr_inner_class, _length);
    size_t _inner_size = _length * sizeof(jattr_inner_class);
    jclass_read(jc, inner->classes, _inner_size);
}

static void parse_attr_enclosing_method(jclass_file *jc, jattr* attr)
{
    jattr_enclosing_method *em = make_obj(jattr_enclosing_method);
    jclass_read(jc, &em->class_index, sizeof(jattr_enclosing_method));
    attr->info = (u1*)em;
}

static void parse_attr_signature(jclass_file *jc, jattr* attr)
{
    jattr_signature *s = make_obj(jattr_signature);
    jclass_read2(jc, &s->signature_index);
    attr->info = (u1*) s;
}

static void parse_attr_method_parameters(jclass_file *jc, jattr* attr)
{
    jattr_method_parameters* mp = make_obj(jattr_method_parameters);
    jclass_read1(jc, &mp->parameters_count);
    uint16_t _length = mp->parameters_count;
    attr->info = (u1*)mp;
    if (_length == 0)
        return;

    mp->parameters = make_obj_arr(jattr_method_parameter, _length);
    size_t _param_size = _length * sizeof(jattr_method_parameter);
    jclass_read(jc, mp->parameters, _param_size);
}

static void parse_attr_source_debug_extension(jclass_file *jc, jattr* attr)
{
    jattr_source_debug_extension *s = make_obj(jattr_source_debug_extension);
    s->debug_extension = x_alloc(be32toh(attr->length));
    jclass_read(jc, s->debug_extension, be32toh(attr->length));
    attr->info = (u1*)s;
}

static void parse_attr_source_file(jclass_file *jc, jattr* attr)
{
    jattr_source_file *sf = make_obj(jattr_source_file);
    jclass_read2(jc, &sf->sourcefile_index);

    attr->info = (u1*) sf;
}

static void parse_attr_module(jclass_file *jc, jattr* attribute)
{
    jattr_module *item = make_obj(jattr_module);
    attribute->info = (u1*)item;
    jclass_read(jc, item, 4 * sizeof(u2));
    uint16_t _requires_count = be16toh(item->requires_count);
    if (_requires_count > 0) {
        item->requires = make_obj_arr(module_attr_requires, _requires_count);
        size_t _req_size = _requires_count * sizeof(module_attr_requires);
        jclass_read(jc, item->requires, _req_size);
    }

    jclass_read2(jc, &item->exports_count);
    uint16_t _exports_count = be16toh(item->exports_count);
    if (_exports_count > 0) {
        item->exports = make_obj_arr(module_attr_exports, _exports_count);
        for (int i = 0; i < _exports_count; ++i) {
            module_attr_exports *export = &item->exports[i];

            jclass_read(jc, export, 3 * sizeof(u2));
            uint16_t _exports_to_count = be16toh(export->exports_to_count);
            if (_exports_to_count > 0) {
                export->exports_to_index = make_obj_arr(u2, _exports_to_count);
                jclass_read(jc,
                            export->exports_to_index,
                            _exports_to_count * sizeof(u2));
            }
        }
    }

    jclass_read2(jc, &item->opens_count);
    uint16_t _opens_count = be16toh(item->opens_count);
    if (_opens_count > 0) {
        item->opens = make_obj_arr(module_attr_opens, _opens_count);
        for (int i = 0; i < _opens_count; ++i) {
            module_attr_opens *open = &item->opens[i];
            // jclass_read2(meta, &open->opens_index);
            // jclass_read2(meta, &open->opens_flags);
            // jclass_read2(meta, &open->opens_to_count);
            jclass_read(jc, open, 3 * sizeof(u2));
            uint16_t _opens_to_count = be16toh(open->opens_to_count);
            if (_opens_to_count == 0)
                continue;

            open->opens_to_index = make_obj_arr(u2, _opens_to_count);
            size_t _opens_size = _opens_to_count * sizeof(u2);
            jclass_read(jc, open->opens_to_index, _opens_size);
        }
    }

    jclass_read2(jc, &item->uses_count);
    uint16_t _uses_count = be16toh(item->uses_count);
    if (_uses_count > 0) {
        item->uses_index = make_obj_arr(u2, _uses_count);
        size_t _uses_size = _uses_count * sizeof(u2);
        jclass_read(jc, item->uses_index, _uses_size);
    }

    jclass_read2(jc, &item->provides_count);
    uint16_t _provides_count = be16toh(item->provides_count);
    if (_provides_count > 0) {
        item->provides = make_obj_arr(module_attr_provides, _provides_count);
        for (int i = 0; i < _provides_count; ++i) {
            module_attr_provides *provide = &item->provides[i];
            jclass_read2(jc, &provide->provides_index);
            jclass_read2(jc, &provide->provides_with_count);
            uint16_t _pcount = be16toh(provide->provides_with_count);
            if (_pcount == 0 )
                continue;
            provide->provides_with_index = make_obj_arr(u2, _pcount);
            size_t _provides_size = _pcount * sizeof(u2);
            jclass_read(jc, provide->provides_with_index, _provides_size);
        }
    }

}

static void parse_attr_module_packages(jclass_file *jc, jattr* attr)
{
    jattr_module_packages *mp = make_obj(jattr_module_packages);
    jclass_read2(jc, &mp->package_count);
    attr->info = (u1*)mp;
    uint16_t _package_count = be16toh(mp->package_count);
    if (_package_count == 0)
        return;
    mp->package_index = make_obj_arr(u2, _package_count);
    jclass_read(jc, mp->package_index, _package_count * sizeof(u2));
}

static void parse_attr_module_main_class(jclass_file *jc, jattr* attr)
{
    jattr_module_main_class *m = make_obj(jattr_module_main_class);
    jclass_read2(jc, &m->main_class_index);
    attr->info = (u1*)m;
}

static void parse_attr_synthetic(jclass_file *jc, jattr* attr)
{
    parse_attr_empty(jc, attr);
}

static void parse_attr_deprecated(jclass_file *jc, jattr* attr)
{
    parse_attr_empty(jc, attr);
}

static void parse_attr_record(jclass_file *jc, jattr *attr)
{
    // TODO: not tested
    jattr_record *item = make_obj(jattr_record);
    attr->info = (u1*)item;
    jclass_read2(jc, &item->component_count);
    uint16_t _component_count = be16toh(item->component_count);
    if (_component_count == 0)
        return;
    item->components = make_obj_arr(u2, _component_count);
    for (int i = 0; i < _component_count; ++i) {
        jattr_record_component *component = &item->components[i];
        jclass_read2(jc, &component->name_index);
        jclass_read2(jc, &component->descriptor_index);
        jclass_read2(jc, &component->attributes_count);
        uint16_t _attr_count = be16toh(component->attributes_count);
        if (_attr_count == 0)
            continue;
        jattr* attributes = parse_attributes_section(jc, _attr_count);
        component->attributes = attributes;
    }
}

static void parse_attr_permitted_subclasses(jclass_file *jc, jattr* attr)
{
    // TODO: not tested
    jattr_permitted_subclasses *item = make_obj(jattr_permitted_subclasses);
    jclass_read2(jc, &item->number_of_classes);
    attr->info = (u1*)item;
    uint16_t _number = be16toh(item->number_of_classes);
    if (_number == 0)
        return;

    item->classes = make_obj_arr(u2, _number);
    jclass_read(jc, item->classes, _number * sizeof(u2));

}

// </editor-fold>

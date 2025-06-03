#include <libgen.h>

#include "decompiler/method.h"
#include "decompiler/klass.h"
#include "decompiler/expression_writter.h"
#include "decompiler/descriptor.h"
#include "decompiler/expression_enum.h"
#include "decompiler/expression_node.h"
#include "jvm/jvm_annotation.h"
#include "jvm/jvm_class.h"
#include "jvm/jvm_decompile.h"
#include "jar/jar.h"
#include "common/file_tools.h"
#include "jvm_descriptor.h"
#include "jvm_method.h"


static void jvm_methods(jsource_file *jf)
{
    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (method_is_lambda(m))
            continue;
        jmethod *jm = m->meta_method;
        jvm_method(jf->jclass, m, jm);
    }
}

void jvm_init_ins_fn(jsource_file *jf)
{
    jd_ins_fn *fn = make_obj(jd_ins_fn);
    fn->is_compare = jvm_ins_is_compare;
    fn->is_return = jvm_ins_is_return;
    fn->is_void_return = jvm_ins_is_voidreturn;
    fn->is_switch = jvm_ins_is_switch;
    fn->is_goto = jvm_ins_is_goto;
    fn->is_unconditional_jump = jvm_ins_is_unconditional_jump;
    fn->is_conditional_jump = jvm_ins_is_conditional_jump;
    fn->is_goto_back = jvm_ins_is_goto_back;
    fn->is_jump_dst = ins_is_jump_destination;
    fn->is_block_start = jvm_ins_is_block_start;
    fn->is_block_end = jvm_ins_is_block_end;
    fn->is_store = jvm_ins_is_store;
    fn->is_load = jvm_ins_is_load;
    fn->is_if = jvm_ins_is_if;
    fn->is_branch = jvm_ins_is_branch;
    fn->is_athrow = jvm_ins_is_athrow;
    fn->is_invoke_dynamic = jvm_ins_is_invokedynamic;
    fn->is_invoke_interface = jvm_ins_is_invokeinterface;
    fn->is_invoke_special = jvm_ins_is_invokespecial;
    fn->is_invoke_static = jvm_ins_is_invokestatic;
    fn->is_invoke_virtual = jvm_ins_is_invokevirtual;

    jf->ins_fn = fn;
}

void jvm_init_method_fn(jsource_file *jf)
{
    jd_method_fn *fn = make_obj(jd_method_fn);
    fn->access_flags_fn = jvm_method_access_flags;
    fn->param_val_fn = jvm_method_parameter_val;
    fn->param_annotation_fn = jvm_method_parameter_annotation;
    fn->is_native = jvm_method_is_native;
    fn->is_init = jvm_method_is_init;
    fn->is_clinit = jvm_method_is_clinit;
    fn->is_abstract = jvm_method_is_abstract;
    fn->is_varargs = jvm_method_is_varargs;
    fn->is_synthetic = jvm_method_is_synthetic;
    fn->is_member = jvm_method_is_member;
    jf->method_fn = fn;
}

void jvm_analyse_class_file_inside(jsource_file *jf)
{
    jf->fname = pool_str(jf->jclass, ((jclass_file*)jf->jclass)->this_class);
    jf->pname = class_package_name(jf);
    jf->sname = class_simple_name(jf->fname);
    jf->access_flags_fn = jvm_class_access_flag;

    jvm_init_ins_fn(jf);

    jvm_init_method_fn(jf);

    jvm_collect_descriptor(jf);

    jvm_fields(jf);

    jvm_methods(jf);

    jvm_signatures(jf);

    class_create_definations(jf);

    jvm_annotations(jf);

    optimize_enum_class(jf);

    class_create_blocks(jf);
}

void jvm_analyse_class_file(jsource_file *jf)
{
    jvm_analyse_class_file_inside(jf);

    if (jf->parent == NULL) {
        writter_for_class(jf, NULL);
        if (jf->source != NULL)
            fclose(jf->source);
    }
}
#include "dalvik/dex_lambda.h"
#include "dalvik/dex_class.h"
#include "dalvik/dex_decompile.h"

#include "decompiler/klass.h"
#include "decompiler/expression.h"
#include "decompiler/descriptor.h"

jd_exp_lambda* dex_lambda(jsource_file *jf, jd_exp_invoke *invoke)
{
    if (jf->methods == NULL || jf->methods->size == 0)
       return NULL;

    jd_method *not_synthetic = NULL;
    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (!access_flags_contains(m->access_flags, ACC_DEX_SYNTHETIC) &&
            !access_flags_contains(m->access_flags, ACC_DEX_CONSTRUCTOR)) {
            not_synthetic = m;
            break;
        }
    }
    if (not_synthetic == NULL)
        return NULL;

    jd_exp *last_invoke = NULL;
    jd_dex_ins *method_last_ins = lget_obj_last(not_synthetic->instructions);
    if (dex_ins_is_return_void(method_last_ins)) {
        int index = not_synthetic->expressions->size - 2;
        if (index <= 0)
            return NULL;
        last_invoke = lget_obj(not_synthetic->expressions, index);
    }
    else if (dex_ins_is_return_object(method_last_ins) ||
            dex_ins_is_return(method_last_ins) ||
            dex_ins_is_return_wide(method_last_ins)) {
        jd_exp *exp = method_last_ins->expression;
        jd_exp_return *ret = exp->data;
        if (ret == NULL)
            return NULL;
        last_invoke = &ret->list->args[0];
    }
    if (last_invoke == NULL || !exp_is_invoke(last_invoke))
        return NULL;


    jd_exp_invoke *last_exp_invoke = last_invoke->data;


    // split invoke
    // if last_invoke is invoke_virtual, invoke_interface, invoke_direct
    // first arg is object reference
    // this object should be from initliazer's parameters which is invoke


    // check last invoke's m
    // if m is synthetic, the m should be lambda m body
    // else, m should be object::method_name

    jd_dex_ins *last_ins = last_invoke->ins;
    // if last_ins is invoke_kind
    u4 method_index = dex_ins_parameter(last_ins, 1);
    jd_meta_dex *meta = dex_ins_meta(last_ins);
    dex_method_id *method_id = &meta->method_ids[method_index];
    string method_name = dex_str_of_idx(meta, method_id->name_idx);
    encoded_method *em = hget_u4obj(meta->lambda_method_map, method_index);

    jd_exp_lambda *exp_lambda = make_obj(jd_exp_lambda);
    jd_ins_fn *fn = last_invoke->ins->fn;
    if (em == NULL) {
        // is object::method_name
        exp_lambda->method = NULL;
        exp_lambda->method_name = last_exp_invoke->method_name;
        exp_lambda->class_name = last_exp_invoke->class_name;
        exp_lambda->is_static = fn->is_invoke_static(last_invoke->ins);
    }
    else {
        jd_method *target = dex_method(jf, em);
        exp_lambda->method = target;
        exp_lambda->method_name = target->name;
        exp_lambda->class_name = last_exp_invoke->class_name;
        exp_lambda->is_static = fn->is_invoke_static(last_invoke->ins);
        jd_stack *enter = target->enter;
        for (int i = 0; i < last_exp_invoke->list->len; ++i) {
            jd_exp *arg = &last_exp_invoke->list->args[i];
            // this.f$0 should be invoke's first arg
            // this.f$1 should be invoke's second arg
            // this.f$2 should be invoke's third arg
            // etc ...

            if (exp_is_get_field(arg)) {
                jd_exp_get_field *get_field = arg->data;
                if (str_start_with(get_field->name, "f$")) {
                    string sub = sub_str(get_field->name,
                                         2,
                                         strlen(get_field->name));
                    int number = atoi(sub);
                    jd_val *enter_val = enter->local_vars[number];
                    if (enter_val == NULL)
                        continue;
                    jd_exp *arg_exp = &invoke->list->args[number];
                    jd_val *arg_val = arg_exp->data;
                    memcpy(enter_val, arg_val, sizeof(jd_val));
                }
            }
        }
    }
    exp_lambda->list = invoke->list;
    exp_lambda->list->len = invoke->list->len;
    return exp_lambda;
}

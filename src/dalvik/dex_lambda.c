#include "dalvik/dex_lambda.h"
#include "dalvik/dex_class.h"
#include "dalvik/dex_decompile.h"

#include "decompiler/klass.h"
#include "decompiler/expression.h"
#include "decompiler/descriptor.h"
#include "decompiler/method.h"
#include "parser/dex/metadata.h"

static bool is_boxing_call(jd_exp_invoke *invoke)
{
    if (invoke->class_name == NULL || invoke->method_name == NULL ||
            invoke->list == NULL || invoke->list->len == 0)
        return false;

    if (!STR_EQL(invoke->method_name, "valueOf"))
        return false;

    static const char *boxes[] = {
            "Integer", 
            "Long", 
            "Double", 
            "Float",
            "Short", 
            "Byte", 
            "Character", 
            "Boolean",
    };
    for (size_t i = 0; i < sizeof(boxes) / sizeof(boxes[0]); ++i)
        if (STR_EQL(invoke->class_name, boxes[i]))
            return true;

    return false;
}

static jd_exp *skip_boxing(jd_exp *exp)
{
    while (exp_is_invoke(exp) && is_boxing_call(exp->data))
        exp = &((jd_exp_invoke *) exp->data)->list->args[0];

    return exp;
}

static u4 dex_forwarder_target_method(encoded_method *em)
{
    if (em == NULL || em->code == NULL || em->code->insns == NULL)
        return 0;

    dex_code_item *code = em->code;
    for (int i = 0; i < code->insns_size; ) {
        u1 opcode = code->insns[i] & 0xFF;
        int len = dex_opcode_len(opcode);
        if (len <= 0)
            return 0;

        switch (opcode) {
            case DEX_INS_INVOKE_VIRTUAL:
            case DEX_INS_INVOKE_SUPER:
            case DEX_INS_INVOKE_DIRECT:
            case DEX_INS_INVOKE_STATIC:
            case DEX_INS_INVOKE_INTERFACE:
            case DEX_INS_INVOKE_VIRTUAL_RANGE:
            case DEX_INS_INVOKE_SUPER_RANGE:
            case DEX_INS_INVOKE_DIRECT_RANGE:
            case DEX_INS_INVOKE_STATIC_RANGE:
            case DEX_INS_INVOKE_INTERFACE_RANGE: {
                jd_dex_ins ins;
                memset(&ins, 0, sizeof(ins));
                ins.code = opcode;
                ins.format = dex_opcode_fmt(opcode);
                ins.param = &code->insns[i];
                return (u4) dex_ins_parameter(&ins, 1);
            }
            default:
                break;
        }
        i += len;
    }
    return 0;
}

static encoded_method* dex_encoded_method_of_id(dex_class_def *cf, u4 method_id)
{
    if (cf == NULL || cf->class_data == NULL)
        return NULL;

    dex_class_data_item *data = cf->class_data;
    for (int i = 0; i < data->direct_methods_size; ++i)
        if (data->direct_methods[i].method_id == method_id)
            return &data->direct_methods[i];
    for (int i = 0; i < data->virtual_methods_size; ++i)
        if (data->virtual_methods[i].method_id == method_id)
            return &data->virtual_methods[i];

    return NULL;
}

static bool lambda_captures_are_named(jd_exp_invoke *invoke, int captures)
{
    for (int i = 0; i < captures && i < invoke->list->len; ++i) {
        jd_exp *arg = &invoke->list->args[i];
        if (arg->type != JD_EXPRESSION_LOCAL_VARIABLE &&
                arg->type != JD_EXPRESSION_STACK_VAR)
            return false;

        jd_val *val = arg->data;
        if (val == NULL || val->name == NULL)
            return false;
    }
    return true;
}

static bool lambda_name_is_internal(string name)
{
    return name == NULL ||
           str_start_with(name, "var_") ||
           str_start_with(name, "svar_") ||
           str_start_with(name, "v_");
}

static void lambda_bind_captures(jd_method *target, jd_exp_invoke *invoke,
                                 int captures)
{
    if (target->enter == NULL || target->desc == NULL ||
            target->fn == NULL || target->fn->param_val_fn == NULL)
        return;

    if (captures < 0 || captures > target->desc->list->size)
        return;

    for (int i = 0; i < captures; ++i) {
        if (i >= invoke->list->len)
            break;

        jd_exp *arg = &invoke->list->args[i];
        if (arg->type != JD_EXPRESSION_LOCAL_VARIABLE &&
                arg->type != JD_EXPRESSION_STACK_VAR)
            continue;

        jd_val *from = arg->data;
        if (from == NULL || from->name == NULL)
            continue;

        jd_val *param = target->fn->param_val_fn(target, i);
        if (param == NULL)
            continue;

        if (param->name != NULL && !lambda_name_is_internal(param->name))
            continue;

        param->name = from->name;
    }
}


jd_lambda_facts* dex_lambda_facts(jsource_file *jf) {
    jd_lambda_facts *facts = make_obj(jd_lambda_facts);
    memset(facts, 0, sizeof(jd_lambda_facts));
    facts->kind = JD_LAMBDA_NONE;

    if (jf->methods == NULL || jf->methods->size == 0)
       return facts;
    if (jf->interfaces != NULL && jf->interfaces->size > 0)
        facts->interface_name = lget_obj(jf->interfaces, 0);

    jd_method *not_synthetic = NULL;
    jd_method *instance = NULL;
    for (int i = 0; i < jf->methods->size; ++i) {
        jd_method *m = lget_obj(jf->methods, i);
        if (m == NULL)
            continue;
        if (access_flags_contains(m->access_flags, ACC_DEX_SYNTHETIC) ||
            access_flags_contains(m->access_flags, ACC_DEX_CONSTRUCTOR))
            continue;

        if (not_synthetic == NULL)
            not_synthetic = m;
        if (instance == NULL &&
            !access_flags_contains(m->access_flags, ACC_DEX_STATIC))
            instance = m;
    }
    not_synthetic = instance != NULL ? instance : not_synthetic;
    if (not_synthetic == NULL || not_synthetic->instructions == NULL ||
            not_synthetic->expressions == NULL)
        return facts;
    if (not_synthetic->desc != NULL && not_synthetic->desc->list != NULL)
        facts->iface_arity = not_synthetic->desc->list->size;

    jd_exp *last_invoke = NULL;
    jd_dex_ins *method_last_ins = lget_obj_last(not_synthetic->instructions);
    if (method_last_ins == NULL)
        return facts;

    if (dex_ins_is_return_void(method_last_ins)) {
        int index = not_synthetic->expressions->size - 2;
        if (index < 0)
            return facts;
        last_invoke = lget_obj(not_synthetic->expressions, index);
    }
    else if (dex_ins_is_return_object(method_last_ins) ||
            dex_ins_is_return(method_last_ins) ||
            dex_ins_is_return_wide(method_last_ins)) {
        jd_exp *exp = method_last_ins->expression;
        if (exp == NULL)
            return facts;
        jd_exp_return *ret = exp->data;
        if (ret == NULL || ret->list == NULL || ret->list->len == 0)
            return facts;
        last_invoke = &ret->list->args[0];
    }
    else {
        return facts;
    }
    if (last_invoke == NULL)
        return facts;

    if (exp_is_initialize(last_invoke)) {
        jd_exp_initialize *initialize = last_invoke->data;
        if (initialize->class_name == NULL)
            return facts;
        facts->kind = JD_LAMBDA_CTOR_REF;
        facts->class_name = initialize->class_name;
        facts->method_name = (string) g_str_init;
        return facts;
    }

    if (!exp_is_invoke(last_invoke))
        return facts;

    last_invoke = skip_boxing(last_invoke);

    if (!exp_is_invoke(last_invoke))
        return facts;

    jd_exp_invoke *last_exp_invoke = last_invoke->data;

    jd_dex_ins *last_ins = last_invoke->ins;
    if (last_ins == NULL || last_ins->fn == NULL ||
            last_ins->method == NULL || last_ins->method->meta == NULL)
        return facts;

    // if last_ins is invoke_kind
    u4 method_index = dex_ins_parameter(last_ins, 1);
    jd_meta_dex *meta = dex_ins_meta(last_ins);
    if (meta == NULL || meta->method_ids == NULL ||
            method_index >= meta->header->method_ids_size)
        return facts;
    facts->meta = meta;
    facts->method_index = method_index;

    encoded_method *em = hget_u4obj(meta->lambda_method_map, method_index);
    jd_ins_fn *fn = last_ins->fn;
    if (em == NULL) {
        if (last_exp_invoke->class_name == NULL ||
                last_exp_invoke->method_name == NULL)
            return facts;
        facts->kind = JD_LAMBDA_REFERENCE;
        facts->method_name = last_exp_invoke->method_name;
        facts->class_name = last_exp_invoke->class_name;
        facts->is_static = fn->is_invoke_static(last_invoke->ins);
    }
    else {
        facts->kind = JD_LAMBDA_BODY;
        facts->body = em;
        facts->class_name = last_exp_invoke->class_name;
        facts->is_static = fn->is_invoke_static(last_invoke->ins);
    }
    return facts;
}

static encoded_method* lambda_forwarder_body(jd_meta_dex *meta,
                                             jd_lambda_facts *facts,
                                             encoded_method **out_forwarder)
{
    if (out_forwarder != NULL)
        *out_forwarder = NULL;

    if (facts->method_index == 0 || meta->method_ids == NULL)
        return NULL;
    if (facts->method_index >= meta->header->method_ids_size)
        return NULL;

    dex_method_id *target_id = &meta->method_ids[facts->method_index];
    dex_class_def *owner = hget_u4obj(meta->class_type_id_map,
                                      target_id->class_idx);
    encoded_method *forwarder = dex_encoded_method_of_id(owner,
                                                         facts->method_index);
    if (forwarder == NULL ||
            (forwarder->access_flags & ACC_DEX_SYNTHETIC) == 0)
        return NULL;

    u4 inner = dex_forwarder_target_method(forwarder);
    if (inner == 0 || inner >= meta->header->method_ids_size)
        return NULL;

    if (out_forwarder != NULL)
        *out_forwarder = forwarder;

    return hget_u4obj(meta->lambda_method_map, inner);
}

jd_exp_lambda* dex_lambda_from_facts(jd_lambda_facts *facts,
                                     jsource_file *jf,
                                     jd_exp_invoke *invoke,
                                     bool invoke_is_static)
{
    if (facts == NULL || facts->kind == JD_LAMBDA_NONE)
        return NULL;
    if (invoke == NULL || invoke->list == NULL)
        return NULL;

    jd_meta_dex *meta = facts->meta;
    if (meta == NULL) {
        jd_dex *dex = jf->meta;
        meta = dex == NULL ? NULL : dex->meta;
    }
    if (meta == NULL)
        return NULL;

    int captures = invoke->list->len;
    if (!invoke_is_static && captures > 0)
        captures--;
    encoded_method *em = facts->body;
    encoded_method *forwarder = NULL;
    bool through_forwarder = false;

    if (facts->kind == JD_LAMBDA_REFERENCE) {
        if (facts->is_static && lambda_captures_are_named(invoke, captures)) {
            em = lambda_forwarder_body(meta, facts, &forwarder);
            if (em != NULL)
                through_forwarder = true;
        }

        if (em == NULL) {
            if (facts->is_static && captures > 0)
                return NULL;
            jd_exp_lambda *exp_lambda = make_obj(jd_exp_lambda);
            memset(exp_lambda, 0, sizeof(jd_exp_lambda));
            exp_lambda->method = NULL;
            exp_lambda->method_name = facts->method_name;
            exp_lambda->class_name = facts->class_name;
            exp_lambda->is_static = facts->is_static;
            exp_lambda->interface_name = facts->interface_name;
            exp_lambda->list = invoke->list;
            exp_lambda->captures = captures;
            return exp_lambda;
        }
    }

    if (facts->kind == JD_LAMBDA_CTOR_REF) {
        jd_exp_lambda *exp_lambda = make_obj(jd_exp_lambda);
        memset(exp_lambda, 0, sizeof(jd_exp_lambda));
        exp_lambda->method = NULL;
        exp_lambda->method_name = facts->method_name;
        exp_lambda->class_name = facts->class_name;
        exp_lambda->interface_name = facts->interface_name;
        exp_lambda->list = invoke->list;
        exp_lambda->captures = captures;
        return exp_lambda;
    }

    if (em == NULL)
        return NULL;

    jd_method *target = dex_method(jf, em);
    if (through_forwarder)
        lambda_bind_captures(target, invoke, captures);
    pthread_mutex_lock(&meta->lambda_lock);
    hset_u4obj(meta->rebuilt_lambda_bodies, em->method_id, em);

    if (through_forwarder && forwarder != NULL)
        hset_u4obj(meta->rebuilt_lambda_bodies, forwarder->method_id,
                   forwarder);
    pthread_mutex_unlock(&meta->lambda_lock);

    jd_exp_lambda *exp_lambda = make_obj(jd_exp_lambda);
    memset(exp_lambda, 0, sizeof(jd_exp_lambda));
    exp_lambda->method = target;
    exp_lambda->body_arity = facts->iface_arity + 1;
    exp_lambda->method_name = target->name;
    exp_lambda->class_name = facts->class_name;
    exp_lambda->is_static = facts->is_static;
    exp_lambda->interface_name = facts->interface_name;
    exp_lambda->list = invoke->list;
    exp_lambda->captures = captures;
    return exp_lambda;
}

static jd_lambda_facts* dex_lambda_facts_cached(jd_meta_dex *meta,
                                                dex_class_def *cf,
                                                jd_dex *dex,
                                                jsource_file *jf);

void dex_lambda_collect_bodies(jd_dex *dex)
{
    jd_meta_dex *meta = dex->meta;
    if (meta->synthetic_classes_map == NULL || meta->lambda_facts == NULL ||
        meta->rebuilt_lambda_bodies == NULL)
        return;

    thread_local_data *tls = get_thread_local_data();
    mem_pool *saved = NULL;
    if (tls != NULL) {
        saved = tls->pool;
        tls->pool = mem_create_pool();
    }
    else {
        saved = mem_scratch_enter();
    }

    bool stat = getenv("GARLIC_LAMBDA_STAT") != NULL;
    int n_synthetic = 0, n_shape = 0, n_none = 0, n_ctor = 0, n_ref = 0,
        n_body = 0, n_fail = 0, n_not_synthetic_body = 0,
        n_ctor_forwarder = 0;

    for (int i = 0; i < meta->header->class_defs_size; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (hget_u4obj(meta->synthetic_classes_map, cf->class_idx) == NULL) {
            if (cf->class_data != NULL) {
                for (int k = 0; k < cf->class_data->direct_methods_size; ++k)
                    if (hget_u4obj(meta->lambda_method_map,
                                   cf->class_data->direct_methods[k].method_id))
                        n_not_synthetic_body++;
                for (int k = 0; k < cf->class_data->virtual_methods_size; ++k)
                    if (hget_u4obj(meta->lambda_method_map,
                                   cf->class_data->virtual_methods[k].method_id))
                        n_not_synthetic_body++;
            }
            continue;
        }
        n_synthetic++;

        /* The same two the call site applies before it will read a class
         * as a lambda at all. */
        if (!dex_class_is_anonymous_class(meta, cf) &&
            !dex_class_is_lambda_shape(meta, cf))
            continue;
        n_shape++;

        jd_lambda_facts *facts = dex_lambda_facts_cached(meta, cf, dex, NULL);
        if (facts == NULL) {
            n_fail++;
            continue;
        }
        if (facts->kind == JD_LAMBDA_NONE) n_none++;
        else if (facts->kind == JD_LAMBDA_CTOR_REF) n_ctor++;
        else if (facts->kind == JD_LAMBDA_REFERENCE) n_ref++;
        else if (facts->kind == JD_LAMBDA_BODY) n_body++;

        if (stat && facts->kind == JD_LAMBDA_CTOR_REF &&
            cf->class_data != NULL) {
            for (int k = 0; k < cf->class_data->direct_methods_size; ++k) {
                u4 t = dex_forwarder_target_method(
                        &cf->class_data->direct_methods[k]);
                if (t && hget_u4obj(meta->lambda_method_map, t))
                    n_ctor_forwarder++;
            }
            for (int k = 0; k < cf->class_data->virtual_methods_size; ++k) {
                u4 t = dex_forwarder_target_method(
                        &cf->class_data->virtual_methods[k]);
                if (t && hget_u4obj(meta->lambda_method_map, t))
                    n_ctor_forwarder++;
            }
        }

        if (facts->kind != JD_LAMBDA_BODY || facts->body == NULL)
            continue;

        pthread_mutex_lock(&meta->lambda_lock);
        hset_u4obj(meta->rebuilt_lambda_bodies, facts->body->method_id,
                   facts->body);

        if (meta->rebuilt_lambda_classes != NULL)
            hset_u4obj(meta->rebuilt_lambda_classes, cf->class_idx, cf);
        pthread_mutex_unlock(&meta->lambda_lock);
    }

    if (stat) {
        fprintf(stderr, "[garlic] collect: %d synthetic, %d pass the shape, "
                        "kinds none=%d ctor=%d ref=%d body=%d (failed=%d), "
                        "ctor-ref classes that call a lambda body=%d\n",
                n_synthetic, n_shape, n_none, n_ctor, n_ref, n_body, n_fail,
                n_ctor_forwarder);
    }

    if (tls != NULL) {
        mem_pool_free(tls->pool);
        tls->pool = saved;
    }
    else {
        mem_scratch_leave(saved);
    }
}

void dex_lambda_report(jd_meta_dex *meta)
{
    if (getenv("GARLIC_LAMBDA_STAT") == NULL)
        return;

    unsigned int candidates = meta->lambda_method_map == NULL
                              ? 0 : meta->lambda_method_map->size;
    unsigned int rebuilt = meta->rebuilt_lambda_bodies == NULL
                           ? 0 : meta->rebuilt_lambda_bodies->size;
    fprintf(stderr, "[garlic] lambda bodies: %u candidates, %u written at "
                    "their call site, %u left declared\n",
            candidates, rebuilt,
            candidates > rebuilt ? candidates - rebuilt : 0);

    if (candidates <= rebuilt || meta->header == NULL)
        return;

    int shown = 0;
    for (int i = 0; i < meta->header->class_defs_size && shown < 10; ++i) {
        dex_class_def *cf = &meta->class_defs[i];
        if (cf->class_data == NULL)
            continue;
        encoded_method *groups[2] = { cf->class_data->direct_methods,
                                      cf->class_data->virtual_methods };
        int sizes[2] = { cf->class_data->direct_methods_size,
                         cf->class_data->virtual_methods_size };
        for (int g = 0; g < 2; ++g) {
            for (int k = 0; k < sizes[g]; ++k) {
                encoded_method *em = &groups[g][k];
                if (hget_u4obj(meta->lambda_method_map, em->method_id) == NULL ||
                    hget_u4obj(meta->rebuilt_lambda_bodies, em->method_id))
                    continue;
                dex_method_id *id = &meta->method_ids[em->method_id];
                string owner = dex_str_of_type_id(meta, id->class_idx);
                string name = id->name_idx < meta->header->string_ids_size
                              ? meta->strings[id->name_idx].data : NULL;
                fprintf(stderr, "[garlic] left declared: %s.%s\n",
                        owner == NULL ? "?" : owner,
                        name == NULL ? "?" : name);
                shown++;
            }
        }
    }
}

static string lambda_keep(jd_meta_dex *meta, string s)
{
    if (s == NULL)
        return NULL;
    return str_create_in(meta->pool, "%s", s);
}

static jd_lambda_facts* dex_lambda_facts_cached(jd_meta_dex *meta,
                                                dex_class_def *cf,
                                                jd_dex *dex,
                                                jsource_file *jf)
{
    pthread_mutex_lock(&meta->lambda_lock);
    jd_lambda_facts *facts = hget_u4obj(meta->lambda_facts, cf->class_idx);
    bool in_progress = facts == NULL &&
            hget_i2i(meta->lambda_in_progress, (int) cf->class_idx) >= 0;
    if (facts == NULL && !in_progress)
        hset_i2i(meta->lambda_in_progress, (int) cf->class_idx, 1);
    pthread_mutex_unlock(&meta->lambda_lock);

    if (facts != NULL)
        return facts;
    if (in_progress)
        return NULL;

    jsource_file *inner = dex_class_inside(dex, cf, jf);
    inner->parent = jf;
    inner->source = jf == NULL ? NULL : jf->source;
    jd_lambda_facts *computed = dex_lambda_facts(inner);

    pthread_mutex_lock(&meta->lambda_lock);
    jd_lambda_facts *stored = hget_u4obj(meta->lambda_facts, cf->class_idx);
    if (stored == NULL) {
        stored = make_obj_in(jd_lambda_facts, meta->pool);
        *stored = *computed;
        stored->interface_name = lambda_keep(meta, computed->interface_name);
        stored->class_name = lambda_keep(meta, computed->class_name);
        stored->method_name = lambda_keep(meta, computed->method_name);
        hset_u4obj(meta->lambda_facts, cf->class_idx, stored);
    }
    hdel_i2i(meta->lambda_in_progress, (int) cf->class_idx);
    pthread_mutex_unlock(&meta->lambda_lock);
    return stored;
}

jd_exp_lambda* dex_lambda_cached(jd_meta_dex *meta,
                                 dex_class_def *cf,
                                 jd_dex *dex,
                                 jsource_file *jf,
                                 jd_exp_invoke *invoke,
                                 bool invoke_is_static)
{
    if (meta->lambda_facts == NULL)
        return NULL;

    return dex_lambda_from_facts(
            dex_lambda_facts_cached(meta, cf, dex, jf), jf, invoke,
            invoke_is_static);
}

bool dex_lambda_is_factory_call(jd_meta_dex *meta, dex_class_def *cf,
                                dex_method_id *method_id)
{
    if (meta->header == NULL || cf == NULL || cf->interfaces == NULL ||
            cf->interfaces->size != 1)
        return false;
    if (method_id->proto_idx >= meta->header->proto_ids_size)
        return false;

    dex_proto_id *proto = &meta->proto_ids[method_id->proto_idx];
    string returns = dex_str_of_type_id(meta, proto->return_type_idx);
    dex_type_item *item = &cf->interfaces->list[0];
    string iface = dex_str_of_type_id(meta, item->type_idx);

    return returns != NULL && iface != NULL && STR_EQL(returns, iface);
}

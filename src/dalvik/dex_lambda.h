#ifndef GARLIC_DEX_LAMBDA_H
#define GARLIC_DEX_LAMBDA_H

#include "dalvik/dex_structure.h"

typedef enum {
    JD_LAMBDA_NONE = 0,
    JD_LAMBDA_CTOR_REF,     // Foo::new 
    JD_LAMBDA_REFERENCE,    // Owner::name,  captured::name
    JD_LAMBDA_BODY,         // the body is a method of the enclosing class
} jd_lambda_kind;

typedef struct {
    jd_lambda_kind  kind;
    jd_meta_dex    *meta;
    string          interface_name;
    string          class_name;
    string          method_name;
    bool            is_static;
    encoded_method *body;
    int             iface_arity;
    u4              method_index;
} jd_lambda_facts;

jd_lambda_facts* dex_lambda_facts(jsource_file *jf);

jd_exp_lambda* dex_lambda_from_facts(jd_lambda_facts *facts,
                                     jsource_file *jf,
                                     jd_exp_invoke *invoke,
                                     bool invoke_is_static);

jd_exp_lambda* dex_lambda_cached(jd_meta_dex *meta,
                                 dex_class_def *cf,
                                 jd_dex *dex,
                                 jsource_file *jf,
                                 jd_exp_invoke *invoke,
                                 bool invoke_is_static);

bool dex_lambda_is_factory_call(jd_meta_dex *meta, dex_class_def *cf,
                                dex_method_id *method_id);

void dex_lambda_collect_bodies(jd_dex *dex);

void dex_lambda_report(jd_meta_dex *meta);

#endif //GARLIC_DEX_LAMBDA_H

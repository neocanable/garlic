#ifndef GARLIC_SIGNATURE_H
#define GARLIC_SIGNATURE_H

// https://cscott.net/Projects/GJ/signature-explained-2_4.html
// https://cscott.net/Projects/GJ/signature-explained.html
#include "list.h"

/**
 * The 2.4 prototype compiler uses the variance specifiers '-', '+', 
 * or '*' of the 2.0 prototype to start a
 * TypeArgument; they are no longer allowed
 * after the open-brackets in an ArrayTypeSignature.
 * The Java syntax corresponding to '-' is "? super ",
 * corresponding to '+' is "? extends ", and '*' corresponds to "?". 
 * The old '=' variance specifier is no longer allowed/generated.
 * The only difference between the 2.2 grammar and the 2.4
 * grammar (that I know of at the moment) is in the
 * grammar for ParameterSignature, where an extra colon has been added.
 * A corrected version of the original JSR-14 spec follows.
 **/

typedef struct base_type_signature {
    int tag;
    /*
     * 0: B
     * 1: C
     * 2: D
     * 3: F
     * 4: I
     * 5: J
     * 6: S
     * 7: Z
     * 8: V
     **/
} base_type_signature;

typedef struct bottom_sig {

} bottom_sig;

typedef struct class_type_sig {
    list_object *path;
} class_type_sig;

typedef struct simple_class_type_sig {
    int dollor;
    string name;
    list_object *type_arguments;
} simple_class_type_sig;

typedef struct simple_class_type_sig sst_sig;

typedef struct type_variable_signature {
    string name;
} type_variable_signature;

typedef struct field_type_sig field_type_sig;
typedef struct field_type_sig type_sig;
typedef struct field_type_sig type_argument;

typedef struct array_type_signature {
    int level;
    type_sig *component_type;
} array_type_signature;

typedef struct wildcard {
    string tag;
    field_type_sig *super_bound;
    field_type_sig *extends_bound;
} wildcard;

typedef enum type_signature_enum {
    BASE_TYPE_SIGNATURE,
    CLASS_TYPE_SIGNATURE,
    TYPE_VARIABLE_SIGNATURE,
    ARRAY_TYPE_SIGNATURE,
    BOTTOM_SIGNATURE,
    SIMPLE_CLASS_TYPE_SIGNATURE,
    WILDCARD,
    FIELD_TYPE_SIGNATURE
} type_signature_enum;

typedef union type_signature_union {
    base_type_signature *base_type;
    class_type_sig *class_type;
    type_variable_signature *type_variable;
    array_type_signature *array_type;
    bottom_sig *bottom;
    simple_class_type_sig *simple_class_type;
    wildcard *wildcard;
    field_type_sig *field_type;
} type_signature_union;

struct field_type_sig {
    type_signature_enum tag;
    type_signature_union *type;
};

typedef struct formal_type_parameter {
    string name;
    list_object *bounds; // store field_type_sig
} formal_type_parameter;

typedef struct class_signature {
    class_type_sig *base_class;
    list_object *formal_type_parameters;
    list_object *interfaces;
} class_signature;

typedef struct method_sig {
    list_object *formal_type_parameters;
    list_object *parameter_types;
    type_sig *return_type;
    list_object *exception_types;
} method_sig;

static string parse_identifier(string sig, int *pint);

static field_type_sig* parse_field_type_signature(string sig, int *pint);

static sst_sig* parse_simple_class_type_sig(string sig,
                                            int *pint,
                                            int use_dollor);

static list_object* parse_bounds(string sig, int *pint);

class_signature* parse_class_signature(string sig);

method_sig* parse_method_signature(string sig);

type_sig* parse_field_signature(string sig);

string formal_type_parameters_to_s(list_object *formal_type_parameters);

string field_type_sig_to_s(field_type_sig *fts);

string simple_class_signature_to_s(simple_class_type_sig *ss);

string interfaces_to_s(class_signature *cs);

#endif //GARLIC_SIGNATURE_H

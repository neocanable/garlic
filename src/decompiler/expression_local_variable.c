#include "common/str_tools.h"
#include "decompiler/expression_local_variable.h"
#include "decompiler/klass.h"
#include "decompiler/expression.h"
#include "decompiler/expression_node.h"
#include "expression_visitor.h"

void variables_rename(jd_method *m);

static void variables_declaration(jd_method *m);

static void variables_scope(jd_method *m);

static void merge_loop_carried_versions(jd_method *m);

static bool is_var_processed(jd_method *m, string name)
{
    for (int i = 0; i < m->var_scopes->size; ++i) {
        jd_variable_scope *r = lget_obj(m->var_scopes, i);
        if (STR_EQL(r->name, name))
            return true;
    }
    return false;
}

static bool is_from_parameter(jd_method *m, string name)
{
    jd_stack *enter_stack = m->enter;
    for (int i = 0; i < enter_stack->local_vars_count; ++i) {
        jd_val *val = enter_stack->local_vars[i];
        if (val != NULL && STR_EQL(val->name, name))
            return true;
    }
    return false;
}

static bool is_catch_param_v2(jd_method *m, jd_exp *exp)
{
    jd_bblock *b = exp->block;
    jd_node *node = b->node;
    jd_node *parent = node->parent;
    return parent != NULL &&
           node_is_catch(parent) &&
           parent->start_idx == exp->idx;
}

static bool first_store_can_be_declaraction(jd_method *m,
                                            jd_variable_scope *scope)
{
    int idx = bitset_minimum(scope->stores);
    jd_exp *first_store_exp = lget_obj(m->expressions, idx);

    jd_node *first_store_node = first_store_exp->block->node;

    jd_exp *last_exp = lget_obj(m->expressions, scope->end_idx);
    jd_node *last_node = last_exp->block->node;

    // 1. first store is smallest goto_offset
    //    first and last in same node
    // 2. first store is smallest goto_offset 
    //    first is anchor's slibing of others

    if (last_node->parent == first_store_node->parent) {
        return true;
    }
    else {
        return node_is_ancestor_sibling_of(first_store_node,
                                           last_node);
    }
}

static void insert_declaration_to(jd_method *m,
                                  jd_variable_scope *scope,
                                  jd_node *node)
{
    jd_exp *new_exp = make_obj(jd_exp);
    new_exp->idx = node->start_idx;
    new_exp->type = JD_EXPRESSION_DECLARATION;
    new_exp->data = scope;
    jd_node *new_block = create_expression_node(m, new_exp);
    new_block->parent = node;

    jd_node *prev = NULL;
    int insert_idx = 0;
    for (int i = 0; i < node->children->size-1; ++i) {
        jd_node *n = lget_obj(node->children, i);
        jd_node *next = lget_obj(node->children, i+1);
        // TODO: if/else if/else/catch/finally/case
        if (n->start_idx < scope->start_idx &&
                (node_is_else_if(next) ||
                node_is_else(next) ||
                node_is_catch(next) ||
                node_is_finally(next) ||
                node_is_case(next))) {
            insert_idx = i;
            break;
        }
    }
    ladd_obj_at(node->children, new_block, insert_idx);
}

static string get_array_variable_name(string name)
{
    char *p = strrchr(name, '[');
    if (p == NULL)
        return name;

    int len = p - name;
    string base_name = x_alloc(len + 1);
    memcpy(base_name, name, len);
    base_name[len] = '\0';

    int count = 0;
    for (size_t i = len; i < strlen(name); ++i) {
        if (name[i] == '[')
            count++;
    }

    if (count == 1)
        return str_create("%sArr", base_name);
    return str_create("%sArr%d", base_name, count);
}

static string gen_variable_name(jd_method *m, jd_exp *exp, string cname)
{
    string stem = NULL;
    bool numbered = true;

    if (STR_EQL(cname, "String"))
        stem = "str";
    else if (STR_EQL(cname, g_str_int))
        stem = "i";
    else if (STR_EQL(cname, g_str_long))
        stem = "l";
    else if (STR_EQL(cname, g_str_double))
        stem = "dbl";
    else if (STR_EQL(cname, g_str_float))
        stem = "flt";
    else if (STR_EQL(cname, g_str_byte))
        stem = "byte";
    else if (STR_EQL(cname, g_str_short))
        stem = "short";
    else if (STR_EQL(cname, g_str_boolean))
        stem = "bool";
    else if (STR_EQL(cname, g_str_char))
        stem = "char";
    else {
        string last_word = get_last_word_lower(cname);
        stem = get_array_variable_name(last_word);
        numbered = false;
    }

    if (m->var_name_taken == NULL)
        m->var_name_taken = hashmap_init((hcmp_fn) s2i_cmp, 0);
    int count = hget_s2i(m->class_counter_map, stem);
    if (count == -1)
        count = 0;

    if (!numbered && count == 0 &&
        (java_keyword(stem) || (cname != NULL && STR_EQL(stem, cname))))
        count = 1;

    string name = NULL;
    do {
        if (numbered)
            name = str_create("%s%d", stem, count);
        else if (count == 0)
            name = str_create("%s", stem);
        else
            name = str_create("%sVar%d", stem, count);
        count++;
    } while (hget_s2i(m->var_name_taken, name) != -1);

    hset_s2i(m->class_counter_map, stem, count);
    hset_s2i(m->var_name_taken, name, 1);
    return name;
}

void analyse_local_variables(jd_method *m)
{
    m->declarations = bitset_create();
    m->var_scopes = linit_object();

    variables_rename(m);

    variables_scope(m);

    variables_declaration(m);

    merge_loop_carried_versions(m);
}

static bool pending_throwable(jd_val *v)
{
    return v->data != NULL && v->data->cname != NULL &&
           (STR_EQL(v->data->cname, "Throwable") ||
            STR_EQL(v->data->cname, "Exception"));
}

static void collect_local_uses(jd_exp *e, list_object *list, bool all)
{
    if (e == NULL)
        return;
    switch (e->type) {
        case JD_EXPRESSION_LOCAL_VARIABLE: {
            jd_val *v = e->data;
            if (all && v != NULL && v->name != NULL)
                ladd_obj(list, v);
            return;
        }
        case JD_EXPRESSION_ASSIGNMENT: {
            jd_exp_assignment *a = e->data;
            collect_local_uses(a->left, list, all);
            collect_local_uses(a->right, list, all);
            return;
        }
        case JD_EXPRESSION_ASSIGNMENT_CHAIN: {
            jd_exp_assignment_chain *c = e->data;
            for (int i = 0; c->left != NULL && i < c->left->size; ++i)
                collect_local_uses(lget_obj(c->left, i), list, all);
            collect_local_uses(c->right, list, all);
            return;
        }
        case JD_EXPRESSION_IF: {
            jd_exp_if *f = e->data;
            collect_local_uses(f->expression, list, all);
            collect_local_uses(f->target_exp, list, all);
            return;
        }
        case JD_EXPRESSION_INVOKE:
        case JD_EXPRESSION_INVOKE_VIRTUAL:
        case JD_EXPRESSION_INVOKE_STATIC:
        case JD_EXPRESSION_INVOKE_INTERFACE:
        case JD_EXPRESSION_INVOKE_SPECIAL:
        case JD_EXPRESSION_INVOKE_DYNAMIC: {
            jd_exp_invoke *inv = e->data;
            if (inv->method_name != NULL &&
                STR_EQL(inv->method_name, "addSuppressed") &&
                inv->list != NULL && inv->list->len > 0) {
                jd_exp *recv = &inv->list->args[inv->list->len - 1];
                if (recv->type == JD_EXPRESSION_LOCAL_VARIABLE) {
                    jd_val *v = recv->data;
                    if (v != NULL && v->name != NULL &&
                        v->name_type != JD_VAR_NAME_DEBUG &&
                        pending_throwable(v))
                        ladd_obj_no_dup(list, v);
                }
            }
            for (int i = 0; inv->list != NULL && i < inv->list->len; ++i)
                collect_local_uses(&inv->list->args[i], list, all);
            return;
        }
        case JD_EXPRESSION_OPERATOR:
        case JD_EXPRESSION_NEW_ARRAY:
        case JD_EXPRESSION_RETURN:
        case JD_EXPRESSION_NEW_OBJ:
        case JD_EXPRESSION_ARRAY_STORE:
        case JD_EXPRESSION_SWITCH:
        case JD_EXPRESSION_TERNARY:
        case JD_EXPRESSION_PUT_STATIC:
        case JD_EXPRESSION_PUT_FIELD:
        case JD_EXPRESSION_STORE:
        case JD_EXPRESSION_DEFINE_STACK_VAR:
        case JD_EXPRESSION_ATHROW:
        case JD_EXPRESSION_INSTANCEOF:
        case JD_EXPRESSION_CAST:
        case JD_EXPRESSION_INITIALIZE:
        case JD_EXPRESSION_ARRAYLENGTH:
        case JD_EXPRESSION_SINGLE_OPERATOR:
        case JD_EXPRESSION_SINGLE_LIST:
        case JD_EXPRESSION_COMPARE:
        case JD_EXPRESSION_LAMBDA:
        case JD_EXPRESSION_ANONYMOUS:
        case JD_EXPRESSION_ARRAY_LOAD:
        case JD_EXPRESSION_IINC:
        case JD_EXPRESSION_UNINITIALIZE:
        case JD_EXPRESSION_FOR:
        case JD_EXPRESSION_WHILE:
        case JD_EXPRESSION_DO_WHILE:
        case JD_EXPRESSION_LOGIC_NOT: {
            jd_exp_reader *reader = e->data;
            for (int i = 0; reader != NULL && reader->list != NULL &&
                            i < reader->list->len; ++i)
                collect_local_uses(&reader->list->args[i], list, all);
            return;
        }
        default:
            return;
    }
}

static bool name_is_declared(list_object *names, string name)
{
    for (int i = 0; i < names->size; ++i) {
        string n = lget_obj(names, i);
        if (n != NULL && STR_EQL(n, name))
            return true;
    }
    return false;
}

void ensure_undeclared_variables(jd_method *m)
{
    list_object *declared = linit_object();
    for (int i = 0; i < m->var_scopes->size; ++i) {
        jd_variable_scope *r = lget_obj(m->var_scopes, i);
        if (r->name != NULL)
            ladd_obj(declared, r->name);
    }
    for (int i = 0; i < m->enter->local_vars_count; ++i) {
        jd_val *v = m->enter->local_vars[i];
        if (v != NULL && v->name != NULL)
            ladd_obj(declared, v->name);
    }
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *n = lget_obj(m->nodes, i);
        if (n->param_exp == NULL ||
            n->param_exp->type != JD_EXPRESSION_LOCAL_VARIABLE)
            continue;
        jd_val *v = n->param_exp->data;
        if (v != NULL && v->name != NULL)
            ladd_obj(declared, v->name);
    }

    list_object *used = linit_object();
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *e = lget_obj(m->expressions, i);
        if (exp_is_nopped(e))
            continue;
        collect_local_uses(e, used, false);
    }

    jd_node *root = lget_obj_first(m->nodes);
    for (int i = 0; i < used->size; ++i) {
        jd_val *v = lget_obj(used, i);
        if (name_is_declared(declared, v->name))
            continue;
        jd_variable_scope *scope = make_obj(jd_variable_scope);
        scope->name = v->name;
        scope->cname = (v->data != NULL && v->data->cname != NULL)
                       ? v->data->cname : (string) "Object";
        scope->start_idx = root->start_idx;
        scope->end_idx = root->end_idx;
        scope->declaration_idx = -2;
        scope->stores = bitset_create();
        ladd_obj(declared, v->name);
        insert_declaration_to(m, scope, root);
    }
}


static bool exp_store_pair(jd_exp *e, jd_exp **left, jd_exp **right)
{
    if (e->type == JD_EXPRESSION_STORE) {
        jd_exp_store *s = e->data;
        if (s->list != NULL && s->list->len >= 2) {
            *left = &s->list->args[0];
            *right = &s->list->args[1];
            return true;
        }
    }
    else if (e->type == JD_EXPRESSION_ASSIGNMENT) {
        jd_exp_assignment *a = e->data;
        if (a->left != NULL && a->right != NULL) {
            *left = a->left;
            *right = a->right;
            return true;
        }
    }
    return false;
}

static bool exp_in_loop(jd_exp *e)
{
    if (e->block == NULL || e->block->node == NULL)
        return false;
    for (jd_node *n = e->block->node; n != NULL; n = n->parent) {
        if (node_is_while(n) || node_is_for(n) ||
            node_is_do_while(n) || node_is_loop(n))
            return true;
    }
    return false;
}

static int val_ref_count(list_object *refs, jd_val *val)
{
    int count = 0;
    for (int i = 0; i < refs->size; ++i) {
        if (lget_obj(refs, i) == val)
            count++;
    }
    return count;
}

static void merge_loop_carried_versions(jd_method *m)
{
    list_object *refs = linit_object();
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *e = lget_obj(m->expressions, i);
        if (exp_is_nopped(e))
            continue;
        collect_local_uses(e, refs, true);
    }
    list_object *unique = linit_object();
    for (int i = 0; i < refs->size; ++i)
        ladd_obj_no_dup(unique, lget_obj(refs, i));

    for (int i = 0; i < unique->size; ++i) {
        jd_val *v = lget_obj(unique, i);
        if (v->data == NULL || v->data->cname == NULL)
            continue;

        jd_exp *store = NULL;
        int stores = 0;
        for (int j = 0; j < m->expressions->size; ++j) {
            jd_exp *e = lget_obj(m->expressions, j);
            if (exp_is_nopped(e))
                continue;
            jd_exp *left = NULL;
            jd_exp *right = NULL;
            if (!exp_store_pair(e, &left, &right))
                continue;
            if (left->type != JD_EXPRESSION_LOCAL_VARIABLE ||
                left->data != v)
                continue;
            store = e;
            stores++;
        }
        if (stores != 1 || store == NULL)
            continue;
        if (val_ref_count(refs, v) != 1)
            continue;
        if (!exp_in_loop(store))
            continue;

        jd_exp *left = NULL;
        jd_exp *right = NULL;
        if (!exp_store_pair(store, &left, &right))
            continue;
        list_object *rhs = linit_object();
        collect_local_uses(right, rhs, true);
        jd_val *target = NULL;
        for (int j = 0; j < rhs->size; ++j) {
            jd_val *w = lget_obj(rhs, j);
            if (w == v || w->slot != v->slot || w->name == NULL ||
                w->data == NULL || w->data->cname == NULL)
                continue;
            if (!STR_EQL(w->data->cname, v->data->cname))
                continue;
            if (val_ref_count(refs, w) <= 1)
                continue;
            target = w;
            break;
        }
        if (target == NULL)
            continue;

        v->name = target->name;
        v->data = target->data;
        if (m->declarations != NULL)
            bitset_set_to_value(m->declarations, store->idx, false);
    }
}

static void variables_declaration(jd_method *m)
{
    jd_node *older;
    jd_node *younger;
    for (int i = 0; i < m->var_scopes->size; ++i) {
        jd_variable_scope *range = lget_obj(m->var_scopes, i);
        if (first_store_can_be_declaraction(m, range)) {
            range->declaration_idx = range->start_idx;
            bitset_set(m->declarations, range->start_idx);
            DEBUG_PRINT("[varibale first declaration]: %s %d\n",
                        range->name, range->declaration_idx);
        }
        else {
            // find declaration of variable
            jd_exp *first = get_exp(m, range->start_idx);
            jd_exp *last = get_exp(m, range->end_idx);
            jd_node *first_node = first->block->node;
            jd_node *last_node = last->block->node;

            int first_scope = node_scope(first_node);
            int last_scope = node_scope(last_node);

            older = first_scope > last_scope ? first_node : last_node;
            younger = first_scope > last_scope ? last_node : first_node;
            jd_node *older_parent = older->parent;
            while (older_parent != NULL) {
                if (node_is_ancestor_of(older_parent, younger))
                    break;
                older_parent = older_parent->parent;
            }
            if (older_parent == NULL)
                older_parent = lget_obj(m->nodes, 0);
            DEBUG_PRINT("[trie_insert declaration]: %s %d\n",
                        range->name, older_parent->start_idx);
            // trie_insert declaration to older_parent
            insert_declaration_to(m, range, older_parent);
        }
    }
}

static void variables_scope(jd_method *m)
{
    list_object *var_scopes = m->var_scopes;
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);

        if (exp_is_nopped(exp))
            continue;

        if (!exp_is_store(exp) && !exp_is_define_stack_var(exp))
            continue;

        string name = NULL;
        string cname = NULL;
        jd_val *val = NULL;
        jd_var *var = NULL;
        if (exp_is_store(exp)) {
            jd_exp_store *store = exp->data;
            jd_exp *left = &store->list->args[0];
            jd_val *_val = left->data;
            val = _val;
            name = _val->name;
            cname = _val->data->cname;
        }
        else if (exp_is_define_stack_var(exp)) {
            jd_exp_def_var *def = exp->data;
            jd_exp *left = &def->list->args[0];
            jd_var *_var = left->data;
            name = _var->name;
            var = _var;
            cname = _var->cname;
        }

        if (is_var_processed(m, name))
            continue;
        if (is_from_parameter(m, name))
            continue;
        if (is_catch_param_v2(m, exp))
            continue;

        jd_variable_scope *range = make_obj(jd_variable_scope);
        range->start_idx = exp->idx;
        range->stores = bitset_create();
        range->cname = cname;
        range->name = name;
        range->var = var;
        range->val = val;
        bitset_set(range->stores, exp->idx);
        range->declaration_idx = -1;
        ladd_obj(var_scopes, range);

        int end_idx = 0;
        if (exp_is_store(exp)) {
            for (int j = i+1; j < m->expressions->size; ++j) {
                jd_exp *e = lget_obj(m->expressions, j);
                if (exp_is_nopped(e))
                    continue;
                if (!exp_is_assignment(e) && !exp_is_store(e))
                    continue;

                if (exp_is_store(e)) {
                    jd_exp_store *s = e->data;
                    jd_exp *l = &s->list->args[0];
                    jd_val *v = l->data;
                    if (STR_EQL(val->name, v->name)) {
                        end_idx = e->idx;
                    }
                }
                else if (exp_is_assignment(e)) {
                    jd_exp_assignment *assign = e->data;
                    jd_exp *r = assign->right;
                    if (exp_is_local_variable(r)) {
                        jd_val *v = r->data;
                        if (STR_EQL(val->name, v->name)) {
                            end_idx = e->idx;
                        }
                    }
                }
                if (expression_reads_name(e, val->name))
                    end_idx = MAX(end_idx, e->idx);
            }
            range->end_idx = end_idx == 0 ? exp->idx : end_idx;

            DEBUG_PRINT("[local variable]: %s range: %d -> %d %s\n",
                        val->name,
                        range->start_idx,
                        range->end_idx,
                        val->data->cname);
        }
        else if (exp_is_define_stack_var(exp)) {
            for (int j = i+1; j < m->expressions->size; ++j) {
                jd_exp *e = lget_obj(m->expressions, j);
                if (exp_is_nopped(e))
                    continue;

                if (has_stack_var(e, var))
                    end_idx = MAX(end_idx, e->idx);
            }
            range->end_idx = end_idx == 0 ? exp->idx : end_idx;
        }
    }
}

void variables_rename(jd_method *m)
{
    if (!DEBUG_RENAME_VARIABLES) return;

    jd_stack *enter_stack = m->enter;
    for (int i = 0; i < enter_stack->local_vars_count; ++i) {
        jd_val *val = enter_stack->local_vars[i];
        if (val == NULL)
            continue;
        if (val->name_type == JD_VAR_NAME_DEBUG)
            continue;
        if (STR_EQL(val->name, g_str_this))
            continue;
        string var_name = gen_variable_name(m, NULL, val->data->cname);
        hset_s2s(m->var_name_map, val->name, var_name);

        val->name = var_name;
    }


    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp))
            continue;

        if (exp_is_define_stack_var(exp)) {
            jd_exp_def_var *def = exp->data;
            jd_exp *left = &def->list->args[0];
            jd_var *var = left->data;
            string var_name = hget_s2s(m->var_name_map, var->name);
            if (var_name != NULL) {
                var->name = var_name;
                continue;
            }
            var_name = gen_variable_name(m, exp, var->cname);
            hset_s2s(m->var_name_map, var->name, var_name);

            var->name = var_name;
            DEBUG_PRINT("[variable name]: %s -> %s -> %s\n",
                        var->name,
                        var_name,
                        var->cname);
        }
        else if (exp_is_store(exp)) {
            jd_exp_store *store = exp->data;
            jd_exp *left = &store->list->args[0];
            jd_val *val = left->data;

            if (is_from_parameter(m, val->name))
                continue;
            if (is_catch_param_v2(m, exp))
                continue;

            if (val->name_type == JD_VAR_NAME_DEBUG)
                continue;

            string var_name = hget_s2s(m->var_name_map, val->name);
            if (var_name != NULL) {
                val->name = var_name;
                continue;
            }

            var_name = gen_variable_name(m, exp, val->data->cname);
            hset_s2s(m->var_name_map, val->name, var_name);

            val->name = var_name;

            DEBUG_PRINT("[variable name]: %s -> %s -> %s\n",
                        val->name,
                        var_name,
                        val->data->cname);
        }
    }
}
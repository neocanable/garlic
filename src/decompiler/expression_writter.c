#include "common/str_tools.h"
#include "decompiler/klass.h"
#include "decompiler/expression.h"
#include "decompiler/expression_writter.h"
#include "decompiler/field.h"
#include "decompiler/method.h"
#include "transformer/transformer.h"
#include "jvm/jvm_annotation.h"
#include "expression_node.h"
#include "ssa.h"
#include "control_flow.h"

static inline FILE* file_output(jsource_file *jf)
{
    return jf->source == NULL ? DEFAULT_WRITE_OUT : jf->source;
}

static inline string front_ins_string(jd_exp *expression, jd_ins *ins)
{

    if (ins == NULL) {
        return str_create("[%24d] ", expression->idx);
    }
    else {
        return str_create("[%4d %23s %4d] ",
                          ins->offset,
                          ins->name,
                          expression->idx);
    }

}

static inline void front_ins_stream(jsource_file *jf, jd_exp *exp, jd_ins *ins)
{
    FILE *stream = file_output(jf);
    if (ins == NULL)
        fprintf(stream, "[%24d] ", exp->idx);
    else
        fprintf(stream, "[%4d %20s %4d] ",
                ins->offset,
                ins->name,
                exp->idx);
}

void print_expression(jd_exp *expression, jd_ins *ins)
{
    if (exp_is_nopped(expression) || exp_is_empty(expression))
        return;


    if (DEBUG_INS_AND_NODE_INFO) {
        if (!isatty(fileno(stdout))) {
            if (DEBUG_WRITE_COLOR) {
                if (!exp_is_copy(expression)) {
                    printf("\033[0;31m");
                    printf("%s", front_ins_string(expression, ins));
                    printf("\033[0m");
                    fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                            exp_to_s(expression));
                }
                else {
                    printf("\033[0;32m");
                    printf("%s", front_ins_string(expression, ins));
                    printf("\033[0m");
                    fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                            exp_to_s(expression));
                }
            }
            else {
                printf("%s ", front_ins_string(expression, ins));
                fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                        exp_to_s(expression));
            }
        }
        else {
            fprintf(DEFAULT_WRITE_OUT, "%s %s\n",
                    front_ins_string(expression, ins),
                    exp_to_s(expression));
        }
    }
    else
        fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                exp_to_s(expression));
}

void print_exp_full(jd_exp *expression, jd_ins *ins)
{
    if (exp_is_empty(expression))
        return;

    if (DEBUG_INS_AND_NODE_INFO) {
        if (!isatty(fileno(stdout))) {
            if (DEBUG_WRITE_COLOR) {
                printf("\033[0;31m");
                printf("%s ", front_ins_string(expression, ins));
                printf("\033[0m");
                fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                        exp_to_s(expression));
            }
            else {
                printf("%s", front_ins_string(expression, ins));
                fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                        exp_to_s(expression));
            }
        }
        else {
            fprintf(DEFAULT_WRITE_OUT, "%s %s\n",
                    front_ins_string(expression, ins),
                    exp_to_s(expression));
        }
    }
    else
        fprintf(DEFAULT_WRITE_OUT, "%s;\n",
                exp_to_s(expression));
}

static void write_expression(jsource_file *jf,
                             jd_node *node,
                             jd_exp *exp,
                             jd_ins *ins,
                             bool terminated)
{
    FILE *stream = file_output(jf);
    if (DEBUG_INS_AND_NODE_INFO) {
        if (DEBUG_WRITE_COLOR) {
            fprintf(stream, "\033[0;31m");
            front_ins_stream(jf, exp, ins);
            fprintf(stream, "\033[0m");
            expression_to_stream(stream, node, exp);
        }
        else {
            front_ins_stream(jf, exp, ins);
            expression_to_stream(stream, node, exp);
        }
    }
    else {
        expression_to_stream(stream, node, exp);
    }
    if (terminated)
        fprintf(stream, ";\n");
}

void print_all_expression(jd_method *m)
{
    printf("---------------- m: %s\n", m->name);
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);
        jd_ins *ins = expression->ins;
        if (exp_is_nopped(expression))
            continue;
        print_expression(expression, ins);
    }
    printf("\n");
}

static bool is_block_start(jd_method *m, jd_ins *ins)
{
    for (int i = 0; i < m->basic_blocks->size; ++i) {
        jd_bblock *b = lget_obj(m->basic_blocks, i);
        if (b->type != JD_BB_NORMAL)
            continue;
        if (b->ub->nblock->start_offset == ins->offset)
            return true;
    }
    return false;
}

static void print_phi_nodes(jd_method *m, jd_ins *ins)
{
    for (int i = 0; i < m->local_phi_list->size; ++i) {
        jd_local_phi_node *phi_node = lget_obj(m->local_phi_list, i);
        if (phi_node->ins_offset != ins->offset)
            continue;
        printf("\t[local phi node]: ssa_var_%d_%d = ",
               phi_node->slot,
               phi_node->version);
        printf("\t(\n");
        for (int j = 0; j < phi_node->params->size; ++j) {
            jd_ssa_param *param = lget_obj(phi_node->params, j);
            printf("\t\tssa_var: "
                   "slot:%d version:%d blk: %d def: %d ssa_var_%d_%d\n",
                   param->slot,
                   param->version,
                   param->block_id,
                   param->def_block_id,
                   param->slot,
                   param->version);
        }
        printf("\t)\n");
    }
}

static void print_ssa_var(jd_method *m, jd_ins *ins)
{
    if (ins == NULL)
        return;
    for (int i = 0; i < m->ssa_vars->size; ++i) {
        jd_ssa_var *var = lget_obj(m->ssa_vars, i);
        if (var->ins->offset == ins->offset) {
            printf("[ssa_var]: var_%d_%d\n", var->slot, var->version);
        }
    }
}

void print_all_expression_with_ssa(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);
        jd_ins *ins = expression->ins;
        if (ins != NULL && ins_is_duplicate(ins))
            continue;
        if (ins != NULL && is_block_start(m, ins)) {
            jd_bblock *block = ins->block;
            printf("#BLK%zu, %d -> %d\n",
                   block->block_id,
                   block->ub->nblock->start_offset,
                   block->ub->nblock->end_offset);
            printf("\033[0;33m");
            string processors = list_in_edge_id_join(block->in);
            string successors = list_out_edge_id_join(block->out);
            string frontiers = list_block_id_join(block->frontier, NULL);
            printf("\tpredecessors: %s\n", processors);
            printf("\tsuccessors: %s\n", successors);
            printf("\tdominate frontiers: %s\n", frontiers);
            printf("\033[0m");

            bitset_t *live_in_set = lget_obj(m->live_ins, block->block_id);
            bitset_t *live_out_set = lget_obj(m->live_outs, block->block_id);
            string live_in = bitset_string(live_in_set);
            string live_out = bitset_string(live_out_set);
            printf("\tlive in: %s\n", live_in);
            printf("\tlive out: %s\n", live_out);

            printf("\033[0;32m");
            print_phi_nodes(m, ins);
            printf("\033[0m");

        }
        if (exp_is_nopped(expression) || exp_is_empty(expression))
            continue;

        printf("\t\t");
        print_expression(expression, ins);
    }
}

void print_full_expression(jd_method *m)
{
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *expression = lget_obj(m->expressions, i);
        jd_ins *ins = expression->ins;
        print_exp_full(expression, ins);
    }
}

void print_code_nodes(jd_method *m, jd_node *node)
{
    if (node == NULL) {
        printf("---------------- m: %s\n", m->name);
        node = lget_obj_first(m->nodes);
    }

    string ident = get_node_ident(node);

    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);

        if (node_is_expression(child)) {
            jd_exp *exp = child->data;
            if (exp_is_nopped(exp) || exp_is_empty(exp))
                continue;
            fprintf(DEFAULT_WRITE_OUT, "%s", ident);
            print_expression(exp, exp->ins);
        }
        else {
            fprintf(DEFAULT_WRITE_OUT, "%s %s "
                    "{ // block_id: %d  "
                    "range: %d - %d  "
                    "parent_id: %d\n",
                    ident,
                    node_name(child),
                    child->node_id,
                    child->start_idx,
                    child->end_idx,
                    child->parent->node_id);
            print_code_nodes(m, child);
            fprintf(DEFAULT_WRITE_OUT, "%s }\n", ident);
        }
    }

}

void print_code_nodes_v2(jd_method *m, jd_node *node)
{
    if (node == NULL) {
        // print_node_tree(m, NULL);
        node = lget_obj_first(m->nodes);
        create_method_defination(m);
        printf("---------------- m: %s\n", m->defination);
    }

    string ident = get_node_ident(node);

    if (node_is_expression(node)) {
        jd_exp *exp = node->data;
        if (exp_is_nopped(exp) || exp_is_empty(exp))
            return;
        fprintf(DEFAULT_WRITE_OUT, "%s", ident);
        print_expression(exp, exp->ins);
        return;
    }

    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);

        if (node_is_basic_block(child)) {
            jd_bblock *b = child->data;
            jd_nblock *nb = b->ub->nblock;
//            debug_basic_block_node(child);

//            for (int j = nb->start_idx; j <= nb->end_idx; ++j) {
//                jd_exp *exp = get_exp(m, j);
//                if (exp_is_nopped(exp) || exp_is_empty(exp))
//                    continue;
//                fprintf(DEFAULT_WRITE_OUT, "%s", ident);
//                print_expression(exp, exp->ins);
//            }

            for (int j = child->start_idx; j <= child->end_idx ; ++j) {
                jd_exp *exp = get_exp(m, j);
                if (exp_is_nopped(exp) || exp_is_empty(exp))
                    continue;
                fprintf(DEFAULT_WRITE_OUT, "%s", ident);
                print_expression(exp, exp->ins);
            }
        }
        else if (node_is_expression(child)) {
            jd_exp *exp = child->data;
            if (exp_is_nopped(exp) || exp_is_empty(exp))
                return;
            fprintf(DEFAULT_WRITE_OUT, "%s", ident);
            print_expression(exp, exp->ins);
        }
        else {
            fprintf(DEFAULT_WRITE_OUT, "%s%s { "
                    "// block_id: %d  "
                    "range: %d - %d  "
                    "parent_id: %d\n",
                    ident,
                    node_name(child),
                    child->node_id,
                    child->start_idx,
                    child->end_idx,
                    child->parent->node_id);
            print_code_nodes_v2(m, child);
            fprintf(DEFAULT_WRITE_OUT, "%s}\n", ident);
        }
    }

}

string lambada_method_ident(jd_method *m)
{
    jd_node *root = lget_obj_first(m->nodes);
    string ident = get_node_ident(root);
    return ident;
}

// for lambda m?
string method_block_to_string(jd_method *m, jd_node *node)
{
    string buf = NULL;
    FILE *stream;
    size_t len = 0;

#ifdef _WIN32
    // Windows doesn't have open_memstream, use tmpfile as alternative
    stream = tmpfile();
#else
    stream = open_memstream(&buf, &len);
#endif

    if (node == NULL)
        node = lget_obj_first(m->nodes);

    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        string ident = get_node_ident(child);
        if (child->type == JD_NODE_EXPRESSION) {
            jd_exp *exp = child->data;
            if (exp_is_nopped(exp) ||
                exp_is_empty(exp) ||
                exp_is_if(exp) ||
                exp_is_switch(exp))
                continue;


            fprintf(stream, "%s%s%s;\n",
                    ident,
                    front_ins_string(exp, exp->ins),
                    exp_to_s(exp));
            fflush(stream);
        }
        else {
            if (child->type == JD_NODE_IF ||
                child->type == JD_NODE_SWITCH ||
                child->type == JD_NODE_ELSE_IF) {
                jd_node *first = node_first_effective_child(child);
                jd_exp *expression = first->data;
                fprintf(stream,
                        "%s%s (%s) { "
                        "// block_id: %d  "
                        "range: %d - %d  "
                        "parent_id: %d\n",
                        ident,
                        node_name(child),
                        exp_to_s(expression),
                        first->node_id,
                        first->start_idx,
                        first->end_idx,
                        first->parent->node_id);
            }
            else {
                fprintf(stream, 
                        "%s%s { "
                        "// block_id: %d  "
                        "range: %d - %d  "
                        "parent_id: %d\n",
                        ident,
                        node_name(child),
                        child->node_id,
                        child->start_idx,
                        child->end_idx,
                        child->parent->node_id);
            }
            fprintf(stream, "%s", method_block_to_string(m, child));
            fprintf(stream, "%s}\n", ident);
            fflush(stream);
        }
    }
#ifdef _WIN32
    // For Windows tmpfile approach, we need to read back the content
    fseek(stream, 0, SEEK_END);
    len = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    
    string result = x_alloc(len + 1);
    fread(result, 1, len, stream);
    result[len] = '\0';
    fclose(stream);
    return result;
#else
    string result = x_alloc(len+1);
    memcpy(result, buf, len+1);
    result[len] = '\0';
    free(buf);
    fclose(stream);
    return result;
#endif
}

static void write_notice(jsource_file *jf)
{
    if (!SOURCE_FILE_NOTICE) return;
    FILE *stream = file_output(jf);

    fprintf(stream, "/*\n");
    fprintf(stream, " * Decompiled by Garlic\n");
    fprintf(stream, " * Version: 1.0\n");
    fprintf(stream, " */ \n");
}

static void write_import(jsource_file *jf)
{
    FILE *stream = file_output(jf);
    if (jf->pname != NULL)
        fprintf(stream, "package %s;\n\n", jf->pname);

    trie_leaf_to_stream(jf->imports, stream);
    fprintf(stream, "\n");
}

static void write_class_annotation(jsource_file *jf)
{
    FILE *stream = file_output(jf);
    for (int i = 0; i < jf->annotations->size; ++i) {
        jd_annotation *ano = lget_obj(jf->annotations, i);
        // annotation里面有不可见字符
        for (int j = 0; j < strlen(ano->str); ++j) {
            unsigned char c = ano->str[j];
            if (iscntrl(c)) {
                fprintf(stream, "\\%02X", c);
            }
            else {
                fprintf(stream, "%c", c);
            }
        }
        fprintf(stream, "\n");
    }
}

static void write_method_annotation(jsource_file *jf, jd_node *node)
{
    string ident = get_node_ident(node);
    jd_method *m = node->data;
    if (method_is_lambda(m))
        return;

    FILE *stream = file_output(jf);
    for (int j = 0; j < m->annotations->size; ++j) {
        jd_annotation *ano = lget_obj(m->annotations, j);
        fprintf(stream, "%s%s\n", ident, ano->str);
    }
}

static void write_field(jsource_file *jf, jd_node *node)
{
    string ident = get_node_ident(node);
    FILE *stream = file_output(jf);
    for (int i = 0; i < jf->fields_count; ++i) {
        jd_field *field = &jf->fields[i];
        if (field_is_hide(field) || field_is_assert(field))
            continue;
//        if (field_is_synthetic(field) && jf->is_anonymous)
//            continue;
        for (int j = 0; j < field->annotations->size; ++j) {
            jd_annotation *ano = lget_obj(field->annotations, j);
            string annotation = ano->str;
            fprintf(stream, "%s%s\n", ident, annotation);
        }
        fprintf(stream, "%s%s;\n", ident, field->defination);
    }
    fprintf(stream, "\n");
}

static void write_node_debug_info(jsource_file *jf, jd_node *node)
{
    if (DEBUG_INS_AND_NODE_INFO) {
        if (DEBUG_WRITE_COLOR) {
            printf("\033[0;32m");
            fprintf(file_output(jf), " // node_id: %d  "
                                       "range: %d - %d  "
                                       "parent_id: %d",
                    node->node_id,
                    node->start_idx,
                    node->end_idx,
                    node->parent->node_id);
            printf("\033[0m");
        } else {
            fprintf(file_output(jf), " // block_id: %d  "
                                       "range: %d - %d  "
                                       "parent_id: %d",
                    node->node_id,
                    node->start_idx,
                    node->end_idx,
                    node->parent->node_id);
        }
    }
    fprintf(file_output(jf), "\n");
}

static void write_class(FILE *stream, jd_node *n)
{
    string ident = get_node_ident(n);
    jsource_file *_source_file = n->data;
    fprintf(stream, "%s// class: %s\n", ident, _source_file->fname);
    write_class_annotation(_source_file);
    fprintf(stream, "%s%s {\n", ident, _source_file->defination);
    writter_for_class(_source_file, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_anonymous_class(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    jsource_file *_source_file = n->data;
    write_class_annotation(_source_file);
    fprintf(stream, "%s%s {\n", ident, _source_file->defination);
    writter_for_anonymous_class(_source_file, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_method(FILE *stream, jsource_file *jf, jd_node *node)
{
    string ident = get_node_ident(node);
    jd_method *m = node->data;
    if (method_is_lambda(m) || method_is_hide(m))
        return;

    write_method_annotation(jf, node);
    if (method_is_empty(m)) {
        fprintf(stream, "%s%s;\n\n", ident,
                create_method_defination(m));
        return;
    }
    else
        fprintf(stream, "%s%s {\n", ident,
                create_method_defination(m));
    writter_for_class(jf, node);
    fprintf(stream, "%s}\n\n", ident);
}

static void write_expression_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    jd_exp *exp = n->data;
    if (exp_is_nopped(exp) ||
        exp_is_empty(exp))
        return;
    string ident = get_node_ident(n);
    fprintf(stream, "%s", ident);
    write_expression(jf, n, exp, exp->ins, true);
}

static void write_basic_block(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    for (int j = n->start_idx; j <= n->end_idx; ++j) {
        jd_exp *exp = get_exp(n->method, j);
        if (exp_is_nopped(exp) ||
            exp_is_empty(exp))
            continue;
        fprintf(stream, "%s", ident);
        write_expression(jf, n, exp, exp->ins, true);
    }
}

static void write_if_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    if (n->children->size == 0)
        return;
    string ident = get_node_ident(n);
    fprintf(stream, "%s", ident);
    fprintf(stream, "%s (", node_name(n));
    write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    fprintf(stream, ") {");
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_switch_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%s%s(", ident, node_name(n));
    write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    fprintf(stream, ") {");
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_for_loop_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%s%s (", ident, node_name(n));
    write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    fprintf(stream, ") {");
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_while_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%s%s (", ident, node_name(n));
    write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    fprintf(stream, ") {");
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_do_while_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%sdo {", ident);
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s} while(", ident);
    write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    fprintf(stream, ");\n");
}

static void write_loop_node(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%swhile (true) {", ident);
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_catch(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%s%s (", ident, node_name(n));
    jd_exp *param_exp = n->param_exp;
    if (param_exp != NULL) {
        jd_val *val = param_exp->data;
        fprintf(stream, "%s ", val->data->cname);
        write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    }
    fprintf(stream, ") {");
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_case(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    jd_case *_case = n->data;
    if (_case->is_default)
        fprintf(stream, "%sdefault: {", ident);
    else {
        fprintf(stream, "%s%s ", ident, node_name(n));
        fprintf(stream, "%d", _case->key);
        fprintf(stream, ": {");
    }
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_synchronized(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%s%s (", ident, node_name(n));
    if (n->param_exp != NULL)
        write_expression(jf, n, n->param_exp, n->param_exp->ins, false);
    fprintf(stream, ") {");
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

static void write_default(FILE *stream, jsource_file *jf, jd_node *n)
{
    string ident = get_node_ident(n);
    fprintf(stream, "%s", ident);
    fprintf(stream, "%s {", node_name(n));
    write_node_debug_info(jf, n);
    writter_for_class(jf, n);
    fprintf(stream, "%s}\n", ident);
}

void writter_for_class(jsource_file *jf, jd_node *node)
{
    if (node == NULL)
        node = lget_obj_first(jf->blocks);
    FILE *stream = file_output(jf);
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        switch (child->type) {
            case JD_NODE_PACKAGE_IMPORT:
                write_notice(jf);
                write_import(jf);
                break;
            case JD_NODE_CLASS:
                write_class(stream, child);
                break;
            case JD_NODE_FIELD:
                write_field(jf, child);
                break;
            case JD_NODE_METHOD:
                write_method(stream, jf, child);
                break;
            case JD_NODE_EXPRESSION:
                write_expression_node(stream, jf, child);
                break;
            case JD_NODE_BASIC_BLOCK:
                write_basic_block(stream, jf, child);
                break;
            case JD_NODE_DELETED:
                break;
            case JD_NODE_IF:
            case JD_NODE_ELSE_IF:
                write_if_node(stream, jf, child);
                break;
            case JD_NODE_SWITCH:
                write_switch_node(stream, jf, child);
                break;
            case JD_NODE_FOR:
                write_for_loop_node(stream, jf, child);
                break;
            case JD_NODE_WHILE:
                write_while_node(stream, jf, child);
                break;
            case JD_NODE_DO_WHILE:
                write_do_while_node(stream, jf, child);
                break;
            case JD_NODE_LOOP:
                write_loop_node(stream, jf, child);
                break;
            case JD_NODE_CATCH:
                write_catch(stream, jf, child);
                break;
            case JD_NODE_CASE:
                write_case(stream, jf, child);
                break;
            case JD_NODE_SYNCHRONIZED:
                write_synchronized(stream, jf, child);
                break;
            default:
                write_default(stream, jf, child);
                break;
        }
    }
}

void writter_for_anonymous_class(jsource_file *jf, jd_node *node)
{
    if (node == NULL)
        node = lget_obj_first(jf->blocks);
    FILE *stream = file_output(jf);
    for (int i = 0; i < node->children->size; ++i) {
        jd_node *child = lget_obj(node->children, i);
        switch (child->type) {
            case JD_NODE_CLASS:
                write_anonymous_class(stream, jf, child);
                break;
            case JD_NODE_FIELD:
                write_field(jf, child);
                break;
            case JD_NODE_METHOD:
                write_method(stream, jf, child);
                break;
            case JD_NODE_EXPRESSION:
                write_expression_node(stream, jf, child);
                break;
            case JD_NODE_BASIC_BLOCK:
                write_basic_block(stream, jf, child);
                break;
            case JD_NODE_DELETED:
                break;
            case JD_NODE_IF:
            case JD_NODE_ELSE_IF:
                write_if_node(stream, jf, child);
                break;
            case JD_NODE_SWITCH:
                write_switch_node(stream, jf, child);
                break;
            case JD_NODE_FOR:
                write_for_loop_node(stream, jf, child);
                break;
            case JD_NODE_WHILE:
                write_while_node(stream, jf, child);
                break;
            case JD_NODE_DO_WHILE:
                write_do_while_node(stream, jf, child);
                break;
            case JD_NODE_LOOP:
                write_loop_node(stream, jf, child);
                break;
            case JD_NODE_CATCH:
                write_catch(stream, jf, child);
                break;
            case JD_NODE_CASE:
                write_case(stream, jf, child);
                break;
            case JD_NODE_SYNCHRONIZED:
                write_synchronized(stream, jf, child);
                break;
            default:
                write_default(stream, jf, child);
                break;
        }
    }
}

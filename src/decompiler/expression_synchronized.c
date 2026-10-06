#include "decompiler/expression.h"
#include "decompiler/expression_synchronized.h"
#include "decompiler/expression_node.h"
#include "common/debug.h"
#include "dalvik/dex_ins.h"
#include "decompiler/klass.h"
#include "decompiler/method.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    SYNC_EXC_NODES,
    SYNC_CONVERTED,
    SYNC_MISS_NEAR,
    SYNC_MISS_FAR,
    SYNC_NO_FINALLY,
    SYNC_LEFTOVER_ENTER,
    SYNC_LEFTOVER_EXIT,
    SYNC_LEFTOVER_RECOG,
    SYNC_LEFTOVER_IN_SYNC,
    SYNC_STAT_N,
};

#define SYNC_NEAR 3

static pthread_mutex_t sync_stat_lock = PTHREAD_MUTEX_INITIALIZER;
static int sync_stat[SYNC_STAT_N];
static const char *sync_stat_name[SYNC_STAT_N] = {
    "exception nodes",
    "converted to synchronized",
    "  an enter within 3 expressions before the node",
    "  an enter further back than that",
    "  monitor-enter but no finally handler",
    "monitor-enter reaching the writer",
    "monitor-exit reaching the writer",
    "  ...of which open a recognised block",
    "  ...exits left inside a recognised block",
};

static bool sync_stat_on(void)
{
    return getenv("GARLIC_SYNC_STAT") != NULL;
}

static bool sync_trace_on(void)
{
    return getenv("GARLIC_SYNC_TRACE") != NULL;
}

static void sync_stat_add(int slot, int n)
{
    if (!sync_stat_on())
        return;
    pthread_mutex_lock(&sync_stat_lock);
    sync_stat[slot] += n;
    pthread_mutex_unlock(&sync_stat_lock);
}

void sync_stat_count_leftover(jd_method *m)
{
    if (!sync_stat_on())
        return;

    int enters = 0, exits = 0, recognised = 0, in_sync = 0;
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp))
            continue;
        if (exp_is_monitor_enter(exp)) {
            enters++;

            for (int j = 0; j < m->nodes->size; ++j) {
                jd_node *n = lget_obj(m->nodes, j);
                if (!node_is_synchronized(n) || n->start_idx <= i)
                    continue;
                if (prev_valid_exp(m, n->start_idx - 1) == exp) {
                    recognised++;
                    break;
                }
            }
        }
        else if (exp_is_monitor_exit(exp)) {
            exits++;

            for (int j = 0; j < m->nodes->size; ++j) {
                jd_node *n = lget_obj(m->nodes, j);
                if (!node_is_synchronized(n))
                    continue;
                if (n->start_idx <= i && i <= n->end_idx) {
                    in_sync++;
                    break;
                }
            }
        }
    }
    sync_stat_add(SYNC_LEFTOVER_ENTER, enters);
    sync_stat_add(SYNC_LEFTOVER_EXIT, exits);
    sync_stat_add(SYNC_LEFTOVER_RECOG, recognised);
    sync_stat_add(SYNC_LEFTOVER_IN_SYNC, in_sync);
}

void sync_stat_report(void)
{
    if (!sync_stat_on())
        return;
    for (int i = 0; i < SYNC_STAT_N; ++i) {
        fprintf(stderr, "[garlic] sync: %-42s %d\n",
                sync_stat_name[i], sync_stat[i]);
    }
}

static jd_exp* find_monitor_enter_back(jd_method *m, int index, int *out_distance)
{
    int distance = 0;
    for (jd_exp *exp = prev_valid_exp(m, index);
         exp != NULL && distance < 32;
         exp = prev_valid_exp(m, exp->idx - 1), ++distance) {
        if (exp_is_monitor_enter(exp)) {
            if (out_distance != NULL)
                *out_distance = distance;
            return exp;
        }
    }
    return NULL;
}

static int monitor_expression_register(jd_exp *exp)
{
    if (exp == NULL || exp->ins == NULL || exp->ins->type != JD_TYPE_DALVIK)
        return -1;
    if (!exp_is_monitor_exit(exp) && !exp_is_monitor_enter(exp))
        return -1;

    return (int) dex_ins_parameter((jd_dex_ins *) exp->ins, 0);
}

static void nop_monitor_exits_of(jd_method *m, int reg)
{
    if (reg < 0)
        return;

    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp) || !exp_is_monitor_exit(exp))
            continue;
        if (monitor_expression_register(exp) == reg)
            exp_mark_nopped(exp);
    }
}

static void nop_monitor_exit_recursive(jd_method *m, jd_node *node)
{
    if (node_is_expression(node)) {
        if (exp_is_monitor_exit(node->data))
            exp_mark_nopped(node->data);
        return;
    }

    if (node_is_basic_block(node)) {
        jd_bblock *b = node->data;
        jd_nblock *nb = b->ub->nblock;
        for (int i = nb->start_idx; i <= nb->end_idx ; ++i) {
            jd_exp *exp = get_exp(m, i);
            if (exp_is_monitor_exit(exp))
                exp_mark_nopped(exp);
        }
    }
    else {
        for (int i = 0; i < node->children->size; ++i) {
            jd_node *child = lget_obj(node->children, i);
            if (node_is_exception(child)) {
                jd_node *inner = child_of_type(child, JD_NODE_TRY);
                if (inner != NULL)
                    nop_monitor_exit_recursive(m, inner);
                continue;
            }
            nop_monitor_exit_recursive(m, child);
        }
    }
}

#define SYNC_CONSUMED_MAX 64

static void settle_synchronized_method(jd_method *m,
                                       int *consumed,
                                       int consumed_size)
{
    u4 sync_flags = ACC_DEX_SYNCHRONIZED | ACC_DEX_DECLARED_SYNCHRONIZED;
    if (!access_flags_contains(m->access_flags, sync_flags))
        return;

    jd_exp *enter = NULL;
    for (int i = 0; i < m->expressions->size && enter == NULL; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (!exp_is_nopped(exp) && exp_is_monitor_enter(exp))
            enter = exp;
    }
    if (enter == NULL)
        return;

    for (int i = 0; i < consumed_size; ++i) {
        if (consumed[i] == enter->idx) {
            method_mark_sync_block(m);
            return;
        }
    }

    int reg = monitor_expression_register(enter);
    for (int i = 0; i < m->expressions->size; ++i) {
        jd_exp *exp = lget_obj(m->expressions, i);
        if (exp_is_nopped(exp))
            continue;
        if (monitor_expression_register(exp) == reg)
            exp_mark_nopped(exp);
    }
}

static void sync_dump_nodes(jd_method *m, const char *when)
{
    if (getenv("GARLIC_SYNC_TRACE") == NULL)
        return;

    fprintf(stderr, "[garlic] sync %s %s: --\n", when, m->name);
    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *n = lget_obj(m->nodes, i);
        if (n->type != JD_NODE_EXCEPTION && n->type != JD_NODE_TRY &&
            n->type != JD_NODE_CATCH && n->type != JD_NODE_FINALLY &&
            n->type != JD_NODE_SYNCHRONIZED)
            continue;
        fprintf(stderr, "[garlic]   node %d type %d [%d..%d] parent %d "
                        "children %zu\n",
                n->node_id, (int) n->type, n->start_idx, n->end_idx,
                n->parent == NULL ? -1 : n->parent->node_id,
                n->children == NULL ? 0 : n->children->size);
    }
}

int identify_synchronized(jd_method *m)
{
    int consumed[SYNC_CONSUMED_MAX];
    int consumed_size = 0;

    for (int i = 0; i < m->nodes->size; ++i) {
        jd_node *node = lget_obj(m->nodes, i);
        if (node_is_not_exception(node))
            continue;
        sync_stat_add(SYNC_EXC_NODES, 1);
        jd_exp *exp = prev_valid_exp(m, node->start_idx - 1);
        bool prev_is_enter = exp != NULL && exp_is_monitor_enter(exp);
        if (!prev_is_enter && (sync_stat_on() || sync_trace_on())) {
            int distance = -1;
            jd_exp *enter = find_monitor_enter_back(m, node->start_idx - 1,
                                                   &distance);
            jd_node *fin = child_of_type(node, JD_NODE_FINALLY);
            if (enter != NULL && fin != NULL) {
                jd_node *tryn = child_of_type(node, JD_NODE_TRY);
                sync_stat_add(distance <= SYNC_NEAR ? SYNC_MISS_NEAR
                                                    : SYNC_MISS_FAR, 1);
                jd_bblock *enter_bb = enter->block;
                jd_nblock *enter_nb = (enter_bb != NULL &&
                                       enter_bb->type == JD_BB_NORMAL)
                                          ? enter_bb->ub->nblock : NULL;
                int prev_idx = exp == NULL ? -1 : exp->idx;
                bool enter_last = enter_nb != NULL &&
                                  enter->idx == enter_nb->end_idx;
                bool enter_in_try_bb = exp != NULL &&
                                       exp->block == enter_bb;
                if (sync_trace_on())
                fprintf(stderr,
                        "[garlic] sync miss: %s%s enter@%d node@%d dist=%d "
                        "enter_block=[%d,%d] enter_last=%d same_bb_as_prev=%d "
                        "prev@%d finally[%d,%d] try[%d,%d]\n",
                        m->name, m->signature == NULL ? "" : m->signature,
                        enter->idx, node->start_idx, distance,
                        enter_nb == NULL ? -1 : enter_nb->start_idx,
                        enter_nb == NULL ? -1 : enter_nb->end_idx,
                        enter_last, enter_in_try_bb, prev_idx,
                        fin->start_idx, fin->end_idx,
                        tryn == NULL ? -1 : tryn->start_idx,
                        tryn == NULL ? -1 : tryn->end_idx);
            }
        }
        if (exp == NULL || !exp_is_monitor_enter(exp))
            continue;

        bool taken = false;
        for (int k = 0; k < consumed_size; ++k) {
            if (consumed[k] == exp->idx) {
                taken = true;
                break;
            }
        }
        if (taken)
            continue;

        jd_node *finally = child_of_type(node, JD_NODE_FINALLY);
        if (finally == NULL) {
            sync_stat_add(SYNC_NO_FINALLY, 1);
            continue;
        }

        if (consumed_size < SYNC_CONSUMED_MAX)
            consumed[consumed_size++] = exp->idx;

        sync_stat_add(SYNC_CONVERTED, 1);
        if (sync_trace_on())
            fprintf(stderr, "[garlic] sync convert: %s%s node[%d,%d] "
                    "enter@%d\n",
                    m->name, m->signature == NULL ? "" : m->signature,
                    node->start_idx, node->end_idx, exp->idx);

        sync_dump_nodes(m, "before");

        jd_node *try = child_of_type(node, JD_NODE_TRY);
        ldel_obj(node->children, finally);

        nop_monitor_exit_recursive(m, try);
        nop_monitor_exits_of(m, monitor_expression_register(exp));

        bool keeps_catch = false;
        for (int k = 0; k < node->children->size; ++k) {
            if (((jd_node *) lget_obj(node->children, k))->type == JD_NODE_CATCH)
                keeps_catch = true;
        }

        if (keeps_catch) {
            node->type = JD_NODE_SYNCHRONIZED;
        }
        else {
            jd_node *parent = node->parent;
            int index = lfind_object(parent->children, node);
            ladd_obj_at(parent->children, try, index);
            ldel_obj(parent->children, node);
            try->parent = parent;
            try->type = JD_NODE_SYNCHRONIZED;
            ldel_obj(m->nodes, node);
            i--;
        }

        sync_dump_nodes(m, "after");
    }

    settle_synchronized_method(m, consumed, consumed_size);
    return 0;
}

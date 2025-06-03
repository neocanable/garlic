#include "scc.h"
#include "debug.h"

static void strong_connect(
        jd_bblock *block,
        jd_tarjan_state *state,
        list_object *stack,
        int *index,
        list_object *sccs
) {
    state[block->block_id].index = *index;
    state[block->block_id].low_link = *index;
    (*index)++;
    ladd_obj(stack, block);
    state[block->block_id].on_stack = true;

    for (int i = 0; i < block->out->size; ++i) {
        jd_edge *edge = lget_obj(block->out, i);
        jd_bblock *target = edge->target_block;
        if (state[target->block_id].index == -1) {
            strong_connect(target, state, stack, index, sccs);
            state[block->block_id].low_link = MIN(
                    state[block->block_id].low_link,
                    state[target->block_id].low_link
            );
        }
        else if (state[target->block_id].on_stack) {
            state[block->block_id].low_link = MIN(
                    state[block->block_id].low_link,
                    state[target->block_id].low_link
            );
        }
    }

    if (state[block->block_id].low_link == state[block->block_id].index) {
        list_object *scc = linit_object();
        jd_bblock *popped = NULL;
        do {
            popped = lget_obj_last(stack);
            ldel_object(stack, popped);
            state[popped->block_id].on_stack = false;
            ladd_obj(scc, popped);
        } while (popped != block);
        if (scc->size > 1)
            ladd_obj(sccs, scc);
    }
}


// 主入口函数
list_object* compute_scc(jd_method *m) {
    list_object *blocks = m->basic_blocks;
    jd_tarjan_state *state = make_obj_arr(jd_tarjan_state, blocks->size);
    for (int i = 0; i < blocks->size; ++i) {
        state[i].index = -1;
        state[i].on_stack = false;
    }


    list_object *stack = linit_object();
    list_object *sccs = linit_object();
    int index = 0;

    for (int i = 0; i < blocks->size; ++i) {
        jd_bblock *block = lget_object(blocks, i);
        if (state[block->block_id].index == -1)
            strong_connect(block, state, stack, &index, sccs);
    }


    for (int i = 0; i < sccs->size; i++) {
        list_object *scc = lget_obj(sccs, i);
        DEBUG_PRINT("SCC %d (include %zu blocks):\n", i+1, scc->size);
        for (int j = 0; j < scc->size; j++) {
            jd_bblock *block = lget_obj(scc, j);
            printf("  Block %zu\n", block->block_id);
        }
    }
    return sccs;
}

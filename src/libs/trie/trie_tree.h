#ifndef GARLIC_TRIE_TREE_H
#define GARLIC_TRIE_TREE_H

#include "types.h"
#include "mem_pool.h"
#include <stdio.h>

typedef struct jd_trie_node {
    char* segment;
    struct jd_trie_node* child;
    struct jd_trie_node* next;
    bool is_leaf;
    string full;
} jd_trie_node;

void trie_insert(jd_trie_node *root, string path);

jd_trie_node* trie_create_node(string str);

int trie_search(jd_trie_node *root, string path);

void trie_leaf_to_stream(jd_trie_node *node, FILE *stream);

void tire_merge(jd_trie_node *dst_root, jd_trie_node *src_root);

#endif //GARLIC_TRIE_TREE_H

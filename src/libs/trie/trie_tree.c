#include "libs/trie/trie_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/str_tools.h"

#define TRIE_SPLIT_CHAR '/'
#define TRIE_SPLIT_STR "/"

jd_trie_node* trie_create_node(char *str) {
    jd_trie_node *node = make_obj(jd_trie_node);
    node->segment = str_dup(str);
    node->child = NULL;
    node->next = NULL;
    return node;
}

static void tire_travel_merge(jd_trie_node *dst, jd_trie_node *node)
{
    if(node == NULL)
        return;
    if (node->is_leaf)
        trie_insert(dst, node->full);

    if (node->child != NULL)
        tire_travel_merge(dst, node->child);
    if (node->next != NULL)
        tire_travel_merge(dst, node->next);
}

void tire_merge(jd_trie_node *dst_root, jd_trie_node *src_root)
{
    if (src_root == NULL)
        return;
    tire_travel_merge(dst_root, src_root);
}

void trie_insert(jd_trie_node *root, string path) {
    char *token;
    char *copy = str_dup(path);
    char *saveptr;
    token = strtok_r(copy, TRIE_SPLIT_STR, &saveptr);
    jd_trie_node *current_level = root;

    bool add_new_leaf = false;
    jd_trie_node *current_node = NULL;
    while(token != NULL) {
        current_node = current_level->child;
        jd_trie_node *prev = NULL;

        while(current_node != NULL &&
                strcmp(current_node->segment, token) != 0) {
            prev = current_node;
            current_node = current_node->next;
        }

        if(current_node == NULL) {
            current_node = trie_create_node(token);
            add_new_leaf = true;
            if(prev == NULL) {
                current_level->child = current_node;
            } else {
                prev->next = current_node;
            }
        }

        current_level = current_node;
        token = strtok_r(NULL, TRIE_SPLIT_STR, &saveptr);
    }
    if (add_new_leaf) {
        current_node->is_leaf = true;
        current_node->full = path;
    }
}

int trie_search(jd_trie_node *root, string path) {
    const char* current_path = path;
    jd_trie_node *current_node = root->child;

    while (*current_path != '\0' && current_node != NULL) {
        if (*current_path == TRIE_SPLIT_CHAR) {
            ++current_path;
            continue;
        }

        jd_trie_node *node = current_node;
        int matched = 0;
        size_t segment_length = 0;
        while (node != NULL) {
            const char* end = strchr(current_path, TRIE_SPLIT_CHAR);
            segment_length = (end ? end - current_path : strlen(current_path));

            if (strncmp(node->segment, current_path, segment_length) == 0 &&
                node->segment[segment_length] == '\0') {
                matched = 1;
                break;
            }
            node = node->next;
        }

        if (!matched) {
            return 0;
        }

        if (node) {
            current_node = node->child;
            current_path += segment_length;
        }

        if (*current_path == TRIE_SPLIT_CHAR) {
            ++current_path;
        }
    }

    return (*current_path == '\0' && current_node == NULL);
}

void trie_leaf_to_stream(jd_trie_node *node, FILE *stream)
{
    if(node == NULL)
        return;
    if (node->is_leaf) {
        fprintf(stream, "import ");
        for (int i = 0; i < strlen(node->full); ++i) {
            unsigned char c = node->full[i];
            if (c == '/')
                fprintf(stream, ".");
            else
                fprintf(stream, "%c", c);
        }
        fprintf(stream, ";\n");
    }

    if (node->child != NULL)
        trie_leaf_to_stream(node->child, stream);
    if (node->next != NULL)
        trie_leaf_to_stream(node->next, stream);
}
#include "libs/trie/trie_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/str_tools.h"

#define TRIE_SPLIT_CHAR '/'
#define TRIE_SPLIT_STR "/"

// 创建一个新的Trie节点
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

// 插入一个字符串到Trie中
void trie_insert(jd_trie_node *root, string path) {
    char *token;
    char *copy = strdup(path);
    token = strtok(copy, TRIE_SPLIT_STR);
    jd_trie_node *current_level = root;

    bool add_new_leaf = false;
    jd_trie_node *current_node = NULL;
    while(token != NULL) {
        current_node = current_level->child;
        jd_trie_node *prev = NULL;

        // 在当前层寻找是否存在相同的segment
        while(current_node != NULL && 
                strcmp(current_node->segment, token) != 0) {
            prev = current_node;
            current_node = current_node->next;
        }

        if(current_node == NULL) { // 如果没有找到匹配的节点，则创建新节点
            current_node = trie_create_node(token);
            add_new_leaf = true;
            if(prev == NULL) {
                current_level->child = current_node;
            } else {
                prev->next = current_node;
            }
        }

        current_level = current_node; // 移动到下一层
        token = strtok(NULL, TRIE_SPLIT_STR);
    }
    if (add_new_leaf) {
        current_node->is_leaf = true;
        current_node->full = path;
    }

    free(copy); // 释放复制的字符串
}

// 搜索特定路径是否存在于Trie中
int trie_search(jd_trie_node *root, string path) {
    const char* current_path = path;
    jd_trie_node *current_node = root->child; // 从第一个实际数据节点开始

    while (*current_path != '\0' && current_node != NULL) {
        // 跳过点号
        if (*current_path == TRIE_SPLIT_CHAR) {
            ++current_path;
            continue;
        }

        // 查找当前段
        jd_trie_node *node = current_node;
        int matched = 0;
        size_t segment_length = 0; // 提前声明segmentLength
        while (node != NULL) {
            // 计算当前段的长度
            const char* end = strchr(current_path, TRIE_SPLIT_CHAR);
            segment_length = (end ? end - current_path : strlen(current_path));

            // 比较当前段
            if (strncmp(node->segment, current_path, segment_length) == 0 &&
                node->segment[segment_length] == '\0') {
                matched = 1;
                break;
            }
            node = node->next;
        }

        if (!matched) {
            return 0; // 当前段未匹配，路径不存在
        }

        // 移动到下一段
        if (node) {
            current_node = node->child;
            current_path += segment_length;
        }

        // 如果下一个字符是点，则跳过它
        if (*current_path == TRIE_SPLIT_CHAR) {
            ++current_path;
        }
    }

    // 如果所有段都匹配完毕且没有剩余路径，返回1表示找到
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
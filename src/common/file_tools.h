#ifndef GARLIC_FILE_TOOLS_H
#define GARLIC_FILE_TOOLS_H

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "types.h"

static bool inline file_exist(const char *path)
{
    return access(path, F_OK) == 0;
}

static inline void make_dir(const char *dir)
{
    if (!dir) {
        fprintf(stderr, "ERROR: make_dir called with NULL\n");
        return;
    }
    
    // fprintf(stderr, "DEBUG: make_dir called with: '%s' (len=%zu)\n", dir, strlen(dir));
    
    char tmp[256];
    char *p = NULL;
    size_t len;

    if (strlen(dir) >= sizeof(tmp)) {
        fprintf(stderr, "ERROR: directory path too long: %zu\n", strlen(dir));
        return;
    }

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

static inline void mkdir_p(const char *dir)
{
    struct stat sb;
    if (stat(dir, &sb) == -1)
        make_dir(dir);
}

#endif //GARLIC_FILE_TOOLS_H

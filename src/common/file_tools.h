#ifndef GARLIC_FILE_TOOLS_H
#define GARLIC_FILE_TOOLS_H

#include <unistd.h>
#include <stdbool.h>

static bool inline file_exist(const char *path)
{
    return access(path, F_OK) == 0;
}

static inline void make_dir(const char *dir)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
#ifdef _WIN32
            mkdir(tmp);
#else
            mkdir(tmp, S_IRWXU);
#endif
            *p = '/';
        }
#ifdef _WIN32
    mkdir(tmp);
#else
    mkdir(tmp, S_IRWXU);
#endif
}

static inline void mkdir_p(string dir)
{
    struct stat sb;
    if (stat(dir, &sb) == -1)
        make_dir(dir);
}

#endif //GARLIC_FILE_TOOLS_H

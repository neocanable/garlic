#ifndef GARLIC_STR_TOOLS_H
#define GARLIC_STR_TOOLS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include "mem_pool.h"
#include "types.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define STR_EQL(a, b) (strcmp(a, b) == 0)

static inline string str_create(string fmt, ...)
{
    va_list args;
    string str;
    va_start(args, fmt);
    va_list args2;
    va_copy(args2, args);
    int len = vsnprintf(NULL, 0, fmt, args2);
    str = x_alloc(len+1);
    vsnprintf(str, len+1, fmt, args);
    va_end(args);

    return str;
}

static inline string str_create_in(mem_pool *pool, string fmt, ...)
{
    va_list args;
    string str;
    va_start(args, fmt);
    va_list args2;
    va_copy(args2, args);
    int len = vsnprintf(NULL, 0, fmt, args2);
    str = x_alloc_in(pool, len+1);
    vsnprintf(str, len+1, fmt, args);
    str[len] = '\0';
    va_end(args);

    return str;
}

static inline string i2a(int num)
{
    return str_create("%d", num);
}

static inline string l2a(long num)
{
    return str_create("%ld", num);
}

static inline string double2a(double num)
{
    return str_create("%f", num);
}

/* Case-sensitive: does s end with the suffix suff? */
static inline int str_end_with(const char *s, const char *suff)
{
    size_t slen     = strlen(s);
    size_t sufflen  = strlen(suff);
    return slen >= sufflen &&
        !memcmp(s + slen - sufflen, suff, sufflen);
}

/* Case-insensitive: does s end with the suffix suff? */
static inline int str_end_with_lower(const char *s, const char *suff)
{
    size_t slen     = strlen(s);
    size_t sufflen  = strlen(suff);
    if (slen < sufflen)
        return 0;
    const char *p = s + slen - sufflen;
    for (size_t i = 0; i < sufflen; ++i) {
        if (tolower((unsigned char)p[i]) != tolower((unsigned char)suff[i]))
            return 0;
    }
    return 1;
}

/* File extensions for Android APK and split-APK containers (.apk/.xapk/.apks) */
#define APK_EXTENSION       ".apk"
#define SPLIT_APK_XAPK      ".xapk"
#define SPLIT_APK_APKS      ".apks"

/* Case-insensitive check: does path end in an APK / split-APK container extension? */
static inline int str_is_apk_path(const char *path)
{
    return str_end_with_lower(path, APK_EXTENSION)  ||
           str_end_with_lower(path, SPLIT_APK_XAPK) ||
           str_end_with_lower(path, SPLIT_APK_APKS);
}

static inline int str_start_with(const string s, const string suff)
{
    size_t slen     = strlen(s);
    size_t sufflen  = strlen(suff);
    return slen >= sufflen && !memcmp(s, suff, sufflen);
}

static inline string str_dup(const char *str)
{
    if(str == NULL)
        return NULL;
    size_t len = strlen(str);
    string new_str = x_alloc(len + 1);
    memcpy(new_str, str, len);
    new_str[len] = '\0';
    return new_str;
//    return str_create("%s", str);
}

static inline int str_replace_char(char *str, char orig, char rep)
{
    char *ix = str;
    int n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

static inline string sub_str(string src, int from, int to)
{
    size_t len = to - from + 1;
    string str = x_alloc(len);
    strncpy(str, src + from, len);
    return str;
}

static inline bool str_contains(const string str, const string sub)
{
    return strstr(str, sub) != NULL;
}


static inline char* get_last_word_lower(const char* class_name) {
    int last_upper_pos = -1;
    size_t len = strlen(class_name);

    for (size_t i = 0; i < len; i++) {
        if (isupper((unsigned char)class_name[i]))
            last_upper_pos = (int)i;
    }

    if (last_upper_pos == -1)
        last_upper_pos = 0;

    char* result = x_alloc(len - last_upper_pos + 1);
    if (!result) 
        return NULL;

    strcpy(result, class_name + last_upper_pos);
    for (char* p = result; *p; p++)
        *p = tolower((unsigned char)*p);

    return result;
}

static inline int number_digits(int num)
{
    if (num == 0) return 1;
    return floor(log10(abs(num))) + 1;
}

static inline string str_replace_nl(string src)
{
    size_t len = strlen(src);
    size_t new_len = len;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) src[i];
        if (c == '\n' || c == '\r' || c == '\t' || c == '"' || c == '\\')
            new_len += 1;
        else if (c < 0x20)
            new_len += 5;
    }

    char *dst = x_alloc(new_len + 1);
    if (!dst) return NULL;

    const char *p = src;
    char *q = dst;

    while (*p) {
        unsigned char c = (unsigned char) *p;
        switch (c) {
            case '\n': *q++ = '\\'; *q++ = 'n';  break;
            case '\r': *q++ = '\\'; *q++ = 'r';  break;
            case '\t': *q++ = '\\'; *q++ = 't';  break;
            case '"':  *q++ = '\\'; *q++ = '"';  break;
            case '\\': *q++ = '\\'; *q++ = '\\'; break;
            default:
                if (c < 0x20)
                    q += sprintf(q, "\\u%04x", c);
                else
                    *q++ = (char) c;
                break;
        }
        p++;
    }
    *q = '\0';
    return dst;
}

#endif //GARLIC_STR_TOOLS_H

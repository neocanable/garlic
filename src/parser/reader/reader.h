#ifndef GARLIC_READER_H
#define GARLIC_READER_H

#include <string.h>
#include "common/types.h"

typedef struct jd_bin {
    char           *buffer;
    size_t         buffer_size;
    size_t         cur_off;
} jd_bin;

static inline size_t jd_bin_read(jd_bin *bin, void *ptr, size_t size)
{
    memcpy(ptr, &bin->buffer[bin->cur_off], size);
    bin->cur_off += size;
    return size;
}

static inline size_t jd_bin_read1(jd_bin *bin, void* ptr)
{
    return jd_bin_read(bin, ptr, sizeof(u1));
}

static inline size_t jd_bin_read2(jd_bin *bin, void* ptr)
{
    return jd_bin_read(bin, ptr, sizeof(u2));
}

static inline size_t jd_bin_read4(jd_bin *bin, void* ptr)
{
    return jd_bin_read(bin, ptr, sizeof(u4));
}

#endif //GARLIC_READER_H

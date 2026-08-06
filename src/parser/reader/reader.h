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
    if (bin->cur_off + size > bin->buffer_size) {
        static int err_count = 0;
//        if (err_count < 10) {
//            fprintf(stderr, "[ERROR] jd_bin_read: reading past buffer end "
//                    "(off=%zu size=%zu buf_size=%zu)\n",
//                    bin->cur_off, size, bin->buffer_size);
//        } else if (err_count == 10) {
//            fprintf(stderr, "[ERROR] jd_bin_read: further overflows suppressed\n");
//        }
        err_count++;
        return 0;
    }
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

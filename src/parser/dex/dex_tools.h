#ifndef GARLIC_DEX_TOOLS_H
#define GARLIC_DEX_TOOLS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "parser/dex/dex.h"
#include "dex_structure.h"

#define jdex_read1(dex, ptr) (jdex_read(dex, ptr, sizeof(u1)))
#define jdex_read2(dex, ptr) (jdex_read(dex, ptr, sizeof(u2)))
#define jdex_read4(dex, ptr) (jdex_read(dex, ptr, sizeof(u4)))
#define jdex_read(dex, ptr, size) (jd_bin_read(dex->bin, ptr, size))

static inline bool dex_encoded_method_is_lambda(jd_meta_dex *meta,
                                                encoded_method *em)
{
    dex_method_id method_id = meta->method_ids[em->method_id];
    string name = meta->strings[method_id.name_idx].data;
    return str_contains(name, "lambda$") &&
            (em->access_flags & ACC_DEX_SYNTHETIC) != 0;
}

#endif //GARLIC_DEX_TOOLS_H

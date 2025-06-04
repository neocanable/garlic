#ifndef GARLIC_CLASS_TOOLS_H
#define GARLIC_CLASS_TOOLS_H

// Include endian headers first
#ifdef __APPLE__
#include "endian_osx.h"
#endif

#ifdef __linux__
#include <endian.h>
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#endif

// Include our common endian header
#include "common/endian_osx.h"

#include "class_structure.h"
#include "decompiler/structure.h"
#include <string.h>
#include <time.h>

#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mem_pool.h"
#include "common/debug.h"

#define jclass_read(jclass, ptr, size) (jd_bin_read(jclass->bin, ptr, size))
#define jclass_read1(jclass, ptr) (jclass_read(jclass, ptr, sizeof(u1)))
#define jclass_read2(jclass, ptr) (jclass_read(jclass, ptr, sizeof(u2)))
#define jclass_read4(jclass, ptr) (jclass_read(jclass, ptr, sizeof(u4)))

char* get_method_access_flags_str(u2 access_flag);

char* get_class_access_flags_str(u2 access_flag);

// for opcode hashmap
//struct hashmap class_opcode;

void init_class_opcode_hashmap(jclass_file *jc);

int get_opcode_param_length(jclass_file *jc, u1 code);

int get_opcode_pushed(jclass_file *jc, u1 code);

int get_opcode_popped(jclass_file *jc, u1 code);

char* get_opcode_name(jclass_file *jc, u1 code);


// TODO: fix the function cname
void expand_descriptor(jd_descriptor *descriptor);

static inline char* pool_u1_str(jclass_file *jc, u1 index)
{
    jcp_info* info = &jc->constant_pool[index - 1];
    return info->readable;
}

static inline char* pool_str(jclass_file* jc, u2 index)
{
    uint16_t idx = be16toh(index);
    jcp_info* info = &jc->constant_pool[idx - 1];
    return info->readable;
}

static inline jcp_info* pool_u1_item(jclass_file *jc, u1 index)
{
    return &jc->constant_pool[index - 1];
}

static inline jcp_info* pool_item(jclass_file *jc, u2 index)
{
    return &jc->constant_pool[be16toh(index) - 1];
}

static inline jcp_info* pool_u4_item(jclass_file *jc, u4 index)
{
    return &jc->constant_pool[be32toh(index) - 1];
}

static inline uint32_t be_32(u1 p1, u1 p2, u1 p3, u1 p4)
{
    return (p1 << 24) | (p2 << 16) | (p3 << 8) | p4;
}

static inline int32_t be_32s(u1 p1, u1 p2, u1 p3, u1 p4)
{
    return (p1 << 24) | (p2 << 16) | (p3 << 8) | p4;
}

static inline uint16_t be_16(u1 p1, u1 p2)
{
    return (p1 << 8) | p2;
}

static inline int16_t be_16s(u1 p1, u1 p2)
{
    return (p1 << 8) | p2;
}


static inline int get_const_int(jcp_info *info)
{
    jconst_integer *i = info->info->integer;
    return be32toh(i->bytes);
}

static inline float get_const_float(jcp_info *info)
{
    jconst_float *f = info->info->float_info;
    return ntohl(f->bytes);
}

static inline double get_const_double(jcp_info *info)
{
    jconst_double *d = info->info->double_info;

    int64_t double_value = ((int64_t) (ntohl(d->high_bytes)) << 32) |
                           (uint32_t) ntohl(d->low_bytes);
    double dval;
    memcpy(&dval, &double_value, sizeof(double));
    return dval;
}


static inline long get_const_long(jcp_info *info)
{
    jconst_long *l = info->info->long_info;
    return ((int64_t) (ntohl(l->high_bytes)) << 32) |
           (uint32_t) ntohl(l->low_bytes);
}

static inline string get_const_string(jclass_file *jc, jcp_info *info)
{
    jconst_string *s    = info->info->string_info;
    jcp_info *utf8_info = pool_item(jc, s->string_index);
    jconst_utf8 *utf8   = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string get_class_name(jclass_file *jc, jcp_info *info)
{
    jconst_class *c     = info->info->class;
    jcp_info *utf8_info = pool_item(jc, c->name_index);
    jconst_utf8 *utf8   = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string get_field_name(jclass_file *jc, jcp_info *info)
{
    jconst_fieldref *f          = info->info->fieldref;
    jcp_info *name_info         = pool_item(jc, f->name_and_type_index);
    jconst_name_and_type *name  = name_info->info->name_and_type;
    jcp_info *utf8_info         = pool_item(jc, name->name_index);
    jconst_utf8 *utf8           = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string get_field_class(jclass_file *jc, jcp_info *info)
{
    jconst_fieldref *f   = info->info->fieldref;
    jcp_info *class_info = pool_item(jc, f->class_index);
    jconst_class *class  = class_info->info->class;
    jcp_info *utf8_info  = pool_item(jc, class->name_index);
    jconst_utf8 *utf8    = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string get_field_descriptor(jclass_file *jc, jcp_info *info)
{
    jconst_fieldref *f          = info->info->fieldref;
    jcp_info *name_info         = pool_item(jc, f->name_and_type_index);
    jconst_name_and_type *name  = name_info->info->name_and_type;
    jcp_info *utf8_info         = pool_item(jc, name->descriptor_index);
    jconst_utf8 *utf8           = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string field_descriptor_of_ins(jd_ins *ins) {
    /**
     * getfiled/putfield/getstatic/putstatic
     **/
    u2 index = be16toh(ins->param[0] << 8 | ins->param[1]);
    jclass_file *jc = ins->method->meta;
    jcp_info *info = pool_item(ins->method->meta, index);
    return get_field_descriptor(jc, info);
}

static inline string get_method_name(jclass_file *jc, jcp_info *info)
{
    jconst_methodref *m         = info->info->methodref;
    jcp_info *name_info         = pool_item(jc, m->name_and_type_index);
    jconst_name_and_type *name  = name_info->info->name_and_type;
    jcp_info *utf8_info         = pool_item(jc, name->name_index);
    jconst_utf8 *utf8           = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string get_method_class(jclass_file *jc, jcp_info *info)
{
    jconst_methodref *m     = info->info->methodref;
    jcp_info *class_info    = pool_item(jc, m->class_index);
    jconst_class *class     = class_info->info->class;
    jcp_info *utf8_info     = pool_item(jc, class->name_index);
    jconst_utf8 *utf8       = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline string get_method_descriptor(jclass_file *jc, jcp_info *info)
{
    jconst_methodref *m         = info->info->methodref;
    jcp_info *name_info         = pool_item(jc, m->name_and_type_index);
    jconst_name_and_type *name  = name_info->info->name_and_type;
    jcp_info *utf8_info         = pool_item(jc, name->descriptor_index);
    jconst_utf8 *utf8           = utf8_info->info->utf8;

    return utf8_info->readable;
}

static inline u2 name_and_type_name_index(jclass_file *jc, u2 nt_index)
{
    jcp_info *cp_info = pool_item(jc, nt_index);
    jconst_name_and_type *name = cp_info->info->name_and_type;
    return name->name_index;
}

static inline string name_and_type_name(jclass_file *jc, u2 nt_index)
{
    jcp_info *cp_info = pool_item(jc, nt_index);
    jconst_name_and_type *name = cp_info->info->name_and_type;
    jcp_info *utf8_info = pool_item(jc, name->name_index);

    return utf8_info->readable;
}

static inline string name_and_type_descriptor(jclass_file *jc, u2 nt_index)
{
    jcp_info *cp_info = pool_item(jc, nt_index);
    jconst_name_and_type *name = cp_info->info->name_and_type;
    jcp_info *utf8_info = pool_item(jc, name->descriptor_index);

    return utf8_info->readable;
}


#endif //GARLIC_CLASS_TOOLS_H

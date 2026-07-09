#ifndef GARLIC_TYPES_H
#define GARLIC_TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef char*       string;
typedef void*       object;

typedef uint8_t     u1;
typedef uint16_t    u2;
typedef uint32_t    u4;
typedef uint64_t    u8;

typedef int8_t      s1;
typedef int16_t     s2;
typedef int32_t     s4;
typedef int64_t     s8;

#define JAVA_CLASS_MAGIC 0xCAFEBABE
#define JAR_FILE_MAGIC   0x504B0304
#define DEX_FILE_MAGIC   0x6465780A
#define ZIP_FILE_MAGIC   0x504B0304

#endif //GARLIC_TYPES_H

#ifndef __COMMON_ENDIAN_H__
#define __COMMON_ENDIAN_H__ 1

/** compatibility header for endian.h
 * This is a simple compatibility shim to convert
 * BSD/Linux endian macros to the Mac OS X equivalents.
 * It is public domain.
 * */

#ifdef __APPLE__
#include "endian_osx.h"
#elif defined(_WIN32) || defined(__MINGW32__)
#include <winsock2.h>
#include <stdint.h>

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)

#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)

// Windows doesn't have htonll/ntohll, so we define them
#ifndef htonll
#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))
#endif
#ifndef ntohll
#define ntohll(x) ((((uint64_t)ntohl(x)) << 32) + ntohl((x) >> 32))
#endif

#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#else
#include <endian.h>
#endif

#endif /* __COMMON_ENDIAN_H__ */
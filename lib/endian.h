/*
 * Portable endian stuff.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#ifndef ENDIAN_H
#define ENDIAN_H

#if ATMEL_SOFTWARE_FRAMEWORK

#include <compiler.h>
#define __LITTLE_ENDIAN 1
#define __BIG_ENDIAN    2
#define __BYTE_ORDER    __LITTLE_ENDIAN

#else

#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/param.h>

#ifndef __BYTE_ORDER
#define __BYTE_ORDER    BYTE_ORDER
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN    BIG_ENDIAN
#endif

#ifndef le32_to_cpu
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le32_to_cpu
#define cpu_to_le32
#define be32_to_cpu     __builtin_bswap32
#define cpu_to_be32     __builtin_bswap32
#else
#define le32_to_cpu     __builtin_bswap32
#define cpu_to_le32     __builtin_bswap32
#define be32_to_cpu
#define cpu_to_be32
#endif
#endif

#endif

#endif

#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//#include "miniz_export.h"
#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#ifdef _WIN32
#ifdef MINIZ_BUILD_SHARED
#ifdef MINIZ_EXPORTS
#define MINIZ_EXPORT __declspec(dllexport)
#else
#define MINIZ_EXPORT __declspec(dllimport)
#endif
#else
#define MINIZ_EXPORT
#endif
#else
#define MINIZ_EXPORT
#endif

#endif /* MINIZ_EXPORT_H */

/* ------------------- Types and macros */
typedef unsigned char mz_uint8;
typedef int16_t mz_int16;
typedef uint16_t mz_uint16;
typedef uint32_t mz_uint32;
typedef uint32_t mz_uint;
typedef int64_t mz_int64;
typedef uint64_t mz_uint64;
typedef int mz_bool;

#define MZ_FALSE (0)
#define MZ_TRUE (1)

/* Works around MSVC's spammy "warning C4127: conditional expression is constant" message. */
#ifdef _MSC_VER
#define MZ_MACRO_END while (0, 0)
#else
#define MZ_MACRO_END while (0)
#endif

#ifdef MINIZ_NO_STDIO
#define MZ_FILE void *
#else
#include <stdio.h>
#define MZ_FILE FILE
#endif /* #ifdef MINIZ_NO_STDIO */

#ifdef MINIZ_NO_TIME
typedef struct mz_dummy_time_t_tag
{
    mz_uint32 m_dummy1;
    mz_uint32 m_dummy2;
} mz_dummy_time_t;
#define MZ_TIME_T mz_dummy_time_t
#else
#define MZ_TIME_T time_t
#endif

#define MZ_ASSERT(x) assert(x)

#ifdef MINIZ_NO_MALLOC
#define MZ_MALLOC(x) NULL
#define MZ_FREE(x) (void)x, ((void)0)
#define MZ_REALLOC(p, x) NULL
#else
#define MZ_MALLOC(x) malloc(x)
#define MZ_FREE(x) free(x)
#define MZ_REALLOC(p, x) realloc(p, x)
#endif

#define MZ_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MZ_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MZ_CLEAR_OBJ(obj) memset(&(obj), 0, sizeof(obj))
#define MZ_CLEAR_ARR(obj) memset((obj), 0, sizeof(obj))
#define MZ_CLEAR_PTR(obj) memset((obj), 0, sizeof(*obj))

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
#define MZ_READ_LE16(p) *((const mz_uint16 *)(p))
#define MZ_READ_LE32(p) *((const mz_uint32 *)(p))
#else
#define MZ_READ_LE16(p) ((mz_uint32)(((const mz_uint8 *)(p))[0]) | ((mz_uint32)(((const mz_uint8 *)(p))[1]) << 8U))
#define MZ_READ_LE32(p) ((mz_uint32)(((const mz_uint8 *)(p))[0]) | ((mz_uint32)(((const mz_uint8 *)(p))[1]) << 8U) | ((mz_uint32)(((const mz_uint8 *)(p))[2]) << 16U) | ((mz_uint32)(((const mz_uint8 *)(p))[3]) << 24U))
#endif

#define MZ_READ_LE64(p) (((mz_uint64)MZ_READ_LE32(p)) | (((mz_uint64)MZ_READ_LE32((const mz_uint8 *)(p) + sizeof(mz_uint32))) << 32U))

#ifdef _MSC_VER
#define MZ_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define MZ_FORCEINLINE __inline__ __attribute__((__always_inline__))
#else
#define MZ_FORCEINLINE inline
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    extern MINIZ_EXPORT void *miniz_def_alloc_func(void *opaque, size_t items, size_t size);
    extern MINIZ_EXPORT void miniz_def_free_func(void *opaque, void *address);
    extern MINIZ_EXPORT void *miniz_def_realloc_func(void *opaque, void *address, size_t items, size_t size);

#define MZ_UINT16_MAX (0xFFFFU)
#define MZ_UINT32_MAX (0xFFFFFFFFU)

#ifdef __cplusplus
}
#endif

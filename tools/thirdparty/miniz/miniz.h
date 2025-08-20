/* miniz.c 3.1.0 - public domain deflate/inflate, zlib-subset, ZIP reading/writing/appending, PNG writing
   See "unlicense" statement at the end of this file.
   Rich Geldreich <richgel99@gmail.com>, last updated Oct. 13, 2013
   Implements RFC 1950: http://www.ietf.org/rfc/rfc1950.txt and RFC 1951: http://www.ietf.org/rfc/rfc1951.txt

   Most API's defined in miniz.c are optional. For example, to disable the archive related functions just define
   MINIZ_NO_ARCHIVE_APIS, or to get rid of all stdio usage define MINIZ_NO_STDIO (see the list below for more macros).

   * Low-level Deflate/Inflate implementation notes:

     Compression: Use the "tdefl" API's. The compressor supports raw, static, and dynamic blocks, lazy or
     greedy parsing, match length filtering, RLE-only, and Huffman-only streams. It performs and compresses
     approximately as well as zlib.

     Decompression: Use the "tinfl" API's. The entire decompressor is implemented as a single function
     coroutine: see tinfl_decompress(). It supports decompression into a 32KB (or larger power of 2) wrapping buffer, or into a memory
     block large enough to hold the entire file.

     The low-level tdefl/tinfl API's do not make any use of dynamic memory allocation.

   * zlib-style API notes:

     miniz.c implements a fairly large subset of zlib. There's enough functionality present for it to be a drop-in
     zlib replacement in many apps:
        The z_stream struct, optional memory allocation callbacks
        deflateInit/deflateInit2/deflate/deflateReset/deflateEnd/deflateBound
        inflateInit/inflateInit2/inflate/inflateReset/inflateEnd
        compress, compress2, compressBound, uncompress
        CRC-32, Adler-32 - Using modern, minimal code size, CPU cache friendly routines.
        Supports raw deflate streams or standard zlib streams with adler-32 checking.

     Limitations:
      The callback API's are not implemented yet. No support for gzip headers or zlib static dictionaries.
      I've tried to closely emulate zlib's various flavors of stream flushing and return status codes, but
      there are no guarantees that miniz.c pulls this off perfectly.

   * PNG writing: See the tdefl_write_image_to_png_file_in_memory() function, originally written by
     Alex Evans. Supports 1-4 bytes/pixel images.

   * ZIP archive API notes:

     The ZIP archive API's where designed with simplicity and efficiency in mind, with just enough abstraction to
     get the job done with minimal fuss. There are simple API's to retrieve file information, read files from
     existing archives, create new archives, append new files to existing archives, or clone archive data from
     one archive to another. It supports archives located in memory or the heap, on disk (using stdio.h),
     or you can specify custom file read/write callbacks.

     - Archive reading: Just call this function to read a single file from a disk archive:

      void *mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name,
        size_t *pSize, mz_uint zip_flags);

     For more complex cases, use the "mz_zip_reader" functions. Upon opening an archive, the entire central
     directory is located and read as-is into memory, and subsequent file access only occurs when reading individual files.

     - Archives file scanning: The simple way is to use this function to scan a loaded archive for a specific file:

     int mz_zip_reader_locate_file(mz_zip_archive *pZip, const char *pName, const char *pComment, mz_uint flags);

     The locate operation can optionally check file comments too, which (as one example) can be used to identify
     multiple versions of the same file in an archive. This function uses a simple linear search through the central
     directory, so it's not very fast.

     Alternately, you can iterate through all the files in an archive (using mz_zip_reader_get_num_files()) and
     retrieve detailed info on each file by calling mz_zip_reader_file_stat().

     - Archive creation: Use the "mz_zip_writer" functions. The ZIP writer immediately writes compressed file data
     to disk and builds an exact image of the central directory in memory. The central directory image is written
     all at once at the end of the archive file when the archive is finalized.

     The archive writer can optionally align each file's local header and file data to any power of 2 alignment,
     which can be useful when the archive will be read from optical media. Also, the writer supports placing
     arbitrary data blobs at the very beginning of ZIP archives. Archives written using either feature are still
     readable by any ZIP tool.

     - Archive appending: The simple way to add a single file to an archive is to call this function:

      mz_bool mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name,
        const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags);

     The archive will be created if it doesn't already exist, otherwise it'll be appended to.
     Note the appending is done in-place and is not an atomic operation, so if something goes wrong
     during the operation it's possible the archive could be left without a central directory (although the local
     file headers and file data will be fine, so the archive will be recoverable).

     For more complex archive modification scenarios:
     1. The safest way is to use a mz_zip_reader to read the existing archive, cloning only those bits you want to
     preserve into a new archive using using the mz_zip_writer_add_from_zip_reader() function (which compiles the
     compressed file data as-is). When you're done, delete the old archive and rename the newly written archive, and
     you're done. This is safe but requires a bunch of temporary disk space or heap memory.

     2. Or, you can convert an mz_zip_reader in-place to an mz_zip_writer using mz_zip_writer_init_from_reader(),
     append new files as needed, then finalize the archive which will write an updated central directory to the
     original archive. (This is basically what mz_zip_add_mem_to_archive_file_in_place() does.) There's a
     possibility that the archive's central directory could be lost with this method if anything goes wrong, though.

     - ZIP archive support limitations:
     No spanning support. Extraction functions can only handle unencrypted, stored or deflated files.
     Requires streams capable of seeking.

   * This is a header file library, like stb_image.c. To get only a header file, either cut and paste the
     below header, or create miniz.h, #define MINIZ_HEADER_FILE_ONLY, and then include miniz.c from it.

   * Important: For best perf. be sure to customize the below macros for your target platform:
     #define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
     #define MINIZ_LITTLE_ENDIAN 1
     #define MINIZ_HAS_64BIT_REGISTERS 1

   * On platforms using glibc, Be sure to "#define _LARGEFILE64_SOURCE 1" before including miniz.c to ensure miniz
     uses the 64-bit variants: fopen64(), stat64(), etc. Otherwise you won't be able to process large files
     (i.e. 32-bit stat() fails for me on files > 0x7FFFFFFF bytes).
*/
#pragma once

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

/* Defines to completely disable specific portions of miniz.c:
   If all macros here are defined the only functionality remaining will be CRC-32 and adler-32. */

/* Define MINIZ_NO_STDIO to disable all usage and any functions which rely on stdio for file I/O. */
/*#define MINIZ_NO_STDIO */

/* If MINIZ_NO_TIME is specified then the ZIP archive functions will not be able to get the current time, or */
/* get/set file times, and the C run-time funcs that get/set times won't be called. */
/* The current downside is the times written to your archives will be from 1979. */
/*#define MINIZ_NO_TIME */

/* Define MINIZ_NO_DEFLATE_APIS to disable all compression API's. */
/*#define MINIZ_NO_DEFLATE_APIS */

/* Define MINIZ_NO_INFLATE_APIS to disable all decompression API's. */
/*#define MINIZ_NO_INFLATE_APIS */

/* Define MINIZ_NO_ARCHIVE_APIS to disable all ZIP archive API's. */
/*#define MINIZ_NO_ARCHIVE_APIS */

/* Define MINIZ_NO_ARCHIVE_WRITING_APIS to disable all writing related ZIP archive API's. */
/*#define MINIZ_NO_ARCHIVE_WRITING_APIS */

/* Define MINIZ_NO_ZLIB_APIS to remove all ZLIB-style compression/decompression API's. */
/*#define MINIZ_NO_ZLIB_APIS */

/* Define MINIZ_NO_ZLIB_COMPATIBLE_NAME to disable zlib names, to prevent conflicts against stock zlib. */
/*#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES */

/* Define MINIZ_NO_MALLOC to disable all calls to malloc, free, and realloc.
   Note if MINIZ_NO_MALLOC is defined then the user must always provide custom user alloc/free/realloc
   callbacks to the zlib and archive API's, and a few stand-alone helper API's which don't provide custom user
   functions (such as tdefl_compress_mem_to_heap() and tinfl_decompress_mem_to_heap()) won't work. */
/*#define MINIZ_NO_MALLOC */

#ifdef MINIZ_NO_INFLATE_APIS
#define MINIZ_NO_ARCHIVE_APIS
#endif

#ifdef MINIZ_NO_DEFLATE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#endif

#if defined(__TINYC__) && (defined(__linux) || defined(__linux__))
/* TODO: Work around "error: include file 'sys\utime.h' when compiling with tcc on Linux */
#define MINIZ_NO_TIME
#endif

#include <stddef.h>

#if !defined(MINIZ_NO_TIME) && !defined(MINIZ_NO_ARCHIVE_APIS)
#include <time.h>
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i486) || defined(i386) || defined(__ia64__) || defined(__x86_64__)
/* MINIZ_X86_OR_X64_CPU is only used to help set the below macros. */
#define MINIZ_X86_OR_X64_CPU 1
#else
#define MINIZ_X86_OR_X64_CPU 0
#endif

/* Set MINIZ_LITTLE_ENDIAN only if not set */
#if !defined(MINIZ_LITTLE_ENDIAN)
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
/* Set MINIZ_LITTLE_ENDIAN to 1 if the processor is little endian. */
#define MINIZ_LITTLE_ENDIAN 1
#else
#define MINIZ_LITTLE_ENDIAN 0
#endif

#else

#if MINIZ_X86_OR_X64_CPU
#define MINIZ_LITTLE_ENDIAN 1
#else
#define MINIZ_LITTLE_ENDIAN 0
#endif

#endif
#endif

/* Using unaligned loads and stores causes errors when using UBSan */
#if defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer)
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#endif
#endif

/* Set MINIZ_USE_UNALIGNED_LOADS_AND_STORES only if not set */
#if !defined(MINIZ_USE_UNALIGNED_LOADS_AND_STORES)
#if MINIZ_X86_OR_X64_CPU
/* Set MINIZ_USE_UNALIGNED_LOADS_AND_STORES to 1 on CPU's that permit efficient integer loads and stores from unaligned addresses. */
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#define MINIZ_UNALIGNED_USE_MEMCPY
#else
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#endif
#endif

#if defined(_M_X64) || defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__) || defined(__ia64__) || defined(__x86_64__)
/* Set MINIZ_HAS_64BIT_REGISTERS to 1 if operations on 64-bit integers are reasonably fast (and don't involve compiler generated calls to helper functions). */
#define MINIZ_HAS_64BIT_REGISTERS 1
#else
#define MINIZ_HAS_64BIT_REGISTERS 0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------- zlib-style API Definitions. */

    /* For more compatibility with zlib, miniz.c uses unsigned long for some parameters/struct members. Beware: mz_ulong can be either 32 or 64-bits! */
    typedef unsigned long mz_ulong;

    /* mz_free() internally uses the MZ_FREE() macro (which by default calls free() unless you've modified the MZ_MALLOC macro) to release a block allocated from the heap. */
    MINIZ_EXPORT void mz_free(void *p);

#define MZ_ADLER32_INIT (1)
    /* mz_adler32() returns the initial adler-32 value to use when called with ptr==NULL. */
    MINIZ_EXPORT mz_ulong mz_adler32(mz_ulong adler, const unsigned char *ptr, size_t buf_len);

#define MZ_CRC32_INIT (0)
    /* mz_crc32() returns the initial CRC-32 value to use when called with ptr==NULL. */
    MINIZ_EXPORT mz_ulong mz_crc32(mz_ulong crc, const unsigned char *ptr, size_t buf_len);

    /* Compression strategies. */
    enum
    {
        MZ_DEFAULT_STRATEGY = 0,
        MZ_FILTERED = 1,
        MZ_HUFFMAN_ONLY = 2,
        MZ_RLE = 3,
        MZ_FIXED = 4
    };

/* Method */
#define MZ_DEFLATED 8

    /* Heap allocation callbacks.
    Note that mz_alloc_func parameter types purposely differ from zlib's: items/size is size_t, not unsigned long. */
    typedef void *(*mz_alloc_func)(void *opaque, size_t items, size_t size);
    typedef void (*mz_free_func)(void *opaque, void *address);
    typedef void *(*mz_realloc_func)(void *opaque, void *address, size_t items, size_t size);

    /* Compression levels: 0-9 are the standard zlib-style levels, 10 is best possible compression (not zlib compatible, and may be very slow), MZ_DEFAULT_COMPRESSION=MZ_DEFAULT_LEVEL. */
    enum
    {
        MZ_NO_COMPRESSION = 0,
        MZ_BEST_SPEED = 1,
        MZ_BEST_COMPRESSION = 9,
        MZ_UBER_COMPRESSION = 10,
        MZ_DEFAULT_LEVEL = 6,
        MZ_DEFAULT_COMPRESSION = -1
    };

#define MZ_VERSION "11.3.0"
#define MZ_VERNUM 0xB300
#define MZ_VER_MAJOR 11
#define MZ_VER_MINOR 3
#define MZ_VER_REVISION 0
#define MZ_VER_SUBREVISION 0

#ifndef MINIZ_NO_ZLIB_APIS

    /* Flush values. For typical usage you only need MZ_NO_FLUSH and MZ_FINISH. The other values are for advanced use (refer to the zlib docs). */
    enum
    {
        MZ_NO_FLUSH = 0,
        MZ_PARTIAL_FLUSH = 1,
        MZ_SYNC_FLUSH = 2,
        MZ_FULL_FLUSH = 3,
        MZ_FINISH = 4,
        MZ_BLOCK = 5
    };

    /* Return status codes. MZ_PARAM_ERROR is non-standard. */
    enum
    {
        MZ_OK = 0,
        MZ_STREAM_END = 1,
        MZ_NEED_DICT = 2,
        MZ_ERRNO = -1,
        MZ_STREAM_ERROR = -2,
        MZ_DATA_ERROR = -3,
        MZ_MEM_ERROR = -4,
        MZ_BUF_ERROR = -5,
        MZ_VERSION_ERROR = -6,
        MZ_PARAM_ERROR = -10000
    };

/* Window bits */
#define MZ_DEFAULT_WINDOW_BITS 15

    struct mz_internal_state;

    /* Compression/decompression stream struct. */
    typedef struct mz_stream_s
    {
        const unsigned char *next_in; /* pointer to next byte to read */
        unsigned int avail_in;        /* number of bytes available at next_in */
        mz_ulong total_in;            /* total number of bytes consumed so far */

        unsigned char *next_out; /* pointer to next byte to write */
        unsigned int avail_out;  /* number of bytes that can be written to next_out */
        mz_ulong total_out;      /* total number of bytes produced so far */

        char *msg;                       /* error msg (unused) */
        struct mz_internal_state *state; /* internal state, allocated by zalloc/zfree */

        mz_alloc_func zalloc; /* optional heap allocation function (defaults to malloc) */
        mz_free_func zfree;   /* optional heap free function (defaults to free) */
        void *opaque;         /* heap alloc function user pointer */

        int data_type;     /* data_type (unused) */
        mz_ulong adler;    /* adler32 of the source or uncompressed data */
        mz_ulong reserved; /* not used */
    } mz_stream;

    typedef mz_stream *mz_streamp;

    /* Returns the version string of miniz.c. */
    MINIZ_EXPORT const char *mz_version(void);

#ifndef MINIZ_NO_DEFLATE_APIS

    /* mz_deflateInit() initializes a compressor with default options: */
    /* Parameters: */
    /*  pStream must point to an initialized mz_stream struct. */
    /*  level must be between [MZ_NO_COMPRESSION, MZ_BEST_COMPRESSION]. */
    /*  level 1 enables a specially optimized compression function that's been optimized purely for performance, not ratio. */
    /*  (This special func. is currently only enabled when MINIZ_USE_UNALIGNED_LOADS_AND_STORES and MINIZ_LITTLE_ENDIAN are defined.) */
    /* Return values: */
    /*  MZ_OK on success. */
    /*  MZ_STREAM_ERROR if the stream is bogus. */
    /*  MZ_PARAM_ERROR if the input parameters are bogus. */
    /*  MZ_MEM_ERROR on out of memory. */
    MINIZ_EXPORT int mz_deflateInit(mz_streamp pStream, int level);

    /* mz_deflateInit2() is like mz_deflate(), except with more control: */
    /* Additional parameters: */
    /*   method must be MZ_DEFLATED */
    /*   window_bits must be MZ_DEFAULT_WINDOW_BITS (to wrap the deflate stream with zlib header/adler-32 footer) or -MZ_DEFAULT_WINDOW_BITS (raw deflate/no header or footer) */
    /*   mem_level must be between [1, 9] (it's checked but ignored by miniz.c) */
    MINIZ_EXPORT int mz_deflateInit2(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy);

    /* Quickly resets a compressor without having to reallocate anything. Same as calling mz_deflateEnd() followed by mz_deflateInit()/mz_deflateInit2(). */
    MINIZ_EXPORT int mz_deflateReset(mz_streamp pStream);

    /* mz_deflate() compresses the input to output, consuming as much of the input and producing as much output as possible. */
    /* Parameters: */
    /*   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members. */
    /*   flush may be MZ_NO_FLUSH, MZ_PARTIAL_FLUSH/MZ_SYNC_FLUSH, MZ_FULL_FLUSH, or MZ_FINISH. */
    /* Return values: */
    /*   MZ_OK on success (when flushing, or if more input is needed but not available, and/or there's more output to be written but the output buffer is full). */
    /*   MZ_STREAM_END if all input has been consumed and all output bytes have been written. Don't call mz_deflate() on the stream anymore. */
    /*   MZ_STREAM_ERROR if the stream is bogus. */
    /*   MZ_PARAM_ERROR if one of the parameters is invalid. */
    /*   MZ_BUF_ERROR if no forward progress is possible because the input and/or output buffers are empty. (Fill up the input buffer or free up some output space and try again.) */
    MINIZ_EXPORT int mz_deflate(mz_streamp pStream, int flush);

    /* mz_deflateEnd() deinitializes a compressor: */
    /* Return values: */
    /*  MZ_OK on success. */
    /*  MZ_STREAM_ERROR if the stream is bogus. */
    MINIZ_EXPORT int mz_deflateEnd(mz_streamp pStream);

    /* mz_deflateBound() returns a (very) conservative upper bound on the amount of data that could be generated by deflate(), assuming flush is set to only MZ_NO_FLUSH or MZ_FINISH. */
    MINIZ_EXPORT mz_ulong mz_deflateBound(mz_streamp pStream, mz_ulong source_len);

    /* Single-call compression functions mz_compress() and mz_compress2(): */
    /* Returns MZ_OK on success, or one of the error codes from mz_deflate() on failure. */
    MINIZ_EXPORT int mz_compress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
    MINIZ_EXPORT int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);

    /* mz_compressBound() returns a (very) conservative upper bound on the amount of data that could be generated by calling mz_compress(). */
    MINIZ_EXPORT mz_ulong mz_compressBound(mz_ulong source_len);

#endif /*#ifndef MINIZ_NO_DEFLATE_APIS*/

#ifndef MINIZ_NO_INFLATE_APIS

    /* Initializes a decompressor. */
    MINIZ_EXPORT int mz_inflateInit(mz_streamp pStream);

    /* mz_inflateInit2() is like mz_inflateInit() with an additional option that controls the window size and whether or not the stream has been wrapped with a zlib header/footer: */
    /* window_bits must be MZ_DEFAULT_WINDOW_BITS (to parse zlib header/footer) or -MZ_DEFAULT_WINDOW_BITS (raw deflate). */
    MINIZ_EXPORT int mz_inflateInit2(mz_streamp pStream, int window_bits);

    /* Quickly resets a compressor without having to reallocate anything. Same as calling mz_inflateEnd() followed by mz_inflateInit()/mz_inflateInit2(). */
    MINIZ_EXPORT int mz_inflateReset(mz_streamp pStream);

    /* Decompresses the input stream to the output, consuming only as much of the input as needed, and writing as much to the output as possible. */
    /* Parameters: */
    /*   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members. */
    /*   flush may be MZ_NO_FLUSH, MZ_SYNC_FLUSH, or MZ_FINISH. */
    /*   On the first call, if flush is MZ_FINISH it's assumed the input and output buffers are both sized large enough to decompress the entire stream in a single call (this is slightly faster). */
    /*   MZ_FINISH implies that there are no more source bytes available beside what's already in the input buffer, and that the output buffer is large enough to hold the rest of the decompressed data. */
    /* Return values: */
    /*   MZ_OK on success. Either more input is needed but not available, and/or there's more output to be written but the output buffer is full. */
    /*   MZ_STREAM_END if all needed input has been consumed and all output bytes have been written. For zlib streams, the adler-32 of the decompressed data has also been verified. */
    /*   MZ_STREAM_ERROR if the stream is bogus. */
    /*   MZ_DATA_ERROR if the deflate stream is invalid. */
    /*   MZ_PARAM_ERROR if one of the parameters is invalid. */
    /*   MZ_BUF_ERROR if no forward progress is possible because the input buffer is empty but the inflater needs more input to continue, or if the output buffer is not large enough. Call mz_inflate() again */
    /*   with more input data, or with more room in the output buffer (except when using single call decompression, described above). */
    MINIZ_EXPORT int mz_inflate(mz_streamp pStream, int flush);

    /* Deinitializes a decompressor. */
    MINIZ_EXPORT int mz_inflateEnd(mz_streamp pStream);

    /* Single-call decompression. */
    /* Returns MZ_OK on success, or one of the error codes from mz_inflate() on failure. */
    MINIZ_EXPORT int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
    MINIZ_EXPORT int mz_uncompress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong *pSource_len);
#endif /*#ifndef MINIZ_NO_INFLATE_APIS*/

    /* Returns a string description of the specified error code, or NULL if the error code is invalid. */
    MINIZ_EXPORT const char *mz_error(int err);

/* Redefine zlib-compatible names to miniz equivalents, so miniz.c can be used as a drop-in replacement for the subset of zlib that miniz.c supports. */
/* Define MINIZ_NO_ZLIB_COMPATIBLE_NAMES to disable zlib-compatibility if you use zlib in the same project. */
#ifndef MINIZ_NO_ZLIB_COMPATIBLE_NAMES
    typedef unsigned char Byte;
    typedef unsigned int uInt;
    typedef mz_ulong uLong;
    typedef Byte Bytef;
    typedef uInt uIntf;
    typedef char charf;
    typedef int intf;
    typedef void *voidpf;
    typedef uLong uLongf;
    typedef void *voidp;
    typedef void *const voidpc;
#define Z_NULL 0
#define Z_NO_FLUSH MZ_NO_FLUSH
#define Z_PARTIAL_FLUSH MZ_PARTIAL_FLUSH
#define Z_SYNC_FLUSH MZ_SYNC_FLUSH
#define Z_FULL_FLUSH MZ_FULL_FLUSH
#define Z_FINISH MZ_FINISH
#define Z_BLOCK MZ_BLOCK
#define Z_OK MZ_OK
#define Z_STREAM_END MZ_STREAM_END
#define Z_NEED_DICT MZ_NEED_DICT
#define Z_ERRNO MZ_ERRNO
#define Z_STREAM_ERROR MZ_STREAM_ERROR
#define Z_DATA_ERROR MZ_DATA_ERROR
#define Z_MEM_ERROR MZ_MEM_ERROR
#define Z_BUF_ERROR MZ_BUF_ERROR
#define Z_VERSION_ERROR MZ_VERSION_ERROR
#define Z_PARAM_ERROR MZ_PARAM_ERROR
#define Z_NO_COMPRESSION MZ_NO_COMPRESSION
#define Z_BEST_SPEED MZ_BEST_SPEED
#define Z_BEST_COMPRESSION MZ_BEST_COMPRESSION
#define Z_DEFAULT_COMPRESSION MZ_DEFAULT_COMPRESSION
#define Z_DEFAULT_STRATEGY MZ_DEFAULT_STRATEGY
#define Z_FILTERED MZ_FILTERED
#define Z_HUFFMAN_ONLY MZ_HUFFMAN_ONLY
#define Z_RLE MZ_RLE
#define Z_FIXED MZ_FIXED
#define Z_DEFLATED MZ_DEFLATED
#define Z_DEFAULT_WINDOW_BITS MZ_DEFAULT_WINDOW_BITS
    /* See mz_alloc_func */
    typedef void *(*alloc_func)(void *opaque, size_t items, size_t size);
    /* See mz_free_func */
    typedef void (*free_func)(void *opaque, void *address);

#define internal_state mz_internal_state
#define z_stream mz_stream

#ifndef MINIZ_NO_DEFLATE_APIS
    /* Compatiblity with zlib API. See called functions for documentation */
    static int deflateInit(mz_streamp pStream, int level)
    {
        return mz_deflateInit(pStream, level);
    }
    static int deflateInit2(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy)
    {
        return mz_deflateInit2(pStream, level, method, window_bits, mem_level, strategy);
    }
    static int deflateReset(mz_streamp pStream)
    {
        return mz_deflateReset(pStream);
    }
    static int deflate(mz_streamp pStream, int flush)
    {
        return mz_deflate(pStream, flush);
    }
    static int deflateEnd(mz_streamp pStream)
    {
        return mz_deflateEnd(pStream);
    }
    static mz_ulong deflateBound(mz_streamp pStream, mz_ulong source_len)
    {
        return mz_deflateBound(pStream, source_len);
    }
    static int compress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len)
    {
        return mz_compress(pDest, pDest_len, pSource, source_len);
    }
    static int compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level)
    {
        return mz_compress2(pDest, pDest_len, pSource, source_len, level);
    }
    static mz_ulong compressBound(mz_ulong source_len)
    {
        return mz_compressBound(source_len);
    }
#endif /*#ifndef MINIZ_NO_DEFLATE_APIS*/

#ifndef MINIZ_NO_INFLATE_APIS
    /* Compatiblity with zlib API. See called functions for documentation */
    static int inflateInit(mz_streamp pStream)
    {
        return mz_inflateInit(pStream);
    }

    static int inflateInit2(mz_streamp pStream, int window_bits)
    {
        return mz_inflateInit2(pStream, window_bits);
    }

    static int inflateReset(mz_streamp pStream)
    {
        return mz_inflateReset(pStream);
    }

    static int inflate(mz_streamp pStream, int flush)
    {
        return mz_inflate(pStream, flush);
    }

    static int inflateEnd(mz_streamp pStream)
    {
        return mz_inflateEnd(pStream);
    }

    static int uncompress(unsigned char* pDest, mz_ulong* pDest_len, const unsigned char* pSource, mz_ulong source_len)
    {
        return mz_uncompress(pDest, pDest_len, pSource, source_len);
    }

    static int uncompress2(unsigned char* pDest, mz_ulong* pDest_len, const unsigned char* pSource, mz_ulong* pSource_len)
    {
        return mz_uncompress2(pDest, pDest_len, pSource, pSource_len);
    }
#endif /*#ifndef MINIZ_NO_INFLATE_APIS*/

    static mz_ulong crc32(mz_ulong crc, const unsigned char *ptr, size_t buf_len)
    {
        return mz_crc32(crc, ptr, buf_len);
    }

    static mz_ulong adler32(mz_ulong adler, const unsigned char *ptr, size_t buf_len)
    {
        return mz_adler32(adler, ptr, buf_len);
    }
    
#define MAX_WBITS 15
#define MAX_MEM_LEVEL 9

    static const char* zError(int err)
    {
        return mz_error(err);
    }
#define ZLIB_VERSION MZ_VERSION
#define ZLIB_VERNUM MZ_VERNUM
#define ZLIB_VER_MAJOR MZ_VER_MAJOR
#define ZLIB_VER_MINOR MZ_VER_MINOR
#define ZLIB_VER_REVISION MZ_VER_REVISION
#define ZLIB_VER_SUBREVISION MZ_VER_SUBREVISION

#define zlibVersion mz_version
#define zlib_version mz_version()
#endif /* #ifndef MINIZ_NO_ZLIB_COMPATIBLE_NAMES */

#endif /* MINIZ_NO_ZLIB_APIS */

#ifdef __cplusplus
}
#endif

#include "miniz_common.h"
#include "miniz_tdef.h"
#include "miniz_tinfl.h"
#include "miniz_zip.h"

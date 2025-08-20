
#pragma once
#include "miniz_common.h"

/* ------------------- ZIP archive reading/writing */

#ifndef MINIZ_NO_ARCHIVE_APIS

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        /* Note: These enums can be reduced as needed to save memory or stack space - they are pretty conservative. */
        MZ_ZIP_MAX_IO_BUF_SIZE = 64 * 1024,
        MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE = 512,
        MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE = 512
    };

    typedef struct
    {
        /* Central directory file index. */
        mz_uint32 m_file_index;

        /* Byte offset of this entry in the archive's central directory. Note we currently only support up to UINT_MAX or less bytes in the central dir. */
        mz_uint64 m_central_dir_ofs;

        /* These fields are copied directly from the zip's central dir. */
        mz_uint16 m_version_made_by;
        mz_uint16 m_version_needed;
        mz_uint16 m_bit_flag;
        mz_uint16 m_method;

        /* CRC-32 of uncompressed data. */
        mz_uint32 m_crc32;

        /* File's compressed size. */
        mz_uint64 m_comp_size;

        /* File's uncompressed size. Note, I've seen some old archives where directory entries had 512 bytes for their uncompressed sizes, but when you try to unpack them you actually get 0 bytes. */
        mz_uint64 m_uncomp_size;

        /* Zip internal and external file attributes. */
        mz_uint16 m_internal_attr;
        mz_uint32 m_external_attr;

        /* Entry's local header file offset in bytes. */
        mz_uint64 m_local_header_ofs;

        /* Size of comment in bytes. */
        mz_uint32 m_comment_size;

        /* MZ_TRUE if the entry appears to be a directory. */
        mz_bool m_is_directory;

        /* MZ_TRUE if the entry uses encryption/strong encryption (which miniz_zip doesn't support) */
        mz_bool m_is_encrypted;

        /* MZ_TRUE if the file is not encrypted, a patch file, and if it uses a compression method we support. */
        mz_bool m_is_supported;

        /* Filename. If string ends in '/' it's a subdirectory entry. */
        /* Guaranteed to be zero terminated, may be truncated to fit. */
        char m_filename[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];

        /* Comment field. */
        /* Guaranteed to be zero terminated, may be truncated to fit. */
        char m_comment[MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE];

#ifdef MINIZ_NO_TIME
        MZ_TIME_T m_padding;
#else
    MZ_TIME_T m_time;
#endif
    } mz_zip_archive_file_stat;

    typedef size_t (*mz_file_read_func)(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n);
    typedef size_t (*mz_file_write_func)(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n);
    typedef mz_bool (*mz_file_needs_keepalive)(void *pOpaque);

    struct mz_zip_internal_state_tag;
    typedef struct mz_zip_internal_state_tag mz_zip_internal_state;

    typedef enum
    {
        MZ_ZIP_MODE_INVALID = 0,
        MZ_ZIP_MODE_READING = 1,
        MZ_ZIP_MODE_WRITING = 2,
        MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED = 3
    } mz_zip_mode;

    typedef enum
    {
        MZ_ZIP_FLAG_CASE_SENSITIVE = 0x0100,
        MZ_ZIP_FLAG_IGNORE_PATH = 0x0200,
        MZ_ZIP_FLAG_COMPRESSED_DATA = 0x0400,
        MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY = 0x0800,
        MZ_ZIP_FLAG_VALIDATE_LOCATE_FILE_FLAG = 0x1000, /* if enabled, mz_zip_reader_locate_file() will be called on each file as its validated to ensure the func finds the file in the central dir (intended for testing) */
        MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY = 0x2000,     /* validate the local headers, but don't decompress the entire file and check the crc32 */
        MZ_ZIP_FLAG_WRITE_ZIP64 = 0x4000,               /* always use the zip64 file format, instead of the original zip file format with automatic switch to zip64. Use as flags parameter with mz_zip_writer_init*_v2 */
        MZ_ZIP_FLAG_WRITE_ALLOW_READING = 0x8000,
        MZ_ZIP_FLAG_ASCII_FILENAME = 0x10000,
        /*After adding a compressed file, seek back
        to local file header and set the correct sizes*/
        MZ_ZIP_FLAG_WRITE_HEADER_SET_SIZE = 0x20000,
        MZ_ZIP_FLAG_READ_ALLOW_WRITING = 0x40000
    } mz_zip_flags;

    typedef enum
    {
        MZ_ZIP_TYPE_INVALID = 0,
        MZ_ZIP_TYPE_USER,
        MZ_ZIP_TYPE_MEMORY,
        MZ_ZIP_TYPE_HEAP,
        MZ_ZIP_TYPE_FILE,
        MZ_ZIP_TYPE_CFILE,
        MZ_ZIP_TOTAL_TYPES
    } mz_zip_type;

    /* miniz error codes. Be sure to update mz_zip_get_error_string() if you add or modify this enum. */
    typedef enum
    {
        MZ_ZIP_NO_ERROR = 0,
        MZ_ZIP_UNDEFINED_ERROR,
        MZ_ZIP_TOO_MANY_FILES,
        MZ_ZIP_FILE_TOO_LARGE,
        MZ_ZIP_UNSUPPORTED_METHOD,
        MZ_ZIP_UNSUPPORTED_ENCRYPTION,
        MZ_ZIP_UNSUPPORTED_FEATURE,
        MZ_ZIP_FAILED_FINDING_CENTRAL_DIR,
        MZ_ZIP_NOT_AN_ARCHIVE,
        MZ_ZIP_INVALID_HEADER_OR_CORRUPTED,
        MZ_ZIP_UNSUPPORTED_MULTIDISK,
        MZ_ZIP_DECOMPRESSION_FAILED,
        MZ_ZIP_COMPRESSION_FAILED,
        MZ_ZIP_UNEXPECTED_DECOMPRESSED_SIZE,
        MZ_ZIP_CRC_CHECK_FAILED,
        MZ_ZIP_UNSUPPORTED_CDIR_SIZE,
        MZ_ZIP_ALLOC_FAILED,
        MZ_ZIP_FILE_OPEN_FAILED,
        MZ_ZIP_FILE_CREATE_FAILED,
        MZ_ZIP_FILE_WRITE_FAILED,
        MZ_ZIP_FILE_READ_FAILED,
        MZ_ZIP_FILE_CLOSE_FAILED,
        MZ_ZIP_FILE_SEEK_FAILED,
        MZ_ZIP_FILE_STAT_FAILED,
        MZ_ZIP_INVALID_PARAMETER,
        MZ_ZIP_INVALID_FILENAME,
        MZ_ZIP_BUF_TOO_SMALL,
        MZ_ZIP_INTERNAL_ERROR,
        MZ_ZIP_FILE_NOT_FOUND,
        MZ_ZIP_ARCHIVE_TOO_LARGE,
        MZ_ZIP_VALIDATION_FAILED,
        MZ_ZIP_WRITE_CALLBACK_FAILED,
        MZ_ZIP_TOTAL_ERRORS
    } mz_zip_error;

    typedef struct
    {
        mz_uint64 m_archive_size;
        mz_uint64 m_central_directory_file_ofs;

        /* We only support up to UINT32_MAX files in zip64 mode. */
        mz_uint32 m_total_files;
        mz_zip_mode m_zip_mode;
        mz_zip_type m_zip_type;
        mz_zip_error m_last_error;

        mz_uint64 m_file_offset_alignment;

        mz_alloc_func m_pAlloc;
        mz_free_func m_pFree;
        mz_realloc_func m_pRealloc;
        void *m_pAlloc_opaque;

        mz_file_read_func m_pRead;
        mz_file_write_func m_pWrite;
        mz_file_needs_keepalive m_pNeeds_keepalive;
        void *m_pIO_opaque;

        mz_zip_internal_state *m_pState;

    } mz_zip_archive;

    typedef struct
    {
        mz_zip_archive *pZip;
        mz_uint flags;

        int status;

        mz_uint64 read_buf_size, read_buf_ofs, read_buf_avail, comp_remaining, out_buf_ofs, cur_file_ofs;
        mz_zip_archive_file_stat file_stat;
        void *pRead_buf;
        void *pWrite_buf;

        size_t out_blk_remain;

        tinfl_decompressor inflator;

#ifdef MINIZ_DISABLE_ZIP_READER_CRC32_CHECKS
        mz_uint padding;
#else
    mz_uint file_crc32;
#endif

    } mz_zip_reader_extract_iter_state;

    /* -------- ZIP reading */

    /* Inits a ZIP archive reader. */
    /* These functions read and validate the archive's central directory. */
    MINIZ_EXPORT mz_bool mz_zip_reader_init(mz_zip_archive *pZip, mz_uint64 size, mz_uint flags);

    MINIZ_EXPORT mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint flags);

#ifndef MINIZ_NO_STDIO
    /* Read a archive from a disk file. */
    /* file_start_ofs is the file offset where the archive actually begins, or 0. */
    /* actual_archive_size is the true total size of the archive, which may be smaller than the file's actual size on disk. If zero the entire file is treated as the archive. */
    MINIZ_EXPORT mz_bool mz_zip_reader_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint32 flags);
    MINIZ_EXPORT mz_bool mz_zip_reader_init_file_v2(mz_zip_archive *pZip, const char *pFilename, mz_uint flags, mz_uint64 file_start_ofs, mz_uint64 archive_size);

    /* Read an archive from an already opened FILE, beginning at the current file position. */
    /* The archive is assumed to be archive_size bytes long. If archive_size is 0, then the entire rest of the file is assumed to contain the archive. */
    /* The FILE will NOT be closed when mz_zip_reader_end() is called. */
    MINIZ_EXPORT mz_bool mz_zip_reader_init_cfile(mz_zip_archive *pZip, MZ_FILE *pFile, mz_uint64 archive_size, mz_uint flags);
#endif

    /* Ends archive reading, freeing all allocations, and closing the input archive file if mz_zip_reader_init_file() was used. */
    MINIZ_EXPORT mz_bool mz_zip_reader_end(mz_zip_archive *pZip);

    /* -------- ZIP reading or writing */

    /* Clears a mz_zip_archive struct to all zeros. */
    /* Important: This must be done before passing the struct to any mz_zip functions. */
    MINIZ_EXPORT void mz_zip_zero_struct(mz_zip_archive *pZip);

    MINIZ_EXPORT mz_zip_mode mz_zip_get_mode(mz_zip_archive *pZip);
    MINIZ_EXPORT mz_zip_type mz_zip_get_type(mz_zip_archive *pZip);

    /* Returns the total number of files in the archive. */
    MINIZ_EXPORT mz_uint mz_zip_reader_get_num_files(mz_zip_archive *pZip);

    MINIZ_EXPORT mz_uint64 mz_zip_get_archive_size(mz_zip_archive *pZip);
    MINIZ_EXPORT mz_uint64 mz_zip_get_archive_file_start_offset(mz_zip_archive *pZip);
    MINIZ_EXPORT MZ_FILE *mz_zip_get_cfile(mz_zip_archive *pZip);

    /* Reads n bytes of raw archive data, starting at file offset file_ofs, to pBuf. */
    MINIZ_EXPORT size_t mz_zip_read_archive_data(mz_zip_archive *pZip, mz_uint64 file_ofs, void *pBuf, size_t n);

    /* All mz_zip funcs set the m_last_error field in the mz_zip_archive struct. These functions retrieve/manipulate this field. */
    /* Note that the m_last_error functionality is not thread safe. */
    MINIZ_EXPORT mz_zip_error mz_zip_set_last_error(mz_zip_archive *pZip, mz_zip_error err_num);
    MINIZ_EXPORT mz_zip_error mz_zip_peek_last_error(mz_zip_archive *pZip);
    MINIZ_EXPORT mz_zip_error mz_zip_clear_last_error(mz_zip_archive *pZip);
    MINIZ_EXPORT mz_zip_error mz_zip_get_last_error(mz_zip_archive *pZip);
    MINIZ_EXPORT const char *mz_zip_get_error_string(mz_zip_error mz_err);

    /* MZ_TRUE if the archive file entry is a directory entry. */
    MINIZ_EXPORT mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *pZip, mz_uint file_index);

    /* MZ_TRUE if the file is encrypted/strong encrypted. */
    MINIZ_EXPORT mz_bool mz_zip_reader_is_file_encrypted(mz_zip_archive *pZip, mz_uint file_index);

    /* MZ_TRUE if the compression method is supported, and the file is not encrypted, and the file is not a compressed patch file. */
    MINIZ_EXPORT mz_bool mz_zip_reader_is_file_supported(mz_zip_archive *pZip, mz_uint file_index);

    /* Retrieves the filename of an archive file entry. */
    /* Returns the number of bytes written to pFilename, or if filename_buf_size is 0 this function returns the number of bytes needed to fully store the filename. */
    MINIZ_EXPORT mz_uint mz_zip_reader_get_filename(mz_zip_archive *pZip, mz_uint file_index, char *pFilename, mz_uint filename_buf_size);

    /* Attempts to locates a file in the archive's central directory. */
    /* Valid flags: MZ_ZIP_FLAG_CASE_SENSITIVE, MZ_ZIP_FLAG_IGNORE_PATH */
    /* Returns -1 if the file cannot be found. */
    MINIZ_EXPORT int mz_zip_reader_locate_file(mz_zip_archive *pZip, const char *pName, const char *pComment, mz_uint flags);
    MINIZ_EXPORT mz_bool mz_zip_reader_locate_file_v2(mz_zip_archive *pZip, const char *pName, const char *pComment, mz_uint flags, mz_uint32 *file_index);

    /* Returns detailed information about an archive file entry. */
    MINIZ_EXPORT mz_bool mz_zip_reader_file_stat(mz_zip_archive *pZip, mz_uint file_index, mz_zip_archive_file_stat *pStat);

    /* MZ_TRUE if the file is in zip64 format. */
    /* A file is considered zip64 if it contained a zip64 end of central directory marker, or if it contained any zip64 extended file information fields in the central directory. */
    MINIZ_EXPORT mz_bool mz_zip_is_zip64(mz_zip_archive *pZip);

    /* Returns the total central directory size in bytes. */
    /* The current max supported size is <= MZ_UINT32_MAX. */
    MINIZ_EXPORT size_t mz_zip_get_central_dir_size(mz_zip_archive *pZip);

    /* Extracts a archive file to a memory buffer using no memory allocation. */
    /* There must be at least enough room on the stack to store the inflator's state (~34KB or so). */
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_to_mem_no_alloc(mz_zip_archive *pZip, mz_uint file_index, void *pBuf, size_t buf_size, mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size);
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_file_to_mem_no_alloc(mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size);

    /* Extracts a archive file to a memory buffer. */
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_to_mem(mz_zip_archive *pZip, mz_uint file_index, void *pBuf, size_t buf_size, mz_uint flags);
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_file_to_mem(mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, mz_uint flags);

    /* Extracts a archive file to a dynamically allocated heap buffer. */
    /* The memory will be allocated via the mz_zip_archive's alloc/realloc functions. */
    /* Returns NULL and sets the last error on failure. */
    MINIZ_EXPORT void *mz_zip_reader_extract_to_heap(mz_zip_archive *pZip, mz_uint file_index, size_t *pSize, mz_uint flags);
    MINIZ_EXPORT void *mz_zip_reader_extract_file_to_heap(mz_zip_archive *pZip, const char *pFilename, size_t *pSize, mz_uint flags);

    /* Extracts a archive file using a callback function to output the file's data. */
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_to_callback(mz_zip_archive *pZip, mz_uint file_index, mz_file_write_func pCallback, void *pOpaque, mz_uint flags);
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_file_to_callback(mz_zip_archive *pZip, const char *pFilename, mz_file_write_func pCallback, void *pOpaque, mz_uint flags);

    /* Extract a file iteratively */
    MINIZ_EXPORT mz_zip_reader_extract_iter_state *mz_zip_reader_extract_iter_new(mz_zip_archive *pZip, mz_uint file_index, mz_uint flags);
    MINIZ_EXPORT mz_zip_reader_extract_iter_state *mz_zip_reader_extract_file_iter_new(mz_zip_archive *pZip, const char *pFilename, mz_uint flags);
    MINIZ_EXPORT size_t mz_zip_reader_extract_iter_read(mz_zip_reader_extract_iter_state *pState, void *pvBuf, size_t buf_size);
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_iter_free(mz_zip_reader_extract_iter_state *pState);

#ifndef MINIZ_NO_STDIO
    /* Extracts a archive file to a disk file and sets its last accessed and modified times. */
    /* This function only extracts files, not archive directory records. */
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_to_file(mz_zip_archive *pZip, mz_uint file_index, const char *pDst_filename, mz_uint flags);
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_file_to_file(mz_zip_archive *pZip, const char *pArchive_filename, const char *pDst_filename, mz_uint flags);

    /* Extracts a archive file starting at the current position in the destination FILE stream. */
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_to_cfile(mz_zip_archive *pZip, mz_uint file_index, MZ_FILE *File, mz_uint flags);
    MINIZ_EXPORT mz_bool mz_zip_reader_extract_file_to_cfile(mz_zip_archive *pZip, const char *pArchive_filename, MZ_FILE *pFile, mz_uint flags);
#endif

#if 0
/* TODO */
    typedef void *mz_zip_streaming_extract_state_ptr;
    mz_zip_streaming_extract_state_ptr mz_zip_streaming_extract_begin(mz_zip_archive *pZip, mz_uint file_index, mz_uint flags);
    mz_uint64 mz_zip_streaming_extract_get_size(mz_zip_archive *pZip, mz_zip_streaming_extract_state_ptr pState);
    mz_uint64 mz_zip_streaming_extract_get_cur_ofs(mz_zip_archive *pZip, mz_zip_streaming_extract_state_ptr pState);
    mz_bool mz_zip_streaming_extract_seek(mz_zip_archive *pZip, mz_zip_streaming_extract_state_ptr pState, mz_uint64 new_ofs);
    size_t mz_zip_streaming_extract_read(mz_zip_archive *pZip, mz_zip_streaming_extract_state_ptr pState, void *pBuf, size_t buf_size);
    mz_bool mz_zip_streaming_extract_end(mz_zip_archive *pZip, mz_zip_streaming_extract_state_ptr pState);
#endif

    /* This function compares the archive's local headers, the optional local zip64 extended information block, and the optional descriptor following the compressed data vs. the data in the central directory. */
    /* It also validates that each file can be successfully uncompressed unless the MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY is specified. */
    MINIZ_EXPORT mz_bool mz_zip_validate_file(mz_zip_archive *pZip, mz_uint file_index, mz_uint flags);

    /* Validates an entire archive by calling mz_zip_validate_file() on each file. */
    MINIZ_EXPORT mz_bool mz_zip_validate_archive(mz_zip_archive *pZip, mz_uint flags);

    /* Misc utils/helpers, valid for ZIP reading or writing */
    MINIZ_EXPORT mz_bool mz_zip_validate_mem_archive(const void *pMem, size_t size, mz_uint flags, mz_zip_error *pErr);
#ifndef MINIZ_NO_STDIO
    MINIZ_EXPORT mz_bool mz_zip_validate_file_archive(const char *pFilename, mz_uint flags, mz_zip_error *pErr);
#endif

    /* Universal end function - calls either mz_zip_reader_end() or mz_zip_writer_end(). */
    MINIZ_EXPORT mz_bool mz_zip_end(mz_zip_archive *pZip);

    /* -------- ZIP writing */

#ifndef MINIZ_NO_ARCHIVE_WRITING_APIS

    /* Inits a ZIP archive writer. */
    /*Set pZip->m_pWrite (and pZip->m_pIO_opaque) before calling mz_zip_writer_init or mz_zip_writer_init_v2*/
    /*The output is streamable, i.e. file_ofs in mz_file_write_func always increases only by n*/
    MINIZ_EXPORT mz_bool mz_zip_writer_init(mz_zip_archive *pZip, mz_uint64 existing_size);
    MINIZ_EXPORT mz_bool mz_zip_writer_init_v2(mz_zip_archive *pZip, mz_uint64 existing_size, mz_uint flags);

    MINIZ_EXPORT mz_bool mz_zip_writer_init_heap(mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size);
    MINIZ_EXPORT mz_bool mz_zip_writer_init_heap_v2(mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size, mz_uint flags);

#ifndef MINIZ_NO_STDIO
    MINIZ_EXPORT mz_bool mz_zip_writer_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint64 size_to_reserve_at_beginning);
    MINIZ_EXPORT mz_bool mz_zip_writer_init_file_v2(mz_zip_archive *pZip, const char *pFilename, mz_uint64 size_to_reserve_at_beginning, mz_uint flags);
    MINIZ_EXPORT mz_bool mz_zip_writer_init_cfile(mz_zip_archive *pZip, MZ_FILE *pFile, mz_uint flags);
#endif

    /* Converts a ZIP archive reader object into a writer object, to allow efficient in-place file appends to occur on an existing archive. */
    /* For archives opened using mz_zip_reader_init_file, pFilename must be the archive's filename so it can be reopened for writing. If the file can't be reopened, mz_zip_reader_end() will be called. */
    /* For archives opened using mz_zip_reader_init_mem, the memory block must be growable using the realloc callback (which defaults to realloc unless you've overridden it). */
    /* Finally, for archives opened using mz_zip_reader_init, the mz_zip_archive's user provided m_pWrite function cannot be NULL. */
    /* Note: In-place archive modification is not recommended unless you know what you're doing, because if execution stops or something goes wrong before */
    /* the archive is finalized the file's central directory will be hosed. */
    MINIZ_EXPORT mz_bool mz_zip_writer_init_from_reader(mz_zip_archive *pZip, const char *pFilename);
    MINIZ_EXPORT mz_bool mz_zip_writer_init_from_reader_v2(mz_zip_archive *pZip, const char *pFilename, mz_uint flags);

    /* Adds the contents of a memory buffer to an archive. These functions record the current local time into the archive. */
    /* To add a directory entry, call this method with an archive name ending in a forwardslash with an empty buffer. */
    /* level_and_flags - compression level (0-10, see MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to MZ_DEFAULT_COMPRESSION. */
    MINIZ_EXPORT mz_bool mz_zip_writer_add_mem(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, mz_uint level_and_flags);

    /* Like mz_zip_writer_add_mem(), except you can specify a file comment field, and optionally supply the function with already compressed data. */
    /* uncomp_size/uncomp_crc32 are only used if the MZ_ZIP_FLAG_COMPRESSED_DATA flag is specified. */
    MINIZ_EXPORT mz_bool mz_zip_writer_add_mem_ex(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags,
                                                  mz_uint64 uncomp_size, mz_uint32 uncomp_crc32);

    MINIZ_EXPORT mz_bool mz_zip_writer_add_mem_ex_v2(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags,
                                                     mz_uint64 uncomp_size, mz_uint32 uncomp_crc32, MZ_TIME_T *last_modified, const char *user_extra_data_local, mz_uint user_extra_data_local_len,
                                                     const char *user_extra_data_central, mz_uint user_extra_data_central_len);

    /* Adds the contents of a file to an archive. This function also records the disk file's modified time into the archive. */
    /* File data is supplied via a read callback function. User mz_zip_writer_add_(c)file to add a file directly.*/
    MINIZ_EXPORT mz_bool mz_zip_writer_add_read_buf_callback(mz_zip_archive *pZip, const char *pArchive_name, mz_file_read_func read_callback, void *callback_opaque, mz_uint64 max_size,
                                                             const MZ_TIME_T *pFile_time, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags, const char *user_extra_data_local, mz_uint user_extra_data_local_len,
                                                             const char *user_extra_data_central, mz_uint user_extra_data_central_len);

#ifndef MINIZ_NO_STDIO
    /* Adds the contents of a disk file to an archive. This function also records the disk file's modified time into the archive. */
    /* level_and_flags - compression level (0-10, see MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to MZ_DEFAULT_COMPRESSION. */
    MINIZ_EXPORT mz_bool mz_zip_writer_add_file(mz_zip_archive *pZip, const char *pArchive_name, const char *pSrc_filename, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags);

    /* Like mz_zip_writer_add_file(), except the file data is read from the specified FILE stream. */
    MINIZ_EXPORT mz_bool mz_zip_writer_add_cfile(mz_zip_archive *pZip, const char *pArchive_name, MZ_FILE *pSrc_file, mz_uint64 max_size,
                                                 const MZ_TIME_T *pFile_time, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags, const char *user_extra_data_local, mz_uint user_extra_data_local_len,
                                                 const char *user_extra_data_central, mz_uint user_extra_data_central_len);
#endif

    /* Adds a file to an archive by fully cloning the data from another archive. */
    /* This function fully clones the source file's compressed data (no recompression), along with its full filename, extra data (it may add or modify the zip64 local header extra data field), and the optional descriptor following the compressed data. */
    MINIZ_EXPORT mz_bool mz_zip_writer_add_from_zip_reader(mz_zip_archive *pZip, mz_zip_archive *pSource_zip, mz_uint src_file_index);

    /* Finalizes the archive by writing the central directory records followed by the end of central directory record. */
    /* After an archive is finalized, the only valid call on the mz_zip_archive struct is mz_zip_writer_end(). */
    /* An archive must be manually finalized by calling this function for it to be valid. */
    MINIZ_EXPORT mz_bool mz_zip_writer_finalize_archive(mz_zip_archive *pZip);

    /* Finalizes a heap archive, returning a pointer to the heap block and its size. */
    /* The heap block will be allocated using the mz_zip_archive's alloc/realloc callbacks. */
    MINIZ_EXPORT mz_bool mz_zip_writer_finalize_heap_archive(mz_zip_archive *pZip, void **ppBuf, size_t *pSize);

    /* Ends archive writing, freeing all allocations, and closing the output file if mz_zip_writer_init_file() was used. */
    /* Note for the archive to be valid, it *must* have been finalized before ending (this function will not do it for you). */
    MINIZ_EXPORT mz_bool mz_zip_writer_end(mz_zip_archive *pZip);

    /* -------- Misc. high-level helper functions: */

    /* mz_zip_add_mem_to_archive_file_in_place() efficiently (but not atomically) appends a memory blob to a ZIP archive. */
    /* Note this is NOT a fully safe operation. If it crashes or dies in some way your archive can be left in a screwed up state (without a central directory). */
    /* level_and_flags - compression level (0-10, see MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to MZ_DEFAULT_COMPRESSION. */
    /* TODO: Perhaps add an option to leave the existing central dir in place in case the add dies? We could then truncate the file (so the old central dir would be at the end) if something goes wrong. */
    MINIZ_EXPORT mz_bool mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags);
    MINIZ_EXPORT mz_bool mz_zip_add_mem_to_archive_file_in_place_v2(const char *pZip_filename, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags, mz_zip_error *pErr);

#ifndef MINIZ_NO_STDIO
    /* Reads a single file from an archive into a heap block. */
    /* If pComment is not NULL, only the file with the specified comment will be extracted. */
    /* Returns NULL on failure. */
    MINIZ_EXPORT void *mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name, size_t *pSize, mz_uint flags);
    MINIZ_EXPORT void *mz_zip_extract_archive_file_to_heap_v2(const char *pZip_filename, const char *pArchive_name, const char *pComment, size_t *pSize, mz_uint flags, mz_zip_error *pErr);
#endif

#endif /* #ifndef MINIZ_NO_ARCHIVE_WRITING_APIS */

#ifdef __cplusplus
}
#endif

#endif /* MINIZ_NO_ARCHIVE_APIS */

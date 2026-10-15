#ifndef SI_7ZIP_H
#define SI_7ZIP_H

#include <stdint.h>
#include <stddef.h>

#ifndef SI7ZAPI
#define SI7ZAPI __cdecl
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SI7Z_FORMAT
{
    SI7Z_FORMAT_AUTO = 0,
    SI7Z_FORMAT_7Z = 1,
    SI7Z_FORMAT_ZIP = 2
} SI7Z_FORMAT;

typedef enum _SI7Z_METHOD
{
    SI7Z_METHOD_DEFAULT = 0,
    SI7Z_METHOD_COPY,
    SI7Z_METHOD_DEFLATE,
    SI7Z_METHOD_DEFLATE64,
    SI7Z_METHOD_BZIP2,
    SI7Z_METHOD_LZMA,
    SI7Z_METHOD_LZMA2,
    SI7Z_METHOD_XZ,
    SI7Z_METHOD_PPMD,
    SI7Z_METHOD_ZSTD
} SI7Z_METHOD;

typedef enum _SI7Z_RESULT
{
    SI7Z_OK = 0,
    SI7Z_ERROR_INVALID_ARGUMENT = 1,
    SI7Z_ERROR_UNSUPPORTED_FORMAT = 2,
    SI7Z_ERROR_UNSUPPORTED_METHOD = 3,
    SI7Z_ERROR_NOT_IMPLEMENTED = 4,
    SI7Z_ERROR_OPEN_FAILED = 5,
    SI7Z_ERROR_ARCHIVE_FAILED = 6,
    SI7Z_ERROR_OUT_OF_MEMORY = 7,
    SI7Z_ERROR_ITEM_NOT_FOUND = 8,
    SI7Z_ERROR_DATA_ERROR = 9, // CRC mismatch, corrupt data, or a wrong or missing password
    SI7Z_ERROR_UNSAFE_PATH = 10, // an entry path would escape the destination or write through a reparse point
    SI7Z_ERROR_WRITE_FAILED = 11
} SI7Z_RESULT;

typedef struct _SI7Z_OPTIONS
{
    SI7Z_FORMAT Format;
    SI7Z_METHOD Method;
    uint32_t Level;
    const wchar_t* Password;
} SI7Z_OPTIONS;

typedef struct _SI7Z_ITEM
{
    wchar_t* Name;
    uint64_t Size;
    uint64_t PackedSize;
    uint32_t Attributes;
    uint32_t IsDirectory;
    uint32_t Method; // SI7Z_METHOD, SI7Z_METHOD_DEFAULT when unknown
} SI7Z_ITEM;

typedef struct _SI7Z_LIST
{
    SI7Z_ITEM* Items;
    uint32_t Count;
} SI7Z_LIST;

SI7Z_RESULT SI7ZAPI Si7zCompressArchive(const wchar_t* archivePath, const wchar_t* sourcePath, const SI7Z_OPTIONS* options);
SI7Z_RESULT SI7ZAPI Si7zExtractArchive(const wchar_t* archivePath, const wchar_t* destinationPath, const SI7Z_OPTIONS* options);
SI7Z_RESULT SI7ZAPI Si7zListArchive(const wchar_t* archivePath, const SI7Z_OPTIONS* options, SI7Z_LIST* list);
SI7Z_RESULT SI7ZAPI Si7zCompressBuffer(const void* sourceData, size_t sourceSize, const wchar_t* itemName, const SI7Z_OPTIONS* options, void** archiveData, size_t* archiveSize);
// Reads 7z and ZIP archives. With Format == SI7Z_FORMAT_AUTO the format is detected
// from the signature; Method is ignored. Password is used for encrypted archives.
// Si7zExtractBuffer requires an archive with exactly one file (otherwise
// SI7Z_ERROR_INVALID_ARGUMENT); Si7zExtractBufferItem extracts the file whose
// path matches itemName ('/' and '\' are equivalent, case-insensitive).
SI7Z_RESULT SI7ZAPI Si7zExtractBuffer(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, void** outputData, size_t* outputSize);
SI7Z_RESULT SI7ZAPI Si7zExtractBufferItem(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, const wchar_t* itemName, void** outputData, size_t* outputSize);
// Extracts every entry under destinationPath. Drive letters and leading separators are
// stripped; entries containing "..", ':' or other unsafe components fail the whole call
// before anything is written, and existing reparse points are never written through.
// Modification times are restored. Extraction stops at the first failed entry.
SI7Z_RESULT SI7ZAPI Si7zExtractBufferToDirectory(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, const wchar_t* destinationPath);
SI7Z_RESULT SI7ZAPI Si7zListBuffer(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, SI7Z_LIST* list);
void SI7ZAPI Si7zFreeBuffer(void* buffer);
void SI7ZAPI Si7zFreeList(SI7Z_LIST* list);

#ifdef __cplusplus
}
#endif

#endif


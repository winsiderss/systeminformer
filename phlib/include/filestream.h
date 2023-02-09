#ifndef _PH_FILESTREAM_H
#define _PH_FILESTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

// Core flags (PhCreateFileStream2)
/** Indicates that the file stream object should not close the file handle upon deletion. */
#define PH_FILE_STREAM_HANDLE_UNOWNED 0x1
/**
 * Indicates that the file stream object should not buffer I/O operations. Note that this does not
 * prevent the operating system from buffering I/O.
 */
#define PH_FILE_STREAM_UNBUFFERED 0x2
/**
 * Indicates that the file handle supports asynchronous operations. The file handle must not have
 * been opened with FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NONALERT.
 */
#define PH_FILE_STREAM_ASYNCHRONOUS 0x4
/**
 * Indicates that the file stream object should maintain the file position and not use the file
 * object's own file position.
 */
#define PH_FILE_STREAM_OWN_POSITION 0x8

// Higher-level flags (PhCreateFileStream)
#define PH_FILE_STREAM_APPEND 0x00010000

// Internal flags
/** Indicates that at least one write has been issued to the file handle. */
#define PH_FILE_STREAM_WRITTEN 0x80000000

// Seek
typedef enum _PH_SEEK_ORIGIN
{
    SeekStart,
    SeekCurrent,
    SeekEnd
} PH_SEEK_ORIGIN;

typedef struct _PH_FILE_STREAM
{
    HANDLE FileHandle;
    ULONG Flags;
    LARGE_INTEGER Position; // file object position, *not* the actual position

    PVOID Buffer;
    ULONG BufferLength;

    ULONG ReadPosition; // read position in buffer
    ULONG ReadLength; // how much available to read from buffer
    ULONG WritePosition; // write position in buffer
} PH_FILE_STREAM, *PPH_FILE_STREAM;

extern PPH_OBJECT_TYPE PhFileStreamType;

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileStream(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareMode,
    _In_ ULONG CreateDisposition,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileStream2(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _In_ ULONG BufferLength
    );

PHLIBAPI
VOID
NTAPI
PhVerifyFileStream(
    _In_ PPH_FILE_STREAM FileStream
    );

PHLIBAPI
NTSTATUS
NTAPI
PhReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReadLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFlushFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ BOOLEAN Full
    );

PHLIBAPI
VOID
NTAPI
PhGetPositionFileStream(
    _In_ PPH_FILE_STREAM FileStream,
    _Out_ PLARGE_INTEGER Position
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSeekFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Offset,
    _In_ PH_SEEK_ORIGIN Origin
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN Shared
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnlockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PPH_STRINGREF String
    );

FORCEINLINE
NTSTATUS 
PhWriteStringAsUtf8FileStream2(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PWSTR String
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);

    return PhWriteStringAsUtf8FileStream(FileStream, &string);
}

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringAsUtf8FileStreamEx(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PWSTR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringFormatAsUtf8FileStream_V(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PWSTR Format,
    _In_ va_list ArgPtr
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringFormatAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    );

#ifdef __cplusplus
}
#endif

#endif

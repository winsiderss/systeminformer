/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#include <ph.h>
#include <filestream.h>
#include <filestreamp.h>

PPH_OBJECT_TYPE PhFileStreamType = NULL;

/**
 * Creates a file stream from a file path.
 *
 * \param FileStream Receives the new file stream object.
 * \param FileName Path to the file to open.
 * \param DesiredAccess Requested access mask.
 * \param ShareAccess Share access flags.
 * \param CreateDisposition Create/open disposition for the file.
 * \param Flags Stream creation flags (buffering, append, asynchronous, etc.)
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateFileStream(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    PPH_FILE_STREAM fileStream;
    HANDLE fileHandle;
    ULONG createOptions;

    if (Flags & PH_FILE_STREAM_ASYNCHRONOUS)
        createOptions = FILE_NON_DIRECTORY_FILE;
    else
        createOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        DesiredAccess,
        FILE_ATTRIBUTE_NORMAL,
        ShareAccess,
        CreateDisposition,
        createOptions
        )))
        return status;

    if (!NT_SUCCESS(status = PhCreateFileStream2(
        &fileStream,
        fileHandle,
        Flags,
        PAGE_SIZE
        )))
    {
        NtClose(fileHandle);
        return status;
    }

    if (Flags & PH_FILE_STREAM_APPEND)
    {
        LARGE_INTEGER zero;

        zero.QuadPart = 0;

        if (!NT_SUCCESS(status = PhSeekFileStream(
            fileStream,
            &zero,
            SeekEnd
            )))
        {
            PhDereferenceObject(fileStream);
            return status;
        }
    }

    *FileStream = fileStream;

    return status;
}

/**
 * Creates a file stream from an existing file handle.
 *
 * \param FileStream Receives the new file stream object.
 * \param FileHandle Open file handle (ownership depends on flags).
 * \param Flags Stream flags (buffering, unowned handle, etc.)
 * \param BufferLength Buffer size used for buffered operations.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateFileStream2(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _In_ ULONG BufferLength
    )
{
    static PH_INITONCE fileStreamInitOnce = PH_INITONCE_INIT;
    PPH_FILE_STREAM fileStream;

    if (PhBeginInitOnce(&fileStreamInitOnce))
    {
        PH_OBJECT_TYPE_PARAMETERS parameters;

        parameters.FreeListSize = sizeof(PH_FILE_STREAM);
        parameters.FreeListCount = 16;

        PhFileStreamType = PhCreateObjectTypeEx(
            L"FileStream",
            PH_OBJECT_TYPE_USE_FREE_LIST,
            PhpFileStreamDeleteProcedure,
            &parameters
            );

        PhEndInitOnce(&fileStreamInitOnce);
    }

    fileStream = PhCreateObject(sizeof(PH_FILE_STREAM), PhFileStreamType);
    fileStream->FileHandle = FileHandle;
    fileStream->Flags = Flags;
    fileStream->Position.QuadPart = 0;

    if (!(Flags & PH_FILE_STREAM_UNBUFFERED))
    {
        fileStream->Buffer = NULL;
        fileStream->BufferLength = BufferLength;
    }
    else
    {
        fileStream->Buffer = NULL;
        fileStream->BufferLength = 0;
    }

    fileStream->ReadPosition = 0;
    fileStream->ReadLength = 0;
    fileStream->WritePosition = 0;

    *FileStream = fileStream;

    return STATUS_SUCCESS;
}

/**
 * The object delete procedure for file stream objects ensures
 * buffers are flushed, closes the handle if owned, and frees the buffer.
 *
 * \param Object The file stream object being deleted.
 * \param Flags Delete flags (unused).
 */
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpFileStreamDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_FILE_STREAM fileStream = (PPH_FILE_STREAM)Object;

    PhFlushFileStream(fileStream, FALSE);

    if (!(fileStream->Flags & PH_FILE_STREAM_HANDLE_UNOWNED))
        NtClose(fileStream->FileHandle);

    if (fileStream->Buffer)
        PhFreePage(fileStream->Buffer);
}

/**
 * Verifies that a file stream's position matches the position held by the file object.
 */
VOID PhVerifyFileStream(
    _In_ PPH_FILE_STREAM FileStream
    )
{
    NTSTATUS status;

    // If the file object is asynchronous, the file object doesn't maintain its position.
    if (!(FileStream->Flags & (
        PH_FILE_STREAM_OWN_POSITION |
        PH_FILE_STREAM_ASYNCHRONOUS
        )))
    {
        FILE_POSITION_INFORMATION positionInfo;
        IO_STATUS_BLOCK isb;

        if (!NT_SUCCESS(status = NtQueryInformationFile(
            FileStream->FileHandle,
            &isb,
            &positionInfo,
            sizeof(FILE_POSITION_INFORMATION),
            FilePositionInformation
            )))
            PhRaiseStatus(status);

        if (FileStream->Position.QuadPart != positionInfo.CurrentByteOffset.QuadPart)
            PhRaiseStatus(STATUS_INTERNAL_ERROR);
    }
}

/**
 * Allocates the page-aligned buffer for a buffered file stream.
 *
 * \param FileStream Stream requiring a buffer.
 * \return STATUS_SUCCESS on success or STATUS_NO_MEMORY.
 */
NTSTATUS PhpAllocateBufferFileStream(
    _Inout_ PPH_FILE_STREAM FileStream
    )
{
    FileStream->Buffer = PhAllocatePage(FileStream->BufferLength, NULL);

    if (FileStream->Buffer)
        return STATUS_SUCCESS;
    else
        return STATUS_NO_MEMORY;
}

/**
 * Performs a raw read from the underlying file object (unbuffered).
 *
 * \param FileStream Stream to read from.
 * \param Buffer Destination buffer.
 * \param Length Number of bytes to read.
 * \param ReadLength Receives number of bytes actually read (optional).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReadLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PLARGE_INTEGER position;

    position = NULL;

    if (FileStream->Flags & PH_FILE_STREAM_OWN_POSITION)
        position = &FileStream->Position;

    status = NtReadFile(
        FileStream->FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        Length,
        position,
        NULL
        );

    if (status == STATUS_PENDING)
    {
        // Wait for the operation to finish. This probably means we got called on an asynchronous
        // file object.
        status = NtWaitForSingleObject(FileStream->FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    if (NT_SUCCESS(status))
    {
        FileStream->Position.QuadPart += isb.Information;

        if (ReadLength)
            *ReadLength = (ULONG)isb.Information;
    }

    return status;
}

/**
 * Reads from a file stream with optional buffering. If the stream is unbuffered
 * this forwards to the raw read routine. For buffered streams it satisfies
 * reads from the internal buffer where possible and fills it as needed.
 *
 * \param FileStream Stream to read from.
 * \param Buffer Destination buffer.
 * \param Length Number of bytes to read.
 * \param ReadLength Receives number of bytes read (optional).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReadLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG availableLength;
    ULONG readLength;

    if (FileStream->Flags & PH_FILE_STREAM_UNBUFFERED)
    {
        return PhpReadFileStream(
            FileStream,
            Buffer,
            Length,
            ReadLength
            );
    }

    // How much do we have available to copy out of the buffer?
    availableLength = FileStream->ReadLength - FileStream->ReadPosition;

    if (availableLength == 0)
    {
        // Make sure buffered writes are flushed.
        if (FileStream->WritePosition != 0)
        {
            if (!NT_SUCCESS(status = PhpFlushWriteFileStream(FileStream)))
                return status;
        }

        // If this read is too big, pass it through.
        if (Length >= FileStream->BufferLength)
        {
            // These are now invalid.
            FileStream->ReadPosition = 0;
            FileStream->ReadLength = 0;

            return PhpReadFileStream(
                FileStream,
                Buffer,
                Length,
                ReadLength
                );
        }

        if (!FileStream->Buffer)
        {
            if (!NT_SUCCESS(status = PhpAllocateBufferFileStream(FileStream)))
                return status;
        }

        // Read as much as we can into our buffer.
        if (!NT_SUCCESS(status = PhpReadFileStream(
            FileStream,
            FileStream->Buffer,
            FileStream->BufferLength,
            &readLength
            )))
            return status;

        if (readLength == 0)
        {
            // No data read.
            if (ReadLength)
                *ReadLength = readLength;

            return status;
        }

        FileStream->ReadPosition = 0;
        FileStream->ReadLength = readLength;
    }
    else
    {
        readLength = availableLength;
    }

    if (readLength > Length)
        readLength = Length;

    // Try to satisfy the request from the buffer.
    memcpy(
        Buffer,
        PTR_ADD_OFFSET(FileStream->Buffer, FileStream->ReadPosition),
        readLength
        );
    FileStream->ReadPosition += readLength;

    // If we didn't completely satisfy the request, read some more.
    if (
        readLength < Length &&
        // Don't try to read more if the buffer wasn't even filled up last time. (No more to read.)
        FileStream->ReadLength == FileStream->BufferLength
        )
    {
        ULONG readLength2;

        if (NT_SUCCESS(status = PhpReadFileStream(
            FileStream,
            PTR_ADD_OFFSET(Buffer, readLength),
            Length - readLength,
            &readLength2
            )))
        {
            readLength += readLength2;
            // These are now invalid.
            FileStream->ReadPosition = 0;
            FileStream->ReadLength = 0;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (ReadLength)
            *ReadLength = readLength;
    }

    return status;
}

/**
 * Performs a raw write to the underlying file object (unbuffered).
 *
 * \param FileStream Stream to write to.
 * \param Buffer Source buffer.
 * \param Length Number of bytes to write.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PLARGE_INTEGER position;

    position = NULL;

    if (FileStream->Flags & PH_FILE_STREAM_OWN_POSITION)
        position = &FileStream->Position;

    status = NtWriteFile(
        FileStream->FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        Length,
        position,
        NULL
        );

    if (status == STATUS_PENDING)
    {
        // Wait for the operation to finish. This probably means we got called on an asynchronous
        // file object.
        status = NtWaitForSingleObject(FileStream->FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    if (NT_SUCCESS(status))
    {
        FileStream->Position.QuadPart += isb.Information;
        FileStream->Flags |= PH_FILE_STREAM_WRITTEN;
    }

    return status;
}

/**
 * Writes to a file stream with optional buffering. For buffered streams
 * this will append to the internal buffer and flush as needed. For
 * unbuffered streams this forwards to the raw write routine.
 *
 * \param FileStream Stream to write to.
 * \param Buffer Source buffer.
 * \param Length Number of bytes to write.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG availableLength;
    ULONG writtenLength;

    if (FileStream->Flags & PH_FILE_STREAM_UNBUFFERED)
    {
        return PhpWriteFileStream(
            FileStream,
            Buffer,
            Length
            );
    }

    if (FileStream->WritePosition == 0)
    {
        // Make sure buffered reads are flushed.
        if (!NT_SUCCESS(status = PhpFlushReadFileStream(FileStream)))
            return status;
    }

    if (FileStream->WritePosition != 0)
    {
        availableLength = FileStream->BufferLength - FileStream->WritePosition;

        // Try to satisfy the request by copying the data to the buffer.
        if (availableLength != 0)
        {
            writtenLength = availableLength;

            if (writtenLength > Length)
                writtenLength = Length;

            memcpy(
                PTR_ADD_OFFSET(FileStream->Buffer, FileStream->WritePosition),
                Buffer,
                writtenLength
                );
            FileStream->WritePosition += writtenLength;

            if (writtenLength == Length)
            {
                // The request has been completely satisfied.
                return status;
            }

            Buffer = PTR_ADD_OFFSET(Buffer, writtenLength);
            Length -= writtenLength;
        }

        // If we didn't completely satisfy the request, it's because the buffer is full. Flush it.
        if (!NT_SUCCESS(status = PhpWriteFileStream(
            FileStream,
            FileStream->Buffer,
            FileStream->WritePosition
            )))
            return status;

        FileStream->WritePosition = 0;
    }

    // If the write is too big, pass it through.
    if (Length >= FileStream->BufferLength)
    {
        if (!NT_SUCCESS(status = PhpWriteFileStream(
            FileStream,
            Buffer,
            Length
            )))
            return status;
    }
    else if (Length != 0)
    {
        if (!FileStream->Buffer)
        {
            if (!NT_SUCCESS(status = PhpAllocateBufferFileStream(FileStream)))
                return status;
        }

        // Completely satisfy the request by copying the data to the buffer.
        memcpy(
            FileStream->Buffer,
            Buffer,
            Length
            );
        FileStream->WritePosition = Length;
    }

    return status;
}

/**
 * Flushes any buffered read state back to the file position. If there is unread
 * buffered data the stream position is moved backward to the first unread byte
 * and subsequent reads align with the file object.
 *
 * \param FileStream Stream to flush read buffer from.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpFlushReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (FileStream->ReadLength - FileStream->ReadPosition != 0)
    {
        LARGE_INTEGER offset;

        // We have some buffered read data, so our position is too far ahead. We need to move it
        // back to the first unused byte.
        offset.QuadPart = -(LONG)(FileStream->ReadLength - FileStream->ReadPosition);

        if (!NT_SUCCESS(status = PhpSeekFileStream(
            FileStream,
            &offset,
            SeekCurrent
            )))
            return status;
    }

    FileStream->ReadPosition = 0;
    FileStream->ReadLength = 0;

    return status;
}

/**
 * Flushes any buffered write data to the underlying file.
 *
 * \param FileStream Stream to flush.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpFlushWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!NT_SUCCESS(status = PhpWriteFileStream(
        FileStream,
        FileStream->Buffer,
        FileStream->WritePosition
        )))
        return status;

    FileStream->WritePosition = 0;

    return status;
}

/**
 * Flushes the file stream buffers and optionally flushes the file through the OS.
 *
 * \param FileStream A file stream object.
 * \param Full TRUE to flush the file object through the operating system, otherwise FALSE to only
 * ensure the buffer is flushed to the operating system.
 */
NTSTATUS PhFlushFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ BOOLEAN Full
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (FileStream->WritePosition != 0)
    {
        if (!NT_SUCCESS(status = PhpFlushWriteFileStream(FileStream)))
            return status;
    }

    if (FileStream->ReadPosition != 0)
    {
        if (!NT_SUCCESS(status = PhpFlushReadFileStream(FileStream)))
            return status;
    }

    if (Full && (FileStream->Flags & PH_FILE_STREAM_WRITTEN))
    {
        IO_STATUS_BLOCK isb;

        if (!NT_SUCCESS(status = NtFlushBuffersFile(
            FileStream->FileHandle,
            &isb
            )))
            return status;
    }

    return status;
}

/**
 * Retrieves the logical position within the stream (taking buffered state into account).
 *
 * \param FileStream Stream to query.
 * \param Position Receives the calculated position.
 */
VOID PhGetPositionFileStream(
    _In_ PPH_FILE_STREAM FileStream,
    _Out_ PLARGE_INTEGER Position
    )
{
    Position->QuadPart =
        FileStream->Position.QuadPart +
        (FileStream->ReadPosition - FileStream->ReadLength) +
        FileStream->WritePosition;
}

/**
 * Performs a seek operation that updates the internal position variable and, if required,
 * writes the position to the underlying file object. This does not account for buffered
 * read/write state; use PhSeekFileStream to handle buffers.
 *
 * \param FileStream Stream to seek.
 * \param Offset Offset value (interpretation per Origin).
 * \param Origin Seek origin (SeekStart, SeekCurrent, SeekEnd).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpSeekFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Offset,
    _In_ PH_SEEK_ORIGIN Origin
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (Origin)
    {
    case SeekStart:
        {
            FileStream->Position = *Offset;
        }
        break;
    case SeekCurrent:
        {
            FileStream->Position.QuadPart += Offset->QuadPart;
        }
        break;
    case SeekEnd:
        {
            if (!NT_SUCCESS(status = PhGetFileSize(
                FileStream->FileHandle,
                &FileStream->Position
                )))
                return status;

            FileStream->Position.QuadPart += Offset->QuadPart;
        }
        break;
    }

    if (!(FileStream->Flags & PH_FILE_STREAM_OWN_POSITION))
    {
        FILE_POSITION_INFORMATION positionInfo;
        IO_STATUS_BLOCK isb;

        positionInfo.CurrentByteOffset = FileStream->Position;

        if (!NT_SUCCESS(status = NtSetInformationFile(
            FileStream->FileHandle,
            &isb,
            &positionInfo,
            sizeof(FILE_POSITION_INFORMATION),
            FilePositionInformation
            )))
            return status;
    }

    return status;
}

/**
 * Public seek that handles buffered state (flushes writes, adjusts for buffered reads).
 *
 * \param FileStream Stream to seek.
 * \param Offset Offset value (interpretation per Origin).
 * \param Origin Seek origin (SeekStart, SeekCurrent, SeekEnd).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSeekFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Offset,
    _In_ PH_SEEK_ORIGIN Origin
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER offset;

    offset = *Offset;

    if (FileStream->WritePosition != 0)
    {
        if (!NT_SUCCESS(status = PhpFlushWriteFileStream(FileStream)))
            return status;
    }
    else if (FileStream->ReadPosition != 0)
    {
        if (Origin == SeekCurrent)
        {
            // We have buffered read data, which means our position is too far ahead. Subtract this
            // difference from the offset (which will affect the position accordingly).
            offset.QuadPart -= FileStream->ReadLength - FileStream->ReadPosition;
        }

        // TODO: Try to keep some of the read buffer.
        FileStream->ReadPosition = 0;
        FileStream->ReadLength = 0;
    }

    if (!NT_SUCCESS(status = PhpSeekFileStream(
        FileStream,
        &offset,
        Origin
        )))
        return status;

    return status;
}

/**
 * Locks a byte range in the file.
 *
 * \param FileStream Stream containing the file handle.
 * \param Position Starting byte offset to lock.
 * \param Length Length of the region to lock.
 * \param Wait If TRUE, wait for the lock; otherwise return immediately if unavailable.
 * \param Shared If TRUE, acquire a shared lock; otherwise exclusive.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhLockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN Shared
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtLockFile(
        FileStream->FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Position,
        Length,
        0,
        !Wait,
        !Shared
        );

    if (status == STATUS_PENDING)
    {
        // Wait for the operation to finish. This probably means we got called on an asynchronous
        // file object.
        NtWaitForSingleObject(FileStream->FileHandle, FALSE, NULL);
        status = isb.Status;
    }

    return status;
}

/**
 * Unlocks a previously locked byte range.
 *
 * \param FileStream Stream containing the file handle.
 * \param Position Starting byte offset of the locked region.
 * \param Length Length of the region to unlock.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhUnlockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length
    )
{
    IO_STATUS_BLOCK isb;

    return NtUnlockFile(
        FileStream->FileHandle,
        &isb,
        Position,
        Length,
        0
        );
}

/**
 * Sets the allocation and end-of-file size for the file.
 *
 * \param FileStream Stream containing the file handle.
 * \param AllocationSize Desired allocation / end-of-file size.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetAllocationSizeFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER AllocationSize
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_END_OF_FILE_INFORMATION endOfFileInfo;
    FILE_ALLOCATION_INFORMATION allocationInfo;

    memset(&endOfFileInfo, 0, sizeof(FILE_END_OF_FILE_INFORMATION));
    endOfFileInfo.EndOfFile.QuadPart = AllocationSize->QuadPart;

    if (!NT_SUCCESS(status = NtSetInformationFile(
        FileStream->FileHandle,
        &isb,
        &endOfFileInfo,
        sizeof(FILE_END_OF_FILE_INFORMATION),
        FileEndOfFileInformation
        )))
        return status;

    memset(&allocationInfo, 0, sizeof(FILE_ALLOCATION_INFORMATION));
    allocationInfo.AllocationSize.QuadPart = AllocationSize->QuadPart;

    if (!NT_SUCCESS(status = NtSetInformationFile(
        FileStream->FileHandle,
        &isb,
        &allocationInfo,
        sizeof(FILE_ALLOCATION_INFORMATION),
        FileAllocationInformation
        )))
        return status;

    return status;
}

/**
 * Writes a Unicode string as UTF-8 to the stream.
 *
 * \param FileStream Stream to write to.
 * \param String String reference to write.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhWriteStringAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PPH_STRINGREF String
    )
{
    return PhWriteStringAsUtf8FileStreamEx(FileStream, String->Buffer, String->Length);
}

/**
 * Writes a UTF-16 buffer as UTF-8 to the stream, handling large buffers in chunks.
 *
 * \param FileStream Stream to write to.
 * \param Buffer UTF-16 buffer to convert and write.
 * \param Length The length of Buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhWriteStringAsUtf8FileStreamEx(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PCWSTR Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PH_STRINGREF block;
    SIZE_T inPlaceUtf8Size = 0;
    PCHAR inPlaceUtf8 = NULL;
    PPH_BYTES utf8 = NULL;

    if (Length > PAGE_SIZE)
    {
        // In UTF-8, the maximum number of bytes per code point is 4.
        inPlaceUtf8Size = PAGE_SIZE / sizeof(WCHAR) * 4;
        inPlaceUtf8 = PhAllocatePage(inPlaceUtf8Size, NULL);
    }

    while (Length != 0)
    {
        block.Buffer = (PWCH)Buffer;
        block.Length = PAGE_SIZE;

        if (block.Length > Length)
            block.Length = Length;

        if (inPlaceUtf8)
        {
            SIZE_T bytesInUtf8String;

            status = PhConvertUtf16ToUtf8Buffer(
                inPlaceUtf8,
                inPlaceUtf8Size,
                &bytesInUtf8String,
                block.Buffer,
                block.Length
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhWriteFileStream(FileStream, inPlaceUtf8, (ULONG)bytesInUtf8String);
        }
        else
        {
            utf8 = PhConvertUtf16ToUtf8Ex(block.Buffer, block.Length);

            if (!utf8)
            {
                status = STATUS_INVALID_PARAMETER;
                goto CleanupExit;
            }

            status = PhWriteFileStream(FileStream, utf8->Buffer, (ULONG)utf8->Length);
            PhDereferenceObject(utf8);
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        Buffer += block.Length / sizeof(WCHAR);
        Length -= block.Length;
    }

CleanupExit:
    if (inPlaceUtf8)
        PhFreePage(inPlaceUtf8);

    return status;
}

/**
 * Formats a Unicode string (va_list) and writes it as UTF-8 to the stream.
 *
 * \param FileStream Stream to write to.
 * \param Format printf-style format string.
 * \param ArgPtr Variadic arguments (va_list).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhWriteStringFormatAsUtf8FileStream_V(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    NTSTATUS status;
    PPH_STRING string;

    string = PhFormatString_V(Format, ArgPtr);
    status = PhWriteStringAsUtf8FileStream(FileStream, &string->sr);
    PhDereferenceObject(string);

    return status;
}

/**
 * Formats a Unicode string and writes it as UTF-8 to the stream.
 *
 * \param FileStream Stream to write to.
 * \param Format printf-style format string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhWriteStringFormatAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    )
{
    NTSTATUS status;
    va_list argptr;

    va_start(argptr, Format);
    status = PhWriteStringFormatAsUtf8FileStream_V(FileStream, Format, argptr);
    va_end(argptr);

    return status;
}

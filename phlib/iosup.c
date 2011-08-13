/*
 * Process Hacker - 
 *   I/O support functions
 * 
 * Copyright (C) 2010-2011 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _PH_IOSUP_PRIVATE
#include <ph.h>
#include <iosupp.h>

PPH_OBJECT_TYPE PhFileStreamType;

BOOLEAN PhIoSupportInitialization()
{
    PH_OBJECT_TYPE_PARAMETERS parameters;

    parameters.FreeListSize = sizeof(PH_FILE_STREAM);
    parameters.FreeListCount = 16;

    if (!NT_SUCCESS(PhCreateObjectTypeEx(
        &PhFileStreamType,
        L"FileStream",
        PHOBJTYPE_USE_FREE_LIST,
        PhpFileStreamDeleteProcedure,
        &parameters
        )))
        return FALSE;

    return TRUE;
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param FileAttributes File attributes applied if the file is 
 * created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does 
 * or does not exist.
 * \li \c FILE_SUPERSEDE If the file exists, replace it. Otherwise, create the file.
 * \li \c FILE_CREATE If the file exists, fail. Otherwise, create the file.
 * \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
 * \li \c FILE_OPEN_IF If the file exists, open it. Otherwise, create the file.
 * \li \c FILE_OVERWRITE If the file exists, open and overwrite it. Otherwise, fail.
 * \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it. Otherwise, create the file.
 * \param CreateOptions The options to apply when the file is opened or created.
 */
NTSTATUS PhCreateFileWin32(
    __out PHANDLE FileHandle,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in_opt ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions
    )
{
    return PhCreateFileWin32Ex(
        FileHandle,
        FileName,
        DesiredAccess,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL
        );
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param FileAttributes File attributes applied if the file is 
 * created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does 
 * or does not exist.
 * \li \c FILE_SUPERSEDE If the file exists, replace it. Otherwise, create the file.
 * \li \c FILE_CREATE If the file exists, fail. Otherwise, create the file.
 * \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
 * \li \c FILE_OPEN_IF If the file exists, open it. Otherwise, create the file.
 * \li \c FILE_OVERWRITE If the file exists, open and overwrite it. Otherwise, fail.
 * \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it. Otherwise, create the file.
 * \param CreateOptions The options to apply when the file is opened or created.
 * \param CreateStatus A variable that receives creation information.
 * \li \c FILE_SUPERSEDED The file was replaced because \c FILE_SUPERSEDE was specified in 
 * \a CreateDisposition.
 * \li \c FILE_OPENED The file was opened because \c FILE_OPEN or \c FILE_OPEN_IF was specified in 
 * \a CreateDisposition.
 * \li \c FILE_CREATED The file was created because \c FILE_CREATE or \c FILE_OPEN_IF was specified 
 * in \a CreateDisposition.
 * \li \c FILE_OVERWRITTEN The file was overwritten because \c FILE_OVERWRITE or \c FILE_OVERWRITE_IF 
 * was specified in \a CreateDisposition.
 * \li \c FILE_EXISTS The file was not opened because it already existed and \c FILE_CREATE was 
 * specified in \a CreateDisposition.
 * \li \c FILE_DOES_NOT_EXIST The file was not opened because it did not exist and \c FILE_OPEN or 
 * \c FILE_OVERWRITE was specified in \a CreateDisposition.
 */
NTSTATUS PhCreateFileWin32Ex(
    __out PHANDLE FileHandle,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in_opt ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions,
    __out_opt PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    if (!FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    if (!RtlDosPathNameToNtPathName_U(
        FileName,
        &fileName,
        NULL,
        NULL
        ))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    InitializeObjectAttributes(
        &oa,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &oa,
        &isb,
        NULL,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    RtlFreeHeap(RtlProcessHeap(), 0, fileName.Buffer);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)isb.Information;

    return status;
}

/**
 * Deletes a file.
 *
 * \param FileName The Win32 file name.
 */
NTSTATUS PhDeleteFileWin32(
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        DELETE,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DELETE_ON_CLOSE
        );

    if (!NT_SUCCESS(status))
        return status;

    NtClose(fileHandle);

    return status;
}

NTSTATUS PhListenNamedPipe(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock
    )
{
    return NtFsControlFile(
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        FSCTL_PIPE_LISTEN,
        NULL,
        0,
        NULL,
        0
        );
}

NTSTATUS PhDisconnectNamedPipe(
    __in HANDLE FileHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_DISCONNECT,
        NULL,
        0,
        NULL,
        0
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

NTSTATUS PhPeekNamedPipe(
    __in HANDLE FileHandle,
    __out_bcount_opt(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG NumberOfBytesRead,
    __out_opt PULONG NumberOfBytesAvailable,
    __out_opt PULONG NumberOfBytesLeftInMessage
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PFILE_PIPE_PEEK_BUFFER peekBuffer;
    ULONG peekBufferLength;

    peekBufferLength = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data) + Length;
    peekBuffer = PhAllocate(peekBufferLength);

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_PEEK,
        NULL,
        0,
        peekBuffer,
        peekBufferLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    // STATUS_BUFFER_OVERFLOW means that there is data remaining; this is normal.
    if (status == STATUS_BUFFER_OVERFLOW)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        ULONG numberOfBytesRead;

        if (Buffer || NumberOfBytesRead || NumberOfBytesLeftInMessage)
            numberOfBytesRead = (ULONG)(isb.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data));

        if (Buffer)
            memcpy(Buffer, peekBuffer->Data, numberOfBytesRead);

        if (NumberOfBytesRead)
            *NumberOfBytesRead = numberOfBytesRead;

        if (NumberOfBytesAvailable)
            *NumberOfBytesAvailable = peekBuffer->ReadDataAvailable;

        if (NumberOfBytesLeftInMessage)
            *NumberOfBytesLeftInMessage = peekBuffer->MessageLength - numberOfBytesRead;
    }

    PhFree(peekBuffer);

    return status;
}

NTSTATUS PhTransceiveNamedPipe(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(InputBufferLength) PVOID InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount(OutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferLength
    )
{
    return NtFsControlFile(
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        FSCTL_PIPE_TRANSCEIVE,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );
}

NTSTATUS PhWaitForNamedPipe(
    __in_opt PPH_STRINGREF FileSystemName,
    __in PPH_STRINGREF Name,
    __in_opt PLARGE_INTEGER Timeout,
    __in BOOLEAN UseDefaultTimeout
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PH_STRINGREF localNpfsName;
    HANDLE fileSystemHandle;
    OBJECT_ATTRIBUTES oa;
    PFILE_PIPE_WAIT_FOR_BUFFER waitForBuffer;
    ULONG waitForBufferLength;

    if (!FileSystemName)
    {
        PhInitializeStringRef(&localNpfsName, L"\\Device\\NamedPipe");
        FileSystemName = &localNpfsName;
    }

    InitializeObjectAttributes(
        &oa,
        &FileSystemName->us,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileSystemHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    waitForBufferLength = FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name) + Name->Length;
    waitForBuffer = PhAllocate(waitForBufferLength);

    if (UseDefaultTimeout)
    {
        waitForBuffer->TimeoutSpecified = FALSE;
    }
    else
    {
        if (Timeout)
        {
            waitForBuffer->Timeout = *Timeout;
        }
        else
        {
            waitForBuffer->Timeout.LowPart = 0;
            waitForBuffer->Timeout.HighPart = MINLONG; // a very long time
        }

        waitForBuffer->TimeoutSpecified = TRUE;
    }

    waitForBuffer->NameLength = Name->Length;
    memcpy(waitForBuffer->Name, Name->Buffer, Name->Length);

    status = NtFsControlFile(
        fileSystemHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_WAIT,
        waitForBuffer,
        waitForBufferLength,
        NULL,
        0
        );

    PhFree(waitForBuffer);
    NtClose(fileSystemHandle);

    return status;
}

NTSTATUS PhImpersonateClientOfNamedPipe(
    __in HANDLE FileHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_IMPERSONATE,
        NULL,
        0,
        NULL,
        0
        );

    return status;
}

NTSTATUS PhCreateFileStream(
    __out PPH_FILE_STREAM *FileStream,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG Flags
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
        0,
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

        if (!NT_SUCCESS(PhSeekFileStream(
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

NTSTATUS PhCreateFileStream2(
    __out PPH_FILE_STREAM *FileStream,
    __in HANDLE FileHandle,
    __in ULONG Flags,
    __in ULONG BufferLength
    )
{
    NTSTATUS status;
    PPH_FILE_STREAM fileStream;

    if (!NT_SUCCESS(status = PhCreateObject(
        &fileStream,
        sizeof(PH_FILE_STREAM),
        0,
        PhFileStreamType
        )))
        return status;

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

    return status;
}

VOID NTAPI PhpFileStreamDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
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
 * Verifies that a file stream's position matches 
 * the position held by the file object.
 */
VOID PhVerifyFileStream(
    __in PPH_FILE_STREAM FileStream
    )
{
    NTSTATUS status;

    // If the file object is asynchronous, the file object doesn't maintain 
    // its position.
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

NTSTATUS PhpAllocateBufferFileStream(
    __inout PPH_FILE_STREAM FileStream
    )
{
    FileStream->Buffer = PhAllocatePage(FileStream->BufferLength, NULL);

    if (FileStream->Buffer)
        return STATUS_SUCCESS;
    else
        return STATUS_NO_MEMORY;
}

NTSTATUS PhpReadFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReadLength
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
        // Wait for the operation to finish. This probably means we got 
        // called on an asynchronous file object.
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

NTSTATUS PhReadFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReadLength
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
        (PCHAR)FileStream->Buffer + FileStream->ReadPosition,
        readLength
        );
    FileStream->ReadPosition += readLength;

    // If we didn't completely satisfy the request, read some more.
    if (
        readLength < Length &&
        // Don't try to read more if the buffer wasn't even filled up 
        // last time. (No more to read.)
        FileStream->ReadLength == FileStream->BufferLength
        )
    {
        ULONG readLength2;

        if (NT_SUCCESS(status = PhpReadFileStream(
            FileStream,
            (PCHAR)Buffer + readLength,
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

NTSTATUS PhpWriteFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
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
        // Wait for the operation to finish. This probably means we got 
        // called on an asynchronous file object.
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

NTSTATUS PhWriteFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
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
                (PCHAR)FileStream->Buffer + FileStream->WritePosition,
                Buffer,
                writtenLength
                );
            FileStream->WritePosition += writtenLength;

            if (writtenLength == Length)
            {
                // The request has been completely satisfied.
                return status;
            }

            Buffer = (PCHAR)Buffer + writtenLength;
            Length -= writtenLength;
        }

        // If we didn't completely satisfy the request, it's because the 
        // buffer is full. Flush it.
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

NTSTATUS PhpFlushReadFileStream(
    __inout PPH_FILE_STREAM FileStream
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (FileStream->ReadLength - FileStream->ReadPosition != 0)
    {
        LARGE_INTEGER offset;

        // We have some buffered read data, so our position is 
        // too far ahead. We need to move it back to the first 
        // unused byte.
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

NTSTATUS PhpFlushWriteFileStream(
    __inout PPH_FILE_STREAM FileStream
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
 * Flushes the file stream.
 *
 * \param FileStream A file stream object.
 * \param Full TRUE to flush the file object through the 
 * operating system, otherwise FALSE to only ensure the buffer 
 * is flushed to the operating system.
 */
NTSTATUS PhFlushFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in BOOLEAN Full
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

VOID PhGetPositionFileStream(
    __in PPH_FILE_STREAM FileStream,
    __out PLARGE_INTEGER Position
    )
{
    Position->QuadPart =
        FileStream->Position.QuadPart +
        (FileStream->ReadPosition - FileStream->ReadLength) +
        FileStream->WritePosition;
}

NTSTATUS PhpSeekFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Offset,
    __in PH_SEEK_ORIGIN Origin
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

NTSTATUS PhSeekFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Offset,
    __in PH_SEEK_ORIGIN Origin
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
            // We have buffered read data, which means our position is too 
            // far ahead. Subtract this difference from the offset (which 
            // will affect the position accordingly).
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

NTSTATUS PhLockFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Position,
    __in PLARGE_INTEGER Length,
    __in BOOLEAN Wait,
    __in BOOLEAN Shared
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
        // Wait for the operation to finish. This probably means we got 
        // called on an asynchronous file object.
        NtWaitForSingleObject(FileStream->FileHandle, FALSE, NULL);
        status = isb.Status;
    }

    return status;
}

NTSTATUS PhUnlockFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Position,
    __in PLARGE_INTEGER Length
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

NTSTATUS PhWriteStringAsAnsiFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PPH_STRINGREF String
    )
{
    return PhWriteStringAsAnsiFileStreamEx(FileStream, String->Buffer, String->Length);
}

NTSTATUS PhWriteStringAsAnsiFileStream2(
    __inout PPH_FILE_STREAM FileStream,
    __in PWSTR String
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);

    return PhWriteStringAsAnsiFileStream(FileStream, &string);
}

NTSTATUS PhWriteStringAsAnsiFileStreamEx(
    __inout PPH_FILE_STREAM FileStream,
    __in PWSTR Buffer,
    __in SIZE_T Length
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING block;
    ANSI_STRING ansiString;

    while (Length != 0)
    {
        block.Buffer = Buffer;
        block.Length = PH_FILE_STREAM_STRING_BLOCK_SIZE;

        if (block.Length > Length)
            block.Length = (USHORT)Length;

        if (!NT_SUCCESS(status = RtlUnicodeStringToAnsiString(
            &ansiString,
            &block,
            TRUE
            )))
            return status;

        PhWriteFileStream(
            FileStream,
            ansiString.Buffer,
            ansiString.Length
            );

        RtlFreeAnsiString(&ansiString);

        Buffer += block.Length / sizeof(WCHAR);
        Length -= block.Length;
    }

    return status;
}

NTSTATUS PhWriteStringFormatFileStream_V(
    __inout PPH_FILE_STREAM FileStream,
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
    )
{
    NTSTATUS status;
    PPH_STRING string;

    string = PhFormatString_V(Format, ArgPtr);
    status = PhWriteStringAsAnsiFileStream(FileStream, &string->sr);
    PhDereferenceObject(string);

    return status;
}

NTSTATUS PhWriteStringFormatFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in __format_string PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PhWriteStringFormatFileStream_V(FileStream, Format, argptr);
}

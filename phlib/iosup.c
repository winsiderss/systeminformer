/*
 * Process Hacker - 
 *   I/O support functions
 * 
 * Copyright (C) 2010 wj32
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

#include <ph.h>

VOID NTAPI PhpFileStreamDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

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
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING bufferFileName;
    RTL_RELATIVE_NAME_U relativeName;
    UNICODE_STRING fileName;
    HANDLE rootDirectory;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    if (!FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    status = RtlDosPathNameToNtPathName_U(
        FileName,
        &bufferFileName,
        NULL,
        &relativeName
        );

    if (!NT_SUCCESS(status))
        return status;

    if (relativeName.RelativeName.Length != 0)
    {
        // We're using a relative name.
        fileName = relativeName.RelativeName;
        rootDirectory = relativeName.ContainingDirectory;
    }
    else
    {
        fileName = bufferFileName;
        rootDirectory = NULL;
    }

    InitializeObjectAttributes(
        &oa,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        rootDirectory,
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

    RtlFreeHeap(RtlProcessHeap(), 0, bufferFileName.Buffer);

    if (NT_SUCCESS(status))
        *FileHandle = fileHandle;

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
    HANDLE fileHandle;
    ULONG createOptions;
    ULONG flags;
    LARGE_INTEGER endOfFile;

    flags = 0;

    if (Flags & PH_FILE_STREAM_ASYNCHRONOUS)
    {
        createOptions = FILE_NON_DIRECTORY_FILE;
        flags |= PH_FILE_STREAM_ASYNCHRONOUS;
    }
    else
    {
        createOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;
    }

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        DesiredAccess,
        0,
        ShareAccess,
        CreateDisposition,
        createOptions
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &endOfFile)))
        return status;

    *FileStream = PhCreateFileStream2(
        fileHandle,
        flags,
        PAGE_SIZE
        );

    return status;
}

PPH_FILE_STREAM PhCreateFileStream2(
    __in HANDLE FileHandle,
    __in ULONG Flags,
    __in ULONG BufferLength
    )
{
    PPH_FILE_STREAM fileStream;
    PVOID buffer;

    buffer = PhAllocatePage(BufferLength, NULL);

    if (!buffer)
        PhRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);

    if (!NT_SUCCESS(PhCreateObject(
        &fileStream,
        sizeof(PH_FILE_STREAM),
        0,
        PhFileStreamType,
        0
        )))
        return NULL;

    fileStream->FileHandle = FileHandle;
    fileStream->Flags = Flags;
    fileStream->Position.QuadPart = 0;

    fileStream->Buffer = buffer;
    fileStream->BufferUsed = 0;
    fileStream->BufferLength = BufferLength;

    return fileStream;
}

VOID NTAPI PhpFileStreamDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_FILE_STREAM fileStream = (PPH_FILE_STREAM)Object;

    if (!(fileStream->Flags & PH_FILE_STREAM_HANDLE_UNOWNED))
        NtClose(fileStream->FileHandle);

    PhFreePage(fileStream->Buffer);
}

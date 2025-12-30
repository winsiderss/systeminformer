/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <kphuser.h>

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param FileAttributes File attributes applied if the file is created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does or does not exist.
 * \li \c FILE_SUPERSEDE If the file exists, replace it. Otherwise, create the file.
 * \li \c FILE_CREATE If the file exists, fail. Otherwise, create the file.
 * \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
 * \li \c FILE_OPEN_IF If the file exists, open it. Otherwise, create the file.
 * \li \c FILE_OVERWRITE If the file exists, open and overwrite it. Otherwise, fail.
 * \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it. Otherwise, create the file.
 * \param CreateOptions The options to apply when the file is opened or created.
 */
NTSTATUS PhCreateFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    )
{
    return PhCreateFileWin32Ex(
        FileHandle,
        FileName,
        DesiredAccess,
        NULL,
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
 * \param AllocationSize The initial allocation size if the file is being created, overwritten, or superseded.
 * \param FileAttributes File attributes applied if the file is created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does or does not exist.
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
 * \li \c FILE_OVERWRITTEN The file was overwritten because \c FILE_OVERWRITE or
 * \c FILE_OVERWRITE_IF was specified in \a CreateDisposition.
 * \li \c FILE_EXISTS The file was not opened because it already existed and \c FILE_CREATE was
 * specified in \a CreateDisposition.
 * \li \c FILE_DOES_NOT_EXIST The file was not opened because it did not exist and \c FILE_OPEN or
 * \c FILE_OVERWRITE was specified in \a CreateDisposition.
 */
NTSTATUS PhCreateFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS PhCreateFileWin32ExAlt(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_ ULONG CreateFlags,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _Out_opt_ PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    EXTENDED_CREATE_INFORMATION extendedInfo;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    memset(&extendedInfo, 0, sizeof(EXTENDED_CREATE_INFORMATION));
    extendedInfo.ExtendedCreateFlags = CreateFlags;

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions | FILE_CONTAINS_EXTENDED_CREATE_INFORMATION,
        &extendedInfo,
        sizeof(EXTENDED_CREATE_INFORMATION)
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions | FILE_CONTAINS_EXTENDED_CREATE_INFORMATION,
            &extendedInfo,
            sizeof(EXTENDED_CREATE_INFORMATION),
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param FileAttributes File attributes applied if the file is created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does or does not exist.
 * \li \c FILE_SUPERSEDE If the file exists, replace it. Otherwise, create the file.
 * \li \c FILE_CREATE If the file exists, fail. Otherwise, create the file.
 * \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
 * \li \c FILE_OPEN_IF If the file exists, open it. Otherwise, create the file.
 * \li \c FILE_OVERWRITE If the file exists, open and overwrite it. Otherwise, fail.
 * \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it. Otherwise, create the file.
 * \param CreateOptions The options to apply when the file is opened or created.
 * \return Successful or errant status.
 */
NTSTATUS PhCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            NULL,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    return status;
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param RootDirectory The root object directory for the file.
 * \param AllocationSize The initial allocation size if the file is being created, overwritten, or superseded.
 * \param FileAttributes File attributes applied if the file is created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does or does not exist.
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
 * \li \c FILE_OVERWRITTEN The file was overwritten because \c FILE_OVERWRITE or
 * \c FILE_OVERWRITE_IF was specified in \a CreateDisposition.
 * \li \c FILE_EXISTS The file was not opened because it already existed and \c FILE_CREATE was
 * specified in \a CreateDisposition.
 * \li \c FILE_DOES_NOT_EXIST The file was not opened because it did not exist and \c FILE_OPEN or
 * \c FILE_OVERWRITE was specified in \a CreateDisposition.
 * \return Successful or errant status.
 */
NTSTATUS PhCreateFileEx(
    _Out_ PHANDLE FileHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS PhOpenFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    )
{
    return PhOpenFileWin32Ex(
        FileHandle,
        FileName,
        DesiredAccess,
        ShareAccess,
        OpenOptions,
        NULL
        );
}

NTSTATUS PhOpenFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions,
    _Out_opt_ PULONG OpenStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        ShareAccess,
        OpenOptions
        );

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (OpenStatus)
        *OpenStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS PhOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions,
    _Out_opt_ PULONG OpenStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        ShareAccess,
        OpenOptions
        );

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (OpenStatus)
    {
        *OpenStatus = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

// rev from OpenFileById
NTSTATUS PhOpenFileById(
    _Out_ PHANDLE FileHandle,
    _In_ HANDLE VolumeHandle,
    _In_ PPH_FILE_ID_DESCRIPTOR FileId,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    switch (FileId->Type)
    {
    case FileIdType:
        {
            fileName.Length = sizeof(LONGLONG);
            fileName.MaximumLength = sizeof(LONGLONG);
            fileName.Buffer = (PWSTR)&FileId->FileId.QuadPart;
        }
        break;
    case ObjectIdType:
        {
            fileName.Length = sizeof(GUID);
            fileName.MaximumLength = sizeof(GUID);
            fileName.Buffer = (PWSTR)&FileId->ObjectId;
        }
        break;
    case ExtendedFileIdType:
        {
            fileName.Length = sizeof(FILE_ID_128);
            fileName.MaximumLength = sizeof(FILE_ID_128);
            fileName.Buffer = (PWSTR)&FileId->ExtendedFileId.Identifier;
        }
        break;
    default:
        return STATUS_UNSUCCESSFUL;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        VolumeHandle,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        ShareAccess,
        OpenOptions | FILE_OPEN_BY_FILE_ID
        );

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    return status;
}

// rev from ReOpenFile (dmex)
/**
 * Reopens the specified file handle with different access rights, sharing mode, and flags.
 * Note: This function creates new FILE_OBJECTs compared to other functions simply referencing the existing object.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param OriginalFileHandle A handle to the object to be reopened.
 * \param DesiredAccess The desired access to the file.
 * \param ShareAccess The file access granted to other threads.
 * \param OpenOptions The options to apply when the file is opened.
 * \return Successful or errant status.
 */
NTSTATUS PhReOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ HANDLE OriginalFileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlInitEmptyUnicodeString(&fileName, NULL, 0);
    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        OriginalFileHandle,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        0,
        ShareAccess,
        FILE_OPEN,
        OpenOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess)
    {
        assert(KPH_FILE_READ_DISPOSITION == FILE_OPEN);

        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            NULL,
            0,
            ShareAccess,
            FILE_OPEN,
            OpenOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    return status;
}

NTSTATUS PhReadFile(
    _In_ HANDLE FileHandle,
    _In_ PVOID Buffer,
    _In_opt_ ULONG NumberOfBytesToRead,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _Out_opt_ PULONG NumberOfBytesRead
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtReadFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        NumberOfBytesToRead,
        ByteOffset,
        NULL
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = isb.Status;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (NumberOfBytesRead)
        {
            *NumberOfBytesRead = (ULONG)isb.Information;
        }
    }

    return status;
}

NTSTATUS PhWriteFile(
    _In_ HANDLE FileHandle,
    _In_ PVOID Buffer,
    _In_opt_ ULONG NumberOfBytesToWrite,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _Out_opt_ PULONG NumberOfBytesWritten
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtWriteFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        NumberOfBytesToWrite,
        ByteOffset,
        NULL
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = isb.Status;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (NumberOfBytesWritten)
        {
            *NumberOfBytesWritten = (ULONG)isb.Information;
        }
    }

    return status;
}

NTSTATUS PhEnumDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ PCPH_STRINGREF SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    return PhEnumDirectoryFileEx(
        FileHandle,
        FileDirectoryInformation,
        FALSE,
        SearchPattern,
        Callback,
        Context
        );
}

NTSTATUS PhEnumDirectoryFileEx(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PCPH_STRINGREF SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    UNICODE_STRING searchPattern;
    PVOID buffer;
    ULONG bufferSize = 0x400;
    ULONG i;
    BOOLEAN cont;

    if (SearchPattern)
    {
        if (!PhStringRefToUnicodeString(SearchPattern, &searchPattern))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&searchPattern, NULL, 0);
    }

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        // Query the directory, doubling the buffer each time NtQueryDirectoryFile fails. (wj32)
        while (TRUE)
        {
            status = NtQueryDirectoryFile(
                FileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bufferSize,
                FileInformationClass,
                ReturnSingleEntry,
                &searchPattern,
                firstTime
                );

            // Our ISB is on the stack, so we have to wait for the operation to complete before continuing. (wj32)
            if (status == STATUS_PENDING)
            {
                status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

                if (NT_SUCCESS(status))
                    status = isb.Status;
            }

            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH)
            {
                PhFree(buffer);
                bufferSize *= 2;
                buffer = PhAllocate(bufferSize);
            }
            else
            {
                break;
            }
        }

        // If we don't have any entries to read, exit. (wj32)
        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        // Read the batch and execute the callback function for each file. (wj32)

        i = 0;
        cont = TRUE;

        while (TRUE)
        {
            CONST PFILE_DIRECTORY_NEXT_INFORMATION information = PTR_ADD_OFFSET(buffer, i);

            if (!Callback(FileHandle, information, Context))
            {
                cont = FALSE;
                break;
            }

            if (information->NextEntryOffset != 0)
                i += information->NextEntryOffset;
            else
                break;
        }

        if (!cont)
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The Win32 file name.
 * \param FileInformation A variable that receives the file information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryFullAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&objectAttributes, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The file name.
 * \param FileInformation A variable that receives the file information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryFullAttributesFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&objectAttributes, FileInformation);

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The Win32 file name.
 * \param FileInformation A variable that receives the file information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryAttributesFile(&objectAttributes, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The file name.
 * \param FileInformation A variable that receives the file information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryAttributesFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryAttributesFile(&objectAttributes, FileInformation);

    return status;
}

// rev from RtlDoesFileExists_U (dmex)
/**
 * Checks whether a file exists at the specified path using Win32 APIs.
 *
 * \param FileName A pointer to a null-terminated string that specifies the path to the file.
 * \return Returns TRUE if the file exists, otherwise FALSE.
 */
BOOLEAN PhDoesFileExistWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFileWin32(FileName, &basicInfo);

    if (
        NT_SUCCESS(status) ||
        status == STATUS_SHARING_VIOLATION ||
        status == STATUS_ACCESS_DENIED
        )
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * Checks whether a file exists at the specified path.
 *
 * \param FileName A pointer to a PH_STRINGREF structure that specifies the path of the file to check.
 * \return TRUE if the file exists, FALSE otherwise.
 */
BOOLEAN PhDoesFileExist(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFile(FileName, &basicInfo);

    if (
        NT_SUCCESS(status) ||
        status == STATUS_SHARING_VIOLATION ||
        status == STATUS_ACCESS_DENIED
        )
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * Checks whether a directory exists at the specified path.
 *
 * \param FileName The path of the directory to check.
 * \return TRUE if the directory exists, FALSE otherwise.
 */
BOOLEAN PhDoesDirectoryExistWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFileWin32(FileName, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return TRUE;
    }

    return FALSE;
}

/**
 * Checks whether a directory exists at the specified path.
 *
 * \param FileName The path of the directory to check.
 * \return TRUE if the directory exists, FALSE otherwise.
 */
BOOLEAN PhDoesDirectoryExist(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFile(FileName, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return TRUE;
    }

    return FALSE;
}

// rev from RtlDetermineDosPathNameType_U (dmex)
/**
 * Determines the type of DOS path name.
 *
 * \param FileName A reference to the file name string.
 * \return The type of the DOS path name.
 */
RTL_PATH_TYPE PhDetermineDosPathNameType(
    _In_ PCPH_STRINGREF FileName
    )
{
#if defined(PHNT_USE_NATIVE_PATHTYPE)
    return RtlDetermineDosPathNameType_U(FileName);
#else
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[0] == OBJ_NAME_ALTPATH_SEPARATOR)
    {
        if (FileName->Buffer[1] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[1] == OBJ_NAME_ALTPATH_SEPARATOR)
        {
            if (FileName->Buffer[2] == L'?' || FileName->Buffer[2] == L'.')
            {
                if (FileName->Buffer[3] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[3] == OBJ_NAME_ALTPATH_SEPARATOR)
                    return RtlPathTypeLocalDevice;

                if (FileName->Buffer[3] != UNICODE_NULL)
                    return RtlPathTypeUncAbsolute;

                return RtlPathTypeRootLocalDevice;
            }

            return RtlPathTypeUncAbsolute;
        }

        return RtlPathTypeRooted;
    }
    else if (FileName->Buffer[0] != UNICODE_NULL && FileName->Buffer[1] == L':')
    {
        if (FileName->Buffer[2] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[2] == OBJ_NAME_ALTPATH_SEPARATOR)
            return RtlPathTypeDriveAbsolute;

        return RtlPathTypeDriveRelative;
    }

    return RtlPathTypeRelative;
#endif
}

/**
 * Deletes a file.
 *
 * \param FileName The Win32 file name.
 */
NTSTATUS PhDeleteFileWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
#if defined(PHNT_USE_NATIVE_DELETE)
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtDeleteFile(&objectAttributes);

    RtlFreeUnicodeString(&fileName);

    if (!NT_SUCCESS(status))
#endif
    {
        HANDLE fileHandle;

        status = PhCreateFileWin32(
            &fileHandle,
            FileName,
            DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE
            );

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                status = STATUS_SUCCESS;
            return status;
        }

        //PhSetFileDelete(fileHandle);

        NtClose(fileHandle);
    }

    return status;
}

/**
 * Deletes a file.
 *
 * \param FileName The file name.
 */
NTSTATUS PhDeleteFile(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
#if defined(PHNT_USE_NATIVE_DELETE)
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtDeleteFile(&objectAttributes);

    RtlFreeUnicodeString(&fileName);

    if (!NT_SUCCESS(status))
#endif
    {
        HANDLE fileHandle;

        status = PhCreateFile(
            &fileHandle,
            FileName,
            DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE
            );

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                status = STATUS_SUCCESS;
            return status;
        }

        NtClose(fileHandle);
    }

    return status;
}

/**
 * Creates a directory path recursively.
 *
 * \param DirectoryPath The directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateDirectory(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    PPH_STRING directoryPath;
    PPH_STRING directoryName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;

    if (PhDoesDirectoryExist(DirectoryPath))
        return STATUS_SUCCESS;

    directoryPath = PhGetExistingPathPrefix(DirectoryPath);

    if (PhIsNullOrEmptyString(directoryPath))
        return STATUS_UNSUCCESSFUL;

    remainingPart.Length = DirectoryPath->Length - directoryPath->Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    remainingPart.Buffer = PTR_ADD_OFFSET(DirectoryPath->Buffer, directoryPath->Length + sizeof(OBJ_NAME_PATH_SEPARATOR));

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            directoryName = PhConcatStringRef3(
                &directoryPath->sr,
                &PhNtPathSeparatorString,
                &directoryPart
                );

            if (!PhDoesDirectoryExist(&directoryName->sr))
            {
                HANDLE directoryHandle;

                if (NT_SUCCESS(PhCreateFile(
                    &directoryHandle,
                    &directoryName->sr,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_CREATE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT //| FILE_OPEN_REPARSE_POINT
                    )))
                {
                    NtClose(directoryHandle);
                }
            }

            PhMoveReference(&directoryPath, directoryName);
        }
    }

    PhClearReference(&directoryPath);

    if (!PhDoesDirectoryExist(DirectoryPath))
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}

/**
 * Creates a directory path recursively.
 *
 * \param DirectoryPath The Win32 directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateDirectoryWin32(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    PPH_STRING directoryPath;
    PPH_STRING directoryName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;

    if (PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_SUCCESS;

    directoryPath = PhGetExistingPathPrefixWin32(DirectoryPath);

    if (PhIsNullOrEmptyString(directoryPath))
        return STATUS_UNSUCCESSFUL;

    remainingPart.Length = DirectoryPath->Length - directoryPath->Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    remainingPart.Buffer = PTR_ADD_OFFSET(DirectoryPath->Buffer, directoryPath->Length + sizeof(OBJ_NAME_PATH_SEPARATOR));

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            if (PhIsNullOrEmptyString(directoryPath))
            {
                PhMoveReference(&directoryPath, PhCreateString2(&directoryPart));
            }
            else
            {
                directoryName = PhConcatStringRef3(&directoryPath->sr, &PhNtPathSeparatorString, &directoryPart);

                // Check if the directory already exists. (dmex)

                if (!PhDoesDirectoryExistWin32(PhGetString(directoryName)))
                {
                    HANDLE directoryHandle;

                    // Create the directory. (dmex)

                    if (NT_SUCCESS(PhCreateFileWin32(
                        &directoryHandle,
                        PhGetString(directoryName),
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT //| FILE_OPEN_REPARSE_POINT
                        )))
                    {
                        NtClose(directoryHandle);
                    }
                }

                PhMoveReference(&directoryPath, directoryName);
            }
        }
    }

    PhClearReference(&directoryPath);

    if (!PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}

/**
 * Creates a directory path recursively.
 *
 * \param DirectoryPath The Win32 directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateDirectoryFullPathWin32(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PH_STRINGREF directoryPart;
    PPH_STRING directoryPath;
    PPH_STRING directory;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        if (directory = PhCreateString2(&directoryPart))
        {
            if (NT_SUCCESS(PhGetFullPath(PhGetString(directory), &directoryPath, NULL)))
            {
                status = PhCreateDirectoryWin32(&directoryPath->sr);
                PhDereferenceObject(directoryPath);
            }

            PhDereferenceObject(directory);
        }
    }

    return status;
}

/**
 * Creates a directory path recursively.
 *
 * \param DirectoryPath The directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateDirectoryFullPath(
    _In_ PCPH_STRINGREF FileName
    )
{
    PH_STRINGREF directoryPart;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        return PhCreateDirectory(&directoryPart);
    }

    return STATUS_UNSUCCESSFUL;
}

// NOTE: This callback handles both Native and Win32 filenames
// since they're both relative to the parent RootDirectory. (dmex)
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN PhDeleteDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF fileName;
    HANDLE fileHandle;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
            return TRUE;

        if (NT_SUCCESS(PhCreateFileEx(
            &fileHandle,
            &fileName,
            FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
            RootDirectory,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, // | FILE_OPEN_REPARSE_POINT
            NULL
            )))
        {
            PhEnumDirectoryFile(fileHandle, NULL, PhDeleteDirectoryCallback, Context);

            PhSetFileDelete(fileHandle);

            NtClose(fileHandle);
        }
    }
    else
    {
        if (NT_SUCCESS(PhCreateFileEx(
            &fileHandle,
            &fileName,
            FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE,
            RootDirectory,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, // | FILE_OPEN_REPARSE_POINT
            NULL
            )))
        {
            if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_READONLY) && WindowsVersion < WINDOWS_10_RS5)
            {
                FILE_BASIC_INFORMATION fileBasicInfo;

                memset(&fileBasicInfo, 0, sizeof(FILE_BASIC_INFORMATION));
                fileBasicInfo.FileAttributes = ClearFlag(Information->FileAttributes, FILE_ATTRIBUTE_READONLY);

                PhSetFileBasicInformation(fileHandle, &fileBasicInfo);
            }

            PhSetFileDelete(fileHandle);

            NtClose(fileHandle);
        }
    }

    return TRUE;
}

/**
 * Deletes a directory path recursively.
 *
 * \param DirectoryPath The directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDeleteDirectory(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFile(
        &directoryHandle,
        DirectoryPath,
        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        // Remove any files or folders inside the directory. (dmex)
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            PhDeleteDirectoryCallback,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // Remove the directory. (dmex)
            status = PhSetFileDelete(directoryHandle);
        }

        NtClose(directoryHandle);
    }

    if (!PhDoesDirectoryExist(DirectoryPath))
        return STATUS_SUCCESS;

    return status;
}

/**
 * Deletes a directory path recursively.
 *
 * \param DirectoryPath The Win32 directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDeleteDirectoryWin32(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFileWin32(
        &directoryHandle,
        PhGetStringRefZ(DirectoryPath),
        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        // Remove any files or folders inside the directory. (dmex)
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            PhDeleteDirectoryCallback,
            (PVOID)DirectoryPath
            );

        if (NT_SUCCESS(status))
        {
            // Remove the directory. (dmex)
            status = PhSetFileDelete(directoryHandle);
        }

        NtClose(directoryHandle);
    }

    if (!PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_SUCCESS;

    return status;
}

/**
 * Deletes a directory path recursively.
 *
 * \param DirectoryPath The directory path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDeleteDirectoryFullPath(
    _In_ PCPH_STRINGREF FileName
    )
{
    PH_STRINGREF directoryPart;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        return PhDeleteDirectory(&directoryPart);
    }

    return STATUS_UNSUCCESSFUL;
}

/**
 * Copies a file from OldFileName to NewFileName using Win32 APIs.
 *
 * \param OldFileName The path to the source file to copy.
 * \param NewFileName The path to the destination file.
 * \param FailIfExists If TRUE, the function fails if the destination file already exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCopyFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE newFileHandle;
    FILE_BASIC_INFORMATION basicInfo;
    LARGE_INTEGER newFileSize;
    IO_STATUS_BLOCK isb;
    ULONG bufferLength;
    PBYTE buffer;

    status = PhCreateFileWin32(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(fileHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(fileHandle, &newFileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32Ex(
        &newFileHandle,
        NewFileName,
        FILE_GENERIC_WRITE,
        &newFileSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    bufferLength = PAGE_SIZE * 2;
    buffer = PhAllocatePage(bufferLength, NULL);

    if (!buffer)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    while (TRUE)
    {
        status = NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            bufferLength,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;

        status = NtWriteFile(
            newFileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            (ULONG)isb.Information,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;
    }

    PhFreePage(buffer);

    if (status == STATUS_END_OF_FILE)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        PhSetFileBasicInformation(
            newFileHandle,
            &basicInfo
            );
    }
    else
    {
        PhSetFileDelete(newFileHandle);
    }

    NtClose(newFileHandle);

CleanupExit:
    NtClose(fileHandle);

    return status;
}

/**
 * Copies a file from OldFileName to NewFileName using file chunk APIs.
 *
 * \param OldFileName The path to the source file to copy.
 * \param NewFileName The path to the destination file.
 * \param FailIfExists If TRUE, the function fails if the destination file already exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCopyFileChunkDirectIoWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE sourceHandle;
    HANDLE destinationHandle;
    FILE_BASIC_INFORMATION basicInfo;
    FILE_FS_SECTOR_SIZE_INFORMATION sourceSectorInfo = { 0 };
    FILE_FS_SECTOR_SIZE_INFORMATION destinationSectorInfo = { 0 };
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER sourceOffset = { 0 };
    LARGE_INTEGER destinationOffset = { 0 };
    LARGE_INTEGER fileSize;
    SIZE_T numberOfBytes;
    ULONG alignSize;
    ULONG blockSize;

    if (!NtCopyFileChunk_Import())
        return STATUS_NOT_SUPPORTED;

    status = PhCreateFileWin32ExAlt(
        &sourceHandle,
        OldFileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
        EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(sourceHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(sourceHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32ExAlt(
        &destinationHandle,
        NewFileName,
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
        EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY,
        &fileSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // https://learn.microsoft.com/en-us/windows/win32/w8cookbook/advanced-format--4k--disk-compatibility-update
    NtQueryVolumeInformationFile(
        sourceHandle,
        &ioStatusBlock,
        &sourceSectorInfo,
        sizeof(FILE_FS_SECTOR_SIZE_INFORMATION),
        FileFsSectorSizeInformation
        );

    NtQueryVolumeInformationFile(
        destinationHandle,
        &ioStatusBlock,
        &destinationSectorInfo,
        sizeof(FILE_FS_SECTOR_SIZE_INFORMATION),
        FileFsSectorSizeInformation
        );

    // Non-cached I/O requires 'blockSize' be sector-aligned with whichever file is opened as non-cached.
    // If both, the length should be aligned with the larger sector size of the two. (dmex)
    alignSize = __max(max(sourceSectorInfo.PhysicalBytesPerSectorForPerformance, destinationSectorInfo.PhysicalBytesPerSectorForPerformance),
        max(sourceSectorInfo.PhysicalBytesPerSectorForAtomicity, destinationSectorInfo.PhysicalBytesPerSectorForAtomicity));

    // Enable BypassIO (skip error checking since might be disabled) (dmex)
    PhSetFileBypassIO(sourceHandle, TRUE);
    PhSetFileBypassIO(destinationHandle, TRUE);

    blockSize = PAGE_SIZE;
    numberOfBytes = (SIZE_T)fileSize.QuadPart;

    while (numberOfBytes != 0)
    {
        if (blockSize > numberOfBytes)
            blockSize = (ULONG)numberOfBytes;
        blockSize = ALIGN_UP_BY(blockSize, alignSize);

        status = NtCopyFileChunk_Import()(
            sourceHandle,
            destinationHandle,
            NULL,
            &ioStatusBlock,
            blockSize,
            &sourceOffset,
            &destinationOffset,
            NULL,
            NULL,
            0
            );

        if (!NT_SUCCESS(status))
            break;

        destinationOffset.QuadPart += blockSize;
        sourceOffset.QuadPart += blockSize;
        numberOfBytes -= blockSize;
    }

    if (status == STATUS_END_OF_FILE)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        status = PhSetFileSize(destinationHandle, &fileSize); // Required (dmex)
    }

    if (NT_SUCCESS(status))
    {
        status = PhSetFileBasicInformation(destinationHandle, &basicInfo);
    }

    if (!NT_SUCCESS(status))
    {
        PhSetFileDelete(destinationHandle);
    }

    NtClose(destinationHandle);

CleanupExit:
    NtClose(sourceHandle);

    return status;
}

/**
 * Copies a file from OldFileName to NewFileName using Win32 APIs, potentially in chunks.
 *
 * \param OldFileName The path to the source file to copy.
 * \param NewFileName The path to the destination file.
 * \param FailIfExists If TRUE, the function fails if the destination file already exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCopyFileChunkWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE sourceHandle;
    HANDLE destinationHandle;
    FILE_BASIC_INFORMATION basicInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER sourceOffset = { 0 };
    LARGE_INTEGER destinationOffset = { 0 };
    LARGE_INTEGER fileSize;

    if (!NtCopyFileChunk_Import())
    {
        return PhCopyFileWin32(OldFileName, NewFileName, FailIfExists);
    }

    status = PhCreateFileWin32ExAlt(
        &sourceHandle,
        OldFileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(sourceHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(sourceHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32ExAlt(
        &destinationHandle,
        NewFileName,
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY,
        &fileSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (fileSize.QuadPart >= ULONG_MAX)
    {
        SIZE_T numberOfBytes = (SIZE_T)fileSize.QuadPart;
        ULONG blockSize = ULONG_MAX;

        // Split into smaller blocks when the length
        // overflows the maximum chunk size. (dmex)

        while (numberOfBytes != 0)
        {
            if (blockSize > numberOfBytes)
                blockSize = (ULONG)numberOfBytes;

            status = NtCopyFileChunk_Import()(
                sourceHandle,
                destinationHandle,
                NULL,
                &ioStatusBlock,
                blockSize,
                &sourceOffset,
                &destinationOffset,
                NULL,
                NULL,
                0
                );

            if (!NT_SUCCESS(status))
                break;

            destinationOffset.QuadPart += blockSize;
            sourceOffset.QuadPart += blockSize;
            numberOfBytes -= blockSize;
        }
    }
    else
    {
        status = NtCopyFileChunk_Import()(
            sourceHandle,
            destinationHandle,
            NULL,
            &ioStatusBlock,
            (ULONG)fileSize.QuadPart,
            &sourceOffset,
            &destinationOffset,
            NULL,
            NULL,
            0
            );
    }

    if (NT_SUCCESS(status))
    {
        PhSetFileBasicInformation(
            destinationHandle,
            &basicInfo
            );
    }
    else
    {
        PhSetFileDelete(destinationHandle);
    }

    NtClose(destinationHandle);

CleanupExit:
    NtClose(sourceHandle);

    return status;
}

/**
 * Moves a file from OldFileName to NewFileName.
 *
 * \param OldFileName Pointer to a PH_STRINGREF structure containing the source file name.
 * \param NewFileName Pointer to a PH_STRINGREF structure containing the destination file name.
 * \param FailIfExists If TRUE, the operation fails if the destination file already exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhMoveFile(
    _In_ PCPH_STRINGREF OldFileName,
    _In_ PCPH_STRINGREF NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    ULONG fileNameLength;
    ULONG renameInfoLength;
    PFILE_RENAME_INFORMATION renameInfo = NULL;

    status = PhCreateFile(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = RtlSIZETToULong(NewFileName->Length, &fileNameLength);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + fileNameLength + sizeof(UNICODE_NULL);
    renameInfo = PhAllocateStack(renameInfoLength);
    if (!renameInfo) return STATUS_NO_MEMORY;
    memset(renameInfo, 0, renameInfoLength);
    renameInfo->ReplaceIfExists = FailIfExists ? FALSE : TRUE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = fileNameLength;
    memcpy(renameInfo->FileName, NewFileName->Buffer, fileNameLength);

    status = NtSetInformationFile(
        fileHandle,
        &isb,
        renameInfo,
        renameInfoLength,
        FileRenameInformation
        );

    if (status == STATUS_NOT_SAME_DEVICE)
    {
        HANDLE newFileHandle;
        LARGE_INTEGER newFileSize;
        ULONG bufferLength;
        PBYTE buffer;

        status = PhGetFileSize(fileHandle, &newFileSize);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhCreateFileEx(
            &newFileHandle,
            NewFileName,
            FILE_GENERIC_WRITE,
            NULL,
            &newFileSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            bufferLength = PAGE_SIZE * 2;
            buffer = PhAllocatePage(bufferLength, NULL);

            if (!buffer)
            {
                status = STATUS_NO_MEMORY;
                goto CleanupExit;
            }

            while (TRUE)
            {
                status = NtReadFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    bufferLength,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;

                status = NtWriteFile(
                    newFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    (ULONG)isb.Information,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;
            }

            PhFreePage(buffer);

            if (status == STATUS_END_OF_FILE)
            {
                status = STATUS_SUCCESS;
            }

            if (status != STATUS_SUCCESS)
            {
                PhSetFileDelete(newFileHandle);
            }

            NtClose(newFileHandle);
        }
    }

CleanupExit:
    NtClose(fileHandle);
    PhFreeStack(renameInfo);

    return status;
}

/**
 * Moves a file from OldFileName to NewFileName using Win32 semantics.
 *
 * \param OldFileName The path to the existing file to be moved.
 * \param NewFileName The destination path for the moved file.
 * \param FailIfExists If TRUE, the operation fails if NewFileName already exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhMoveFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    ULONG renameInfoLength;
    UNICODE_STRING newFileName;
    PFILE_RENAME_INFORMATION renameInfo;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        NewFileName,
        &newFileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreateFileWin32(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&newFileName);
        return status;
    }

    renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + newFileName.Length + sizeof(UNICODE_NULL);
    renameInfo = PhAllocateStack(renameInfoLength);
    if (!renameInfo) return STATUS_NO_MEMORY;
    memset(renameInfo, 0, renameInfoLength);
    renameInfo->ReplaceIfExists = FailIfExists ? FALSE : TRUE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = newFileName.Length;
    memcpy(renameInfo->FileName, newFileName.Buffer, newFileName.Length);
    RtlFreeUnicodeString(&newFileName);

    status = NtSetInformationFile(
        fileHandle,
        &isb,
        renameInfo,
        renameInfoLength,
        FileRenameInformation
        );

    if (status == STATUS_NOT_SAME_DEVICE)
    {
        HANDLE newFileHandle;
        LARGE_INTEGER newFileSize;
        PBYTE buffer;

        status = PhGetFileSize(fileHandle, &newFileSize);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhCreateFileWin32Ex(
            &newFileHandle,
            NewFileName,
            FILE_GENERIC_WRITE,
            &newFileSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            buffer = PhAllocatePage(PAGE_SIZE * 2, NULL);

            if (!buffer)
            {
                status = STATUS_NO_MEMORY;
                goto CleanupExit;
            }

            while (TRUE)
            {
                status = NtReadFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    PAGE_SIZE * 2,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;

                status = NtWriteFile(
                    newFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    (ULONG)isb.Information,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;
            }

            PhFreePage(buffer);

            if (status == STATUS_END_OF_FILE)
            {
                status = STATUS_SUCCESS;
            }

            if (status != STATUS_SUCCESS)
            {
                PhSetFileDelete(newFileHandle);
            }

            NtClose(newFileHandle);
        }
    }

CleanupExit:
    NtClose(fileHandle);
    PhFreeStack(renameInfo);

    return status;
}

/**
 * \brief Enumerates the volume Reparse Points using the \\$Extend\\$Reparse:$R:$INDEX_ALLOCATION directory.
 *
 * \param FileHandle A handle to the volume.
 * \param Callback A pointer to a callback function.
 * \param Context A user-defined value to pass to the callback function.
 * \return Successful or errant status
 */
NTSTATUS PhEnumReparsePointInformation(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_REPARSE_POINT Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = sizeof(FILE_REPARSE_POINT_INFORMATION[512]);
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryDirectoryFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            bufferSize,
            FileReparsePointInformation,
            FALSE,
            NULL,
            firstTime
            );

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

            if (NT_SUCCESS(status))
                status = isb.Status;
        }

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        status = Callback(FileHandle, buffer, isb.Information, Context);

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

/**
 * \brief Enumerates the volume ObjectIDs using the \\$Extend\\$ObjId:$O:$INDEX_ALLOCATION directory.
 *
 * \param FileHandle A handle to the volume.
 * \param Callback A pointer to a callback function.
 * \param Context A user-defined value to pass to the callback function.
 * \return Successful or errant status
 */
NTSTATUS PhEnumObjectIdInformation(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_OBJECT_ID Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = sizeof(FILE_OBJECTID_INFORMATION[128]);
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryDirectoryFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            bufferSize,
            FileObjectIdInformation,
            FALSE,
            NULL,
            firstTime
            );

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

            if (NT_SUCCESS(status))
                status = isb.Status;
        }

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        status = Callback(FileHandle, buffer, isb.Information, Context);

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhEnumFileExtendedAttributes(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_FILE_EA Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    BOOLEAN firstTime = TRUE;
    IO_STATUS_BLOCK isb;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = 0x400;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        while (TRUE)
        {
            status = NtQueryEaFile(
                FileHandle,
                &isb,
                buffer,
                bufferSize,
                FALSE,
                NULL,
                0,
                NULL,
                firstTime
                );

            if (status == STATUS_PENDING)
            {
                status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

                if (NT_SUCCESS(status))
                    status = isb.Status;
            }

            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH)
            {
                PhFree(buffer);
                bufferSize *= 2;
                buffer = PhAllocate(bufferSize);
            }
            else
            {
                break;
            }
        }

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        status = Callback(FileHandle, buffer, Context);

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhSetFileExtendedAttributes(
    _In_ HANDLE FileHandle,
    _In_ PPH_BYTESREF Name,
    _In_opt_ PPH_BYTESREF Value
    )
{
    NTSTATUS status;
    ULONG infoLength;
    PFILE_FULL_EA_INFORMATION info;
    IO_STATUS_BLOCK isb;

    infoLength = sizeof(FILE_FULL_EA_INFORMATION) + (ULONG)Name->Length + sizeof(ANSI_NULL);
    if (Value) infoLength += (ULONG)Value->Length + sizeof(ANSI_NULL);

    info = PhAllocateZero(infoLength);
    info->EaNameLength = (UCHAR)Name->Length;
    memcpy(
        info->EaName,
        Name->Buffer,
        Name->Length
        );

    if (Value)
    {
        info->EaValueLength = (USHORT)Value->Length;
        memcpy(
            PTR_ADD_OFFSET(info->EaName, info->EaNameLength + sizeof(ANSI_NULL)),
            Value->Buffer,
            Value->Length
            );
    }

    status = NtSetEaFile(
        FileHandle,
        &isb,
        info,
        infoLength
        );

    PhFree(info);

    return status;
}

NTSTATUS PhpQueryFileVariableSize(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationFile(
            FileHandle,
            &isb,
            buffer,
            bufferSize,
            FileInformationClass
            );

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            bufferSize *= 2;
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhEnumFileStreams(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *Streams
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileStreamInformation,
        Streams
        );
}

NTSTATUS PhEnumFileHardLinks(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *HardLinks
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileHardLinkInformation,
        HardLinks
        );
}

/**
 * Queries information about the volume associated with a given file, directory, storage device, or volume.
 *
 * \param ProcessHandle A handle to a process.
 * \param FileHandle A handle to the volume.
 * \param FsInformationClass Type of information to be returned about the volume.
 * \param FsInformation A pointer to a caller-allocated buffer.
 * \param FsInformationLength Size in bytes of the buffer pointed to by FsInformation.
 * \param IoStatusBlock A pointer to an IO_STATUS_BLOCK structure that receives the final completion status and information about the query operation.
 */
NTSTATUS PhQueryVolumeInformationFile(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status;

    if (ProcessHandle)
    {
        if (KsiLevel() >= KphLevelMed)
        {
            status = KphQueryVolumeInformationFile(
                ProcessHandle,
                FileHandle,
                FsInformationClass,
                FsInformation,
                FsInformationLength,
                IoStatusBlock
                );
        }
        else if (ProcessHandle == NtCurrentProcess())
        {
            status = NtQueryVolumeInformationFile(
                FileHandle,
                IoStatusBlock,
                FsInformation,
                FsInformationLength,
                FsInformationClass
                );
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        status = NtQueryVolumeInformationFile(
            FileHandle,
            IoStatusBlock,
            FsInformation,
            FsInformationLength,
            FsInformationClass
            );
    }

    return status;
}

NTSTATUS PhGetFileBasicInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_BASIC_INFORMATION BasicInfo
    )
{
    IO_STATUS_BLOCK isb;

    return NtQueryInformationFile(
        FileHandle,
        &isb,
        BasicInfo,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation
        );
}

NTSTATUS PhSetFileBasicInformation(
    _In_ HANDLE FileHandle,
    _In_ PFILE_BASIC_INFORMATION BasicInfo
    )
{
    IO_STATUS_BLOCK isb;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        BasicInfo,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation
        );
}

NTSTATUS PhGetFileFullAttributesInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_NETWORK_OPEN_INFORMATION fullAttributesInfo;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &fullAttributesInfo,
        sizeof(FILE_NETWORK_OPEN_INFORMATION),
        FileNetworkOpenInformation
        );

    if (NT_SUCCESS(status))
    {
        *FileInformation = fullAttributesInfo;
    }

    return status;
}

NTSTATUS PhGetFileStandardInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_STANDARD_INFORMATION StandardInfo
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_STANDARD_INFORMATION standardInfo;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &standardInfo,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    if (NT_SUCCESS(status))
    {
        *StandardInfo = standardInfo;
    }

    return status;
}

// rev from SetFileCompletionNotificationModes (dmex)
NTSTATUS PhSetFileCompletionNotificationMode(
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
    )
{
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION completionMode;
    IO_STATUS_BLOCK isb;

    completionMode.Flags = Flags;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &completionMode,
        sizeof(FILE_IO_COMPLETION_NOTIFICATION_INFORMATION),
        FileIoCompletionNotificationInformation
        );
}

NTSTATUS PhGetFileSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Size
    )
{
    NTSTATUS status;
    FILE_STANDARD_INFORMATION standardInfo;

    status = PhGetFileStandardInformation(
        FileHandle,
        &standardInfo
        );

    if (NT_SUCCESS(status))
    {
        *Size = standardInfo.EndOfFile;
    }

    return status;
}

NTSTATUS PhSetFileSize(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER Size
    )
{
    FILE_END_OF_FILE_INFORMATION endOfFileInfo;
    IO_STATUS_BLOCK isb;

    endOfFileInfo.EndOfFile = *Size;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &endOfFileInfo,
        sizeof(FILE_END_OF_FILE_INFORMATION),
        FileEndOfFileInformation
        );
}

NTSTATUS PhGetFilePosition(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Position
    )
{
    NTSTATUS status;
    FILE_POSITION_INFORMATION positionInfo;
    IO_STATUS_BLOCK isb;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &positionInfo,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation
        );

    if (!NT_SUCCESS(status))
        return status;

    *Position = positionInfo.CurrentByteOffset;

    return status;
}

NTSTATUS PhSetFilePosition(
    _In_ HANDLE FileHandle,
    _In_opt_ PLARGE_INTEGER Position
    )
{
    FILE_POSITION_INFORMATION positionInfo;
    IO_STATUS_BLOCK isb;

    if (Position)
        positionInfo.CurrentByteOffset.QuadPart = Position->QuadPart;
    else
        positionInfo.CurrentByteOffset.QuadPart = 0;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &positionInfo,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation
        );
}

NTSTATUS PhGetFileAllocationSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER AllocationSize
    )
{
    NTSTATUS status;
    FILE_ALLOCATION_INFORMATION allocationInfo;
    IO_STATUS_BLOCK isb;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &allocationInfo,
        sizeof(FILE_ALLOCATION_INFORMATION),
        FileAllocationInformation
        );

    if (!NT_SUCCESS(status))
        return status;

    *AllocationSize = allocationInfo.AllocationSize;

    return status;
}

NTSTATUS PhSetFileAllocationSize(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER AllocationSize
    )
{
    FILE_ALLOCATION_INFORMATION allocationInfo;
    IO_STATUS_BLOCK isb;

    allocationInfo.AllocationSize = *AllocationSize;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &allocationInfo,
        sizeof(FILE_ALLOCATION_INFORMATION),
        FileAllocationInformation
        );
}

NTSTATUS PhGetFileIndexNumber(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_INTERNAL_INFORMATION IndexNumber
    )
{
    IO_STATUS_BLOCK isb;

    return NtQueryInformationFile(
        FileHandle,
        &isb,
        IndexNumber,
        sizeof(FILE_INTERNAL_INFORMATION),
        FileInternalInformation
        );
}

NTSTATUS PhGetFileIsRemoteDevice(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN FileIsRemoteDevice
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_IS_REMOTE_DEVICE_INFORMATION fileRemoteInfo;

    status = NtQueryInformationFile(
        FileHandle,
        &ioStatusBlock,
        &fileRemoteInfo,
        sizeof(FILE_IS_REMOTE_DEVICE_INFORMATION),
        FileIsRemoteDeviceInformation
        );

    if (NT_SUCCESS(status))
    {
        *FileIsRemoteDevice = !!fileRemoteInfo.IsRemote;
    }

    return status;
}

NTSTATUS PhSetFileDelete(
    _In_ HANDLE FileHandle
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        FILE_DISPOSITION_INFO_EX dispositionInfo;
        IO_STATUS_BLOCK ioStatusBlock;

        dispositionInfo.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS | FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE;
        status = NtSetInformationFile(
            FileHandle,
            &ioStatusBlock,
            &dispositionInfo,
            sizeof(FILE_DISPOSITION_INFO_EX),
            FileDispositionInformationEx
            );
    }

    if (!NT_SUCCESS(status))
    {
        FILE_DISPOSITION_INFORMATION dispositionInfo;
        IO_STATUS_BLOCK ioStatusBlock;

        dispositionInfo.DeleteFile = TRUE;
        status = NtSetInformationFile(
            FileHandle,
            &ioStatusBlock,
            &dispositionInfo,
            sizeof(FILE_DISPOSITION_INFORMATION),
            FileDispositionInformation
            );
    }

    //if (!NT_SUCCESS(status))
    //{
    //    HANDLE deleteHandle;
    //
    //    if (NT_SUCCESS(PhReOpenFile(
    //        &deleteHandle,
    //        FileHandle,
    //        DELETE,
    //        FILE_SHARE_DELETE,
    //        FILE_DELETE_ON_CLOSE
    //        )))
    //    {
    //        NtClose(deleteHandle);
    //        status = STATUS_SUCCESS;
    //    }
    //}

    return status;
}

NTSTATUS PhSetFileRename(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE RootDirectory,
    _In_ BOOLEAN ReplaceIfExists,
    _In_ PCPH_STRINGREF NewFileName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (WindowsVersion >= WINDOWS_10_RS2)
    {
        PFILE_RENAME_INFORMATION_EX renameInfo;
        IO_STATUS_BLOCK ioStatusBlock;
        ULONG renameInfoLength;

        renameInfoLength = sizeof(FILE_RENAME_INFORMATION_EX) + (ULONG)NewFileName->Length + sizeof(UNICODE_NULL);
        renameInfo = PhAllocateStack(renameInfoLength);

        if (renameInfo)
        {
            RtlZeroMemory(renameInfo, renameInfoLength);
            renameInfo->Flags = (ReplaceIfExists ? FILE_RENAME_REPLACE_IF_EXISTS : 0) | FILE_RENAME_POSIX_SEMANTICS;
            renameInfo->RootDirectory = RootDirectory;
            renameInfo->FileNameLength = (ULONG)NewFileName->Length;
            RtlCopyMemory(renameInfo->FileName, NewFileName->Buffer, NewFileName->Length);

            if (WindowsVersion >= WINDOWS_10_RS5)
            {
                SetFlag(renameInfo->Flags, FILE_RENAME_IGNORE_READONLY_ATTRIBUTE);
            }
            else
            {
                FILE_BASIC_INFORMATION basicInfo;

                RtlZeroMemory(&basicInfo, sizeof(FILE_BASIC_INFORMATION));
                basicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

                PhSetFileBasicInformation(FileHandle, &basicInfo);
            }

            status = NtSetInformationFile(
                FileHandle,
                &ioStatusBlock,
                renameInfo,
                renameInfoLength,
                FileRenameInformationEx
                );

            PhFreeStack(renameInfo);
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PFILE_RENAME_INFORMATION renameInfo;
        IO_STATUS_BLOCK ioStatusBlock;
        ULONG renameInfoLength;

        renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + (ULONG)NewFileName->Length + sizeof(UNICODE_NULL);
        renameInfo = PhAllocateStack(renameInfoLength);

        if (renameInfo)
        {
            RtlZeroMemory(renameInfo, renameInfoLength);
            renameInfo->ReplaceIfExists = ReplaceIfExists;
            renameInfo->RootDirectory = RootDirectory;
            renameInfo->FileNameLength = (ULONG)NewFileName->Length;
            RtlCopyMemory(renameInfo->FileName, NewFileName->Buffer, NewFileName->Length);

            {
                FILE_BASIC_INFORMATION basicInfo;

                RtlZeroMemory(&basicInfo, sizeof(FILE_BASIC_INFORMATION));
                basicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

                PhSetFileBasicInformation(FileHandle, &basicInfo);
            }

            status = NtSetInformationFile(
                FileHandle,
                &ioStatusBlock,
                renameInfo,
                renameInfoLength,
                FileRenameInformation
                );

            PhFreeStack(renameInfo);
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }

    return status;
}

NTSTATUS PhGetFileIoPriorityHint(
    _In_ HANDLE FileHandle,
    _Out_ IO_PRIORITY_HINT* IoPriorityHint
    )
{
    NTSTATUS status;
    FILE_IO_PRIORITY_HINT_INFORMATION ioPriorityHint;
    IO_STATUS_BLOCK ioStatusBlock;

    status = NtQueryInformationFile(
        FileHandle,
        &ioStatusBlock,
        &ioPriorityHint,
        sizeof(FILE_IO_PRIORITY_HINT_INFORMATION),
        FileIoPriorityHintInformation
        );

    if (!NT_SUCCESS(status))
        return status;

    *IoPriorityHint = ioPriorityHint.PriorityHint;

    return status;
}

NTSTATUS PhSetFileIoPriorityHint(
    _In_ HANDLE FileHandle,
    _In_ IO_PRIORITY_HINT IoPriorityHint
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
#ifndef _WIN64
    FILE_IO_PRIORITY_HINT_INFORMATION ioPriorityHint;
#else
    FILE_IO_PRIORITY_HINT_INFORMATION_EX ioPriorityHint;
#endif
    memset(&ioPriorityHint, 0, sizeof(ioPriorityHint));

    ioPriorityHint.PriorityHint = IoPriorityHint;

    return NtSetInformationFile(
        FileHandle,
        &ioStatusBlock,
        &ioPriorityHint,
        sizeof(ioPriorityHint),
        FileIoPriorityHintInformation
        );
}

/**
 * Flushes the buffers of a file.
 * - Write any modified data for the given file from the Windows in-memory cache.
 * - Commit all pending metadata changes for the given file from the Windows in-memory cache.
 * - Send a SYNC command to the storage device to commit in-memory cache to persistent storage.
 * \param FileHandle Handle to the file.
 * \return NTSTATUS Status code.
 */
NTSTATUS PhFlushBuffersFile(
    _In_ HANDLE FileHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //if (WindowsVersion >= WINDOWS_8)
    //{
    //    status = NtFlushBuffersFileEx(volumeHandle, 0, 0, 0, &ioStatusBlock);
    //}
    //else
    {
        status = NtFlushBuffersFile(FileHandle, &ioStatusBlock);
    }

    return status;
}

NTSTATUS PhGetFileHandleName(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PFILE_NAME_INFORMATION buffer;
    IO_STATUS_BLOCK isb;

    bufferSize = sizeof(FILE_NAME_INFORMATION) + MAX_PATH;
    buffer = PhAllocateZero(bufferSize);

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        buffer,
        bufferSize,
        FileNameInformation
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        bufferSize = sizeof(FILE_NAME_INFORMATION) + buffer->FileNameLength + sizeof(UNICODE_NULL);
        PhFree(buffer);
        buffer = PhAllocateZero(bufferSize);

        status = NtQueryInformationFile(
            FileHandle,
            &isb,
            buffer,
            bufferSize,
            FileNameInformation
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *FileName = PhCreateStringEx(buffer->FileName, buffer->FileNameLength);
    PhFree(buffer);

    return status;
}

NTSTATUS PhGetFileNetworkPhysicalName(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* FileName
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG bufferLength;
    PFILE_NETWORK_PHYSICAL_NAME_INFORMATION buffer;

    bufferLength = UFIELD_OFFSET(FILE_NETWORK_PHYSICAL_NAME_INFORMATION, FileName[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    buffer = PhAllocate(bufferLength);

    status = NtQueryInformationFile(
        FileHandle,
        &ioStatusBlock,
        buffer,
        bufferLength,
        FileNetworkPhysicalNameInformation
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        bufferLength = sizeof(FILE_NETWORK_PHYSICAL_NAME_INFORMATION) + buffer->FileNameLength;
        PhFree(buffer);
        buffer = PhAllocate(bufferLength);

        status = NtQueryInformationFile(
            FileHandle,
            &ioStatusBlock,
            buffer,
            bufferLength,
            FileNetworkPhysicalNameInformation
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *FileName = PhCreateStringEx(buffer->FileName, buffer->FileNameLength);
    PhFree(buffer);

    return status;
}

NTSTATUS PhGetFileAllInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_ALL_INFORMATION *FileInformation
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileAllInformation,
        FileInformation
        );
}

NTSTATUS PhGetFileId(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_ID_INFORMATION FileId
    )
{
    IO_STATUS_BLOCK isb;

    return NtQueryInformationFile(
        FileHandle,
        &isb,
        FileId,
        sizeof(FILE_ID_INFORMATION),
        FileIdInformation
        );
}

NTSTATUS PhGetProcessIdsUsingFile(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *ProcessIdsUsingFile
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileProcessIdsUsingFileInformation,
        ProcessIdsUsingFile
        );
}

NTSTATUS PhGetProcessIdsUsingFileByName(
    _In_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *ProcessIdsUsingFile
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhOpenFile(
        &fileHandle,
        FileName,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        RootDirectory,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = PhpQueryFileVariableSize(
            fileHandle,
            FileProcessIdsUsingFileInformation,
            ProcessIdsUsingFile
            );

        NtClose(fileHandle);
    }

    return status;
}

NTSTATUS PhGetProcessesUsingVolumeOrFile(
    _In_ HANDLE VolumeOrFileHandle,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *Information
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    IO_STATUS_BLOCK isb;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQueryInformationFile(
        VolumeOrFileHandle,
        &isb,
        buffer,
        bufferSize,
        FileProcessIdsUsingFileInformation
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > (1 << 30)) // 1GiB
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Information = (PFILE_PROCESS_IDS_USING_FILE_INFORMATION)buffer;

    return status;
}

NTSTATUS PhGetFileUsn(
    _In_ HANDLE FileHandle,
    _Out_ PUSN Usn
    )
{
    NTSTATUS status;
    ULONG recordLength;
    PUSN_RECORD_V2 recordBuffer; // USN_RECORD_UNION
    UCHAR buffer[sizeof(USN_RECORD_V2) + MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)];
    IO_STATUS_BLOCK isb;

    recordLength = sizeof(buffer);
    recordBuffer = (PUSN_RECORD_V2)buffer;

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_READ_FILE_USN_DATA, // FSCTL_WRITE_USN_CLOSE_RECORD
        NULL, // READ_FILE_USN_DATA
        0,
        recordBuffer,
        recordLength
        );

    if (NT_SUCCESS(status))
    {
        *Usn = recordBuffer->Usn;

        //switch (recordBuffer->Header.MajorVersion)
        //{
        //case 2:
        //    *Usn = recordBuffer->V2.Usn;
        //    break;
        //case 3:
        //    *Usn = recordBuffer->V3.Usn;
        //    break;
        //case 4:
        //    *Usn = recordBuffer->V4.Usn;
        //    break;
        //}
    }

    return status;
}

NTSTATUS PhSetFileBypassIO(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Enable
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FS_BPIO_INPUT bypassIoInput;
    FS_BPIO_OUTPUT bypassIoOutput;

    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/bypassio
    memset(&bypassIoInput, 0, sizeof(FS_BPIO_INPUT));
    bypassIoInput.Operation = Enable ? FS_BPIO_OP_ENABLE : FS_BPIO_OP_DISABLE;
    memset(&bypassIoOutput, 0, sizeof(FS_BPIO_OUTPUT));

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        FSCTL_MANAGE_BYPASS_IO,
        &bypassIoInput,
        sizeof(bypassIoInput),
        &bypassIoOutput,
        sizeof(bypassIoOutput)
        );

#ifdef DEBUG
    if (bypassIoOutput.OutFlags != FSBPIO_OUTFL_COMPATIBLE_STORAGE_DRIVER) // NT_SUCCESS(bypassIoOutput.Enable.OpStatus)
    {
        dprintf("BypassIO failed: (%S) %S\n", bypassIoOutput.Enable.FailingDriverName, bypassIoOutput.Enable.FailureReason);
    }
#endif

    return status;
}

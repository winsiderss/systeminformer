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
 *
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
 * \brief Enumerates the volume Reparse Points using the \\$Extend\\$Reparse:$R:$INDEX_ALLOCATION directory.
 *
 * \param FileHandle A handle to the volume.
 * \param Callback A pointer to a callback function.
 * \param Context A user-defined value to pass to the callback function.
 *
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
 *
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
    NTSTATUS status;

    if (WindowsVersion < WINDOWS_10_RS2)
    {
        PFILE_RENAME_INFORMATION renameInfo;
        IO_STATUS_BLOCK ioStatusBlock;
        ULONG renameInfoLength;

        renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + (ULONG)NewFileName->Length + sizeof(UNICODE_NULL);
        renameInfo = _malloca(renameInfoLength);

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

            _freea(renameInfo);
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        PFILE_RENAME_INFORMATION_EX renameInfo;
        IO_STATUS_BLOCK ioStatusBlock;
        ULONG renameInfoLength;

        renameInfoLength = sizeof(FILE_RENAME_INFORMATION_EX) + (ULONG)NewFileName->Length + sizeof(UNICODE_NULL);
        renameInfo = _malloca(renameInfoLength);

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

            _freea(renameInfo);
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
    FILE_IO_PRIORITY_HINT_INFORMATION ioPriorityHint;
    IO_STATUS_BLOCK ioStatusBlock;

    ioPriorityHint.PriorityHint = IoPriorityHint;

    return NtSetInformationFile(
        FileHandle,
        &ioStatusBlock,
        &ioPriorityHint,
        sizeof(FILE_IO_PRIORITY_HINT_INFORMATION),
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
        if (bufferSize > SIZE_MAX)
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

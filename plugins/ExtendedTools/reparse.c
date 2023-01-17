/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2023
 *
 */

#include "exttools.h"
#include <hndlinfo.h>
#include <sddl.h>

typedef BOOLEAN (NTAPI *PPH_ENUM_REPARSE_INDEX_FILE)(
    _In_ PFILE_REPARSE_POINT_INFORMATION Information,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_OBJECTID_INDEX_FILE)(
    _In_ PFILE_OBJECTID_INFORMATION Information,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI* PPH_ENUM_SD_ENTRY)(
    _In_ PSD_ENUM_SDS_ENTRY SDEntry,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    );

typedef struct _REPARSE_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    ULONG MenuItemIndex;
    PH_LAYOUT_MANAGER LayoutManager;
    ULONG Count;
    PPH_LIST Items;
} REPARSE_WINDOW_CONTEXT, *PREPARSE_WINDOW_CONTEXT;

typedef struct _REPARSE_LISTVIEW_ENTRY
{
    LONGLONG FileReference;
    PPH_STRING RootDirectory;
    PPH_STRING BestObjectName;

    PPH_STRING VolumeName;
    PPH_STRING SidFullName;
    PPH_STRING SDDLString;
    ULONG Tag;
    ULONG Hash;
    ULONG SecurityId;
    ULONG Length;
    UCHAR ObjectId[16];
} REPARSE_LISTVIEW_ENTRY, *PREPARSE_LISTVIEW_ENTRY;

#define ET_REPARSE_UPDATE_MSG (WM_APP + 1)
#define ET_REPARSE_UPDATE_ERROR (WM_APP + 2)

#define FILE_LAYOUT_ENTRY_VERSION 0x1
#define STREAM_LAYOUT_ENTRY_VERSION 0x1
#define PH_FIRST_LAYOUT_ENTRY(LayoutEntry) \
    ((PFILE_LAYOUT_ENTRY)(PTR_ADD_OFFSET((LayoutEntry), \
    ((PQUERY_FILE_LAYOUT_OUTPUT)(LayoutEntry))->FirstFileOffset)))
#define PH_NEXT_LAYOUT_ENTRY(LayoutEntry) ( \
    ((PFILE_LAYOUT_ENTRY)(LayoutEntry))->NextFileOffset ? \
    (PFILE_LAYOUT_ENTRY)(PTR_ADD_OFFSET((LayoutEntry), \
    ((PFILE_LAYOUT_ENTRY)(LayoutEntry))->NextFileOffset)) : \
    NULL \
    )

NTSTATUS EtGetFileHandleName(
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

NTSTATUS EtEnumerateVolumeReparsePoints(
    _In_ ULONG64 VolumeIndex,
    _In_ PPH_ENUM_REPARSE_INDEX_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    PPH_STRING fileName;

    fileName = PhFormatString(L"\\Device\\HarddiskVolume%I64u\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION", VolumeIndex);
    status = PhOpenFile(
        &fileHandle,
        &fileName->sr,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        BOOLEAN firstTime = TRUE;
        PVOID buffer;
        ULONG bufferSize = 0x400;

        buffer = PhAllocate(bufferSize);

        while (TRUE)
        {
            while (TRUE)
            {
                status = NtQueryDirectoryFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    bufferSize,
                    FileReparsePointInformation,
                    TRUE,
                    NULL,
                    firstTime
                    );

                if (status == STATUS_PENDING)
                {
                    status = NtWaitForSingleObject(fileHandle, FALSE, NULL);

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

            if (Callback)
            {
                if (!Callback(buffer, fileHandle, Context))
                    break;
            }

            firstTime = FALSE;
        }

        PhFree(buffer);
        NtClose(fileHandle);
    }

    PhDereferenceObject(fileName);

    return status;
}

NTSTATUS EtEnumerateVolumeObjectIds(
    _In_ ULONG64 VolumeIndex,
    _In_ PPH_ENUM_OBJECTID_INDEX_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    PPH_STRING fileName;

    fileName = PhFormatString(L"\\Device\\HarddiskVolume%I64u\\$Extend\\$ObjId:$O:$INDEX_ALLOCATION", VolumeIndex);
    status = PhOpenFile(
        &fileHandle,
        &fileName->sr,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        BOOLEAN firstTime = TRUE;
        PVOID buffer;
        ULONG bufferSize = 0x400;

        buffer = PhAllocate(bufferSize);

        while (TRUE)
        {
            while (TRUE)
            {
                status = NtQueryDirectoryFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    bufferSize,
                    FileObjectIdInformation,
                    TRUE,
                    NULL,
                    firstTime
                    );

                if (status == STATUS_PENDING)
                {
                    status = NtWaitForSingleObject(fileHandle, FALSE, NULL);

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

            if (status == STATUS_RECEIVE_PARTIAL)
                status = STATUS_SUCCESS;

            if (!NT_SUCCESS(status))
                break;

            if (Callback)
            {
                if (!Callback(buffer, fileHandle, Context))
                    break;
            }

            firstTime = FALSE;
        }

        PhFree(buffer);
        NtClose(fileHandle);
    }

    PhDereferenceObject(fileName);

    return status;
}

NTSTATUS EtEnumerateVolumeSecurityDescriptors(
    _In_ ULONG64 VolumeIndex,
    _In_ PPH_ENUM_SD_ENTRY Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PPH_STRING volumeName;

    volumeName = PhFormatString(L"\\Device\\HarddiskVolume%I64u", VolumeIndex);
    status = PhOpenFile(
        &fileHandle,
        &volumeName->sr,
        FILE_READ_DATA | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        SD_GLOBAL_CHANGE_INPUT input;
        PSD_GLOBAL_CHANGE_OUTPUT output;
        ULONG outputLength;

        memset(&input, 0, sizeof(SD_GLOBAL_CHANGE_INPUT));
        input.ChangeType = SD_GLOBAL_CHANGE_TYPE_ENUM_SDS;
        input.SdEnumSds.MaxSDEntriesToReturn = 0;
        input.SdEnumSds.StartingOffset = 0;

        outputLength = 0x2000000;
        output = PhAllocateZero(outputLength);

        while (TRUE)
        {
            PSD_ENUM_SDS_OUTPUT enumSecureOutput;
            PSD_ENUM_SDS_ENTRY enumSecureEntry;

            status = PhDeviceIoControlFile(
                fileHandle,
                FSCTL_SD_GLOBAL_CHANGE,
                &input,
                sizeof(SD_GLOBAL_CHANGE_INPUT),
                output,
                outputLength,
                NULL
                );

            if (!NT_SUCCESS(status))
                break;
            if (!output->SdEnumSds.NumSDEntriesReturned)
                break;

            enumSecureOutput = &output->SdEnumSds;
            enumSecureEntry = &enumSecureOutput->SDEntry[0];

            for (ULONGLONG i = 0; i < enumSecureOutput->NumSDEntriesReturned; i++)
            {
                if (Callback)
                {
                    if (!Callback(enumSecureEntry, fileHandle, Context))
                        break;
                }

                enumSecureEntry = PTR_ADD_OFFSET(enumSecureEntry, ALIGN_UP_BY(enumSecureEntry->Length, 16));
            }

            input.SdEnumSds.StartingOffset = enumSecureOutput->NextOffset;
        }

        NtClose(fileHandle);
        PhFree(output);
    }

    PhDereferenceObject(volumeName);

    return status;
}

NTSTATUS EtDeleteFileReparsePoint(
    _In_ PREPARSE_LISTVIEW_ENTRY Entry
    )
{
#define REPARSE_DATA_BUFFER_HEADER_SIZE UFIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
    NTSTATUS status;
    HANDLE directoryHandle;
    HANDLE fileHandle;
    PH_FILE_ID_DESCRIPTOR fileName;
    PREPARSE_DATA_BUFFER reparseBuffer;
    ULONG reparseLength;

    status = PhOpenFile(
        &directoryHandle,
        &Entry->RootDirectory->sr,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    fileName.Type = FileIdType;
    fileName.FileId.QuadPart = Entry->FileReference;

    status = PhOpenFileById(
        &fileHandle,
        directoryHandle,
        &fileName,
        FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(directoryHandle);
        return status;
    }

    reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    reparseBuffer = PhAllocate(reparseLength);

    status = PhDeviceIoControlFile(
        fileHandle,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        reparseBuffer,
        reparseLength,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(fileHandle);
        NtClose(directoryHandle);
        return status;
    }

    reparseBuffer->ReparseDataLength = 0;

    status = PhDeviceIoControlFile(
        fileHandle,
        FSCTL_DELETE_REPARSE_POINT,
        reparseBuffer,
        REPARSE_DATA_BUFFER_HEADER_SIZE,
        NULL,
        0,
        NULL
        );

    NtClose(fileHandle);
    NtClose(directoryHandle);

    return status;
}

NTSTATUS EtDeleteFileObjectId(
    _In_ PREPARSE_LISTVIEW_ENTRY Entry
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    HANDLE fileHandle;
    PH_FILE_ID_DESCRIPTOR fileName;

    status = PhOpenFile(
        &directoryHandle,
        &Entry->RootDirectory->sr,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    fileName.Type = FileIdType;
    fileName.FileId.QuadPart = Entry->FileReference;

    status = PhOpenFileById(
        &fileHandle,
        directoryHandle,
        &fileName,
        FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(directoryHandle);
        return status;
    }

    status = PhDeviceIoControlFile(
        fileHandle,
        FSCTL_DELETE_OBJECT_ID,
        NULL,
        0,
        NULL,
        0,
        NULL
        );

    NtClose(fileHandle);
    NtClose(directoryHandle);

    return status;
}

PWSTR EtReparseTagToString(
    _In_ ULONG Tag)
{
#ifndef IO_REPARSE_TAG_LX_SYMLINK
#define IO_REPARSE_TAG_LX_SYMLINK (0xA000001DL)
#endif
#ifndef IO_REPARSE_TAG_LX_FIFO
#define IO_REPARSE_TAG_LX_FIFO (0x80000024L)
#endif
#ifndef IO_REPARSE_TAG_LX_CHR
#define IO_REPARSE_TAG_LX_CHR (0x80000025L)
#endif
#ifndef IO_REPARSE_TAG_LX_BLK
#define IO_REPARSE_TAG_LX_BLK (0x80000026L)
#endif
#ifndef IO_REPARSE_TAG_DATALESS_CIM
#define IO_REPARSE_TAG_DATALESS_CIM (0xA0000028L)
#endif

    switch (Tag)
    {
    case IO_REPARSE_TAG_MOUNT_POINT:
        return L"MOUNT_POINT";
    case IO_REPARSE_TAG_HSM:
        return L"HSM";
    case IO_REPARSE_TAG_HSM2:
        return L"HSM2";
    case IO_REPARSE_TAG_SIS:
        return L"SIS";
    case IO_REPARSE_TAG_WIM:
        return L"WIM";
    case IO_REPARSE_TAG_CSV:
        return L"CSV";
    case IO_REPARSE_TAG_DFS:
        return L"DFS";
    case IO_REPARSE_TAG_SYMLINK:
        return L"SYMLINK";
    case IO_REPARSE_TAG_DFSR:
        return L"DFSR";
    case IO_REPARSE_TAG_DEDUP:
        return L"DEDUP";
    case IO_REPARSE_TAG_NFS:
        return L"NFS";
    case IO_REPARSE_TAG_FILE_PLACEHOLDER:
        return L"FILE_PLACEHOLDER";
    case IO_REPARSE_TAG_WOF:
        return L"WOF";
    case IO_REPARSE_TAG_WCI:
    case IO_REPARSE_TAG_WCI_1:
        return L"WCI";
    case IO_REPARSE_TAG_GLOBAL_REPARSE:
        return L"GLOBAL_REPARSE";
    case IO_REPARSE_TAG_CLOUD:
    case IO_REPARSE_TAG_CLOUD_1:
    case IO_REPARSE_TAG_CLOUD_2:
    case IO_REPARSE_TAG_CLOUD_3:
    case IO_REPARSE_TAG_CLOUD_4:
    case IO_REPARSE_TAG_CLOUD_5:
    case IO_REPARSE_TAG_CLOUD_6:
    case IO_REPARSE_TAG_CLOUD_7:
    case IO_REPARSE_TAG_CLOUD_8:
    case IO_REPARSE_TAG_CLOUD_9:
    case IO_REPARSE_TAG_CLOUD_A:
    case IO_REPARSE_TAG_CLOUD_B:
    case IO_REPARSE_TAG_CLOUD_C:
    case IO_REPARSE_TAG_CLOUD_D:
    case IO_REPARSE_TAG_CLOUD_E:
    case IO_REPARSE_TAG_CLOUD_F:
    case IO_REPARSE_TAG_CLOUD_MASK:
        return L"CLOUD";
    case IO_REPARSE_TAG_APPEXECLINK:
        return L"APPEXECLINK";
    case IO_REPARSE_TAG_PROJFS:
        return L"PROJFS";
    case IO_REPARSE_TAG_STORAGE_SYNC:
        return L"STORAGE_SYNC";
    case IO_REPARSE_TAG_WCI_TOMBSTONE:
        return L"WCI_TOMBSTONE";
    case IO_REPARSE_TAG_UNHANDLED:
        return L"UNHANDLED";
    case IO_REPARSE_TAG_ONEDRIVE:
        return L"ONEDRIVE";
    case IO_REPARSE_TAG_PROJFS_TOMBSTONE:
        return L"PROJFS_TOMBSTONE";
    case IO_REPARSE_TAG_AF_UNIX:
        return L"AF_UNIX";
    case IO_REPARSE_TAG_WCI_LINK:
    case IO_REPARSE_TAG_WCI_LINK_1:
        return L"WCI_LINK";
    case IO_REPARSE_TAG_DATALESS_CIM:
        return L"DATALESS_CIM";
    case IO_REPARSE_TAG_LX_SYMLINK:
        return L"LX_SYMLINK";
    case IO_REPARSE_TAG_LX_FIFO:
        return L"LX_FIFO";
    case IO_REPARSE_TAG_LX_CHR:
        return L"LX_CHR";
    case IO_REPARSE_TAG_LX_BLK:
        return L"LX_BLK";
    }

    return PhaFormatString(L"UNKNOWN: %lu", Tag)->Buffer;
}

BOOLEAN NTAPI EtEnumVolumeReparseCallback(
    _In_ PFILE_REPARSE_POINT_INFORMATION Information,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE reparseHandle;
    PPH_STRING objectName = NULL;
    PPH_STRING bestObjectName = NULL;
    PH_FILE_ID_DESCRIPTOR fileName;

    fileName.Type = FileIdType;
    fileName.FileId.QuadPart = Information->FileReference;

    status = PhOpenFileById(
        &reparseHandle,
        RootDirectory,
        &fileName,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        if (NT_SUCCESS(PhQueryObjectName(reparseHandle, &objectName)))
        {
            PhMoveReference(&bestObjectName, PhGetFileName(objectName));
        }

        //PREPARSE_DATA_BUFFER reparseBuffer;
        //ULONG reparseLength;
        //reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        //reparseBuffer = PhAllocate(reparseLength);
        //
        //if (NT_SUCCESS(PhDeviceIoControlFile(
        //    reparseHandle,
        //    FSCTL_GET_REPARSE_POINT,
        //    NULL,
        //    0,
        //    reparseBuffer,
        //    reparseLength,
        //    NULL
        //    )))
        //{
        //}
        //
        //PhFree(reparseBuffer);
        NtClose(reparseHandle);
    }

    if (context)
    {
        PREPARSE_LISTVIEW_ENTRY entry;
        PPH_STRING rootFileName = NULL;

        if (NT_SUCCESS(PhQueryObjectName(RootDirectory, &objectName)))
        {
            PhMoveReference(&rootFileName, objectName);
        }

        entry = PhAllocateZero(sizeof(REPARSE_LISTVIEW_ENTRY));
        entry->FileReference = Information->FileReference;
        entry->Tag = Information->Tag;
        entry->RootDirectory = rootFileName;
        PhSetReference(&entry->BestObjectName, bestObjectName);
        PhAddItemList(context->Items, entry);
    }

    PhClearReference(&bestObjectName);

    return TRUE;
}

BOOLEAN NTAPI EtEnumVolumeObjectIdCallback(
    _In_ PFILE_OBJECTID_INFORMATION Information,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE reparseHandle;
    PPH_STRING objectName = NULL;
    PPH_STRING bestObjectName = NULL;
    PH_FILE_ID_DESCRIPTOR fileName;

    fileName.Type = FileIdType;
    fileName.FileId.QuadPart = Information->FileReference;

    status = PhOpenFileById(
        &reparseHandle,
        RootDirectory,
        &fileName,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID
        );

    if (NT_SUCCESS(status))
    {
        if (NT_SUCCESS(PhQueryObjectName(reparseHandle, &objectName)))
        {
            PhMoveReference(&bestObjectName, PhGetFileName(objectName));
        }

        NtClose(reparseHandle);
    }

    if (context)
    {
        PREPARSE_LISTVIEW_ENTRY entry;
        PPH_STRING rootFileName = NULL;

        if (NT_SUCCESS(PhQueryObjectName(RootDirectory, &objectName)))
        {
            PhMoveReference(&rootFileName, objectName);
        }

        entry = PhAllocateZero(sizeof(REPARSE_LISTVIEW_ENTRY));
        entry->FileReference = Information->FileReference;
        entry->RootDirectory = rootFileName;
        PhSetReference(&entry->BestObjectName, bestObjectName);
        memcpy_s(entry->ObjectId, sizeof(entry->ObjectId), Information->ObjectId, sizeof(Information->ObjectId));

        PhAddItemList(context->Items, entry);
    }

    PhClearReference(&bestObjectName);

    return TRUE;
}

BOOLEAN NTAPI EtEnumVolumeSecurityDescriptorsCallback(
    _In_ PSD_ENUM_SDS_ENTRY SDEntry,
    _In_ HANDLE RootDirectory,
    _In_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    PREPARSE_LISTVIEW_ENTRY entry;
    PPH_STRING objectName = NULL;
    PPH_STRING rootFileName = NULL;
    PPH_STRING volumeName = NULL;

    if (NT_SUCCESS(PhQueryObjectName(RootDirectory, &objectName)))
    {
        rootFileName = objectName;
        volumeName = PhGetFileName(rootFileName);
    }

    entry = PhAllocateZero(sizeof(REPARSE_LISTVIEW_ENTRY));
    entry->FileReference = SDEntry->SecurityId;
    entry->RootDirectory = rootFileName;
    entry->VolumeName = volumeName;
    entry->SecurityId = SDEntry->SecurityId;
    entry->Hash = SDEntry->Hash;
    entry->Length = SDEntry->Length;

    if (RtlValidSecurityDescriptor(SDEntry->Descriptor))
    {
        ULONG stringSecurityDescriptorLength = 0;
        PWSTR stringSecurityDescriptor = NULL;
        PSID owner = NULL;
        BOOLEAN defaulted = FALSE;

        RtlGetOwnerSecurityDescriptor(SDEntry->Descriptor, &owner, &defaulted);

        if (owner)
        {
            entry->SidFullName = PhGetSidFullName(owner, TRUE, NULL);
        }

        if (ConvertSecurityDescriptorToStringSecurityDescriptor(
            SDEntry->Descriptor,
            SDDL_REVISION,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION |
            LABEL_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION |
            SCOPE_SECURITY_INFORMATION | PROCESS_TRUST_LABEL_SECURITY_INFORMATION |
            ACCESS_FILTER_SECURITY_INFORMATION | BACKUP_SECURITY_INFORMATION |
            PROTECTED_DACL_SECURITY_INFORMATION | PROTECTED_SACL_SECURITY_INFORMATION |
            UNPROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION,
            &stringSecurityDescriptor,
            &stringSecurityDescriptorLength
            ))
        {
            entry->SDDLString = PhCreateStringEx(stringSecurityDescriptor, stringSecurityDescriptorLength * sizeof(WCHAR));
            LocalFree(stringSecurityDescriptor);
        }

        PhAddItemList(context->Items, entry);
    }

    return TRUE;
}

PPH_LIST EtFindVolumeFilesWithSecurityId(
    _In_ PPH_STRING VolumeDeviceName,
    _In_ ULONG SecurityId
    )
{
    NTSTATUS status;
    HANDLE volumeFileList = NULL;
    HANDLE volumeHandle = NULL;
    ULONG outputLength;
    QUERY_FILE_LAYOUT_INPUT input;
    PQUERY_FILE_LAYOUT_OUTPUT output;
    PFILE_LAYOUT_ENTRY fileLayoutEntry;
    PFILE_LAYOUT_NAME_ENTRY fileLayoutNameEntry;
    PFILE_LAYOUT_INFO_ENTRY fileLayoutInfoEntry;

    status = PhCreateFile(
        &volumeHandle,
        &VolumeDeviceName->sr,
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE, // magic value
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    memset(&input, 0, sizeof(QUERY_FILE_LAYOUT_INPUT));
    input.Flags =
        QUERY_FILE_LAYOUT_RESTART |
        QUERY_FILE_LAYOUT_INCLUDE_NAMES |
        QUERY_FILE_LAYOUT_INCLUDE_STREAMS |
        QUERY_FILE_LAYOUT_INCLUDE_EXTENTS |
        QUERY_FILE_LAYOUT_INCLUDE_EXTRA_INFO |
        QUERY_FILE_LAYOUT_INCLUDE_STREAMS_WITH_NO_CLUSTERS_ALLOCATED |
        QUERY_FILE_LAYOUT_INCLUDE_FULL_PATH_IN_NAMES |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_DSC_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_TXF_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_EFS_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_FILES_WITH_DSC_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_DATA_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_REPARSE_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_EA_ATTRIBUTE;

    outputLength = 0x2000000; // magic value
    output = PhAllocateZero(outputLength);

    while (TRUE)
    {
        status = PhDeviceIoControlFile(
            volumeHandle,
            FSCTL_QUERY_FILE_LAYOUT,
            &input,
            sizeof(QUERY_FILE_LAYOUT_INPUT),
            output,
            outputLength,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        for (fileLayoutEntry = PH_FIRST_LAYOUT_ENTRY(output); fileLayoutEntry; fileLayoutEntry = PH_NEXT_LAYOUT_ENTRY(fileLayoutEntry))
        {
            if (fileLayoutEntry->Version != FILE_LAYOUT_ENTRY_VERSION)
            {
                status = STATUS_INVALID_KERNEL_INFO_VERSION;
                break;
            }

            if (!fileLayoutEntry->ExtraInfoOffset)
                continue;

            fileLayoutInfoEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->ExtraInfoOffset);

            if (fileLayoutInfoEntry->SecurityId == SecurityId)
            {
                if (!volumeFileList)
                {
                    volumeFileList = PhCreateList(10);
                }

                if (fileLayoutEntry->FirstNameOffset)
                {
                    fileLayoutNameEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->FirstNameOffset);

                    while (TRUE)
                    {
                        PPH_STRING fileName;

                        if (fileLayoutNameEntry->FileNameLength >= sizeof(UNICODE_NULL))
                            fileName = PhCreateStringEx(fileLayoutNameEntry->FileName, fileLayoutNameEntry->FileNameLength);
                        else
                            fileName = PhReferenceEmptyString();

                        if (PhStartsWithString2(fileName, L"\\", TRUE))
                            PhMoveReference(&fileName, PhConcatStrings(2, PhGetString(VolumeDeviceName), PhGetString(fileName)));
                        else
                            PhMoveReference(&fileName, PhConcatStrings(3, PhGetString(VolumeDeviceName), L"\\", PhGetString(fileName)));

                        PhMoveReference(&fileName, PhGetFileName(fileName));
                        if (volumeFileList) PhAddItemList(volumeFileList, fileName);

                        //if (fileLayoutNameEntry->Flags == 0 || fileLayoutNameEntry->Flags == FILE_LAYOUT_NAME_ENTRY_PRIMARY)
                        //{
                        //    PPH_STRING fileName = PhCreateStringEx(fileLayoutNameEntry->FileName, fileLayoutNameEntry->FileNameLength);
                        //    PhMoveReference(&fileName, PhConcatStrings(2, PhGetString(VolumeDeviceName), PhGetString(fileName)));
                        //    PhMoveReference(&fileName, PhGetFileName(fileName));
                        //    PhAddItemList(volumeFileList, fileName);
                        //    break;
                        //}

                        if (fileLayoutNameEntry->NextNameOffset == 0)
                            break;

                        fileLayoutNameEntry = PTR_ADD_OFFSET(fileLayoutNameEntry, fileLayoutNameEntry->NextNameOffset);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status))
            break;

        if (input.Flags & QUERY_FILE_LAYOUT_RESTART)
        {
            input.Flags &= ~QUERY_FILE_LAYOUT_RESTART;
        }
    }

CleanupExit:

    if (volumeHandle)
    {
        NtClose(volumeHandle);
    }

    return volumeFileList;
}

BOOLEAN NTAPI EtEnumDirectoryObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    static PH_STRINGREF volumePath = PH_STRINGREF_INIT(L"HarddiskVolume");
    PREPARSE_WINDOW_CONTEXT context = Context;
    PH_STRINGREF stringBefore;
    PH_STRINGREF stringAfter;
    ULONG64 volumeIndex = ULLONG_MAX;

    if (context && PhStartsWithStringRef(Name, &volumePath, TRUE))
    {
        if (PhSplitStringRefAtString(Name, &volumePath, FALSE, &stringBefore, &stringAfter))
        {
            if (PhStringToInteger64(&stringAfter, 0, &volumeIndex))
            {
                switch (context->MenuItemIndex)
                {
                case ID_REPARSE_POINTS:
                    EtEnumerateVolumeReparsePoints(volumeIndex, EtEnumVolumeReparseCallback, Context);
                    break;
                case ID_REPARSE_OBJID:
                    EtEnumerateVolumeObjectIds(volumeIndex, EtEnumVolumeObjectIdCallback, Context);
                    break;
                case ID_REPARSE_SDDL:
                    EtEnumerateVolumeSecurityDescriptors(volumeIndex, EtEnumVolumeSecurityDescriptorsCallback, Context);
                    break;
                }
            }
        }
    }

    return TRUE;
}

NTSTATUS EtEnumerateVolumeDirectoryObjects(
    _In_ PREPARSE_WINDOW_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING name;

    RtlInitUnicodeString(&name, L"\\Device");
    InitializeObjectAttributes(
        &objectAttributes,
        &name,
        0,
        NULL,
        NULL
        );

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        );

    if (NT_SUCCESS(status))
    {
        PhEnumDirectoryObjects(
            directoryHandle,
            EtEnumDirectoryObjectsCallback,
            Context
            );

        NtClose(directoryHandle);
    }

    if (NT_SUCCESS(status))
        PostMessage(Context->WindowHandle, ET_REPARSE_UPDATE_MSG, 0, 0);
    else
        PostMessage(Context->WindowHandle, ET_REPARSE_UPDATE_ERROR, 0, status);

    return status;
}

VOID EtFreeReparseListViewEntries(
    _In_ PREPARSE_WINDOW_CONTEXT Context
    )
{
    //INT index = INT_ERROR;
    //
    //while ((index = PhFindListViewItemByFlags(
    //    Context->ListViewHandle,
    //    index,
    //    LVNI_ALL
    //    )) != INT_ERROR)
    //{
    //    PREPARSE_LISTVIEW_ENTRY param;
    //
    //    if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
    //    {
    //        PhClearReference(&param->RootDirectory);
    //        PhFree(param);
    //    }
    //}

    if (Context->Items)
    {
        for (ULONG i = 0; i < Context->Items->Count; i++)
        {
            PREPARSE_LISTVIEW_ENTRY entry = Context->Items->Items[i];

            if (entry->RootDirectory)
                PhDereferenceObject(entry->RootDirectory);
            if (entry->BestObjectName)
                PhDereferenceObject(entry->BestObjectName);
            if (entry->VolumeName)
                PhDereferenceObject(entry->VolumeName);
            if (entry->SidFullName)
                PhDereferenceObject(entry->SidFullName);
            if (entry->SDDLString)
                PhDereferenceObject(entry->SDDLString);
            PhFree(entry);
        }

        PhDereferenceObject(Context->Items);
        Context->Items = NULL;
    }
}

typedef struct _REPARSE_SECURITYID_CONTEXT
{
    HWND ListViewHandle;
    PPH_LIST FileList;
    PH_LAYOUT_MANAGER LayoutManager;
} REPARSE_SECURITYID_CONTEXT, *PREPARSE_SECURITYID_CONTEXT;

INT_PTR CALLBACK EtFindSecurityIdsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PREPARSE_SECURITYID_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(REPARSE_SECURITYID_CONTEXT));
        context->FileList = (PPH_LIST)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_REPARSE_LIST);

            PhSetWindowText(hwndDlg, L"NTFS SecurityID");
            PhSetApplicationWindowIcon(hwndDlg);

            ShowWindow(GetDlgItem(hwndDlg, IDRETRY), SW_HIDE);

            PhCenterWindow(hwndDlg, PhMainWndHandle);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            //PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            PhSetExtendedListView(context->ListViewHandle);
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Filename");

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            if (context->FileList)
            {
                for (ULONG i = 0; i < context->FileList->Count; i++)
                {
                    INT index;
                    WCHAR number[PH_PTR_STR_LEN_1];

                    PhPrintUInt32(number, i);
                    index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, NULL);
                    PhSetListViewSubItem(context->ListViewHandle, index, 1, PhGetStringOrEmpty(context->FileList->Items[i]));

                    PhDereferenceObject(context->FileList->Items[i]);
                }

                PhDereferenceObject(context->FileList);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            switch (hdr->code)
            {
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        POINT point;
                        PPH_EMENU menu;
                        ULONG numberOfItems;
                        PVOID* listviewItems;
                        PPH_EMENU_ITEM selectedItem;

                        PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                        if (numberOfItems == 0)
                            break;

                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyListViewEMenuItem(menu, USHRT_MAX, context->ListViewHandle);

                        GetCursorPos(&point);
                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            point.x,
                            point.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (PhHandleCopyListViewEMenuItem(selectedItem))
                                break;

                            switch (selectedItem->Id)
                            {
                            case USHRT_MAX:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }

                        PhDestroyEMenu(menu);

                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

INT_PTR CALLBACK EtReparseDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PREPARSE_WINDOW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(REPARSE_WINDOW_CONTEXT));
        context->MenuItemIndex = (ULONG)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_REPARSE_LIST);
            context->Items = PhCreateList(100);

            switch (context->MenuItemIndex)
            {
            case ID_REPARSE_POINTS:
                PhSetWindowText(hwndDlg, L"NTFS Reparse Points");
                break;
            case ID_REPARSE_OBJID:
                PhSetWindowText(hwndDlg, L"NTFS Object Identifiers");
                break;
            case ID_REPARSE_SDDL:
                PhSetWindowText(hwndDlg, L"NTFS Security Descriptors");
                break;
            }

            PhSetApplicationWindowIcon(hwndDlg);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_REPARSE_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_REPARSE_WINDOW_POSITION, SETTING_NAME_REPARSE_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle); // TODO: Pass ParentWindowHandle from EtShowReparseDialog (dmex)

            PhSetExtendedListView(context->ListViewHandle);
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");

            switch (context->MenuItemIndex)
            {
            case ID_REPARSE_POINTS:
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"File index");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Reparse tag");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 250, L"Filename");
                PhLoadListViewColumnsFromSetting(SETTING_NAME_REPARSE_LISTVIEW_COLUMNS, context->ListViewHandle);
                break;
            case ID_REPARSE_OBJID:
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"File index");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Object identifier");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 250, L"Filename");
                PhLoadListViewColumnsFromSetting(SETTING_NAME_REPARSE_OBJECTID_LISTVIEW_COLUMNS, context->ListViewHandle);
                break;
            case ID_REPARSE_SDDL:
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Volume");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"SecurityID");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Hash");
                PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Length");
                PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 150, L"Owner");
                PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 250, L"SDDL");
                PhLoadListViewColumnsFromSetting(SETTING_NAME_REPARSE_SD_LISTVIEW_COLUMNS, context->ListViewHandle);
                break;
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EnableWindow(GetDlgItem(hwndDlg, IDRETRY), FALSE);
            PhCreateThread2(EtEnumerateVolumeDirectoryObjects, context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_REPARSE_WINDOW_POSITION, SETTING_NAME_REPARSE_WINDOW_SIZE, hwndDlg);

            switch (context->MenuItemIndex)
            {
            case ID_REPARSE_POINTS:
                PhSaveListViewColumnsToSetting(SETTING_NAME_REPARSE_LISTVIEW_COLUMNS, context->ListViewHandle);
                break;
            case ID_REPARSE_OBJID:
                PhSaveListViewColumnsToSetting(SETTING_NAME_REPARSE_OBJECTID_LISTVIEW_COLUMNS, context->ListViewHandle);
                break;
            case ID_REPARSE_SDDL:
                PhSaveListViewColumnsToSetting(SETTING_NAME_REPARSE_SD_LISTVIEW_COLUMNS, context->ListViewHandle);
                break;
            }

            PhDeleteLayoutManager(&context->LayoutManager);

            EtFreeReparseListViewEntries(context);

            PhFree(context);
        }
        break;
    case ET_REPARSE_UPDATE_MSG:
        {
            WCHAR number[PH_PTR_STR_LEN_1];
            INT index;

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            switch (context->MenuItemIndex)
            {
            case ID_REPARSE_POINTS:
                {
                    if (context->Items)
                    {
                        for (ULONG i = 0; i < context->Items->Count; i++)
                        {
                            PREPARSE_LISTVIEW_ENTRY entry = context->Items->Items[i];

                            PhPrintUInt32(number, ++context->Count);
                            index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, entry);
                            PhSetListViewSubItem(context->ListViewHandle, index, 1, PhaFormatString(L"%I64u (0x%I64x)", entry->FileReference, entry->FileReference)->Buffer);
                            PhSetListViewSubItem(context->ListViewHandle, index, 2, EtReparseTagToString(entry->Tag));
                            PhSetListViewSubItem(context->ListViewHandle, index, 3, PhGetStringOrEmpty(entry->BestObjectName));
                        }
                    }
                }
                break;
            case ID_REPARSE_OBJID:
                {
                    if (context->Items)
                    {
                        for (ULONG i = 0; i < context->Items->Count; i++)
                        {
                            PREPARSE_LISTVIEW_ENTRY entry = context->Items->Items[i];

                            PhPrintUInt32(number, ++context->Count);
                            index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, entry);
                            PhSetListViewSubItem(context->ListViewHandle, index, 1, PhaFormatString(L"%I64u (0x%I64x)", entry->FileReference, entry->FileReference)->Buffer);

                            {
                                PPH_STRING string;
                                PGUID guid = (PGUID)entry->ObjectId;

                                // The swap returns the same value as 'fsutil objectid query filepath' (dmex)
                                PhReverseGuid(guid);

                                string = PhFormatGuid(guid);
                                PhSetListViewSubItem(context->ListViewHandle, index, 2, PhGetStringOrEmpty(string));
                                PhDereferenceObject(string);
                            }

                            PhSetListViewSubItem(context->ListViewHandle, index, 3, PhGetStringOrEmpty(entry->BestObjectName));
                        }
                    }
                }
                break;
            case ID_REPARSE_SDDL:
                {
                    if (context->Items)
                    {
                        for (ULONG i = 0; i < context->Items->Count; i++)
                        {
                            PREPARSE_LISTVIEW_ENTRY entry = context->Items->Items[i];

                            PhPrintUInt32(number, ++context->Count);
                            index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, entry);
                            PhSetListViewSubItem(context->ListViewHandle, index, 1, PhGetStringOrEmpty(entry->VolumeName));
                            PhSetListViewSubItem(context->ListViewHandle, index, 2, PhaFormatUInt64(entry->SecurityId, FALSE)->Buffer);
                            PhSetListViewSubItem(context->ListViewHandle, index, 3, PhaFormatUInt64(entry->Hash, FALSE)->Buffer);
                            PhSetListViewSubItem(context->ListViewHandle, index, 4, PhaFormatUInt64(entry->Length, FALSE)->Buffer);
                            PhSetListViewSubItem(context->ListViewHandle, index, 5, PhGetStringOrEmpty(entry->SidFullName));
                            PhSetListViewSubItem(context->ListViewHandle, index, 6, PhGetStringOrEmpty(entry->SDDLString));
                        }
                    }
                }
                break;
            }

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

            EnableWindow(GetDlgItem(hwndDlg, IDRETRY), TRUE);
        }
        break;
    case ET_REPARSE_UPDATE_ERROR:
        {
            NTSTATUS status = (NTSTATUS)lParam;

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"Unable to enumerate the objects.", status, 0);
            }

            EnableWindow(GetDlgItem(hwndDlg, IDRETRY), TRUE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDRETRY:
                {
                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    EtFreeReparseListViewEntries(context);
                    ListView_DeleteAllItems(context->ListViewHandle);
                    context->Count = 0;
                    context->Items = PhCreateList(100);
                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

                    EnableWindow(GetDlgItem(hwndDlg, IDRETRY), FALSE);
                    PhCreateThread2(EtEnumerateVolumeDirectoryObjects, context);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            switch (hdr->code)
            {
            case LVN_GETEMPTYMARKUP:
                {
                    NMLVEMPTYMARKUP* listview = (NMLVEMPTYMARKUP*)lParam;

                    listview->dwFlags = EMF_CENTERED;
                    wcsncpy_s(listview->szMarkup, RTL_NUMBER_OF(listview->szMarkup), L"Querying objects...", _TRUNCATE);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        POINT point;
                        PPH_EMENU menu;
                        ULONG numberOfItems;
                        PVOID* listviewItems;
                        PPH_EMENU_ITEM selectedItem;

                        PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                        if (numberOfItems == 0)
                            break;

                        menu = PhCreateEMenu();

                        switch (context->MenuItemIndex)
                        {
                        case ID_REPARSE_POINTS:
                        case ID_REPARSE_OBJID:
                            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Remove...", NULL, NULL), ULONG_MAX);
                            break;
                        case ID_REPARSE_SDDL:
                            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Find files...", NULL, NULL), ULONG_MAX);
                            break;
                        }

                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyListViewEMenuItem(menu, USHRT_MAX, context->ListViewHandle);

                        GetCursorPos(&point);
                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            point.x,
                            point.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (PhHandleCopyListViewEMenuItem(selectedItem))
                                break;

                            switch (selectedItem->Id)
                            {
                            case 1:
                                {
                                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

                                    for (ULONG i = 0; i < numberOfItems; i++)
                                    {
                                        PREPARSE_LISTVIEW_ENTRY entry = listviewItems[i];
                                        INT index = PhFindListViewItemByParam(context->ListViewHandle, INT_ERROR, entry);

                                        if (index != INT_ERROR)
                                        {
                                            switch (context->MenuItemIndex)
                                            {
                                            case ID_REPARSE_POINTS:
                                                {
                                                    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                                                        hwndDlg,
                                                        L"remove",
                                                        L"the repase point",
                                                        L"The repase point will be permanently deleted.",
                                                        FALSE
                                                        ))
                                                    {
                                                        NTSTATUS status;

                                                        status = EtDeleteFileReparsePoint(entry);

                                                        if (NT_SUCCESS(status))
                                                        {
                                                            PhRemoveListViewItem(context->ListViewHandle, index);
                                                        }
                                                        else
                                                        {
                                                            PhShowStatus(hwndDlg, L"Unable to remove the reparse point.", status, 0);
                                                        }
                                                    }
                                                }
                                                break;
                                            case ID_REPARSE_OBJID:
                                                {
                                                    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                                                        hwndDlg,
                                                        L"remove",
                                                        L"the object identifier",
                                                        L"The object identifier will be permanently deleted.",
                                                        FALSE
                                                        ))
                                                    {
                                                        NTSTATUS status;

                                                        status = EtDeleteFileObjectId(entry);

                                                        if (NT_SUCCESS(status))
                                                        {
                                                            PhRemoveListViewItem(context->ListViewHandle, index);
                                                        }
                                                        else
                                                        {
                                                            PhShowStatus(hwndDlg, L"Unable to remove the object identifier.", status, 0);
                                                        }
                                                    }
                                                }
                                                break;
                                            case ID_REPARSE_SDDL:
                                                {
                                                    PPH_LIST fileNames;

                                                    if (fileNames = EtFindVolumeFilesWithSecurityId(entry->RootDirectory, (ULONG)entry->FileReference))
                                                    {
                                                        PhDialogBox(
                                                            PluginInstance->DllBase,
                                                            MAKEINTRESOURCE(IDD_REPARSEDIALOG),
                                                            hwndDlg,
                                                            EtFindSecurityIdsDlgProc,
                                                            fileNames
                                                            );
                                                    }
                                                    else
                                                    {
                                                        PhShowStatus(hwndDlg, L"Unable to locate files with the SecurityId.", STATUS_NOT_FOUND, 0);
                                                    }
                                                }
                                                break;
                                            }
                                        }
                                    }

                                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                                }
                                break;
                            case USHRT_MAX:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }

                        PhDestroyEMenu(menu);

                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID EtShowReparseDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Context
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_REPARSEDIALOG),
        NULL,
        EtReparseDlgProc,
        Context
        );
}

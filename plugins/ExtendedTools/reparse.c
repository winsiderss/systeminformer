/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2022
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
    HWND ListViewHandle;
    ULONG MenuItemIndex;
    PH_LAYOUT_MANAGER LayoutManager;
    ULONG Count;
} REPARSE_WINDOW_CONTEXT, *PREPARSE_WINDOW_CONTEXT;

typedef struct _REPARSE_LISTVIEW_ENTRY
{
    LONGLONG FileReference;
    PPH_STRING RootDirectory;
} REPARSE_LISTVIEW_ENTRY, *PREPARSE_LISTVIEW_ENTRY;

NTSTATUS GetFileHandleName(
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

NTSTATUS EnumerateVolumeReparsePoints(
    _In_ ULONG64 VolumeIndex,
    _In_ PPH_ENUM_REPARSE_INDEX_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;
    UNICODE_STRING fileInfoUs;
    PPH_STRING volumeName;

    volumeName = PhFormatString(L"\\Device\\HarddiskVolume%I64u\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION", VolumeIndex);
    PhStringRefToUnicodeString(&volumeName->sr, &fileInfoUs);
    InitializeObjectAttributes(&oa, &fileInfoUs, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenFile(
        &fileHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT
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

    PhDereferenceObject(volumeName);

    return status;
}

NTSTATUS EnumerateVolumeObjectIds(
    _In_ ULONG64 VolumeIndex,
    _In_ PPH_ENUM_OBJECTID_INDEX_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;
    UNICODE_STRING fileInfoUs;
    PPH_STRING volumeName;

    volumeName = PhFormatString(L"\\Device\\HarddiskVolume%I64u\\$Extend\\$ObjId:$O:$INDEX_ALLOCATION", VolumeIndex);
    PhStringRefToUnicodeString(&volumeName->sr, &fileInfoUs);
    InitializeObjectAttributes(&oa, &fileInfoUs, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenFile(
        &fileHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT
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

    PhDereferenceObject(volumeName);

    return status;
}

NTSTATUS EnumerateVolumeSecurityDescriptors(
    _In_ ULONG64 VolumeIndex,
    _In_ PPH_ENUM_SD_ENTRY Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb = { 0 };
    UNICODE_STRING fileInfoUs;
    PPH_STRING volumeName;

    volumeName = PhFormatString(L"\\Device\\HarddiskVolume%I64u", VolumeIndex);
    PhStringRefToUnicodeString(&volumeName->sr, &fileInfoUs);
    InitializeObjectAttributes(&oa, &fileInfoUs, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenFile(
        &fileHandle,
        FILE_READ_DATA | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT
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

            status = NtFsControlFile(
                fileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                FSCTL_SD_GLOBAL_CHANGE,
                &input,
                sizeof(SD_GLOBAL_CHANGE_INPUT),
                output,
                outputLength
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

NTSTATUS PhDeleteFileReparsePoint(
    _In_ PREPARSE_LISTVIEW_ENTRY Entry
    )
{
#define REPARSE_DATA_BUFFER_HEADER_SIZE UFIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
    NTSTATUS status;
    HANDLE directoryHandle;
    HANDLE fileHandle;
    UNICODE_STRING fileNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;
    PREPARSE_DATA_BUFFER reparseBuffer;
    ULONG reparseLength;

    PhStringRefToUnicodeString(&Entry->RootDirectory->sr, &fileNameUs);
    InitializeObjectAttributes(&oa, &fileNameUs, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenFile(
        &directoryHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    fileNameUs.Length = sizeof(LONGLONG);
    fileNameUs.MaximumLength = sizeof(LONGLONG);
    fileNameUs.Buffer = (PWSTR)&Entry->FileReference;

    InitializeObjectAttributes(
        &oa,
        &fileNameUs,
        OBJ_CASE_INSENSITIVE,
        directoryHandle,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
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

    status = NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        reparseBuffer,
        reparseLength
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(fileHandle);
        NtClose(directoryHandle);
        return status;
    }

    reparseBuffer->ReparseDataLength = 0;

    status = NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_DELETE_REPARSE_POINT,
        reparseBuffer,
        REPARSE_DATA_BUFFER_HEADER_SIZE,
        NULL,
        0
        );

    NtClose(fileHandle);
    NtClose(directoryHandle);

    return status;
}

NTSTATUS PhDeleteFileObjectId(
    _In_ PREPARSE_LISTVIEW_ENTRY Entry
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    HANDLE fileHandle;
    UNICODE_STRING fileNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    PhStringRefToUnicodeString(&Entry->RootDirectory->sr, &fileNameUs);
    InitializeObjectAttributes(&oa, &fileNameUs, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenFile(
        &directoryHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    fileNameUs.Length = sizeof(LONGLONG);
    fileNameUs.MaximumLength = sizeof(LONGLONG);
    fileNameUs.Buffer = (PWSTR)&Entry->FileReference;

    InitializeObjectAttributes(
        &oa,
        &fileNameUs,
        OBJ_CASE_INSENSITIVE,
        directoryHandle,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(directoryHandle);
        return status;
    }

    status = NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_DELETE_OBJECT_ID,
        NULL,
        0,
        NULL,
        0
        );

    NtClose(fileHandle);
    NtClose(directoryHandle);

    return status;
}

PWSTR ReparseTagToString(
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

BOOLEAN NTAPI EnumVolumeReparseCallback(
    _In_ PFILE_REPARSE_POINT_INFORMATION Information,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE reparseHandle;
    PPH_STRING objectName = NULL;
    PPH_STRING fileName = NULL;
    UNICODE_STRING fileNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    fileNameUs.Length = sizeof(LONGLONG);
    fileNameUs.MaximumLength = sizeof(LONGLONG);
    fileNameUs.Buffer = (PWSTR)&Information->FileReference;

    InitializeObjectAttributes(
        &oa,
        &fileNameUs,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenFile(
        &reparseHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        if (NT_SUCCESS(PhGetHandleInformation(
            NtCurrentProcess(),
            reparseHandle,
            ULONG_MAX,
            NULL,
            NULL,
            NULL,
            &objectName
            )))
        {
            fileName = objectName;
        }

        //PREPARSE_DATA_BUFFER reparseBuffer;
        //ULONG reparseLength;
        //reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        //reparseBuffer = PhAllocate(reparseLength);
        //
        //if (NT_SUCCESS(NtFsControlFile(
        //    reparseHandle,
        //    NULL,
        //    NULL,
        //    NULL,
        //    &isb,
        //    FSCTL_GET_REPARSE_POINT,
        //    NULL,
        //    0,
        //    reparseBuffer,
        //    reparseLength
        //    )))
        //{
        //}
        //
        //PhFree(reparseBuffer);
        NtClose(reparseHandle);
    }

    if (context)
    {
        INT index;
        PREPARSE_LISTVIEW_ENTRY entry;
        PPH_STRING rootFileName = NULL;
        WCHAR number[PH_PTR_STR_LEN_1];

        objectName = NULL;

        if (NT_SUCCESS(PhGetHandleInformation(
            NtCurrentProcess(),
            RootDirectory,
            ULONG_MAX,
            NULL,
            NULL,
            &objectName,
            NULL
            )))
        {
            rootFileName = objectName;
        }

        entry = PhAllocate(sizeof(REPARSE_LISTVIEW_ENTRY));
        entry->FileReference = Information->FileReference;
        entry->RootDirectory = rootFileName;

        PhPrintUInt32(number, ++context->Count);
        index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, entry);
        PhSetListViewSubItem(context->ListViewHandle, index, 1, PhaFormatString(L"%I64u (0x%I64x)", Information->FileReference, Information->FileReference)->Buffer);
        PhSetListViewSubItem(context->ListViewHandle, index, 2, ReparseTagToString(Information->Tag));
        PhSetListViewSubItem(context->ListViewHandle, index, 3, PhGetStringOrEmpty(fileName));
    }

    PhClearReference(&fileName);

    return TRUE;
}

BOOLEAN NTAPI EnumVolumeObjectIdCallback(
    _In_ PFILE_OBJECTID_INFORMATION Information,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE reparseHandle;
    PPH_STRING objectName = NULL;
    PPH_STRING fileName = NULL;
    UNICODE_STRING fileNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    fileNameUs.Length = sizeof(LONGLONG);
    fileNameUs.MaximumLength = sizeof(LONGLONG);
    fileNameUs.Buffer = (PWSTR)&Information->FileReference;

    InitializeObjectAttributes(
        &oa,
        &fileNameUs,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenFile(
        &reparseHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID
        );

    if (NT_SUCCESS(status))
    {
        if (NT_SUCCESS(PhGetHandleInformation(
            NtCurrentProcess(),
            reparseHandle,
            ULONG_MAX,
            NULL,
            NULL,
            NULL,
            &objectName
            )))
        {
            fileName = objectName;
        }

        NtClose(reparseHandle);
    }

    if (context)
    {
        INT index;
        PREPARSE_LISTVIEW_ENTRY entry;
        PPH_STRING rootFileName = NULL;
        WCHAR number[PH_PTR_STR_LEN_1];

        if (NT_SUCCESS(PhGetHandleInformation(
            NtCurrentProcess(),
            RootDirectory,
            ULONG_MAX,
            NULL,
            NULL,
            &objectName,
            NULL
            )))
        {
            rootFileName = objectName;
        }

        entry = PhAllocate(sizeof(REPARSE_LISTVIEW_ENTRY));
        entry->FileReference = Information->FileReference;
        entry->RootDirectory = rootFileName;

        PhPrintUInt32(number, ++context->Count);
        index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, entry);
        PhSetListViewSubItem(context->ListViewHandle, index, 1, PhaFormatString(L"%I64u (0x%I64x)", Information->FileReference, Information->FileReference)->Buffer);

        {
            PPH_STRING string;
            PGUID guid = (PGUID)Information->ObjectId;

            // The swap returns the same value as 'fsutil objectid query filepath' (dmex)
            guid->Data1 = _byteswap_ulong(guid->Data1);
            guid->Data2 = _byteswap_ushort(guid->Data2);
            guid->Data3 = _byteswap_ushort(guid->Data3);

            string = PhFormatGuid(guid);
            PhSetListViewSubItem(context->ListViewHandle, index, 2, PhGetStringOrEmpty(string));
            PhDereferenceObject(string);
        }

        PhSetListViewSubItem(context->ListViewHandle, index, 3, PhGetStringOrEmpty(fileName));
    }

    PhClearReference(&fileName);

    return TRUE;
}

BOOLEAN NTAPI EnumVolumeSecurityDescriptorsCallback(
    _In_ PSD_ENUM_SDS_ENTRY SDEntry,
    _In_ HANDLE RootDirectory,
    _In_opt_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    INT index;
    PPH_STRING objectName = NULL;
    PPH_STRING rootFileName = NULL;
    PPH_STRING volumeName = NULL;
    WCHAR number[PH_PTR_STR_LEN_1];

    if (NT_SUCCESS(PhGetHandleInformation(
        NtCurrentProcess(),
        RootDirectory,
        ULONG_MAX,
        NULL,
        NULL,
        &objectName,
        NULL
        )))
    {
        rootFileName = objectName;
        volumeName = PhGetFileName(rootFileName);
    }

    PREPARSE_LISTVIEW_ENTRY entry;

    entry = PhAllocate(sizeof(REPARSE_LISTVIEW_ENTRY));
    entry->FileReference = SDEntry->SecurityId;
    entry->RootDirectory = rootFileName;


    PhPrintUInt32(number, ++context->Count);
    index = PhAddListViewItem(context->ListViewHandle, MAXINT, number, entry);
    PhSetListViewSubItem(context->ListViewHandle, index, 1, PhGetStringOrEmpty(volumeName));
    PhSetListViewSubItem(context->ListViewHandle, index, 2, PhaFormatUInt64(SDEntry->SecurityId, FALSE)->Buffer);
    PhSetListViewSubItem(context->ListViewHandle, index, 3, PhaFormatUInt64(SDEntry->Hash, FALSE)->Buffer);
    PhSetListViewSubItem(context->ListViewHandle, index, 4, PhaFormatUInt64(SDEntry->Length, FALSE)->Buffer);

    if (RtlValidSecurityDescriptor(SDEntry->Descriptor))
    {
        PPH_STRING securityDescriptorString;
        ULONG stringSecurityDescriptorLength = 0;
        PWSTR stringSecurityDescriptor = NULL;
        PSID owner = NULL;
        BOOLEAN defaulted = FALSE;

        RtlGetOwnerSecurityDescriptor(SDEntry->Descriptor, &owner, &defaulted);

        if (owner)
        {
            PPH_STRING fullName = PhGetSidFullName(owner, TRUE, NULL);

            if (fullName)
            {
                PhSetListViewSubItem(context->ListViewHandle, index, 5, fullName->Buffer);
                PhDereferenceObject(fullName);
            }
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
            securityDescriptorString = PhCreateStringEx(stringSecurityDescriptor, stringSecurityDescriptorLength * sizeof(WCHAR));
            PhSetListViewSubItem(context->ListViewHandle, index, 6, securityDescriptorString->Buffer);
            PhDereferenceObject(securityDescriptorString);

            LocalFree(stringSecurityDescriptor);
        }
    }

    return TRUE;
}

#define FILE_LAYOUT_ENTRY_VERSION 0x1
#define STREAM_LAYOUT_ENTRY_VERSION 0x1
#define FIRST_LAYOUT_ENTRY(LayoutEntry) ((LayoutEntry) ? PTR_ADD_OFFSET(LayoutEntry, (LayoutEntry)->FirstFileOffset) : NULL)
#define NEXT_LAYOUT_ENTRY(LayoutEntry) (((LayoutEntry))->NextFileOffset ? PTR_ADD_OFFSET((LayoutEntry), (LayoutEntry)->NextFileOffset) : NULL)

PPH_LIST FindVolumeFilesWithSecurityId(
    _In_ PPH_STRING VolumeDeviceName,
    _In_ ULONG SecurityId
    )
{
    NTSTATUS status;
    HANDLE volumeFileList = NULL;
    HANDLE volumeHandle = NULL;
    IO_STATUS_BLOCK isb;
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
        status = NtFsControlFile(
            volumeHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            FSCTL_QUERY_FILE_LAYOUT,
            &input,
            sizeof(QUERY_FILE_LAYOUT_INPUT),
            output,
            outputLength
            );

        if (!NT_SUCCESS(status))
            break;

        for (fileLayoutEntry = FIRST_LAYOUT_ENTRY(output); fileLayoutEntry; fileLayoutEntry = NEXT_LAYOUT_ENTRY(fileLayoutEntry))
        {
            if (fileLayoutEntry->Version != FILE_LAYOUT_ENTRY_VERSION)
            {
                status = STATUS_INVALID_KERNEL_INFO_VERSION;
                break;
            }

            fileLayoutNameEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->FirstNameOffset);
            fileLayoutInfoEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->ExtraInfoOffset);

            if (fileLayoutInfoEntry->SecurityId == SecurityId)
            {
                if (!volumeFileList)
                {
                    volumeFileList = PhCreateList(10);
                }

                while (TRUE)
                {
                    PPH_STRING fileName = PhCreateStringEx(fileLayoutNameEntry->FileName, fileLayoutNameEntry->FileNameLength);
                    PhMoveReference(&fileName, PhConcatStrings(2, PhGetString(VolumeDeviceName), PhGetString(fileName)));
                    PhMoveReference(&fileName, PhGetFileName(fileName));
                    PhAddItemList(volumeFileList, fileName);

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


BOOLEAN NTAPI EnumDirectoryObjectsCallback(
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
                    EnumerateVolumeReparsePoints(volumeIndex, EnumVolumeReparseCallback, Context);
                    break;
                case ID_REPARSE_OBJID:
                    EnumerateVolumeObjectIds(volumeIndex, EnumVolumeObjectIdCallback, Context);
                    break;
                case ID_REPARSE_SDDL:
                    EnumerateVolumeSecurityDescriptors(volumeIndex, EnumVolumeSecurityDescriptorsCallback, Context);
                    break;
                }
            }
        }
    }

    return TRUE;
}

NTSTATUS EnumerateVolumeDirectoryObjects(
    _In_opt_ PVOID Context
    )
{
    PREPARSE_WINDOW_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING name;

    RtlInitUnicodeString(&name, L"\\Device");
    InitializeObjectAttributes(
        &oa, 
        &name, 
        0, 
        NULL, 
        NULL
        );

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &oa
        );

    if (NT_SUCCESS(status))
    {
        if (context)
        {
            context->Count = 0;
        }

        PhEnumDirectoryObjects(
            directoryHandle, 
            EnumDirectoryObjectsCallback, 
            Context
            );

        NtClose(directoryHandle);
    }

    return status;
}

typedef struct _REPARSE_SECURITYID_CONTEXT
{
    HWND ListViewHandle;
    PPH_LIST FileList;
    PH_LAYOUT_MANAGER LayoutManager;
} REPARSE_SECURITYID_CONTEXT, *PREPARSE_SECURITYID_CONTEXT;

INT_PTR CALLBACK FindSecurityIdsDlgProc(
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
    }

    return FALSE;
}

INT_PTR CALLBACK ReparseDlgProc(
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
            NTSTATUS status;

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_REPARSE_LIST);

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

            PhCenterWindow(hwndDlg, PhMainWndHandle);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_REPARSE_WINDOW_POSITION, SETTING_NAME_REPARSE_WINDOW_SIZE, hwndDlg);

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

            status = EnumerateVolumeDirectoryObjects(context);

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(NULL, L"Unable to enumerate the objects.", status, 0);
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_DESTROY:
        {
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

            PhFree(context);
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
                    NTSTATUS status;

                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    ListView_DeleteAllItems(context->ListViewHandle);
                    status = EnumerateVolumeDirectoryObjects(context);
                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

                    if (!NT_SUCCESS(status))
                    {
                        PhShowStatus(hwndDlg, L"Unable to enumerate the objects.", status, 0);
                    }
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
                            PhMainWndHandle,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            point.x,
                            point.y
                            );

                        if (selectedItem && selectedItem->Id != -1)
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
                                        INT index = PhFindListViewItemByParam(context->ListViewHandle, -1, entry);

                                        if (index != -1)
                                        {
                                            switch (context->MenuItemIndex)
                                            {
                                            case ID_REPARSE_POINTS:
                                                {
                                                    NTSTATUS status;

                                                    status = PhDeleteFileReparsePoint(entry);

                                                    if (NT_SUCCESS(status))
                                                    {
                                                        PhRemoveListViewItem(context->ListViewHandle, index);
                                                    }
                                                    else
                                                    {
                                                        PhShowStatus(hwndDlg, L"Unable to remove the reparse point.", status, 0);
                                                    }
                                                }
                                                break;
                                            case ID_REPARSE_OBJID:
                                                {
                                                    NTSTATUS status;

                                                    status = PhDeleteFileObjectId(entry);

                                                    if (NT_SUCCESS(status))
                                                    {
                                                        PhRemoveListViewItem(context->ListViewHandle, index);
                                                    }
                                                    else
                                                    {
                                                        PhShowStatus(hwndDlg, L"Unable to remove the object identifier.", status, 0);
                                                    }
                                                }
                                                break;
                                            case ID_REPARSE_SDDL:
                                                {
                                                    PPH_LIST fileNames;

                                                    if (fileNames = FindVolumeFilesWithSecurityId(entry->RootDirectory, (ULONG)entry->FileReference))
                                                    {
                                                        DialogBoxParam(
                                                            PluginInstance->DllBase,
                                                            MAKEINTRESOURCE(IDD_REPARSEDIALOG),
                                                            NULL,
                                                            FindSecurityIdsDlgProc,
                                                            (LPARAM)fileNames
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
    }

    return FALSE;
}

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_HNDLPRV_H
#define PH_HNDLPRV_H

extern PPH_OBJECT_TYPE PhHandleProviderType;
extern PPH_OBJECT_TYPE PhHandleItemType;

// begin_phapppub
#define PH_HANDLE_FILE_SHARED_READ 0x1
#define PH_HANDLE_FILE_SHARED_WRITE 0x2
#define PH_HANDLE_FILE_SHARED_DELETE 0x4
#define PH_HANDLE_FILE_SHARED_MASK 0x7

typedef struct _PH_HANDLE_ITEM
{
    PH_HASH_ENTRY HashEntry;

    HANDLE Handle;
    PVOID Object;
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG TypeIndex;
    ULONG FileFlags;

    PPH_STRING TypeName;
    PPH_STRING ObjectName;
    PPH_STRING BestObjectName;

    WCHAR HandleString[PH_PTR_STR_LEN_1];
    WCHAR GrantedAccessString[PH_PTR_STR_LEN_1];
    WCHAR ObjectString[PH_PTR_STR_LEN_1];
} PH_HANDLE_ITEM, *PPH_HANDLE_ITEM;

typedef struct _PH_HANDLE_PROVIDER
{
    PPH_HASH_ENTRY *HandleHashSet;
    ULONG HandleHashSetSize;
    ULONG HandleHashSetCount;
    PH_QUEUED_LOCK HandleHashSetLock;

    PH_CALLBACK HandleAddedEvent;
    PH_CALLBACK HandleModifiedEvent;
    PH_CALLBACK HandleRemovedEvent;
    PH_CALLBACK HandleUpdatedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;

    PPH_HASHTABLE TempListHashtable;
    NTSTATUS RunStatus;
} PH_HANDLE_PROVIDER, *PPH_HANDLE_PROVIDER;
// end_phapppub

PPH_HANDLE_PROVIDER PhCreateHandleProvider(
    _In_ HANDLE ProcessId
    );

PPH_HANDLE_ITEM PhCreateHandleItem(
    _In_opt_ PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle
    );

PPH_HANDLE_ITEM PhReferenceHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ HANDLE Handle
    );

VOID PhDereferenceAllHandleItems(
    _In_ PPH_HANDLE_PROVIDER HandleProvider
    );

VOID PhHandleProviderUpdate(
    _In_ PVOID Object
    );

#endif

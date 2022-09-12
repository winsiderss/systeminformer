/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2020
 *
 */

#include "dn.h"
#include "clr/dbgappdomain.h"
#include "clr/ipcheader.h"
#include "clr/ipcshared.h"

PVOID GetLegacyBlockTableEntry(
    _In_ BOOLEAN Wow64,
    _In_ PVOID IpcBlockAddress,
    _In_ ULONG EntryId
    )
{
    if (Wow64)
    {
        LegacyPrivateIPCControlBlock_Wow64* IpcBlock = IpcBlockAddress;

        // skip over directory (variable size)
        ULONG offsetBase = IPC_ENTRY_OFFSET_BASE_X86 + IpcBlock->FullIPCHeader.Header.NumEntries * sizeof(IPCEntry);
        // Directory has offset in bytes of block
        ULONG offsetEntry = IpcBlock->FullIPCHeader.EntryTable[EntryId].Offset;

        return PTR_ADD_OFFSET(IpcBlock, UInt32Add32To64(offsetBase, offsetEntry));
    }
    else
    {
        LegacyPrivateIPCControlBlock* IpcBlock = IpcBlockAddress;

        // skip over directory (variable size)
        ULONG offsetBase = IPC_ENTRY_OFFSET_BASE_X64 + IpcBlock->FullIPCHeader.Header.NumEntries * sizeof(IPCEntry);
        // Directory has offset in bytes of block
        ULONG offsetEntry = IpcBlock->FullIPCHeader.EntryTable[EntryId].Offset;

        return PTR_ADD_OFFSET(IpcBlock, UInt32Add32To64(offsetBase, offsetEntry));
    }
}

PVOID GetPerfIpcBlock_V2(
    _In_ BOOLEAN Wow64,
    _In_ PVOID BlockTableAddress
    )
{
    if (Wow64)
    {
        LegacyPublicIPCControlBlock_Wow64* ipcLegacyBlockTable_Wow64 = BlockTableAddress;

        if (ipcLegacyBlockTable_Wow64->FullIPCHeaderLegacyPublic.Header.Version > VER_LEGACYPUBLIC_IPC_BLOCK)
        {
            return NULL;
        }

        return &ipcLegacyBlockTable_Wow64->PerfIpcBlock;
    }
    else
    {
        LegacyPublicIPCControlBlock* ipcLegacyBlockTable = BlockTableAddress;

        if (ipcLegacyBlockTable->FullIPCHeaderLegacyPublic.Header.Version > VER_IPC_BLOCK)
        {
            return NULL;
        }

        return &ipcLegacyBlockTable->PerfIpcBlock;
    }
}

PVOID GetPerfIpcBlock_V4(
    _In_ BOOLEAN Wow64,
    _In_ PVOID BlockTableAddress
    )
{
    if (Wow64)
    {
        IPCControlBlockTable_Wow64* ipcBlockTable_Wow64 = BlockTableAddress;

        if (ipcBlockTable_Wow64->Blocks->Header.Version > VER_IPC_BLOCK)
        {
            return NULL;
        }

        return &ipcBlockTable_Wow64->Blocks->PerfIpcBlock;
    }
    else
    {
        IPCControlBlockTable_Wow64* ipcBlockTable = BlockTableAddress;

        if (ipcBlockTable->Blocks->Header.Version > VER_IPC_BLOCK)
        {
            return NULL;
        }

        return &ipcBlockTable->Blocks->PerfIpcBlock;
    }
}

PPH_LIST EnumerateAppDomainIpcBlock(
    _In_ HANDLE ProcessHandle,
    _In_ AppDomainEnumerationIPCBlock* AppDomainIpcBlock
    )
{
    LARGE_INTEGER timeout;
    SIZE_T appDomainInfoBlockLength;
    HANDLE legacyPrivateBlockMutexHandle = NULL;
    AppDomainEnumerationIPCBlock tempBlock;
    AppDomainInfo* appDomainInfoBlock = NULL;
    PPH_LIST appDomainsList = PhCreateList(1);

    // If the mutex isn't filled in, the CLR is either starting up or shutting down
    if (!AppDomainIpcBlock->Mutex)
    {
        goto CleanupExit;
    }

    // Dup the valid mutex handle into this process.
    if (!NT_SUCCESS(NtDuplicateObject(
        ProcessHandle,
        AppDomainIpcBlock->Mutex,
        NtCurrentProcess(),
        &legacyPrivateBlockMutexHandle,
        GENERIC_ALL,
        0,
        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
        )))
    {
        goto CleanupExit;
    }

    // Acquire the mutex, only waiting two seconds.
    // We can't actually gaurantee that the target put a mutex object in here.
    if (NtWaitForSingleObject(
        legacyPrivateBlockMutexHandle,
        FALSE,
        PhTimeoutFromMilliseconds(&timeout, 2000)
        ) == STATUS_WAIT_0)
    {
        // Make sure the mutex handle is still valid. If its not, then we lost a shutdown race.
        if (!AppDomainIpcBlock->Mutex)
        {
            goto CleanupExit;
        }
    }
    else
    {
        // Again, landing here is most probably a shutdown race.
        goto CleanupExit;
    }

    // Beware: If the target pid is not properly honoring the mutex, the data in the IPC block may still shift underneath us.
    // If we get here, then hMutex is held by this process.

    // Make a copy of the IPC block so that we can gaurantee that it's not changing on us.
    memcpy_s(&tempBlock, sizeof(tempBlock), AppDomainIpcBlock, sizeof(AppDomainEnumerationIPCBlock));

    // It's possible the process will not have any appdomains.
    if ((tempBlock.ListOfAppDomains == NULL) != (tempBlock.SizeInBytes == 0))
    {
        goto CleanupExit;
    }

    // All the data in the IPC block is signed integers. They should never be negative,
    // so check that now.
    if ((tempBlock.TotalSlots < 0) ||
        (tempBlock.NumOfUsedSlots < 0) ||
        (tempBlock.LastFreedSlot < 0) ||
        (tempBlock.SizeInBytes < 0) ||
        (tempBlock.ProcessNameLengthInBytes < 0))
    {
        goto CleanupExit;
    }

    // Allocate memory to read the remote process' memory into
    appDomainInfoBlockLength = tempBlock.SizeInBytes;

    // Check other invariants.
    if (appDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo))
    {
        goto CleanupExit;
    }

    appDomainInfoBlock = PhAllocate(appDomainInfoBlockLength);
    memset(appDomainInfoBlock, 0, appDomainInfoBlockLength);

    if (!NT_SUCCESS(NtReadVirtualMemory(
        ProcessHandle,
        tempBlock.ListOfAppDomains,
        appDomainInfoBlock,
        appDomainInfoBlockLength,
        NULL
        )))
    {
        PhFree(appDomainInfoBlock);
        goto CleanupExit;
    }

    // Collect all the AppDomain names into a list of strings.
    for (INT i = 0; i < tempBlock.NumOfUsedSlots; i++)
    {
        SIZE_T appDomainNameLength;
        PVOID appDomainName;

        if (!appDomainInfoBlock[i].AppDomainName)
            continue;

        // Should be positive, and at least have a null-terminator character.
        if (appDomainInfoBlock[i].NameLengthInBytes <= 1)
            continue;

        // Make sure buffer has right geometry.
        if (appDomainInfoBlock[i].NameLengthInBytes < 0)
            continue;

        // If it's not on a WCHAR boundary, then we may have a 1-byte buffer-overflow.
        appDomainNameLength = appDomainInfoBlock[i].NameLengthInBytes / sizeof(WCHAR);

        if ((appDomainNameLength * sizeof(WCHAR)) != (SIZE_T)appDomainInfoBlock[i].NameLengthInBytes)
            continue;

        // It should at least have 1 char for the null terminator.
        if (appDomainNameLength < 1)
            continue;

        // We know the string is a well-formed null-terminated string,
        // but beyond that, we can't verify that the data is actually truthful.
        appDomainName = PhAllocate(appDomainInfoBlock[i].NameLengthInBytes + 1);
        memset(appDomainName, 0, appDomainInfoBlock[i].NameLengthInBytes + 1);

        if (!NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            appDomainInfoBlock[i].AppDomainName,
            appDomainName,
            appDomainInfoBlock[i].NameLengthInBytes,
            NULL
            )))
        {
            PhFree(appDomainName);
            continue;
        }

        PhAddItemList(appDomainsList, appDomainName);
    }

CleanupExit:
    if (appDomainInfoBlock)
    {
        PhFree(appDomainInfoBlock);
    }

    if (legacyPrivateBlockMutexHandle)
    {
        NtReleaseMutant(legacyPrivateBlockMutexHandle, NULL);
        NtClose(legacyPrivateBlockMutexHandle);
    }

    return appDomainsList;
}

PPH_LIST EnumerateAppDomainIpcBlockWow64(
    _In_ HANDLE ProcessHandle,
    _In_ AppDomainEnumerationIPCBlock_Wow64* AppDomainIpcBlock
    )
{
    LARGE_INTEGER timeout;
    SIZE_T appDomainInfoBlockLength;
    HANDLE legacyPrivateBlockMutexHandle = NULL;
    AppDomainEnumerationIPCBlock_Wow64 tempBlock;
    AppDomainInfo_Wow64* appDomainInfoBlock = NULL;
    PPH_LIST appDomainsList = PhCreateList(1);

    // If the mutex isn't filled in, the CLR is either starting up or shutting down
    if (!AppDomainIpcBlock->Mutex)
    {
        goto CleanupExit;
    }

    // Dup the valid mutex handle into this process.
    if (!NT_SUCCESS(NtDuplicateObject(
        ProcessHandle,
        UlongToHandle(AppDomainIpcBlock->Mutex),
        NtCurrentProcess(),
        &legacyPrivateBlockMutexHandle,
        GENERIC_ALL,
        0,
        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
        )))
    {
        goto CleanupExit;
    }

    // Acquire the mutex, only waiting two seconds.
    // We can't actually gaurantee that the target put a mutex object in here.
    if (NtWaitForSingleObject(
        legacyPrivateBlockMutexHandle,
        FALSE,
        PhTimeoutFromMilliseconds(&timeout, 2000)
        ) == STATUS_WAIT_0)
    {
        // Make sure the mutex handle is still valid. If its not, then we lost a shutdown race.
        if (!AppDomainIpcBlock->Mutex)
        {
            goto CleanupExit;
        }
    }
    else
    {
        // Again, landing here is most probably a shutdown race.
        goto CleanupExit;
    }

    // Beware: If the target pid is not properly honoring the mutex, the data in the IPC block may still shift underneath us.
    // If we get here, then hMutex is held by this process.

    // Make a copy of the IPC block so that we can gaurantee that it's not changing on us.
    memcpy_s(&tempBlock, sizeof(tempBlock), AppDomainIpcBlock, sizeof(AppDomainEnumerationIPCBlock_Wow64));

    // It's possible the process will not have any appdomains.
    if ((tempBlock.ListOfAppDomains == 0) != (tempBlock.SizeInBytes == 0))
    {
        goto CleanupExit;
    }

    // All the data in the IPC block is signed integers. They should never be negative,
    // so check that now.
    if ((tempBlock.TotalSlots < 0) ||
        (tempBlock.NumOfUsedSlots < 0) ||
        (tempBlock.LastFreedSlot < 0) ||
        (tempBlock.SizeInBytes < 0) ||
        (tempBlock.ProcessNameLengthInBytes < 0))
    {
        goto CleanupExit;
    }

    // Allocate memory to read the remote process' memory into
    appDomainInfoBlockLength = tempBlock.SizeInBytes;

    // Check other invariants.
    if (appDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo_Wow64))
    {
        goto CleanupExit;
    }

    appDomainInfoBlock = PhAllocate(appDomainInfoBlockLength);
    memset(appDomainInfoBlock, 0, appDomainInfoBlockLength);

    if (!NT_SUCCESS(NtReadVirtualMemory(
        ProcessHandle,
        UlongToPtr(tempBlock.ListOfAppDomains),
        appDomainInfoBlock,
        appDomainInfoBlockLength,
        NULL
        )))
    {
        PhFree(appDomainInfoBlock);
        goto CleanupExit;
    }

    // Collect all the AppDomain names into a list of strings.
    for (INT i = 0; i < tempBlock.NumOfUsedSlots; i++)
    {
        SIZE_T appDomainNameLength;
        PVOID appDomainName;

        if (!appDomainInfoBlock[i].AppDomainName)
            continue;

        // Should be positive, and at least have a null-terminator character.
        if (appDomainInfoBlock[i].NameLengthInBytes <= 1)
            continue;

        // Make sure buffer has right geometry.
        if (appDomainInfoBlock[i].NameLengthInBytes < 0)
            continue;

        // If it's not on a WCHAR boundary, then we may have a 1-byte buffer-overflow.
        appDomainNameLength = appDomainInfoBlock[i].NameLengthInBytes / sizeof(WCHAR);

        if ((appDomainNameLength * sizeof(WCHAR)) != (SIZE_T)appDomainInfoBlock[i].NameLengthInBytes)
            continue;

        // It should at least have 1 char for the null terminator.
        if (appDomainNameLength < 1)
            continue;

        // We know the string is a well-formed null-terminated string,
        // but beyond that, we can't verify that the data is actually truthful.
        appDomainName = PhAllocate(appDomainInfoBlock[i].NameLengthInBytes + 1);
        memset(appDomainName, 0, appDomainInfoBlock[i].NameLengthInBytes + 1);

        if (!NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(appDomainInfoBlock[i].AppDomainName),
            appDomainName,
            appDomainInfoBlock[i].NameLengthInBytes,
            NULL
            )))
        {
            PhFree(appDomainName);
            continue;
        }

        PhAddItemList(appDomainsList, appDomainName);
    }

CleanupExit:

    if (appDomainInfoBlock)
    {
        PhFree(appDomainInfoBlock);
    }

    if (legacyPrivateBlockMutexHandle)
    {
        NtReleaseMutant(legacyPrivateBlockMutexHandle, NULL);
        NtClose(legacyPrivateBlockMutexHandle);
    }

    return appDomainsList;
}

_Success_(return)
BOOLEAN OpenDotNetPublicControlBlock_V2(
    _In_ HANDLE ProcessId,
    _Out_ PVOID* BlockTableAddress
    )
{
    HANDLE blockTableHandle = NULL;
    PVOID blockTableAddress = NULL;
    PPH_STRING legacyPublicIPCBlockName;
    UNICODE_STRING sectionNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    LARGE_INTEGER sectionOffset = { 0 };
    SIZE_T viewSize = 0;

    legacyPublicIPCBlockName = PhaFormatString(
        L"\\BaseNamedObjects\\" CorLegacyPublicIPCBlock,
        HandleToUlong(ProcessId)
        );

    if (!PhStringRefToUnicodeString(&legacyPublicIPCBlockName->sr, &sectionNameUs))
        return FALSE;

    InitializeObjectAttributes(
        &objectAttributes,
        &sectionNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(NtOpenSection(
        &blockTableHandle,
        SECTION_MAP_READ,
        &objectAttributes
        )))
    {
        return FALSE;
    }

    if (NT_SUCCESS(NtMapViewOfSection(
        blockTableHandle,
        NtCurrentProcess(),
        &blockTableAddress,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        )))
    {
        *BlockTableAddress = blockTableAddress;

        NtClose(blockTableHandle);
        return TRUE;
    }

    if (blockTableHandle)
        NtClose(blockTableHandle);

    if (blockTableAddress)
        NtUnmapViewOfSection(NtCurrentProcess(), blockTableAddress);

    return FALSE;
}

_Success_(return)
BOOLEAN OpenDotNetPublicControlBlock_V4(
    _In_ BOOLEAN IsImmersive,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _Out_ PVOID* BlockTableAddress
    )
{
    static SID everyoneSid = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, { SECURITY_WORLD_RID } };
    BOOLEAN result = FALSE;
    PPH_STRING legacyBoundaryDescriptorName;
    POBJECT_BOUNDARY_DESCRIPTOR boundaryDescriptorHandle = NULL;
    HANDLE privateNamespaceHandle = NULL;
    HANDLE blockTableHandle = NULL;
    HANDLE tokenHandle = NULL;
    PVOID blockTableAddress = NULL;
    LARGE_INTEGER sectionOffset = { 0 };
    SIZE_T viewSize = 0;
    UNICODE_STRING prefixNameUs;
    UNICODE_STRING sectionNameUs;
    UNICODE_STRING boundaryNameUs;
    OBJECT_ATTRIBUTES namespaceObjectAttributes;
    OBJECT_ATTRIBUTES sectionObjectAttributes;
    PTOKEN_APPCONTAINER_INFORMATION appContainerInfo = NULL;
    SID_IDENTIFIER_AUTHORITY SIDWorldAuth = SECURITY_WORLD_SID_AUTHORITY;

    legacyBoundaryDescriptorName = PhFormatString(
        CorSxSBoundaryDescriptor,
        HandleToUlong(ProcessId)
        );

    if (!PhStringRefToUnicodeString(&legacyBoundaryDescriptorName->sr, &boundaryNameUs))
        goto CleanupExit;

    if (!(boundaryDescriptorHandle = RtlCreateBoundaryDescriptor(&boundaryNameUs, 0)))
        goto CleanupExit;

    if (!NT_SUCCESS(RtlAddSIDToBoundaryDescriptor(&boundaryDescriptorHandle, &everyoneSid)))
        goto CleanupExit;

    if (IsImmersive && PhWindowsVersion >= WINDOWS_8)
    {
        if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
        {
            if (!NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenAppContainerSid, &appContainerInfo)))
                goto CleanupExit;

            if (!NT_SUCCESS(RtlAddSIDToBoundaryDescriptor(&boundaryDescriptorHandle, appContainerInfo->TokenAppContainer)))
                goto CleanupExit;
        }
    }

    RtlInitUnicodeString(&prefixNameUs, CorSxSReaderPrivateNamespacePrefix);
    InitializeObjectAttributes(
        &namespaceObjectAttributes,
        &prefixNameUs,
        OBJ_CASE_INSENSITIVE,
        boundaryDescriptorHandle,
        NULL
        );

    if (!NT_SUCCESS(NtOpenPrivateNamespace(
        &privateNamespaceHandle,
        MAXIMUM_ALLOWED,
        &namespaceObjectAttributes,
        boundaryDescriptorHandle
        )))
    {
        goto CleanupExit;
    }

    RtlInitUnicodeString(&sectionNameUs, CorSxSVistaPublicIPCBlock);
    InitializeObjectAttributes(
        &sectionObjectAttributes,
        &sectionNameUs,
        OBJ_CASE_INSENSITIVE,
        privateNamespaceHandle,
        NULL
        );

    if (!NT_SUCCESS(NtOpenSection(
        &blockTableHandle,
        SECTION_MAP_READ,
        &sectionObjectAttributes
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(NtMapViewOfSection(
        blockTableHandle,
        NtCurrentProcess(),
        &blockTableAddress,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        )))
    {
        goto CleanupExit;
    }

    *BlockTableAddress = blockTableAddress;

    result = TRUE;

CleanupExit:

    if (!result)
    {
        if (blockTableAddress)
        {
            NtUnmapViewOfSection(NtCurrentProcess(), blockTableAddress);
        }

        *BlockTableAddress = NULL;
    }

    if (blockTableHandle)
    {
        NtClose(blockTableHandle);
    }

    if (tokenHandle)
    {
        NtClose(tokenHandle);
    }

    if (appContainerInfo)
    {
        PhFree(appContainerInfo);
    }

    if (privateNamespaceHandle)
    {
        NtClose(privateNamespaceHandle);
    }

    if (boundaryDescriptorHandle)
    {
        RtlDeleteBoundaryDescriptor(boundaryDescriptorHandle);
    }

    PhDereferenceObject(legacyBoundaryDescriptorName);

    return result;
}

PPH_LIST QueryDotNetAppDomainsForPid_V2(
    _In_ BOOLEAN Wow64,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    )
{
    PPH_LIST appDomainsList = NULL;
    PPH_STRING legacyPrivateBlockName;
    HANDLE legacyPrivateBlockHandle = NULL;
    PVOID ipcControlBlockTable = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING sectionNameUs;
    LARGE_INTEGER sectionOffset;
    SIZE_T viewSize;

    legacyPrivateBlockName = PhFormatString(
        L"\\BaseNamedObjects\\" CorLegacyPrivateIPCBlock,
        HandleToUlong(ProcessId)
        );

    if (!PhStringRefToUnicodeString(&legacyPrivateBlockName->sr, &sectionNameUs))
        goto CleanupExit;

    InitializeObjectAttributes(
        &objectAttributes,
        &sectionNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(NtOpenSection(
        &legacyPrivateBlockHandle,
        SECTION_MAP_READ,
        &objectAttributes
        )))
    {
        goto CleanupExit;
    }

    viewSize = 0;
    sectionOffset.QuadPart = 0;

    if (!NT_SUCCESS(NtMapViewOfSection(
        legacyPrivateBlockHandle,
        NtCurrentProcess(),
        &ipcControlBlockTable,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        )))
    {
        goto CleanupExit;
    }

    if (Wow64)
    {
        LegacyPrivateIPCControlBlock_Wow64* legacyPrivateBlock;
        AppDomainEnumerationIPCBlock_Wow64* appDomainEnumBlock;

        legacyPrivateBlock = (LegacyPrivateIPCControlBlock_Wow64*)ipcControlBlockTable;

        // NOTE: .NET 2.0 processes do not have the IPC_FLAG_INITIALIZED flag.

        // Check the IPCControlBlock version is valid.
        if (legacyPrivateBlock->FullIPCHeader.Header.Version > VER_LEGACYPRIVATE_IPC_BLOCK)
        {
            goto CleanupExit;
        }

        appDomainEnumBlock = GetLegacyBlockTableEntry(
            Wow64,
            ipcControlBlockTable,
            eLegacyPrivateIPC_AppDomain
            );

        appDomainsList = EnumerateAppDomainIpcBlockWow64(
            ProcessHandle,
            appDomainEnumBlock
            );
    }
    else
    {
        LegacyPrivateIPCControlBlock* legacyPrivateBlock;
        AppDomainEnumerationIPCBlock* appDomainEnumBlock;

        legacyPrivateBlock = (LegacyPrivateIPCControlBlock*)ipcControlBlockTable;

        // NOTE: .NET 2.0 processes do not have the IPC_FLAG_INITIALIZED flag.

        // Check the IPCControlBlock version is valid.
        if (legacyPrivateBlock->FullIPCHeader.Header.Version > VER_LEGACYPRIVATE_IPC_BLOCK)
        {
            goto CleanupExit;
        }

        appDomainEnumBlock = GetLegacyBlockTableEntry(
            Wow64,
            ipcControlBlockTable,
            eLegacyPrivateIPC_AppDomain
            );

        appDomainsList = EnumerateAppDomainIpcBlock(
            ProcessHandle,
            appDomainEnumBlock
            );
    }

CleanupExit:

    if (ipcControlBlockTable)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), ipcControlBlockTable);
    }

    if (legacyPrivateBlockHandle)
    {
        NtClose(legacyPrivateBlockHandle);
    }

    PhDereferenceObject(legacyPrivateBlockName);

    return appDomainsList;
}

PPH_LIST QueryDotNetAppDomainsForPid_V4(
    _In_ BOOLEAN Wow64,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    )
{
    PPH_LIST appDomainsList = NULL;
    PPH_STRING legacyPrivateBlockName;
    HANDLE legacyPrivateBlockHandle = NULL;
    PVOID ipcControlBlockTable = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING sectionNameUs;
    LARGE_INTEGER sectionOffset;
    SIZE_T viewSize;
    
    legacyPrivateBlockName = PhFormatString(
        L"\\BaseNamedObjects\\" CorLegacyPrivateIPCBlockTempV4,
        HandleToUlong(ProcessId)
        );

    if (!PhStringRefToUnicodeString(&legacyPrivateBlockName->sr, &sectionNameUs))
        goto CleanupExit;

    InitializeObjectAttributes(
        &objectAttributes,
        &sectionNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(NtOpenSection(
        &legacyPrivateBlockHandle,
        SECTION_MAP_READ,
        &objectAttributes
        )))
    {
        goto CleanupExit;
    }

    viewSize = 0;
    sectionOffset.QuadPart = 0;

    if (!NT_SUCCESS(NtMapViewOfSection(
        legacyPrivateBlockHandle,
        NtCurrentProcess(),
        &ipcControlBlockTable,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        )))
    {
        goto CleanupExit;
    }

    if (Wow64)
    {
        LegacyPrivateIPCControlBlock_Wow64* legacyPrivateBlock;
        AppDomainEnumerationIPCBlock_Wow64* appDomainEnumBlock;

        legacyPrivateBlock = (LegacyPrivateIPCControlBlock_Wow64*)ipcControlBlockTable;
        appDomainEnumBlock = &legacyPrivateBlock->AppDomainBlock;

        // Check the IPCControlBlock is initialized.
        if ((legacyPrivateBlock->FullIPCHeader.Header.Flags & IPC_FLAG_INITIALIZED) != IPC_FLAG_INITIALIZED)
        {
            goto CleanupExit;
        }

        // Check the IPCControlBlock version is valid.
        if (legacyPrivateBlock->FullIPCHeader.Header.Version > VER_LEGACYPRIVATE_IPC_BLOCK)
        {
            goto CleanupExit;
        }

        appDomainsList = EnumerateAppDomainIpcBlockWow64(
            ProcessHandle,
            appDomainEnumBlock
            );
    }
    else
    {
        LegacyPrivateIPCControlBlock* legacyPrivateBlock;
        AppDomainEnumerationIPCBlock* appDomainEnumBlock;

        legacyPrivateBlock = (LegacyPrivateIPCControlBlock*)ipcControlBlockTable;
        appDomainEnumBlock = &legacyPrivateBlock->AppDomainBlock;

        // Check the IPCControlBlock is initialized.
        if ((legacyPrivateBlock->FullIPCHeader.Header.Flags & IPC_FLAG_INITIALIZED) != IPC_FLAG_INITIALIZED)
        {
            goto CleanupExit;
        }

        // Check the IPCControlBlock version is valid.
        if (legacyPrivateBlock->FullIPCHeader.Header.Version > VER_LEGACYPRIVATE_IPC_BLOCK)
        {
            goto CleanupExit;
        }

        appDomainsList = EnumerateAppDomainIpcBlock(
            ProcessHandle,
            appDomainEnumBlock
            );
    }

CleanupExit:

    if (ipcControlBlockTable)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), ipcControlBlockTable);
    }

    if (legacyPrivateBlockHandle)
    {
        NtClose(legacyPrivateBlockHandle);
    }

    PhDereferenceObject(legacyPrivateBlockName);

    return appDomainsList;
}

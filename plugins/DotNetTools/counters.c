/*
 * Process Hacker .NET Tools -
 *   IPC support functions
 *
 * Copyright (C) 2015-2016 dmex
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

#include "dn.h"
#include "clr/dbgappdomain.h"
#include "clr/ipcheader.h"
#include "clr/ipcshared.h"

typedef PVOID (NTAPI* _RtlCreateBoundaryDescriptor)(
    _In_ PUNICODE_STRING Name,
    _In_ ULONG Flags
    );

typedef NTSTATUS (NTAPI* _RtlAddSIDToBoundaryDescriptor)(
    _Inout_ PVOID* BoundaryDescriptor,
    _In_ PSID RequiredSid
    );

typedef VOID (NTAPI* _RtlDeleteBoundaryDescriptor)(
    _In_ PVOID BoundaryDescriptor
    );

typedef NTSTATUS (WINAPI* _NtOpenPrivateNamespace)(
    _Out_ PHANDLE NamespaceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PVOID BoundaryDescriptor
    );

static _NtOpenPrivateNamespace NtOpenPrivateNamespace_I = NULL;
static _RtlCreateBoundaryDescriptor RtlCreateBoundaryDescriptor_I = NULL;
static _RtlDeleteBoundaryDescriptor RtlDeleteBoundaryDescriptor_I = NULL;
static _RtlAddSIDToBoundaryDescriptor RtlAddSIDToBoundaryDescriptor_I = NULL;

PPH_STRING GeneratePrivateName(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"\\BaseNamedObjects\\" CorLegacyPrivateIPCBlock, HandleToUlong(ProcessId));
}

PPH_STRING GeneratePrivateNameV4(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"\\BaseNamedObjects\\" CorLegacyPrivateIPCBlockTempV4, HandleToUlong(ProcessId));
}

PPH_STRING GenerateLegacyPublicName(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"\\BaseNamedObjects\\" CorLegacyPublicIPCBlock, HandleToUlong(ProcessId));
}

PPH_STRING GenerateSxSPublicNameV4(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"\\BaseNamedObjects\\" CorSxSPublicIPCBlock, HandleToUlong(ProcessId));
}

PPH_STRING GenerateBoundaryDescriptorName(_In_ HANDLE ProcessId)
{
    return PhaFormatString(CorSxSBoundaryDescriptor, HandleToUlong(ProcessId));
}

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

        return ((PBYTE)IpcBlock) + offsetBase + offsetEntry;
    }
    else
    {
        LegacyPrivateIPCControlBlock* IpcBlock = IpcBlockAddress;

        // skip over directory (variable size)
        ULONG offsetBase = IPC_ENTRY_OFFSET_BASE_X64 + IpcBlock->FullIPCHeader.Header.NumEntries * sizeof(IPCEntry);
        // Directory has offset in bytes of block
        ULONG offsetEntry = IpcBlock->FullIPCHeader.EntryTable[EntryId].Offset;

        return ((PBYTE)IpcBlock) + offsetBase + offsetEntry;
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

PPH_LIST EnumAppDomainIpcBlock(
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

    __try
    {
        // If the mutex isn't filled in, the CLR is either starting up or shutting down
        if (!AppDomainIpcBlock->Mutex)
        {
            __leave;
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
            __leave;
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
                __leave;
            }
        }
        else
        {
            // Again, landing here is most probably a shutdown race.
            __leave;
        }

        // Beware: If the target pid is not properly honoring the mutex, the data in the IPC block may still shift underneath us.
        // If we get here, then hMutex is held by this process.

        // Make a copy of the IPC block so that we can gaurantee that it's not changing on us.
        memcpy(&tempBlock, AppDomainIpcBlock, sizeof(AppDomainEnumerationIPCBlock));

        // It's possible the process will not have any appdomains.
        if ((tempBlock.ListOfAppDomains == NULL) != (tempBlock.SizeInBytes == 0))
        {
            __leave;
        }

        // All the data in the IPC block is signed integers. They should never be negative,
        // so check that now.
        if ((tempBlock.TotalSlots < 0) ||
            (tempBlock.NumOfUsedSlots < 0) ||
            (tempBlock.LastFreedSlot < 0) ||
            (tempBlock.SizeInBytes < 0) ||
            (tempBlock.ProcessNameLengthInBytes < 0))
        {
            __leave;
        }

        // Allocate memory to read the remote process' memory into
        appDomainInfoBlockLength = tempBlock.SizeInBytes;

        // Check other invariants.
        if (appDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo))
        {
            __leave;
        }

        appDomainInfoBlock = (AppDomainInfo*)PhAllocate(appDomainInfoBlockLength);
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
            __leave;
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

            if ((appDomainNameLength * sizeof(WCHAR)) != appDomainInfoBlock[i].NameLengthInBytes)
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
    }
    __finally
    {
        if (appDomainInfoBlock)
        {
            PhFree(appDomainInfoBlock);
        }

        if (legacyPrivateBlockMutexHandle)
        {
            NtReleaseMutant(legacyPrivateBlockMutexHandle, NULL);
            NtClose(legacyPrivateBlockMutexHandle);
        }
    }

    return appDomainsList;
}

PPH_LIST EnumAppDomainIpcBlockWow64(
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

    __try
    {
        // If the mutex isn't filled in, the CLR is either starting up or shutting down
        if (!AppDomainIpcBlock->Mutex)
        {
            __leave;
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
            __leave;
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
                __leave;
            }
        }
        else
        {
            // Again, landing here is most probably a shutdown race.
            __leave;
        }

        // Beware: If the target pid is not properly honoring the mutex, the data in the IPC block may still shift underneath us.
        // If we get here, then hMutex is held by this process.

        // Make a copy of the IPC block so that we can gaurantee that it's not changing on us.
        memcpy(&tempBlock, AppDomainIpcBlock, sizeof(AppDomainEnumerationIPCBlock_Wow64));

        // It's possible the process will not have any appdomains.
        if ((tempBlock.ListOfAppDomains == 0) != (tempBlock.SizeInBytes == 0))
        {
            __leave;
        }

        // All the data in the IPC block is signed integers. They should never be negative,
        // so check that now.
        if ((tempBlock.TotalSlots < 0) ||
            (tempBlock.NumOfUsedSlots < 0) ||
            (tempBlock.LastFreedSlot < 0) ||
            (tempBlock.SizeInBytes < 0) ||
            (tempBlock.ProcessNameLengthInBytes < 0))
        {
            __leave;
        }

        // Allocate memory to read the remote process' memory into
        appDomainInfoBlockLength = tempBlock.SizeInBytes;

        // Check other invariants.
        if (appDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo_Wow64))
        {
            __leave;
        }

        appDomainInfoBlock = (AppDomainInfo_Wow64*)PhAllocate(appDomainInfoBlockLength);
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
            __leave;
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

            if ((appDomainNameLength * sizeof(WCHAR)) != appDomainInfoBlock[i].NameLengthInBytes)
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
    }
    __finally
    {
        if (appDomainInfoBlock)
        {
            PhFree(appDomainInfoBlock);
        }

        if (legacyPrivateBlockMutexHandle)
        {
            NtReleaseMutant(legacyPrivateBlockMutexHandle, NULL);
            NtClose(legacyPrivateBlockMutexHandle);
        }
    }

    return appDomainsList;
}

BOOLEAN OpenDotNetPublicControlBlock_V2(
    _In_ HANDLE ProcessId,
    _Out_ HANDLE* BlockTableHandle,
    _Out_ PVOID* BlockTableAddress
    )
{
    BOOLEAN result = FALSE;
    HANDLE blockTableHandle = NULL;
    PVOID blockTableAddress = NULL;
    UNICODE_STRING sectionNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    LARGE_INTEGER sectionOffset = { 0 };
    SIZE_T viewSize = 0;

    __try
    {
        if (!PhStringRefToUnicodeString(&GenerateLegacyPublicName(ProcessId)->sr, &sectionNameUs))
            __leave;

        InitializeObjectAttributes(
            &objectAttributes,
            &sectionNameUs,
            0,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(NtOpenSection(
            &blockTableHandle,
            SECTION_MAP_READ,
            &objectAttributes
            )))
        {
            __leave;
        }

        if (!NT_SUCCESS(NtMapViewOfSection(
            blockTableHandle,
            NtCurrentProcess(),
            &blockTableAddress,
            0,
            viewSize,
            &sectionOffset,
            &viewSize,
            ViewShare,
            0,
            PAGE_READONLY
            )))
        {
            __leave;
        }

        *BlockTableHandle = blockTableHandle;
        *BlockTableAddress = blockTableAddress;

        result = TRUE;
    }
    __finally
    {
        if (!result)
        {
            if (blockTableHandle)
            {
                NtClose(blockTableHandle);
            }

            if (blockTableAddress)
            {
                NtUnmapViewOfSection(NtCurrentProcess(), blockTableAddress);
            }

            *BlockTableHandle = NULL;
            *BlockTableAddress = NULL;
        }
    }

    return result;
}

BOOLEAN OpenDotNetPublicControlBlock_V4(
    _In_ BOOLEAN IsImmersive,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _Out_ HANDLE* BlockTableHandle,
    _Out_ PVOID* BlockTableAddress
    )
{
    BOOLEAN result = FALSE;
    PVOID boundaryDescriptorHandle = NULL;
    HANDLE privateNamespaceHandle = NULL;
    HANDLE blockTableHandle = NULL;
    HANDLE tokenHandle = NULL;
    PSID everyoneSIDHandle = NULL;
    PVOID blockTableAddress = NULL;
    LARGE_INTEGER sectionOffset = { 0 };
    SIZE_T viewSize = 0;
    PTOKEN_APPCONTAINER_INFORMATION appContainerInfo = NULL;
    SID_IDENTIFIER_AUTHORITY SIDWorldAuth = SECURITY_WORLD_SID_AUTHORITY;

    __try
    {
        if (WindowsVersion < WINDOWS_VISTA)
        {
            UNICODE_STRING sectionNameUs;
            OBJECT_ATTRIBUTES objectAttributes;

            if (!PhStringRefToUnicodeString(&GenerateSxSPublicNameV4(ProcessId)->sr, &sectionNameUs))
                __leave;

            InitializeObjectAttributes(
                &objectAttributes,
                &sectionNameUs,
                0,
                NULL,
                NULL
                );

            if (!NT_SUCCESS(NtOpenSection(
                &blockTableHandle,
                SECTION_MAP_READ,
                &objectAttributes
                )))
            {
                __leave;
            }
        }
        else
        {
            static PH_INITONCE initOnce = PH_INITONCE_INIT;
            UNICODE_STRING prefixNameUs;
            UNICODE_STRING sectionNameUs;
            UNICODE_STRING boundaryNameUs;
            OBJECT_ATTRIBUTES namespaceObjectAttributes;
            OBJECT_ATTRIBUTES sectionObjectAttributes;

            if (PhBeginInitOnce(&initOnce))
            {
                PVOID ntdll;

                ntdll = PhGetDllHandle(L"ntdll.dll");
                NtOpenPrivateNamespace_I = PhGetProcedureAddress(ntdll, "NtOpenPrivateNamespace", 0);
                RtlCreateBoundaryDescriptor_I = PhGetProcedureAddress(ntdll, "RtlCreateBoundaryDescriptor", 0);
                RtlDeleteBoundaryDescriptor_I = PhGetProcedureAddress(ntdll, "RtlDeleteBoundaryDescriptor", 0);
                RtlAddSIDToBoundaryDescriptor_I = PhGetProcedureAddress(ntdll, "RtlAddSIDToBoundaryDescriptor", 0);

                PhEndInitOnce(&initOnce);
            }

            if (!PhStringRefToUnicodeString(&GenerateBoundaryDescriptorName(ProcessId)->sr, &boundaryNameUs))
                __leave;

            if (!(boundaryDescriptorHandle = RtlCreateBoundaryDescriptor_I(&boundaryNameUs, 0)))
                __leave;

            if (!NT_SUCCESS(RtlAllocateAndInitializeSid(&SIDWorldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyoneSIDHandle)))
                __leave;

            if (!NT_SUCCESS(RtlAddSIDToBoundaryDescriptor_I(&boundaryDescriptorHandle, everyoneSIDHandle)))
                __leave;

            if (WINDOWS_HAS_IMMERSIVE && IsImmersive)
            {
                if (NT_SUCCESS(NtOpenProcessToken(&tokenHandle, TOKEN_QUERY, ProcessHandle)))
                {
                    ULONG returnLength = 0;

                    if (NtQueryInformationToken(
                        tokenHandle,
                        TokenAppContainerSid,
                        NULL,
                        0,
                        &returnLength
                        ) != STATUS_BUFFER_TOO_SMALL)
                    {
                        __leave;
                    }

                    if (returnLength < 1)
                        __leave;

                    appContainerInfo = PhAllocate(returnLength);

                    if (!NT_SUCCESS(NtQueryInformationToken(
                        tokenHandle,
                        TokenAppContainerSid,
                        appContainerInfo,
                        returnLength,
                        &returnLength
                        )))
                    {
                        __leave;
                    }

                    if (!appContainerInfo->TokenAppContainer)
                        __leave;

                    if (!NT_SUCCESS(RtlAddSIDToBoundaryDescriptor_I(&boundaryDescriptorHandle, appContainerInfo->TokenAppContainer)))
                        __leave;
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

            if (!NT_SUCCESS(NtOpenPrivateNamespace_I(
                &privateNamespaceHandle,
                MAXIMUM_ALLOWED,
                &namespaceObjectAttributes,
                boundaryDescriptorHandle
                )))
            {
                __leave;
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
                __leave;
            }
        }

        if (!NT_SUCCESS(NtMapViewOfSection(
            blockTableHandle,
            NtCurrentProcess(),
            &blockTableAddress,
            0,
            viewSize,
            &sectionOffset,
            &viewSize,
            ViewShare,
            0,
            PAGE_READONLY
            )))
        {
            __leave;
        }

        *BlockTableHandle = blockTableHandle;
        *BlockTableAddress = blockTableAddress;

        result = TRUE;
    }
    __finally
    {
        if (!result)
        {
            if (blockTableHandle)
            {
                NtClose(blockTableHandle);
            }

            if (blockTableAddress)
            {
                NtUnmapViewOfSection(NtCurrentProcess(), blockTableAddress);
            }

            *BlockTableHandle = NULL;
            *BlockTableAddress = NULL;
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

        if (everyoneSIDHandle)
        {
            RtlFreeSid(everyoneSIDHandle);
        }

        if (RtlDeleteBoundaryDescriptor_I && boundaryDescriptorHandle)
        {
            RtlDeleteBoundaryDescriptor_I(boundaryDescriptorHandle);
        }
    }

    return result;
}

PPH_LIST QueryDotNetAppDomainsForPid_V2(
    _In_ BOOLEAN Wow64,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    )
{
    LARGE_INTEGER sectionOffset = { 0 };
    SIZE_T viewSize = 0;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING sectionNameUs;
    HANDLE legacyPrivateBlockHandle = NULL;
    PVOID ipcControlBlockTable = NULL;
    PPH_LIST appDomainsList = NULL;

    __try
    {
        if (!PhStringRefToUnicodeString(&GeneratePrivateName(ProcessId)->sr, &sectionNameUs))
            __leave;

        InitializeObjectAttributes(
            &objectAttributes,
            &sectionNameUs,
            0,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(NtOpenSection(
            &legacyPrivateBlockHandle,
            SECTION_MAP_READ,
            &objectAttributes
            )))
        {
            __leave;
        }

        if (!NT_SUCCESS(NtMapViewOfSection(
            legacyPrivateBlockHandle,
            NtCurrentProcess(),
            &ipcControlBlockTable,
            0,
            viewSize,
            &sectionOffset,
            &viewSize,
            ViewShare,
            0,
            PAGE_READONLY
            )))
        {
            __leave;
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
                __leave;
            }

            appDomainEnumBlock = GetLegacyBlockTableEntry(
                Wow64,
                ipcControlBlockTable,
                eLegacyPrivateIPC_AppDomain
                );

            appDomainsList = EnumAppDomainIpcBlockWow64(
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
                __leave;
            }

            appDomainEnumBlock = GetLegacyBlockTableEntry(
                Wow64,
                ipcControlBlockTable, 
                eLegacyPrivateIPC_AppDomain
                );

            appDomainsList = EnumAppDomainIpcBlock(
                ProcessHandle, 
                appDomainEnumBlock
                );
        }
    }
    __finally
    {
        if (ipcControlBlockTable)
        {
            NtUnmapViewOfSection(NtCurrentProcess(), ipcControlBlockTable);
        }

        if (legacyPrivateBlockHandle)
        {
            NtClose(legacyPrivateBlockHandle);
        }
    }

    return appDomainsList;
}

PPH_LIST QueryDotNetAppDomainsForPid_V4(
    _In_ BOOLEAN Wow64,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    )
{
    HANDLE legacyPrivateBlockHandle = NULL;
    PVOID ipcControlBlockTable = NULL;
    LARGE_INTEGER sectionOffset = { 0 };
    SIZE_T viewSize = 0;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING sectionNameUs;
    PPH_LIST appDomainsList = NULL;

    __try
    {
        if (!PhStringRefToUnicodeString(&GeneratePrivateNameV4(ProcessId)->sr, &sectionNameUs))
            __leave;

        InitializeObjectAttributes(
            &objectAttributes,
            &sectionNameUs,
            0,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(NtOpenSection(
            &legacyPrivateBlockHandle,
            SECTION_MAP_READ,
            &objectAttributes
            )))
        {
            __leave;
        }

        if (!NT_SUCCESS(NtMapViewOfSection(
            legacyPrivateBlockHandle,
            NtCurrentProcess(),
            &ipcControlBlockTable,
            0,
            viewSize,
            &sectionOffset,
            &viewSize,
            ViewShare,
            0,
            PAGE_READONLY
            )))
        {
            __leave;
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
                __leave;
            }

            // Check the IPCControlBlock version is valid.
            if (legacyPrivateBlock->FullIPCHeader.Header.Version > VER_LEGACYPRIVATE_IPC_BLOCK)
            {
                __leave;
            }

            appDomainsList = EnumAppDomainIpcBlockWow64(
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
                __leave;
            }

            // Check the IPCControlBlock version is valid.
            if (legacyPrivateBlock->FullIPCHeader.Header.Version > VER_LEGACYPRIVATE_IPC_BLOCK)
            {
                __leave;
            }

            appDomainsList = EnumAppDomainIpcBlock(
                ProcessHandle, 
                appDomainEnumBlock
                );
        }
    }
    __finally
    {
        if (ipcControlBlockTable)
        {
            NtUnmapViewOfSection(NtCurrentProcess(), ipcControlBlockTable);
        }

        if (legacyPrivateBlockHandle)
        {
            NtClose(legacyPrivateBlockHandle);
        }
    }

    return appDomainsList;
}
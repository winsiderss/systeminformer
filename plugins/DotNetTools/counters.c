/*
 * Process Hacker .NET Tools -
 *   IPC support functions
 *
 * Copyright (C) 2015 dmex
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
    _Inout_ PVOID *BoundaryDescriptor,
    _In_ PSID RequiredSid
    );

typedef VOID (NTAPI* _RtlDeleteBoundaryDescriptor)(
    _In_ PVOID BoundaryDescriptor
    );

typedef HANDLE (WINAPI* _OpenPrivateNamespaceW)(
    _In_ LPVOID lpBoundaryDescriptor,
    _In_ LPCWSTR lpAliasPrefix
    );

typedef BOOLEAN (WINAPI* _ClosePrivateNamespace)(
    _In_ HANDLE Handle,
    _In_ ULONG Flags
    );

static _OpenPrivateNamespaceW OpenPrivateNamespace_I = NULL;
static _ClosePrivateNamespace ClosePrivateNamespace_I = NULL;
static _RtlCreateBoundaryDescriptor RtlCreateBoundaryDescriptor_I = NULL;
static _RtlDeleteBoundaryDescriptor RtlDeleteBoundaryDescriptor_I = NULL;
static _RtlAddSIDToBoundaryDescriptor RtlAddSIDToBoundaryDescriptor_I = NULL;


static PPH_STRING GeneratePrivateName(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"Global\\" CorLegacyPrivateIPCBlock, HandleToUlong(ProcessId));
}

static PPH_STRING GeneratePrivateNameV4(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"Global\\" CorLegacyPrivateIPCBlockTempV4, HandleToUlong(ProcessId));
}

static PPH_STRING GenerateLegacyPublicName(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"Global\\" CorLegacyPublicIPCBlock, HandleToUlong(ProcessId));
}

static PPH_STRING GenerateSxSPublicNameV4(_In_ HANDLE ProcessId)
{
    return PhaFormatString(L"Global\\" CorSxSPublicIPCBlock, HandleToUlong(ProcessId));
}


static PBYTE GetEntryBlockOffset(
    _In_ LegacyPrivateIPCControlBlock* IpcBlock,
    _In_ ULONG EntryId
    )
{
    // skip over directory (variable size)
    ULONG offsetBase = IPC_ENTRY_OFFSET_BASE_X64 + IpcBlock->FullIPCHeader.Header.NumEntries * sizeof(IPCEntry);
    // Directory has offset in bytes of block
    ULONG offsetEntry = IpcBlock->FullIPCHeader.EntryTable[EntryId].Offset;

    return ((PBYTE)IpcBlock) + offsetBase + offsetEntry;
}

static PBYTE GetEntryBlockOffset_Wow64(
    _In_ LegacyPrivateIPCControlBlock_Wow64* IpcBlock,
    _In_ ULONG EntryId
    )
{
    // skip over directory (variable size)
    ULONG offsetBase = IPC_ENTRY_OFFSET_BASE_X86 + IpcBlock->FullIPCHeader.Header.NumEntries * sizeof(IPCEntry);
    // Directory has offset in bytes of block
    ULONG offsetEntry = IpcBlock->FullIPCHeader.EntryTable[EntryId].Offset;

    return ((PBYTE)IpcBlock) + offsetBase + offsetEntry;
}



PVOID GetPerfIpcBlock_V2(
    _In_ BOOLEAN Wow64,
    _In_ PVOID BlockTableAddress
    )
{
    if (Wow64)
    {
        return &((LegacyPublicIPCControlBlock_Wow64*)BlockTableAddress)->PerfIpcBlock;
    }

    return &((LegacyPublicIPCControlBlock*)BlockTableAddress)->PerfIpcBlock;
}

PVOID GetPerfIpcBlock_V4(
    _In_ BOOLEAN Wow64,
    _In_ PVOID BlockTableAddress
    )
{
    if (Wow64)
    {
        return &((IPCControlBlockTable_Wow64*)BlockTableAddress)->Blocks->PerfIpcBlock;
    }

    return &((IPCControlBlockTable*)BlockTableAddress)->Blocks->PerfIpcBlock;
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

    __try
    {
        if (!(blockTableHandle = OpenFileMapping(FILE_MAP_READ, FALSE, GenerateLegacyPublicName(ProcessId)->Buffer)))
            __leave;

        if (!(blockTableAddress = MapViewOfFile(blockTableHandle, FILE_MAP_READ, 0, 0, 0)))
            __leave;

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
    PTOKEN_APPCONTAINER_INFORMATION appContainerInfo = NULL;
    SID_IDENTIFIER_AUTHORITY SIDWorldAuth = SECURITY_WORLD_SID_AUTHORITY;

    __try
    {
        if (WindowsVersion < WINDOWS_VISTA)
        {
            if (!(blockTableHandle = OpenFileMapping(FILE_MAP_READ, FALSE, GenerateSxSPublicNameV4(ProcessId)->Buffer)))
                __leave;
        }
        else
        {
            static PH_INITONCE initOnce = PH_INITONCE_INIT;
            PPH_STRING boundaryDescriptorName;
            UNICODE_STRING boundaryNameUs;

            if (PhBeginInitOnce(&initOnce))
            {
                HMODULE ntdll;
                HMODULE kernel32;

                ntdll = GetModuleHandle(L"ntdll.dll");
                kernel32 = GetModuleHandle(L"kernel32.dll");

                RtlCreateBoundaryDescriptor_I = PhGetProcedureAddress(ntdll, "RtlCreateBoundaryDescriptor", 0);
                RtlDeleteBoundaryDescriptor_I = PhGetProcedureAddress(ntdll, "RtlDeleteBoundaryDescriptor", 0);
                RtlAddSIDToBoundaryDescriptor_I = PhGetProcedureAddress(ntdll, "RtlAddSIDToBoundaryDescriptor", 0);

                OpenPrivateNamespace_I = PhGetProcedureAddress(kernel32, "OpenPrivateNamespaceW", 0);
                ClosePrivateNamespace_I = PhGetProcedureAddress(kernel32, "ClosePrivateNamespace", 0);

                PhEndInitOnce(&initOnce);
            }

            boundaryDescriptorName = PhaFormatString(CorSxSBoundaryDescriptor, HandleToUlong(ProcessId));

            if (!PhStringRefToUnicodeString(&boundaryDescriptorName->sr, &boundaryNameUs))
                __leave;

            if (!(boundaryDescriptorHandle = RtlCreateBoundaryDescriptor_I(&boundaryNameUs, 0)))
                __leave;

            if (!NT_SUCCESS(RtlAllocateAndInitializeSid(&SIDWorldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyoneSIDHandle)))
                __leave;

            if (!NT_SUCCESS(RtlAddSIDToBoundaryDescriptor_I(&boundaryDescriptorHandle, everyoneSIDHandle)))
                __leave;

            if (WINDOWS_HAS_IMMERSIVE && IsImmersive)
            {
                if (NT_SUCCESS(PhOpenProcessToken(
                    &tokenHandle,
                    TOKEN_QUERY, 
                    ProcessHandle
                    )))
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

            // TODO: Why doesn't NtOpenPrivateNamespace work?
            if (!(privateNamespaceHandle = OpenPrivateNamespace_I(boundaryDescriptorHandle, CorSxSReaderPrivateNamespacePrefix)))
                __leave;

            if (!(blockTableHandle = OpenFileMapping(FILE_MAP_READ, FALSE, CorSxSReaderPrivateNamespacePrefix L"\\" CorSxSVistaPublicIPCBlock)))
                __leave;
        }

        if (!(blockTableAddress = MapViewOfFile(blockTableHandle, FILE_MAP_READ, 0, 0, 0)))
            __leave;

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
            if (ClosePrivateNamespace_I)
                ClosePrivateNamespace_I(privateNamespaceHandle, 0);
        }

        if (everyoneSIDHandle)
        {
            RtlFreeSid(everyoneSIDHandle);
        }

        if (boundaryDescriptorHandle)
        {
            if (RtlDeleteBoundaryDescriptor_I)
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
    HANDLE legacyPrivateBlockMutexHandle = NULL;
    HANDLE legacyPrivateBlockHandle = NULL;
    PVOID ipcControlBlockTable = NULL;
    PPH_LIST appDomainsList = PhCreateList(1);

    __try
    {
        if (!(legacyPrivateBlockHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, GeneratePrivateName(ProcessId)->Buffer)))
            __leave;

        if (!(ipcControlBlockTable = MapViewOfFile(legacyPrivateBlockHandle, FILE_MAP_READ, 0, 0, 0)))
            __leave;

        if (Wow64)
        {
            LegacyPrivateIPCControlBlock_Wow64* legacyPrivateBlock_Wow64 = (LegacyPrivateIPCControlBlock_Wow64*)ipcControlBlockTable;
            AppDomainEnumerationIPCBlock_Wow64* appDomainEnumBlock = (AppDomainEnumerationIPCBlock_Wow64*)GetEntryBlockOffset_Wow64(legacyPrivateBlock_Wow64, eLegacyPrivateIPC_AppDomain);

            // If the mutex isn't filled in, the CLR is either starting up or shutting down
            if (!appDomainEnumBlock->Mutex)
            {
                __leave;
            }

            // Dup the valid mutex handle into this process.
            if (!NT_SUCCESS(PhDuplicateObject(
                ProcessHandle,
                UlongToHandle(appDomainEnumBlock->Mutex),
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
            LARGE_INTEGER timeout;

            if (NtWaitForSingleObject(
                legacyPrivateBlockMutexHandle,
                FALSE,
                PhTimeoutFromMilliseconds(&timeout, 2000)
                ) == STATUS_WAIT_0)
            {
                // Make sure the mutex handle is still valid. If its not, then we lost a shutdown race.
                if (!appDomainEnumBlock->Mutex)
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
            AppDomainEnumerationIPCBlock_Wow64 tempBlock;
            memcpy(&tempBlock, appDomainEnumBlock, sizeof(tempBlock));

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
            size_t pAppDomainInfoBlockLength = tempBlock.SizeInBytes;

            // Check other invariants.
            if (pAppDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo_Wow64))
            {
                __leave;
            }

            AppDomainInfo_Wow64* pAppDomainInfoBlock = (AppDomainInfo_Wow64*)PhAllocate(pAppDomainInfoBlockLength);
            memset(pAppDomainInfoBlock, 0, pAppDomainInfoBlockLength);

            if (!NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                UlongToPtr(tempBlock.ListOfAppDomains),
                pAppDomainInfoBlock,
                pAppDomainInfoBlockLength,
                NULL
                )))
            {
                PhFree(pAppDomainInfoBlock);
                __leave;
            }

            // Collect all the AppDomain info info a list of CorpubAppDomains
            for (int i = 0; i < tempBlock.NumOfUsedSlots; i++)
            {
                SIZE_T pAppDomainNameLength;
                PVOID pAppDomainName;

                if (!pAppDomainInfoBlock[i].AppDomainName)
                    continue;

                // Should be positive, and at least have a null-terminator character.
                if (pAppDomainInfoBlock[i].NameLengthInBytes <= 1)
                    continue;

                // Make sure buffer has right geometry.
                if (pAppDomainInfoBlock[i].NameLengthInBytes < 0)
                    continue;

                // If it's not on a WCHAR boundary, then we may have a 1-byte buffer-overflow.
                pAppDomainNameLength = pAppDomainInfoBlock[i].NameLengthInBytes / sizeof(WCHAR);

                if ((pAppDomainNameLength * sizeof(WCHAR)) != pAppDomainInfoBlock[i].NameLengthInBytes)
                    continue;

                // It should at least have 1 char for the null terminator.
                if (pAppDomainNameLength < 1)
                    continue;

                // We know the string is a well-formed null-terminated string,
                // but beyond that, we can't verify that the data is actually truthful.
                pAppDomainName = PhAllocate(pAppDomainInfoBlock[i].NameLengthInBytes + 1);
                memset(pAppDomainName, 0, pAppDomainInfoBlock[i].NameLengthInBytes + 1);

                if (!NT_SUCCESS(PhReadVirtualMemory(
                    ProcessHandle,
                    UlongToPtr(pAppDomainInfoBlock[i].AppDomainName),
                    pAppDomainName,
                    pAppDomainInfoBlock[i].NameLengthInBytes,
                    NULL
                    )))
                {
                    PhFree(pAppDomainName);
                    continue;
                }

                PhAddItemList(appDomainsList, pAppDomainName);
            }

            PhFree(pAppDomainInfoBlock);
        }
        else
        {
            LegacyPrivateIPCControlBlock* legacyPrivateBlock = (LegacyPrivateIPCControlBlock*)ipcControlBlockTable;
            AppDomainEnumerationIPCBlock* appDomainEnumBlock = (AppDomainEnumerationIPCBlock*)GetEntryBlockOffset(legacyPrivateBlock, eLegacyPrivateIPC_AppDomain);

            // If the mutex isn't filled in, the CLR is either starting up or shutting down
            if (!appDomainEnumBlock->Mutex)
            {
                __leave;
            }

            // Dup the valid mutex handle into this process.
            if (!NT_SUCCESS(PhDuplicateObject(
                ProcessHandle,
                appDomainEnumBlock->Mutex,
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
            LARGE_INTEGER timeout;

            if (NtWaitForSingleObject(
                legacyPrivateBlockMutexHandle, 
                FALSE, 
                PhTimeoutFromMilliseconds(&timeout, 2000)
                ) == STATUS_WAIT_0)
            {
                // Make sure the mutex handle is still valid. If its not, then we lost a shutdown race.
                if (!appDomainEnumBlock->Mutex)
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
            AppDomainEnumerationIPCBlock tempBlock;
            memcpy(&tempBlock, appDomainEnumBlock, sizeof(tempBlock));

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
            size_t pAppDomainInfoBlockLength = tempBlock.SizeInBytes;

            // Check other invariants.
            if (pAppDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo))
            {
                __leave;
            }


            AppDomainInfo* pAppDomainInfoBlock = (AppDomainInfo*)PhAllocate(pAppDomainInfoBlockLength);
            memset(pAppDomainInfoBlock, 0, pAppDomainInfoBlockLength);

            if (!NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                tempBlock.ListOfAppDomains,
                pAppDomainInfoBlock,
                pAppDomainInfoBlockLength,
                NULL
                )))
            {
                PhFree(pAppDomainInfoBlock);
                __leave;
            }

            // Collect all the AppDomain info info a list of CorpubAppDomains
            for (int i = 0; i < tempBlock.NumOfUsedSlots; i++)
            {
                SIZE_T pAppDomainNameLength;
                PVOID pAppDomainName;

                if (!pAppDomainInfoBlock[i].AppDomainName)
                    continue;

                // Should be positive, and at least have a null-terminator character.
                if (pAppDomainInfoBlock[i].NameLengthInBytes <= 1)
                    continue;

                // Make sure buffer has right geometry.
                if (pAppDomainInfoBlock[i].NameLengthInBytes < 0)
                    continue;

                // If it's not on a WCHAR boundary, then we may have a 1-byte buffer-overflow.
                pAppDomainNameLength = pAppDomainInfoBlock[i].NameLengthInBytes / sizeof(WCHAR);

                if ((pAppDomainNameLength * sizeof(WCHAR)) != pAppDomainInfoBlock[i].NameLengthInBytes)
                    continue;

                // It should at least have 1 char for the null terminator.
                if (pAppDomainNameLength < 1)
                    continue;

                // We know the string is a well-formed null-terminated string,
                // but beyond that, we can't verify that the data is actually truthful.
                pAppDomainName = PhAllocate(pAppDomainInfoBlock[i].NameLengthInBytes + 1);
                memset(pAppDomainName, 0, pAppDomainInfoBlock[i].NameLengthInBytes + 1);

                if (!NT_SUCCESS(PhReadVirtualMemory(
                    ProcessHandle,
                    pAppDomainInfoBlock[i].AppDomainName,
                    pAppDomainName,
                    pAppDomainInfoBlock[i].NameLengthInBytes,
                    NULL
                    )))
                {
                    PhFree(pAppDomainName);
                    continue;
                }

                PhAddItemList(appDomainsList, pAppDomainName);
            }

            PhFree(pAppDomainInfoBlock);
        }
    }
    __finally
    {
        if (legacyPrivateBlockMutexHandle)
        {
            NtReleaseMutant(legacyPrivateBlockMutexHandle, NULL);
        }

        if (legacyPrivateBlockMutexHandle)
        {
            NtClose(legacyPrivateBlockMutexHandle);
        }

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
    HANDLE legacyPrivateBlockMutexHandle = NULL;
    HANDLE legacyPrivateBlockHandle = NULL;
    PVOID ipcControlBlockTable = NULL;
    PPH_LIST appDomainsList = PhCreateList(1);

    __try
    {
        if (!(legacyPrivateBlockHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, GeneratePrivateNameV4(ProcessId)->Buffer)))
            __leave;

        if (!(ipcControlBlockTable = MapViewOfFile(legacyPrivateBlockHandle, FILE_MAP_READ, 0, 0, 0)))
            __leave;

        if (Wow64)
        {
            LegacyPrivateIPCControlBlock_Wow64* legacyPrivateBlock_Wow64 = (LegacyPrivateIPCControlBlock_Wow64*)ipcControlBlockTable;
            AppDomainEnumerationIPCBlock_Wow64 appDomainEnumBlock = legacyPrivateBlock_Wow64->AppDomainBlock;
            
            if ((legacyPrivateBlock_Wow64->FullIPCHeader.Header.Flags & IPC_FLAG_INITIALIZED) != IPC_FLAG_INITIALIZED)
            {
                __leave;
            }

            // If the mutex isn't filled in, the CLR is either starting up or shutting down
            if (!appDomainEnumBlock.Mutex)
            {
                __leave;
            }

            // Dup the valid mutex handle into this process.
            if (!NT_SUCCESS(PhDuplicateObject(
                ProcessHandle,
                UlongToHandle(appDomainEnumBlock.Mutex),
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
            LARGE_INTEGER timeout;

            if (NtWaitForSingleObject(
                legacyPrivateBlockMutexHandle,
                FALSE,
                PhTimeoutFromMilliseconds(&timeout, 2000)
                ) == STATUS_WAIT_0)
            {
                // Make sure the mutex handle is still valid. If its not, then we lost a shutdown race.
                if (!appDomainEnumBlock.Mutex)
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
            AppDomainEnumerationIPCBlock_Wow64 tempBlock;
            memcpy(&tempBlock, &appDomainEnumBlock, sizeof(tempBlock));

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
            size_t pAppDomainInfoBlockLength = tempBlock.SizeInBytes;

            // Check other invariants.
            if (pAppDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo_Wow64))
            {
                __leave;
            }

            AppDomainInfo_Wow64* pAppDomainInfoBlock = (AppDomainInfo_Wow64*)PhAllocate(pAppDomainInfoBlockLength);
            memset(pAppDomainInfoBlock, 0, pAppDomainInfoBlockLength);

            if (!NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                UlongToPtr(tempBlock.ListOfAppDomains),
                pAppDomainInfoBlock,
                pAppDomainInfoBlockLength,
                NULL
                )))
            {
                PhFree(pAppDomainInfoBlock);
                __leave;
            }

            // Collect all the AppDomain info info a list of CorpubAppDomains
            for (int i = 0; i < tempBlock.NumOfUsedSlots; i++)
            {
                SIZE_T pAppDomainNameLength;
                PVOID pAppDomainName;

                if (!pAppDomainInfoBlock[i].AppDomainName)
                    continue;

                // Should be positive, and at least have a null-terminator character.
                if (pAppDomainInfoBlock[i].NameLengthInBytes <= 1)
                    continue;

                // Make sure buffer has right geometry.
                if (pAppDomainInfoBlock[i].NameLengthInBytes < 0)
                    continue;

                // If it's not on a WCHAR boundary, then we may have a 1-byte buffer-overflow.
                pAppDomainNameLength = pAppDomainInfoBlock[i].NameLengthInBytes / sizeof(WCHAR);

                if ((pAppDomainNameLength * sizeof(WCHAR)) != pAppDomainInfoBlock[i].NameLengthInBytes)
                    continue;

                // It should at least have 1 char for the null terminator.
                if (pAppDomainNameLength < 1)
                    continue;

                // We know the string is a well-formed null-terminated string,
                // but beyond that, we can't verify that the data is actually truthful.
                pAppDomainName = PhAllocate(pAppDomainInfoBlock[i].NameLengthInBytes + 1);
                memset(pAppDomainName, 0, pAppDomainInfoBlock[i].NameLengthInBytes + 1);

                if (!NT_SUCCESS(PhReadVirtualMemory(
                    ProcessHandle,
                    UlongToPtr(pAppDomainInfoBlock[i].AppDomainName),
                    pAppDomainName,
                    pAppDomainInfoBlock[i].NameLengthInBytes,
                    NULL
                    )))
                {
                    PhFree(pAppDomainName);
                    continue;
                }

                PhAddItemList(appDomainsList, pAppDomainName);
            }

            PhFree(pAppDomainInfoBlock);
        }
        else
        {
            LegacyPrivateIPCControlBlock* legacyPrivateBlock = (LegacyPrivateIPCControlBlock*)ipcControlBlockTable;
            AppDomainEnumerationIPCBlock appDomainEnumBlock = legacyPrivateBlock->AppDomainBlock;

            if ((legacyPrivateBlock->FullIPCHeader.Header.Flags & IPC_FLAG_INITIALIZED) != IPC_FLAG_INITIALIZED)
            {
                __leave;
            }

            // If the mutex isn't filled in, the CLR is either starting up or shutting down
            if (!appDomainEnumBlock.Mutex)
            {
                __leave;
            }

            // Dup the valid mutex handle into this process.
            if (!NT_SUCCESS(PhDuplicateObject(
                ProcessHandle,
                appDomainEnumBlock.Mutex,
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
            LARGE_INTEGER timeout;

            if (NtWaitForSingleObject(
                legacyPrivateBlockMutexHandle,
                FALSE,
                PhTimeoutFromMilliseconds(&timeout, 2000)
                ) == STATUS_WAIT_0)
            {
                // Make sure the mutex handle is still valid. If its not, then we lost a shutdown race.
                if (!appDomainEnumBlock.Mutex)
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
            AppDomainEnumerationIPCBlock tempBlock;
            memcpy(&tempBlock, &appDomainEnumBlock, sizeof(tempBlock));

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
            size_t pAppDomainInfoBlockLength = tempBlock.SizeInBytes;

            // Check other invariants.
            if (pAppDomainInfoBlockLength != tempBlock.TotalSlots * sizeof(AppDomainInfo))
            {
                __leave;
            }

            AppDomainInfo* pAppDomainInfoBlock = (AppDomainInfo*)PhAllocate(pAppDomainInfoBlockLength);
            memset(pAppDomainInfoBlock, 0, pAppDomainInfoBlockLength);

            if (!NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                tempBlock.ListOfAppDomains,
                pAppDomainInfoBlock,
                pAppDomainInfoBlockLength,
                NULL
                )))
            {
                PhFree(pAppDomainInfoBlock);
                __leave;
            }

            // Collect all the AppDomain info info a list of CorpubAppDomains
            for (int i = 0; i < tempBlock.NumOfUsedSlots; i++)
            {
                SIZE_T pAppDomainNameLength;
                PVOID pAppDomainName;

                if (!pAppDomainInfoBlock[i].AppDomainName)
                    continue;

                // Should be positive, and at least have a null-terminator character.
                if (pAppDomainInfoBlock[i].NameLengthInBytes <= 1)
                    continue;

                // Make sure buffer has right geometry.
                if (pAppDomainInfoBlock[i].NameLengthInBytes < 0)
                    continue;

                // If it's not on a WCHAR boundary, then we may have a 1-byte buffer-overflow.
                pAppDomainNameLength = pAppDomainInfoBlock[i].NameLengthInBytes / sizeof(WCHAR);

                if ((pAppDomainNameLength * sizeof(WCHAR)) != pAppDomainInfoBlock[i].NameLengthInBytes)
                    continue;

                // It should at least have 1 char for the null terminator.
                if (pAppDomainNameLength < 1)
                    continue;

                // We know the string is a well-formed null-terminated string,
                // but beyond that, we can't verify that the data is actually truthful.
                pAppDomainName = PhAllocate(pAppDomainInfoBlock[i].NameLengthInBytes + 1);
                memset(pAppDomainName, 0, pAppDomainInfoBlock[i].NameLengthInBytes + 1);

                if (!NT_SUCCESS(PhReadVirtualMemory(
                    ProcessHandle,
                    pAppDomainInfoBlock[i].AppDomainName,
                    pAppDomainName,
                    pAppDomainInfoBlock[i].NameLengthInBytes,
                    NULL
                    )))
                {
                    PhFree(pAppDomainName);
                    continue;
                }

                PhAddItemList(appDomainsList, pAppDomainName);
            }

            PhFree(pAppDomainInfoBlock);
        }
    }
    __finally
    {
        if (legacyPrivateBlockMutexHandle)
        {
            NtReleaseMutant(legacyPrivateBlockMutexHandle, NULL);
        }

        if (legacyPrivateBlockMutexHandle)
        {
            NtClose(legacyPrivateBlockMutexHandle);
        }

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
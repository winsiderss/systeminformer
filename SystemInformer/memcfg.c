/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

/*
 * Enumerates a process's Control Flow Guard (CFG) bitmap.
 *
 * ntdll!LdrSystemDllInitBlock reports the CFG bitmap address and its full
 * reservation size (~2 TB on x64), but only a sparse set of pages are actually
 * committed as CFG-instrumented modules load. We locate the true mapped extent
 * by enumerating the committed sub-regions sharing the bitmap's allocation base
 * (the same regions tagged "CFG Bitmap" on the Memory tab), decode the committed
 * bits back into the valid call-target addresses they encode, and report what is
 * excluded: the uncommitted remainder of the reservation (excluded at runtime)
 * and fully-valid ranges that indicate modules without CFG instrumentation
 * (excluded from the image config).
 *
 * Bitmap layout (x64): 1 bit per 8 bytes of address space, i.e. 2 bits per
 * 16-byte-aligned call target. A 64-bit element covers 512 bytes of VA and a
 * 4 KB bitmap page covers 256 KB of VA. For bitmap byte offset B the covered VA
 * base is B * 64; within a 64-bit element, slot s (0..31) maps to VA base + s*16
 * and its 2-bit value encodes target validity (0 = none). The WOW64 32-bit
 * bitmap uses the analogous layout over the 32-bit address space.
 */

#include <phapp.h>
#include <procprv.h>
#include <memprv.h>
#include <settings.h>
#include <phsettings.h>
#include <emenu.h>
#include <mapimg.h>
#include <symprv.h>
#include <workqueue.h>

#define PH_CFG_MAX_ENTRIES 800000
#define PH_CFG_READ_CHUNK_SIZE 0x10000 // 64 KB

#define PH_CFG_MENU_SHOW_IMAGE_TARGETS 1
#define PH_CFG_MENU_SHOW_MODIFIED_TARGETS 2
#define PH_CFG_MENU_SHOW_ALL_VALID_RANGES 3

typedef struct _PH_CFG_BITMAP_DIALOG_CONTEXT
{
    LONG RefCount;

    HWND WindowHandle;
    HWND ListViewHandle;
    HWND StatusHandle;
    HWND MessageHandle;
    HWND ParentWindowHandle;
    PPH_PROCESS_ITEM ProcessItem;
    PH_LAYOUT_MANAGER LayoutManager;

    BOOLEAN Destroying;
    BOOLEAN Truncated;
    BOOLEAN ShowImageTargets;
    BOOLEAN ShowModifiedTargets;
    BOOLEAN ShowAllValidRanges;

    HANDLE ProcessHandle;
    ULONG EntryCount;
    PPH_LIST EntryList;
    PPH_LIST DisplayEntryList;
    PPH_LIST RegionList;
    PPH_HASHTABLE ImageCfgTargetHashtable;

    PH_INITONCE SymbolProviderInitOnce;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    SINGLE_LIST_ENTRY ResultListHead;
    PH_QUEUED_LOCK ResultListLock;
    PH_CALLBACK_REGISTRATION SymbolProviderEventRegistration;
} PH_CFG_BITMAP_DIALOG_CONTEXT, *PPH_CFG_BITMAP_DIALOG_CONTEXT;

typedef enum _CFG_BITMAP_STATE
{
    CfgTargetInvalid = 0x0, // 00b: Invalid call target. Triggers __fastfail(FAST_FAIL_GUARD_ICALL_CHECK_FAILURE).
    CfgTargetExportSuppressed = 0x1, // 01b: Export Suppressed. Valid function, but restricted from general indirect calls.
    CfgTargetValidAligned = 0x2, // 10b: Valid call target (16-byte aligned).
    CfgTargetValidUnaligned = 0x3  // 11b: Valid call target (Not 16-byte aligned).
} CFG_BITMAP_STATE;

typedef enum _PH_CFG_BITMAP_ENTRY_TYPE
{
    PhCfgBitmapAllValidRange,
    PhCfgBitmapImageTarget,
    PhCfgBitmapModifiedTarget,
    PhCfgBitmapExportSuppressedTarget,
} PH_CFG_BITMAP_ENTRY_TYPE;

typedef struct _PH_CFG_MEMORY_REGION PH_CFG_MEMORY_REGION, *PPH_CFG_MEMORY_REGION;

typedef struct _PH_CFG_ENTRY
{
    PVOID TargetAddress;
    PVOID TargetEndAddress;
    ULONG Value; // 2-bit slot value, or ULONG_MAX for an all-valid range
    BOOLEAN Wow64;
    BOOLEAN RangeValid;
    PH_CFG_BITMAP_ENTRY_TYPE Type;
    ULONG ListIndex;
    PPH_STRING Symbol;
    PPH_CFG_MEMORY_REGION Region;
} PH_CFG_ENTRY, *PPH_CFG_ENTRY;

typedef struct _PH_CFG_MEMORY_REGION
{
    MEMORY_BASIC_INFORMATION BasicInformation;
    BOOLEAN MappedFileNameQueried;
    PPH_STRING MappedFileName;
} PH_CFG_MEMORY_REGION, *PPH_CFG_MEMORY_REGION;

typedef struct _CFG_SYMBOL_LOOKUP_RESULT
{
    SINGLE_LIST_ENTRY ListEntry;
    PPH_CFG_BITMAP_DIALOG_CONTEXT Context;
    PPH_CFG_ENTRY Entry;
    PVOID Address;
    PPH_STRING Symbol;
} CFG_SYMBOL_LOOKUP_RESULT, *PCFG_SYMBOL_LOOKUP_RESULT;

NTSTATUS PhInitializeProcessCfgTargetContext(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_CFG_TARGET_CONTEXT Context
    )
{
    NTSTATUS status;
    PS_SYSTEM_DLL_INIT_BLOCK initBlock = { 0 };
    BOOLEAN enabled;

    memset(Context, 0, sizeof(PH_CFG_TARGET_CONTEXT));

    status = PhGetProcessIsCFGuardEnabled(ProcessHandle, &enabled);

    if (!NT_SUCCESS(status))
        return status;

    Context->Enabled = enabled;

    if (!enabled)
        return STATUS_SUCCESS;

    status = PhGetProcessSystemDllInitBlock(ProcessHandle, &initBlock);

    if (!NT_SUCCESS(status))
        return status;

    if (RTL_CONTAINS_FIELD(&initBlock, initBlock.Size, CfgBitMapSize))
    {
        Context->BitmapBase = (ULONG_PTR)initBlock.CfgBitMap;
        Context->BitmapSize = initBlock.CfgBitMapSize;
    }

    if (RTL_CONTAINS_FIELD(&initBlock, initBlock.Size, Wow64CfgBitMapSize))
    {
        Context->Wow64BitmapBase = (ULONG_PTR)initBlock.Wow64CfgBitMap;
        Context->Wow64BitmapSize = initBlock.Wow64CfgBitMapSize;
    }

    return STATUS_SUCCESS;
}

PH_CFG_TARGET_STATUS PhQueryProcessCfgTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_CFG_TARGET_CONTEXT Context,
    _In_ PVOID Address,
    _In_ BOOLEAN Wow64
    )
{
    ULONG_PTR bitmapBase;
    ULONG64 bitmapSize;
    ULONG_PTR address;
    ULONG_PTR bitmapByteOffset;
    ULONG_PTR wordByteOffset;
    ULONG slot;
    ULONG64 word;

    if (!Context->Enabled)
        return PhCfgTargetDisabled;

    bitmapBase = Wow64 ? Context->Wow64BitmapBase : Context->BitmapBase;
    bitmapSize = Wow64 ? Context->Wow64BitmapSize : Context->BitmapSize;

    if (!bitmapBase || !bitmapSize || !Address)
        return PhCfgTargetUnavailable;

    address = (ULONG_PTR)Address;
    bitmapByteOffset = address / 64;
    wordByteOffset = ALIGN_DOWN_BY(bitmapByteOffset, sizeof(ULONG64));

    if (wordByteOffset > bitmapSize || sizeof(ULONG64) > bitmapSize - wordByteOffset)
        return PhCfgTargetInvalid;

    if (!NT_SUCCESS(PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(bitmapBase, wordByteOffset),
        &word,
        sizeof(ULONG64),
        NULL
        )))
    {
        return PhCfgTargetUnavailable;
    }

    slot = (ULONG)((address - wordByteOffset * 64) / 16);

    if (slot >= 32)
        return PhCfgTargetInvalid;

    return ((word >> (slot * 2)) & 0x3) != 0 ? PhCfgTargetValid : PhCfgTargetInvalid;
}

VOID PhCfgUpdateStatus(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    PhSetWindowText(Context->StatusHandle, PhaFormatString(
        L"%lu of %lu entries shown",
        Context->DisplayEntryList ? Context->DisplayEntryList->Count : 0,
        Context->EntryList ? Context->EntryList->Count : 0
        )->Buffer);
}

BOOLEAN PhCfgIsEntryVisible(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context,
    _In_ PPH_CFG_ENTRY Entry
    )
{
    switch (Entry->Type)
    {
    case PhCfgBitmapImageTarget:
        return Context->ShowImageTargets;
    case PhCfgBitmapModifiedTarget:
        return Context->ShowModifiedTargets;
    case PhCfgBitmapAllValidRange:
        return Context->ShowAllValidRanges;
    default:
        return FALSE;
    }
}

VOID PhCfgApplyFilters(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    if (!Context->DisplayEntryList)
        Context->DisplayEntryList = PhCreateList(256);
    else
        PhClearList(Context->DisplayEntryList);

    if (Context->EntryList)
    {
        for (ULONG i = 0; i < Context->EntryList->Count; i++)
        {
            PPH_CFG_ENTRY entry = Context->EntryList->Items[i];

            entry->ListIndex = ULONG_MAX;

            if (PhCfgIsEntryVisible(Context, entry))
            {
                entry->ListIndex = Context->DisplayEntryList->Count;
                PhAddItemList(Context->DisplayEntryList, entry);
            }
        }
    }

    ListView_SetItemCount(Context->ListViewHandle, Context->DisplayEntryList->Count);
    InvalidateRect(Context->ListViewHandle, NULL, FALSE);
    PhCfgUpdateStatus(Context);
}

VOID PhCfgReferenceContext(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    _InterlockedIncrement(&Context->RefCount);
}

VOID PhCfgCleanupEntries(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    PSINGLE_LIST_ENTRY listEntry;

    if (Context->EntryList)
    {
        for (ULONG i = 0; i < Context->EntryList->Count; i++)
        {
            PPH_CFG_ENTRY entry = Context->EntryList->Items[i];
            PhClearReference(&entry->Symbol);
            PhFree(entry);
        }
        PhClearReference(&Context->EntryList);
    }

    PhClearReference(&Context->DisplayEntryList);
    PhClearReference(&Context->ImageCfgTargetHashtable);

    if (Context->RegionList)
    {
        for (ULONG i = 0; i < Context->RegionList->Count; i++)
        {
            PPH_CFG_MEMORY_REGION region = Context->RegionList->Items[i];

            PhClearReference(&region->MappedFileName);
            PhFree(region);
        }

        PhClearReference(&Context->RegionList);
    }

    // Free all unused symbol results.

    PhAcquireQueuedLockExclusive(&Context->ResultListLock);

    listEntry = Context->ResultListHead.Next;
    Context->ResultListHead.Next = NULL;

    while (listEntry)
    {
        PCFG_SYMBOL_LOOKUP_RESULT result;

        result = CONTAINING_RECORD(listEntry, CFG_SYMBOL_LOOKUP_RESULT, ListEntry);
        listEntry = listEntry->Next;

        PhClearReference(&result->Symbol);
        PhFree(result);
    }

    PhReleaseQueuedLockExclusive(&Context->ResultListLock);
}

VOID PhCfgDereferenceContext(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    if (_InterlockedDecrement(&Context->RefCount) == 0)
    {
        if (Context->SymbolProvider)
            PhDereferenceObject(Context->SymbolProvider);
        if (Context->ProcessHandle)
            NtClose(Context->ProcessHandle);

        PhCfgCleanupEntries(Context);

        PhDereferenceObject(Context->ProcessItem);

        PhFree(Context);
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhCfgSymbolLookupFunction(
    _In_ PVOID Parameter
    )
{
    PCFG_SYMBOL_LOOKUP_RESULT result = Parameter;

    if (PhBeginInitOnce(&result->Context->SymbolProviderInitOnce))
    {
        PhLoadSymbolProviderOptions(result->Context->SymbolProvider);
        PhLoadModulesForVirtualSymbolProvider(
            result->Context->SymbolProvider,
            result->Context->ProcessItem->ProcessId,
            NULL
            );
        PhEndInitOnce(&result->Context->SymbolProviderInitOnce);
    }

    if (result->Context->Destroying)
    {
        PhCfgDereferenceContext(result->Context);
        PhFree(result);
        return STATUS_SUCCESS;
    }

    result->Symbol = PhGetSymbolFromAddress(
        result->Context->SymbolProvider,
        result->Address,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (!result->Symbol)
    {
        PhCfgDereferenceContext(result->Context);
        PhFree(result);
        return STATUS_SUCCESS;
    }

    PhAcquireQueuedLockExclusive(&result->Context->ResultListLock);
    PushEntryList(&result->Context->ResultListHead, &result->ListEntry);
    PhReleaseQueuedLockExclusive(&result->Context->ResultListLock);
    PhCfgDereferenceContext(result->Context);

    return STATUS_SUCCESS;
}

VOID PhCfgQueueSymbolLookup(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context,
    _In_ PPH_CFG_ENTRY Entry
    )
{
    PCFG_SYMBOL_LOOKUP_RESULT result;

    if (!Context->SymbolProvider)
    {
        Context->SymbolProvider = PhCreateSymbolProvider(NULL);
    }

    result = PhAllocateZero(sizeof(CFG_SYMBOL_LOOKUP_RESULT));
    result->Context = Context;
    result->Entry = Entry;
    result->Address = Entry->TargetAddress;
    PhCfgReferenceContext(Context);

    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhCfgSymbolLookupFunction, result);
}

VOID PhCfgProcessSymbolLookupResults(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    PSINGLE_LIST_ENTRY listEntry;

    PhAcquireQueuedLockExclusive(&Context->ResultListLock);
    listEntry = Context->ResultListHead.Next;
    Context->ResultListHead.Next = NULL;
    PhReleaseQueuedLockExclusive(&Context->ResultListLock);

    while (listEntry)
    {
        PCFG_SYMBOL_LOOKUP_RESULT result;

        result = CONTAINING_RECORD(listEntry, CFG_SYMBOL_LOOKUP_RESULT, ListEntry);
        listEntry = listEntry->Next;

        PhClearReference(&result->Entry->Symbol);
        PhMoveReference(&result->Entry->Symbol, result->Symbol);

        if (result->Entry->ListIndex != ULONG_MAX)
        {
            ListView_RedrawItems(Context->ListViewHandle, (INT)result->Entry->ListIndex, (INT)result->Entry->ListIndex);
        }

        PhFree(result);
    }
}

VOID PhCfgAddEntryToListView(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context,
    _In_ PPH_CFG_ENTRY Entry
    )
{
    ULONG low = 0;
    ULONG high;
    ULONG_PTR targetAddress = (ULONG_PTR)Entry->TargetAddress;

    high = Context->RegionList ? Context->RegionList->Count : 0;

    while (low < high)
    {
        ULONG middle = low + (high - low) / 2;
        PPH_CFG_MEMORY_REGION region = Context->RegionList->Items[middle];
        ULONG_PTR baseAddress = (ULONG_PTR)region->BasicInformation.BaseAddress;

        if (targetAddress < baseAddress)
        {
            high = middle;
        }
        else if (targetAddress - baseAddress >= region->BasicInformation.RegionSize)
        {
            low = middle + 1;
        }
        else
        {
            Entry->Region = region;

            if (!region->MappedFileNameQueried &&
                FlagOn(region->BasicInformation.Type, MEM_IMAGE | MEM_MAPPED))
            {
                PPH_STRING fileName;

                region->MappedFileNameQueried = TRUE;

                if (NT_SUCCESS(PhGetProcessMappedFileName(
                    Context->ProcessHandle,
                    region->BasicInformation.BaseAddress,
                    &fileName
                    )))
                {
                    PPH_STRING newFileName = PhResolveDevicePrefix(&fileName->sr);

                    if (newFileName)
                        PhMoveReference(&fileName, newFileName);

                    region->MappedFileName = fileName;
                }
            }

            break;
        }
    }

    PhCfgQueueSymbolLookup(Context, Entry);

    Context->EntryCount++;
}

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI PhCfgEnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PPH_CFG_BITMAP_DIALOG_CONTEXT context = Context;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_CFG cfgConfig;

    if (!Module->FileName || PhIsNullOrEmptyString(Module->FileName))
        return TRUE;

    if (NT_SUCCESS(PhLoadMappedImageEx(&Module->FileName->sr, NULL, &mappedImage)))
    {
        if (NT_SUCCESS(PhGetMappedImageCfg(&cfgConfig, &mappedImage)))
        {
            for (ULONGLONG i = 0; i < cfgConfig.NumberOfGuardFunctionEntries; i++)
            {
                IMAGE_CFG_ENTRY cfgEntry;

                if (NT_SUCCESS(PhGetMappedImageCfgEntry(
                    &cfgConfig,
                    i,
                    ControlFlowGuardFunction,
                    &cfgEntry
                    )))
                {
                    ULONG_PTR targetAddress;

                    targetAddress = (ULONG_PTR)PTR_ADD_OFFSET(Module->BaseAddress, cfgEntry.Rva);
                    targetAddress = ALIGN_DOWN_BY(targetAddress, 16);
                    PhAddItemSimpleHashtable(context->ImageCfgTargetHashtable, (PVOID)targetAddress, NULL);
                }
            }
        }

        PhUnloadMappedImage(&mappedImage);
    }

    return TRUE;
}

VOID PhCfgCollectImageTargets(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    Context->ImageCfgTargetHashtable = PhCreateSimpleHashtable(1024);

    PhEnumGenericModules(
        Context->ProcessItem->ProcessId,
        Context->ProcessHandle,
        0,
        PhCfgEnumModulesCallback,
        Context
        );
}

_Function_class_(PH_ENUM_MEMORY_CALLBACK)
NTSTATUS NTAPI PhCfgEnumRegionsCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PMEMORY_BASIC_INFORMATION BasicInformation,
    _In_ PVOID Context
    )
{
    PPH_LIST list = Context;

    PPH_CFG_MEMORY_REGION region;

    region = PhAllocateZero(sizeof(PH_CFG_MEMORY_REGION));
    region->BasicInformation = *BasicInformation;

    PhAddItemList(list, region);

    return STATUS_SUCCESS;
}

// Decode the committed bitmap bytes for a single sub-region into entries.
VOID PhCfgDecodeRegion(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context,
    _In_ PPH_CFG_MEMORY_REGION Region,
    _In_ ULONG_PTR BitmapBase,
    _In_ BOOLEAN Wow64
    )
{
    PVOID buffer;
    ULONG_PTR regionByteOffset; // offset of this region from the bitmap base
    SIZE_T remaining;
    SIZE_T chunkOffset;

    if ((ULONG_PTR)Region->BasicInformation.BaseAddress < BitmapBase)
        return;

    regionByteOffset = (ULONG_PTR)Region->BasicInformation.BaseAddress - BitmapBase;
    remaining = Region->BasicInformation.RegionSize;
    chunkOffset = 0;
    buffer = PhAllocate(PH_CFG_READ_CHUNK_SIZE);

    while (remaining != 0)
    {
        SIZE_T chunkSize = min(remaining, PH_CFG_READ_CHUNK_SIZE);
        ULONG_PTR chunkByteOffset = regionByteOffset + chunkOffset;
        PULONG64 words = buffer;
        SIZE_T wordCount;
        SIZE_T i;

        if (!NT_SUCCESS(PhReadVirtualMemory(
            Context->ProcessHandle,
            PTR_ADD_OFFSET(Region->BasicInformation.BaseAddress, chunkOffset),
            buffer,
            chunkSize,
            NULL
            )))
        {
            break;
        }

        wordCount = chunkSize / sizeof(ULONG64);

        for (i = 0; i < wordCount; )
        {
            if (Context->EntryCount >= PH_CFG_MAX_ENTRIES)
            {
                Context->Truncated = TRUE;
                break;
            }

            if (words[i] == MAXULONG64)
            {
                SIZE_T j = i;
                ULONG_PTR runByteOffset;
                ULONG_PTR runBytes;
                PPH_CFG_ENTRY entry;

                // Collapse a contiguous all-valid run into a single range entry.
                // Instead of allocating 32 separate PH_CFG_ENTRY structs for a contiguous block of valid memory, 
                // skip forward to find the end MAXULONG64 and collapses it into a single PhCfgBitmapAllValidRange entry. 
                // This heavily reduces memory overhead and UI clutter in the list view.

                while (j < wordCount && words[j] == MAXULONG64)
                    j++;

                runByteOffset = chunkByteOffset + i * sizeof(ULONG64);
                runBytes = (ULONG_PTR)(j - i) * sizeof(ULONG64);

                entry = PhAllocateZero(sizeof(PH_CFG_ENTRY));
                entry->TargetAddress = (PVOID)(runByteOffset * 64);
                entry->TargetEndAddress = (PVOID)((runByteOffset + runBytes) * 64);
                entry->Value = ULONG_MAX;
                entry->RangeValid = TRUE;
                entry->Wow64 = Wow64;
                entry->Type = PhCfgBitmapAllValidRange;
                entry->ListIndex = ULONG_MAX;

                PhAddItemList(Context->EntryList, entry);
                PhCfgAddEntryToListView(Context, entry);

                i = j;
            }
            else if (words[i] == 0)
            {
                // skip 512 bytes of virtual memory space where no valid indirect call targets exist.
                i++;
            }
            else
            {
                ULONG_PTR wordByteOffset = chunkByteOffset + i * sizeof(ULONG64);
                ULONG64 word = words[i];
                ULONG slot;

                for (slot = 0; slot < 32; slot++)
                {
                    ULONG slotValue = (ULONG)((word >> (slot * 2)) & 0x3);
                    PPH_CFG_ENTRY entry;

                    if (slotValue == 0)
                        continue;

                    if (Context->EntryCount >= PH_CFG_MAX_ENTRIES)
                    {
                        Context->Truncated = TRUE;
                        break;
                    }

                    entry = PhAllocateZero(sizeof(PH_CFG_ENTRY));
                    entry->TargetAddress = (PVOID)(wordByteOffset * 64 + (ULONG_PTR)slot * 16);
                    entry->Value = slotValue;
                    entry->Wow64 = Wow64;
                    entry->ListIndex = ULONG_MAX;

                    if (slotValue == 1)
                    {
                        entry->Type = PhCfgBitmapExportSuppressedTarget;
                    }
                    else
                    {
                        entry->Type = PhFindItemSimpleHashtable(
                            Context->ImageCfgTargetHashtable,
                            entry->TargetAddress
                            ) ? PhCfgBitmapImageTarget : PhCfgBitmapModifiedTarget;
                    }

                    PhAddItemList(Context->EntryList, entry);
                    PhCfgAddEntryToListView(Context, entry);
                }

                i++;
            }
        }

        if (Context->Truncated)
            break;

        chunkOffset += chunkSize;
        remaining -= chunkSize;
    }

    PhFree(buffer);
}

// Locate the committed extent of one bitmap and decode it. Appends a summary
// line (claimed vs actual) to the message builder.
VOID PhCfgProcessBitmap(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context,
    _In_ PPH_LIST Regions,
    _In_ ULONG_PTR BitmapBase,
    _In_ ULONG64 ClaimedSize,
    _In_ BOOLEAN Wow64,
    _Inout_ PPH_STRING_BUILDER Message
    )
{
    PVOID allocationBase = (PVOID)BitmapBase;
    BOOLEAN found = FALSE;
    SIZE_T committed = 0;
    ULONG_PTR low = ULONG_PTR_MAX;
    ULONG_PTR high = 0;
    ULONG count = 0;
    PPH_STRING claimedString;
    PPH_STRING committedString;
    PPH_STRING excludedString;

    if (BitmapBase == 0)
        return;

    // The reservation's allocation base is shared by all its committed
    // sub-regions; CfgBitMap usually equals it, but fall back to the region
    // that actually contains the bitmap address.
    for (ULONG i = 0; i < Regions->Count; i++)
    {
        PPH_CFG_MEMORY_REGION region = Regions->Items[i];

        if (region->BasicInformation.State == MEM_COMMIT &&
            region->BasicInformation.AllocationBase == (PVOID)BitmapBase)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        for (ULONG i = 0; i < Regions->Count; i++)
        {
            PPH_CFG_MEMORY_REGION region = Regions->Items[i];

            if (region->BasicInformation.State == MEM_COMMIT &&
                (ULONG_PTR)region->BasicInformation.BaseAddress <= BitmapBase &&
                BitmapBase < (ULONG_PTR)PTR_ADD_OFFSET(region->BasicInformation.BaseAddress, region->BasicInformation.RegionSize)
                )
            {
                allocationBase = region->BasicInformation.AllocationBase;
                break;
            }
        }
    }

    for (ULONG i = 0; i < Regions->Count; i++)
    {
        PPH_CFG_MEMORY_REGION region = Regions->Items[i];

        if (region->BasicInformation.State != MEM_COMMIT ||
            region->BasicInformation.AllocationBase != allocationBase)
            continue;

        committed += region->BasicInformation.RegionSize;
        count++;

        if ((ULONG_PTR)region->BasicInformation.BaseAddress < low)
            low = (ULONG_PTR)region->BasicInformation.BaseAddress;
        if ((ULONG_PTR)PTR_ADD_OFFSET(region->BasicInformation.BaseAddress, region->BasicInformation.RegionSize) > high)
            high = (ULONG_PTR)PTR_ADD_OFFSET(region->BasicInformation.BaseAddress, region->BasicInformation.RegionSize);
    }

    claimedString = PhFormatSize(ClaimedSize, ULONG_MAX);
    committedString = PhFormatSize(committed, ULONG_MAX);
    excludedString = PhFormatSize(ClaimedSize > committed ? ClaimedSize - committed : 0, ULONG_MAX);

    PhAppendFormatStringBuilder(
        Message,
        L"%s bitmap @ 0x%I64x: claimed %s, committed %s (%lu region%s), uncommitted %s.\r\n",
        Wow64 ? L"32-bit" : L"Native",
        (ULONG64)BitmapBase,
        PhGetString(claimedString),
        PhGetString(committedString),
        count,
        count == 1 ? L"" : L"s",
        PhGetString(excludedString)
        );

    PhDereferenceObject(claimedString);
    PhDereferenceObject(committedString);
    PhDereferenceObject(excludedString);

    for (ULONG i = 0; i < Regions->Count; i++)
    {
        PPH_CFG_MEMORY_REGION region = Regions->Items[i];

        if (region->BasicInformation.State != MEM_COMMIT ||
            region->BasicInformation.AllocationBase != allocationBase)
            continue;

        PhCfgDecodeRegion(Context, region, BitmapBase, Wow64);

        if (Context->Truncated)
            break;
    }
}

NTSTATUS PhCfgEnumerateBitmap(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;
    PS_SYSTEM_DLL_INIT_BLOCK initBlock = { 0 };
    PPH_LIST regions;
    PH_STRING_BUILDER message;
    BOOLEAN isWow64 = FALSE;

    Context->EntryList = PhCreateList(256);
    Context->DisplayEntryList = PhCreateList(256);
    Context->EntryCount = 0;
    Context->Truncated = FALSE;

    PhCfgCollectImageTargets(Context);

    status = PhGetProcessSystemDllInitBlock(Context->ProcessHandle, &initBlock);

    if (!NT_SUCCESS(status))
        return status;

    regions = PhCreateList(512);
    Context->RegionList = regions;

    status = PhEnumVirtualMemory(
        Context->ProcessHandle,
        PhCfgEnumRegionsCallback,
        regions
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitializeStringBuilder(&message, 256);

    if (RTL_CONTAINS_FIELD(&initBlock, initBlock.Size, CfgBitMapSize) && initBlock.CfgBitMap)
    {
        PhCfgProcessBitmap(
            Context,
            regions,
            (ULONG_PTR)initBlock.CfgBitMap,
            initBlock.CfgBitMapSize,
            FALSE,
            &message
            );
    }
    else
    {
        PhAppendStringBuilder2(&message, L"No CFG bitmap is present for this process.\r\n");
    }

    PhGetProcessIsWow64(Context->ProcessHandle, &isWow64);

    if (isWow64 &&
        RTL_CONTAINS_FIELD(&initBlock, initBlock.Size, Wow64CfgBitMapSize) &&
        initBlock.Wow64CfgBitMap)
    {
        PhCfgProcessBitmap(
            Context,
            regions,
            (ULONG_PTR)initBlock.Wow64CfgBitMap,
            initBlock.Wow64CfgBitMapSize,
            TRUE,
            &message
            );
    }

    if (Context->Truncated)
    {
        PhAppendFormatStringBuilder(
            &message,
            L"Listing truncated to %lu entries.\r\n",
            PH_CFG_MAX_ENTRIES
            );
    }

    PhSetWindowText(Context->MessageHandle, message.String->Buffer);
    PhDeleteStringBuilder(&message);

    PhClearReference(&Context->ImageCfgTargetHashtable);
    PhCfgApplyFilters(Context);
    PhCfgProcessSymbolLookupResults(Context);

    return STATUS_SUCCESS;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhCfgSymbolProviderEventCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_SYMBOL_EVENT_DATA event = Parameter;
    PPH_CFG_BITMAP_DIALOG_CONTEXT context = Context;
    PPH_STRING statusMessage = NULL;

    switch (event->EventType)
    {
    case PH_SYMBOL_EVENT_TYPE_LOAD_START:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    case PH_SYMBOL_EVENT_TYPE_LOAD_END:
        PhCfgUpdateStatus(context);
        break;
    case PH_SYMBOL_EVENT_TYPE_PROGRESS:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    }

    if (statusMessage)
    {
        PhSetWindowText(context->StatusHandle, PhGetStringOrEmpty(statusMessage));
        PhDereferenceObject(statusMessage);
    }
}

VOID PhCfgRefresh(
    _In_ PPH_CFG_BITMAP_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_SetItemCount(Context->ListViewHandle, 0);
    ListView_DeleteAllItems(Context->ListViewHandle);

    PhCfgCleanupEntries(Context);

    status = PhCfgEnumerateBitmap(Context);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(Context->WindowHandle, L"Unable to enumerate the CFG bitmap.", status, 0);
    }
}

INT_PTR CALLBACK PhCfgBitmapDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_CFG_BITMAP_DIALOG_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PPH_CFG_BITMAP_DIALOG_CONTEXT)lParam;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);
            context->StatusHandle = GetDlgItem(WindowHandle, IDC_STATE);
            context->MessageHandle = GetDlgItem(WindowHandle, IDC_MESSAGE);

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 55, L"Bitmap");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 110, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 120, L"Address");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 120, L"End");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 70, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 260, L"Symbol");
            PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 120, L"Allocation base");
            PhAddListViewColumn(context->ListViewHandle, 8, 8, 8, LVCFMT_LEFT, 120, L"Region base");
            PhAddListViewColumn(context->ListViewHandle, 9, 9, 9, LVCFMT_RIGHT, 90, L"Region size");
            PhAddListViewColumn(context->ListViewHandle, 10, 10, 10, LVCFMT_LEFT, 90, L"Memory type");
            PhAddListViewColumn(context->ListViewHandle, 11, 11, 11, LVCFMT_LEFT, 80, L"State");
            PhAddListViewColumn(context->ListViewHandle, 12, 12, 12, LVCFMT_LEFT, 90, L"Protection");
            PhAddListViewColumn(context->ListViewHandle, 13, 13, 13, LVCFMT_LEFT, 260, L"Mapped file");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_MEMORY_CFG_LIST_VIEW_COLUMNS, context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(SETTING_MEMORY_CFG_LIST_VIEW_SORT, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->MessageHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->StatusHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_OPTIONS), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_MEMORY_CFG_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_MEMORY_CFG_WINDOW_POSITION, SETTING_MEMORY_CFG_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            {
                PPH_STRING windowTitle;

                windowTitle = PhGetWindowText(WindowHandle);
                PhMoveReference(&windowTitle, PhFormatString(
                    L"%s: %s (%lu)",
                    PhGetStringOrEmpty(windowTitle),
                    PhGetStringOrEmpty(context->ProcessItem->ProcessName),
                    HandleToUlong(context->ProcessItem->ProcessId)
                    ));

                PhSetWindowText(WindowHandle, PhGetStringOrEmpty(windowTitle));
                PhDereferenceObject(windowTitle);
            }

            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 1000, NULL);

            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);

            PhRegisterCallback(
                &PhSymbolEventCallback,
                PhCfgSymbolProviderEventCallback,
                context,
                &context->SymbolProviderEventRegistration
                );

            PhCfgRefresh(context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(SETTING_MEMORY_CFG_LIST_VIEW_SORT, context->ListViewHandle);
            PhSaveListViewColumnsToSetting(SETTING_MEMORY_CFG_LIST_VIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_MEMORY_CFG_WINDOW_POSITION, SETTING_MEMORY_CFG_WINDOW_SIZE, WindowHandle);

            context->Destroying = TRUE;

            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhUnregisterCallback(
                &PhSymbolEventCallback,
                &context->SymbolProviderEventRegistration
                );

            PhCfgDereferenceContext(context);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                PhCfgProcessSymbolLookupResults(context);
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDOK:
                EndDialog(WindowHandle, IDOK);
                break;
            case IDC_REFRESH:
                PhCfgRefresh(context);
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM imageItem;
                    PPH_EMENU_ITEM modifiedItem;
                    PPH_EMENU_ITEM allValidItem;
                    PPH_EMENU_ITEM selectedItem;

                    if (!PhGetWindowRect(GetDlgItem(WindowHandle, IDC_OPTIONS), &rect))
                        break;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, imageItem = PhCreateEMenuItem(0, PH_CFG_MENU_SHOW_IMAGE_TARGETS, L"Show image CFG targets", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, modifiedItem = PhCreateEMenuItem(0, PH_CFG_MENU_SHOW_MODIFIED_TARGETS, L"Show modified/allowed targets", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, allValidItem = PhCreateEMenuItem(0, PH_CFG_MENU_SHOW_ALL_VALID_RANGES, L"Show all-valid (no CFG) ranges", NULL, NULL), ULONG_MAX);

                    if (context->ShowImageTargets)
                        imageItem->Flags |= PH_EMENU_CHECKED;
                    if (context->ShowModifiedTargets)
                        modifiedItem->Flags |= PH_EMENU_CHECKED;
                    if (context->ShowAllValidRanges)
                        allValidItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem)
                    {
                        switch (selectedItem->Id)
                        {
                        case PH_CFG_MENU_SHOW_IMAGE_TARGETS:
                            context->ShowImageTargets = !context->ShowImageTargets;
                            break;
                        case PH_CFG_MENU_SHOW_MODIFIED_TARGETS:
                            context->ShowModifiedTargets = !context->ShowModifiedTargets;
                            break;
                        case PH_CFG_MENU_SHOW_ALL_VALID_RANGES:
                            context->ShowAllValidRanges = !context->ShowAllValidRanges;
                            break;
                        }

                        PhCfgApplyFilters(context);
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            if (header->hwndFrom == context->ListViewHandle)
            {
                switch (header->code)
                {
                case LVN_GETDISPINFO:
                    {
                        NMLVDISPINFO *dispInfo = (NMLVDISPINFO *)header;

                        if (FlagOn(dispInfo->item.mask, LVIF_TEXT) &&
                            dispInfo->item.iItem >= 0 &&
                            context->DisplayEntryList &&
                            (ULONG)dispInfo->item.iItem < context->DisplayEntryList->Count)
                        {
                            PPH_CFG_ENTRY entry = context->DisplayEntryList->Items[dispInfo->item.iItem];
                            WCHAR value[PH_INT64_STR_LEN_1];
                            PPH_STRING sizeString;

                            switch (dispInfo->item.iSubItem)
                            {
                            case 0:
                                {
                                    PhPrintUInt32(value, (ULONG)dispInfo->item.iItem + 1);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                                }
                                break;
                            case 1:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, entry->Wow64 ? L"32-bit" : L"Native", _TRUNCATE);
                                }
                                break;
                            case 2:
                                {
                                    switch (entry->Type)
                                    {
                                    case PhCfgBitmapImageTarget:
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"Image CFG", _TRUNCATE);
                                        break;
                                    case PhCfgBitmapModifiedTarget:
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"Modified/allowed", _TRUNCATE);
                                        break;
                                    case PhCfgBitmapAllValidRange:
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"All valid (no CFG)", _TRUNCATE);
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                {
                                    PhPrintPointer(value, entry->TargetAddress);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                                }
                                break;
                            case 4:
                                {
                                    if (entry->RangeValid)
                                    {
                                        PhPrintPointer(value, entry->TargetEndAddress);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                                    }
                                    else
                                    {
                                        dispInfo->item.pszText[0] = 0;
                                    }
                                }
                                break;
                            case 5:
                                {
                                    if (entry->RangeValid)
                                    {
                                        PH_FORMAT format[1];

                                        PhInitFormatSize(&format[0], (ULONG64)PTR_SUB_OFFSET(entry->TargetEndAddress, entry->TargetAddress));
                                        sizeString = PhFormat(format, 1, 16);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetString(sizeString), _TRUNCATE);
                                        PhDereferenceObject(sizeString);
                                    }
                                    else
                                    {
                                        // The 2-bit value: 1 = aligned target, 3 = aligned + 8-byte sub-offset (XFG),
                                        // 2 = export-suppressed. Show the raw value for expert interpretation.
                                        PhPrintUInt32(value, entry->Value);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                                    }
                                }
                                break;
                            case 6:
                                {
                                    wcsncpy_s(
                                        dispInfo->item.pszText,
                                        dispInfo->item.cchTextMax,
                                        entry->Symbol ? PhGetString(entry->Symbol) : L"Resolving symbols...",
                                        _TRUNCATE
                                        );
                                }
                                break;
                            case 7:
                                {
                                    if (entry->Region && entry->Region->BasicInformation.AllocationBase)
                                    {
                                        PhPrintPointer(value, entry->Region->BasicInformation.AllocationBase);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                                    }
                                    else
                                    {
                                        dispInfo->item.pszText[0] = UNICODE_NULL;
                                    }
                                }
                                break;
                            case 8:
                                {
                                    if (entry->Region)
                                    {
                                        PhPrintPointer(value, entry->Region->BasicInformation.BaseAddress);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                                    }
                                    else
                                    {
                                        dispInfo->item.pszText[0] = UNICODE_NULL;
                                    }
                                }
                                break;
                            case 9:
                                {
                                    if (entry->Region)
                                    {
                                        PH_FORMAT format[1];

                                        PhInitFormatSize(&format[0], entry->Region->BasicInformation.RegionSize);
                                        sizeString = PhFormat(format, 1, 16);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetString(sizeString), _TRUNCATE);
                                        PhDereferenceObject(sizeString);
                                    }
                                    else
                                    {
                                        dispInfo->item.pszText[0] = UNICODE_NULL;
                                    }
                                }
                                break;
                            case 10:
                                {
                                    PCPH_STRINGREF typeText;

                                    if (entry->Region && (typeText = PhGetMemoryTypeString(entry->Region->BasicInformation.Type)))
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, typeText->Buffer, _TRUNCATE);
                                    else
                                        dispInfo->item.pszText[0] = UNICODE_NULL;
                                }
                                break;
                            case 11:
                                {
                                    PCPH_STRINGREF stateText;

                                    if (entry->Region && (stateText = PhGetMemoryStateString(entry->Region->BasicInformation.State)))
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, stateText->Buffer, _TRUNCATE);
                                    else
                                        dispInfo->item.pszText[0] = UNICODE_NULL;
                                }
                                break;
                            case 12:
                                {
                                    WCHAR protectionText[17];

                                    if (entry->Region && entry->Region->BasicInformation.Protect)
                                    {
                                        PhGetMemoryProtectionString(entry->Region->BasicInformation.Protect, protectionText);
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, protectionText, _TRUNCATE);
                                    }
                                    else
                                    {
                                        dispInfo->item.pszText[0] = UNICODE_NULL;
                                    }
                                }
                                break;
                            case 13:
                                {
                                    wcsncpy_s(
                                        dispInfo->item.pszText,
                                        dispInfo->item.cchTextMax,
                                        entry->Region ? PhGetStringOrEmpty(entry->Region->MappedFileName) : L"",
                                        _TRUNCATE
                                        );
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (ListView_GetSelectedCount(context->ListViewHandle) > 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhShowProcessCfgBitmapDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_CFG_BITMAP_DIALOG_CONTEXT context;
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process.", status, 0);
        return;
    }

    context = PhAllocateZero(sizeof(PH_CFG_BITMAP_DIALOG_CONTEXT));
    context->RefCount = 1;
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->ParentWindowHandle = ParentWindowHandle;
    context->ProcessHandle = processHandle;
    context->ShowModifiedTargets = TRUE;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMCFG),
        NULL,
        PhCfgBitmapDlgProc,
        context
        );
}

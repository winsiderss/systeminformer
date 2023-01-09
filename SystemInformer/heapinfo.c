/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2020-2023
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <phsvccl.h>
#include <actions.h>
#include <emenu.h>
#include <mainwnd.h>
#include <procprv.h>
#include <settings.h>

typedef struct _PH_PROCESS_HEAPS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HFONT BoldFont;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN Initialized : 1;
            BOOLEAN IsWow64 : 1;
            BOOLEAN Spare : 6;
        };
    };
    PPH_PROCESS_ITEM ProcessItem;
    PVOID ProcessHeap;
    PVOID DebugBuffer;
    PH_LAYOUT_MANAGER LayoutManager;
} PH_PROCESS_HEAPS_CONTEXT, *PPH_PROCESS_HEAPS_CONTEXT;

typedef struct _HEAP_COUNTERS
{
    ULONG_PTR TotalMemoryReserved;
    ULONG_PTR TotalMemoryCommitted;
    ULONG_PTR TotalMemoryLargeUCR;
    ULONG_PTR TotalSizeInVirtualBlocks;
    ULONG TotalSegments;
    ULONG TotalUCRs;
    ULONG CommittOps;
    ULONG DeCommitOps;
    ULONG LockAcquires;
    ULONG LockCollisions;
    ULONG CommitRate;
    ULONG DecommittRate;
    ULONG CommitFailures;
    ULONG InBlockCommitFailures;
    ULONG PollIntervalCounter;
    ULONG DecommitsSinceLastCheck;
    ULONG HeapPollInterval;
    ULONG AllocAndFreeOps;
    ULONG AllocationIndicesActive;
    ULONG InBlockDeccommits;
    ULONG_PTR InBlockDeccomitSize;
    ULONG_PTR HighWatermarkSize;
    ULONG_PTR LastPolledSize;
} HEAP_COUNTERS, *PHEAP_COUNTERS;

typedef struct _HEAP_COUNTERS32
{
    ULONG TotalMemoryReserved;
    ULONG TotalMemoryCommitted;
    ULONG TotalMemoryLargeUCR;
    ULONG TotalSizeInVirtualBlocks;
    ULONG TotalSegments;
    ULONG TotalUCRs;
    ULONG CommittOps;
    ULONG DeCommitOps;
    ULONG LockAcquires;
    ULONG LockCollisions;
    ULONG CommitRate;
    ULONG DecommittRate;
    ULONG CommitFailures;
    ULONG InBlockCommitFailures;
    ULONG PollIntervalCounter;
    ULONG DecommitsSinceLastCheck;
    ULONG HeapPollInterval;
    ULONG AllocAndFreeOps;
    ULONG AllocationIndicesActive;
    ULONG InBlockDeccommits;
    ULONG InBlockDeccomitSize;
    ULONG HighWatermarkSize;
    ULONG LastPolledSize;
} HEAP_COUNTERS32, *PHEAP_COUNTERS32;

typedef struct _HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS
{
    ULONG_PTR SmallPagesInUseWithinLarge;
    ULONG_PTR OpportunisticLargePageCount;
} HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS, *PHEAP_OPPORTUNISTIC_LARGE_PAGE_STATS;

typedef struct _HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS32
{
    ULONG SmallPagesInUseWithinLarge;
    ULONG OpportunisticLargePageCount;
} HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS32, *PHEAP_OPPORTUNISTIC_LARGE_PAGE_STATS32;

typedef struct _HEAP_RUNTIME_MEMORY_STATS
{
    ULONG_PTR TotalReservedPages;
    ULONG_PTR TotalCommittedPages;
    ULONG_PTR FreeCommittedPages;
    ULONG_PTR LfhFreeCommittedPages;
    HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS LargePageStats[2];
    //RTL_HP_SEG_ALLOC_POLICY LargePageUtilizationPolicy;
} HEAP_RUNTIME_MEMORY_STATS, *PHEAP_RUNTIME_MEMORY_STATS;

typedef struct _HEAP_RUNTIME_MEMORY_STATS32
{
    ULONG TotalReservedPages;
    ULONG TotalCommittedPages;
    ULONG FreeCommittedPages;
    ULONG LfhFreeCommittedPages;
    HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS32 LargePageStats[2];
    //RTL_HP_SEG_ALLOC_POLICY32 LargePageUtilizationPolicy;
} HEAP_RUNTIME_MEMORY_STATS32, *PHEAP_RUNTIME_MEMORY_STATS32;

NTSTATUS PhGetProcessDefaultHeap(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID *Heap
    );

NTSTATUS PhGetProcessHeapSignature(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ ULONG* HeapSignature
    );

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowProcessHeapsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_HEAPS_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_PROCESS_HEAPS_CONTEXT));
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->IsWow64 = !!ProcessItem->IsWow64;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_HEAPS),
        NULL,
        PhpProcessHeapsDlgProc,
        (LPARAM)context
        );
}

static INT NTAPI PhpHeapAddressCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_HEAPS_CONTEXT context = Context;

    if (context->IsWow64)
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo2 = Item2;

        return uintptrcmp((ULONG_PTR)heapInfo1->BaseAddress, (ULONG_PTR)heapInfo2->BaseAddress);
    }
    else
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo2 = Item2;

        return uintptrcmp((ULONG_PTR)heapInfo1->BaseAddress, (ULONG_PTR)heapInfo2->BaseAddress);
    }
}

static INT NTAPI PhpHeapUsedCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_HEAPS_CONTEXT context = Context;

    if (context->IsWow64)
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo2 = Item2;

        return uintptrcmp(heapInfo1->BytesAllocated, heapInfo2->BytesAllocated);
    }
    else
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo2 = Item2;

        return uintptrcmp(heapInfo1->BytesAllocated, heapInfo2->BytesAllocated);
    }
}

static INT NTAPI PhpHeapCommittedCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_HEAPS_CONTEXT context = Context;

    if (context->IsWow64)
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo2 = Item2;

        return uintptrcmp(heapInfo1->BytesCommitted, heapInfo2->BytesCommitted);
    }
    else
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo2 = Item2;

        return uintptrcmp(heapInfo1->BytesCommitted, heapInfo2->BytesCommitted);
    }
}

static INT NTAPI PhpHeapEntriesCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_HEAPS_CONTEXT context = Context;

    if (context->IsWow64)
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo2 = Item2;

        return uintcmp(heapInfo1->NumberOfEntries, heapInfo2->NumberOfEntries);
    }
    else
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo1 = Item1;
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo2 = Item2;

        return uintcmp(heapInfo1->NumberOfEntries, heapInfo2->NumberOfEntries);
    }
}

static HFONT NTAPI PhpHeapFontFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_ PVOID Context
    )
{
    PVOID heapBaseAddress = Param;
    PPH_PROCESS_HEAPS_CONTEXT context = Context;

    if (context->IsWow64)
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo = Param;
        heapBaseAddress = UlongToPtr(heapInfo->BaseAddress);
    }
    else
    {
        PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo = Param;
        heapBaseAddress = heapInfo->BaseAddress;
    }

    if (!context->ProcessHeap)
    {
        HANDLE processHandle;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
            context->ProcessItem->ProcessId
            )))
        {
            PhGetProcessDefaultHeap(processHandle, &context->ProcessHeap);
            NtClose(processHandle);
        }
    }

    if (heapBaseAddress == context->ProcessHeap)
    {
        if (!context->BoldFont)
            context->BoldFont = PhDuplicateFontWithNewWeight((HFONT)SendMessage(context->ListViewHandle, WM_GETFONT, 0, 0), FW_BOLD);

        return context->BoldFont;
    }

    return NULL;
}

PPH_STRING PhGetProcessHeapFlagsText(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (Flags & HEAP_NO_SERIALIZE)
        PhAppendStringBuilder2(&stringBuilder, L"No serialize, ");
    if (Flags & HEAP_GROWABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Growable, ");
    if (Flags & HEAP_GENERATE_EXCEPTIONS)
        PhAppendStringBuilder2(&stringBuilder, L"Generate exceptions, ");
    if (Flags & HEAP_ZERO_MEMORY)
        PhAppendStringBuilder2(&stringBuilder, L"Zero memory, ");
    if (Flags & HEAP_REALLOC_IN_PLACE_ONLY)
        PhAppendStringBuilder2(&stringBuilder, L"Realloc in-place, ");
    if (Flags & HEAP_TAIL_CHECKING_ENABLED)
        PhAppendStringBuilder2(&stringBuilder, L"Tail checking, ");
    if (Flags & HEAP_FREE_CHECKING_ENABLED)
        PhAppendStringBuilder2(&stringBuilder, L"Free checking, ");
    if (Flags & HEAP_DISABLE_COALESCE_ON_FREE)
        PhAppendStringBuilder2(&stringBuilder, L"Coalesce on free, ");
    if (Flags & HEAP_CREATE_ALIGN_16)
        PhAppendStringBuilder2(&stringBuilder, L"Align 16, ");
    if (Flags & HEAP_CREATE_ENABLE_TRACING)
        PhAppendStringBuilder2(&stringBuilder, L"Traceable, ");
    if (Flags & HEAP_CREATE_ENABLE_EXECUTE)
        PhAppendStringBuilder2(&stringBuilder, L"Executable, ");
    if (Flags & HEAP_CREATE_SEGMENT_HEAP)
        PhAppendStringBuilder2(&stringBuilder, L"Segment heap, ");
    if (Flags & HEAP_CREATE_HARDENED)
        PhAppendStringBuilder2(&stringBuilder, L"Segment hardened, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    if (Flags)
    {
        WCHAR pointer[PH_PTR_STR_LEN_1];

        PhPrintPointer(pointer, UlongToPtr(Flags));

        if (PhIsNullOrEmptyString(stringBuilder.String))
            PhAppendFormatStringBuilder(&stringBuilder, L"%s", pointer);
        else
            PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

PWSTR PhGetProcessHeapClassText(
    _In_ ULONG HeapClass
    )
{
    switch (HeapClass)
    {
    case HEAP_CLASS_0:
        return L"Process Heap";
    case HEAP_CLASS_1:
        return L"Private Heap";
    case HEAP_CLASS_2:
        return L"Kernel Heap";
    case HEAP_CLASS_3:
        return L"GDI Heap";
    case HEAP_CLASS_4:
        return L"User Heap";
    case HEAP_CLASS_5:
        return L"Console Heap";
    case HEAP_CLASS_6:
        return L"Desktop Heap";
    case HEAP_CLASS_7:
        return L"CSRSS Shared Heap";
    case HEAP_CLASS_8:
        return L"CSRSS Port Heap";
    }

    return L"Unknown";
}

VOID PhpEnumerateProcessHeaps(
    _In_ PPH_PROCESS_HEAPS_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PROCESS_REFLECTION_INFORMATION reflectionInfo = { 0 };
    HANDLE clientProcessId = Context->ProcessItem->ProcessId;
    BOOLEAN sizesInBytes;

    sizesInBytes = Button_GetCheck(GetDlgItem(Context->WindowHandle, IDC_SIZESINBYTES)) == BST_CHECKED;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    if (Context->DebugBuffer)
    {
        PhFree(Context->DebugBuffer);
        Context->DebugBuffer = NULL;
    }

    if (WindowsVersion >= WINDOWS_8 && WindowsVersion <= WINDOWS_8_1)
    {
        // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
        status = PhOpenProcess(
            &processHandle,
            PROCESS_ALL_ACCESS,
            Context->ProcessItem->ProcessId
            );
    }
    else if (WindowsVersion >= WINDOWS_10)
    {
        // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_LIMITED_INFORMATION,
            Context->ProcessItem->ProcessId
            );
    }

    if (processHandle)
    {
        PhCreateExecutionRequiredRequest(processHandle, &powerRequestHandle);
    }

    if (PhGetIntegerSetting(L"EnableHeapReflection"))
    {
        // NOTE: RtlQueryProcessDebugInformation injects a thread into the process causing deadlocks and other issues in rare cases.
        // We mitigate these problems by reflecting the process and querying heap information from the clone. (dmex)

        status = PhCreateProcessReflection(
            &reflectionInfo,
            NULL,
            clientProcessId
            );

        if (NT_SUCCESS(status))
        {
            clientProcessId = reflectionInfo.ReflectionClientId.UniqueProcess;
        }
    }

#ifdef _WIN64
    if (Context->ProcessItem->IsWow64)
    {
        if (PhUiConnectToPhSvcEx(Context->WindowHandle, Wow64PhSvcMode, FALSE))
        {
            PPH_STRING capturedHeapInfoString;
            PPH_PROCESS_DEBUG_HEAP_INFORMATION32 capturedHeapInfo;
            ULONG capturedHeapInfoLength;

            status = PhSvcCallQueryProcessHeapInformation(
                clientProcessId,
                &capturedHeapInfoString
                );

            if (!NT_SUCCESS(status))
            {
                PhUiDisconnectFromPhSvc();

                PhShowStatus(Context->WindowHandle, L"Unable to query heap information.", status, 0);
                goto CleanupExit;
            }

            PhUiDisconnectFromPhSvc();

            capturedHeapInfoLength = sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION32) + 100 * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY32);
            capturedHeapInfo = PhAllocateZero(capturedHeapInfoLength);

            if (!PhHexStringToBufferEx(
                &capturedHeapInfoString->sr,
                capturedHeapInfoLength,
                capturedHeapInfo
                ))
            {
                PhFree(capturedHeapInfo);
                goto CleanupExit;
            }

            Context->DebugBuffer = capturedHeapInfo;
            Context->ProcessHeap = UlongToPtr(capturedHeapInfo->DefaultHeap);

            for (ULONG i = 0; i < capturedHeapInfo->NumberOfHeaps; i++)
            {
                PPH_PROCESS_DEBUG_HEAP_ENTRY32 entry = &capturedHeapInfo->Heaps[i];
                INT lvItemIndex;
                WCHAR value[PH_INT64_STR_LEN_1];

                PhPrintUInt32(value, i + 1);
                lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, entry);
                PhPrintPointer(value, UlongToPtr(entry->BaseAddress));
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, value);
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhaFormatSize(entry->BytesAllocated, sizesInBytes ? 0 : ULONG_MAX)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhaFormatSize(entry->BytesCommitted, sizesInBytes ? 0 : ULONG_MAX)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, PhaFormatUInt64(entry->NumberOfEntries, TRUE)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, PH_AUTO_T(PH_STRING, PhGetProcessHeapFlagsText(entry->Flags & ~HEAP_CLASS_MASK))->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 6, PhGetProcessHeapClassText(entry->Flags & HEAP_CLASS_MASK));

                switch (entry->Signature)
                {
                case RTL_HEAP_SIGNATURE:
                    {
                        switch (entry->HeapFrontEndType)
                        {
                        case 1:
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"NT Heap (Lookaside)");
                            break;
                        case 2:
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"NT Heap (LFH)");
                            break;
                        default:
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"NT Heap");
                            break;
                        }
                    }
                    break;
                case RTL_HEAP_SEGMENT_SIGNATURE:
                    {
                        switch (entry->HeapFrontEndType)
                        {
                        case 1:
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"Segment Heap (Lookaside)");
                            break;
                        case 2:
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"Segment Heap (LFH)");
                            break;
                        default:
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"Segment Heap");
                            break;
                        }
                    }
                    break;
                }
            }

            PhDereferenceObject(capturedHeapInfoString);
        }
        else
        {
            PhShowError2(
                Context->WindowHandle,
                L"Unable to query 32bit heap information.",
                L"%s",
                L"The 32-bit version of System Informer could not be located."
                );
            goto CleanupExit;
        }
    }
    else
    {
#endif
        PPH_PROCESS_DEBUG_HEAP_INFORMATION heapDebugInfo;

        status = PhQueryProcessHeapInformation(
            clientProcessId,
            &heapDebugInfo
            );

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(Context->WindowHandle, L"Unable to query heap information.", status, 0);
            goto CleanupExit;
        }

        Context->DebugBuffer = heapDebugInfo;
        Context->ProcessHeap = heapDebugInfo->DefaultHeap;

        for (ULONG i = 0; i < heapDebugInfo->NumberOfHeaps; i++)
        {
            PPH_PROCESS_DEBUG_HEAP_ENTRY entry = &heapDebugInfo->Heaps[i];
            INT lvItemIndex;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt32(value, i + 1);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, entry);
            PhPrintPointer(value, entry->BaseAddress);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, value);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhaFormatSize(entry->BytesAllocated, sizesInBytes ? 0 : ULONG_MAX)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhaFormatSize(entry->BytesCommitted, sizesInBytes ? 0 : ULONG_MAX)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, PhaFormatUInt64(entry->NumberOfEntries, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, PH_AUTO_T(PH_STRING, PhGetProcessHeapFlagsText(entry->Flags & ~HEAP_CLASS_MASK))->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 6, PhGetProcessHeapClassText(entry->Flags & HEAP_CLASS_MASK));

            switch (entry->Signature)
            {
            case RTL_HEAP_SIGNATURE:
                {
                    switch (entry->HeapFrontEndType)
                    {
                    case 1:
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"NT Heap (Lookaside)");
                        break;
                    case 2:
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"NT Heap (LFH)");
                        break;
                    default:
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"NT Heap");
                        break;
                    }
                }
                break;
            case RTL_HEAP_SEGMENT_SIGNATURE:
                {
                    switch (entry->HeapFrontEndType)
                    {
                    case 1:
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"Segment Heap (Lookaside)");
                        break;
                    case 2:
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"Segment Heap (LFH)");
                        break;
                    default:
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"Segment Heap");
                        break;
                    }
                }
                break;
            }
        }

#ifdef _WIN64
    }
#endif

CleanupExit:
    PhFreeProcessReflection(&reflectionInfo);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

VOID PhpSetProcessHeapsWindowText(
    _In_ PPH_PROCESS_HEAPS_CONTEXT Context
    )
{
    PH_FORMAT format[5];
    WCHAR formatBuffer[260];

    PhInitFormatS(&format[0], L"Heaps - ");
    PhInitFormatSR(&format[1], Context->ProcessItem->ProcessName->sr);
    PhInitFormatS(&format[2], L" (");
    PhInitFormatU(&format[3], HandleToUlong(Context->ProcessItem->ProcessId));
    PhInitFormatC(&format[4], L')');

    if (PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        formatBuffer,
        sizeof(formatBuffer),
        NULL
        ))
    {
        PhSetWindowText(Context->WindowHandle, formatBuffer);
    }
}

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
     PPH_PROCESS_HEAPS_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_PROCESS_HEAPS_CONTEXT)lParam;
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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhpSetProcessHeapsWindowText(context);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Address");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 120, L"Used");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 120, L"Committed");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Entries");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Flags");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Class");
            PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 80, L"Type");
            PhSetExtendedListView(context->ListViewHandle);

            ExtendedListView_SetContext(context->ListViewHandle, context);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PhpHeapAddressCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, PhpHeapUsedCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 3, PhpHeapCommittedCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 4, PhpHeapEntriesCompareFunction);
            ExtendedListView_SetItemFontFunction(context->ListViewHandle, PhpHeapFontFunction);

            PhLoadListViewColumnsFromSetting(L"SegmentHeapListViewColumns", context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(L"SegmentHeapListViewSort", context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SIZESINBYTES), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhGetIntegerPairSetting(L"SegmentHeapWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"SegmentHeapWindowPosition", L"SegmentHeapWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(L"SegmentHeapListViewSort", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"SegmentHeapListViewColumns", context->ListViewHandle);
            PhSaveWindowPlacementToSetting(L"SegmentHeapWindowPosition", L"SegmentHeapWindowSize", hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->BoldFont)
            {
                DeleteFont(context->BoldFont);
                context->BoldFont = NULL;
            }

            if (context->DebugBuffer)
            {
                PhFree(context->DebugBuffer);
                context->DebugBuffer = NULL;
            }

            if (context->ProcessItem)
            {
                PhDereferenceObject(context->ProcessItem);
                context->ProcessItem = NULL;
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!context->Initialized)
            {
                PostMessage(context->WindowHandle, WM_COMMAND, MAKEWPARAM(IDC_REFRESH, 0), 0);
                context->Initialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_SIZESINBYTES:
                {
                    BOOLEAN sizesInBytes = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED;
                    INT index = -1;

                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

                    while ((index = ListView_GetNextItem(context->ListViewHandle, index, LVNI_ALL)) != -1)
                    {
                        PPH_STRING usedString = NULL;
                        PPH_STRING committedString = NULL;

                        if (context->IsWow64)
                        {
                            PPH_PROCESS_DEBUG_HEAP_ENTRY32 heapInfo;

                            if (PhGetListViewItemParam(context->ListViewHandle, index, &heapInfo))
                            {
                                usedString = PhFormatSize(heapInfo->BytesAllocated, sizesInBytes ? 0 : ULONG_MAX);
                                committedString = PhFormatSize(heapInfo->BytesCommitted, sizesInBytes ? 0 : ULONG_MAX);
                            }
                        }
                        else
                        {
                            PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo;

                            if (PhGetListViewItemParam(context->ListViewHandle, index, &heapInfo))
                            {
                                usedString = PhFormatSize(heapInfo->BytesAllocated, sizesInBytes ? 0 : ULONG_MAX);
                                committedString = PhFormatSize(heapInfo->BytesCommitted, sizesInBytes ? 0 : ULONG_MAX);
                            }
                        }

                        if (usedString)
                        {
                            PhSetListViewSubItem(context->ListViewHandle, index, 2, usedString->Buffer);
                            PhDereferenceObject(usedString);
                        }

                        if (committedString)
                        {
                            PhSetListViewSubItem(context->ListViewHandle, index, 3, committedString->Buffer);
                            PhDereferenceObject(committedString);
                        }
                    }

                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                }
                break;
            case IDC_REFRESH:
                {
                    PhpEnumerateProcessHeaps(context);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_PROCESS_DEBUG_HEAP_ENTRY heapInfo;
                PPH_EMENU menu;
                INT selectedCount;
                PPH_EMENU_ITEM menuItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                selectedCount = ListView_GetSelectedCount(context->ListViewHandle);
                heapInfo = PhGetSelectedListViewItemParam(context->ListViewHandle);

                if (selectedCount != 0)
                {
                    menu = PhCreateEMenu();
                    //PhInsertEMenuItem(menu, PhCreateEMenuItem(selectedCount != 1 ? PH_EMENU_DISABLED : 0, 1, L"Destroy", NULL, NULL), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, USHRT_MAX, context->ListViewHandle);

                    menuItem = PhShowEMenu(
                        menu,
                        context->ListViewHandle,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (menuItem)
                    {
                        if (PhHandleCopyListViewEMenuItem(menuItem))
                            break;

                        switch (menuItem->Id)
                        {
                        case 1:
                            //if (PhpDestroyHeap(hwndDlg, context->ProcessItem->ProcessId, heapInfo->BaseAddress))
                            //    ListView_DeleteItem(context->ListViewHandle, PhFindListViewItemByParam(context->ListViewHandle, -1, heapInfo));
                            //break;
                        case USHRT_MAX:
                            PhCopyListView(context->ListViewHandle);
                            break;
                        }
                    }

                    PhDestroyEMenu(menu);
                }
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

NTSTATUS PhGetProcessDefaultHeap(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID *Heap
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN IsWow64;

    status = PhGetProcessIsWow64(ProcessHandle, &IsWow64);

    if (!NT_SUCCESS(status))
        return status;

    if (IsWow64)
    {
        PVOID peb32;
        ULONG processHeapsPtr32;

        status = PhGetProcessPeb32(ProcessHandle, &peb32);

        if (!NT_SUCCESS(status))
            return status;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(peb32, UFIELD_OFFSET(PEB32, ProcessHeap)),
            &processHeapsPtr32,
            sizeof(ULONG),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        if (Heap)
        {
            *Heap = UlongToPtr(processHeapsPtr32);
        }

        return status;
    }
    else
    {
#endif
        PROCESS_BASIC_INFORMATION basicInfo;
        PVOID processHeapsPtr;

        status = PhGetProcessBasicInformation(
            ProcessHandle,
            &basicInfo
            );

        if (!NT_SUCCESS(status))
            return status;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, UFIELD_OFFSET(PEB, ProcessHeap)),
            &processHeapsPtr,
            sizeof(PVOID),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        if (Heap)
        {
            *Heap = processHeapsPtr;
        }

        return status;
#ifdef _WIN64
    }
#endif
}

//NTSTATUS PhGetProcessHeapCompatibilityInformation(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID HeapAddress,
//    _In_ ULONG IsWow64,
//    _Out_ ULONG* HeapCompatibility
//    )
//{
//    NTSTATUS status = STATUS_UNSUCCESSFUL;
//    ULONG frontEndHeapType = ULONG_MAX;
//
//    if (WindowsVersion >= WINDOWS_10_20H1)
//    {
//        status = NtReadVirtualMemory(
//            ProcessHandle,
//            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0xEA : 0x1A2),
//            &frontEndHeapType,
//            sizeof(ULONG),
//            NULL
//            );
//    }
//
//    if (NT_SUCCESS(status))
//    {
//        if (HeapCompatibility)
//            *HeapCompatibility = frontEndHeapType;
//    }
//
//    return status;
//}
//
//NTSTATUS PhGetProcessHeapCounters(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID HeapAddress,
//    _In_ ULONG IsWow64,
//    _Out_ PVOID *HeapCounters
//    )
//{
//    NTSTATUS status = STATUS_UNSUCCESSFUL;
//    PVOID heapCounters;
//    ULONG heapCountersLength;
//
//    if (IsWow64)
//        heapCountersLength = sizeof(HEAP_COUNTERS32);
//    else
//        heapCountersLength = sizeof(HEAP_COUNTERS);
//
//    heapCounters = PhAllocate(heapCountersLength);
//    memset(heapCounters, 0, heapCountersLength);
//
//    if (WindowsVersion >= WINDOWS_10_20H1)
//    {
//        status = NtReadVirtualMemory(
//            ProcessHandle,
//            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x1F4 : 0x238),
//            heapCounters,
//            heapCountersLength,
//            NULL
//            );
//    }
//
//    if (NT_SUCCESS(status))
//    {
//        if (HeapCounters)
//            *HeapCounters = heapCounters;
//        return status;
//    }
//
//    PhFree(heapCounters);
//    return status;
//}
//
//NTSTATUS PhGetProcessSegmentHeapCounters(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID HeapAddress,
//    _In_ ULONG IsWow64,
//    _Out_ PVOID *HeapCounters
//    )
//{
//    NTSTATUS status = STATUS_UNSUCCESSFUL;
//    PVOID heapCounters;
//    ULONG heapCountersLength;
//
//    if (IsWow64)
//        heapCountersLength = sizeof(HEAP_RUNTIME_MEMORY_STATS32);
//    else
//        heapCountersLength = sizeof(HEAP_RUNTIME_MEMORY_STATS);
//
//    heapCounters = PhAllocate(heapCountersLength);
//    memset(heapCounters, 0, heapCountersLength);
//
//    if (WindowsVersion >= WINDOWS_10_20H1)
//    {
//        status = NtReadVirtualMemory(
//            ProcessHandle,
//            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x80 : 0x80),
//            heapCounters,
//            heapCountersLength,
//            NULL
//            );
//    }
//
//    if (NT_SUCCESS(status))
//    {
//        if (HeapCounters)
//            *HeapCounters = heapCounters;
//        return status;
//    }
//
//    PhFree(heapCounters);
//    return status;
//}
//
//NTSTATUS PhGetProcessHeaps(
//    _In_ HANDLE ProcessHandle
//    )
//{
//    PROCESS_BASIC_INFORMATION basicInfo;
//    ULONG numberOfHeaps;
//    PVOID processHeapsPtr;
//    PVOID* processHeaps;
//#ifdef _WIN64
//    PVOID peb32;
//    ULONG processHeapsPtr32;
//    ULONG* processHeaps32;
//#endif
//
//    if (NT_SUCCESS(PhGetProcessBasicInformation(ProcessHandle, &basicInfo)) && basicInfo.PebBaseAddress != 0)
//    {
//        if (NT_SUCCESS(NtReadVirtualMemory(
//            ProcessHandle,
//            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, UFIELD_OFFSET(PEB, NumberOfHeaps)),
//            &numberOfHeaps,
//            sizeof(ULONG),
//            NULL
//            )) && numberOfHeaps < 1000)
//        {
//            processHeaps = PhAllocateZero(numberOfHeaps * sizeof(PVOID));
//
//            if (
//                NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, PTR_ADD_OFFSET(basicInfo.PebBaseAddress, UFIELD_OFFSET(PEB, ProcessHeaps)), &processHeapsPtr, sizeof(PVOID), NULL)) &&
//                NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, processHeapsPtr, processHeaps, numberOfHeaps * sizeof(PVOID), NULL))
//                )
//            {
//                for (ULONG i = 0; i < numberOfHeaps; i++)
//                {
//                    ULONG signature = ULONG_MAX;
//
//                    if (!NT_SUCCESS(PhGetProcessHeapSignature(ProcessHandle, processHeaps[i], FALSE, &signature)))
//                        continue;
//
//                    if (signature == RTL_HEAP_SIGNATURE)
//                    {
//                        PHEAP_COUNTERS counters;
//
//                        if (NT_SUCCESS(PhGetProcessHeapCounters(ProcessHandle, processHeaps[i], FALSE, &counters)))
//                        {
//                            dprintf(
//                                "NT-%lu - 0x%I64x - commited: %lu - allocated: %lu - count: %lu\n",
//                                i + 1,
//                                processHeaps[i],
//                                counters->TotalMemoryCommitted,
//                                counters->LastPolledSize,
//                                counters->TotalSegments
//                                );
//
//                            PhFree(counters);
//                        }
//                    }
//                    else if (signature == RTL_HEAP_SEGMENT_SIGNATURE)
//                    {
//                        dprintf("Segment Heap");
//                    }
//                }
//            }
//
//            PhFree(processHeaps);
//        }
//    }
//#ifdef _WIN64
//
//    if (NT_SUCCESS(PhGetProcessPeb32(ProcessHandle, &peb32)) && peb32 != 0)
//    {
//        if (NT_SUCCESS(NtReadVirtualMemory(
//            ProcessHandle,
//            PTR_ADD_OFFSET(peb32, UFIELD_OFFSET(PEB32, NumberOfHeaps)),
//            &numberOfHeaps,
//            sizeof(ULONG),
//            NULL
//            )) && numberOfHeaps < 1000)
//        {
//            processHeaps32 = PhAllocateZero(numberOfHeaps * sizeof(ULONG));
//
//            if (
//                NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, PTR_ADD_OFFSET(peb32, UFIELD_OFFSET(PEB32, ProcessHeaps)), &processHeapsPtr32, sizeof(ULONG), NULL)) &&
//                NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, UlongToPtr(processHeapsPtr32), processHeaps32, numberOfHeaps * sizeof(ULONG), NULL))
//                )
//            {
//                for (ULONG i = 0; i < numberOfHeaps; i++)
//                {
//                    ULONG signature = ULONG_MAX;
//
//                    if (!NT_SUCCESS(PhGetProcessHeapSignature(ProcessHandle, UlongToPtr(processHeaps32[i]), TRUE, &signature)))
//                        continue;
//
//                    if (signature == RTL_HEAP_SIGNATURE)
//                    {
//                        PHEAP_COUNTERS32 counters;
//
//                        if (NT_SUCCESS(PhGetProcessHeapCounters(ProcessHandle, UlongToPtr(processHeaps32[i]), TRUE, &counters)))
//                        {
//                            PhFree(counters);
//                        }
//                    }
//                    else if (signature == RTL_HEAP_SEGMENT_SIGNATURE)
//                    {
//                        dprintf("Segment Heap\n");
//                    }
//                }
//            }
//
//            PhFree(processHeaps32);
//        }
//    }
//#endif
//
//    return STATUS_SUCCESS;
//}
//
//BOOLEAN PhpDestroyHeap(
//    _In_ HWND hWnd,
//    _In_ HANDLE ProcessId,
//    _In_ PVOID HeapHandle
//    )
//{
//    NTSTATUS status;
//    BOOLEAN cont = FALSE;
//    HANDLE processHandle;
//    HANDLE threadHandle;
//
//    if (PhGetIntegerSetting(L"EnableWarnings"))
//    {
//        cont = PhShowConfirmMessage(
//            hWnd,
//            L"destroy",
//            L"the selected heap",
//            L"Destroying heaps may cause the process to crash.",
//            FALSE
//            );
//    }
//    else
//    {
//        cont = TRUE;
//    }
//
//    if (!cont)
//        return FALSE;
//
//    if (NT_SUCCESS(status = PhOpenProcess(
//        &processHandle,
//        PROCESS_CREATE_THREAD,
//        ProcessId
//        )))
//    {
//        if (WindowsVersion >= WINDOWS_VISTA)
//        {
//            status = RtlCreateUserThread(
//                processHandle,
//                NULL,
//                FALSE,
//                0,
//                0,
//                0,
//                PhGetModuleProcAddress(L"ntdll.dll", "RtlDestroyHeap"),
//                HeapHandle,
//                &threadHandle,
//                NULL
//                );
//        }
//        else
//        {
//            if (!(threadHandle = CreateRemoteThread(
//                processHandle,
//                NULL,
//                0,
//                PhGetModuleProcAddress(L"ntdll.dll", "RtlDestroyHeap"),
//                HeapHandle,
//                0,
//                NULL
//                )))
//            {
//                status = PhGetLastWin32ErrorAsNtStatus();
//            }
//        }
//
//        if (NT_SUCCESS(status))
//            NtClose(threadHandle);
//
//        NtClose(processHandle);
//    }
//
//    if (!NT_SUCCESS(status))
//    {
//        PhShowStatus(hWnd, L"Unable to destroy the heap", status, 0);
//        return FALSE;
//    }
//
//    return TRUE;
//}
//
//#include <tlhelp32.h>
//
//BOOLEAN PhpEnumerateHeapsWin32(
//    _In_ PPROCESS_HEAPS_CONTEXT Context
//    )
//{
//    HANDLE snapshotHandle;
//    HEAPLIST32 heapList32;
//
//    snapshotHandle = CreateToolhelp32Snapshot(
//        TH32CS_SNAPHEAPLIST,
//        Context->ProcessItem->ProcessId
//        );
//
//    if (snapshotHandle == INVALID_HANDLE_VALUE)
//        return FALSE;
//
//    memset(&heapList32, 0, sizeof(HEAPLIST32));
//    heapList32.dwSize = sizeof(HEAPLIST32);
//
//    if (Heap32ListFirst(snapshotHandle, &heapList32))
//    {
//        do
//        {
//            HEAPENTRY32 entry;
//
//            memset(&entry, 0, sizeof(HEAPENTRY32));
//            entry.dwSize = sizeof(HEAPENTRY32);
//
//            dprintf("\nHeap ID: %lu\n", heapList32.th32HeapID);
//
//            if (Heap32First(&entry, Context->ProcessItem->ProcessId, heapList32.th32HeapID))
//            {
//                do
//                {
//                    dprintf("Block size: %lu\n", entry.dwBlockSize);
//
//                    memset(&entry, 0, sizeof(HEAPENTRY32));
//                    entry.dwSize = sizeof(HEAPENTRY32);
//                } while (Heap32Next(&entry));
//            }
//
//            memset(&heapList32, 0, sizeof(HEAPLIST32));
//            heapList32.dwSize = sizeof(HEAPLIST32);
//        } while (Heap32ListNext(snapshotHandle, &heapList32));
//    }
//
//    NtClose(snapshotHandle);
//}

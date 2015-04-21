/*
 * Process Hacker Extended Tools -
 *   unloaded DLLs display
 *
 * Copyright (C) 2010-2011 wj32
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

#include "exttools.h"
#include "resource.h"

typedef struct _UNLOADED_DLLS_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    HWND ListViewHandle;
    PVOID CapturedEventTrace;
} UNLOADED_DLLS_CONTEXT, *PUNLOADED_DLLS_CONTEXT;

INT_PTR CALLBACK EtpUnloadedDllsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EtShowUnloadedDllsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    UNLOADED_DLLS_CONTEXT context;

    context.ProcessItem = ProcessItem;
    context.CapturedEventTrace = NULL;

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_UNLOADEDDLLS),
        ParentWindowHandle,
        EtpUnloadedDllsDlgProc,
        (LPARAM)&context
        );

    if (context.CapturedEventTrace)
        PhFree(context.CapturedEventTrace);
}

BOOLEAN EtpRefreshUnloadedDlls(
    _In_ HWND hwndDlg,
    _In_ PUNLOADED_DLLS_CONTEXT Context
    )
{
    NTSTATUS status;
    PULONG elementSize;
    PULONG elementCount;
    PVOID eventTrace;
    HANDLE processHandle = NULL;
    ULONG eventTraceSize;
    ULONG capturedElementSize;
    ULONG capturedElementCount;
    PVOID capturedEventTracePointer;
    PVOID capturedEventTrace = NULL;
    ULONG i;
    PVOID currentEvent;
    HWND lvHandle;

    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
    ListView_DeleteAllItems(lvHandle);

    RtlGetUnloadEventTraceEx(&elementSize, &elementCount, &eventTrace);

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_VM_READ, Context->ProcessItem->ProcessId)))
        goto CleanupExit;

    // We have the pointers for the unload event trace information.
    // Since ntdll is loaded at the same base address across all processes,
    // we can read the information in.

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        elementSize,
        &capturedElementSize,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        elementCount,
        &capturedElementCount,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        eventTrace,
        &capturedEventTracePointer,
        sizeof(PVOID),
        NULL
        )))
        goto CleanupExit;

    if (!capturedEventTracePointer)
        goto CleanupExit; // no events

    if (capturedElementCount > 0x4000)
        capturedElementCount = 0x4000;

    eventTraceSize = capturedElementSize * capturedElementCount;

    capturedEventTrace = PhAllocateSafe(eventTraceSize);

    if (!capturedEventTrace)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        capturedEventTracePointer,
        capturedEventTrace,
        eventTraceSize,
        NULL
        )))
        goto CleanupExit;

    currentEvent = capturedEventTrace;

    ExtendedListView_SetRedraw(lvHandle, FALSE);

    for (i = 0; i < capturedElementCount; i++)
    {
        PRTL_UNLOAD_EVENT_TRACE rtlEvent = currentEvent;
        INT lvItemIndex;
        WCHAR buffer[128];
        PPH_STRING string;
        LARGE_INTEGER time;
        SYSTEMTIME systemTime;

        if (!rtlEvent->BaseAddress)
            break;

        PhPrintUInt32(buffer, rtlEvent->Sequence);
        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, buffer, rtlEvent);

        // Name
        if (PhCopyStringZ(rtlEvent->ImageName, sizeof(rtlEvent->ImageName) / sizeof(WCHAR),
            buffer, sizeof(buffer) / sizeof(WCHAR), NULL))
        {
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, buffer);
        }

        // Base Address
        PhPrintPointer(buffer, rtlEvent->BaseAddress);
        PhSetListViewSubItem(lvHandle, lvItemIndex, 2, buffer);

        // Size
        string = PhFormatSize(rtlEvent->SizeOfImage, -1);
        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, string->Buffer);
        PhDereferenceObject(string);

        // Time Stamp
        RtlSecondsSince1970ToTime(rtlEvent->TimeDateStamp, &time);
        PhLargeIntegerToLocalSystemTime(&systemTime, &time);
        string = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(lvHandle, lvItemIndex, 4, string->Buffer);
        PhDereferenceObject(string);

        // Checksum
        PhPrintPointer(buffer, UlongToPtr(rtlEvent->CheckSum));
        PhSetListViewSubItem(lvHandle, lvItemIndex, 5, buffer);

        currentEvent = PTR_ADD_OFFSET(currentEvent, capturedElementSize);
    }

    ExtendedListView_SortItems(lvHandle);
    ExtendedListView_SetRedraw(lvHandle, TRUE);

    if (Context->CapturedEventTrace)
        PhFree(Context->CapturedEventTrace);

    Context->CapturedEventTrace = capturedEventTrace;

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        return TRUE;
    }
    else
    {
        PhShowStatus(hwndDlg, L"Unable to retrieve unload event trace information", status, 0);
        return FALSE;
    }
}

static INT NTAPI EtpNumberCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
    PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

    return uintcmp(item1->Sequence, item2->Sequence);
}

static INT NTAPI EtpBaseAddressCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
    PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->BaseAddress, (ULONG_PTR)item2->BaseAddress);
}

static INT NTAPI EtpSizeCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
    PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

    return uintptrcmp(item1->SizeOfImage, item2->SizeOfImage);
}

static INT NTAPI EtpTimeStampCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
    PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

    return uintcmp(item1->TimeDateStamp, item2->TimeDateStamp);
}

static INT NTAPI EtpCheckSumCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
    PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

    return uintcmp(item1->CheckSum, item2->CheckSum);
}

INT_PTR CALLBACK EtpUnloadedDllsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PUNLOADED_DLLS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PUNLOADED_DLLS_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PUNLOADED_DLLS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"No.");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Base Address");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 60, L"Size");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Time Stamp");
            PhAddListViewColumn(lvHandle, 5, 5, 5, LVCFMT_LEFT, 60, L"Checksum");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetCompareFunction(lvHandle, 0, EtpNumberCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 2, EtpBaseAddressCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 3, EtpSizeCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 4, EtpTimeStampCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 5, EtpCheckSumCompareFunction);

            if (!EtpRefreshUnloadedDlls(hwndDlg, context))
            {
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_REFRESH:
                EtpRefreshUnloadedDlls(hwndDlg, context);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    }

    return FALSE;
}

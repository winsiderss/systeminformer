/*
 * Process Hacker - 
 *   process heaps dialog
 * 
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <windowsx.h>
#include <emenu.h>

typedef struct _PROCESS_HEAPS_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PRTL_PROCESS_HEAPS ProcessHeaps;
    PVOID ProcessHeap;

    HWND ListViewHandle;
} PROCESS_HEAPS_CONTEXT, *PPROCESS_HEAPS_CONTEXT;

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessHeapsDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    PROCESS_HEAPS_CONTEXT context;
    PRTL_DEBUG_INFORMATION debugBuffer;
    HANDLE processHandle;

    context.ProcessItem = ProcessItem;
    context.ProcessHeap = NULL;

    debugBuffer = RtlCreateQueryDebugBuffer(0, FALSE);

    if (!debugBuffer)
        return;

    status = RtlQueryProcessDebugInformation(
        ProcessItem->ProcessId,
        RTL_QUERY_PROCESS_HEAP_SUMMARY | RTL_QUERY_PROCESS_HEAP_ENTRIES,
        debugBuffer
        );

    if (NT_SUCCESS(status))
    {
        context.ProcessHeaps = debugBuffer->Heaps;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_VM_READ,
            ProcessItem->ProcessId
            )))
        {
            PhGetProcessDefaultHeap(processHandle, &context.ProcessHeap);
            NtClose(processHandle);
        }

        DialogBoxParam(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_HEAPS),
            ParentWindowHandle,
            PhpProcessHeapsDlgProc,
            (LPARAM)&context
            );
    }
    else
    {
        PhShowStatus(ParentWindowHandle, L"Unable to query heap information", status, 0);
    }

    RtlDestroyQueryDebugBuffer(debugBuffer);
}

static INT NTAPI PhpHeapAddressCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PRTL_HEAP_INFORMATION heapInfo1 = Item1;
    PRTL_HEAP_INFORMATION heapInfo2 = Item2;

    return uintptrcmp((ULONG_PTR)heapInfo1->BaseAddress, (ULONG_PTR)heapInfo2->BaseAddress);
}

static INT NTAPI PhpHeapUsedCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PRTL_HEAP_INFORMATION heapInfo1 = Item1;
    PRTL_HEAP_INFORMATION heapInfo2 = Item2;

    return uintptrcmp(heapInfo1->BytesAllocated, heapInfo2->BytesAllocated);
}

static INT NTAPI PhpHeapCommittedCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PRTL_HEAP_INFORMATION heapInfo1 = Item1;
    PRTL_HEAP_INFORMATION heapInfo2 = Item2;

    return uintptrcmp(heapInfo1->BytesCommitted, heapInfo2->BytesCommitted);
}

static INT NTAPI PhpHeapEntriesCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PRTL_HEAP_INFORMATION heapInfo1 = Item1;
    PRTL_HEAP_INFORMATION heapInfo2 = Item2;

    return uintcmp(heapInfo1->NumberOfEntries, heapInfo2->NumberOfEntries);
}

static HFONT NTAPI PhpHeapFontFunction(
    __in INT Index,
    __in PVOID Param,
    __in_opt PVOID Context
    )
{
    PRTL_HEAP_INFORMATION heapInfo = Param;
    PPROCESS_HEAPS_CONTEXT context = Context;

    if (heapInfo->BaseAddress == context->ProcessHeap)
        return PhBoldMessageFont;

    return NULL;
}

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPROCESS_HEAPS_CONTEXT context = NULL;

    if (uMsg != WM_INITDIALOG)
    {
        context = (PPROCESS_HEAPS_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
    }
    else
    {
        context = (PPROCESS_HEAPS_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            ULONG i;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Address");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 120, L"Used");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 120, L"Committed");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Entries");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetContext(lvHandle, context);
            ExtendedListView_SetCompareFunction(lvHandle, 0, PhpHeapAddressCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 1, PhpHeapUsedCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 2, PhpHeapCommittedCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 3, PhpHeapEntriesCompareFunction);
            ExtendedListView_SetItemFontFunction(lvHandle, PhpHeapFontFunction);

            for (i = 0; i < context->ProcessHeaps->NumberOfHeaps; i++)
            {
                PRTL_HEAP_INFORMATION heapInfo = &context->ProcessHeaps->Heaps[i];
                WCHAR addressString[PH_PTR_STR_LEN_1];
                INT lvItemIndex;
                PPH_STRING usedString;
                PPH_STRING committedString;
                PPH_STRING numberOfEntriesString;

                PhPrintPointer(addressString, heapInfo->BaseAddress);
                lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, addressString, heapInfo);

                usedString = PhFormatSize(heapInfo->BytesAllocated, -1);
                committedString = PhFormatSize(heapInfo->BytesCommitted, -1);
                numberOfEntriesString = PhFormatUInt64(heapInfo->NumberOfEntries, TRUE);

                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, usedString->Buffer); 
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, committedString->Buffer);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, numberOfEntriesString->Buffer);

                PhDereferenceObject(usedString);
                PhDereferenceObject(committedString);
                PhDereferenceObject(numberOfEntriesString);
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
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
            case IDC_SIZESINBYTES:
                {
                    BOOLEAN sizesInBytes = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SIZESINBYTES)) == BST_CHECKED;
                    INT index = -1;

                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

                    while ((index = ListView_GetNextItem(context->ListViewHandle, index, LVNI_ALL)) != -1)
                    {
                        PRTL_HEAP_INFORMATION heapInfo;
                        PPH_STRING usedString;
                        PPH_STRING committedString;

                        if (PhGetListViewItemParam(context->ListViewHandle, index, &heapInfo))
                        {
                            usedString = PhFormatSize(heapInfo->BytesAllocated, sizesInBytes ? 0 : -1);
                            committedString = PhFormatSize(heapInfo->BytesCommitted, sizesInBytes ? 0 : -1);

                            PhSetListViewSubItem(context->ListViewHandle, index, 1, usedString->Buffer); 
                            PhSetListViewSubItem(context->ListViewHandle, index, 2, committedString->Buffer);

                            PhDereferenceObject(usedString);
                            PhDereferenceObject(committedString);
                        }
                    }

                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            switch (header->code)
            {
            case NM_RCLICK:
                {
                    LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                    PRTL_HEAP_INFORMATION heapInfo;
                    PPH_EMENU menu;
                    INT selectedCount;
                    POINT cursorPos;
                    PPH_EMENU_ITEM menuItem;

                    selectedCount = ListView_GetSelectedCount(context->ListViewHandle);
                    heapInfo = PhGetSelectedListViewItemParam(context->ListViewHandle);

                    if (selectedCount != 0)
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(selectedCount != 1 ? PH_EMENU_DISABLED : 0, 1, L"Destroy", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Copy\bCtrl+C", NULL, NULL), -1);

                        GetCursorPos(&cursorPos);
                        menuItem = PhShowEMenu(menu, context->ListViewHandle, PH_EMENU_SHOW_LEFTRIGHT | PH_EMENU_SHOW_NONOTIFY,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP, cursorPos.x, cursorPos.y);

                        if (menuItem)
                        {
                            switch (menuItem->Id)
                            {
                            case 1:
                                if (PhUiDestroyHeap(hwndDlg, context->ProcessItem->ProcessId, heapInfo->BaseAddress))
                                    ListView_DeleteItem(context->ListViewHandle, PhFindListViewItemByParam(context->ListViewHandle, -1, heapInfo));
                                break;
                            case 2:
                                PhCopyListView(context->ListViewHandle);
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

    REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);

    return FALSE;
}

NTSTATUS PhGetProcessDefaultHeap(
    __in HANDLE ProcessHandle,
    __out PPVOID Heap
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;

    if (!NT_SUCCESS(status = PhGetProcessBasicInformation(
        ProcessHandle,
        &basicInfo
        )))
        return status;

    return status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessHeap)),
        Heap,
        sizeof(PVOID),
        NULL
        );
}

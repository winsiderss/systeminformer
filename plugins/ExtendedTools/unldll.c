/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2016-2023
 *
 */

#include "exttools.h"

typedef struct _UNLOADED_DLLS_CONTEXT
{
    BOOLEAN IsWow64;
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PVOID CapturedEventTrace;
    HANDLE ProcessId;
    HANDLE QueryHandle;
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
    NTSTATUS status;
    PUNLOADED_DLLS_CONTEXT context;

    context = PhAllocateZero(sizeof(UNLOADED_DLLS_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;
    context->ProcessId = ProcessItem->ProcessId;
    context->IsWow64 = !!ProcessItem->IsWow64;

    status = PhOpenProcess(
        &context->QueryHandle,
        PROCESS_QUERY_INFORMATION,
        context->ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            &context->QueryHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            context->ProcessId
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process.", status, 0);
        PhFree(context);
        return;
    }

    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_UNLOADEDDLLS),
        !!PhGetIntegerSetting(L"ForceNoParent") ? NULL : ParentWindowHandle,
        EtpUnloadedDllsDlgProc,
        context
        );
}

NTSTATUS EtpRefreshUnloadedDlls(
    _In_ HWND hwndDlg,
    _In_ PUNLOADED_DLLS_CONTEXT Context
    )
{
    NTSTATUS status;
    ULONG capturedElementSize;
    ULONG capturedElementCount;
    PVOID capturedEventTrace = NULL;
    PVOID currentEvent;
    ULONG i;

#ifdef _WIN64
    if (!Context->QueryHandle)
        return STATUS_FAIL_CHECK;

    if (Context->IsWow64)
    {
        PPH_STRING eventTraceString;
        ULONG capturedEventTraceLength;

        if (!PhUiConnectToPhSvcEx(hwndDlg, Wow64PhSvcMode, FALSE))
            return STATUS_FAIL_CHECK;

        if (!NT_SUCCESS(status = CallGetProcessUnloadedDlls(Context->ProcessId, &eventTraceString)))
        {
            PhUiDisconnectFromPhSvc();
            return status;
        }

        capturedEventTraceLength = sizeof(RTL_UNLOAD_EVENT_TRACE32) * RTL_UNLOAD_EVENT_TRACE_NUMBER;
        capturedEventTrace = PhAllocateZero(capturedEventTraceLength);

        if (!PhHexStringToBufferEx(
            &eventTraceString->sr,
            capturedEventTraceLength,
            capturedEventTrace
            ))
        {
            PhUiDisconnectFromPhSvc();

            PhFree(capturedEventTrace);

            return STATUS_FAIL_CHECK;
        }

        PhUiDisconnectFromPhSvc();

        ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
        ListView_DeleteAllItems(Context->ListViewHandle);

        for (i = 0; i < RTL_UNLOAD_EVENT_TRACE_NUMBER; i++)
        {
            PRTL_UNLOAD_EVENT_TRACE32 rtlEvent = PTR_ADD_OFFSET(capturedEventTrace, sizeof(RTL_UNLOAD_EVENT_TRACE32) * i);
            INT lvItemIndex;
            WCHAR buffer[128];
            PPH_STRING string;
            LARGE_INTEGER time;
            SYSTEMTIME systemTime;

            if (!rtlEvent->BaseAddress)
                break;

            PhPrintUInt32(buffer, rtlEvent->Sequence);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, buffer, rtlEvent);

            // Name
            if (PhCopyStringZ(rtlEvent->ImageName, RTL_NUMBER_OF(rtlEvent->ImageName), buffer, RTL_NUMBER_OF(buffer), NULL))
            {
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, buffer);
            }

            // Base Address
            PhPrintPointer(buffer, (PVOID)(ULONG_PTR)rtlEvent->BaseAddress);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, buffer);

            // Size
            string = PhFormatSize(rtlEvent->SizeOfImage, ULONG_MAX);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, string->Buffer);
            PhDereferenceObject(string);

            // Time Stamp
            RtlSecondsSince1970ToTime(rtlEvent->TimeDateStamp, &time);
            PhLargeIntegerToLocalSystemTime(&systemTime, &time);
            string = PhFormatDateTime(&systemTime);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, string->Buffer);
            PhDereferenceObject(string);

            // Checksum
            PhPrintPointer(buffer, UlongToPtr(rtlEvent->CheckSum));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, buffer);
        }

        ExtendedListView_SortItems(Context->ListViewHandle);
        ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

        PhDereferenceObject(eventTraceString);
    }
    else
    {
#endif
        status = PhGetProcessUnloadedDlls(
            Context->ProcessId,
            &capturedEventTrace,
            &capturedElementSize,
            &capturedElementCount
            );

        if (!NT_SUCCESS(status))
            return status;

        currentEvent = capturedEventTrace;

        ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
        ListView_DeleteAllItems(Context->ListViewHandle);

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
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, buffer, rtlEvent);

            // Name
            if (PhCopyStringZ(rtlEvent->ImageName, RTL_NUMBER_OF(rtlEvent->ImageName), buffer, RTL_NUMBER_OF(buffer), NULL))
            {
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, buffer);
            }

            // Base Address
            PhPrintPointer(buffer, rtlEvent->BaseAddress);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, buffer);

            // Size
            string = PhFormatSize(rtlEvent->SizeOfImage, ULONG_MAX);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, string->Buffer);
            PhDereferenceObject(string);

            // Time Stamp
            RtlSecondsSince1970ToTime(rtlEvent->TimeDateStamp, &time);
            PhLargeIntegerToLocalSystemTime(&systemTime, &time);
            string = PhFormatDateTime(&systemTime);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, string->Buffer);
            PhDereferenceObject(string);

            // Checksum
            PhPrintPointer(buffer, UlongToPtr(rtlEvent->CheckSum));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, buffer);

            currentEvent = PTR_ADD_OFFSET(currentEvent, capturedElementSize);
        }

        ExtendedListView_SortItems(Context->ListViewHandle);
        ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

#ifdef _WIN64
    }
#endif

    if (Context->CapturedEventTrace)
        PhFree(Context->CapturedEventTrace);

    Context->CapturedEventTrace = capturedEventTrace;

    return status;
}

static INT NTAPI EtpNumberCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
#ifdef _WIN64
    PUNLOADED_DLLS_CONTEXT context = Context;

    if (context && context->IsWow64)
    {
        PRTL_UNLOAD_EVENT_TRACE32 item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE32 item2 = Item2;

        return uintcmp(item1->Sequence, item2->Sequence);
    }
    else
#endif
    {
        PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

        return uintcmp(item1->Sequence, item2->Sequence);
    }
}

static INT NTAPI EtpBaseAddressCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
#ifdef _WIN64
    PUNLOADED_DLLS_CONTEXT context = Context;

    if (context && context->IsWow64)
    {
        PRTL_UNLOAD_EVENT_TRACE32 item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE32 item2 = Item2;

        return uintptrcmp((ULONG_PTR)item1->BaseAddress, (ULONG_PTR)item2->BaseAddress);
    }
    else
#endif
    {
        PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

        return uintptrcmp((ULONG_PTR)item1->BaseAddress, (ULONG_PTR)item2->BaseAddress);
    }
}

static INT NTAPI EtpSizeCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
#ifdef _WIN64
    PUNLOADED_DLLS_CONTEXT context = Context;

    if (context && context->IsWow64)
    {
        PRTL_UNLOAD_EVENT_TRACE32 item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE32 item2 = Item2;

        return uintptrcmp(item1->SizeOfImage, item2->SizeOfImage);
    }
    else
#endif
    {
        PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

        return uintptrcmp(item1->SizeOfImage, item2->SizeOfImage);
    }
}

static INT NTAPI EtpTimeStampCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
#ifdef _WIN64
    PUNLOADED_DLLS_CONTEXT context = Context;

    if (context && context->IsWow64)
    {
        PRTL_UNLOAD_EVENT_TRACE32 item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE32 item2 = Item2;

        return uintcmp(item1->TimeDateStamp, item2->TimeDateStamp);
    }
    else
#endif
    {
        PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

        return uintcmp(item1->TimeDateStamp, item2->TimeDateStamp);
    }
}

static INT NTAPI EtpCheckSumCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
#ifdef _WIN64
    PUNLOADED_DLLS_CONTEXT context = Context;

    if (context && context->IsWow64)
    {
        PRTL_UNLOAD_EVENT_TRACE32 item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE32 item2 = Item2;

        return uintcmp(item1->CheckSum, item2->CheckSum);
    }
    else
#endif
    {
        PRTL_UNLOAD_EVENT_TRACE item1 = Item1;
        PRTL_UNLOAD_EVENT_TRACE item2 = Item2;

        return uintcmp(item1->CheckSum, item2->CheckSum);
    }
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

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            HWND lvHandle;

            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
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
            ExtendedListView_SetContext(lvHandle, context);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_UNLOADED_COLUMNS, lvHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, lvHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhGetIntegerPairSetting(SETTING_NAME_UNLOADED_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_UNLOADED_WINDOW_POSITION, SETTING_NAME_UNLOADED_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (!NT_SUCCESS(status = EtpRefreshUnloadedDlls(hwndDlg, context)))
            {
                PhShowStatus(context->ParentWindowHandle, L"Unable to retrieve unload event trace information.", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_UNLOADED_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_UNLOADED_WINDOW_POSITION, SETTING_NAME_UNLOADED_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->CapturedEventTrace)
                PhFree(context->CapturedEventTrace);

            if (context->QueryHandle)
                NtClose(context->QueryHandle);

            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
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
                            case PHAPP_IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
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

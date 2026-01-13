/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2014-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <settings.h>
#include <phsettings.h>
#include <mainwnd.h>
#include <emenu.h>

#define WM_PH_LOG_UPDATED (WM_APP + 300)

INT_PTR CALLBACK PhpLogDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

HWND PhLogWindowHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;
static HWND AutoScrollHandle;
static HWND ListViewHandle;
static PPH_LISTVIEW_CONTEXT ListViewContext;
static ULONG ListViewCount;
static PH_CALLBACK_REGISTRATION LoggedRegistration;
static BOOLEAN ListViewStateInitializing = FALSE;
static BOOLEAN ListViewAutoScroll = FALSE;

VOID PhShowLogDialog(
    VOID
    )
{
    if (!PhLogWindowHandle)
    {
        PhLogWindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_LOG),
            NULL,
            PhpLogDlgProc,
            NULL
            );
        PhRegisterDialog(PhLogWindowHandle);
        ShowWindow(PhLogWindowHandle, SW_SHOW);
    }

    if (IsMinimized(PhLogWindowHandle))
        ShowWindow(PhLogWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhLogWindowHandle);
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID NTAPI LoggedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (PhLogWindowHandle)
    {
        PostMessage(PhLogWindowHandle, WM_PH_LOG_UPDATED, 0, 0);
    }
}

static VOID PhpUpdateLogList(
    VOID
    )
{
    ListViewCount = PhLogBuffer.Count;
    PhListView_SetItemCount(ListViewContext, ListViewCount, LVSICF_NOSCROLL);

    if (ListViewCount >= 2 && ReadBooleanAcquire(&ListViewAutoScroll))
    {
        BOOLEAN itemVisible;

        if (PhListView_IsItemVisible(ListViewContext, ListViewCount - 1, &itemVisible) && !itemVisible)
        {
            PhListView_EnsureItemVisible(ListViewContext, ListViewCount - 1, FALSE);
        }
    }
}

static PPH_STRING PhpGetStringForSelectedLogEntries(
    _In_ BOOLEAN All
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    if (ListViewCount == 0)
        return PhReferenceEmptyString();

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    i = ListViewCount - 1;

    while (TRUE)
    {
        PPH_LOG_ENTRY entry;
        SYSTEMTIME systemTime;
        PCPH_STRINGREF string;
        PPH_STRING temp;
        ULONG itemState;

        if (!All)
        {
            // The list view displays the items in reverse order...
            if (!PhListView_GetItemState(ListViewContext, ListViewCount - i - 1, LVIS_SELECTED, &itemState) && FlagOn(itemState, LVIS_SELECTED))
            {
                goto ContinueLoop;
            }
        }

        entry = PhGetItemCircularBuffer_PVOID(&PhLogBuffer, i);

        if (!entry)
            goto ContinueLoop;

        PhLargeIntegerToLocalSystemTime(&systemTime, &entry->Time);
        temp = PhFormatDateTime(&systemTime);
        PhAppendStringBuilder(&stringBuilder, &temp->sr);
        PhDereferenceObject(temp);
        PhAppendStringBuilder2(&stringBuilder, L": ");

        string = PhFormatLogType(entry);
        PhAppendStringBuilder(&stringBuilder, string);
        PhAppendStringBuilder2(&stringBuilder, L" ");
        temp = PhFormatLogEntry(entry);
        PhAppendStringBuilder(&stringBuilder, &temp->sr);
        PhDereferenceObject(temp);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");

ContinueLoop:

        if (i == 0)
            break;

        i--;
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

INT_PTR CALLBACK PhpLogDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);

            AutoScrollHandle = GetDlgItem(hwndDlg, IDC_AUTOSCROLL);
            ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            ListViewContext = PhListView_Initialize(ListViewHandle);

            PhSetListViewStyle(ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(ListViewHandle, L"explorer");
            PhSetExtendedListView(ListViewHandle);
            PhListView_AddColumn(ListViewContext, 0, 0, 0, LVCFMT_LEFT, 140, L"Time");
            PhListView_AddColumn(ListViewContext, 1, 1, 1, LVCFMT_LEFT, 140, L"Type");
            PhListView_AddColumn(ListViewContext, 2, 2, 2, LVCFMT_LEFT, 260, L"Message");
            PhLoadListViewColumnsFromSetting(SETTING_LOG_LIST_VIEW_COLUMNS, ListViewHandle);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, AutoScrollHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_CLEAR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 290;
            MinimumSize.bottom = 150;
            MapDialogRect(hwndDlg, &MinimumSize);

            if (PhValidWindowPlacementFromSetting(SETTING_LOG_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_LOG_WINDOW_POSITION, SETTING_LOG_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            ListViewStateInitializing = TRUE;
            WriteBooleanRelease(&ListViewAutoScroll, TRUE);
            Button_SetCheck(AutoScrollHandle, BST_CHECKED);
            ListViewStateInitializing = FALSE;

            PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackLoggedEvent), LoggedCallback, NULL, &LoggedRegistration);
            PhpUpdateLogList();

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_LOG_LIST_VIEW_COLUMNS, ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_LOG_WINDOW_POSITION, SETTING_LOG_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackLoggedEvent), &LoggedRegistration);
            PhUnregisterDialog(PhLogWindowHandle);
            PhLogWindowHandle = NULL;

            PhListView_Destroy(ListViewContext);
            ListViewContext = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_CLEAR:
                {
                    PhClearLogEntries();
                    PhpUpdateLogList();
                }
                break;
            case IDC_COPY:
                {
                    PPH_STRING string;
                    ULONG selectedCount = 0;

                    if (!PhListView_GetSelectedCount(ListViewContext, &selectedCount))
                        break;

                    if (selectedCount == 0)
                    {
                        // User didn't select anything, so copy all items.
                        string = PhpGetStringForSelectedLogEntries(TRUE);
                        PhListView_SetStateAllItems(ListViewContext, LVIS_SELECTED, LVIS_SELECTED);
                    }
                    else
                    {
                        string = PhpGetStringForSelectedLogEntries(FALSE);
                    }

                    PhSetClipboardString(hwndDlg, &string->sr);
                    PhDereferenceObject(string);
                }
                break;
            case IDC_SAVE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Text files (*.txt)", L"*.txt" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;

                    fileDialog = PhCreateSaveFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                    PhSetFileDialogFileName(fileDialog, L"System Informer Log.txt");

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        NTSTATUS status;
                        PPH_STRING fileName;
                        PPH_FILE_STREAM fileStream;
                        PPH_STRING string;

                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));

                        if (NT_SUCCESS(status = PhCreateFileStream(
                            &fileStream,
                            fileName->Buffer,
                            FILE_GENERIC_WRITE,
                            FILE_SHARE_READ,
                            FILE_OVERWRITE_IF,
                            0
                            )))
                        {
                            PhWriteStringAsUtf8FileStream(fileStream, (PPH_STRINGREF)&PhUnicodeByteOrderMark);
                            PhWritePhTextHeader(fileStream);

                            string = PhpGetStringForSelectedLogEntries(TRUE);
                            PhWriteStringAsUtf8FileStreamEx(fileStream, string->Buffer, string->Length);
                            PhDereferenceObject(string);

                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_AUTOSCROLL:
                {
                    WriteBooleanRelease(&ListViewAutoScroll, Button_GetCheck(AutoScrollHandle) == BST_CHECKED);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case LVN_GETDISPINFO:
                {
                    NMLVDISPINFO *dispInfo = (NMLVDISPINFO *)header;
                    PPH_LOG_ENTRY entry;

                    entry = PhGetItemCircularBuffer_PVOID(&PhLogBuffer, ListViewCount - dispInfo->item.iItem - 1);

                    if (dispInfo->item.iSubItem == 0)
                    {
                        if (FlagOn(dispInfo->item.mask, LVIF_TEXT))
                        {
                            SYSTEMTIME systemTime;
                            PPH_STRING dateTime;

                            PhLargeIntegerToLocalSystemTime(&systemTime, &entry->Time);
                            dateTime = PhFormatDateTime(&systemTime);
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, dateTime->Buffer, _TRUNCATE);
                            PhDereferenceObject(dateTime);
                        }
                    }
                    else if (dispInfo->item.iSubItem == 1)
                    {
                        if (FlagOn(dispInfo->item.mask, LVIF_TEXT))
                        {
                            PCPH_STRINGREF string;

                            string = PhFormatLogType(entry);
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, string->Buffer, _TRUNCATE);
                        }
                    }
                    else if (dispInfo->item.iSubItem == 2)
                    {
                        if (FlagOn(dispInfo->item.mask, LVIF_TEXT))
                        {
                            PPH_STRING string;

                            string = PhFormatLogEntry(entry);
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, string->Buffer, _TRUNCATE);
                            PhDereferenceObject(string);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&WindowLayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_PH_LOG_UPDATED:
        {
            PhpUpdateLogList();
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(ListViewHandle, &point);

                PhGetSelectedListViewItemParams(ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, ListViewHandle);

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
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(ListViewHandle);
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

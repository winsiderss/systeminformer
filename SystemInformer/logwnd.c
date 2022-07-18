/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2014-2022
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <phsettings.h>
#include <settings.h>
#include <mainwnd.h>

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
static HWND ListViewHandle;
static ULONG ListViewCount;
static PH_CALLBACK_REGISTRATION LoggedRegistration;

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

static VOID NTAPI LoggedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhLogWindowHandle, WM_PH_LOG_UPDATED, 0, 0);
}

static VOID PhpUpdateLogList(
    VOID
    )
{
    ListViewCount = PhLogBuffer.Count;
    ListView_SetItemCountEx(ListViewHandle, ListViewCount, LVSICF_NOSCROLL);

    if (ListViewCount >= 2 && Button_GetCheck(GetDlgItem(PhLogWindowHandle, IDC_AUTOSCROLL)) == BST_CHECKED)
    {
        if (ListView_IsItemVisible(ListViewHandle, ListViewCount - 2))
        {
            ListView_EnsureVisible(ListViewHandle, ListViewCount - 1, FALSE);
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
        PPH_STRING temp;

        if (!All)
        {
            // The list view displays the items in reverse order...
            if (!(ListView_GetItemState(ListViewHandle, ListViewCount - i - 1, LVIS_SELECTED) & LVIS_SELECTED))
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

            ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewHandle, L"explorer");
            PhAddListViewColumn(ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Time");
            PhAddListViewColumn(ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 260, L"Message");
            PhLoadListViewColumnsFromSetting(L"LogListViewColumns", ListViewHandle);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_AUTOSCROLL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_CLEAR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 290;
            MinimumSize.bottom = 150;
            MapDialogRect(hwndDlg, &MinimumSize);

            if (PhGetIntegerPairSetting(L"LogWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"LogWindowPosition", L"LogWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_AUTOSCROLL), BST_CHECKED);

            PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackLoggedEvent), LoggedCallback, NULL, &LoggedRegistration);
            PhpUpdateLogList();
            ListView_EnsureVisible(ListViewHandle, ListViewCount - 1, FALSE);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"LogListViewColumns", ListViewHandle);
            PhSaveWindowPlacementToSetting(L"LogWindowPosition", L"LogWindowSize", hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackLoggedEvent), &LoggedRegistration);
            PhUnregisterDialog(PhLogWindowHandle);
            PhLogWindowHandle = NULL;
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
                    ULONG selectedCount;

                    selectedCount = ListView_GetSelectedCount(ListViewHandle);

                    if (selectedCount == 0)
                    {
                        // User didn't select anything, so copy all items.
                        string = PhpGetStringForSelectedLogEntries(TRUE);
                        PhSetStateAllListViewItems(ListViewHandle, LVIS_SELECTED, LVIS_SELECTED);
                    }
                    else
                    {
                        string = PhpGetStringForSelectedLogEntries(FALSE);
                    }

                    PhSetClipboardString(hwndDlg, &string->sr);
                    PhDereferenceObject(string);

                    PhSetDialogFocus(hwndDlg, ListViewHandle);
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
                            PhWriteStringAsUtf8FileStream(fileStream, &PhUnicodeByteOrderMark);
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
                        if (dispInfo->item.mask & LVIF_TEXT)
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
                        if (dispInfo->item.mask & LVIF_TEXT)
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

/*
 * Process Hacker Extra Plugins -
 *   Debug View Plugin
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

#include "main.h"

static HANDLE DbgDialogThreadHandle = NULL;
static HWND DbgDialogHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;

static VOID NTAPI DbgLoggedEventCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_DBGEVENTS_CONTEXT context = (PPH_DBGEVENTS_CONTEXT)Context;

    if (context->DialogHandle)
    {
        PostMessage(context->DialogHandle, WM_DEBUG_LOG_UPDATED, 0, 0);
    }
}

static VOID DbgUpdateLogList(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    )
{
    Context->ListViewCount = Context->LogMessageList->Count;
    ListView_SetItemCountEx(Context->ListViewHandle, Context->ListViewCount, LVSICF_NOSCROLL);

    if (Context->ListViewCount >= 2 && Button_GetCheck(Context->AutoScrollHandle) == BST_CHECKED)
    {
        if (ListView_IsItemVisible(Context->ListViewHandle, Context->ListViewCount - 2))
        {
            ListView_EnsureVisible(Context->ListViewHandle, Context->ListViewCount - 1, FALSE);
        }
    }
}

static PPH_STRING DbgGetStringForSelectedLogEntries(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context,
    _In_ BOOLEAN All
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    if (Context->ListViewCount == 0)
        return PhReferenceEmptyString();

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    i = Context->ListViewCount - 1;

    while (TRUE)
    {
        PDEBUG_LOG_ENTRY entry;
        SYSTEMTIME systemTime;
        PPH_STRING temp;

        if (!All)
        {
            if (!(ListView_GetItemState(Context->ListViewHandle, i, LVIS_SELECTED) & LVIS_SELECTED))
            {
                goto ContinueLoop;
            }
        }

        entry =  Context->LogMessageList->Items[i];

        if (!entry)
            goto ContinueLoop;

        PhLargeIntegerToLocalSystemTime(&systemTime, &entry->Time);
        temp = PhFormatDateTime(&systemTime);
        PhAppendStringBuilder(&stringBuilder, temp);
        PhDereferenceObject(temp);
        PhAppendStringBuilder2(&stringBuilder, L": ");

        temp = PhFormatString( 
            L"%s (%u): %s",
            entry->ProcessName->Buffer, // entry->FilePath->Buffer;
            HandleToUlong(entry->ProcessId),
            entry->Message->Buffer
            );
        PhAppendStringBuilder(&stringBuilder, temp);
        PhDereferenceObject(temp);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");

ContinueLoop:

        if (i == 0)
            break;

        i--;
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

static VOID ShowListViewMenu(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    )
{
    INT index;
    ULONG id;
    POINT cursorPos;
    HMENU menu;
    HMENU subMenu;
    PDEBUG_LOG_ENTRY entry;

    index = PhFindListViewItemByFlags(Context->ListViewHandle, -1, LVNI_SELECTED);

    if (index == -1)
        return;

    entry = Context->LogMessageList->Items[index];

    if (entry)
    {
        GetCursorPos(&cursorPos);

        menu = LoadMenu(PluginInstance->DllBase, MAKEINTRESOURCE(IDR_MAIN_MENU));
        subMenu = GetSubMenu(menu, 0);

        id = (ULONG)TrackPopupMenu(
            subMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
            cursorPos.x,
            cursorPos.y,
            0,
            Context->DialogHandle,
            NULL
            );

        DestroyMenu(menu);

        switch (id)
        {
        case ID_GOTOOWNINGPROCESS:
            {
                PPH_PROCESS_NODE processNode;

                if (processNode = PhFindProcessNode(entry->ProcessId))
                {
                    ProcessHacker_ToggleVisible(PhMainWndHandle, TRUE);
                    ProcessHacker_SelectTabPage(PhMainWndHandle, 0);
                    ProcessHacker_SelectProcessNode(PhMainWndHandle, processNode);
                }
            }
            break;
        case ID_PROPERTIES:
            {
                DialogBoxParam(
                    PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDD_MESSAGE_DIALOG),
                    Context->DialogHandle,
                    DbgPropDlgProc,
                    (LPARAM)entry
                    );
            }
            break;
        case ID_COPY:
            {
                PPH_STRING string;

                string = DbgGetStringForSelectedLogEntries(Context, FALSE);

                PhSetClipboardString(Context->DialogHandle, &string->sr);
                PhDereferenceObject(string);

                SetFocus(Context->ListViewHandle);
            }
            break;
        case ID_EXCLUDEPROCESS_BYPID:
            {
                AddFilterType(Context, FilterByPid, entry->ProcessId, entry->ProcessName);
                DbgUpdateLogList(Context);
            }
            break;
        case ID_EXCLUDEPROCESS_BYNAME:
            {
                AddFilterType(Context, FilterByName, entry->ProcessId, entry->ProcessName);
                DbgUpdateLogList(Context);
            }
            break;
        }
    }
}

static VOID ShowDropdownMenu(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    )
{
    RECT rect;
    PPH_EMENU menu = NULL;
    PPH_EMENU_ITEM selectedItem = NULL;
    PPH_EMENU_ITEM resetMenuItem = NULL;
    PPH_EMENU_ITEM captureMenuItem = NULL;
    PPH_EMENU_ITEM captureGlobalMenuItem = NULL;

    GetWindowRect(Context->OptionsHandle, &rect);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, captureMenuItem = PhCreateEMenuItem(0, ID_CAPTURE_WIN32, L"Capture Win32", NULL, NULL), -1);
    PhInsertEMenuItem(menu, captureGlobalMenuItem = PhCreateEMenuItem(0, ID_CAPTURE_WIN32_GLOBAL, L"Capture Global Win32", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
    PhInsertEMenuItem(menu, resetMenuItem = PhCreateEMenuItem(PH_EMENU_DISABLED, ID_RESET_FILTERS, L"Reset Filters", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_CLEAR_EVENTS, L"Clear", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_SAVE_EVENTS, L"Save", NULL, NULL), -1);

    if (Context->ExcludeList->Count > 0)
    {
        resetMenuItem->Text = PhaFormatString(L"Reset Filters [%u]", Context->ExcludeList->Count)->Buffer;
        resetMenuItem->Flags &= ~PH_EMENU_DISABLED;
    }

    if (Context->CaptureLocalEnabled)
    {
        captureMenuItem->Flags |= PH_EMENU_CHECKED;
    }

    if (Context->CaptureGlobalEnabled)
    {
        captureGlobalMenuItem->Flags |= PH_EMENU_CHECKED;
    }

    selectedItem = PhShowEMenu(
        menu,
        Context->DialogHandle,
        PH_EMENU_SHOW_NONOTIFY | PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        rect.left,
        rect.bottom
        );

    if (selectedItem && selectedItem->Id != -1)
    {
        switch (selectedItem->Id)
        {
        case ID_CAPTURE_WIN32:
            {
                if (!Context->CaptureLocalEnabled)
                    DbgEventsCreate(Context, FALSE);
                else
                    DbgEventsCleanup(Context, FALSE);
            }
            break;
        case ID_CAPTURE_WIN32_GLOBAL:
            {
                if (!PhElevated)
                {
                    PhShowInformation(Context->DialogHandle, L"This option requires elevation.");
                    break;
                }

                if (!Context->CaptureGlobalEnabled)
                    DbgEventsCreate(Context, TRUE);
                else
                    DbgEventsCleanup(Context, TRUE);
            }
            break;
        case ID_RESET_FILTERS:
            {
                ResetFilters(Context);
            }
            break;
        case ID_CLEAR_EVENTS:
            {
                DbgClearLogEntries(Context);
                DbgUpdateLogList(Context);
            }
            break;
        case ID_SAVE_EVENTS:
            {
                static PH_FILETYPE_FILTER filters [] =
                {
                    { L"Text files (*.txt)", L"*.txt" },
                    { L"All files (*.*)", L"*.*" }
                };

                PVOID fileDialog = PhCreateSaveFileDialog();

                PhSetFileDialogFilter(fileDialog, filters, ARRAYSIZE(filters));
                PhSetFileDialogFileName(fileDialog, L"DbgView.txt");

                if (PhShowFileDialog(Context->DialogHandle, fileDialog))
                {
                    NTSTATUS status;
                    PPH_STRING fileName;
                    PPH_FILE_STREAM fileStream;
                    PPH_STRING string;

                    fileName = PhGetFileDialogFileName(fileDialog);
                    PhaDereferenceObject(fileName);

                    if (NT_SUCCESS(status = PhCreateFileStream(
                        &fileStream,
                        fileName->Buffer,
                        FILE_GENERIC_WRITE,
                        FILE_SHARE_READ,
                        FILE_OVERWRITE_IF,
                        0
                        )))
                    {
                        PhWritePhTextHeader(fileStream);

                        string = DbgGetStringForSelectedLogEntries(Context, TRUE);
                        PhWriteStringAsAnsiFileStreamEx(fileStream, string->Buffer, string->Length);
                        PhDereferenceObject(string);

                        PhDereferenceObject(fileStream);
                    }

                    if (!NT_SUCCESS(status))
                        PhShowStatus(PhMainWndHandle, L"Unable to create the file", status, 0);
                }

                PhFreeFileDialog(fileDialog);
            }
            break;
        }
    }

    PhDestroyEMenu(menu);
}

static INT_PTR CALLBACK DbgViewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_DBGEVENTS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_DBGEVENTS_CONTEXT)PhAllocate(sizeof(PH_DBGEVENTS_CONTEXT));
        memset(context, 0, sizeof(PH_DBGEVENTS_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_DBGEVENTS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, PhMainWndHandle);

            context->DialogHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DEBUGLISTVIEW);
            context->AutoScrollHandle = GetDlgItem(hwndDlg, IDC_AUTOSCROLL);
            context->OptionsHandle = GetDlgItem(hwndDlg, IDC_OPTIONS);
           
            context->LogMessageList = PhCreateList(1);
            context->ExcludeList = PhCreateList(1);

            PhRegisterDialog(hwndDlg);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Process");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Timestamp");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 220, L"Message");
            //PhSetExtendedListView(context->ListViewHandle);

            if (context->ListViewImageList = ImageList_Create(19, 19, ILC_COLOR32 | ILC_MASK, 0, 40))
            {
                HICON defaultIcon = PhGetFileShellIcon(NULL, L".exe", TRUE);
                ListView_SetImageList(context->ListViewHandle, context->ListViewImageList, LVSIL_SMALL);
                ImageList_AddIcon(context->ListViewImageList, defaultIcon);
                DestroyIcon(defaultIcon);
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->OptionsHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, context->AutoScrollHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCLOSE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_COLUMNS, context->ListViewHandle);
                       
            if (PhGetIntegerSetting(SETTING_NAME_ALWAYSONTOP))
            {
                context->AlwaysOnTop = TRUE;
                SetFocus(hwndDlg);
                SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), BST_CHECKED);
            }

            if (PhGetIntegerSetting(SETTING_NAME_AUTOSCROLL))
            {
                context->AutoScroll = TRUE;
                Button_SetCheck(context->AutoScrollHandle, BST_CHECKED);
            }

            DbgCreateSecurityAttributes(context);
            DbgUpdateLogList(context);

            PhRegisterCallback(&DbgLoggedCallback, DbgLoggedEventCallback, context, &context->DebugLoggedRegistration);
        }
        return TRUE;
    case WM_DESTROY:
        {
            DbgEventsCleanup(context, FALSE);
            DbgEventsCleanup(context, TRUE);
            DbgCleanupSecurityAttributes(context);

            if (context->ListViewImageList)
                ImageList_Destroy(context->ListViewImageList);

            if (context->ExcludeList)
            {
                ResetFilters(context);
                PhDereferenceObject(context->ExcludeList);
            }

            if (context->LogMessageList)
            {
                DbgClearLogEntries(context);
                PhDereferenceObject(context->LogMessageList);
            }
            
            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_COLUMNS, context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhUnregisterCallback(&DbgLoggedCallback, &context->DebugLoggedRegistration);
            PhUnregisterDialog(hwndDlg);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_SHOWDIALOG:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_OPTIONS:
                ShowDropdownMenu(context);
                break;
            case IDCLOSE:
            case IDCANCEL:
                PostQuitMessage(0);
                break;
            case IDC_AUTOSCROLL:
                {
                    context->AutoScroll = !context->AutoScroll;
                    PhSetIntegerSetting(SETTING_NAME_AUTOSCROLL, context->AutoScroll);
                    Button_SetCheck(context->AutoScrollHandle, context->AutoScroll ? BST_CHECKED : BST_UNCHECKED);
                }
                break;
            case IDC_ALWAYSONTOP:
                {
                    context->AlwaysOnTop = !context->AlwaysOnTop;
                    SetWindowPos(hwndDlg, context->AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                        0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                    PhSetIntegerSetting(SETTING_NAME_ALWAYSONTOP, context->AlwaysOnTop);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            switch (hdr->code)
            {
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        ShowListViewMenu(context);
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    PDEBUG_LOG_ENTRY entry;
                    INT index;

                    index = PhFindListViewItemByFlags(context->ListViewHandle, -1, LVNI_SELECTED);

                    if (index == -1)
                        break;

                    entry = context->LogMessageList->Items[index];

                    DialogBoxParam(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_MESSAGE_DIALOG),
                        hwndDlg,
                        DbgPropDlgProc,
                        (LPARAM)entry
                        );
                }
                break;
            case LVN_GETDISPINFO:
                {
                    NMLVDISPINFO* dispInfo = (NMLVDISPINFO*)hdr;
                    PDEBUG_LOG_ENTRY entry;

                    entry = context->LogMessageList->Items[dispInfo->item.iItem];

                    if (dispInfo->item.mask & LVIF_IMAGE)
                    {
                        dispInfo->item.iImage = entry->ImageIndex;
                    }

                    if (dispInfo->item.iSubItem == 0)
                    {
                        if (dispInfo->item.mask & LVIF_TEXT)
                        {
                            wcsncpy_s(
                                dispInfo->item.pszText,
                                dispInfo->item.cchTextMax,
                                entry->ProcessName->Buffer,
                                _TRUNCATE
                                );
                        }
                    }
                    else if (dispInfo->item.iSubItem == 1)
                    {
                        if (dispInfo->item.mask & LVIF_TEXT)
                        {
                            SYSTEMTIME systemTime;
                            PPH_STRING dateTime;
                            PPH_STRING dateString;

                            PhLargeIntegerToLocalSystemTime(&systemTime, &entry->Time);

                            dateTime = PhFormatTime(&systemTime, L"hh:mm:ss");
                            dateString = PhFormatString(
                                L"%s.%u",
                                dateTime->Buffer,
                                systemTime.wMilliseconds
                                );

                            wcsncpy_s(
                                dispInfo->item.pszText,
                                dispInfo->item.cchTextMax,
                                dateString->Buffer,
                                _TRUNCATE
                                );

                            PhDereferenceObject(dateString);
                            PhDereferenceObject(dateTime);
                        }
                    }
                    else if (dispInfo->item.iSubItem == 2)
                    {
                        if (dispInfo->item.mask & LVIF_TEXT)
                        {
                            wcsncpy_s(
                                dispInfo->item.pszText,
                                dispInfo->item.cchTextMax,
                                entry->Message->Buffer,
                                _TRUNCATE
                                );
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_DEBUG_LOG_UPDATED:
        DbgUpdateLogList(context);
        break;

    }

    return FALSE;
}

static NTSTATUS DbgViewDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    DbgDialogHandle = CreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_DBGVIEW_DIALOG),
        NULL,
        DbgViewDlgProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(DbgDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);

    if (DbgDialogHandle)
    {
        DestroyWindow(DbgDialogHandle);
        DbgDialogHandle = NULL;
    }

    if (DbgDialogThreadHandle)
    {
        NtClose(DbgDialogThreadHandle);
        DbgDialogThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID ShowDebugEventsDialog(
    VOID
    )
{
    if (!DbgDialogThreadHandle)
    {
        if (!(DbgDialogThreadHandle = PhCreateThread(0, DbgViewDialogThread, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the window.", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    PostMessage(DbgDialogHandle, WM_SHOWDIALOG, 0, 0);
}
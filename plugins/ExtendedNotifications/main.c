/*
 * Process Hacker Extended Notifications -
 *   main program
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

#include <phdk.h>
#include <windowsx.h>
#include "extnoti.h"
#include "resource.h"
#include "gntp-send/growl.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI NotifyEventCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID RegisterGrowl(
    VOID
    );

VOID NotifyGrowl(
    __in PPH_PLUGIN_NOTIFY_EVENT NotifyEvent
    );

INT_PTR CALLBACK ProcessesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK ServicesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK LoggingDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK GrowlDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION NotifyEventCallbackRegistration;

PPH_LIST ProcessFilterList;
PPH_LIST ServiceFilterList;

PSTR GrowlNotifications[] =
{
    "Process Created",
    "Process Terminated",
    "Service Created",
    "Service Deleted",
    "Service Started",
    "Service Stopped"
};

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ExtendedNotifications", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Extended Notifications";
            info->Author = L"wj32";
            info->Description = L"Filters notifications.";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNotifyEvent),
                NotifyEventCallback,
                NULL,
                &NotifyEventCallbackRegistration
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { IntegerSettingType, SETTING_NAME_ENABLE_GROWL, L"0" },
                    { StringSettingType, SETTING_NAME_LOG_FILENAME, L"" },
                    { StringSettingType, SETTING_NAME_PROCESS_LIST, L"\\i*" },
                    { StringSettingType, SETTING_NAME_SERVICE_LIST, L"\\i*" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            }

            ProcessFilterList = PhCreateList(10);
            ServiceFilterList = PhCreateList(10);
        }
        break;
    }

    return TRUE;
}

VOID FreeFilterEntry(
    __in PFILTER_ENTRY Entry
    )
{
    PhDereferenceObject(Entry->Filter);
    PhFree(Entry);
}

VOID ClearFilterList(
    __inout PPH_LIST FilterList
    )
{
    ULONG i;

    for (i = 0; i < FilterList->Count; i++)
        FreeFilterEntry(FilterList->Items[i]);

    PhClearList(FilterList);
}

VOID CopyFilterList(
    __inout PPH_LIST Destination,
    __in PPH_LIST Source
    )
{
    ULONG i;

    for (i = 0; i < Source->Count; i++)
    {
        PFILTER_ENTRY entry = Source->Items[i];
        PFILTER_ENTRY newEntry;

        newEntry = PhAllocate(sizeof(FILTER_ENTRY));
        newEntry->Type = entry->Type;
        newEntry->Filter = entry->Filter;
        PhReferenceObject(newEntry->Filter);

        PhAddItemList(Destination, newEntry);
    }
}

VOID LoadFilterList(
    __inout PPH_LIST FilterList,
    __in PPH_STRING String
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG length;
    ULONG i;
    PFILTER_ENTRY entry;

    length = String->Length / 2;
    PhInitializeStringBuilder(&stringBuilder, 20);

    entry = NULL;

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == '\\')
        {
            if (i != length - 1)
            {
                i++;

                switch (String->Buffer[i])
                {
                case 'i':
                case 'e':
                    if (entry)
                    {
                        entry->Filter = PhFinalStringBuilderString(&stringBuilder);
                        PhAddItemList(FilterList, entry);
                        PhInitializeStringBuilder(&stringBuilder, 20);
                    }

                    entry = PhAllocate(sizeof(FILTER_ENTRY));
                    entry->Type = String->Buffer[i] == 'i' ? FilterInclude : FilterExclude;

                    break;

                default:
                    PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);
                    break;
                }
            }
            else
            {
                // Trailing backslash. Just ignore it.
                break;
            }
        }
        else
        {
            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);
        }
    }

    if (entry)
    {
        entry->Filter = PhFinalStringBuilderString(&stringBuilder);
        PhAddItemList(FilterList, entry);
    }
    else
    {
        PhDeleteStringBuilder(&stringBuilder);
    }
}

PPH_STRING SaveFilterList(
    __inout PPH_LIST FilterList
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;
    ULONG j;
    WCHAR temp[2];

    PhInitializeStringBuilder(&stringBuilder, 100);

    temp[0] = '\\';

    for (i = 0; i < FilterList->Count; i++)
    {
        PFILTER_ENTRY entry = FilterList->Items[i];
        ULONG length;

        // Write the entry type.

        temp[1] = entry->Type == FilterInclude ? 'i' : 'e';

        PhAppendStringBuilderEx(&stringBuilder, temp, 4);

        // Write the filter string.

        length = entry->Filter->Length / 2;

        for (j = 0; j < length; j++)
        {
            if (entry->Filter->Buffer[j] == '\\') // escape backslashes
            {
                temp[1] = entry->Filter->Buffer[j];
                PhAppendStringBuilderEx(&stringBuilder, temp, 4);
            }
            else
            {
                PhAppendCharStringBuilder(&stringBuilder, entry->Filter->Buffer[j]);
            }
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(SETTING_NAME_PROCESS_LIST);
    LoadFilterList(ProcessFilterList, string);
    PhDereferenceObject(string);

    string = PhGetStringSetting(SETTING_NAME_SERVICE_LIST);
    LoadFilterList(ServiceFilterList, string);
    PhDereferenceObject(string);

    FileLogInitialization();

    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL))
        RegisterGrowl();
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[4];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = (HWND)Parameter;
    propSheetHeader.pszCaption = L"Extended Notifications";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // Processes
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSES);
    propSheetPage.pfnDlgProc = ProcessesDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Services
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SERVICES);
    propSheetPage.pfnDlgProc = ServicesDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Logging
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_LOGGING);
    propSheetPage.pfnDlgProc = LoggingDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Growl
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_GROWL);
    propSheetPage.pfnDlgProc = GrowlDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PropertySheet(&propSheetHeader);
}

BOOLEAN MatchFilterList(
    __in PPH_LIST FilterList,
    __in PPH_STRING String,
    __out FILTER_TYPE *FilterType
    )
{
    ULONG i;
    BOOLEAN isFileName;

    isFileName = PhFindCharInString(String, 0, '\\') != -1;

    for (i = 0; i < FilterList->Count; i++)
    {
        PFILTER_ENTRY entry = FilterList->Items[i];

        if (isFileName && PhFindCharInString(entry->Filter, 0, '\\') == -1)
            continue; // ignore filters without backslashes if we're matching a file name

        if (entry->Filter->Length == 2 && entry->Filter->Buffer[0] == '*') // shortcut
        {
            *FilterType = entry->Type;
            return TRUE;
        }

        if (PhMatchWildcards(entry->Filter->Buffer, String->Buffer, TRUE))
        {
            *FilterType = entry->Type;
            return TRUE;
        }
    }

    return FALSE;
}

VOID NTAPI NotifyEventCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_NOTIFY_EVENT notifyEvent = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_SERVICE_ITEM serviceItem;
    FILTER_TYPE filterType;
    BOOLEAN found;

    filterType = FilterExclude;

    switch (notifyEvent->Type)
    {
    case PH_NOTIFY_PROCESS_CREATE:
    case PH_NOTIFY_PROCESS_DELETE:
        processItem = notifyEvent->Parameter;

        if (processItem->FileName)
            found = MatchFilterList(ProcessFilterList, processItem->FileName, &filterType);

        if (!found)
            MatchFilterList(ProcessFilterList, processItem->ProcessName, &filterType);

        break;

    case PH_NOTIFY_SERVICE_CREATE:
    case PH_NOTIFY_SERVICE_DELETE:
    case PH_NOTIFY_SERVICE_START:
    case PH_NOTIFY_SERVICE_STOP:
        serviceItem = notifyEvent->Parameter;

        MatchFilterList(ServiceFilterList, serviceItem->Name, &filterType);

        break;
    }

    if (filterType == FilterExclude)
        notifyEvent->Handled = TRUE; // pretend we handled the notification to prevent it from displaying

    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL))
        NotifyGrowl(notifyEvent);
}

VOID RegisterGrowl(
    VOID
    )
{
    static BOOLEAN registered = FALSE;

    if (registered)
        return;

    growl_tcp_register("127.0.0.1", "Process Hacker", GrowlNotifications, sizeof(GrowlNotifications) / sizeof(PSTR), NULL, NULL);

    registered = TRUE;
}

VOID NotifyGrowl(
    __in PPH_PLUGIN_NOTIFY_EVENT NotifyEvent
    )
{
    PSTR notification;
    PPH_STRING title;
    PPH_ANSI_STRING titleAnsi;
    PPH_STRING message;
    PPH_ANSI_STRING messageAnsi;
    PPH_PROCESS_ITEM processItem;
    PPH_SERVICE_ITEM serviceItem;
    PPH_PROCESS_ITEM parentProcessItem;

    if (NotifyEvent->Handled)
        return;

    switch (NotifyEvent->Type)
    {
    case PH_NOTIFY_PROCESS_CREATE:
        processItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[0];
        title = processItem->ProcessName;
        PhReferenceObject(title);

        parentProcessItem = PhReferenceProcessItemForParent(
            processItem->ParentProcessId,
            processItem->ProcessId,
            &processItem->CreateTime
            );

        message = PhFormatString(
            L"The process %s (%u) was started by %s.",
            processItem->ProcessName->Buffer,
            (ULONG)processItem->ProcessId,
            parentProcessItem ? parentProcessItem->ProcessName->Buffer : L"an unknown process"
            );

        if (parentProcessItem)
            PhDereferenceObject(parentProcessItem);

        break;
    case PH_NOTIFY_PROCESS_DELETE:
        processItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[1];
        title = processItem->ProcessName;
        PhReferenceObject(title);

        message = PhFormatString(L"The process %s (%u) was terminated.",
            processItem->ProcessName->Buffer,
            (ULONG)processItem->ProcessId
            );

        break;
    case PH_NOTIFY_SERVICE_CREATE:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[2];
        title = serviceItem->DisplayName;
        PhReferenceObject(title);

        message = PhFormatString(L"The service %s (%s) has been created.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    case PH_NOTIFY_SERVICE_DELETE:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[3];
        title = serviceItem->DisplayName;
        PhReferenceObject(title);

        message = PhFormatString(L"The service %s (%s) has been deleted.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    case PH_NOTIFY_SERVICE_START:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[4];
        title = serviceItem->DisplayName;
        PhReferenceObject(title);

        message = PhFormatString(L"The service %s (%s) has been started.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    case PH_NOTIFY_SERVICE_STOP:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[5];
        title = serviceItem->DisplayName;
        PhReferenceObject(title);

        message = PhFormatString(L"The service %s (%s) has been stopped.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    default:
        return;
    }

    titleAnsi = PhCreateAnsiStringFromUnicodeEx(title->Buffer, title->Length);
    messageAnsi = PhCreateAnsiStringFromUnicodeEx(message->Buffer, message->Length);

    if (growl_tcp_notify("127.0.0.1", "Process Hacker", notification, titleAnsi->Buffer, messageAnsi->Buffer, NULL, NULL, NULL) == 0)
        NotifyEvent->Handled = TRUE;

    PhDereferenceObject(messageAnsi);
    PhDereferenceObject(titleAnsi);
    PhDereferenceObject(message);
    PhDereferenceObject(title);
}

PPH_STRING FormatFilterEntry(
    __in PFILTER_ENTRY Entry
    )
{
    return PhConcatStrings2(Entry->Type == FilterInclude ? L"[Include] " : L"[Exclude] ", Entry->Filter->Buffer);
}

VOID AddEntriesToListBox(
    __in HWND ListBox,
    __in PPH_LIST FilterList
    )
{
    ULONG i;

    for (i = 0; i < FilterList->Count; i++)
    {
        PFILTER_ENTRY entry = FilterList->Items[i];
        PPH_STRING string;

        string = FormatFilterEntry(entry);
        ListBox_AddString(ListBox, string->Buffer);
        PhDereferenceObject(string);
    }
}

PPH_LIST EditingProcessFilterList;
PPH_LIST EditingServiceFilterList;

static LRESULT CALLBACK TextBoxWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    oldWndProc = (WNDPROC)GetProp(hwnd, L"OldWndProc");

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            RemoveProp(hwnd, L"OldWndProc");
        }
        break;
    case WM_GETDLGCODE:
        {
            if (wParam == VK_RETURN)
                return DLGC_WANTALLKEYS;
        }
        break;
    case WM_CHAR:
        {
            if (wParam == VK_RETURN)
            {
                SendMessage(GetParent(hwnd), WM_COMMAND, IDC_TEXT_RETURN, 0);
                return 0;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

VOID FixControlStates(
    __in HWND hwndDlg,
    __in HWND ListBox
    )
{
    ULONG i;
    ULONG count;

    i = ListBox_GetCurSel(ListBox);
    count = ListBox_GetCount(ListBox);

    EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), i != LB_ERR);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), i != LB_ERR && i != 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), i != LB_ERR && i != count - 1);
}

INT_PTR HandleCommonMessages(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in HWND ListBox,
    __in PPH_LIST FilterList
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND textBoxHandle;
            WNDPROC oldWndProc;

            textBoxHandle = GetDlgItem(hwndDlg, IDC_TEXT);
            oldWndProc = (WNDPROC)GetWindowLongPtr(textBoxHandle, GWLP_WNDPROC);
            SetWindowLongPtr(textBoxHandle, GWLP_WNDPROC, (LONG_PTR)TextBoxWndProc);
            SetProp(textBoxHandle, L"OldWndProc", (HANDLE)oldWndProc);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_INCLUDE), BST_CHECKED);

            FixControlStates(hwndDlg, ListBox);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_LIST:
                {
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        ULONG i;

                        i = ListBox_GetCurSel(ListBox);

                        if (i != LB_ERR)
                        {
                            PFILTER_ENTRY entry;

                            entry = FilterList->Items[i];
                            SetDlgItemText(hwndDlg, IDC_TEXT, entry->Filter->Buffer);
                            Button_SetCheck(GetDlgItem(hwndDlg, IDC_INCLUDE),
                                entry->Type == FilterInclude ? BST_CHECKED : BST_UNCHECKED);
                            Button_SetCheck(GetDlgItem(hwndDlg, IDC_EXCLUDE),
                                entry->Type == FilterExclude ? BST_CHECKED : BST_UNCHECKED);
                        }

                        FixControlStates(hwndDlg, ListBox);
                    }
                }
                break;
            case IDC_ADD:
            case IDC_TEXT_RETURN:
                {
                    ULONG i;
                    PPH_STRING string;
                    PFILTER_ENTRY entry;
                    FILTER_TYPE type;
                    PPH_STRING entryString;

                    string = PhGetWindowText(GetDlgItem(hwndDlg, IDC_TEXT));

                    if (string->Length == 0)
                    {
                        PhDereferenceObject(string);
                        return FALSE;
                    }

                    for (i = 0; i < FilterList->Count; i++)
                    {
                        entry = FilterList->Items[i];

                        if (PhEqualString(entry->Filter, string, TRUE))
                            break;
                    }

                    type = Button_GetCheck(GetDlgItem(hwndDlg, IDC_INCLUDE)) == BST_CHECKED ? FilterInclude : FilterExclude;

                    if (i == FilterList->Count)
                    {
                        // No existing entry, so add a new one.

                        entry = PhAllocate(sizeof(FILTER_ENTRY));
                        entry->Type = type;
                        entry->Filter = string;
                        PhInsertItemList(FilterList, 0, entry);

                        entryString = FormatFilterEntry(entry);
                        ListBox_InsertString(ListBox, 0, entryString->Buffer);
                        PhDereferenceObject(entryString);

                        ListBox_SetCurSel(ListBox, 0);
                    }
                    else
                    {
                        entry->Type = type;
                        PhDereferenceObject(entry->Filter);
                        entry->Filter = string;

                        ListBox_DeleteString(ListBox, i);
                        entryString = FormatFilterEntry(entry);
                        ListBox_InsertString(ListBox, i, entryString->Buffer);
                        PhDereferenceObject(entryString);

                        ListBox_SetCurSel(ListBox, i);
                    }

                    SetFocus(GetDlgItem(hwndDlg, IDC_TEXT));
                    Edit_SetSel(GetDlgItem(hwndDlg, IDC_TEXT), 0, -1);

                    FixControlStates(hwndDlg, ListBox);
                }
                break;
            case IDC_REMOVE:
                {
                    ULONG i;
                    PFILTER_ENTRY entry;

                    i = ListBox_GetCurSel(ListBox);

                    if (i != LB_ERR)
                    {
                        entry = FilterList->Items[i];
                        FreeFilterEntry(entry);
                        PhRemoveItemList(FilterList, i);
                        ListBox_DeleteString(ListBox, i);

                        if (i >= FilterList->Count)
                            i = FilterList->Count - 1;

                        ListBox_SetCurSel(ListBox, i);

                        FixControlStates(hwndDlg, ListBox);
                    }
                }
                break;
            case IDC_MOVEUP:
                {
                    ULONG i;
                    PFILTER_ENTRY entry;
                    PPH_STRING entryString;

                    i = ListBox_GetCurSel(ListBox);

                    if (i != LB_ERR && i != 0)
                    {
                        entry = FilterList->Items[i];

                        PhRemoveItemList(FilterList, i);
                        PhInsertItemList(FilterList, i - 1, entry);

                        ListBox_DeleteString(ListBox, i);
                        entryString = FormatFilterEntry(entry);
                        ListBox_InsertString(ListBox, i - 1, entryString->Buffer);
                        PhDereferenceObject(entryString);

                        i--;
                        ListBox_SetCurSel(ListBox, i);

                        FixControlStates(hwndDlg, ListBox);
                    }
                }
                break;
            case IDC_MOVEDOWN:
                {
                    ULONG i;
                    PFILTER_ENTRY entry;
                    PPH_STRING entryString;

                    i = ListBox_GetCurSel(ListBox);

                    if (i != LB_ERR && i != FilterList->Count - 1)
                    {
                        entry = FilterList->Items[i];

                        PhRemoveItemList(FilterList, i);
                        PhInsertItemList(FilterList, i + 1, entry);

                        ListBox_DeleteString(ListBox, i);
                        entryString = FormatFilterEntry(entry);
                        ListBox_InsertString(ListBox, i + 1, entryString->Buffer);
                        PhDereferenceObject(entryString);

                        i++;
                        ListBox_SetCurSel(ListBox, i);

                        FixControlStates(hwndDlg, ListBox);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ProcessesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    INT_PTR result;

    if (result = HandleCommonMessages(hwndDlg, uMsg, wParam, lParam,
        GetDlgItem(hwndDlg, IDC_LIST), EditingProcessFilterList))
        return result;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            EditingProcessFilterList = PhCreateList(ProcessFilterList->Count + 10);
            CopyFilterList(EditingProcessFilterList, ProcessFilterList);

            AddEntriesToListBox(GetDlgItem(hwndDlg, IDC_LIST), EditingProcessFilterList);
        }
        break;
    case WM_DESTROY:
        {
            ClearFilterList(EditingProcessFilterList);
            PhDereferenceObject(EditingProcessFilterList);
            EditingProcessFilterList = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                // Nothing
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_APPLY:
                {
                    PPH_STRING string;

                    ClearFilterList(ProcessFilterList);
                    CopyFilterList(ProcessFilterList, EditingProcessFilterList);

                    string = SaveFilterList(ProcessFilterList);
                    PhSetStringSetting2(SETTING_NAME_PROCESS_LIST, &string->sr);
                    PhDereferenceObject(string);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ServicesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    if (HandleCommonMessages(hwndDlg, uMsg, wParam, lParam,
        GetDlgItem(hwndDlg, IDC_LIST), EditingServiceFilterList))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            EditingServiceFilterList = PhCreateList(ServiceFilterList->Count + 10);
            CopyFilterList(EditingServiceFilterList, ServiceFilterList);

            AddEntriesToListBox(GetDlgItem(hwndDlg, IDC_LIST), EditingServiceFilterList);
        }
        break;
    case WM_DESTROY:
        {
            ClearFilterList(EditingServiceFilterList);
            PhDereferenceObject(EditingServiceFilterList);
            EditingServiceFilterList = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                // Nothing
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_APPLY:
                {
                    PPH_STRING string;

                    ClearFilterList(ServiceFilterList);
                    CopyFilterList(ServiceFilterList, EditingServiceFilterList);

                    string = SaveFilterList(ServiceFilterList);
                    PhSetStringSetting2(SETTING_NAME_SERVICE_LIST, &string->sr);
                    PhDereferenceObject(string);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK LoggingDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetDlgItemText(hwndDlg, IDC_LOGFILENAME, ((PPH_STRING)PHA_DEREFERENCE(PhGetStringSetting(SETTING_NAME_LOG_FILENAME)))->Buffer);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Log files (*.txt;*.log)", L"*.txt;*.log" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateSaveFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PhGetFileName(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_LOGFILENAME));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_LOGFILENAME, fileName->Buffer);
                        PhDereferenceObject(fileName);
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
            case PSN_APPLY:
                {
                    PhSetStringSetting2(SETTING_NAME_LOG_FILENAME, &PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_LOGFILENAME)->sr);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK GrowlDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING licenseText;

            licenseText = PhCreateStringFromAnsi(gntp_send_license_text);
            SetDlgItemText(hwndDlg, IDC_LICENSE, licenseText->Buffer);
            PhDereferenceObject(licenseText);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLEGROWL), PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL) ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_ENABLEGROWL));
                }
                return TRUE;
            case PSN_APPLY:
                {
                    PhSetIntegerSetting(SETTING_NAME_ENABLE_GROWL, Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLEGROWL)) == BST_CHECKED);

                    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL))
                        RegisterGrowl();

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

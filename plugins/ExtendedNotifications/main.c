/*
 * Process Hacker Extended Notifications -
 *   main program
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017 dmex
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
#include <workqueue.h>
#include <settings.h>

#include "extnoti.h"
#include "resource.h"
#include "gntp-send/growl.h"

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI NotifyEventCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID RegisterGrowl(
    _In_ BOOLEAN Force
    );

VOID NotifyGrowl(
    _In_ PPH_PLUGIN_NOTIFY_EVENT NotifyEvent
    );

NTSTATUS NTAPI RegisterGrowlCallback(
    _In_ PVOID Parameter
    );

INT_PTR CALLBACK ProcessesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK LoggingDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK GrowlDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Extended Notifications";
            info->Author = L"wj32";
            info->Description = L"Filters notifications.";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1112";

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
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
    _In_ PFILTER_ENTRY Entry
    )
{
    PhDereferenceObject(Entry->Filter);
    PhFree(Entry);
}

VOID ClearFilterList(
    _Inout_ PPH_LIST FilterList
    )
{
    ULONG i;

    for (i = 0; i < FilterList->Count; i++)
        FreeFilterEntry(FilterList->Items[i]);

    PhClearList(FilterList);
}

VOID CopyFilterList(
    _Inout_ PPH_LIST Destination,
    _In_ PPH_LIST Source
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
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T length;
    SIZE_T i;
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
    _Inout_ PPH_LIST FilterList
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T i;
    SIZE_T j;
    WCHAR temp[2];

    PhInitializeStringBuilder(&stringBuilder, 100);

    temp[0] = '\\';

    for (i = 0; i < FilterList->Count; i++)
    {
        PFILTER_ENTRY entry = FilterList->Items[i];
        SIZE_T length;

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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    LoadFilterList(ProcessFilterList, PhaGetStringSetting(SETTING_NAME_PROCESS_LIST));
    LoadFilterList(ServiceFilterList, PhaGetStringSetting(SETTING_NAME_SERVICE_LIST));

    FileLogInitialization();

    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL))
    {
        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), RegisterGrowlCallback, NULL);
    }
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    if (!optionsEntry)
        return;

    optionsEntry->CreateSection(
        L"Notifications - Processes",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROCESSES),
        ProcessesDlgProc,
        NULL
        );
    optionsEntry->CreateSection(
        L"Notifications - Services",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SERVICES),
        ServicesDlgProc,
        NULL
        );
    optionsEntry->CreateSection(
        L"Notifications - Logging",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_LOGGING),
        LoggingDlgProc,
        NULL
        );
    optionsEntry->CreateSection(
        L"Notifications - Growl",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_GROWL),
        GrowlDlgProc,
        NULL
        );
}

_Success_(return)
BOOLEAN MatchFilterList(
    _In_ PPH_LIST FilterList,
    _In_ PPH_STRING String,
    _Out_ FILTER_TYPE *FilterType
    )
{
    ULONG i;
    BOOLEAN isFileName;

    isFileName = PhFindCharInString(String, 0, OBJ_NAME_PATH_SEPARATOR) != -1;

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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_NOTIFY_EVENT notifyEvent = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_SERVICE_ITEM serviceItem;
    FILTER_TYPE filterType = FilterExclude;
    BOOLEAN found = FALSE;

    if (!notifyEvent)
        return;

    switch (notifyEvent->Type)
    {
    case PH_NOTIFY_PROCESS_CREATE:
    case PH_NOTIFY_PROCESS_DELETE:
        processItem = notifyEvent->Parameter;

        if (processItem->FileNameWin32)
            found = MatchFilterList(ProcessFilterList, processItem->FileNameWin32, &filterType);

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
    _In_ BOOLEAN Force
    )
{
    static BOOLEAN registered = FALSE;

    if (!Force && registered)
        return;

    growl_tcp_register("127.0.0.1", "Process Hacker", GrowlNotifications, sizeof(GrowlNotifications) / sizeof(PSTR), NULL, NULL);

    registered = TRUE;
}

VOID NotifyGrowl(
    _In_ PPH_PLUGIN_NOTIFY_EVENT NotifyEvent
    )
{
    PSTR notification;
    PPH_STRING title;
    PPH_BYTES titleUtf8;
    PPH_STRING message;
    PPH_BYTES messageUtf8;
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

        parentProcessItem = PhReferenceProcessItemForParent(processItem);

        message = PhaFormatString(
            L"The process %s (%lu) was started by %s.",
            processItem->ProcessName->Buffer,
            HandleToUlong(processItem->ProcessId),
            parentProcessItem ? parentProcessItem->ProcessName->Buffer : L"an unknown process"
            );

        if (parentProcessItem)
            PhDereferenceObject(parentProcessItem);

        break;
    case PH_NOTIFY_PROCESS_DELETE:
        processItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[1];
        title = processItem->ProcessName;

        message = PhaFormatString(L"The process %s (%lu) was terminated.",
            processItem->ProcessName->Buffer,
            HandleToUlong(processItem->ProcessId)
            );

        break;
    case PH_NOTIFY_SERVICE_CREATE:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[2];
        title = serviceItem->DisplayName;

        message = PhaFormatString(L"The service %s (%s) has been created.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    case PH_NOTIFY_SERVICE_DELETE:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[3];
        title = serviceItem->DisplayName;

        message = PhaFormatString(L"The service %s (%s) has been deleted.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    case PH_NOTIFY_SERVICE_START:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[4];
        title = serviceItem->DisplayName;

        message = PhaFormatString(L"The service %s (%s) has been started.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    case PH_NOTIFY_SERVICE_STOP:
        serviceItem = NotifyEvent->Parameter;
        notification = GrowlNotifications[5];
        title = serviceItem->DisplayName;

        message = PhaFormatString(L"The service %s (%s) has been stopped.",
            serviceItem->Name->Buffer,
            serviceItem->DisplayName->Buffer
            );

        break;
    default:
        return;
    }

    titleUtf8 = PH_AUTO(PhConvertUtf16ToUtf8Ex(title->Buffer, title->Length));
    messageUtf8 = PH_AUTO(PhConvertUtf16ToUtf8Ex(message->Buffer, message->Length));

    RegisterGrowl(TRUE);

    if (growl_tcp_notify("127.0.0.1", "Process Hacker", notification, titleUtf8->Buffer, messageUtf8->Buffer, NULL, NULL, NULL) == 0)
        NotifyEvent->Handled = TRUE;
}

NTSTATUS NTAPI RegisterGrowlCallback(
    _In_ PVOID Parameter
    )
{
    RegisterGrowl(FALSE);

    return STATUS_SUCCESS;
}

PPH_STRING FormatFilterEntry(
    _In_ PFILTER_ENTRY Entry
    )
{
    return PhConcatStrings2(Entry->Type == FilterInclude ? L"[Include] " : L"[Exclude] ", Entry->Filter->Buffer);
}

VOID AddEntriesToListBox(
    _In_ HWND ListBox,
    _In_ PPH_LIST FilterList
    )
{
    ULONG i;

    for (i = 0; i < FilterList->Count; i++)
    {
        PFILTER_ENTRY entry = FilterList->Items[i];

        ListBox_AddString(ListBox, PH_AUTO_T(PH_STRING, FormatFilterEntry(entry))->Buffer);
    }
}

PPH_LIST EditingProcessFilterList;
PPH_LIST EditingServiceFilterList;

LRESULT CALLBACK TextBoxSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc = PhGetWindowContext(hWnd, UCHAR_MAX);

    if (!oldWndProc)
        return 0;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hWnd, UCHAR_MAX);
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
                SendMessage(GetParent(hWnd), WM_COMMAND, IDC_TEXT_RETURN, 0);
                return 0;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID FixControlStates(
    _In_ HWND hwndDlg,
    _In_ HWND ListBox
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ HWND ListBox,
    _In_ PPH_LIST FilterList
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
            PhSetWindowContext(textBoxHandle, UCHAR_MAX, oldWndProc);
            SetWindowLongPtr(textBoxHandle, GWLP_WNDPROC, (LONG_PTR)TextBoxSubclassProc);
            
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_INCLUDE), BST_CHECKED);

            FixControlStates(hwndDlg, ListBox);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_LIST:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
                    {
                        ULONG i;

                        i = ListBox_GetCurSel(ListBox);

                        if (i != LB_ERR)
                        {
                            PFILTER_ENTRY entry;

                            entry = FilterList->Items[i];
                            PhSetDialogItemText(hwndDlg, IDC_TEXT, entry->Filter->Buffer);
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
                    PFILTER_ENTRY entry = NULL;
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

                    PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_TEXT));
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;
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

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_MOVEUP), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_MOVEDOWN), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_TEXT), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_INCLUDE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_EXCLUDE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_ADD), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_REMOVE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_INFO), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            AddEntriesToListBox(GetDlgItem(hwndDlg, IDC_LIST), EditingProcessFilterList);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING string;

            ClearFilterList(ProcessFilterList);
            CopyFilterList(ProcessFilterList, EditingProcessFilterList);

            string = SaveFilterList(ProcessFilterList);
            PhSetStringSetting2(SETTING_NAME_PROCESS_LIST, &string->sr);
            PhDereferenceObject(string);

            ClearFilterList(EditingProcessFilterList);
            PhDereferenceObject(EditingProcessFilterList);
            EditingProcessFilterList = NULL;

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;
    INT_PTR result;

    if (result = HandleCommonMessages(hwndDlg, uMsg, wParam, lParam,
        GetDlgItem(hwndDlg, IDC_LIST), EditingServiceFilterList))
        return result;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            EditingServiceFilterList = PhCreateList(ServiceFilterList->Count + 10);
            CopyFilterList(EditingServiceFilterList, ServiceFilterList);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_MOVEUP), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_MOVEDOWN), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_TEXT), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_INCLUDE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_EXCLUDE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_ADD), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_REMOVE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_INFO), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            AddEntriesToListBox(GetDlgItem(hwndDlg, IDC_LIST), EditingServiceFilterList);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING string;

            ClearFilterList(ServiceFilterList);
            CopyFilterList(ServiceFilterList, EditingServiceFilterList);

            string = SaveFilterList(ServiceFilterList);
            PhSetStringSetting2(SETTING_NAME_SERVICE_LIST, &string->sr);
            PhDereferenceObject(string);

            ClearFilterList(EditingServiceFilterList);
            PhDereferenceObject(EditingServiceFilterList);
            EditingServiceFilterList = NULL;

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK LoggingDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetDialogItemText(hwndDlg, IDC_LOGFILENAME, PhaGetStringSetting(SETTING_NAME_LOG_FILENAME)->Buffer);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_INFO), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LOGFILENAME), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_BROWSE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
        }
        break;
    case WM_DESTROY:
        {
            PhSetStringSetting2(SETTING_NAME_LOG_FILENAME, &PhaGetDlgItemText(hwndDlg, IDC_LOGFILENAME)->sr);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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

                    fileName = PH_AUTO(PhGetFileName(PhaGetDlgItemText(hwndDlg, IDC_LOGFILENAME)));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                        PhSetDialogItemText(hwndDlg, IDC_LOGFILENAME, fileName->Buffer);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK GrowlDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetDialogItemText(hwndDlg, IDC_LICENSE, PH_AUTO_T(PH_STRING, PhConvertUtf8ToUtf16(gntp_send_license_text))->Buffer);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLEGROWL), PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL) ? BST_CHECKED : BST_UNCHECKED);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LICENSE), NULL, PH_ANCHOR_ALL);
        }
        break;
    case WM_DESTROY:
        {
            PhSetIntegerSetting(SETTING_NAME_ENABLE_GROWL, Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLEGROWL)) == BST_CHECKED);

            if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GROWL))
                RegisterGrowl(FALSE);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    }

    return FALSE;
}

/*
 * Process Hacker Extra Plugins -
 *   Network Adapters Plugin
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

#define ITEM_CHECKED (INDEXTOSTATEIMAGEMASK(2))
#define ITEM_UNCHECKED (INDEXTOSTATEIMAGEMASK(1))

static VOID FreeAdaptersEntry(
    _In_ PPH_NETADAPTER_ENTRY Entry
    )
{
    if (Entry->InterfaceGuid)
        PhDereferenceObject(Entry->InterfaceGuid);

    PhFree(Entry);
}

static VOID ClearAdaptersList(
    _Inout_ PPH_LIST FilterList
    )
{
    for (ULONG i = 0; i < FilterList->Count; i++)
    {
        FreeAdaptersEntry((PPH_NETADAPTER_ENTRY)FilterList->Items[i]);
    }

    PhClearList(FilterList);
}

static VOID CopyAdaptersList(
    _Inout_ PPH_LIST Destination,
    _In_ PPH_LIST Source
    )
{
    for (ULONG i = 0; i < Source->Count; i++)
    {
        PPH_NETADAPTER_ENTRY entry = (PPH_NETADAPTER_ENTRY)Source->Items[i];
        PPH_NETADAPTER_ENTRY newEntry;

        newEntry = (PPH_NETADAPTER_ENTRY)PhAllocate(sizeof(PH_NETADAPTER_ENTRY));
        memset(newEntry, 0, sizeof(PH_NETADAPTER_ENTRY));

        newEntry->InterfaceIndex = entry->InterfaceIndex;
        newEntry->InterfaceLuid = entry->InterfaceLuid;
        newEntry->InterfaceGuid = entry->InterfaceGuid;

        PhAddItemList(Destination, newEntry);
    }
}

VOID LoadAdaptersList(
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    )
{
    PH_STRINGREF remaining = String->sr;

    while (remaining.Length != 0)
    {
        PPH_NETADAPTER_ENTRY entry = NULL;

        ULONG64 ifindex;
        ULONG64 luid64;
        PH_STRINGREF part1;
        PH_STRINGREF part2;
        PH_STRINGREF part3;

        entry = (PPH_NETADAPTER_ENTRY)PhAllocate(sizeof(PH_NETADAPTER_ENTRY));
        memset(entry, 0, sizeof(PH_NETADAPTER_ENTRY));

        PhSplitStringRefAtChar(&remaining, ',', &part1, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part2, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part3, &remaining);

        if (PhStringToInteger64(&part1, 10, &ifindex))
        {
            entry->InterfaceIndex = (IF_INDEX)ifindex;
        }
        if (PhStringToInteger64(&part2, 10, &luid64))
        {
            entry->InterfaceLuid.Value = luid64;
        }
        if (part3.Buffer)
        {
            entry->InterfaceGuid = PhCreateString2(&part3);
        }

        PhAddItemList(FilterList, entry);
    }
}

static PPH_STRING SaveAdaptersList(
    _Inout_ PPH_LIST FilterList
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (SIZE_T i = 0; i < FilterList->Count; i++)
    {
        PPH_NETADAPTER_ENTRY entry = (PPH_NETADAPTER_ENTRY)FilterList->Items[i];

        PhAppendFormatStringBuilder(&stringBuilder,
            L"%lu,%I64u,%s,",
            entry->InterfaceIndex,    // This value is UNSAFE and may change after reboot.
            entry->InterfaceLuid.Value, // This value is SAFE and does not change (Vista+).
            entry->InterfaceGuid->Buffer
            );
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static VOID AddNetworkAdapterToListView(
    _In_ PPH_NETADAPTER_CONTEXT Context,
    _In_ PIP_ADAPTER_ADDRESSES Adapter
    )
{
    PPH_NETADAPTER_ENTRY entry;

    entry = (PPH_NETADAPTER_ENTRY)PhAllocate(sizeof(PH_NETADAPTER_ENTRY));
    memset(entry, 0, sizeof(PH_NETADAPTER_ENTRY));

    entry->InterfaceIndex = Adapter->IfIndex;
    entry->InterfaceLuid = Adapter->Luid;
    entry->InterfaceGuid = PhConvertMultiByteToUtf16(Adapter->AdapterName);

    INT index = PhAddListViewItem(
        Context->ListViewHandle,
        MAXINT,
        Adapter->Description,
        entry
        );

    for (ULONG i = 0; i < Context->NetworkAdaptersListEdited->Count; i++)
    {
        PPH_NETADAPTER_ENTRY currentEntry = (PPH_NETADAPTER_ENTRY)Context->NetworkAdaptersListEdited->Items[i];

        if (WindowsVersion > WINDOWS_XP)
        {
            if (entry->InterfaceLuid.Value == currentEntry->InterfaceLuid.Value)
            {
                ListView_SetItemState(Context->ListViewHandle, index, ITEM_CHECKED, LVIS_STATEIMAGEMASK);
                break;
            }
        }
        else
        {
            if (entry->InterfaceIndex == currentEntry->InterfaceIndex)
            {
                ListView_SetItemState(Context->ListViewHandle, index, ITEM_CHECKED, LVIS_STATEIMAGEMASK);
                break;
            }
        }
    }
}

static VOID FindNetworkAdapters(
    _In_ PPH_NETADAPTER_CONTEXT Context,
    _In_ BOOLEAN ShowHiddenAdapters
    )
{
    ULONG bufferLength = 0;
    PVOID buffer = NULL;

    ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    if (ShowHiddenAdapters && WindowsVersion >= WINDOWS_VISTA)
    {
        flags |= GAA_FLAG_INCLUDE_ALL_INTERFACES;
    }

    __try
    {
        if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &bufferLength) != ERROR_BUFFER_OVERFLOW)
            __leave;

        buffer = PhAllocate(bufferLength);
        memset(buffer, 0, bufferLength);

        if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &bufferLength) == ERROR_SUCCESS)
        {
            PIP_ADAPTER_ADDRESSES addressesBuffer = buffer;

            while (addressesBuffer)
            {
                //if (addressesBuffer->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                AddNetworkAdapterToListView(Context, addressesBuffer);
                addressesBuffer = addressesBuffer->Next;
            }
        }
    }
    __finally
    {
        if (buffer)
        {
            PhFree(buffer);
        }
    }
}

static INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_NETADAPTER_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_NETADAPTER_CONTEXT)PhAllocate(sizeof(PH_NETADAPTER_CONTEXT));
        memset(context, 0, sizeof(PH_NETADAPTER_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_NETADAPTER_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PPH_STRING string;
            PPH_LIST list;
            ULONG index;
            PVOID param;

            list = PhCreateList(2);
            index = -1;

            while ((index = PhFindListViewItemByFlags(
                context->ListViewHandle,
                index,
                LVNI_ALL
                )) != -1)
            {
                BOOL checked = ListView_GetItemState(context->ListViewHandle, index, LVIS_STATEIMAGEMASK) == ITEM_CHECKED;

                if (checked)
                {
                    if (PhGetListViewItemParam(context->ListViewHandle, index, &param))
                    {
                        PhAddItemList(list, param);
                    }
                }
            }

            ClearAdaptersList(NetworkAdaptersList);
            CopyAdaptersList(NetworkAdaptersList, list);
            PhDereferenceObject(context->NetworkAdaptersListEdited);

            string = SaveAdaptersList(NetworkAdaptersList);
            PhSetStringSetting2(SETTING_NAME_INTERFACE_LIST, &string->sr);
            PhDereferenceObject(string);

            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->NetworkAdaptersListEdited = PhCreateList(2);
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_NETADAPTERS_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 420, L"Network Adapters");
            PhSetExtendedListView(context->ListViewHandle);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHOW_HIDDEN_ADAPTERS), PhGetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS) ? BST_CHECKED : BST_UNCHECKED);

            ClearAdaptersList(context->NetworkAdaptersListEdited);
            CopyAdaptersList(context->NetworkAdaptersListEdited, NetworkAdaptersList);

            FindNetworkAdapters(context, PhGetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS) == 1 ? TRUE : FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_SHOW_HIDDEN_ADAPTERS:
                {
                    PhSetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS, Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHOW_HIDDEN_ADAPTERS)) == BST_CHECKED);

                    ListView_DeleteAllItems(context->ListViewHandle);

                    FindNetworkAdapters(context, PhGetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS) == 1 ? TRUE : FALSE);
                }
                break;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_NETADAPTER_OPTIONS),
        ParentHandle,
        OptionsDlgProc
        );
}
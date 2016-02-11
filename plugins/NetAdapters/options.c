/*
 * Process Hacker Plugins -
 *   Network Adapters Plugin
 *
 * Copyright (C) 2015-2016 dmex
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

static BOOLEAN OptionsChanged = FALSE;
#define ITEM_CHECKED (INDEXTOSTATEIMAGEMASK(2))
#define ITEM_UNCHECKED (INDEXTOSTATEIMAGEMASK(1))

static VOID ClearAdaptersList(
    _Inout_ PPH_LIST FilterList
    )
{
    for (ULONG i = 0; i < FilterList->Count; i++)
    {
        PhDereferenceObject(FilterList->Items[i]);
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

        newEntry = PhCreateObject(sizeof(PH_NETADAPTER_ENTRY), PhAdapterItemType);
        memset(newEntry, 0, sizeof(PH_NETADAPTER_ENTRY));

        PhInitializeCircularBuffer_ULONG64(&newEntry->InboundBuffer, PhGetIntegerSetting(L"SampleCount"));
        PhInitializeCircularBuffer_ULONG64(&newEntry->OutboundBuffer, PhGetIntegerSetting(L"SampleCount"));

        newEntry->InterfaceIndex = entry->InterfaceIndex;
        newEntry->InterfaceLuid = entry->InterfaceLuid;
        newEntry->InterfaceGuid = entry->InterfaceGuid;

        PhAddItemList(Destination, newEntry);
    }
}

VOID NetAdaptersLoadList(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhaGetStringSetting(SETTING_NAME_INTERFACE_LIST);
    remaining = settingsString->sr;

    if (remaining.Length == 0)
    {
        return;
    }

    while (remaining.Length != 0)
    {
        ULONG64 ifindex;
        ULONG64 luid64;
        PH_STRINGREF part1;
        PH_STRINGREF part2;
        PH_STRINGREF part3;
        PPH_NETADAPTER_ENTRY entry;

        if (remaining.Length == 0)
            break;

        entry = PhCreateObject(sizeof(PH_NETADAPTER_ENTRY), PhAdapterItemType);
        memset(entry, 0, sizeof(PH_NETADAPTER_ENTRY));

        PhSplitStringRefAtChar(&remaining, ',', &part1, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part2, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part3, &remaining);

        PhStringToInteger64(&part1, 10, &ifindex);
        PhStringToInteger64(&part2, 10, &luid64);

        entry->InterfaceIndex = (IF_INDEX)ifindex;
        entry->InterfaceLuid.Value = luid64;
        entry->InterfaceGuid = PhCreateString2(&part3);

        PhInitializeCircularBuffer_ULONG64(&entry->InboundBuffer, PhGetIntegerSetting(L"SampleCount"));
        PhInitializeCircularBuffer_ULONG64(&entry->OutboundBuffer, PhGetIntegerSetting(L"SampleCount"));

        PhAddItemList(NetworkAdaptersList, entry);
    }
}

static VOID SaveAdaptersList(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING settingsString;

    PhInitializeStringBuilder(&stringBuilder, 260);

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PPH_NETADAPTER_ENTRY entry = (PPH_NETADAPTER_ENTRY)NetworkAdaptersList->Items[i];

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%lu,%I64u,%s,",
            entry->InterfaceIndex,    // This value is UNSAFE and may change after reboot.
            entry->InterfaceLuid.Value, // This value is SAFE and does not change (Vista+).
            entry->InterfaceGuid->Buffer
            );
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
    PhSetStringSetting2(SETTING_NAME_INTERFACE_LIST, &settingsString->sr);
}

static VOID AddNetworkAdapterToListView(
    _In_ PPH_NETADAPTER_CONTEXT Context,
    _In_ PIP_ADAPTER_ADDRESSES Adapter
    )
{
    INT lvItemIndex;
    PPH_NETADAPTER_ENTRY entry;

    entry = PhCreateObject(sizeof(PH_NETADAPTER_ENTRY), PhAdapterItemType);
    memset(entry, 0, sizeof(PH_NETADAPTER_ENTRY));

    entry->InterfaceIndex = Adapter->IfIndex;
    entry->InterfaceLuid = Adapter->Luid;
    entry->InterfaceGuid = PhConvertMultiByteToUtf16(Adapter->AdapterName);

    lvItemIndex = PhAddListViewItem(
        Context->ListViewHandle,
        MAXINT,
        Adapter->Description,
        entry
        );

    for (ULONG i = 0; i < Context->NetworkAdaptersListEdited->Count; i++)
    {
        PPH_NETADAPTER_ENTRY currentEntry;
        
        currentEntry = (PPH_NETADAPTER_ENTRY)Context->NetworkAdaptersListEdited->Items[i];

        if (WindowsVersion > WINDOWS_XP)
        {
            if (entry->InterfaceLuid.Value == currentEntry->InterfaceLuid.Value)
            {
                ListView_SetItemState(Context->ListViewHandle, lvItemIndex, ITEM_CHECKED, LVIS_STATEIMAGEMASK);
                break;
            }
        }
        else
        {
            if (entry->InterfaceIndex == currentEntry->InterfaceIndex)
            {
                ListView_SetItemState(Context->ListViewHandle, lvItemIndex, ITEM_CHECKED, LVIS_STATEIMAGEMASK);
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
            for (PIP_ADAPTER_ADDRESSES i = buffer; i; i = i->Next)
            {
                //if (addressesBuffer->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                AddNetworkAdapterToListView(Context, i);
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
            if (context->OptionsChanged)
            {
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

                PhAcquireQueuedLockExclusive(&NetworkAdaptersListLock);
                ClearAdaptersList(NetworkAdaptersList);
                CopyAdaptersList(NetworkAdaptersList, list);
                PhReleaseQueuedLockExclusive(&NetworkAdaptersListLock);

                SaveAdaptersList();

                PhDereferenceObject(context->NetworkAdaptersListEdited);
            }

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
            
            PhAcquireQueuedLockShared(&NetworkAdaptersListLock);
            ClearAdaptersList(context->NetworkAdaptersListEdited);
            CopyAdaptersList(context->NetworkAdaptersListEdited, NetworkAdaptersList);
            PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

            FindNetworkAdapters(context, !!PhGetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS));

            context->OptionsChanged = FALSE;
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

                    FindNetworkAdapters(context, !!PhGetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS));
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & 0x3000)
                    {
                    case 0x2000: // checked
                    case 0x1000: // unchecked
                        context->OptionsChanged = TRUE;
                        break;
                    }
                }
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
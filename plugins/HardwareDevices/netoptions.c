/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

#include "devices.h"

#define ITEM_CHECKED (INDEXTOSTATEIMAGEMASK(2))
#define ITEM_UNCHECKED (INDEXTOSTATEIMAGEMASK(1))

static BOOLEAN FindAdapterEntry(
    _In_ PDV_NETADAPTER_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY currentEntry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!currentEntry)
            continue;

        found = EquivalentNetAdapterId(&currentEntry->Id, Id);

        if (found)
        {
            if (RemoveUserReference)
            {
                if (currentEntry->UserReference)
                {
                    PhDereferenceObjectDeferDelete(currentEntry);
                    currentEntry->UserReference = FALSE;
                }
            }

            PhDereferenceObjectDeferDelete(currentEntry);

            break;
        }
        else
        {
            PhDereferenceObjectDeferDelete(currentEntry);
        }
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    return found;
}

VOID NetAdaptersLoadList(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhaGetStringSetting(SETTING_NAME_INTERFACE_LIST);
    remaining = settingsString->sr;

    while (remaining.Length != 0)
    {
        ULONG64 ifindex;
        ULONG64 luid64;
        PH_STRINGREF part1;
        PH_STRINGREF part2;
        PH_STRINGREF part3;
        IF_LUID ifLuid;
        DV_NETADAPTER_ID id;
        PDV_NETADAPTER_ENTRY entry;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, ',', &part1, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part2, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part3, &remaining);

        PhStringToInteger64(&part1, 10, &ifindex);
        PhStringToInteger64(&part2, 10, &luid64);

        ifLuid.Value = luid64;
        InitializeNetAdapterId(&id, (IF_INDEX)ifindex, ifLuid, PhCreateString2(&part3));
        entry = CreateNetAdapterEntry(&id);
        DeleteNetAdapterId(&id);

        entry->UserReference = TRUE;
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
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (entry->UserReference)
        {
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%lu,%I64u,%s,",
                entry->Id.InterfaceIndex,    // This value is UNSAFE and may change after reboot.
                entry->Id.InterfaceLuid.Value, // This value is SAFE and does not change (Vista+).
                entry->Id.InterfaceGuid->Buffer
                );
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
    PhSetStringSetting2(SETTING_NAME_INTERFACE_LIST, &settingsString->sr);
}

static VOID AddNetworkAdapterToListView(
    _In_ PDV_NETADAPTER_CONTEXT Context,
    _In_ PIP_ADAPTER_ADDRESSES Adapter
    )
{
    DV_NETADAPTER_ID adapterId;
    INT lvItemIndex;
    BOOLEAN found = FALSE;
    PDV_NETADAPTER_ID newId = NULL;

    InitializeNetAdapterId(&adapterId, Adapter->IfIndex, Adapter->Luid, NULL);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (EquivalentNetAdapterId(&entry->Id, &adapterId))
        {
            newId = PhAllocate(sizeof(DV_NETADAPTER_ID));
            CopyNetAdapterId(newId, &entry->Id);

            if (entry->UserReference)
                found = TRUE;
        }

        PhDereferenceObjectDeferDelete(entry);

        if (newId)
            break;
    }

    if (!newId)
    {
        newId = PhAllocate(sizeof(DV_NETADAPTER_ID));
        CopyNetAdapterId(newId, &adapterId);
        PhMoveReference(&newId->InterfaceGuid, PhConvertMultiByteToUtf16(Adapter->AdapterName));
    }

    lvItemIndex = PhAddListViewItem(
        Context->ListViewHandle,
        MAXINT,
        Adapter->Description,
        newId
        );

    if (found)
        ListView_SetItemState(Context->ListViewHandle, lvItemIndex, ITEM_CHECKED, LVIS_STATEIMAGEMASK);

    DeleteNetAdapterId(&adapterId);
}

static VOID FindNetworkAdapters(
    _In_ PDV_NETADAPTER_CONTEXT Context,
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
            PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

            for (PIP_ADAPTER_ADDRESSES i = buffer; i; i = i->Next)
            {
                //if (addressesBuffer->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                AddNetworkAdapterToListView(Context, i);
            }

            PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
        }
    }
    __finally
    {
        if (buffer)
        {
            PhFree(buffer);
        }
    }

    // HACK: Remove all disconnected devices.
    BOOLEAN needsrefresh = FALSE;

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        ULONG index = -1;
        BOOLEAN found = FALSE;
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != -1)
        {
            PDV_NETADAPTER_ID param;

            if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
            {
                if (EquivalentNetAdapterId(param, &entry->Id))
                {
                    found = TRUE;
                }
            }
        }

        if (!found)
        {
            needsrefresh = TRUE;
            FindAdapterEntry(&entry->Id, TRUE);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    if (needsrefresh)
    {
        SaveAdaptersList();
    }
}

INT_PTR CALLBACK NetworkAdapterOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_CONTEXT)PhAllocate(sizeof(DV_NETADAPTER_CONTEXT));
        memset(context, 0, sizeof(DV_NETADAPTER_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_NETADAPTER_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            ULONG index = -1;

            while ((index = PhFindListViewItemByFlags(
                context->ListViewHandle,
                index,
                LVNI_ALL
                )) != -1)
            {
                PDV_NETADAPTER_ID param;

                if (PhGetListViewItemParam(context->ListViewHandle, index, &param))
                {
                    if (context->OptionsChanged)
                    {
                        BOOLEAN checked;

                        checked = ListView_GetItemState(context->ListViewHandle, index, LVIS_STATEIMAGEMASK) == ITEM_CHECKED;

                        if (checked)
                        {
                            if (!FindAdapterEntry(param, FALSE))
                            {
                                PDV_NETADAPTER_ENTRY entry;

                                entry = CreateNetAdapterEntry(param);
                                entry->UserReference = TRUE;
                            }
                        }
                        else
                        {
                            FindAdapterEntry(param, TRUE);
                        }
                    }

                    DeleteNetAdapterId(param);
                    PhFree(param);
                }
            }

            if (context->OptionsChanged)
                SaveAdaptersList();

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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_NETADAPTERS_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Network Adapters");
            PhSetExtendedListView(context->ListViewHandle);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHOW_HIDDEN_ADAPTERS), PhGetIntegerSetting(SETTING_NAME_ENABLE_HIDDEN_ADAPTERS) ? BST_CHECKED : BST_UNCHECKED);

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
/*
 * Process Hacker Extra Plugins -
 *   Disk Drives Plugin
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

static VOID FreeDiskDriveEntry(
    _In_ PPH_DISK_ENTRY Entry
    )
{
    PhSwapReference2(&Entry->DiskFriendlyName, NULL);
    PhSwapReference2(&Entry->DiskDevicePath, NULL);

    PhFree(Entry);
}

static  VOID ClearDiskDriveList(
    _Inout_ PPH_LIST FilterList
    )
{
    for (ULONG i = 0; i < FilterList->Count; i++)
    {
        FreeDiskDriveEntry((PPH_DISK_ENTRY)FilterList->Items[i]);
    }

    PhClearList(FilterList);
}

static VOID CopyDiskDriveList(
    _Inout_ PPH_LIST Destination,
    _In_ PPH_LIST Source
    )
{
    for (ULONG i = 0; i < Source->Count; i++)
    {
        PPH_DISK_ENTRY entry;
        PPH_DISK_ENTRY newEntry;

        newEntry = (PPH_DISK_ENTRY)PhAllocate(sizeof(PH_DISK_ENTRY));
        memset(newEntry, 0, sizeof(PH_DISK_ENTRY));
        
        entry = (PPH_DISK_ENTRY)Source->Items[i];

        newEntry->DiskFriendlyName = entry->DiskFriendlyName;
        newEntry->DiskDevicePath = entry->DiskDevicePath;

        PhAddItemList(Destination, newEntry);
    }
}

VOID LoadDiskDriveList(
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    )
{
    PH_STRINGREF remaining = String->sr;

    while (remaining.Length != 0)
    {
        PH_STRINGREF part1;
        PH_STRINGREF part2;
        PPH_DISK_ENTRY entry;

        entry = (PPH_DISK_ENTRY)PhAllocate(sizeof(PH_DISK_ENTRY));
        memset(entry, 0, sizeof(PH_DISK_ENTRY));

        PhSplitStringRefAtChar(&remaining, '|', &part1, &remaining);
        PhSplitStringRefAtChar(&remaining, '|', &part2, &remaining);

        entry->DiskFriendlyName = PhCreateStringEx(part1.Buffer, part1.Length);
        entry->DiskDevicePath = PhCreateStringEx(part2.Buffer, part2.Length);

        PhAddItemList(FilterList, entry);
    }
}

static PPH_STRING SaveDiskDriveList(
    _Inout_ PPH_LIST FilterList
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (SIZE_T i = 0; i < FilterList->Count; i++)
    {
        PPH_DISK_ENTRY entry = (PPH_DISK_ENTRY)FilterList->Items[i];

        PhAppendFormatStringBuilder(&stringBuilder, 
            L"%s|%s|",
            entry->DiskFriendlyName->Buffer,
            entry->DiskDevicePath->Buffer
            );
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static VOID AddDiskDriveToListView(
    _In_ PPH_DISKDRIVE_CONTEXT Context,
    _In_ PPH_STRING DiskDevicePath,
    _In_ PPH_STRING DiskFriendlyName
    )
{
    PPH_DISK_ENTRY entry;

    entry = (PPH_DISK_ENTRY)PhAllocate(sizeof(PH_DISK_ENTRY));
    memset(entry, 0, sizeof(PH_DISK_ENTRY));
    
    entry->DiskFriendlyName = DiskFriendlyName;
    entry->DiskDevicePath = DiskDevicePath;

    INT index = PhAddListViewItem(
        Context->ListViewHandle,
        MAXINT,
        DiskFriendlyName->Buffer,
        entry
        );

    for (ULONG i = 0; i < Context->DiskDrivesListEdited->Count; i++)
    {
        PPH_DISK_ENTRY currentEntry = (PPH_DISK_ENTRY)Context->DiskDrivesListEdited->Items[i];

        if (PhEqualString(currentEntry->DiskDevicePath, entry->DiskDevicePath, TRUE))
        {
            ListView_SetItemState(Context->ListViewHandle, index, ITEM_CHECKED, LVIS_STATEIMAGEMASK); 
            break;
        }
    }
}

static VOID NTAPI DiskDriveCallback(
    _In_ PWSTR DevicePath,
    _In_ PWSTR FriendlyName,
    _In_ PVOID Context
    )
{
    //HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    //PhCreateFileWin32(
    //    &deviceHandle,
    //    DevicePath,
    //    FILE_READ_ATTRIBUTES | SYNCHRONIZE,
    //    FILE_ATTRIBUTE_NORMAL,
    //    FILE_SHARE_READ | FILE_SHARE_WRITE,
    //    FILE_OPEN,
    //    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
    //    );
    //if (deviceHandle != INVALID_HANDLE_VALUE)
    //if (NT_SUCCESS(DiskDriveQueryDeviceInformation(deviceHandle, NULL, &diskModel, NULL, &diskSerial)))
    //if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(deviceHandle, NULL, NULL)))
     
    AddDiskDriveToListView(Context, PhCreateString(DevicePath), PhCreateString(FriendlyName));    
}

static INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_DISKDRIVE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_DISKDRIVE_CONTEXT)PhAllocate(sizeof(PH_DISKDRIVE_CONTEXT));
        memset(context, 0, sizeof(PH_DISKDRIVE_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_DISKDRIVE_CONTEXT)GetProp(hwndDlg, L"Context");

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

            ClearDiskDriveList(DiskDrivesList);
            CopyDiskDriveList(DiskDrivesList, list);
            PhDereferenceObject(context->DiskDrivesListEdited);

            string = SaveDiskDriveList(DiskDrivesList);
            PhSetStringSetting2(SETTING_NAME_DISK_LIST, &string->sr);
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
            context->DiskDrivesListEdited = PhCreateList(2);
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DISKDRIVE_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 410, L"Disk");
            PhSetExtendedListView(context->ListViewHandle);

            ClearDiskDriveList(context->DiskDrivesListEdited);
            CopyDiskDriveList(context->DiskDrivesListEdited, DiskDrivesList);

            EnumerateDiskDrives(DiskDriveCallback, context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
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
        MAKEINTRESOURCE(IDD_DISKDRIVE_OPTIONS),
        ParentHandle,
        OptionsDlgProc
        );
}
/*
 * Process Hacker Extra Plugins -
 *   Boot Entries Plugin
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

#include <stdint.h>
#include "Efi\EfiTypes.h"
#include "Efi\EfiDevicePath.h"

static _NtAddBootEntry NtAddBootEntry_I = NULL;
static _NtDeleteBootEntry NtDeleteBootEntry_I = NULL;
static _NtModifyBootEntry NtModifyBootEntry_I = NULL;
static _NtEnumerateBootEntries NtEnumerateBootEntries_I = NULL;
static _NtQueryBootEntryOrder NtQueryBootEntryOrder_I = NULL;
static _NtSetBootEntryOrder NtSetBootEntryOrder_I = NULL;
static _NtQueryBootOptions NtQueryBootOptions_I = NULL;
static _NtSetBootOptions NtSetBootOptions_I = NULL;
static _NtTranslateFilePath NtTranslateFilePath_I = NULL;

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

static BOOLEAN BootEntryCallback(
    _In_ PBOOT_ENTRY BootEntry,
    _In_ PVOID Context
    )
{
    HWND listViewHandle = (HWND)Context;
    PWSTR bootEntryName = (PWSTR)((PBYTE)BootEntry + BootEntry->FriendlyNameOffset);
    PFILE_PATH bootFilePath = (PFILE_PATH)((PBYTE)BootEntry + BootEntry->BootFilePathOffset);

    PPH_STRING bootEntryID = PhFormatString(L"%lu", BootEntry->Id);
    INT index = PhAddListViewItem(
        listViewHandle,
        MAXINT,
        bootEntryID->Buffer,
        bootEntryID->Buffer
        );
    PhDereferenceObject(bootEntryID);

    PhSetListViewSubItem(
        listViewHandle,
        index,
        1,
        bootEntryName
        );

    if (bootFilePath->Type == FILE_PATH_TYPE_EFI)
    {
        EFI_DEV_PATH* efiDevPath = (EFI_DEV_PATH*)bootFilePath->FilePath;
        EFI_DEVICE_PATH_PROTOCOL efiDevPathProtocol = efiDevPath->DevPath;

        if (efiDevPathProtocol.Type == MEDIA_DEVICE_PATH)
        {
            //HARDDRIVE_DEVICE_PATH hardDriveDevicePath = efiDevPath->HardDrive;

            //PPH_STRING partitionLength = PhFormatSize(hardDriveDevicePath.PartitionSize, -1);
            //if (hardDriveDevicePath.SignatureType == SIGNATURE_TYPE_GUID)
            //PPH_STRING guidString = PhFormatGuid(hardDriveDevicePath.Signature);

            PhSetListViewSubItem(listViewHandle, index, 2, L"MEDIA_DEVICE_PATH");
        }
        else if (efiDevPathProtocol.Type == ACPI_DEVICE_PATH)
        {
            if (efiDevPathProtocol.SubType == ACPI_DP)
            {
                //ACPI_HID_DEVICE_PATH vendorPath1 = efiDevPath->Acpi;
            }
            else if (efiDevPathProtocol.SubType == ACPI_EXTENDED_DP)
            {
                //ACPI_EXTENDED_HID_DEVICE_PATH extendedAcpi = efiDevPath->ExtendedAcpi;
            }
            else if (efiDevPathProtocol.SubType == ACPI_ADR_DP)
            {
                //ACPI_ADR_DEVICE_PATH adrAcpi = efiDevPath->AdrAcpi;
            }
                   
            PhSetListViewSubItem(listViewHandle, index, 2, L"ACPI_DEVICE_PATH");
        }
        else if (efiDevPathProtocol.Type == BBS_DEVICE_PATH)
        {
            if (efiDevPathProtocol.SubType == BBS_BBS_DP)
            {
                // BIOS Boot Specification
                //BBS_BBS_DEVICE_PATH vendorPath = efiDevPath->Bbs;

                //union
                //{
                //    USHORT StatusFlag;
                //    struct
                //    {
                //        USHORT OldPosition : 3;    // This entry’s index in the table at the last boot. For updating the IPL or BCV Priority if individual device detection is done.
                //        USHORT Reserved1 : 7;      // Reserved for future use, must be zero.
                //        USHORT Enabled : 8;        // 0 = Entry will be ignored for booting (IPL); entry will not be called for boot connection (BCV). 1 = Entry will be attempted for booting (IPL); entry will be called for boot connection (BCV).
                //        USHORT Failed : 9;         // 0 = Has not been attempted for boot, or it is unknown if boot failure occurred (IPL); entry connected successfully (BCV). 1 = Failed boot attempt (IPL); failed connection attempt (BCV).
                //        USHORT MediaPresent : 11;  // 0 = No bootable media present in the device. 1 = Unknown if bootable media present. 2 = Media present and appears bootable. 3 = Reserved for future use.
                //        USHORT Reserved2 : 15;     // Reserved for future use, must be zero.
                //    };
                //} DeviceStatusFlag = { vendorPath.StatusFlag };

                //if (vendorPath.DeviceType == BBS_TYPE_HARDDRIVE) { }
            }

            PhSetListViewSubItem(listViewHandle, index, 2, L"BIOS Boot Specification");
        }
        else
        {
            PhSetListViewSubItem(listViewHandle, index, 2, L"[Unknown]");
        }
    }

    return TRUE;
}

static NTSTATUS PhEnumerateBootEntries(    
    _In_ PPH_BOOT_ENTRY_CALLBACK BootEntryCallback,
    _In_opt_ PVOID Context
    )
{
    ULONG bufferLength = 0;
    PVOID buffer = NULL;

    __try
    {
        if (!NtEnumerateBootEntries_I)
            __leave;

        if (NtEnumerateBootEntries_I(NULL, &bufferLength) != STATUS_BUFFER_TOO_SMALL)
            __leave;

        buffer = PhAllocate(bufferLength);

        if (NT_SUCCESS(NtEnumerateBootEntries_I(buffer, &bufferLength)))
        {
            PBOOT_ENTRY_LIST bootEntryList = buffer;

            do
            {
                if (!BootEntryCallback(&bootEntryList->BootEntry, Context))
                    break;

            } while (bootEntryList->NextEntryOffset && (bootEntryList = (PBOOT_ENTRY_LIST)PTR_ADD_OFFSET(bootEntryList, bootEntryList->NextEntryOffset)));
        }
    }
    __finally
    {
        if (buffer)
        {
            PhFree(buffer);
        }
    }

    return STATUS_SUCCESS;
}

static VOID QueryBootOptions(
    VOID
    )
{
    ULONG bufferLength = 0;
    PVOID buffer = NULL;

    __try
    {
        if (NtQueryBootOptions_I(NULL, &bufferLength) != STATUS_BUFFER_TOO_SMALL)
            __leave;

        buffer = PhAllocate(bufferLength);

        if (NT_SUCCESS(NtQueryBootOptions_I(buffer, &bufferLength)))
        {
            PBOOT_OPTIONS bootOptions = buffer;
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

static NTSTATUS EnumerateBootEntriesThread(
    _In_ PVOID ThreadParam
    )
{
    HANDLE tokenHandle;

    // Enable required privileges
    if (NT_SUCCESS(PhOpenProcessToken(
        &tokenHandle,
        TOKEN_ADJUST_PRIVILEGES,
        NtCurrentProcess()
        )))
    {
        PhSetTokenPrivilege(tokenHandle, SE_SYSTEM_ENVIRONMENT_NAME, NULL, SE_PRIVILEGE_ENABLED);
        NtClose(tokenHandle);
    }

    //QueryBootOptions();
    PhEnumerateBootEntries(BootEntryCallback, ThreadParam);

    return STATUS_SUCCESS;
}

static BOOLEAN IsLegacySystem(
    VOID
    )
{
    // Windows 8
    //FIRMWARE_TYPE fwtype;
    //GetFirmwareType(&fwtype);
    //if (fwtype == FirmwareTypeUefi)
    //{
    //    return TRUE;
    //}

    if (GetFirmwareEnvironmentVariable(L"", L"{00000000-0000-0000-0000-000000000000}", NULL, 0) == ERROR_INVALID_FUNCTION)
    {
        return TRUE;
    }

    return FALSE;
}

static PPH_STRING PhGetSelectedListViewItemText(
    _In_ HWND hWnd
    )
{
    INT index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        WCHAR textBuffer[MAX_PATH] = L"";

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = textBuffer;
        item.cchTextMax = _countof(textBuffer);

        if (ListView_GetItem(hWnd, &item))
            return PhCreateString(textBuffer);
    }

    return NULL;
}

static VOID ShowBootEntryMenu(
    _In_ PBOOT_WINDOW_CONTEXT Context,
    _In_ HWND hwndDlg
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG id;
    POINT cursorPos = { 0, 0 };

    PPH_STRING bootEntryName = PhGetSelectedListViewItemText(Context->ListViewHandle);

    if (bootEntryName)
    {
        GetCursorPos(&cursorPos);

        menu = LoadMenu(
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDR_BOOT_ENTRY_MENU)
            );

        subMenu = GetSubMenu(menu, 0);

        id = (ULONG)TrackPopupMenu(
            subMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
            cursorPos.x,
            cursorPos.y,
            0,
            hwndDlg,
            NULL
            );

        DestroyMenu(menu);

        switch (id)
        {
        case ID_MENU_MOVEUP:
            {
                //INT lvItemIndex = PhFindListViewItemByFlags(
                //    Context->ListViewHandle,
                //    -1,
                //    LVNI_SELECTED
                //    );
                //
                //if (lvItemIndex != -1)
                //{
                //    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                //        hwndDlg,
                //        L"remove",
                //        bootEntryName->Buffer,
                //        NULL,
                //        FALSE
                //        ))
                //    {
                //        if (NtDeleteBootEntry_I && NtDeleteBootEntry_I(0))
                //        {
                //            ListView_DeleteItem(Context->ListViewHandle, lvItemIndex);
                //        }
                //    }
                //}
            }
            break;
        }

        PhDereferenceObject(bootEntryName);
    }
}

static INT_PTR CALLBACK BootEntriesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PBOOT_WINDOW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PBOOT_WINDOW_CONTEXT)PhAllocate(sizeof(BOOT_WINDOW_CONTEXT));
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PBOOT_WINDOW_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);
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
            HANDLE threadHandle;

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_BOOT_LIST);

            PhRegisterDialog(hwndDlg);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 30, L"ID");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 140, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 340, L"Name");
            PhSetExtendedListView(context->ListViewHandle);

            ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            //ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, PhpObjectProcessCompareFunction);
            //ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PhpObjectProcessCompareFunction);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_BOOT_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            if (threadHandle = PhCreateThread(0, EnumerateBootEntriesThread, context->ListViewHandle))
            {
                NtClose(threadHandle);
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BOOT_REFRESH:
                {
                    HANDLE threadHandle;

                    ListView_DeleteAllItems(context->ListViewHandle);

                    if (threadHandle = PhCreateThread(0, EnumerateBootEntriesThread, context->ListViewHandle))
                    {
                        NtClose(threadHandle);
                    }
                }
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
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
                        ShowBootEntryMenu(context, hwndDlg);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}


static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NtAddBootEntry_I = PhGetModuleProcAddress(L"ntdll.dll", "NtAddBootEntry");
    NtDeleteBootEntry_I = PhGetModuleProcAddress(L"ntdll.dll", "NtDeleteBootEntry");
    NtModifyBootEntry_I = PhGetModuleProcAddress(L"ntdll.dll", "NtModifyBootEntry");
    NtEnumerateBootEntries_I = PhGetModuleProcAddress(L"ntdll.dll", "NtEnumerateBootEntries");
    NtQueryBootEntryOrder_I = PhGetModuleProcAddress(L"ntdll.dll", "NtQueryBootEntryOrder");
    NtSetBootEntryOrder_I = PhGetModuleProcAddress(L"ntdll.dll", "NtSetBootEntryOrder");
    NtQueryBootOptions_I = PhGetModuleProcAddress(L"ntdll.dll", "NtQueryBootOptions");
    NtSetBootOptions_I = PhGetModuleProcAddress(L"ntdll.dll", "NtSetBootOptions");
    NtTranslateFilePath_I = PhGetModuleProcAddress(L"ntdll.dll", "NtTranslateFilePath");
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

static VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case BOOT_ENTRIES_MENUITEM:
        {
            if (!PhElevated)
            {
                PhShowInformation(menuItem->OwnerWindow, L"This option requires elevation.");
                break;
            }

            DialogBox(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_BOOT),
                NULL,
                BootEntriesDlgProc
                );
        }
        break;
    }
}

static VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", BOOT_ENTRIES_MENUITEM, L"Boot Entries", NULL);
}

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
            PH_SETTING_CREATE settings[] =
            {
                { IntegerPairSettingType, SETTING_NAME_WINDOW_POSITION, L"100,100" },
                { IntegerPairSettingType, SETTING_NAME_WINDOW_SIZE, L"490,340" },
                { StringSettingType, SETTING_NAME_LISTVIEW_COLUMNS, L"" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"EFI Boot Entries Plugin";
            info->Author = L"dmex";
            info->Description = L"Plugin for viewing native EFI Boot Entries via the Tools menu.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            PhAddSettings(settings, _countof(settings));
        }
        break;
    }

    return TRUE;
}
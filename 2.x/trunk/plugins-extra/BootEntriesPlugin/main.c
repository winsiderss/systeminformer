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

PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

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

            info->DisplayName = L"Boot Entries Plugin";
            info->Author = L"dmex";
            info->Description = L"Plugin for viewing native Boot Entries via the Tools menu.";
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
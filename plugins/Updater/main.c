/*
 * Process Hacker Update Checker -
 *   main program
 *
 * Copyright (C) 2011-2012 dmex
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
 *x
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "updater.h"

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

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.UpdateChecker", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Update Checker";
            info->Author = L"dmex";
            info->Description = L"Plugin for checking new Process Hacker releases via the Help menu.";
            info->HasOptions = TRUE;
    
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
             PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

             {
                 PH_SETTING_CREATE settings[] =
                 {
                     { IntegerSettingType, SETTING_ENABLE_CACHE, L"1" },
                     { IntegerSettingType, SETTING_AUTO_CHECK, L"1" },
                 };

                 PhAddSettings(settings, ARRAYSIZE(settings));
             }
        }
        break;
    }

    return TRUE;
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Add our menu item, 4 = Help menu.
    PhPluginAddMenuItem(PluginInstance, 4, NULL, UPDATE_MENUITEM, L"Check for Updates", NULL);

    if (PhGetIntegerSetting(SETTING_AUTO_CHECK))
    {
        // Queue up our initial update check.
        StartInitialCheck();
    }
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    if (menuItem != NULL)
    {
        switch (menuItem->Id)
        {
        case UPDATE_MENUITEM:
            {
                ShowDialog();
            }
            break;
        }
    }
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}
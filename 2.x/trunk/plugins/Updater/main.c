/*
 * Process Hacker Update Checker - 
 *   main program
 * 
 * Copyright (C) 2011 dmex
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
#define MAIN_PRIVATE
#include "updater.h"
#include "resource.h"

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

            info->DisplayName = L"Updater";
            info->Author = L"dmex";
            info->Description = L"Process Hacker Update Checker.";
			info->Url = L"http://processhacker.sourceforge.net/forums/viewtopic.php?f=18&t=273";
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

			  /*static PH_SETTING_CREATE settings[] =
                {
                    { IntegerSettingType, L"ProcessHacker.Updater.EnableCache", L"1" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));*/
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
    PhPluginAddMenuItem(PluginInstance, 4, NULL, UPDATE_MENUITEM, L"Check for Update", NULL);
}

VOID NTAPI MenuItemCallback(
	__in_opt PVOID Parameter,
	__in_opt PVOID Context
	)
{
	PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

	switch (menuItem->Id)
	{
	case UPDATE_MENUITEM:
		{
			DialogBox(
				(HINSTANCE)PluginInstance->DllBase,
				MAKEINTRESOURCE(IDD_OUTPUT),
				PhMainWndHandle,
				NetworkOutputDlgProc
				);
		}
		break;
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
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
            info->HasOptions = FALSE;
			
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
        }
        break;
    }

    return TRUE;
}

//INT VersionParser(char* version1, char* version2) 
//{
//	INT i = 0, a1 = 0, b1 = 0, ret = 0;
//	size_t a = strlen(version1); 
//	size_t b = strlen(version2);
//
//	if (b > a) 
//		a = b;
//
//	for (i = 0; i < a; i++) 
//	{
//		a1 += version1[i];
//		b1 += version2[i];
//	}
//
//	if (b1 > a1)
//	{
//		// second version is fresher
//		ret = 1; 
//	}
//	else if (b1 == a1) 
//	{
//		// versions is equal
//		ret = 0;
//	}
//	else 
//	{
//		// first version is fresher
//		ret = -1; 
//	}
//
//	return ret;
//}

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
				PluginInstance->DllBase,
				MAKEINTRESOURCE(IDD_OUTPUT),
				PhMainWndHandle,
				NetworkOutputDlgProc
				);
		}
		break;
    }
}
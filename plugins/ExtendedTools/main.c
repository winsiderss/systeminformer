/*
 * Process Hacker Extended Tools - 
 *   main program
 * 
 * Copyright (C) 2010 wj32
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

#include "extendedtools.h"
#include "resource.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI HandlePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION HandlePropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;

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
            PH_PLUGIN_INFORMATION info;

            info.DisplayName = L"Extended Tools";
            info.Author = L"wj32";
            info.Description = L"Extended functionality for Windows Vista and above.";
            info.HasOptions = FALSE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ExtendedTools", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
            //    LoadCallback,
            //    NULL,
            //    &PluginLoadCallbackRegistration
            //    );
            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
            //    ShowOptionsCallback,
            //    NULL,
            //    &PluginShowOptionsCallbackRegistration
            //    );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            //PhRegisterCallback(
            //    PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
            //    MainWindowShowingCallback,
            //    NULL,
            //    &MainWindowShowingCallbackRegistration
            //    );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackHandlePropertiesInitializing),
                HandlePropertiesInitializingCallback,
                NULL,
                &HandlePropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
                ProcessMenuInitializingCallback,
                NULL,
                &ProcessMenuInitializingCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case ID_PROCESS_UNLOADEDMODULES:
        {
            EtShowUnloadedDllsDialog(PhMainWndHandle, menuItem->Context);
        }
        break;
    }
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI HandlePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EtHandlePropertiesInitializing(Parameter);
}

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_EMENU_ITEM miscMenu;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    miscMenu = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0);

    if (miscMenu)
    {
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_PROCESS_UNLOADEDMODULES, L"Unloaded Modules", processItem), -1);
    }
}

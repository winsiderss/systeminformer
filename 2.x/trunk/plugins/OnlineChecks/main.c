/*
 * Process Hacker Online Checks -
 *   main program
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2012 dmex
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
#include "onlnchk.h"
#include "resource.h"

PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI ModuleMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

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

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.OnlineChecks", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Online Checks";
            info->Author = L"dmex";
            info->Description = L"Allows files to be checked with online services.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
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
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
                ProcessMenuInitializingCallback,
                NULL,
                &ProcessMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing),
                ModuleMenuInitializingCallback,
                NULL,
                &ModuleMenuInitializingCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    //PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, NULL, UPLOAD_SERVICE_VIRUSTOTAL, L"Upload file to VirusTotal", NULL);
    //PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, NULL, UPLOAD_SERVICE_JOTTI, L"Upload file to JOTTI", NULL);
    //PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, NULL, UPLOAD_SERVICE_CIMA, L"Upload file to Comodo", NULL);
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case UPLOAD_SERVICE_VIRUSTOTAL:
        {
            if (menuItem->Context)
            {
                UploadToOnlineService(
                    menuItem->OwnerWindow,
                    menuItem->Context ? menuItem->Context : NULL,
                    UPLOAD_SERVICE_VIRUSTOTAL
                    );
            }
        }
        break;
    case UPLOAD_SERVICE_JOTTI:
        {
            if (menuItem->Context)
            {
                UploadToOnlineService(
                    menuItem->OwnerWindow,
                    menuItem->Context ? menuItem->Context : NULL,
                    UPLOAD_SERVICE_JOTTI
                    );
            }
        }
        break;
    case UPLOAD_SERVICE_CIMA:
        {
            if (menuItem->Context)
            {
                UploadToOnlineService(
                    menuItem->OwnerWindow,
                    menuItem->Context ? menuItem->Context : NULL,
                    UPLOAD_SERVICE_CIMA
                    );
            }
        }
        break;
    }
}

PPH_EMENU_ITEM CreateSendToMenu(
    __in PPH_EMENU_ITEM Parent,
    __in PWSTR InsertAfter,
    __in PPH_STRING FileName
    )
{
    PPH_EMENU_ITEM sendToMenu;
    PPH_EMENU_ITEM menuItem;
    ULONG insertIndex;

    // Create the Send To menu.
    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Send To", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, UPLOAD_SERVICE_VIRUSTOTAL, L"virustotal.com", FileName), -1);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, UPLOAD_SERVICE_JOTTI, L"virusscan.jotti.org", FileName), -1);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, UPLOAD_SERVICE_CIMA, L"camas.comodo.com", FileName), -1);

    menuItem = PhFindEMenuItem(Parent, PH_EMENU_FIND_STARTSWITH, InsertAfter, 0);

    if (menuItem)
        insertIndex = PhIndexOfEMenuItem(Parent, menuItem);
    else
        insertIndex = -1;

    PhInsertEMenuItem(Parent, sendToMenu, insertIndex + 1);

    return sendToMenu;
}

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_EMENU_ITEM sendToMenu;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    sendToMenu = CreateSendToMenu(menuInfo->Menu, L"Search Online", processItem ? processItem->FileName : NULL);

    // Only enable the Send To menu if there is exactly one process selected and it has a file name.
    if (!processItem || !processItem->FileName)
        sendToMenu->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ModuleMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_MODULE_ITEM moduleItem;
    PPH_EMENU_ITEM sendToMenu;

    if (menuInfo->u.Module.NumberOfModules == 1)
        moduleItem = menuInfo->u.Module.Modules[0];
    else
        moduleItem = NULL;

    sendToMenu = CreateSendToMenu(menuInfo->Menu, L"Search Online", moduleItem ? moduleItem->FileName : NULL);

    if (!moduleItem || !moduleItem->FileName)
        sendToMenu->Flags |= PH_EMENU_DISABLED;
}
/*
 * Process Hacker Online Checks -
 *   Main Program
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2016 dmex
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

#include "onlnchk.h"

PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;

static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    // Nothing
}

static VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    // Nothing
}

static VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;
    PPH_STRING fileName;

    switch (menuItem->Id)
    {
    case ID_SENDTO_SERVICE1:
        fileName = menuItem->Context;
        UploadToOnlineService(fileName, UPLOAD_SERVICE_VIRUSTOTAL);
        break;
    case ID_SENDTO_SERVICE2:
        fileName = menuItem->Context;
        UploadToOnlineService(fileName, UPLOAD_SERVICE_JOTTI);
        break;
    case ID_SENDTO_SERVICE3:
        fileName = menuItem->Context;
        UploadToOnlineService(fileName, UPLOAD_SERVICE_CIMA);
        break;
    }
}

static PPH_EMENU_ITEM CreateSendToMenu(
    _In_ PPH_EMENU_ITEM Parent,
    _In_ PWSTR InsertAfter,
    _In_ PPH_STRING FileName
    )
{
    PPH_EMENU_ITEM sendToMenu;
    PPH_EMENU_ITEM menuItem;
    ULONG insertIndex;

    // Create the Send To menu.
    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Send To", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_SENDTO_SERVICE1, L"virustotal.com", FileName), -1);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_SENDTO_SERVICE2, L"virusscan.jotti.org", FileName), -1);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_SENDTO_SERVICE3, L"camas.comodo.com", FileName), -1);

    menuItem = PhFindEMenuItem(Parent, PH_EMENU_FIND_STARTSWITH, InsertAfter, 0);

    if (menuItem)
        insertIndex = PhIndexOfEMenuItem(Parent, menuItem);
    else
        insertIndex = -1;

    PhInsertEMenuItem(Parent, sendToMenu, insertIndex + 1);

    return sendToMenu;
}

static VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
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
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
    }
}

static VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
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

    if (!moduleItem)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
    }
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

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Online Checks";
            info->Author = L"dmex, wj32";
            info->Description = L"Allows files to be checked with online services.";
            info->Url = L"http://processhacker.sf.net/forums/viewtopic.php?t=1118";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
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
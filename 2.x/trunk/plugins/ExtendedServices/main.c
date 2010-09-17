/*
 * Process Hacker Extended Services - 
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

#include <phdk.h>
#define MAIN_PRIVATE
#include "extsrv.h"
#include "resource.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ServicePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION ServicePropertiesInitializingCallbackRegistration;

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

            info.DisplayName = L"Extended Services";
            info.Author = L"wj32";
            info.Description = L"Adds more tabs to service properties.";
            info.HasOptions = FALSE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ExtendedServices", Instance, &info);
            
            if (!PluginInstance)
                return FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializaing),
                ServicePropertiesInitializingCallback,
                NULL,
                &ServicePropertiesInitializingCallbackRegistration
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

VOID NTAPI ServicePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PROPSHEETPAGE propSheetPage;
    PPH_SERVICE_ITEM serviceItem;

    serviceItem = objectProperties->Parameter;

    // Recovery
    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.lParam = (LPARAM)serviceItem;

        if (!(serviceItem->Flags & SERVICE_RUNS_IN_SYSTEM_PROCESS))
        {
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVRECOVERY);
            propSheetPage.pfnDlgProc = EspServiceRecoveryDlgProc;
        }
        else
        {
            // Services which run in system processes don't support failure actions.
            // Create a different page with a message saying this.
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVRECOVERY2);
            propSheetPage.pfnDlgProc = EspServiceRecovery2DlgProc;
        }

        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    // Dependencies
    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVLIST);
        propSheetPage.pszTitle = L"Dependencies";
        propSheetPage.pfnDlgProc = EspServiceDependenciesDlgProc;
        propSheetPage.lParam = (LPARAM)serviceItem;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    // Dependents
    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVLIST);
        propSheetPage.pszTitle = L"Dependents";
        propSheetPage.pfnDlgProc = EspServiceDependentsDlgProc;
        propSheetPage.lParam = (LPARAM)serviceItem;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }
}

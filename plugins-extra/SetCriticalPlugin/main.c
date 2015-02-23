/*
 * Process Hacker Extra Plugins -
 *   Average CPU Plugin
 *
 * Copyright (C) 2012 wj32
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
#include "resource.h"

#define CRITICAL_MENU_ITEM 1

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION MenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;

VOID MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case CRITICAL_MENU_ITEM:
        {
            NTSTATUS status;
            PPH_PROCESS_ITEM processItem = menuItem->Context;
            HANDLE processHandle;
            ULONG breakOnTermination;

            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION, processItem->ProcessId)))
            {
                if (NT_SUCCESS(status = NtQueryInformationProcess(processHandle, ProcessBreakOnTermination, &breakOnTermination, sizeof(ULONG), NULL)))
                {
                    if (!breakOnTermination && PhShowConfirmMessage(
                        menuItem->OwnerWindow,
                        L"enable",
                        L"critical status on the process",
                        L"If the process ends, the operating system will shut down immediately.",
                        TRUE
                        ))
                    {
                        breakOnTermination = TRUE;
                        status = NtSetInformationProcess(processHandle, ProcessBreakOnTermination, &breakOnTermination, sizeof(ULONG));
                    }
                    else if (breakOnTermination && PhShowConfirmMessage(
                        menuItem->OwnerWindow,
                        L"disable",
                        L"critical status on the process",
                        NULL,
                        FALSE
                        ))
                    {
                        breakOnTermination = FALSE;
                        status = NtSetInformationProcess(processHandle, ProcessBreakOnTermination, &breakOnTermination, sizeof(ULONG));
                    }

                    if (!NT_SUCCESS(status))
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to set critical status", status, 0);
                }
                else
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to query critical status", status, 0);
                }

                NtClose(processHandle);
            }
            else
            {
                PhShowStatus(menuItem->OwnerWindow, L"Unable to open the process", status, 0);
            }
        }
        break;
    }
}

VOID ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM miscMenuItem;
    PPH_EMENU_ITEM criticalMenuItem;
    PPH_PROCESS_ITEM processItem;

    miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0);

    if (!miscMenuItem)
        return;

    processItem = menuInfo->u.Process.NumberOfProcesses == 1 ? menuInfo->u.Process.Processes[0] : NULL;
    criticalMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, CRITICAL_MENU_ITEM, L"Critical", processItem);
    PhInsertEMenuItem(miscMenuItem, criticalMenuItem, 0);

    if (processItem)
    {
        HANDLE processHandle;
        ULONG breakOnTermination;

        if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId)))
        {
            if (NT_SUCCESS(NtQueryInformationProcess(processHandle, ProcessBreakOnTermination, &breakOnTermination, sizeof(ULONG), NULL)))
            {
                if (breakOnTermination)
                    criticalMenuItem->Flags |= PH_EMENU_CHECKED;
            }

            NtClose(processHandle);
        }
    }
    else
    {
        criticalMenuItem->Flags |= PH_EMENU_DISABLED;
    }
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        PPH_PLUGIN_INFORMATION info;

        PluginInstance = PhRegisterPlugin(L"Wj32.SetCriticalPlugin", Instance, &info);

        if (!PluginInstance)
            return FALSE;

        info->DisplayName = L"Set process critical status plugin";
        info->Description = L"Adds Miscellaneous > Critical menu item.";
        info->Author = L"wj32";

        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
            MenuItemCallback, NULL, &MenuItemCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
            ProcessMenuInitializingCallback, NULL, &ProcessMenuInitializingCallbackRegistration);
    }

    return TRUE;
}

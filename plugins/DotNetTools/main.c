/*
 * Process Hacker .NET Tools -
 *   main program
 *
 * Copyright (C) 2011 wj32
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

#include "dn.h"
#include "resource.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI UnloadCallback(
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

VOID NTAPI TreeNewMessageCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessPropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ThreadMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ModuleMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ThreadStackControlCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginTreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadStackControlCallbackRegistration;

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

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.DotNetTools", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L".NET Tools";
            info->Author = L"wj32";
            info->Description = L"Adds .NET performance counters, assembly information, thread stack support, and more.";
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
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
            //    MenuItemCallback,
            //    NULL,
            //    &PluginMenuItemCallbackRegistration
            //    );
            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
            //    TreeNewMessageCallback,
            //    NULL,
            //    &PluginTreeNewMessageCallbackRegistration
            //    );

            //PhRegisterCallback(
            //    PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
            //    MainWindowShowingCallback,
            //    NULL,
            //    &MainWindowShowingCallbackRegistration
            //    );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing),
                ProcessPropertiesInitializingCallback,
                NULL,
                &ProcessPropertiesInitializingCallbackRegistration
                );
            //PhRegisterCallback(
            //    PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
            //    ProcessMenuInitializingCallback,
            //    NULL,
            //    &ProcessMenuInitializingCallbackRegistration
            //    );
            //PhRegisterCallback(
            //    PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing),
            //    ThreadMenuInitializingCallback,
            //    NULL,
            //    &ThreadMenuInitializingCallbackRegistration
            //    );
            //PhRegisterCallback(
            //    PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing),
            //    ModuleMenuInitializingCallback,
            //    NULL,
            //    &ModuleMenuInitializingCallbackRegistration
            //    );
            //PhRegisterCallback(
            //    PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
            //    ProcessTreeNewInitializingCallback,
            //    NULL,
            //    &ProcessTreeNewInitializingCallbackRegistration
            //    );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadStackControl),
                ThreadStackControlCallback,
                NULL,
                &ThreadStackControlCallbackRegistration
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { StringSettingType, SETTING_NAME_ASM_TREE_LIST_COLUMNS, L"" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            }
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
    NOTHING;
}

VOID NTAPI UnloadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    default:
        NOTHING;
        break;
    }
}

VOID NTAPI TreeNewMessageCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    NOTHING;
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;
    BOOLEAN isDotNet;

    if (NT_SUCCESS(PhGetProcessIsDotNet(propContext->ProcessItem->ProcessId, &isDotNet)) && isDotNet)
    {
        if (WindowsVersion >= WINDOWS_VISTA)
            AddAsmPageToPropContext(propContext);
        AddPerfPageToPropContext(propContext);

        if (!propContext->ProcessItem->IsDotNet)
            propContext->ProcessItem->UpdateIsDotNet = TRUE; // force a refresh
    }
}

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ThreadMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ModuleMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ThreadStackControlCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessThreadStackControl(Parameter);
}

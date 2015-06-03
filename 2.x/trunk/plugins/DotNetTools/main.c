/*
 * Process Hacker .NET Tools -
 *   main program
 *
 * Copyright (C) 2011-2015 wj32
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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhSvcRequestCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ThreadTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ThreadTreeNewUninitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ThreadStackControlCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ThreadItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

VOID NTAPI ThreadItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginTreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginPhSvcRequestCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadTreeNewUninitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadStackControlCallbackRegistration;

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

            info->DisplayName = L".NET Tools";
            info->Author = L"wj32";
            info->Description = L"Adds .NET performance counters, assembly information, thread stack support, and more.";
            info->Url = L"http://processhacker.sf.net/forums/viewtopic.php?t=1111";
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
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &PluginTreeNewMessageCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackPhSvcRequest),
                PhSvcRequestCallback,
                NULL,
                &PluginPhSvcRequestCallbackRegistration
                );

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
                PhGetGeneralCallback(GeneralCallbackThreadTreeNewInitializing),
                ThreadTreeNewInitializingCallback,
                NULL,
                &ThreadTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadTreeNewUninitializing),
                ThreadTreeNewUninitializingCallback,
                NULL,
                &ThreadTreeNewUninitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadStackControl),
                ThreadStackControlCallback,
                NULL,
                &ThreadStackControlCallbackRegistration
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmThreadItemType,
                sizeof(DN_THREAD_ITEM),
                ThreadItemCreateCallback,
                ThreadItemDeleteCallback
                );
            InitializeTreeNewObjectExtensions();

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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DispatchTreeNewMessage(Parameter);
}

VOID NTAPI PhSvcRequestCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DispatchPhSvcRequest(Parameter);
}

VOID NTAPI ThreadTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ThreadTreeNewInitializing(Parameter);
}

VOID NTAPI ThreadTreeNewUninitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ThreadTreeNewUninitializing(Parameter);
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;
    BOOLEAN isDotNet;

    if (NT_SUCCESS(PhGetProcessIsDotNet(propContext->ProcessItem->ProcessId, &isDotNet)))
    {
        if (isDotNet)
        {
            if (WindowsVersion >= WINDOWS_VISTA)
                AddAsmPageToPropContext(propContext);
            AddPerfPageToPropContext(propContext);
        }

        if (propContext->ProcessItem->IsDotNet != isDotNet)
            propContext->ProcessItem->UpdateIsDotNet = TRUE; // force a refresh
    }
}

VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ThreadStackControlCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessThreadStackControl(Parameter);
}

VOID NTAPI ThreadItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PDN_THREAD_ITEM dnThread = Extension;

    memset(dnThread, 0, sizeof(DN_THREAD_ITEM));
    dnThread->ThreadItem = Object;
}

VOID NTAPI ThreadItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PDN_THREAD_ITEM dnThread = Extension;

    PhClearReference(&dnThread->AppDomainText);
}

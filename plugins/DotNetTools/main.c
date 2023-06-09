/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2015-2023
 *
 */

#include "dn.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
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

VOID NTAPI MenuItemCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
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
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    DispatchTreeNewMessage(Parameter);
}

VOID NTAPI PhSvcRequestCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    DispatchPhSvcRequest(Parameter);
}

VOID NTAPI ThreadTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ThreadTreeNewInitializing(Parameter);
}

VOID NTAPI ThreadTreeNewUninitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ThreadTreeNewUninitializing(Parameter);
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;
    BOOLEAN isDotNet = FALSE;
    ULONG flags = 0;

    if (NT_SUCCESS(PhGetProcessIsDotNetEx(
        propContext->ProcessItem->ProcessId,
        NULL,
        propContext->ProcessItem->IsImmersive ? 0 : PH_CLR_USE_SECTION_CHECK,
        &isDotNet,
        &flags
        )))
    {
        if (isDotNet)
        {
            AddAsmPageToPropContext(propContext);
            AddPerfPageToPropContext(propContext);
        }
        else if (flags & PH_CLR_CORELIB_PRESENT)
        {
            isDotNet = TRUE;
            AddAsmPageToPropContext(propContext);
        }
        else if (flags & PH_CLR_CORE_3_0_ABOVE)
        {
            isDotNet = TRUE;
            AddAsmPageToPropContext(propContext);
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
    _In_ PVOID Parameter,
    _In_ PVOID Context
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
            PH_SETTING_CREATE settings[] =
            {
                { StringSettingType, SETTING_NAME_ASM_TREE_LIST_COLUMNS, L"" },
                { IntegerSettingType, SETTING_NAME_ASM_TREE_LIST_FLAGS, L"0" },
                { IntegerPairSettingType, SETTING_NAME_ASM_TREE_LIST_SORT, L"0,0" },
                { IntegerSettingType, SETTING_NAME_DOT_NET_CATEGORY_INDEX, L"5" },
                { StringSettingType, SETTING_NAME_DOT_NET_COUNTERS_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_DOT_NET_COUNTERS_SORTCOLUMN, L"" },
                { StringSettingType, SETTING_NAME_DOT_NET_COUNTERS_GROUPSTATES, L"" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L".NET Tools";
            info->Author = L"dmex, wj32";
            info->Description = L"Adds .NET performance counters, assembly information, thread stack support, and more.";

            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
            //    LoadCallback,
            //    NULL,
            //    &PluginLoadCallbackRegistration
            //    );
            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
            //    UnloadCallback,
            //    NULL,
            //    &PluginUnloadCallbackRegistration
            //    );
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

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
        }
        break;
    }

    return TRUE;
}

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

#include <trace.h>

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI LoadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI UnloadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

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
                { IntegerSettingType, SETTING_NAME_ENABLED, L"0" },
                { IntegerSettingType, SETTING_NAME_ALLOW_SANDBOXED_CLIENTS, L"0" },
                { IntegerSettingType, SETTING_NAME_CONFIRM_CONNECTIONS, L"1" },
                { StringSettingType, SETTING_NAME_AGENTS_LISTVIEW_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_TOOLS_LISTVIEW_COLUMNS, L"" },
            };

            WPP_INIT_TRACING(PLUGIN_NAME);

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Agent Tools";
            info->Description = L"Exposes System Informer's process data and actions to AI agents over the Model Context Protocol.";

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
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
            AtRegisterToolSettings();
        }
        break;
    }

    return TRUE;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI LoadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    AtConsentInitialize();

    // Off by default: nothing listens until the user enables it in options.
    if (PhGetIntegerSetting(SETTING_NAME_ENABLED))
    {
        AtServerStart();
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI UnloadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    AtServerStop(SimcpCloseServerShutdown);
    AtConsentUninitialize();
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"Agent Tools",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        AtOptionsDlgProc,
        NULL
        );
    optionsEntry->CreateSection(
        L"Agent Tools - Agents",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS_AGENTS),
        AtAgentsDlgProc,
        NULL
        );
}

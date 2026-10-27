/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2015-2026
 *
 */

#include "dn.h"
#include "clrsup.h"

#include <trace.h>

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginTreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginPhSvcRequestCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadTreeNewUninitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadStackControlCallbackRegistration;

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI TreeNewMessageCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    DispatchTreeNewMessage(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhSvcRequestCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    DispatchPhSvcRequest(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ThreadTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ThreadTreeNewInitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ThreadTreeNewUninitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ThreadTreeNewUninitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
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
        propContext->ProcessItem->QueryHandle,
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

_Function_class_(PH_CALLBACK_FUNCTION)
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


// DOTNETTOOLS_INTERFACE

DOTNETTOOLS_ASSEMBLY_STATUS NTAPI DotNetToolsEnumProcessAssemblies(
    _In_ HANDLE ProcessId,
    _In_ PDOTNETTOOLS_ASSEMBLY_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_opt_ PULONG UnreadableAppDomains
    )
{
    PCLR_PROCESS_SUPPORT support;
    PPH_LIST appDomainList;
    BOOLEAN isDotNet = FALSE;
    ULONG unreadableAppDomains = 0;
    ULONG i;
    ULONG j;

    if (UnreadableAppDomains)
        *UnreadableAppDomains = 0;

#ifdef _WIN64
    {
        HANDLE processHandle;
        BOOLEAN isWow64 = FALSE;

        if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessId)))
        {
            PhGetProcessIsWow64(processHandle, &isWow64);
            NtClose(processHandle);
        }

        if (isWow64)
            return DotNetToolsAssembliesWow64;
    }
#endif

    // Whether the data access layer attaches is the real test, so it is tried first and the
    // detection is only used to explain a failure.
    if (!(support = CreateClrProcessSupport(ProcessId)))
    {
        NTSTATUS sectionStatus;
        NTSTATUS handleStatus;

        sectionStatus = PhGetProcessIsDotNetEx(ProcessId, NULL, PH_CLR_USE_SECTION_CHECK, &isDotNet, NULL);

        if (NT_SUCCESS(sectionStatus) && isDotNet)
            return DotNetToolsAssembliesFailed;

        handleStatus = PhGetProcessIsDotNetEx(ProcessId, NULL, 0, &isDotNet, NULL);

        if (NT_SUCCESS(handleStatus) && isDotNet)
            return DotNetToolsAssembliesFailed;

        // Only a check that ran can say a process is not .NET. When neither could be made - both
        // want access this caller may not have - that is a failure to tell, and reporting it as a
        // finding told the user the process is not .NET when nobody had looked.
        if (!NT_SUCCESS(sectionStatus) && !NT_SUCCESS(handleStatus))
            return DotNetToolsAssembliesFailed;

        return DotNetToolsAssembliesNotDotNet;
    }

    if (!(appDomainList = DnGetClrAppDomainAssemblyList(support)))
    {
        FreeClrProcessSupport(support);
        return DotNetToolsAssembliesFailed;
    }

    for (i = 0; i < appDomainList->Count; i++)
    {
        PDN_PROCESS_APPDOMAIN_ENTRY appDomain = appDomainList->Items[i];

        // The domain was enumerated but its assemblies could not be read, so what follows is not
        // the whole list. Only the caller can decide what a partial answer is worth.
        if (!appDomain->AssemblyList)
        {
            unreadableAppDomains++;
            continue;
        }

        for (j = 0; j < appDomain->AssemblyList->Count; j++)
        {
            PDN_DOTNET_ASSEMBLY_ENTRY entry = appDomain->AssemblyList->Items[j];
            DOTNETTOOLS_ASSEMBLY assembly;

            memset(&assembly, 0, sizeof(DOTNETTOOLS_ASSEMBLY));
            assembly.AppDomainType = appDomain->AppDomainType;
            assembly.AppDomainNumber = appDomain->AppDomainNumber;
            assembly.AppDomainId = appDomain->AppDomainID;
            assembly.AppDomainName = appDomain->AppDomainName;
            assembly.IsDynamic = !!entry->IsDynamicAssembly;
            assembly.IsReflection = !!entry->IsReflection;
            // CLRDataModuleFlag says how the module was loaded, and carries nothing about native
            // images; NativeFileName is where a precompiled image shows up.
            assembly.IsDynamicModule = !!FlagOn(entry->ModuleFlag, CLRDATA_MODULE_IS_DYNAMIC);
            assembly.IsMemoryStream = !!FlagOn(entry->ModuleFlag, CLRDATA_MODULE_IS_MEMORY_STREAM);
            assembly.IsMainModule = !!FlagOn(entry->ModuleFlag, CLRDATA_MODULE_IS_MAIN_MODULE);
            assembly.BaseAddress = entry->BaseAddress;
            assembly.AssemblyId = entry->AssemblyID;
            assembly.ModuleId = entry->ModuleID;
            assembly.AssemblyName = entry->AssemblyName;
            assembly.DisplayName = entry->DisplayName;
            assembly.ModuleName = entry->ModuleName;
            assembly.NativeFileName = entry->NativeFileName;
            assembly.Mvid = entry->Mvid;

            if (!Callback(&assembly, Context))
                goto CleanupExit;
        }
    }

CleanupExit:
    DnDestroyProcessDotNetAppDomainList(appDomainList);
    FreeClrProcessSupport(support);

    if (UnreadableAppDomains)
        *UnreadableAppDomains = unreadableAppDomains;

    return DotNetToolsAssembliesOk;
}

DOTNETTOOLS_INTERFACE PluginInterface =
{
    DOTNETTOOLS_INTERFACE_VERSION,
    DotNetToolsEnumProcessAssemblies
};

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
                { StringSettingType, SETTING_NAME_DOT_NET_COUNTERS_GROUPSTATES, L"" },
            };

            WPP_INIT_TRACING(PLUGIN_NAME);

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->Interface = &PluginInterface;
            info->DisplayName = L".NET Tools";
            info->Description = L"Adds .NET performance counters, assembly information, thread stack support, and more.";

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

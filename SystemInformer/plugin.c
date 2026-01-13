/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2024
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <colmgr.h>
#include <emenu.h>
#include <extmgri.h>
#include <phsvccl.h>
#include <procprv.h>
#include <settings.h>
#include <phsettings.h>
#include <mapldr.h>

#include <trace.h>

typedef struct _PHP_PLUGIN_LOAD_ERROR
{
    PPH_STRING FileName;
    PPH_STRING ErrorMessage;
} PHP_PLUGIN_LOAD_ERROR, *PPHP_PLUGIN_LOAD_ERROR;

typedef struct _PHP_PLUGIN_MENU_HOOK
{
    PPH_PLUGIN Plugin;
    PVOID Context;
} PHP_PLUGIN_MENU_HOOK, *PPHP_PLUGIN_MENU_HOOK;

typedef struct _PH_LOADPLUGIN_CONTEXT
{
    BOOLEAN LoadNative;
    BOOLEAN LoadDefault;

    PPH_STRING PluginsDirectory;
    PPH_LIST LoadErrors;
} PH_LOADPLUGIN_CONTEXT, *PPH_LOADPLUGIN_CONTEXT;

LONG NTAPI PhpPluginsCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    );

NTSTATUS PhLoadPlugin(
    _In_ PCPH_STRINGREF FileName
    );

VOID PhLoadPluginErrorMessage(
    _In_ PPH_LOADPLUGIN_CONTEXT Context,
    _In_ PPH_STRING FileName,
    _In_ NTSTATUS Status
    );

VOID PhInvokeCallbackForAllPlugins(
    _In_ PH_PLUGIN_CALLBACK Callback,
    _In_opt_ PVOID Parameters
    );

VOID PhpExecuteCallbackForAllPlugins(
    _In_ PH_PLUGIN_CALLBACK Callback,
    _In_ BOOLEAN StartupParameters
    );

PH_AVL_TREE PhPluginsByName = PH_AVL_TREE_INIT(PhpPluginsCompareFunction);
static PH_CALLBACK GeneralCallbacks[GeneralCallbackMaximum];
static ULONG NextPluginId = IDPLUGINS + 1;
static CONST PH_STRINGREF DefaultPluginName[] =
{
    PH_STRINGREF_INIT(L"DotNetTools.dll"),
    PH_STRINGREF_INIT(L"ExtendedNotifications.dll"),
    PH_STRINGREF_INIT(L"ExtendedServices.dll"),
    PH_STRINGREF_INIT(L"ExtendedTools.dll"),
    PH_STRINGREF_INIT(L"HardwareDevices.dll"),
    PH_STRINGREF_INIT(L"NetworkTools.dll"),
    PH_STRINGREF_INIT(L"OnlineChecks.dll"),
    PH_STRINGREF_INIT(L"ToolStatus.dll"),
    PH_STRINGREF_INIT(L"Updater.dll"),
    PH_STRINGREF_INIT(L"UserNotes.dll"),
    PH_STRINGREF_INIT(L"WindowExplorer.dll"),
};

LONG NTAPI PhpPluginsCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPH_PLUGIN plugin1 = CONTAINING_RECORD(Links1, PH_PLUGIN, Links);
    PPH_PLUGIN plugin2 = CONTAINING_RECORD(Links2, PH_PLUGIN, Links);

    return PhCompareStringRef(&plugin1->Name, &plugin2->Name, FALSE);
}

_Success_(return)
BOOLEAN PhpLocateDisabledPlugin(
    _In_ PPH_STRING List,
    _In_ PCPH_STRINGREF BaseName,
    _Out_opt_ PULONG_PTR FoundIndex
    )
{
    PH_STRINGREF namePart;
    PH_STRINGREF remainingPart;

    remainingPart = PhGetStringRef(List);

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &namePart, &remainingPart);

        if (PhEqualStringRef(&namePart, BaseName, TRUE))
        {
            if (FoundIndex)
                *FoundIndex = (ULONG_PTR)(namePart.Buffer - List->Buffer);

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PhIsPluginDisabled(
    _In_ PCPH_STRINGREF BaseName
    )
{
    BOOLEAN found;
    PPH_STRING disabled;

    disabled = PhGetStringSetting(SETTING_DISABLED_PLUGINS);
    found = PhpLocateDisabledPlugin(disabled, BaseName, NULL);
    PhDereferenceObject(disabled);

    return found;
}

VOID PhSetPluginDisabled(
    _In_ PCPH_STRINGREF BaseName,
    _In_ BOOLEAN Disable
    )
{
    BOOLEAN found;
    PPH_STRING disabled;
    ULONG_PTR foundIndex;
    PPH_STRING newDisabled;

    disabled = PhGetStringSetting(SETTING_DISABLED_PLUGINS);
    found = PhpLocateDisabledPlugin(disabled, BaseName, &foundIndex);

    if (Disable && !found)
    {
        // We need to add the plugin to the disabled list.

        if (disabled->Length != 0)
        {
            // We have other disabled plugins. Append a pipe character followed by the plugin name.
            newDisabled = PhCreateStringEx(NULL, disabled->Length + sizeof(WCHAR) + BaseName->Length);
            memcpy(newDisabled->Buffer, disabled->Buffer, disabled->Length);
            newDisabled->Buffer[disabled->Length / sizeof(WCHAR)] = L'|';
            memcpy(&newDisabled->Buffer[disabled->Length / sizeof(WCHAR) + 1], BaseName->Buffer, BaseName->Length);
            PhSetStringSetting2(SETTING_DISABLED_PLUGINS, &newDisabled->sr);
            PhDereferenceObject(newDisabled);
        }
        else
        {
            // This is the first disabled plugin.
            PhSetStringSetting2(SETTING_DISABLED_PLUGINS, BaseName);
        }
    }
    else if (!Disable && found)
    {
        SIZE_T removeCount;

        // We need to remove the plugin from the disabled list.

        removeCount = BaseName->Length / sizeof(WCHAR);

        if (foundIndex + BaseName->Length / sizeof(WCHAR) < disabled->Length / sizeof(WCHAR))
        {
            // Remove the following pipe character as well.
            removeCount++;
        }
        else if (foundIndex != 0)
        {
            // Remove the preceding pipe character as well.
            foundIndex--;
            removeCount++;
        }

        newDisabled = PhCreateStringEx(NULL, disabled->Length - removeCount * sizeof(WCHAR));
        memmove(newDisabled->Buffer, disabled->Buffer, foundIndex * sizeof(WCHAR));
        memmove(&newDisabled->Buffer[foundIndex], &disabled->Buffer[foundIndex + removeCount],
            disabled->Length - removeCount * sizeof(WCHAR) - foundIndex * sizeof(WCHAR));
        PhSetStringSetting2(SETTING_DISABLED_PLUGINS, &newDisabled->sr);
        PhDereferenceObject(newDisabled);
    }

    PhDereferenceObject(disabled);
}

//PPH_STRING PhGetPluginDirectoryPath(
//    _In_ BOOLEAN NativeFileName
//    )
//{
//    static CONST PH_STRINGREF pluginsDirectory = PH_STRINGREF_INIT(L"plugins\\");
//
//    return PhGetApplicationDirectoryFileName(&pluginsDirectory, NativeFileName);
//}
//
//PPH_STRING PhpGetPluginDirectoryPath(
//    VOID
//    )
//{
//    static PPH_STRING cachedPluginDirectory = NULL;
//    PPH_STRING pluginsDirectory;
//    SIZE_T returnLength;
//    WCHAR pluginsDirectoryName[MAX_PATH];
//    PH_FORMAT format[3];
//
//    if (pluginsDirectory = InterlockedCompareExchangePointer(
//        &cachedPluginDirectory,
//        NULL,
//        NULL
//        ))
//    {
//        return PhReferenceObject(pluginsDirectory);
//    }
//
//    pluginsDirectory = PhGetStringSetting(L"PluginsDirectory");
//
//    if (RtlDetermineDosPathNameType_U(pluginsDirectory->Buffer) == RtlPathTypeRelative)
//    {
//        PPH_STRING applicationDirectory;
//
//        if (applicationDirectory = PhGetApplicationDirectoryWin32())
//        {
//            PH_STRINGREF pluginsDirectoryNameSr;
//
//            // Not absolute. Make sure it is.
//            PhInitFormatSR(&format[0], applicationDirectory->sr);
//            PhInitFormatSR(&format[1], pluginsDirectory->sr);
//            PhInitFormatC(&format[2], OBJ_NAME_PATH_SEPARATOR);
//
//            if (PhFormatToBuffer(
//                format,
//                RTL_NUMBER_OF(format),
//                pluginsDirectoryName,
//                sizeof(pluginsDirectoryName),
//                &returnLength
//                ))
//            {
//                pluginsDirectoryNameSr.Buffer = pluginsDirectoryName;
//                pluginsDirectoryNameSr.Length = returnLength - sizeof(UNICODE_NULL);
//
//                PhMoveReference(&pluginsDirectory, PhCreateString2(&pluginsDirectoryNameSr));
//            }
//
//            PhDereferenceObject(applicationDirectory);
//        }
//    }
//
//    if (!InterlockedCompareExchangePointer(
//        &cachedPluginDirectory,
//        pluginsDirectory,
//        NULL
//        ))
//    {
//        PhReferenceObject(pluginsDirectory);
//    }
//
//    return pluginsDirectory;
//}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN EnumPluginsDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PFILE_NAMES_INFORMATION fileNamesInfo = (PFILE_NAMES_INFORMATION)Information;
    PPH_LOADPLUGIN_CONTEXT context = (PPH_LOADPLUGIN_CONTEXT)Context;
    NTSTATUS status;
    PPH_STRING fileName;
    PH_STRINGREF baseName;

    baseName.Buffer = fileNamesInfo->FileName;
    baseName.Length = fileNamesInfo->FileNameLength;

    if (!PhIsPluginDisabled(&baseName))
    {
        if (context->LoadNative)
        {
            status = PhLoadPluginImage(&baseName, RootDirectory, NULL);
        }
        else
        {
            if (fileName = PhConcatStringRef2(&context->PluginsDirectory->sr, &baseName))
            {
                status = PhLoadPlugin(&fileName->sr);

                PhDereferenceObject(fileName);
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }
        }

        if (!NT_SUCCESS(status))
        {
            fileName = PhCreateString2(&baseName);
            PhLoadPluginErrorMessage(context, PhCreateString2(&baseName), status);
            PhDereferenceObject(fileName);
        }
    }

    return TRUE;
}

VOID PhpShowPluginErrorMessage(
    _Inout_ PPH_LIST PluginLoadErrors
    )
{
    TASKDIALOGCONFIG config;
    PH_STRING_BUILDER stringBuilder;
    PPHP_PLUGIN_LOAD_ERROR loadError;
    PPH_STRING baseName;
    INT result;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (ULONG i = 0; i < PluginLoadErrors->Count; i++)
    {
        loadError = PluginLoadErrors->Items[i];
        baseName = PhGetBaseName(loadError->FileName);

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%s: %s\n",
            baseName->Buffer,
            PhGetStringOrDefault(loadError->ErrorMessage, L"An unknown error occurred.")
            );

        PhDereferenceObject(baseName);
    }

    if (PhEndsWithStringRef2(&stringBuilder.String->sr, L"\n", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.dwCommonButtons = TDCBF_OK_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_INFORMATION_ICON;
    config.pszMainInstruction = L"Unable to load the following plugin(s)";
    config.pszContent = PhGetString(PhFinalStringBuilderString(&stringBuilder));
    config.nDefaultButton = IDOK;

    if (PhShowTaskDialog(
        &config,
        &result,
        NULL,
        NULL
        ))
    {
        //switch (result)
        //{
        //case IDNO:
        //    for (i = 0; i < pluginLoadErrors->Count; i++)
        //    {
        //        loadError = pluginLoadErrors->Items[i];
        //        baseName = PhGetBaseName(loadError->FileName);
        //        PhSetPluginDisabled(&baseName->sr, TRUE);
        //        PhDereferenceObject(baseName);
        //    }
        //    break;
        //case IDYES:
        //    for (i = 0; i < pluginLoadErrors->Count; i++)
        //    {
        //        loadError = pluginLoadErrors->Items[i];
        //        PhDeleteFileWin32(loadError->FileName->Buffer);
        //    }
        //    break;
        //}
    }

    PhDeleteStringBuilder(&stringBuilder);
}

NTSTATUS PhLoadPluginsEnumDirectory(
    _In_ PPH_STRINGREF PluginsDirectory,
    _In_ PPH_LOADPLUGIN_CONTEXT Context
    )
{
    static CONST PH_STRINGREF pluginsSearchPattern = PH_STRINGREF_INIT(L"*.dll");
    NTSTATUS status;
    HANDLE directoryHandle;

    if (Context->LoadNative)
    {
        status = PhCreateFile(
            &directoryHandle,
            PluginsDirectory,
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );
    }
    else
    {
        status = PhCreateFileWin32(
            &directoryHandle,
            PhGetStringRefZ(PluginsDirectory),
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    status = PhEnumDirectoryFileEx(
        directoryHandle,
        FileNamesInformation,
        FALSE,
        &pluginsSearchPattern,
        EnumPluginsDirectoryCallback,
        Context
        );

    if (!NT_SUCCESS(status))
    {
        status = PhEnumDirectoryFileEx(
            directoryHandle,
            FileNamesInformation,
            TRUE,
            &pluginsSearchPattern,
            EnumPluginsDirectoryCallback,
            Context
            );
    }

    NtClose(directoryHandle);

    return status;
}

/**
 * Loads plugins from the default plugins directory.
 */
VOID PhLoadPlugins(
    VOID
    )
{
    static CONST PH_STRINGREF pluginsDirectory = PH_STRINGREF_INIT(L"plugins\\");
    NTSTATUS status;
    BOOLEAN pluginLoadNative;
    BOOLEAN pluginLoadDefault;
    PPH_STRING pluginFileName;
    PPH_STRING pluginDirectoryPath;
    PH_LOADPLUGIN_CONTEXT pluginLoadContext;

    pluginLoadNative = !!PhGetIntegerSetting(SETTING_ENABLE_PLUGINS_NATIVE);
    pluginLoadDefault = !!PhGetIntegerSetting(SETTING_ENABLE_DEFAULT_SAFE_PLUGINS);
    pluginDirectoryPath = PhGetApplicationDirectoryFileName(&pluginsDirectory, pluginLoadNative);

    if (PhIsNullOrEmptyString(pluginDirectoryPath))
        return;

    memset(&pluginLoadContext, 0, sizeof(PH_LOADPLUGIN_CONTEXT));
    pluginLoadContext.LoadNative = pluginLoadNative;
    pluginLoadContext.LoadDefault = pluginLoadDefault;
    pluginLoadContext.PluginsDirectory = pluginDirectoryPath;

    if (pluginLoadDefault)
    {
        // Load default plugins

        for (ULONG i = 0; i < RTL_NUMBER_OF(DefaultPluginName); i++)
        {
            if (PhIsPluginDisabled(&DefaultPluginName[i]))
                continue;

            if (pluginFileName = PhConcatStringRef2(&pluginDirectoryPath->sr, &DefaultPluginName[i]))
            {
                status = PhLoadPlugin(&pluginFileName->sr);

                if (!NT_SUCCESS(status))
                {
                    PhLoadPluginErrorMessage(&pluginLoadContext, pluginFileName, status);
                }

                PhDereferenceObject(pluginFileName);
            }
        }
    }
    else
    {
        // Load non-default plugins

        PhLoadPluginsEnumDirectory(&pluginDirectoryPath->sr, &pluginLoadContext);
    }

    // Handle load errors.
    // In certain startup modes we want to ignore all plugin load errors.
    if (
        pluginLoadContext.LoadErrors &&
        pluginLoadContext.LoadErrors->Count != 0 &&
        PhGetIntegerSetting(SETTING_SHOW_PLUGIN_LOAD_ERRORS) &&
        !PhStartupParameters.PhSvc
        )
    {
        PhpShowPluginErrorMessage(pluginLoadContext.LoadErrors);
    }

    // When we loaded settings before, we didn't know about plugin settings, so they
    // went into the ignored settings list. Now that they've had a chance to add
    // settings, we should scan the ignored settings list and move the settings to
    // the right places.
    if (PhSettingsFileName)
        PhConvertIgnoredSettings();

    PhpExecuteCallbackForAllPlugins(PluginCallbackLoad, TRUE);

    if (pluginLoadContext.LoadErrors)
    {
        for (ULONG i = 0; i < pluginLoadContext.LoadErrors->Count; i++)
        {
            PPHP_PLUGIN_LOAD_ERROR loadError;

            loadError = pluginLoadContext.LoadErrors->Items[i];

            if (loadError->FileName)
                PhDereferenceObject(loadError->FileName);
            if (loadError->ErrorMessage)
                PhDereferenceObject(loadError->ErrorMessage);

            PhFree(loadError);
        }

        PhDereferenceObject(pluginLoadContext.LoadErrors);
    }

    PhDereferenceObject(pluginDirectoryPath);
}

/**
 * Notifies all plugins that the program is shutting down.
 * 
 * \param SessionEnding TRUE if the user ends the Windows session by logging off
 * or shutting down the operating system. otherwise, FALSE.
 */
VOID PhUnloadPlugins(
    _In_ BOOLEAN SessionEnding
    )
{
    PhInvokeCallbackForAllPlugins(PluginCallbackUnload, UlongToPtr(SessionEnding));
}

/**
 * Loads a plugin.
 *
 * \param FileName The file name of the plugin.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhLoadPlugin(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;

    if (LoadLibraryEx(PhGetStringRefZ(FileName), NULL, 0))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

VOID PhLoadPluginErrorMessage(
    _In_ PPH_LOADPLUGIN_CONTEXT Context,
    _In_ PPH_STRING FileName,
    _In_ NTSTATUS Status
    )
{
    PPHP_PLUGIN_LOAD_ERROR loadError;
    PPH_STRING errorMessage;

    loadError = PhAllocateZero(sizeof(PHP_PLUGIN_LOAD_ERROR));
    PhSetReference(&loadError->FileName, FileName);

    if (errorMessage = PhGetNtMessage(Status))
    {
        PhSetReference(&loadError->ErrorMessage, errorMessage);
        PhDereferenceObject(errorMessage);
    }

    if (!Context->LoadErrors)
    {
        Context->LoadErrors = PhCreateList(1);
    }

    if (Context->LoadErrors)
    {
        PhAddItemList(Context->LoadErrors, loadError);
    }
}

VOID PhInvokeCallbackForAllPlugins(
    _In_ PH_PLUGIN_CALLBACK Callback,
    _In_opt_ PVOID Parameters
    )
{
    PPH_AVL_LINKS links;

    for (links = PhMinimumElementAvlTree(&PhPluginsByName); links; links = PhSuccessorElementAvlTree(links))
    {
        PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);

        PhInvokeCallback(PhGetPluginCallback(plugin, Callback), Parameters);
    }
}

VOID PhpExecuteCallbackForAllPlugins(
    _In_ PH_PLUGIN_CALLBACK Callback,
    _In_ BOOLEAN StartupParameters
    )
{
    PPH_AVL_LINKS links;

    for (links = PhMinimumElementAvlTree(&PhPluginsByName); links; links = PhSuccessorElementAvlTree(links))
    {
        PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);
        PPH_LIST parameters = NULL;

        // Find relevant startup parameters for this plugin.
        if (StartupParameters && PhStartupParameters.PluginParameters)
        {
            ULONG i;

            for (i = 0; i < PhStartupParameters.PluginParameters->Count; i++)
            {
                PPH_STRING string = PhStartupParameters.PluginParameters->Items[i];
                PH_STRINGREF pluginName;
                PH_STRINGREF parameter;

                if (PhSplitStringRefAtChar(&string->sr, L':', &pluginName, &parameter) &&
                    PhEqualStringRef(&pluginName, &plugin->Name, FALSE) &&
                    parameter.Length != 0)
                {
                    if (!parameters)
                        parameters = PhCreateList(3);

                    if (parameters)
                        PhAddItemList(parameters, PhCreateString2(&parameter));
                }
            }
        }

        PhInvokeCallback(PhGetPluginCallback(plugin, Callback), parameters);

        if (parameters)
        {
            PhDereferenceObjects(parameters->Items, parameters->Count);
            PhDereferenceObject(parameters);
        }
    }
}

/**
 * Validate the plugin name contains only valid characters.
 * Valid characters are alphanumeric characters, spaces, dots, and underscores.
 *
 * \param Name A pointer to a PH_STRINGREF structure that contains the plugin name to validate.
 * \return TRUE if the name is valid; otherwise, FALSE.
 */
BOOLEAN PhpValidatePluginName(
    _In_ PCPH_STRINGREF Name
    )
{
    SIZE_T i;
    PWSTR buffer;
    SIZE_T count;

    buffer = Name->Buffer;
    count = Name->Length / sizeof(WCHAR);

    for (i = 0; i < count; i++)
    {
        if (!iswalnum(buffer[i]) && buffer[i] != L' ' && buffer[i] != L'.' && buffer[i] != L'_')
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Registers a plugin with the host.
 *
 * \param Name A unique identifier for the plugin. The function fails
 * if another plugin has already been registered with the same name. The
 * name must only contain alphanumeric characters, spaces, dots and
 * underscores.
 * \param DllBase The base address of the plugin DLL. This is passed
 * to the DllMain function.
 * \param Information A variable which receives a pointer to the
 * plugin's additional information block. This should be filled in after
 * the function returns.
 * \return A pointer to the plugin instance structure, or NULL if the
 * function failed.
 */
PPH_PLUGIN PhRegisterPluginByName(
    _In_ PCPH_STRINGREF Name,
    _In_ PVOID DllBase,
    _Out_opt_ PPH_PLUGIN_INFORMATION *Information
    )
{
    PPH_PLUGIN plugin;
    PPH_AVL_LINKS existingLinks;
    ULONG i;

    PhTraceInfo("%ls plugin registering", PhGetStringRefZ(Name));

    if (!PhpValidatePluginName(Name))
        return NULL;

    plugin = PhAllocateZero(sizeof(PH_PLUGIN));
    plugin->Name = *Name;
    plugin->DllBase = DllBase;

    existingLinks = PhAddElementAvlTree(&PhPluginsByName, &plugin->Links);

    if (existingLinks)
    {
        // Another plugin has already been registered with the same name.
        PhFree(plugin);
        return NULL;
    }

    for (i = 0; i < PluginCallbackMaximum; i++)
        PhInitializeCallback(&plugin->Callbacks[i]);

    PhEmInitializeAppContext(&plugin->AppContext, Name);

    if (Information)
        *Information = &plugin->Information;

    return plugin;
}

/**
 * Locates a plugin instance structure.
 *
 * \param Name The name of the plugin.
 * \return A plugin instance structure, or NULL if the plugin was not found.
 */
PPH_PLUGIN PhFindPlugin2(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_AVL_LINKS links;
    PH_PLUGIN lookupPlugin;

    lookupPlugin.Name = *Name;
    links = PhFindElementAvlTree(&PhPluginsByName, &lookupPlugin.Links);

    if (links)
        return CONTAINING_RECORD(links, PH_PLUGIN, Links);
    else
        return NULL;
}

/**
 * Gets a pointer to a plugin's additional information block.
 *
 * \param Name The name of the plugin.
 * \param Version The version of the interface.
 * \return The plugin's internal interface.
 */
PVOID PhGetPluginInterface(
    _In_ PCPH_STRINGREF Name,
    _In_opt_ ULONG Version
    )
{
    PPH_PLUGIN plugin;
    PVOID iface;

    if (plugin = PhFindPlugin2(Name))
    {
        iface = PhGetPluginInformation(plugin)->Interface;

        if (Version)
        {
            struct
            {
                ULONG Version;
            } *Interface = iface;

            if (Interface->Version <= Version)
            {
                return iface;
            }
        }
        else
        {
            return iface;
        }
    }

    return NULL;
}

/**
 * Gets a pointer to a plugin's additional information block.
 *
 * \param Plugin The plugin instance structure.
 * \return The plugin's additional information block.
 */
PPH_PLUGIN_INFORMATION PhGetPluginInformation(
    _In_ PPH_PLUGIN Plugin
    )
{
    return &Plugin->Information;
}

/**
 * Retrieves a pointer to a plugin callback.
 *
 * \param Plugin A plugin instance structure.
 * \param Callback The type of callback.
 * \remarks The program invokes plugin callbacks for notifications
 * specific to a plugin.
 */
PPH_CALLBACK PhGetPluginCallback(
    _In_ PPH_PLUGIN Plugin,
    _In_ PH_PLUGIN_CALLBACK Callback
    )
{
    if (Callback >= PluginCallbackMaximum)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    return &Plugin->Callbacks[Callback];
}

VOID PhInitializeCallbacks(
    VOID
    )
{
    for (ULONG i = 0; i < GeneralCallbackMaximum; i++)
        PhInitializeCallback(&GeneralCallbacks[i]);
}

/**
 * Retrieves a pointer to a general callback.
 *
 * \param Callback The type of callback.
 * \remarks The program invokes general callbacks for system-wide
 * notifications.
 */
PPH_CALLBACK PhGetGeneralCallback(
    _In_ PH_GENERAL_CALLBACK Callback
    )
{
    if (Callback >= GeneralCallbackMaximum)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    return &GeneralCallbacks[Callback];
}

/**
 * Reserves unique GUI identifiers.
 *
 * \param Count The number of identifiers to reserve.
 *
 * \return The start of the reserved range.
 *
 * \remarks The identifiers reserved by this function are
 * guaranteed to be unique throughout the program.
 */
ULONG PhPluginReserveIds(
    _In_ ULONG Count
    )
{
    ULONG nextPluginId;

    nextPluginId = NextPluginId;
    NextPluginId += Count;

    return nextPluginId;
}

/**
 * Retrieves current system statistics.
 */
VOID PhPluginGetSystemStatistics(
    _Out_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
    if (!PhProcessStatisticsInitialized)
    {
        memset(Statistics, 0, sizeof(PH_PLUGIN_SYSTEM_STATISTICS));
        return;
    }

    Statistics->Performance = &PhPerfInformation;

    Statistics->NumberOfProcesses = PhTotalProcesses;
    Statistics->NumberOfThreads = PhTotalThreads;
    Statistics->NumberOfHandles = PhTotalHandles;

    Statistics->CpuKernelUsage = PhCpuKernelUsage;
    Statistics->CpuUserUsage = PhCpuUserUsage;

    Statistics->IoReadDelta = PhIoReadDelta;
    Statistics->IoWriteDelta = PhIoWriteDelta;
    Statistics->IoOtherDelta = PhIoOtherDelta;

    Statistics->CommitPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, 0);
    Statistics->PhysicalPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, 0);

    Statistics->MaxCpuProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0));
    Statistics->MaxIoProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, 0));

    Statistics->CpuKernelHistory = &PhCpuKernelHistory;
    Statistics->CpuUserHistory = &PhCpuUserHistory;
    Statistics->CpusKernelHistory = &PhCpusKernelHistory;
    Statistics->CpusUserHistory = &PhCpusUserHistory;
    Statistics->IoReadHistory = &PhIoReadHistory;
    Statistics->IoWriteHistory = &PhIoWriteHistory;
    Statistics->IoOtherHistory = &PhIoOtherHistory;
    Statistics->CommitHistory = &PhCommitHistory;
    Statistics->PhysicalHistory = &PhPhysicalHistory;
    Statistics->MaxCpuHistory = &PhMaxCpuHistory;
    Statistics->MaxIoHistory = &PhMaxIoHistory;
#ifdef PH_RECORD_MAX_USAGE
    Statistics->MaxCpuUsageHistory = &PhMaxCpuUsageHistory;
    Statistics->MaxIoReadOtherHistory = &PhMaxIoReadOtherHistory;
    Statistics->MaxIoWriteHistory = &PhMaxIoWriteHistory;
#else
    Statistics->MaxCpuUsageHistory = NULL;
    Statistics->MaxIoReadOtherHistory = NULL;
    Statistics->MaxIoWriteHistory = NULL;
#endif
}

_Function_class_(PH_EMENU_ITEM_DELETE_FUNCTION)
static VOID NTAPI PhpPluginEMenuItemDeleteFunction(
    _In_ PPH_EMENU_ITEM Item
    )
{
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;

    pluginMenuItem = Item->Context;

    if (pluginMenuItem->DeleteFunction)
        pluginMenuItem->DeleteFunction(pluginMenuItem);

    PhFree(pluginMenuItem);
}

/**
 * Creates a menu item.
 *
 * \param Plugin A plugin instance structure.
 * \param Flags A combination of flags.
 * \param Id An identifier for the menu item. This should be unique
 * within the plugin.
 * \param Text The text of the menu item.
 * \param Context A user-defined value for the menu item.
 *
 * \return A menu item object. This can then be inserted into another
 * menu using PhInsertEMenuItem().
 *
 * \remarks The \ref PluginCallbackMenuItem callback is invoked when
 * the menu item is chosen, and the \ref PH_PLUGIN_MENU_ITEM structure
 * will contain the \a Id and \a Context values passed to this function.
 */
PPH_EMENU_ITEM PhPluginCreateEMenuItem(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG Flags,
    _In_ ULONG Id,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;
    PPH_EMENU_ITEM item;

    pluginMenuItem = PhAllocateZero(sizeof(PH_PLUGIN_MENU_ITEM));
    pluginMenuItem->Plugin = Plugin;
    pluginMenuItem->Id = Id;
    pluginMenuItem->Context = Context;

    item = PhCreateEMenuItem(Flags, ID_PLUGIN_MENU_ITEM, Text, NULL, pluginMenuItem);
    item->DeleteFunction = PhpPluginEMenuItemDeleteFunction;

    return item;
}

/**
 * Adds a menu hook.
 *
 * \param MenuInfo The plugin menu information structure.
 * \param Plugin A plugin instance structure.
 * \param Context A user-defined value that is later accessible from the callback.
 *
 * \remarks The \ref PluginCallbackMenuHook callback is invoked when any menu item
 * from the menu is chosen.
 */
BOOLEAN PhPluginAddMenuHook(
    _Inout_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_ PPH_PLUGIN Plugin,
    _In_opt_ PVOID Context
    )
{
    PPHP_PLUGIN_MENU_HOOK hook;

    if (MenuInfo->Flags & PH_PLUGIN_MENU_DISALLOW_HOOKS)
        return FALSE;

    if (!MenuInfo->PluginHookList)
        MenuInfo->PluginHookList = PH_AUTO(PhCreateList(2));

    hook = PH_AUTO(PhCreateAlloc(sizeof(PHP_PLUGIN_MENU_HOOK)));
    hook->Plugin = Plugin;
    hook->Context = Context;
    PhAddItemList(MenuInfo->PluginHookList, hook);

    return TRUE;
}

/**
 * Initializes a plugin menu information structure.
 *
 * \param MenuInfo The structure to initialize.
 * \param Menu The menu being shown.
 * \param OwnerWindow The window that owns the menu.
 * \param Flags Additional flags.
 *
 * \remarks This function is reserved for internal use.
 */
VOID PhPluginInitializeMenuInfo(
    _Out_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_opt_ PPH_EMENU Menu,
    _In_ HWND OwnerWindow,
    _In_ ULONG Flags
    )
{
    memset(MenuInfo, 0, sizeof(PH_PLUGIN_MENU_INFORMATION));
    MenuInfo->Menu = Menu;
    MenuInfo->OwnerWindow = OwnerWindow;
    MenuInfo->Flags = Flags;
}

/**
 * Triggers a plugin menu item.
 *
 * \param MenuInfo The plugin menu information structure.
 * \param Item The menu item chosen by the user.
 *
 * \remarks This function is reserved for internal use.
 */
BOOLEAN PhPluginTriggerEMenuItem(
    _In_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_ PPH_EMENU_ITEM Item
    )
{
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;
    ULONG i;
    PPHP_PLUGIN_MENU_HOOK hook;
    PH_PLUGIN_MENU_HOOK_INFORMATION menuHookInfo;

    if (MenuInfo->PluginHookList)
    {
        for (i = 0; i < MenuInfo->PluginHookList->Count; i++)
        {
            hook = MenuInfo->PluginHookList->Items[i];
            menuHookInfo.MenuInfo = MenuInfo;
            menuHookInfo.SelectedItem = Item;
            menuHookInfo.Context = hook->Context;
            menuHookInfo.Handled = FALSE;
            PhInvokeCallback(PhGetPluginCallback(hook->Plugin, PluginCallbackMenuHook), &menuHookInfo);

            if (menuHookInfo.Handled)
                return TRUE;
        }
    }

    if (Item->Id != ID_PLUGIN_MENU_ITEM)
        return FALSE;

    pluginMenuItem = Item->Context;

    pluginMenuItem->OwnerWindow = MenuInfo->OwnerWindow;

    PhInvokeCallback(PhGetPluginCallback(pluginMenuItem->Plugin, PluginCallbackMenuItem), pluginMenuItem);

    return TRUE;
}

/**
 * Adds a column to a tree new control.
 *
 * \param Plugin A plugin instance structure.
 * \param CmData The CmData value from the \ref PH_PLUGIN_TREENEW_INFORMATION
 * structure.
 * \param Column The column properties.
 * \param SubId An identifier for the column. This should be unique within the
 * plugin.
 * \param Context A user-defined value.
 * \param SortFunction The sort function for the column.
 */
BOOLEAN PhPluginAddTreeNewColumn(
    _In_ PPH_PLUGIN Plugin,
    _In_ PVOID CmData,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_PLUGIN_TREENEW_SORT_FUNCTION SortFunction
    )
{
    return !!PhCmCreateColumn(
        CmData,
        Column,
        Plugin,
        SubId,
        Context,
        SortFunction
        );
}

/**
 * Sets the object extension size and callbacks for an object type.
 *
 * \param Plugin A plugin instance structure.
 * \param ObjectType The type of object for which the extension is being registered.
 * \param ExtensionSize The size of the extension, in bytes.
 * \param CreateCallback The object creation callback.
 * \param DeleteCallback The object deletion callback.
 */
VOID PhPluginSetObjectExtension(
    _In_ PPH_PLUGIN Plugin,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ ULONG ExtensionSize,
    _In_opt_ PPH_EM_OBJECT_CALLBACK CreateCallback,
    _In_opt_ PPH_EM_OBJECT_CALLBACK DeleteCallback
    )
{
    PhEmSetObjectExtension(
        &Plugin->AppContext,
        ObjectType,
        ExtensionSize,
        CreateCallback,
        DeleteCallback
        );
}

/**
 * Gets the object extension for an object.
 *
 * \param Plugin A plugin instance structure.
 * \param Object The object.
 * \param ObjectType The type of object for which an extension has been registered.
 */
PVOID PhPluginGetObjectExtension(
    _In_ PPH_PLUGIN Plugin,
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType
    )
{
    return PhEmGetObjectExtension(
        &Plugin->AppContext,
        ObjectType,
        Object
        );
}

/**
 * Allows a plugin to receive all treenew messages, not just column-related ones.
 *
 * \param Plugin A plugin instance structure.
 * \param CmData The CmData value from the \ref PH_PLUGIN_TREENEW_INFORMATION
 * structure.
 */
VOID PhPluginEnableTreeNewNotify(
    _In_ PPH_PLUGIN Plugin,
    _In_ PVOID CmData
    )
{
    PhCmSetNotifyPlugin(CmData, Plugin);
}

NTSTATUS PhPluginQueryPhSvc(
    _Out_ PPH_PLUGIN_PHSVC_CLIENT Client
    )
{
    if (!PhSvcClServerProcessId)
        return STATUS_INVALID_CID;

    Client->ServerProcessId = PhSvcClServerProcessId;
    Client->FreeHeap = PhSvcpFreeHeap;
    Client->CreateString = PhSvcpCreateString;

    return STATUS_SUCCESS;
}

NTSTATUS PhPluginCallPhSvc(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG SubId,
    _In_reads_bytes_opt_(InLength) PVOID InBuffer,
    _In_ ULONG InLength,
    _Out_writes_bytes_opt_(OutLength) PVOID OutBuffer,
    _In_ ULONG OutLength
    )
{
    NTSTATUS status;
    PPH_STRING apiId;
    PH_FORMAT format[4];

    PhInitFormatC(&format[0], L'+');
    PhInitFormatSR(&format[1], Plugin->Name);
    PhInitFormatC(&format[2], L'+');
    PhInitFormatU(&format[3], SubId);
    apiId = PhFormat(format, 4, 50);

    status = PhSvcCallPlugin(&apiId->sr, InBuffer, InLength, OutBuffer, OutLength);
    PhDereferenceObject(apiId);

    return status;
}

PPH_STRING PhGetPluginName(
    _In_ PPH_PLUGIN Plugin
    )
{
    return PhCreateString2(&Plugin->Name);
}

PPH_STRING PhGetPluginFileName(
    _In_ PPH_PLUGIN Plugin
    )
{
    PPH_STRING fileName = NULL;

    if (!NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), Plugin->DllBase, &fileName)))
        return NULL;

    return fileName;
}

VOID PhEnumeratePlugins(
    _In_ PPH_PLUGIN_ENUMERATE Callback,
    _In_opt_ PVOID Context
    )
{
    PPH_AVL_LINKS links;

    for (links = PhMinimumElementAvlTree(&PhPluginsByName); links; links = PhSuccessorElementAvlTree(links))
    {
        PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);

        if (!NT_SUCCESS(Callback(plugin, Context)))
            break;
    }
}

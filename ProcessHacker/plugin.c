/*
 * Process Hacker -
 *   plugin support
 *
 * Copyright (C) 2010-2015 wj32
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

#include <phapp.h>
#include <settings.h>
#include <emenu.h>
#include <phplug.h>
#include <extmgri.h>
#include <notifico.h>
#include <phsvccl.h>

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

INT NTAPI PhpPluginsCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    );

BOOLEAN PhLoadPlugin(
    _In_ PPH_STRING FileName
    );

VOID PhpExecuteCallbackForAllPlugins(
    _In_ PH_PLUGIN_CALLBACK Callback,
    _In_ BOOLEAN StartupParameters
    );

PH_AVL_TREE PhPluginsByName = PH_AVL_TREE_INIT(PhpPluginsCompareFunction);

static PH_CALLBACK GeneralCallbacks[GeneralCallbackMaximum];
static PPH_STRING PluginsDirectory;
static PPH_LIST LoadErrors;
static ULONG NextPluginId = IDPLUGINS + 1;

VOID PhPluginsInitialization(
    VOID
    )
{
    ULONG i;

    for (i = 0; i < GeneralCallbackMaximum; i++)
        PhInitializeCallback(&GeneralCallbacks[i]);

    if (WindowsVersion <= WINDOWS_XP)
    {
        PH_STRINGREF extendedTools = PH_STRINGREF_INIT(L"ExtendedTools.dll");

        // HACK and violation of abstraction.
        // Always disable ExtendedTools on XP to avoid the annoying error message.
        PhSetPluginDisabled(&extendedTools, TRUE);
    }
}

INT NTAPI PhpPluginsCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPH_PLUGIN plugin1 = CONTAINING_RECORD(Links1, PH_PLUGIN, Links);
    PPH_PLUGIN plugin2 = CONTAINING_RECORD(Links2, PH_PLUGIN, Links);

    return PhCompareStringRef(&plugin1->Name, &plugin2->Name, FALSE);
}

BOOLEAN PhpLocateDisabledPlugin(
    _In_ PPH_STRING List,
    _In_ PPH_STRINGREF BaseName,
    _Out_opt_ PULONG FoundIndex
    )
{
    PH_STRINGREF namePart;
    PH_STRINGREF remainingPart;

    remainingPart = List->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &namePart, &remainingPart);

        if (PhEqualStringRef(&namePart, BaseName, TRUE))
        {
            if (FoundIndex)
                *FoundIndex = (ULONG)(namePart.Buffer - List->Buffer);

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PhIsPluginDisabled(
    _In_ PPH_STRINGREF BaseName
    )
{
    BOOLEAN found;
    PPH_STRING disabled;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    found = PhpLocateDisabledPlugin(disabled, BaseName, NULL);
    PhDereferenceObject(disabled);

    return found;
}

VOID PhSetPluginDisabled(
    _In_ PPH_STRINGREF BaseName,
    _In_ BOOLEAN Disable
    )
{
    BOOLEAN found;
    PPH_STRING disabled;
    ULONG foundIndex;
    PPH_STRING newDisabled;

    disabled = PhGetStringSetting(L"DisabledPlugins");

    found = PhpLocateDisabledPlugin(disabled, BaseName, &foundIndex);

    if (Disable && !found)
    {
        // We need to add the plugin to the disabled list.

        if (disabled->Length != 0)
        {
            // We have other disabled plugins. Append a pipe character followed by the plugin name.
            newDisabled = PhCreateStringEx(NULL, disabled->Length + sizeof(WCHAR) + BaseName->Length);
            memcpy(newDisabled->Buffer, disabled->Buffer, disabled->Length);
            newDisabled->Buffer[disabled->Length / 2] = '|';
            memcpy(&newDisabled->Buffer[disabled->Length / 2 + 1], BaseName->Buffer, BaseName->Length);
            PhSetStringSetting2(L"DisabledPlugins", &newDisabled->sr);
            PhDereferenceObject(newDisabled);
        }
        else
        {
            // This is the first disabled plugin.
            PhSetStringSetting2(L"DisabledPlugins", BaseName);
        }
    }
    else if (!Disable && found)
    {
        ULONG removeCount;

        // We need to remove the plugin from the disabled list.

        removeCount = (ULONG)BaseName->Length / 2;

        if (foundIndex + (ULONG)BaseName->Length / 2 < (ULONG)disabled->Length / 2)
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
        memcpy(newDisabled->Buffer, disabled->Buffer, foundIndex * sizeof(WCHAR));
        memcpy(&newDisabled->Buffer[foundIndex], &disabled->Buffer[foundIndex + removeCount],
            disabled->Length - removeCount * sizeof(WCHAR) - foundIndex * sizeof(WCHAR));
        PhSetStringSetting2(L"DisabledPlugins", &newDisabled->sr);
        PhDereferenceObject(newDisabled);
    }

    PhDereferenceObject(disabled);
}

static BOOLEAN EnumPluginsDirectoryCallback(
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PH_STRINGREF baseName;
    PPH_STRING fileName;

    baseName.Buffer = Information->FileName;
    baseName.Length = Information->FileNameLength;

    if (PhEndsWithStringRef2(&baseName, L".dll", TRUE))
    {
        if (!PhIsPluginDisabled(&baseName))
        {
            fileName = PhCreateStringEx(NULL, PluginsDirectory->Length + Information->FileNameLength);
            memcpy(fileName->Buffer, PluginsDirectory->Buffer, PluginsDirectory->Length);
            memcpy(&fileName->Buffer[PluginsDirectory->Length / 2], Information->FileName, Information->FileNameLength);

            PhLoadPlugin(fileName);

            PhDereferenceObject(fileName);
        }
    }

    return TRUE;
}

/**
 * Loads plugins from the default plugins directory.
 */
VOID PhLoadPlugins(
    VOID
    )
{
    HANDLE pluginsDirectoryHandle;
    PPH_STRING pluginsDirectory;

    pluginsDirectory = PhGetStringSetting(L"PluginsDirectory");

    if (RtlDetermineDosPathNameType_U(pluginsDirectory->Buffer) == RtlPathTypeRelative)
    {
        // Not absolute. Make sure it is.
        PluginsDirectory = PhConcatStrings(4, PhApplicationDirectory->Buffer, L"\\", pluginsDirectory->Buffer, L"\\");
        PhDereferenceObject(pluginsDirectory);
    }
    else
    {
        PluginsDirectory = pluginsDirectory;
    }

    if (NT_SUCCESS(PhCreateFileWin32(
        &pluginsDirectoryHandle,
        PluginsDirectory->Buffer,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        UNICODE_STRING pattern = RTL_CONSTANT_STRING(L"*.dll");

        PhEnumDirectoryFile(pluginsDirectoryHandle, &pattern, EnumPluginsDirectoryCallback, NULL);
        NtClose(pluginsDirectoryHandle);
    }

    // Handle load errors.
    // In certain startup modes we want to ignore all plugin load errors.
    if (LoadErrors && LoadErrors->Count != 0 && !PhStartupParameters.PhSvc)
    {
        PH_STRING_BUILDER sb;
        ULONG i;
        PPHP_PLUGIN_LOAD_ERROR loadError;
        PPH_STRING baseName;

        PhInitializeStringBuilder(&sb, 100);
        PhAppendStringBuilder2(&sb, L"Unable to load the following plugin(s):\n\n");

        for (i = 0; i < LoadErrors->Count; i++)
        {
            loadError = LoadErrors->Items[i];
            baseName = PhGetBaseName(loadError->FileName);
            PhAppendFormatStringBuilder(&sb, L"%s: %s\n",
                baseName->Buffer, PhGetStringOrDefault(loadError->ErrorMessage, L"An unknown error occurred."));
            PhDereferenceObject(baseName);
        }

        PhAppendStringBuilder2(&sb, L"\nDo you want to disable the above plugin(s)?");

        if (PhShowMessage(
            NULL,
            MB_ICONERROR | MB_YESNO,
            sb.String->Buffer
            ) == IDYES)
        {
            ULONG i;

            for (i = 0; i < LoadErrors->Count; i++)
            {
                loadError = LoadErrors->Items[i];
                baseName = PhGetBaseName(loadError->FileName);
                PhSetPluginDisabled(&baseName->sr, TRUE);
                PhDereferenceObject(baseName);
            }
        }

        PhDeleteStringBuilder(&sb);
    }

    // When we loaded settings before, we didn't know about plugin settings, so they
    // went into the ignored settings list. Now that they've had a chance to add
    // settings, we should scan the ignored settings list and move the settings to
    // the right places.
    if (PhSettingsFileName)
        PhConvertIgnoredSettings();

    PhpExecuteCallbackForAllPlugins(PluginCallbackLoad, TRUE);
}

/**
 * Notifies all plugins that the program is shutting down.
 */
VOID PhUnloadPlugins(
    VOID
    )
{
    PhpExecuteCallbackForAllPlugins(PluginCallbackUnload, FALSE);
}

/**
 * Loads a plugin.
 *
 * \param FileName The full file name of the plugin.
 */
BOOLEAN PhLoadPlugin(
    _In_ PPH_STRING FileName
    )
{
    BOOLEAN success;
    PPH_STRING fileName;
    PPH_STRING errorMessage;

    fileName = PhGetFullPath(FileName->Buffer, NULL);

    if (!fileName)
        PhSetReference(&fileName, FileName);

    success = TRUE;

    if (!LoadLibrary(fileName->Buffer))
    {
        success = FALSE;
        errorMessage = PhGetWin32Message(GetLastError());
    }

    if (!success)
    {
        PPHP_PLUGIN_LOAD_ERROR loadError;

        loadError = PhAllocate(sizeof(PHP_PLUGIN_LOAD_ERROR));
        PhSetReference(&loadError->FileName, fileName);
        PhSetReference(&loadError->ErrorMessage, errorMessage);

        if (!LoadErrors)
            LoadErrors = PhCreateList(2);

        PhAddItemList(LoadErrors, loadError);

        if (errorMessage)
            PhDereferenceObject(errorMessage);
    }

    PhDereferenceObject(fileName);

    return success;
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

                if (PhSplitStringRefAtChar(&string->sr, ':', &pluginName, &parameter) &&
                    PhEqualStringRef(&pluginName, &plugin->Name, FALSE) &&
                    parameter.Length != 0)
                {
                    if (!parameters)
                        parameters = PhCreateList(3);

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

BOOLEAN PhpValidatePluginName(
    _In_ PPH_STRINGREF Name
    )
{
    SIZE_T i;
    PWSTR buffer;
    SIZE_T count;

    buffer = Name->Buffer;
    count = Name->Length / sizeof(WCHAR);

    for (i = 0; i < count; i++)
    {
        if (!iswalnum(buffer[i]) && buffer[i] != ' ' && buffer[i] != '.' && buffer[i] != '_')
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
 *
 * \return A pointer to the plugin instance structure, or NULL if the
 * function failed.
 */
PPH_PLUGIN PhRegisterPlugin(
    _In_ PWSTR Name,
    _In_ PVOID DllBase,
    _Out_opt_ PPH_PLUGIN_INFORMATION *Information
    )
{
    PPH_PLUGIN plugin;
    PH_STRINGREF pluginName;
    PPH_AVL_LINKS existingLinks;
    ULONG i;
    PPH_STRING fileName;

    PhInitializeStringRefLongHint(&pluginName, Name);

    if (!PhpValidatePluginName(&pluginName))
        return NULL;

    fileName = PhGetDllFileName(DllBase, NULL);

    if (!fileName)
        return NULL;

    plugin = PhAllocate(sizeof(PH_PLUGIN));
    memset(plugin, 0, sizeof(PH_PLUGIN));

    plugin->Name = pluginName;
    plugin->DllBase = DllBase;

    plugin->FileName = fileName;

    existingLinks = PhAddElementAvlTree(&PhPluginsByName, &plugin->Links);

    if (existingLinks)
    {
        // Another plugin has already been registered with the same name.
        PhFree(plugin);
        return NULL;
    }

    for (i = 0; i < PluginCallbackMaximum; i++)
        PhInitializeCallback(&plugin->Callbacks[i]);

    PhEmInitializeAppContext(&plugin->AppContext, &pluginName);

    if (Information)
        *Information = &plugin->Information;

    return plugin;
}

/**
 * Locates a plugin instance structure.
 *
 * \param Name The name of the plugin.
 *
 * \return A plugin instance structure, or NULL if the plugin was not found.
 */
PPH_PLUGIN PhFindPlugin(
    _In_ PWSTR Name
    )
{
    PH_STRINGREF name;

    PhInitializeStringRefLongHint(&name, Name);

    return PhFindPlugin2(&name);
}

/**
 * Locates a plugin instance structure.
 *
 * \param Name The name of the plugin.
 *
 * \return A plugin instance structure, or NULL if the plugin was not found.
 */
PPH_PLUGIN PhFindPlugin2(
    _In_ PPH_STRINGREF Name
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
 * \param Plugin The plugin instance structure.
 *
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
 *
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

/**
 * Retrieves a pointer to a general callback.
 *
 * \param Callback The type of callback.
 *
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
 * Adds a menu item to the program's main menu. This function is
 * deprecated. Use \c GeneralCallbackMainMenuInitializing instead.
 *
 * \param Plugin A plugin instance structure.
 * \param Location A handle to the parent menu, or one of the following:
 * \li \c PH_MENU_ITEM_LOCATION_VIEW The "View" menu.
 * \li \c PH_MENU_ITEM_LOCATION_TOOLS The "Tools" menu.
 * \param InsertAfter The text of the menu item to insert the
 * new menu item after. The search is a case-insensitive prefix search
 * that ignores prefix characters (ampersands).
 * \param Id An identifier for the menu item. This should be unique
 * within the plugin.
 * \param Text The text of the menu item.
 * \param Context A user-defined value for the menu item.
 *
 * \return TRUE if the function succeeded, otherwise FALSE.
 *
 * \remarks The \ref PluginCallbackMenuItem callback is invoked when
 * the menu item is chosen, and the \ref PH_PLUGIN_MENU_ITEM structure
 * will contain the \a Id and \a Context values passed to this function.
 */
ULONG_PTR PhPluginAddMenuItem(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG_PTR Location,
    _In_opt_ PWSTR InsertAfter,
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_opt_ PVOID Context
    )
{
    PH_ADDMENUITEM addMenuItem;

    addMenuItem.Plugin = Plugin;
    addMenuItem.InsertAfter = InsertAfter;
    addMenuItem.Text = Text;
    addMenuItem.Context = Context;

    if (Location < 0x1000)
    {
        addMenuItem.Location = (ULONG)Location;
    }
    else
    {
        return 0;
    }

    addMenuItem.Flags = Id & PH_MENU_ITEM_VALID_FLAGS;
    Id &= ~PH_MENU_ITEM_VALID_FLAGS;
    addMenuItem.Id = Id;

    return ProcessHacker_AddMenuItem(PhMainWndHandle, &addMenuItem);
}

/**
 * Retrieves current system statistics.
 */
VOID PhPluginGetSystemStatistics(
    _Out_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
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
    _In_ PWSTR Text,
    _In_opt_ PVOID Context
    )
{
    PPH_EMENU_ITEM item;
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;

    item = PhCreateEMenuItem(Flags, ID_PLUGIN_MENU_ITEM, Text, NULL, NULL);

    pluginMenuItem = PhAllocate(sizeof(PH_PLUGIN_MENU_ITEM));
    memset(pluginMenuItem, 0, sizeof(PH_PLUGIN_MENU_ITEM));
    pluginMenuItem->Plugin = Plugin;
    pluginMenuItem->Id = Id;
    pluginMenuItem->Context = Context;

    item->Context = pluginMenuItem;
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
        MenuInfo->PluginHookList = PhAutoDereferenceObject(PhCreateList(2));

    hook = PhAutoDereferenceObject(PhCreateAlloc(sizeof(PHP_PLUGIN_MENU_HOOK)));
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
 * Creates a notification icon.
 *
 * \param Plugin A plugin instance structure.
 * \param SubId An identifier for the column. This should be unique within the
 * plugin.
 * \param Context A user-defined value.
 * \param Text A string describing the notification icon.
 * \param Flags A combination of flags.
 * \li \c PH_NF_ICON_UNAVAILABLE The notification icon is currently unavailable.
 * \param RegistrationData A \ref PH_NF_ICON_REGISTRATION_DATA structure that
 * contains registration information.
 */
struct _PH_NF_ICON *PhPluginRegisterIcon(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_ struct _PH_NF_ICON_REGISTRATION_DATA *RegistrationData
    )
{
    return PhNfRegisterIcon(
        Plugin,
        SubId,
        Context,
        Text,
        Flags,
        RegistrationData->UpdateCallback,
        RegistrationData->MessageCallback
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

BOOLEAN PhPluginQueryPhSvc(
    _Out_ PPH_PLUGIN_PHSVC_CLIENT Client
    )
{
    if (!PhSvcClServerProcessId)
        return FALSE;

    Client->ServerProcessId = PhSvcClServerProcessId;
    Client->FreeHeap = PhSvcpFreeHeap;
    Client->CreateString = PhSvcpCreateString;

    return TRUE;
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

    PhInitFormatC(&format[0], '+');
    PhInitFormatSR(&format[1], Plugin->Name);
    PhInitFormatC(&format[2], '+');
    PhInitFormatU(&format[3], SubId);
    apiId = PhFormat(format, 4, 50);

    status = PhSvcCallPlugin(&apiId->sr, InBuffer, InLength, OutBuffer, OutLength);
    PhDereferenceObject(apiId);

    return status;
}

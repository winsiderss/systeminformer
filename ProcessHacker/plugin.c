/*
 * Process Hacker - 
 *   plugin support
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

#include <phapp.h>
#include <settings.h>
#include <emenu.h>
#include <phplug.h>
#define CINTERFACE
#define COBJMACROS
#include <mscoree.h>

typedef HRESULT (STDAPICALLTYPE *_CorBindToRuntimeEx)(
    __in LPCWSTR pwszVersion,
    __in LPCWSTR pwszBuildFlavor,
    __in DWORD startupFlags,
    __in REFCLSID rclsid,
    __in REFIID riid,
    __out LPVOID *ppv
    );

INT NTAPI PhpPluginsCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

VOID PhLoadPlugin(
    __in PPH_STRING FileName
    );

VOID PhpExecuteCallbackForAllPlugins(
    __in PH_PLUGIN_CALLBACK Callback
    );

PH_AVL_TREE PhPluginsByName = PH_AVL_TREE_INIT(PhpPluginsCompareFunction);

static PH_CALLBACK GeneralCallbacks[GeneralCallbackMaximum];
static PPH_STRING PluginsDirectory;
static PPH_STRING LoadingPluginFileName;
static BOOLEAN LoadingPluginIsClr = FALSE;
static ULONG NextPluginId = IDPLUGINS + 1;

static BOOLEAN PhPluginsClrHostInitialized = FALSE;
static PVOID PhPluginsClrHost;

static CLSID CLSID_CLRRuntimeHost_I = { 0x90f1a06e, 0x7712, 0x4762, { 0x86, 0xb5, 0x7a, 0x5e, 0xba, 0x6b, 0xdb, 0x02 } };
static IID IID_ICLRRuntimeHost_I = { 0x90f1a06c, 0x7712, 0x4762, { 0x86, 0xb5, 0x7a, 0x5e, 0xba, 0x6b, 0xdb, 0x02 } };

VOID PhPluginsInitialization()
{
    ULONG i;

    for (i = 0; i < GeneralCallbackMaximum; i++)
        PhInitializeCallback(&GeneralCallbacks[i]);
}

INT NTAPI PhpPluginsCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PPH_PLUGIN plugin1 = CONTAINING_RECORD(Links1, PH_PLUGIN, Links);
    PPH_PLUGIN plugin2 = CONTAINING_RECORD(Links2, PH_PLUGIN, Links);

    return wcscmp(plugin1->Name, plugin2->Name);
}

static BOOLEAN EnumPluginsDirectoryCallback(
    __in PFILE_DIRECTORY_INFORMATION Information,
    __in_opt PVOID Context
    )
{
    PH_STRINGREF baseName;
    PPH_STRING fileName;

    baseName.Buffer = Information->FileName;
    baseName.Length = (USHORT)Information->FileNameLength;

    if (PhEndsWithStringRef2(&baseName, L".dll", TRUE))
    {
        fileName = PhCreateStringEx(NULL, PluginsDirectory->Length + Information->FileNameLength);
        memcpy(fileName->Buffer, PluginsDirectory->Buffer, PluginsDirectory->Length);
        memcpy(&fileName->Buffer[PluginsDirectory->Length / 2], Information->FileName, Information->FileNameLength);

        PhLoadPlugin(fileName);

        PhDereferenceObject(fileName);
    }

    return TRUE;
}

VOID PhLoadPlugins()
{
    HANDLE pluginsDirectoryHandle;
    PPH_STRING pluginsDirectory;

    pluginsDirectory = PhGetStringSetting(L"PluginsDirectory");

    if (PhFindCharInString(pluginsDirectory, 0, ':') == -1)
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
        PH_STRINGREF pattern;

        PhInitializeStringRef(&pattern, L"*.dll");
        PhEnumDirectoryFile(pluginsDirectoryHandle, &pattern, EnumPluginsDirectoryCallback, NULL);
        NtClose(pluginsDirectoryHandle);
    }

    // When we loaded settings before, we didn't know about plugin settings, so they 
    // went into the ignored settings list. Now that they've had a chance to add 
    // settings, we should scan the ignored settings list and move the settings to 
    // the right places.
    if (PhSettingsFileName)
        PhConvertIgnoredSettings();

    PhpExecuteCallbackForAllPlugins(PluginCallbackLoad);
}

VOID PhUnloadPlugins()
{
    PhpExecuteCallbackForAllPlugins(PluginCallbackUnload);
}

VOID PhLoadPlugin(
    __in PPH_STRING FileName
    )
{
    PPH_STRING fileName;

    fileName = PhGetFullPath(FileName->Buffer, NULL);

    if (!fileName)
    {
        fileName = FileName;
        PhReferenceObject(fileName);
    }

    LoadingPluginFileName = fileName;

    if (!PhEndsWithString2(fileName, L".clr.dll", TRUE))
    {
        LoadLibrary(fileName->Buffer);
    }
    else
    {
        ICLRRuntimeHost *clrHost;
        ULONG returnValue;

        if (!PhPluginsClrHostInitialized)
        {
            _CorBindToRuntimeEx CorBindToRuntimeEx_I;
            HMODULE mscoreeHandle;

            if (
                (mscoreeHandle = LoadLibrary(L"mscoree.dll")) &&
                (CorBindToRuntimeEx_I = (_CorBindToRuntimeEx)GetProcAddress(mscoreeHandle, "CorBindToRuntimeEx"))
                )
            {
                if (SUCCEEDED(CorBindToRuntimeEx_I(NULL, L"wks", 0, &CLSID_CLRRuntimeHost_I,
                    &IID_ICLRRuntimeHost_I, &clrHost)))
                {
                    ICLRRuntimeHost_Start(clrHost);
                    PhPluginsClrHost = clrHost;
                }
            }

            PhPluginsClrHostInitialized = TRUE;
        }

        clrHost = (ICLRRuntimeHost *)PhPluginsClrHost;

        if (clrHost)
        {
            LoadingPluginIsClr = TRUE;
            ICLRRuntimeHost_ExecuteInDefaultAppDomain(
                clrHost,
                fileName->Buffer,
                L"ProcessHacker2.Plugin",
                L"PluginEntry",
                fileName->Buffer,
                &returnValue
                );
            LoadingPluginIsClr = FALSE;
        }
    }

    PhDereferenceObject(fileName);
    LoadingPluginFileName = NULL;
}

VOID PhpExecuteCallbackForAllPlugins(
    __in PH_PLUGIN_CALLBACK Callback
    )
{
    PPH_AVL_LINKS links;

    links = PhMinimumElementAvlTree(&PhPluginsByName);

    while (links)
    {
        PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);

        PhInvokeCallback(PhGetPluginCallback(plugin, Callback), NULL);

        links = PhSuccessorElementAvlTree(links);
    }
}

PPH_PLUGIN PhRegisterPlugin(
    __in PWSTR Name,
    __in PVOID DllBase,
    __in_opt PPH_PLUGIN_INFORMATION Information
    )
{
    PPH_PLUGIN plugin;
    PPH_AVL_LINKS existingLinks;
    ULONG i;
    PPH_STRING fileName;

    if (DllBase)
    {
        fileName = PhGetDllFileName(DllBase, NULL);

        if (!fileName)
            return NULL;
    }
    else
    {
        // Should only happen for .NET DLLs.

        if (!LoadingPluginFileName)
            return NULL;

        fileName = LoadingPluginFileName;
        PhReferenceObject(fileName);
    }

    plugin = PhAllocate(sizeof(PH_PLUGIN));
    memset(plugin, 0, sizeof(PH_PLUGIN));

    plugin->Name = Name;
    plugin->DllBase = DllBase;

    plugin->FileName = fileName;

    existingLinks = PhAddElementAvlTree(&PhPluginsByName, &plugin->Links);

    if (existingLinks)
    {
        PhFree(plugin);
        return NULL;
    }

    if (LoadingPluginIsClr)
        plugin->Flags |= PH_PLUGIN_FLAG_IS_CLR;

    if (Information)
    {
        plugin->DisplayName = Information->DisplayName;
        plugin->Author = Information->Author;
        plugin->Description = Information->Description;

        if (Information->HasOptions)
            plugin->Flags |= PH_PLUGIN_FLAG_HAS_OPTIONS;
    }

    for (i = 0; i < PluginCallbackMaximum; i++)
        PhInitializeCallback(&plugin->Callbacks[i]);

    return plugin;
}

PPH_PLUGIN PhFindPlugin(
    __in PWSTR Name
    )
{
    PPH_AVL_LINKS links;
    PH_PLUGIN lookupPlugin;

    lookupPlugin.Name = Name;
    links = PhFindElementAvlTree(&PhPluginsByName, &lookupPlugin.Links);

    if (links)
        return CONTAINING_RECORD(links, PH_PLUGIN, Links);
    else
        return NULL;
}

PPH_CALLBACK PhGetPluginCallback(
    __in PPH_PLUGIN Plugin,
    __in PH_PLUGIN_CALLBACK Callback
    )
{
    if (Callback >= PluginCallbackMaximum)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    return &Plugin->Callbacks[Callback];
}

PPH_CALLBACK PhGetGeneralCallback(
    __in PH_GENERAL_CALLBACK Callback
    )
{
    if (Callback >= GeneralCallbackMaximum)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    return &GeneralCallbacks[Callback];
}

ULONG PhPluginReserveIds(
    __in ULONG Count
    )
{
    ULONG nextPluginId;

    nextPluginId = NextPluginId;
    NextPluginId += Count;

    return nextPluginId;
}

BOOLEAN PhPluginAddMenuItem(
    __in PPH_PLUGIN Plugin,
    __in ULONG Location,
    __in_opt PWSTR InsertAfter,
    __in ULONG Id,
    __in PWSTR Text,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem;
    HMENU menu;
    HMENU subMenu;
    ULONG insertIndex;
    ULONG insertAfterCount;
    ULONG textCount;
    WCHAR textBuffer[256];
    MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };

    if (InsertAfter)
        insertAfterCount = (ULONG)wcslen(InsertAfter);

    textCount = (ULONG)wcslen(Text);

    if (textCount > 128)
        return FALSE;

    menuItem = PhAllocate(sizeof(PH_PLUGIN_MENU_ITEM));
    menuItem->Plugin = Plugin;
    menuItem->Id = Id;
    menuItem->Context = Context;

    menu = GetMenu(PhMainWndHandle);
    subMenu = GetSubMenu(menu, Location);

    if (InsertAfter)
    {
        ULONG count;

        menuItemInfo.fMask = MIIM_STRING;
        menuItemInfo.dwTypeData = textBuffer;
        menuItemInfo.cch = sizeof(textBuffer) / sizeof(WCHAR);

        insertIndex = 0;
        count = GetMenuItemCount(subMenu);

        if (count == -1)
            return FALSE;

        for (insertIndex = 0; insertIndex < count; insertIndex++)
        {
            if (GetMenuItemInfo(subMenu, insertIndex, TRUE, &menuItemInfo) && menuItemInfo.dwTypeData)
            {
                if (wcsnicmp(InsertAfter, menuItemInfo.dwTypeData, insertAfterCount) == 0)
                {
                    insertIndex++;
                    break;
                }
            }
        }
    }
    else
    {
        insertIndex = 0;
    }

    if (textCount == 1 && Text[0] == '-')
    {
        menuItem->RealId = 0;

        menuItemInfo.fMask = 0;
        menuItemInfo.fType = MFT_SEPARATOR;
    }
    else
    {
        menuItem->RealId = PhPluginReserveIds(1);

        menuItemInfo.fMask = MIIM_DATA | MIIM_ID | MIIM_STRING;
        menuItemInfo.wID = menuItem->RealId;
        menuItemInfo.dwItemData = (ULONG_PTR)menuItem;
        menuItemInfo.dwTypeData = Text;
    }

    InsertMenuItem(subMenu, insertIndex, TRUE, &menuItemInfo);

    return TRUE;
}

VOID PhPluginGetSystemStatistics(
    __out PPH_PLUGIN_SYSTEM_STATISTICS Statistics
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
}

static VOID NTAPI PhpPluginEMenuItemDeleteFunction(
    __in PPH_EMENU_ITEM Item
    )
{
    PhFree(Item->Context);
}

PPH_EMENU_ITEM PhPluginCreateEMenuItem(
    __in PPH_PLUGIN Plugin,
    __in ULONG Flags,
    __in ULONG Id,
    __in PWSTR Text,
    __in_opt PVOID Context
    )
{
    PPH_EMENU_ITEM item;
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;

    item = PhCreateEMenuItem(Flags, ID_PLUGIN_MENU_ITEM, Text, NULL, NULL);

    pluginMenuItem = PhAllocate(sizeof(PH_PLUGIN_MENU_ITEM));
    pluginMenuItem->Plugin = Plugin;
    pluginMenuItem->Id = Id;
    pluginMenuItem->RealId = 0;
    pluginMenuItem->Context = Context;

    item->Context = pluginMenuItem;
    item->DeleteFunction = PhpPluginEMenuItemDeleteFunction;

    return item;
}

BOOLEAN PhPluginTriggerEMenuItem(
    __in HWND OwnerWindow,
    __in PPH_EMENU_ITEM Item
    )
{
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;

    if (Item->Id != ID_PLUGIN_MENU_ITEM)
        return FALSE;

    pluginMenuItem = Item->Context;

    pluginMenuItem->OwnerWindow = OwnerWindow;

    PhInvokeCallback(PhGetPluginCallback(pluginMenuItem->Plugin, PluginCallbackMenuItem), pluginMenuItem);

    return TRUE;
}

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
#include <phplug.h>

INT NTAPI PhpPluginsCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

VOID PhpExecuteCallbackForAllPlugins(
    __in PH_PLUGIN_CALLBACK Callback
    );

PH_AVL_TREE PhPluginsByName = PH_AVL_TREE_INIT(PhpPluginsCompareFunction);
LIST_ENTRY PhPluginsListHead;

static PH_CALLBACK GeneralCallbacks[GeneralCallbackMaximum];
static PPH_STRING PluginsDirectory;
static ULONG NextPluginId = IDPLUGINS + 1;

VOID PhPluginsInitialization()
{
    ULONG i;

    InitializeListHead(&PhPluginsListHead);

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
    __in PVOID Context
    )
{
    PPH_STRING fileName;

    fileName = PhCreateStringEx(NULL, PluginsDirectory->Length + Information->FileNameLength);
    memcpy(fileName->Buffer, PluginsDirectory->Buffer, PluginsDirectory->Length);
    memcpy(&fileName->Buffer[PluginsDirectory->Length / 2], Information->FileName, Information->FileNameLength);

    LoadLibrary(fileName->Buffer);

    PhDereferenceObject(fileName);

    return TRUE;
}

VOID PhLoadPlugins()
{
    HANDLE pluginsDirectoryHandle;
    PPH_STRING pluginsDirectory;

    pluginsDirectory = PhGetStringSetting(L"PluginsDirectory");

    if (PhStringIndexOfChar(pluginsDirectory, 0, ':') == -1)
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

    // Reload settings because plugins may have added settings.
    if (PhSettingsFileName)
        PhLoadSettings(PhSettingsFileName->Buffer);

    PhpExecuteCallbackForAllPlugins(PluginCallbackLoad);
}

VOID PhUnloadPlugins()
{
    PhpExecuteCallbackForAllPlugins(PluginCallbackUnload);
}

VOID PhpExecuteCallbackForAllPlugins(
    __in PH_PLUGIN_CALLBACK Callback
    )
{
    PLIST_ENTRY listEntry;

    listEntry = PhPluginsListHead.Flink;

    while (listEntry != &PhPluginsListHead)
    {
        PPH_PLUGIN plugin = CONTAINING_RECORD(listEntry, PH_PLUGIN, ListEntry);

        PhInvokeCallback(PhGetPluginCallback(plugin, Callback), NULL);

        listEntry = listEntry->Flink;
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

    plugin = PhAllocate(sizeof(PH_PLUGIN));
    memset(plugin, 0, sizeof(PH_PLUGIN));

    plugin->Name = Name;
    plugin->DllBase = DllBase;

    existingLinks = PhAvlTreeAdd(&PhPluginsByName, &plugin->Links);

    if (existingLinks)
    {
        PhFree(plugin);
        return NULL;
    }

    InsertTailList(&PhPluginsListHead, &plugin->ListEntry);

    if (Information)
    {
        plugin->Author = Information->Author;
        plugin->Description = Information->Description;
    }

    for (i = 0; i < PluginCallbackMaximum; i++)
        PhInitializeCallback(&plugin->Callbacks[i]);

    return plugin;
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

BOOLEAN PhPluginAddMenuItem(
    __in PPH_PLUGIN Plugin,
    __in ULONG Location,
    __in_opt PWSTR InsertAfter,
    __in ULONG Id,
    __in PWSTR Text,
    __in PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem;
    HMENU menu;
    HMENU subMenu;
    ULONG insertIndex;
    ULONG textCount;
    WCHAR textBuffer[256];
    MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };

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
            if (GetMenuItemInfo(subMenu, insertIndex, TRUE, &menuItemInfo))
            {
                if (wcsnicmp(InsertAfter, menuItemInfo.dwTypeData, textCount) == 0)
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

    menuItemInfo.fMask = MIIM_DATA | MIIM_ID | MIIM_STRING;
    menuItemInfo.wID = NextPluginId++;
    menuItemInfo.dwItemData = (ULONG_PTR)menuItem;
    menuItemInfo.dwTypeData = Text;

    InsertMenuItem(subMenu, insertIndex, TRUE, &menuItemInfo);

    return TRUE;
}

/*
 * Process Hacker Extra Plugins -
 *   Plugin Manager
 *
 * Copyright (C) 2016 dmex
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

#include "main.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;

BOOLEAN LastCleanupExpired(
    VOID
    )
{
    ULONG64 lastUpdateTimeTicks = 0;
    LARGE_INTEGER currentUpdateTimeTicks;
    PPH_STRING lastUpdateTimeString;

    PhQuerySystemTime(&currentUpdateTimeTicks);

    lastUpdateTimeString = PhGetStringSetting(SETTING_NAME_LAST_CLEANUP);
    PhStringToInteger64(&lastUpdateTimeString->sr, 0, &lastUpdateTimeTicks);
    PhDereferenceObject(lastUpdateTimeString);

    if (currentUpdateTimeTicks.QuadPart - lastUpdateTimeTicks >= 25 * PH_TICKS_PER_DAY)
    {
        PPH_STRING currentUpdateTimeString = PhFormatUInt64(currentUpdateTimeTicks.QuadPart, FALSE);

        PhSetStringSetting2(SETTING_NAME_LAST_CLEANUP, &currentUpdateTimeString->sr);

        PhDereferenceObject(currentUpdateTimeString);     
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PluginsCleanupDirectoryCallback(
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PH_STRINGREF baseName;
    PPH_STRING fileName;
    PPH_STRING rootDirectoryPath;

    rootDirectoryPath = Context;
    baseName.Buffer = Information->FileName;
    baseName.Length = Information->FileNameLength;

    if (PhEqualStringRef2(&baseName, L".", TRUE) || PhEqualStringRef2(&baseName, L"..", TRUE))
        return TRUE;

    if (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        HANDLE pluginsDirectoryHandle;
        PPH_STRING directoryPath;
        PPH_STRING PluginsDirectoryPath;

        directoryPath = PhCreateString2(&baseName);
        PluginsDirectoryPath = PhConcatStrings(4, rootDirectoryPath->Buffer, L"\\", directoryPath->Buffer, L"\\");

        if (NT_SUCCESS(PhCreateFileWin32(
            &pluginsDirectoryHandle,
            PluginsDirectoryPath->Buffer,
            FILE_GENERIC_READ,
            0,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            UNICODE_STRING pattern = RTL_CONSTANT_STRING(L"*");

            PhEnumDirectoryFile(pluginsDirectoryHandle, &pattern, PluginsCleanupDirectoryCallback, PluginsDirectoryPath);
            NtClose(pluginsDirectoryHandle);
        }
    }
    else if (PhEndsWithStringRef2(&baseName, L".bak", TRUE))
    {
        NTSTATUS status;
        HANDLE fileHandle;
        FILE_DISPOSITION_INFORMATION dispositionInfo;
        IO_STATUS_BLOCK isb;

        fileName = PhCreateStringEx(NULL, rootDirectoryPath->Length + Information->FileNameLength);
        memcpy(fileName->Buffer, rootDirectoryPath->Buffer, rootDirectoryPath->Length);
        memcpy(&fileName->Buffer[rootDirectoryPath->Length / 2], Information->FileName, Information->FileNameLength);

        if (NT_SUCCESS(status = PhCreateFileWin32(
            &fileHandle,
            fileName->Buffer,
            FILE_GENERIC_WRITE | DELETE,
            0,
            0,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            dispositionInfo.DeleteFile = TRUE;

            NtSetInformationFile(
                fileHandle,
                &isb,
                &dispositionInfo,
                sizeof(FILE_DISPOSITION_INFORMATION),
                FileDispositionInformation
                );

            NtClose(fileHandle);
        }

        PhDereferenceObject(fileName);
    }

    return TRUE;
}

VOID PluginsCleanup(
    VOID
    )
{
    static UNICODE_STRING pluginsPattern = RTL_CONSTANT_STRING(L"*");
    HANDLE pluginsDirectoryHandle;
    PPH_STRING pluginsDirectory;
    PPH_STRING PluginsDirectoryPath;
    
    pluginsDirectory = PhGetStringSetting(L"PluginsDirectory");

    if (RtlDetermineDosPathNameType_U(PhGetString(pluginsDirectory)) == RtlPathTypeRelative)
    {
        PluginsDirectoryPath = PhConcatStrings(
            4,
            PhGetString(PhGetApplicationDirectory()),
            L"\\",
            PhGetStringOrEmpty(pluginsDirectory),
            L"\\"
            );
        PhDereferenceObject(pluginsDirectory);
    }
    else
    {
        PluginsDirectoryPath = pluginsDirectory;
    }

    if (NT_SUCCESS(PhCreateFileWin32(
        &pluginsDirectoryHandle,
        PhGetStringOrEmpty(PluginsDirectoryPath),
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PhEnumDirectoryFile(
            pluginsDirectoryHandle, 
            &pluginsPattern,
            PluginsCleanupDirectoryCallback, 
            PluginsDirectoryPath
            );

        NtClose(pluginsDirectoryHandle);
    }
}

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_LIST pluginArgvList = Parameter;

    if (pluginArgvList)
    {
        for (ULONG i = 0; i < pluginArgvList->Count; i++)
        {
            PPH_STRING pluginCommandParam = pluginArgvList->Items[i];

            if (PhEqualString2(pluginCommandParam, L"INSTALL", TRUE))
            {
                //PPH_STRING directory;
                //PPH_STRING path;
                //
                //directory = PH_AUTO(PhGetApplicationDirectory());
                //path = PhaCreateString(L"\\plugins");
                //path = PH_AUTO(PhConcatStringRef2(&directory->sr, &path->sr));
                //
                //if (MoveFileWithProgress(
                //    L"new.plugin",
                //    path->Buffer,
                //    NULL,
                //    NULL,
                //    MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING
                //    ))
                //{
                //
                //}

                //NtTerminateProcess(NtCurrentProcess(), EXIT_SUCCESS);
            }
            else if (PhEqualString2(pluginCommandParam, L"UNINSTALL", TRUE))
            {
                //NtTerminateProcess(NtCurrentProcess(), EXIT_SUCCESS);
            }
        }
    }
    else
    {
        PluginsCleanup();
    }
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI MainMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM pluginMenu;

    if (menuInfo->u.MainMenu.SubMenuIndex != 0)
        return;

    if (pluginMenu = PhFindEMenuItem(menuInfo->Menu, PH_EMENU_FIND_DESCEND, NULL, PHAPP_ID_HACKER_PLUGINS))
    {
        PhInsertEMenuItem(menuInfo->Menu, PhPluginCreateEMenuItem(PluginInstance, 0, pluginMenu->Id, L"&Plugins... (Beta)", NULL), PhIndexOfEMenuItem(menuInfo->Menu, pluginMenu));
        PhRemoveEMenuItem(menuInfo->Menu, pluginMenu, 0);    
    }
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case PHAPP_ID_HACKER_PLUGINS:
        ShowPluginManagerDialog();
        break;
    }
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    //ShowOptionsDialog((HWND)Parameter);
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
                { StringSettingType, SETTING_NAME_TREE_LIST_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_LAST_CLEANUP, L"" },
                { IntegerPairSettingType, SETTING_NAME_WINDOW_POSITION, L"100,100" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WINDOW_SIZE, L"@96|690,540" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Extra Plugins Manager";
            info->Author = L"dmex";
            info->Description = L"Process Hacker plugins from the community.";
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
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
                );

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            PhAddSettings(settings, ARRAYSIZE(settings));
        }
        break;
    }

    return TRUE;
}

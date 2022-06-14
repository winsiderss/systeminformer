/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2017-2021
 *
 */

#include "wndexp.h"
#include "resource.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;

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

static BOOL CALLBACK WepEnumDesktopProc(
    _In_ LPTSTR lpszDesktop,
    _In_ LPARAM lParam
    )
{
    PhAddItemList((PPH_LIST)lParam, PhaCreateString(lpszDesktop)->Buffer);

    return TRUE;
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    if (!menuItem)
        return;

    switch (menuItem->Id)
    {
    case ID_VIEW_WINDOWS:
        {
            WE_WINDOW_SELECTOR selector;

            selector.Type = WeWindowSelectorAll;
            WeShowWindowsDialog(WeGetMainWindowHandle(), &selector);
        }
        break;
    case ID_VIEW_DESKTOPWINDOWS:
        {
            PPH_LIST desktopNames;
            PPH_STRING selectedChoice = NULL;

            desktopNames = PH_AUTO(PhCreateList(4));
            EnumDesktops(GetProcessWindowStation(), WepEnumDesktopProc, (LPARAM)desktopNames);

            if (PhaChoiceDialog(
                WeGetMainWindowHandle(),
                L"Desktop Windows",
                L"Display windows for the following desktop:",
                (PWSTR *)desktopNames->Items,
                desktopNames->Count,
                NULL,
                PH_CHOICE_DIALOG_CHOICE,
                &selectedChoice,
                NULL,
                NULL
                ))
            {
                WE_WINDOW_SELECTOR selector;

                selector.Type = WeWindowSelectorDesktop;
                PhSetReference(&selector.Desktop.DesktopName, selectedChoice);
                WeShowWindowsDialog(WeGetMainWindowHandle(), &selector);
            }
        }
        break;
    case ID_PROCESS_WINDOWS:
        {
            WE_WINDOW_SELECTOR selector;

            selector.Type = WeWindowSelectorProcess;
            selector.Process.ProcessId = ((PPH_PROCESS_ITEM)menuItem->Context)->ProcessId;
            WeShowWindowsDialog(WeGetMainWindowHandle(), &selector);
        }
        break;
    case ID_THREAD_WINDOWS:
        {
            WE_WINDOW_SELECTOR selector;

            selector.Type = WeWindowSelectorThread;
            selector.Thread.ThreadId = ((PPH_THREAD_ITEM)menuItem->Context)->ThreadId;
            WeShowWindowsDialog(WeGetMainWindowHandle(), &selector);
        }
        break;
    }
}

VOID NTAPI MainMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    if (!menuInfo)
        return;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_VIEW)
        return;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_VIEW_SYSTEMINFORMATION))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = ULONG_MAX;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_VIEW_WINDOWS, L"&Windows", NULL), insertIndex);

    if (PhGetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS))
    {
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;

        PhInsertEMenuItem(menuInfo->Menu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_VIEW_DESKTOPWINDOWS, L"Deskto&p Windows...", NULL), insertIndex);
    }
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (!propContext)
        return;

    if (
        propContext->ProcessItem->ProcessId != SYSTEM_IDLE_PROCESS_ID && 
        propContext->ProcessItem->ProcessId != SYSTEM_PROCESS_ID
        )
    {
        WE_WINDOW_SELECTOR selector;

        selector.Type = WeWindowSelectorProcess;
        selector.Process.ProcessId = propContext->ProcessItem->ProcessId;
        WeShowWindowsPropPage(propContext, &selector);
    }
}

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_THREAD_ITEM threadItem;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    if (!menuInfo)
        return;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_THREAD_TOKEN))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = ULONG_MAX;

    menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_THREAD_WINDOWS, L"&Windows", threadItem);
    PhInsertEMenuItem(menuInfo->Menu, menuItem, insertIndex);

    if (!threadItem)
        PhSetDisabledEMenuItem(menuItem);
    if (menuInfo->u.Thread.ProcessId == SYSTEM_IDLE_PROCESS_ID)
        PhSetDisabledEMenuItem(menuItem);
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
                { IntegerSettingType, SETTING_NAME_SHOW_DESKTOP_WINDOWS, L"0" },
                { StringSettingType, SETTING_NAME_WINDOW_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_WINDOWS_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WINDOWS_WINDOW_SIZE, L"@96|690,540" },
                { StringSettingType, SETTING_NAME_WINDOWS_PROPERTY_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_WINDOWS_PROPERTY_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WINDOWS_PROPERTY_SIZE, L"@96|690,540" },
                { StringSettingType, SETTING_NAME_WINDOWS_PROPLIST_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS, L"" },
                { IntegerSettingType, SETTING_NAME_WINDOW_ENUM_ALTERNATE, L"0" },
                { IntegerSettingType, SETTING_NAME_WINDOW_ENABLE_ICONS, L"0" },
                { IntegerSettingType, SETTING_NAME_WINDOW_ENABLE_PREVIEW, L"0" },
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Window Explorer";
            info->Author = L"dmex, wj32";
            info->Description = L"View and manipulate windows.";

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
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing),
                ProcessPropertiesInitializingCallback,
                NULL,
                &ProcessPropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing),
                ThreadMenuInitializingCallback,
                NULL,
                &ThreadMenuInitializingCallbackRegistration
                );

            PhAddSettings(settings, RTL_NUMBER_OF(settings));    
        }
        break;
    }

    return TRUE;
}

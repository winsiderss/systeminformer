/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2023
 *
 */

#include "updater.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE) && PhGetIntegerSetting(SETTING_NAME_UPDATE_AVAILABLE))
    {
        PPH_STRING setting;

        if (setting = PhGetStringSetting(SETTING_NAME_UPDATE_DATA))
        {
            ShowStartupUpdateDialog();

            PhSetIntegerSetting(SETTING_NAME_UPDATE_AVAILABLE, FALSE);
            PhSetStringSetting(SETTING_NAME_UPDATE_DATA, L"");

            PhDereferenceObject(setting);
        }
    }

    // Check if the user want's us to auto-check for updates.
    if (PhGetIntegerSetting(SETTING_NAME_AUTO_CHECK))
    {
        // All good, queue up our update check.
        StartInitialCheck();
    }
}

VOID NTAPI MainMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    //PPH_EMENU_ITEM channelMenuItem;
    //PPH_EMENU_ITEM releaseMenuItem;
    //PPH_EMENU_ITEM previewMenuItem;
    //PPH_EMENU_ITEM canaryMenuItem;
    //PPH_EMENU_ITEM developerMenuItem;

    // Check this menu is the Help menu
    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_HELP)
        return;

    //channelMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, UPDATE_MENUITEM_SWITCH, L"Switch update &channel", NULL);
    //PhInsertEMenuItem(channelMenuItem, releaseMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, UPDATE_SWITCH_RELEASE, L"Release", NULL), ULONG_MAX);
    ////PhInsertEMenuItem(channelMenuItem, previewMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, UPDATE_SWITCH_PREVIEW, L"Preview", NULL), ULONG_MAX);
    //PhInsertEMenuItem(channelMenuItem, canaryMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, UPDATE_SWITCH_CANARY, L"Canary", NULL), ULONG_MAX);
    ////PhInsertEMenuItem(channelMenuItem, developerMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, UPDATE_SWITCH_DEVELOPER, L"Developer", NULL), ULONG_MAX);
    //PhInsertEMenuItem(menuInfo->Menu, channelMenuItem, 0);
    PhInsertEMenuItem(menuInfo->Menu, PhPluginCreateEMenuItem(PluginInstance, 0, UPDATE_MENUITEM_UPDATE, L"Check for &updates", NULL), 0);

    //switch (PhGetPhReleaseChannel())
    //{
    //case PhReleaseChannel:
    //    releaseMenuItem->Flags |= (PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
    //    break;
    ////case PhPreviewChannel:
    ////    previewMenuItem->Flags |= (PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
    ////    break;
    //case PhCanaryChannel:
    //    canaryMenuItem->Flags |= (PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
    //    break;
    ////case PhDeveloperChannel:
    ////    developerMenuItem->Flags |= (PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
    ////    break;
    //default:
    //    break;
    //}
}

VOID NTAPI MenuItemCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;
    PH_RELEASE_CHANNEL channel;
    PPH_UPDATER_CONTEXT context;

    switch (menuItem->Id)
    {
    case UPDATE_MENUITEM_UPDATE:
        ShowUpdateDialog(NULL);
        return;
    case UPDATE_SWITCH_RELEASE:
        channel = PhReleaseChannel;
        break;
    //case UPDATE_SWITCH_PREVIEW:
    //    channel = PhPreviewChannel;
    //    break;
    case UPDATE_SWITCH_CANARY:
        channel = PhCanaryChannel;
        break;
    //case UPDATE_SWITCH_DEVELOPER:
    //    channel = PhDeveloperChannel;
    //    break;
    default:
        return;
    }

    if (PhGetPhReleaseChannel() != channel)
    {
        context = CreateUpdateContext(FALSE);
        context->Channel = channel;
        context->SwitchingChannel = TRUE;
        ShowUpdateDialog(context);
    }
}

VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"Updater",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        OptionsDlgProc,
        NULL
        );
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
                { IntegerSettingType, SETTING_NAME_AUTO_CHECK, L"1" },
                { StringSettingType, SETTING_NAME_LAST_CHECK, L"0" },
                { IntegerPairSettingType, SETTING_NAME_CHANGELOG_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_CHANGELOG_WINDOW_SIZE, L"@96|420,250" },
                { StringSettingType, SETTING_NAME_CHANGELOG_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_CHANGELOG_SORTCOLUMN, L"" },
                { IntegerSettingType, SETTING_NAME_UPDATE_MODE, L"0" },
                { IntegerSettingType, SETTING_NAME_UPDATE_AVAILABLE, L"0" },
                { StringSettingType, SETTING_NAME_UPDATE_DATA, L"" },
                { IntegerSettingType, SETTING_NAME_AUTO_CHECK_PAGE, L"0" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Update Checker";
            info->Author = L"dmex";
            info->Description = L"Plugin for checking new System Informer releases via the Help menu.";

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
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
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            PhAddSettings(settings, ARRAYSIZE(settings));
        }
        break;
    }

    return TRUE;
}

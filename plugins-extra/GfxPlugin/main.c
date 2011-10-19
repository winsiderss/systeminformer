#include "gfxinfo.h"

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    static PH_SETTING_CREATE settings[] =
    {
        { IntegerSettingType, SETTING_NAME_GFX_ALWAYS_ON_TOP, L"0" },
        { IntegerPairSettingType, SETTING_NAME_GFX_WINDOW_POSITION, L"400,400" },
        { IntegerPairSettingType, SETTING_NAME_GFX_WINDOW_SIZE, L"500,400" },
    };

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.GraphicsInfo", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"GPU Information";
            info->Author = L"dmex";
            info->Description = L"Extended functionality for nVidia and ATI graphics cards.";
            info->HasOptions = TRUE;

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
				PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    InitGfx();
}

VOID NTAPI UnloadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    if (NvAPI_Unload)
        NvAPI_Unload();
}

VOID NTAPI MainWindowShowingCallback(
	__in_opt PVOID Parameter,
	__in_opt PVOID Context
	)
{
	PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_VIEW, NULL, GFXINFO_MENUITEM, L"Graphics Information", NULL);
}

VOID NTAPI MenuItemCallback(
	__in_opt PVOID Parameter,
	__in_opt PVOID Context
	)
{
	PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

	switch (menuItem->Id)
	{
	case GFXINFO_MENUITEM:
		{
			ShowDialog();
		}
		break;
	}
}

VOID NTAPI ShowOptionsCallback(
	__in_opt PVOID Parameter,
	__in_opt PVOID Context
	)
{
	//DialogBox(
	//	(HINSTANCE)PluginInstance->DllBase,
	//	MAKEINTRESOURCE(IDD_OPTIONS),
	//	(HWND)Parameter,
	//	OptionsDlgProc
	//	);
}
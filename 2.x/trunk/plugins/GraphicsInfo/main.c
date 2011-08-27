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
            info->Description = L"Extended functionality.";
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
    HMODULE module;

    // Specify the nVidia Library to load.
#ifdef _M_IX86
    module = LoadLibrary(L"nvapi.dll");
#else
    module = LoadLibrary(L"nvapi64.dll");
#endif

    // Check if we loaded our module.
    if (module != NULL)
    {
        // Find our QueryInterface API
        NvAPI_QueryInterface = (P_NvAPI_QueryInterface)GetProcAddress(module, "nvapi_QueryInterface");

        // Check if QueryInterface was found.
        if (NvAPI_QueryInterface != NULL)
        {
            // 50/50 these ID's and API defs are correct.

            // Library initialization functions
            NvAPI_Initialize = (P_NvAPI_Initialize)NvAPI_QueryInterface(0x150E828u);
            NvAPI_Unload = (P_NvAPI_Unload)NvAPI_QueryInterface(0xD22BDD7E);

            // Error Functions
            NvAPI_GetErrorMessage = (P_NvAPI_GetErrorMessage)NvAPI_QueryInterface(0x6C2D048Cu);

            // Handle Functions
            NvAPI_EnumPhysicalGPUs = (P_NvAPI_EnumPhysicalGPUs)NvAPI_QueryInterface(0xE5AC921F);
            NvAPI_EnumNvidiaDisplayHandle = (P_NvAPI_EnumNvidiaDisplayHandle)NvAPI_QueryInterface(0x9ABDD40D);
           
            // Query Functions
            NvAPI_GetUsages = (P_NvAPI_GPU_GetUsages)NvAPI_QueryInterface(0x189A1FDF);
            NvAPI_GetMemoryInfo = (P_NvAPI_GetMemoryInfo)NvAPI_QueryInterface(0x774AA982);
            NvAPI_GetPhysicalGPUsFromDisplay = (P_NvAPI_GetPhysicalGPUsFromDisplay)NvAPI_QueryInterface(0x34EF9506);

            // Driver Info Functions
            NvAPI_GetFullName = (P_NvAPI_GPU_GetFullName)NvAPI_QueryInterface(0xCEEE8E9F);
            NvAPI_GetDisplayDriverVersion = (P_NvAPI_GetDisplayDriverVersion)NvAPI_QueryInterface(0xF951A4D1);
        }
    }
          
    NvInit();
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
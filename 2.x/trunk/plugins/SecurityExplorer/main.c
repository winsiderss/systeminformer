#define SX_MAIN_PRIVATE
#include "explorer.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.SecurityExplorer", Instance, &info);
            
            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Security Explorer";
            info->Author = L"wj32";
            info->Description = L"Manages LSA and SAM objects.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
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

}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case 1:
        {
            SxShowExplorer();
        }
        break;
    }
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"Pagefiles",
        1, L"Security Explorer", NULL);
}

#define SX_MAIN_PRIVATE
#include "explorer.h"

VOID NTAPI LoadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in PVOID Parameter,
    __in PVOID Context
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
            PluginInstance = PhRegisterPlugin(L"ProcessHacker.SecurityExplorer", Instance, NULL);

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
    __in PVOID Parameter,
    __in PVOID Context
    )
{

}

VOID NTAPI MenuItemCallback(
    __in PVOID Parameter,
    __in PVOID Context
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
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"Pagefiles",
        1, L"Security Explorer", NULL);
}

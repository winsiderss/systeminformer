#include <phdk.h>
#include "resource.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

LRESULT CALLBACK MainWndSubclassProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;

HWND ToolBarHandle;
HWND StatusBarHandle;

ULONG IdRangeBase;
ULONG ButtonIdRangeBase;

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
            PH_PLUGIN_INFORMATION info;

            info.DisplayName = L"Toolbar and Statusbar";
            info.Author = L"wj32";
            info.Description = L"Adds a toolbar and a statusbar.";
            info.HasOptions = TRUE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ToolStatus", Instance, &info);
            
            if (!PluginInstance)
                return FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );

            //{
            //    static PH_SETTING_CREATE settings[] =
            //    {
            //        { StringSettingType, SETTING_NAME_PROCESS_LIST, L"\\i*" },
            //        { StringSettingType, SETTING_NAME_SERVICE_LIST, L"\\i*" }
            //    };

            //    PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            //}
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
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;

    InitCommonControlsEx(&icex);
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    static TBBUTTON buttons[2] =
    {
        { I_IMAGENONE, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT },
        { I_IMAGENONE, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT }
    };
    PTBBUTTON button;

    IdRangeBase = PhPluginReserveIds(4);
    ButtonIdRangeBase = IdRangeBase + 2;

    ToolBarHandle = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | BTNS_AUTOSIZE,
        0,
        0,
        0,
        0,
        PhMainWndHandle,
        (HMENU)(IdRangeBase),
        PluginInstance->DllBase,
        NULL
        );

    SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS);

    button = &buttons[0];

    button->idCommand = ButtonIdRangeBase++;
    button->iString = (INT_PTR)L"Test item";
    button++;
    button->idCommand = ButtonIdRangeBase++;
    button->iString = (INT_PTR)L"ASDF";
    button++;

    SendMessage(ToolBarHandle, TB_ADDBUTTONS, sizeof(buttons) / sizeof(TBBUTTON), (LPARAM)buttons);
    SendMessage(ToolBarHandle, WM_SIZE, 0, 0);

    StatusBarHandle = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP | SBARS_TOOLTIPS,
        0,
        0,
        0,
        0,
        PhMainWndHandle,
        (HMENU)(IdRangeBase + 1),
        PluginInstance->DllBase,
        NULL
        );

    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0); 
}

LRESULT CALLBACK MainWndSubclassProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case WM_SIZE:
        {
            RECT padding;
            RECT rect;

            // Get the toolbar and statusbar to resize themselves.
            SendMessage(ToolBarHandle, WM_SIZE, 0, 0);
            SendMessage(StatusBarHandle, WM_SIZE, 0, 0);

            padding.left = 0;
            padding.right = 0;

            GetClientRect(ToolBarHandle, &rect);
            padding.top = rect.bottom;

            GetClientRect(StatusBarHandle, &rect);
            padding.bottom = rect.bottom;

            ProcessHacker_SetLayoutPadding(hWnd, &padding);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

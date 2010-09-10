#include <phdk.h>
#include "resource.h"
#include "toolstatus.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;

HWND ToolBarHandle;
HWND StatusBarHandle;
HIMAGELIST ToolBarImageList;

ULONG IdRangeBase;
ULONG ToolBarIdRangeBase;
ULONG ToolBarIdRangeEnd;

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
    static TBBUTTON buttons[7];
    ULONG buttonIndex;
    ULONG idIndex;
    ULONG imageIndex;

    IdRangeBase = PhPluginReserveIds(2 + 6);

    ToolBarIdRangeBase = IdRangeBase + 2;
    ToolBarIdRangeEnd = ToolBarIdRangeBase + 6;

    ToolBarHandle = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
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
    SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0,
        TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS);
    SendMessage(ToolBarHandle, TB_SETMAXTEXTROWS, 0, 0); // hide the text

    ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
    ImageList_SetImageCount(ToolBarImageList, sizeof(buttons) / sizeof(TBBUTTON));
    PhSetImageListBitmap(ToolBarImageList, 0, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH));
    PhSetImageListBitmap(ToolBarImageList, 1, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT));
    PhSetImageListBitmap(ToolBarImageList, 2, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND));
    PhSetImageListBitmap(ToolBarImageList, 3, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE));
    PhSetImageListBitmap(ToolBarImageList, 4, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION));
    PhSetImageListBitmap(ToolBarImageList, 5, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO));

    SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);

    buttonIndex = 0;
    idIndex = 0;
    imageIndex = 0;

#define DEFINE_BUTTON(Text) \
    buttons[buttonIndex].iBitmap = imageIndex++; \
    buttons[buttonIndex].idCommand = ToolBarIdRangeBase + (idIndex++); \
    buttons[buttonIndex].fsState = TBSTATE_ENABLED; \
    buttons[buttonIndex].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE; \
    buttons[buttonIndex].iString = (INT_PTR)(Text); \
    buttonIndex++

#define DEFINE_SEPARATOR() \
    buttons[buttonIndex].iBitmap = 0; \
    buttons[buttonIndex].idCommand = 0; \
    buttons[buttonIndex].fsState = 0; \
    buttons[buttonIndex].fsStyle = TBSTYLE_SEP; \
    buttons[buttonIndex].iString = 0; \
    buttonIndex++

    DEFINE_BUTTON(L"Refresh");
    DEFINE_BUTTON(L"Options");
    DEFINE_BUTTON(L"Find Handles or DLLs");
    DEFINE_BUTTON(L"System Information");
    DEFINE_SEPARATOR();
    DEFINE_BUTTON(L"Find Window");
    DEFINE_BUTTON(L"Find Window and Thread");

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
    case WM_COMMAND:
        {
            ULONG id = (ULONG)(USHORT)LOWORD(wParam);
            ULONG toolbarId;

            if (id >= ToolBarIdRangeBase && id < ToolBarIdRangeEnd)
            {
                toolbarId = id - ToolBarIdRangeBase;

                switch (toolbarId)
                {
                case TIDC_REFRESH:
                    SendMessage(hWnd, WM_COMMAND, PHID_VIEW_REFRESH, 0);
                    break;
                case TIDC_OPTIONS:
                    SendMessage(hWnd, WM_COMMAND, PHID_HACKER_OPTIONS, 0);
                    break;
                case TIDC_FINDOBJ:
                    SendMessage(hWnd, WM_COMMAND, PHID_HACKER_FINDHANDLESORDLLS, 0);
                    break;
                case TIDC_SYSINFO:
                    SendMessage(hWnd, WM_COMMAND, PHID_VIEW_SYSTEMINFORMATION, 0);
                    break;
                }
            }
        }
        break;
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
            padding.top = rect.bottom + 2;

            GetClientRect(StatusBarHandle, &rect);
            padding.bottom = rect.bottom;

            ProcessHacker_SetLayoutPadding(hWnd, &padding);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

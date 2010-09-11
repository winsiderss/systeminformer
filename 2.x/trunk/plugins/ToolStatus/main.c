#include <phdk.h>
#include "resource.h"
#include "toolstatus.h"
#include <phappresource.h>

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

BOOLEAN TargetingWindow = FALSE;
HWND TargetingCurrentWindow = NULL;
BOOLEAN TargetingWithThread;

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

VOID DrawWindowBorderForTargeting(
    __in HWND hWnd
    )
{
    RECT rect;
    HDC hdc;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        ULONG penWidth = GetSystemMetrics(SM_CXBORDER) * 3;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectObject(hdc, pen);

        brush = GetStockObject(NULL_BRUSH);
        SelectObject(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeleteObject(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

VOID RedrawWindowForTargeting(
    __in HWND hWnd,
    __in BOOLEAN Workaround
    )
{
    if (!RedrawWindow(hWnd, NULL, NULL,
        RDW_INVALIDATE |
        RDW_ERASE | // for toolbar backgrounds and empty forms
        RDW_UPDATENOW |
        RDW_ALLCHILDREN |
        RDW_FRAME
        ) && Workaround)
    {
        // Since the rectangle is just an inversion we can undo it.
        DrawWindowBorderForTargeting(hWnd);
    }
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
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_VIEW_REFRESH, 0);
                    break;
                case TIDC_OPTIONS:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_HACKER_OPTIONS, 0);
                    break;
                case TIDC_FINDOBJ:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_HACKER_FINDHANDLESORDLLS, 0);
                    break;
                case TIDC_SYSINFO:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_VIEW_SYSTEMINFORMATION, 0);
                    break;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == ToolBarHandle)
            {
                switch (hdr->code)
                {
                case TBN_BEGINDRAG:
                    {
                        LPNMTOOLBAR toolbar = (LPNMTOOLBAR)hdr;
                        ULONG id;

                        id = (ULONG)toolbar->iItem - ToolBarIdRangeBase;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(hWnd);
                            // Send the window to the bottom.
                            SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                            TargetingWindow = TRUE;
                            TargetingWithThread = id == TIDC_FINDWINDOWTHREAD;

                            SendMessage(hWnd, WM_MOUSEMOVE, 0, 0);
                        }
                    }
                    break;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                ULONG processId;
                ULONG threadId;

                GetCursorPos(&cursorPos);
                windowOverMouse = WindowFromPoint(cursorPos);

                if (TargetingCurrentWindow != windowOverMouse)
                {
                    if (TargetingCurrentWindow)
                    {
                        // Refresh the old window.
                        RedrawWindowForTargeting(TargetingCurrentWindow, TRUE);
                    }

                    if (windowOverMouse)
                    {
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
                        }
                    }

                    TargetingCurrentWindow = windowOverMouse;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            if (TargetingWindow)
            {
                ULONG processId;
                ULONG threadId;

                BringWindowToTop(hWnd);
                TargetingWindow = FALSE;
                ReleaseCapture();

                if (TargetingCurrentWindow)
                {
                    // Redraw the window we found.
                    RedrawWindowForTargeting(TargetingCurrentWindow, FALSE);
                    threadId = GetWindowThreadProcessId(TargetingCurrentWindow, &processId);

                    if (threadId && processId)
                    {
                        PPH_PROCESS_NODE processNode;

                        processNode = PhFindProcessNode(UlongToHandle(processId));

                        if (processNode)
                        {
                            ProcessHacker_SelectTabPage(hWnd, 0);
                            ProcessHacker_SelectProcessNode(hWnd, processNode);
                        }

                        if (TargetingWithThread)
                        {
                            PPH_PROCESS_PROPCONTEXT propContext;
                            PPH_PROCESS_ITEM processItem;

                            if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                            {
                                if (propContext = PhCreateProcessPropContext(hWnd, processItem))
                                {
                                    PhSetSelectThreadIdProcessPropContext(propContext, UlongToHandle(threadId));
                                    PhShowProcessProperties(propContext);
                                    PhDereferenceObject(propContext);
                                }

                                PhDereferenceObject(processItem);
                            }
                        }
                    }
                }

                goto DefaultWndProc;
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

            ProcessHacker_GetLayoutPadding(hWnd, &padding);

            GetClientRect(ToolBarHandle, &rect);
            padding.top = rect.bottom + 2;

            GetClientRect(StatusBarHandle, &rect);
            padding.bottom = rect.bottom;

            ProcessHacker_SetLayoutPadding(hWnd, &padding);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

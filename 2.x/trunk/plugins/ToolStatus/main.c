/*
 * Process Hacker ToolStatus -
 *   main program
 *
 * Copyright (C) 2011-2013 dmex
 * Copyright (C) 2010-2012 wj32
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

#include "toolstatus.h"
#include "rebar.h"
#include "toolbar.h"
#include "statusbar.h"

#define ID_SEARCH_CLEAR (WM_USER + 1)

static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION LayoutPaddingCallbackRegistration;
static PH_CALLBACK_REGISTRATION TabPageCallbackRegistration;

static HACCEL AcceleratorTable = NULL;
static ULONG TargetingMode = 0;
static HWND TargetingCurrentWindow = NULL;
static BOOLEAN TargetingWindow = FALSE;
static BOOLEAN TargetingCurrentWindowDraw = FALSE;
static BOOLEAN TargetingCompleted = FALSE;

BOOLEAN EnableToolBar = FALSE;
BOOLEAN EnableStatusBar = FALSE;
BOOLEAN EnableSearch = FALSE;
TOOLBAR_DISPLAY_STYLE DisplayStyle = SelectiveText;
PPH_PLUGIN PluginInstance = NULL;

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessesUpdatedCount++;

    if (EnableStatusBar)
        UpdateStatusBar();
}

static VOID NTAPI TabPageUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    INT index = (INT)Parameter;

    if (TextboxHandle)
    {
        switch (index)
        {
        case 0:
            Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+K)");
            break;
        case 1:
            Edit_SetCueBannerText(TextboxHandle, L"Search Services (Ctrl+K)");
            break;
        case 2:
            Edit_SetCueBannerText(TextboxHandle, L"Search Network (Ctrl+K)");
            break;
        default:
            {
                // Disable the textbox if we're on an unsupported tab.
                Edit_SetCueBannerText(TextboxHandle, L"Search Disabled");
            }
            break;
        }
    }
}

static VOID NTAPI LayoutPaddingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_LAYOUT_PADDING_DATA data = (PPH_LAYOUT_PADDING_DATA)Parameter;

    if (EnableToolBar)
    {
        data->Padding.top += ReBarRect.bottom; // Width
    }

    if (EnableStatusBar)
    {
        data->Padding.bottom += StatusBarRect.bottom;
    }
}

static BOOLEAN NTAPI MessageLoopFilter(
    __in PMSG Message,
    __in PVOID Context
    )
{
    if (
        Message->hwnd == PhMainWndHandle ||
        IsChild(PhMainWndHandle, Message->hwnd)
        )
    {
        if (TranslateAccelerator(PhMainWndHandle, AcceleratorTable, Message))
            return TRUE;
    }

    return FALSE;
}

VOID SetRebarMenuLayout(
    VOID
    )
{
    ULONG i = 0;
    ULONG buttonCount = 0;

    buttonCount = (ULONG)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0L, 0L);

    for (i = 0; i < buttonCount; i++)
    {
        TBBUTTONINFO button = { sizeof(TBBUTTONINFO) };
        button.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND;

        // Get settings for first button
        SendMessage(ToolBarHandle, TB_GETBUTTONINFO, i, (LPARAM)&button);

        // Skip separator buttons
        if (button.fsStyle != BTNS_SEP)
        {
            switch (DisplayStyle)
            {
            case ImageOnly:
                button.fsStyle = button.fsStyle | BTNS_AUTOSIZE;
                break;
            case SelectiveText:
                {
                    button.fsStyle = button.fsStyle | BTNS_AUTOSIZE;

                    switch (button.idCommand)
                    {
                    case PHAPP_ID_VIEW_REFRESH:
                    case PHAPP_ID_HACKER_OPTIONS: 
                    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                        button.fsStyle = BTNS_SHOWTEXT;
                        break;
                    }
                }
                break;
            default:
                button.fsStyle = BTNS_SHOWTEXT;
                break;
            }

            // Set updated button info
            SendMessage(ToolBarHandle, TB_SETBUTTONINFO, i, (LPARAM)&button);
        }
    }
}

VOID ApplyToolbarSettings(
    VOID
    )
{
    if (EnableToolBar)
    {
        if (!ReBarHandle)
            RebarCreate(PhMainWndHandle);

        if (!ToolBarHandle)
        {
            ToolBarCreate(PhMainWndHandle);       
            ToolBarCreateImageList(ToolBarHandle);
            ToolbarAddMenuItems(ToolBarHandle);

            // inset the toolbar into the rebar control
            RebarAddMenuItem(ReBarHandle, ToolBarHandle, 0, 23, 0);
        }
        
        SetRebarMenuLayout();

        ShowWindow(ReBarHandle, SW_SHOW); 
        ShowWindow(ToolBarHandle, SW_SHOW);
    }
    else
    {
        if (ToolBarHandle)
            ShowWindow(ToolBarHandle, SW_HIDE);

        if (ReBarHandle)
            ShowWindow(ReBarHandle, SW_HIDE); 

        if (TextboxHandle)
        { 
            // Clear searchbox - ensures treenew filters are inactive when the user disables the toolbar
            Edit_SetSel(TextboxHandle, 0, -1);    
            SetWindowText(TextboxHandle, L"");

            ShowWindow(TextboxHandle, SW_HIDE);
        }

        ToolBarDestroy();
        RebarDestroy();
    }

    if (EnableSearch)
    {
        if (!TextboxHandle)
        {
            ToolbarCreateSearch(ToolBarHandle);
            // inset the edit control into the rebar control
            RebarAddMenuItem(ReBarHandle, TextboxHandle, 0, 20, 200);
        }
 
        ShowWindow(TextboxHandle, SW_SHOW);
    }

    if (EnableStatusBar)
    {
        if (!StatusBarHandle)
            StatusBarCreate(PhMainWndHandle);

        ShowWindow(StatusBarHandle, SW_SHOW);  
    }
    else
    {
        if (StatusBarHandle)
            ShowWindow(StatusBarHandle, SW_HIDE);

        StatusBarDestroy();
    }
}

static VOID DrawWindowBorderForTargeting(
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

static LRESULT CALLBACK MainWndSubclassProc(
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
            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
                {
                    PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());
                    goto DefaultWndProc;
                }
            }

            switch (LOWORD(wParam))
            {
            case PHAPP_ID_ESC_EXIT:
                {
                    // If we're targeting and the user presses the Esc key, cancel the targeting.
                    // We also make sure the window doesn't get closed, by filtering out the message.
                    if (TargetingWindow)
                    {
                        TargetingWindow = FALSE;
                        ReleaseCapture();

                        goto DefaultWndProc;
                    }
                }
                break;
            case ID_SEARCH:
                {
                    // handle keybind Ctrl + K 
                    if (EnableToolBar)
                    {
                        SetFocus(TextboxHandle);
                        Edit_SetSel(TextboxHandle, 0, -1);
                    }

                    goto DefaultWndProc;
                }
                break;
            case ID_SEARCH_CLEAR:
                {
                    SetFocus(TextboxHandle);
                    SetWindowText(TextboxHandle, L"");

                    goto DefaultWndProc;
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == ReBarHandle)
            {
                if (hdr->code == RBN_HEIGHTCHANGE)
                {
                    // HACK: Invoke LayoutPaddingCallback and adjust rebar for multiple rows.
                    PostMessage(PhMainWndHandle, WM_SIZE, 0L, 0L);
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == ToolBarHandle)
            {
                switch (hdr->code)
                {
                case TBN_BEGINDRAG:
                    {
                        LPNMTOOLBAR toolbar = (LPNMTOOLBAR)hdr;
                        ULONG id;

                        id = (ULONG)toolbar->iItem;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD || id == TIDC_FINDWINDOWKILL)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(hWnd);

                            // Send the window to the bottom.
                            SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                            TargetingWindow = TRUE;
                            TargetingCurrentWindow = NULL;
                            TargetingCurrentWindowDraw = FALSE;
                            TargetingCompleted = FALSE;
                            TargetingMode = id;

                            SendMessage(hWnd, WM_MOUSEMOVE, 0, 0);
                        }
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == StatusBarHandle)
            {
                switch (hdr->code)
                {
                case NM_RCLICK:
                    {
                        POINT cursorPos;

                        GetCursorPos(&cursorPos);

                        ShowStatusMenu(&cursorPos);
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
                    if (TargetingCurrentWindow && TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
                            TargetingCurrentWindowDraw = TRUE;
                        }
                        else
                        {
                            TargetingCurrentWindowDraw = FALSE;
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

                TargetingCompleted = TRUE;

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(PhMainWndHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingWindow = FALSE;
                ReleaseCapture();

                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ResolveGhostWindows"))
                    {
                        // This is an undocumented function exported by user32.dll that
                        // retrieves the hung window represented by a ghost window.
                        static HWND (WINAPI *HungWindowFromGhostWindow_I)(
                            __in HWND hWnd
                            );

                        if (!HungWindowFromGhostWindow_I)
                            HungWindowFromGhostWindow_I = PhGetProcAddress(L"user32.dll", "HungWindowFromGhostWindow");

                        if (HungWindowFromGhostWindow_I)
                        {
                            HWND hungWindow = HungWindowFromGhostWindow_I(TargetingCurrentWindow);

                            // The call will have failed if the window wasn't actually a ghost
                            // window.
                            if (hungWindow)
                                TargetingCurrentWindow = hungWindow;
                        }
                    }

                    threadId = GetWindowThreadProcessId(TargetingCurrentWindow, &processId);

                    if (threadId && processId && UlongToHandle(processId) != NtCurrentProcessId())
                    {
                        PPH_PROCESS_NODE processNode;

                        processNode = PhFindProcessNode(UlongToHandle(processId));

                        if (processNode)
                        {
                            ProcessHacker_SelectTabPage(hWnd, 0);
                            ProcessHacker_SelectProcessNode(hWnd, processNode);
                        }

                        switch (TargetingMode)
                        {
                        case TIDC_FINDWINDOWTHREAD:
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
                                else
                                {
                                    PhShowError(hWnd, L"The process (PID %u) does not exist.", processId);
                                }
                            }
                            break;
                        case TIDC_FINDWINDOWKILL:
                            {
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                                {
                                    PhUiTerminateProcesses(hWnd, &processItem, 1);
                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError(hWnd, L"The process (PID %u) does not exist.", processId);
                                }
                            }
                            break;
                        }
                    }
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            if (!TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                // Remove the border on the currently selected window.
                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }
                }

                SetWindowPos(PhMainWndHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingCompleted = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            if (EnableToolBar)
            {
                GetClientRect(ReBarHandle, &ReBarRect);
                SendMessage(ReBarHandle, WM_SIZE, 0, 0);
            }

            if (EnableStatusBar)
            {
                GetClientRect(StatusBarHandle, &StatusBarRect);
                SendMessage(StatusBarHandle, WM_SIZE, 0, 0);
            }

            ProcessHacker_InvalidateLayoutPadding(hWnd);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static VOID NTAPI MainWindowShowingCallback(
     __in_opt PVOID Parameter,
     __in_opt PVOID Context
    )
{       
    ApplyToolbarSettings();

    PhRegisterMessageLoopFilter(MessageLoopFilter, NULL);
    PhRegisterCallback(
        ProcessHacker_GetCallbackLayoutPadding(PhMainWndHandle), 
        LayoutPaddingCallback, 
        NULL, 
        &LayoutPaddingCallbackRegistration
        );

    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0);
}

static VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EnableToolBar = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableToolBar");
    EnableSearch = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableSearch"); 
    EnableStatusBar = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableStatusBar"); 

    StatusMask = PhGetIntegerSetting(L"ProcessHacker.ToolStatus.StatusMask");
    DisplayStyle = (TOOLBAR_DISPLAY_STYLE)PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ToolbarDisplayStyle");     
}

static VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

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
            PH_SETTING_CREATE settings[] =
            {
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableToolBar", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableStatusBar", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableSearch", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableTheme", L"0" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.ResolveGhostWindows", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.StatusMask", L"d" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.ToolbarDisplayStyle", L"1" }
            };

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ToolStatus", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Toolbar and Status Bar";
            info->Author = L"dmex & wj32";
            info->Description = L"Adds a toolbar and a status bar.";
            info->HasOptions = TRUE;

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
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowTabChanged),
                TabPageUpdatedCallback,
                NULL,
                &TabPageCallbackRegistration
                );
            
            PhAddSettings(settings, _countof(settings));

            AcceleratorTable = LoadAccelerators(
                Instance,
                MAKEINTRESOURCE(IDR_MAINWND_ACCEL)
                );
        }
        break;
    }

    return TRUE;
}
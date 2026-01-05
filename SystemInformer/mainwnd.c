/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <mainwnd.h>

#include <cpysave.h>
#include <emenu.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <lsasup.h>
#include <svcsup.h>
#include <workqueue.h>
#include <phsettings.h>

#include <actions.h>
#include <colsetmgr.h>
#include <memsrch.h>
#include <netlist.h>
#include <netprv.h>
#include <notifico.h>
#include <phconsole.h>
#include <phplug.h>
#include <phsvccl.h>
#include <procprv.h>
#include <proctree.h>
#include <secedit.h>
#include <settings.h>
#include <srvlist.h>
#include <srvprv.h>

#include <mainwndp.h>

HWND PhMainWndHandle = NULL;
BOOLEAN PhMainWndExiting = FALSE;
BOOLEAN PhMainWndEarlyExit = FALSE;
WNDPROC PhMainWndProc = PhMwpWndProc;
KPH_LEVEL PhMainWndLevel = KphLevelNone;

PH_PROVIDER_REGISTRATION PhMwpProcessProviderRegistration;
PH_PROVIDER_REGISTRATION PhMwpServiceProviderRegistration;
PH_PROVIDER_REGISTRATION PhMwpNetworkProviderRegistration;
BOOLEAN PhMwpUpdateAutomatically = TRUE;

ULONG PhMwpNotifyIconNotifyMask = 0;
ULONG PhMwpLastNotificationType = 0;
PH_MWP_NOTIFICATION_DETAILS PhMwpLastNotificationDetails;

static BOOLEAN AlwaysOnTop = FALSE;
static BOOLEAN NeedsMaximize = FALSE;
static BOOLEAN DelayedLoadCompleted = FALSE;
static PH_CALLBACK_DECLARE(LayoutPaddingCallback);
static RECT LayoutPadding = { 0, 0, 0, 0 };
static BOOLEAN LayoutPaddingValid = TRUE;
static LONG LayoutWindowDpi = 96;
static LONG LayoutBorderSize = 0;

static HWND TabControlHandle = NULL;
static PPH_LIST PageList = NULL;
static PPH_MAIN_TAB_PAGE CurrentPage = NULL;
static LONG OldTabIndex = 0;

static HMENU SubMenuHandles[5];
static PPH_EMENU SubMenuObjects[5];
static ULONG SelectedUserSessionId = ULONG_MAX;

/**
 * Initializes the main window and data providers.
 *
 * \param ShowCommand The initial show command (e.g., SW_SHOW, SW_HIDE, SW_MAXIMIZE).
 * \return TRUE if initialization succeeded, FALSE otherwise.
 * \remarks Delayed initialization tasks are queued for execution after the window is shown.
 */
BOOLEAN PhMainWndInitialization(
    _In_ LONG ShowCommand
    )
{
    RTL_ATOM windowAtom;
    PH_RECTANGLE windowRectangle;
    RECT windowRect;
    LONG windowDpi;

    // Set FirstRun default settings.

    if (PhGetIntegerSetting(SETTING_FIRST_RUN))
        PhSetIntegerSetting(SETTING_FIRST_RUN, FALSE);

    // Initialize the window class.

    if ((windowAtom = PhMwpInitializeWindowClass()) == INVALID_ATOM)
        return FALSE;

    // Initialize the window size and position.

    memset(&windowRectangle, 0, sizeof(PH_RECTANGLE));
    windowRectangle.Position = PhGetIntegerPairSetting(SETTING_MAIN_WINDOW_POSITION);
    PhRectangleToRect(&windowRect, &windowRectangle);
    windowDpi = PhGetMonitorDpi(NULL, &windowRect);
    windowRectangle.Size = PhGetScalableIntegerPairSetting(SETTING_MAIN_WINDOW_SIZE, TRUE, windowDpi)->Pair;
    PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

    // Initialize the window.

    PhMainWndHandle = CreateWindow(
        MAKEINTATOM(windowAtom),
        NULL,
        WS_OVERLAPPEDWINDOW | (PhEnableDeferredLayout ? 0 : WS_CLIPCHILDREN),
        windowRectangle.Left,
        windowRectangle.Top,
        windowRectangle.Width,
        windowRectangle.Height,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (!PhMainWndHandle)
        return FALSE;

    // Initialize window metrics.
    PhMwpInitializeMetrics(PhMainWndHandle, 0);

    // Initialize window controls.
    PhMwpInitializeControls(PhMainWndHandle);

    // Initialize window fonts.
    PhMwpOnSettingChange(PhMainWndHandle, 0, NULL);

    // Initialize window settings.
    PhMwpLoadSettings(PhMainWndHandle);

    // Initialize window theme.
    PhInitializeWindowTheme(PhMainWndHandle, PhEnableThemeSupport);

    // Initialize window menu.
    PhMwpInitializeMainMenu(PhMainWndHandle);

    // Initialize providers.
    PhMwpInitializeProviders();

    // Perform window layout.
    PhMwpSelectionChangedTabControl(INT_ERROR);

    // Perform main window showing.
    PhMwpShowWindow(ShowCommand);

    // Queue delayed init functions.
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhMwpLoadStage1Worker, PhMainWndHandle);

    return TRUE;
}

/**
 * Window procedure for the main window.
 *
 * \param hWnd Handle to the window.
 * \param uMsg Message identifier.
 * \param wParam First message parameter.
 * \param lParam Second message parameter.
 * \return LRESULT Message result.
 */
LRESULT CALLBACK PhMwpWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhMwpOnDestroy(hWnd);
        }
        break;
    case WM_ENDSESSION:
        {
            PhMwpOnEndSession(hWnd, !!wParam, (ULONG)lParam);
        }
        break;
    case WM_SETTINGCHANGE:
        {
            PhMwpOnSettingChange(hWnd, (ULONG)wParam, (PWSTR)lParam);
        }
        break;
    case WM_COMMAND:
        {
            PhMwpOnCommand(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
        }
        break;
    case WM_SHOWWINDOW:
        {
            PhMwpOnShowWindow(hWnd, !!wParam, (ULONG)lParam);
        }
        break;
    case WM_SYSCOMMAND:
        {
            if (PhMwpOnSysCommand(hWnd, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
        }
        break;
    case WM_MENUCOMMAND:
        {
            PhMwpOnMenuCommand(hWnd, (ULONG)wParam, (HMENU)lParam);
        }
        break;
    case WM_INITMENUPOPUP:
        {
            PhMwpOnInitMenuPopup(hWnd, (HMENU)wParam, LOWORD(lParam), !!HIWORD(lParam));
        }
        break;
    case WM_SIZE:
        {
            PhMwpOnSize(hWnd, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_SIZING:
        {
            PhMwpOnSizing((ULONG)wParam, (PRECT)lParam);
        }
        break;
    case WM_SETFOCUS:
        {
            PhMwpOnSetFocus(hWnd);
        }
        break;
    case WM_TIMER:
        {
            PhMwpOnTimer(hWnd, wParam, lParam);
        }
        break;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhMwpOnNotify((NMHDR *)lParam, &result))
                return result;
        }
        break;
    case WM_DEVICECHANGE:
        {
            PhMwpOnDeviceChanged(hWnd, wParam, lParam);
        }
        break;
    case WM_DPICHANGED:
        {
            PhMwpOnDpiChanged(hWnd, LOWORD(wParam));
        }
        break;
    case WM_NCPAINT:
    case WM_NCACTIVATE:
        {
            if (WindowsVersion >= WINDOWS_10 && !PhEnableThemeSupport)
            {
                LRESULT result = DefWindowProc(hWnd, uMsg, wParam, lParam);
                PhWindowThemeMainMenuBorder(hWnd);
                return result;
            }
        }
        break;
    }

    if (uMsg >= WM_PH_FIRST && uMsg <= WM_PH_LAST)
    {
        return PhMwpOnUserMessage(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Registers the main window class for the main window.
 *
 * \return The atom for the registered window class, or INVALID_ATOM on failure.
 */
RTL_ATOM PhMwpInitializeWindowClass(
    VOID
    )
{
    WNDCLASSEX wcex;
    PPH_STRING className;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = PhMainWndProc;
    wcex.hInstance = NtCurrentImageBase();
    className = PhaGetStringSetting(SETTING_MAIN_WINDOW_CLASS_NAME);
    wcex.lpszClassName = PhGetStringOrDefault(className, SETTING_MAIN_WINDOW_CLASS_NAME);
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);

    if (PhEnableWindowText)
    {
        wcex.hIcon = PhGetApplicationIcon(FALSE);
        wcex.hIconSm = PhGetApplicationIcon(TRUE);
    }

    return RegisterClassEx(&wcex);
}

/**
 * Builds the main window title based on application name, user, privilege level, and elevation.
 *
 * \param KphLevel The current privilege level.
 * \return A string containing the window title, or NULL if window text is disabled.
 */
PPH_STRING PhMwpInitializeWindowTitle(
    _In_ ULONG KphLevel
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING currentUserName;

    if (!PhEnableWindowText)
    {
        PhApplicationName = L" ";
        return NULL;
    }

    PhInitializeStringBuilder(&stringBuilder, 100);
    PhAppendStringBuilder2(&stringBuilder, PhApplicationName);

    if (currentUserName = PhGetSidFullName(PhGetOwnTokenAttributes().TokenSid, TRUE, NULL))
    {
        PhAppendStringBuilder2(&stringBuilder, L" [");
        PhAppendStringBuilder(&stringBuilder, &currentUserName->sr);
        PhAppendCharStringBuilder(&stringBuilder, L']');
        PhDereferenceObject(currentUserName);
    }

    switch (KphLevel)
    {
    case KphLevelMax:
        PhAppendStringBuilder2(&stringBuilder, L"++");
        break;
    case KphLevelHigh:
        PhAppendStringBuilder2(&stringBuilder, L"+");
        break;
    case KphLevelMed:
        PhAppendStringBuilder2(&stringBuilder, L"~");
        break;
    case KphLevelLow:
        PhAppendStringBuilder2(&stringBuilder, L"-");
        break;
    case KphLevelMin:
        PhAppendStringBuilder2(&stringBuilder, L"--");
        break;
    }

    if (PhGetOwnTokenAttributes().ElevationType == TokenElevationTypeFull)
        PhAppendStringBuilder2(&stringBuilder, L" (Administrator)");

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Initializes provider threads for processes, services, and network.
 */
VOID PhMwpInitializeProviders(
    VOID
    )
{
    if (PhCsUpdateInterval == 0)
    {
        PH_SET_INTEGER_CACHED_SETTING(UpdateInterval, PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM);
    }

    // See PhMwpLoadStage1Worker for more details.

    PhInitializeProviderThread(&PhPrimaryProviderThread, PhCsUpdateInterval);
    PhInitializeProviderThread(&PhSecondaryProviderThread, PhCsUpdateInterval);
    PhInitializeProviderThread(&PhTertiaryProviderThread, PhCsUpdateInterval);

    PhRegisterProvider(&PhPrimaryProviderThread, PhProcessProviderUpdate, NULL, &PhMwpProcessProviderRegistration);
    PhRegisterProvider(&PhSecondaryProviderThread, PhServiceProviderUpdate, NULL, &PhMwpServiceProviderRegistration);
    PhRegisterProvider(&PhSecondaryProviderThread, PhNetworkProviderUpdate, NULL, &PhMwpNetworkProviderRegistration);

    PhSetEnabledProvider(&PhMwpProcessProviderRegistration, TRUE);
    PhSetEnabledProvider(&PhMwpServiceProviderRegistration, TRUE);

    PhStartProviderThread(&PhPrimaryProviderThread);
    PhStartProviderThread(&PhSecondaryProviderThread);
    PhStartProviderThread(&PhTertiaryProviderThread);
}

/**
 * Shows the main window, Handles startup parameters, tab selection, system info dialog, and window state.
 *
 * \param ShowCommand The show command (e.g., SW_SHOW, SW_HIDE, SW_MAXIMIZE).
 */
VOID PhMwpShowWindow(
    _In_ LONG ShowCommand
    )
{
    if ((PhStartupParameters.ShowHidden || PhGetIntegerSetting(SETTING_START_HIDDEN)) && PhNfIconsEnabled())
        ShowCommand = SW_HIDE;
    if (PhStartupParameters.ShowVisible)
        ShowCommand = SW_SHOW;

    if (PhGetIntegerSetting(SETTING_MAIN_WINDOW_STATE) == SW_MAXIMIZE)
    {
        if (ShowCommand != SW_HIDE)
        {
            ShowCommand = SW_MAXIMIZE;
        }
        else
        {
            // We can't maximize it while having it hidden. Set it as pending.
            NeedsMaximize = TRUE;
        }
    }

    if (PhPluginsEnabled)
    {
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowShowing), LongToPtr(ShowCommand));
    }

    if (PhStartupParameters.SelectTab)
    {
        PPH_MAIN_TAB_PAGE page;

        if (page = PhMwpFindPage(&PhStartupParameters.SelectTab->sr))
        {
            PhMwpSelectPage(page->Index);
        }
    }
    else
    {
        if (PhGetIntegerSetting(SETTING_MAIN_WINDOW_TAB_RESTORE_ENABLED))
        {
            PhMwpSelectPage(PhGetIntegerSetting(SETTING_MAIN_WINDOW_TAB_RESTORE_INDEX));
        }
    }

    if (PhStartupParameters.SysInfo)
    {
        PhShowSystemInformationDialog(PhGetString(PhStartupParameters.SysInfo));
    }

    if (ShowCommand != SW_HIDE)
    {
        ShowWindow(PhMainWndHandle, ShowCommand);
        UpdateWindow(PhMainWndHandle);

        if (!SetForegroundWindow(PhMainWndHandle))
        {
            PhBringWindowToTop(PhMainWndHandle);
        }
    }

    if (PhGetIntegerSetting(SETTING_MINI_INFO_WINDOW_PINNED))
    {
        PhPinMiniInformation(MiniInfoManualPinType, 1, 0, PH_MINIINFO_LOAD_POSITION, NULL, NULL);
    }
}

/**
 * Applies the update interval to all provider threads.
 *
 * \param Interval The update interval in milliseconds.
 */
VOID PhMwpApplyUpdateInterval(
    _In_ ULONG Interval
    )
{
    PhSetIntervalProviderThread(&PhPrimaryProviderThread, Interval);
    PhSetIntervalProviderThread(&PhSecondaryProviderThread, Interval);
    PhSetIntervalProviderThread(&PhTertiaryProviderThread, Interval);
}

/**
 * Initializes window metrics such as DPI and border size.
 *
 * \param WindowHandle Handle to the window.
 * \param WindowDpi The DPI value for the window.
 */
VOID PhMwpInitializeMetrics(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    )
{
    LayoutWindowDpi = PhGetWindowDpi(WindowHandle);
    LayoutBorderSize = PhGetSystemMetrics(SM_CXBORDER, LayoutWindowDpi);

    PhProcessImageListInitialization(WindowHandle, LayoutWindowDpi);
}

/**
 * Initializes controls for the main window, including tab and tree views.
 *
 * \param WindowHandle Handle to the window.
 */
VOID PhMwpInitializeControls(
    _In_ HWND WindowHandle
    )
{
    ULONG thinRows;
    ULONG treelistBorder;
    ULONG treelistCustomColors;
    ULONG treelistCustomHeaderDraw;
    ULONG treelistCustomDragReorder;
    PH_TREENEW_CREATEPARAMS treelistCreateParams = { sizeof(PH_TREENEW_CREATEPARAMS) };

    thinRows = PhGetIntegerSetting(SETTING_THIN_ROWS) ? TN_STYLE_THIN_ROWS : 0;
    treelistBorder = (PhGetIntegerSetting(SETTING_TREE_LIST_BORDER_ENABLE) && !PhEnableThemeSupport) ? WS_BORDER : 0;
    treelistCustomColors = PhGetIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLORS_ENABLE) ? TN_STYLE_CUSTOM_COLORS : 0;
    treelistCustomHeaderDraw = PhGetIntegerSetting(SETTING_TREE_LIST_ENABLE_HEADER_TOTALS) ? TN_STYLE_CUSTOM_HEADERDRAW : 0;
    treelistCustomDragReorder = PhGetIntegerSetting(SETTING_TREE_LIST_ENABLE_DRAG_REORDER) ? TN_STYLE_DRAG_REORDER_ROWS : 0;

    if (treelistCustomColors)
    {
        treelistCreateParams.TextColor = PhGetIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLOR_TEXT);
        treelistCreateParams.FocusColor = PhGetIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLOR_FOCUS);
        treelistCreateParams.SelectionColor = PhGetIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLOR_SELECTION);
    }

    if (PhGetIntegerSetting(SETTING_TREE_LIST_CUSTOM_ROW_SIZE))
    {
        treelistCreateParams.RowHeight = PhGetIntegerSetting(SETTING_TREE_LIST_CUSTOM_ROW_SIZE);
    }

    TabControlHandle = PhCreateWindow(
        WC_TABCONTROL,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_MULTILINE,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        NULL,
        NULL
        );

    PhMwpProcessTreeNewHandle = PhCreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | TN_STYLE_ANIMATE_DIVIDER |
        thinRows | treelistBorder | treelistCustomColors | treelistCustomHeaderDraw | treelistCustomDragReorder,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        NULL,
        &treelistCreateParams
        );

    PhMwpServiceTreeNewHandle = PhCreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows | treelistBorder | treelistCustomColors,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        NULL,
        &treelistCreateParams
        );

    PhMwpNetworkTreeNewHandle = PhCreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows | treelistBorder | treelistCustomColors,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        NULL,
        &treelistCreateParams
        );

    PageList = PhCreateList(10);

    PhMwpCreateInternalPage(L"Processes", 0, PhMwpProcessesPageCallback);
    PhProcessTreeListInitialization();
    PhInitializeProcessTreeList(PhMwpProcessTreeNewHandle);

    PhMwpCreateInternalPage(L"Services", 0, PhMwpServicesPageCallback);
    PhServiceTreeListInitialization();
    PhInitializeServiceTreeList(PhMwpServiceTreeNewHandle);

    PhMwpCreateInternalPage(L"Network", 0, PhMwpNetworkPageCallback);
    PhNetworkTreeListInitialization();
    PhInitializeNetworkTreeList(PhMwpNetworkTreeNewHandle);

    CurrentPage = PageList->Items[0];
}

/**
 * Worker routine for delayed stage 1 initialization.
 *
 * Performs additional initialization tasks after the main window is shown.
 *
 * \param Parameter The window handle.
 * \return NTSTATUS status code.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhMwpLoadStage1Worker(
    _In_ PVOID Parameter
    )
{
    // Initialize window title (dmex)
    {
        PPH_STRING windowTitle;

        PhMainWndLevel = KphLevelEx(FALSE);

        if (windowTitle = PhMwpInitializeWindowTitle(PhMainWndLevel))
        {
            PhSetWindowText((HWND)Parameter, PhGetString(windowTitle));
            PhDereferenceObject(windowTitle);
        }
    }

    // If the update interval is too large, the user might have to wait a while before seeing some types of
    // process-related data. We force an update by boosting the provider shortly after the program
    // starts up to make things appear more quickly.

    if (PhCsUpdateInterval > PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM)
    {
        PhBoostProvider(&PhMwpProcessProviderRegistration, NULL);
        PhBoostProvider(&PhMwpServiceProviderRegistration, NULL);
    }

    PhNfLoadStage2();

    if (PhGetIntegerSetting(SETTING_ENABLE_LAST_PROCESS_SHUTDOWN))
    {
        // Make sure we get closed late in the shutdown process.
        // This is needed for the shutdown cancel debugging scenario included with Task Manager.
        // Note: Windows excludes system binaries from the shutdown dialog while not excluding
        // programs registered for late shutdown, so when services delay shutdown they won't be shown
        // to the user while we are shown and this has caused some users to blame us instead. (dmex)

        PhSetProcessShutdownParameters(0x1, SHUTDOWN_NORETRY);
    }

    // Allow WM_PH_ACTIVATE to pass through UIPI. (wj32)
    if (PhGetOwnTokenAttributes().Elevated)
    {
        ChangeWindowMessageFilterEx((HWND)Parameter, WM_PH_ACTIVATE, MSGFLT_ADD, NULL);
    }

    // N.B. Devices tab is handled by the HardwareDevices plug-in. The provider is managed internally
    // such that we can handle notifications and dispatch them to other plug-ins. Here we initialize
    // only the device notifications. (jxy-s).
    PhMwpInitializeDeviceNotifications();

    DelayedLoadCompleted = TRUE;
    //PostMessage((HWND)Parameter, WM_PH_DELAYED_LOAD_COMPLETED, 0, 0);

    return STATUS_SUCCESS;
}

/**
 * Handles cleanup and shutdown when the main window is destroyed.
 *
 * \param WindowHandle Handle to the main window being destroyed.
 */
VOID PhMwpOnDestroy(
    _In_ HWND WindowHandle
    )
{
    PhMainWndExiting = TRUE;

    // Notify pages and plugins that we are shutting down.

    PhMwpNotifyAllPages(MainTabPageUpdateAutomaticallyChanged, UlongToPtr(FALSE), NULL); // disable providers (dmex)
    PhMwpNotifyAllPages(MainTabPageDestroy, NULL, NULL);

    if (PhPluginsEnabled)
        PhUnloadPlugins(FALSE);

    if (!PhMainWndEarlyExit)
        PhMwpSaveSettings(WindowHandle);

    PhNfUninitialization();

    PostQuitMessage(0);
}

/**
 * Handles the end session event for the main window.
 *
 * This function is called when the system is ending the current session,
 * such as during shutdown or user logoff. It allows the application to
 * perform any necessary cleanup or state saving before the session ends.
 *
 * \param WindowHandle Handle to the window receiving the end session event.
 * \param SessionEnding TRUE if the session is ending, FALSE otherwise.
 * \param Reason Reason code for the session end (e.g., shutdown, logoff).
 */
VOID PhMwpOnEndSession(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN SessionEnding,
    _In_ ULONG Reason
    )
{
    if (!SessionEnding)
        return;

    PhMainWndExiting = TRUE;

    // Notify pages and plugins that we are shutting down.
    // This callback must only perform the bare minimum cleanup. (dmex)

    PhMwpNotifyAllPages(MainTabPageUpdateAutomaticallyChanged, UlongToPtr(FALSE), NULL); // disable providers (dmex)

    if (PhPluginsEnabled)
        PhUnloadPlugins(TRUE);

    PhMwpSaveSettings(WindowHandle);

    PhExitApplication(STATUS_SUCCESS);
}

/**
 * Handles changes to system or application settings.
 *
 * \param WindowHandle Handle to the window receiving the setting change notification.
 * \param Action Optional action code specifying the type of setting change.
 * \param Metric Optional string specifying the particular metric or setting that changed.
 */
VOID PhMwpOnSettingChange(
    _In_ HWND WindowHandle,
    _In_opt_ ULONG Action,
    _In_opt_ PCWSTR Metric
    )
{
    {
        HFONT oldFont = PhApplicationFont;
        PhApplicationFont = PhInitializeFont(LayoutWindowDpi);
        if (oldFont) DeleteFont(oldFont);
    }

    if (PhGetIntegerSetting(SETTING_ENABLE_MONOSPACE_FONT))
    {
        HFONT oldFont = PhMonospaceFont;
        PhMonospaceFont = PhInitializeMonospaceFont(LayoutWindowDpi);
        if (oldFont) DeleteFont(oldFont);
    }

    if (Action == 0 && Metric)
    {
        // Reload dark theme metrics

        //if (PhEqualStringZ(Metric, L"ImmersiveColorSet", TRUE))
        //{
        //    NOTHING;
        //}
    }

    //if (Action == SPI_SETNONCLIENTMETRICS && Metric && PhEqualStringZ(Metric, L"WindowMetrics", TRUE))
    //{
    //    // Reload non-client metrics
    //}
}

/**
 * Opens a handle to the Service Control Manager (SCM) on the local computer.
 *
 * \param Handle Pointer to a variable that receives the SCM handle.
 * \param DesiredAccess Access mask specifying the desired access rights to the SCM.
 * \param Context Optional context parameter (can be NULL).
 * \return NTSTATUS code indicating success or failure of the operation.
 */
_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenServiceControlManager(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    SC_HANDLE serviceHandle;

    if (serviceHandle = OpenSCManager(NULL, NULL, DesiredAccess))
    {
        *Handle = serviceHandle;
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Closes a handle to the Service Control Manager.
 *
 * \param Handle Optional handle to the Service Control Manager to be closed.
 * \param Release Indicates whether to release associated resources.
 * \param Context Optional context pointer for additional information.
 * \return NTSTATUS code indicating success or failure of the close operation.
 */
_Function_class_(PH_CLOSE_OBJECT)
static NTSTATUS PhpCloseServiceControlManager(
    _In_opt_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    if (Handle)
        PhCloseServiceHandle(Handle);
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenSecurityDummyHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenComDummyAccessPermissionsHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    *Handle = (HANDLE)SD_ACCESSPERMISSIONS;
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenComDummyAccessRestrictionsHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    *Handle = (HANDLE)SD_ACCESSRESTRICTIONS;
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenComDummyLaunchPermissionsHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    *Handle = (HANDLE)SD_LAUNCHPERMISSIONS;
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenComDummyLaunchRestrictionsHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    *Handle = (HANDLE)SD_LAUNCHRESTRICTIONS;
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenSecurityDesktopHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    HDESK desktopHandle;

    if (desktopHandle = OpenDesktop(
        L"Default",
        0,
        FALSE,
        MAXIMUM_ALLOWED
        ))
    {
        *Handle = desktopHandle;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

_Function_class_(PH_CLOSE_OBJECT)
static NTSTATUS PhpCloseSecurityDesktopHandle(
    _In_opt_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    if (Handle)
        CloseDesktop(Handle);
    return STATUS_SUCCESS;
}

_Function_class_(PH_OPEN_OBJECT)
static NTSTATUS PhpOpenSecurityStationHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    HWINSTA stationHandle;

    if (stationHandle = OpenWindowStation(
        L"WinSta0",
        FALSE,
        MAXIMUM_ALLOWED
        ))
    {
        *Handle = stationHandle;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

_Function_class_(PH_CLOSE_OBJECT)
static NTSTATUS PhpCloseSecurityStationHandle(
    _In_opt_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    if (Handle)
        CloseWindowStation(Handle);
    return STATUS_SUCCESS;
}

/**
 * Handles the dump command for a process in the main window.
 *
 * \param WindowHandle Handle to the window receiving the command.
 * \param Id Identifier of the command.
 * \param ProcessItem Pointer to the process item to be dumped.
 */
static VOID PhpMwpOnDumpCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
    {
        PH_LIVE_DUMP_OPTIONS options;

        if (Id == ID_PROCESS_DUMP_CUSTOM)
        {
            PhShowLiveDumpDialog(WindowHandle);
            return;
        }

        memset(&options, 0, sizeof(PH_LIVE_DUMP_OPTIONS));

        options.UseDumpStorageStack = TRUE;
        options.CompressMemoryPages = TRUE;

        switch (Id)
        {
        case ID_PROCESS_DUMP_MINIMAL:
            options.OnlyKernelThreadStacks = TRUE;
            break;
        default: case ID_PROCESS_DUMP_NORMAL:
            break;
        case ID_PROCESS_DUMP_FULL:
            options.IncludeHypervisorPages = TRUE;
            options.IncludeNonEssentialHypervisorPages = TRUE;
            break;
        }

        PhUiCreateLiveDump(WindowHandle, &options);
    }
    else
    {
        ULONG dumpType;

        if (Id == ID_PROCESS_DUMP_CUSTOM)
        {
            PhShowCreateDumpFileProcessDialog(WindowHandle, ProcessItem);
            return;
        }

        switch (Id)
        {
        case ID_PROCESS_DUMP_MINIMAL:
            dumpType =
                MiniDumpWithDataSegs |
                MiniDumpWithUnloadedModules |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory;
            break;
        case ID_PROCESS_DUMP_LIMITED:
            dumpType =
                MiniDumpWithFullMemory |
                MiniDumpWithUnloadedModules |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory;
        default: case ID_PROCESS_DUMP_NORMAL:
            // task manager uses these flags (wj32)
            dumpType =
                MiniDumpWithFullMemory |
                MiniDumpWithHandleData |
                MiniDumpWithUnloadedModules |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory |
                MiniDumpWithIptTrace;
            break;
        case ID_PROCESS_DUMP_FULL:
            dumpType =
                MiniDumpWithDataSegs |
                MiniDumpWithFullMemory |
                MiniDumpWithHandleData |
                MiniDumpWithUnloadedModules |
                MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpWithProcessThreadData |
                MiniDumpWithPrivateReadWriteMemory |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithCodeSegs |
                MiniDumpWithFullAuxiliaryState |
                MiniDumpWithPrivateWriteCopyMemory |
                MiniDumpIgnoreInaccessibleMemory |
                MiniDumpWithTokenInformation |
                MiniDumpWithModuleHeaders |
                MiniDumpWithAvxXStateContext |
                MiniDumpWithIptTrace;
            break;
        }

        PhUiCreateDumpFileProcess(WindowHandle, ProcessItem, dumpType);
    }
}

/**
 * Handles command messages for the main window.
 *
 * \param WindowHandle Handle to the main window receiving the command.
 * \param Id Identifier of the command to process.
 */
VOID PhMwpOnCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_ESC_EXIT:
        {
            if (PhGetIntegerSetting(SETTING_HIDE_ON_CLOSE))
            {
                if (PhNfIconsEnabled())
                    ShowWindow(WindowHandle, SW_HIDE);
            }
            else if (PhGetIntegerSetting(SETTING_CLOSE_ON_ESCAPE))
            {
                SystemInformer_Destroy();
            }
        }
        break;
    case ID_HACKER_RUN:
        {
            PhShowRunFileDialog(WindowHandle);
        }
        break;
    case ID_HACKER_RUNAS:
        {
            PhShowRunAsDialog(WindowHandle, NULL);
        }
        break;
    case ID_HACKER_RUNASPACKAGE:
        {
            PhShowRunAsPackageDialog(WindowHandle);
        }
        break;
    case ID_HACKER_SHOWDETAILSFORALLPROCESSES:
        {
            SystemInformer_PrepareForEarlyShutdown();

            if (NT_SUCCESS(PhShellProcessHacker(
                WindowHandle,
                L"-v -newinstance",
                SW_SHOW,
                PH_SHELL_EXECUTE_ADMIN,
                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                0,
                NULL
                )))
            {
                SystemInformer_Destroy();
            }
            else
            {
                SystemInformer_CancelEarlyShutdown();
            }
        }
        break;
    case ID_HACKER_SAVE:
        {
            static PH_FILETYPE_FILTER filters[] =
            {
                { L"Text files (*.txt;*.log)", L"*.txt;*.log" },
                { L"Comma-separated values (*.csv)", L"*.csv" },
                { L"All files (*.*)", L"*.*" }
            };
            PVOID fileDialog = PhCreateSaveFileDialog();
            PH_FORMAT format[3];

            PhInitFormatS(&format[0], L"System Informer ");
            PhInitFormatSR(&format[1], CurrentPage->Name);
            PhInitFormatS(&format[2], L".txt");

            PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
            PhSetFileDialogFileName(fileDialog, PH_AUTO_T(PH_STRING, PhFormat(format, 3, 60))->Buffer);

            if (PhShowFileDialog(WindowHandle, fileDialog))
            {
                NTSTATUS status;
                PPH_STRING fileName;
                ULONG filterIndex;
                PPH_FILE_STREAM fileStream;

                fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                filterIndex = PhGetFileDialogFilterIndex(fileDialog);

                if (NT_SUCCESS(status = PhCreateFileStream(
                    &fileStream,
                    fileName->Buffer,
                    FILE_GENERIC_WRITE,
                    FILE_SHARE_READ,
                    FILE_OVERWRITE_IF,
                    0
                    )))
                {
                    ULONG mode;
                    PH_MAIN_TAB_PAGE_EXPORT_CONTENT exportContent;

                    if (filterIndex == 2)
                        mode = PH_EXPORT_MODE_CSV;
                    else
                        mode = PH_EXPORT_MODE_TABS;

                    PhWriteStringAsUtf8FileStream(fileStream, (PPH_STRINGREF)&PhUnicodeByteOrderMark);

                    if (mode != PH_EXPORT_MODE_CSV)
                    {
                        PhWritePhTextHeader(fileStream);
                    }

                    exportContent.FileStream = fileStream;
                    exportContent.Mode = mode;
                    CurrentPage->Callback(CurrentPage, MainTabPageExportContent, &exportContent, NULL);

                    PhDereferenceObject(fileStream);
                }

                if (!NT_SUCCESS(status))
                    PhShowStatus(WindowHandle, L"Unable to create the file", status, 0);
            }

            PhFreeFileDialog(fileDialog);
        }
        break;
    case ID_HACKER_FINDHANDLESORDLLS:
        {
            PhShowFindObjectsDialog(WindowHandle);
        }
        break;
    case ID_HACKER_OPTIONS:
        {
            PhShowOptionsDialog(WindowHandle);
        }
        break;
    case ID_COMPUTER_LOCK:
    case ID_COMPUTER_LOGOFF:
    case ID_COMPUTER_SLEEP:
    case ID_COMPUTER_HIBERNATE:
    case ID_COMPUTER_RESTART:
    case ID_COMPUTER_RESTARTADVOPTIONS:
    case ID_COMPUTER_RESTARTBOOTOPTIONS:
    case ID_COMPUTER_RESTARTFWOPTIONS:
    case ID_COMPUTER_SHUTDOWN:
    case ID_COMPUTER_SHUTDOWNHYBRID:
    case ID_COMPUTER_RESTART_NATIVE:
    case ID_COMPUTER_SHUTDOWN_NATIVE:
    case ID_COMPUTER_RESTART_CRITICAL:
    case ID_COMPUTER_SHUTDOWN_CRITICAL:
    case ID_COMPUTER_RESTART_UPDATE:
    case ID_COMPUTER_SHUTDOWN_UPDATE:
    case ID_COMPUTER_RESTARTWDOSCAN:
        PhMwpExecuteComputerCommand(WindowHandle, Id);
        break;
    case ID_HACKER_EXIT:
        SystemInformer_Destroy();
        break;
    case ID_VIEW_SYSTEMINFORMATION:
        PhShowSystemInformationDialog(NULL);
        break;
    case ID_NOTIFICATIONS_ENABLEALL:
    case ID_NOTIFICATIONS_DISABLEALL:
    case ID_NOTIFICATIONS_NEWPROCESSES:
    case ID_NOTIFICATIONS_TERMINATEDPROCESSES:
    case ID_NOTIFICATIONS_NEWSERVICES:
    case ID_NOTIFICATIONS_STARTEDSERVICES:
    case ID_NOTIFICATIONS_STOPPEDSERVICES:
    case ID_NOTIFICATIONS_DELETEDSERVICES:
    case ID_NOTIFICATIONS_MODIFIEDSERVICES:
    case ID_NOTIFICATIONS_ARRIVEDDEVICES:
    case ID_NOTIFICATIONS_REMOVEDDEVICES:
        {
            PhMwpExecuteNotificationMenuCommand(WindowHandle, Id);
        }
        break;
    case ID_NOTIFICATIONS_RESETPERSISTLAYOUT:
    case ID_NOTIFICATIONS_ENABLEDELAYSTART:
    case ID_NOTIFICATIONS_ENABLEPERSISTLAYOUT:
    case ID_NOTIFICATIONS_ENABLETRANSPARENTICONS:
    case ID_NOTIFICATIONS_ENABLESINGLECLICKICONS:
        {
            PhMwpExecuteNotificationSettingsMenuCommand(WindowHandle, Id);
        }
        break;
    case ID_VIEW_COLLAPSEALL:
        {
            PhExpandAllProcessNodes(FALSE);
        }
        break;
    case ID_VIEW_EXPANDALL:
        {
            PhExpandAllProcessNodes(TRUE);
        }
        break;
    case ID_VIEW_HIDEPROCESSESFROMOTHERUSERS:
        {
            PhMwpToggleCurrentUserProcessTreeFilter();
        }
        break;
    case ID_VIEW_HIDESIGNEDPROCESSES:
        {
            PhMwpToggleSignedProcessTreeFilter(WindowHandle);
        }
        break;
    case ID_VIEW_HIDEMICROSOFTPROCESSES:
        {
            PhMwpToggleMicrosoftProcessTreeFilter();
        }
        break;
    case ID_VIEW_SCROLLTONEWPROCESSES:
        {
            PH_SET_INTEGER_CACHED_SETTING(ScrollToNewProcesses, !PhCsScrollToNewProcesses);
        }
        break;
    case ID_VIEW_SORTCHILDPROCESSES:
        {
            PH_SET_INTEGER_CACHED_SETTING(SortChildProcesses, !PhCsSortChildProcesses);
        }
        break;
    case ID_VIEW_SORTROOTPROCESSES:
        {
            PH_SET_INTEGER_CACHED_SETTING(SortRootProcesses, !PhCsSortRootProcesses);
        }
        break;
    case ID_VIEW_SHOWCPUBELOW001:
        {
            PH_SET_INTEGER_CACHED_SETTING(ShowCpuBelow001, !PhCsShowCpuBelow001);
            PhInvalidateAllProcessNodes();
        }
        break;
    case ID_VIEW_HIDEDRIVERSERVICES:
        {
            PhMwpToggleDriverServiceTreeFilter();
        }
        break;
    case ID_VIEW_HIDEMICROSOFTSERVICES:
        {
            PhMwpToggleMicrosoftServiceTreeFilter();
        }
        break;
    case ID_VIEW_HIDEWAITINGCONNECTIONS:
        {
            PhMwpToggleNetworkWaitingConnectionTreeFilter();
        }
        break;
    case ID_VIEW_ALWAYSONTOP:
        {
            AlwaysOnTop = !AlwaysOnTop;
            SetWindowPos(WindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            PhSetIntegerSetting(SETTING_MAIN_WINDOW_ALWAYS_ON_TOP, AlwaysOnTop);

            PhWindowNotifyTopMostEvent(AlwaysOnTop);
        }
        break;
    case ID_OPACITY_10:
    case ID_OPACITY_20:
    case ID_OPACITY_30:
    case ID_OPACITY_40:
    case ID_OPACITY_50:
    case ID_OPACITY_60:
    case ID_OPACITY_70:
    case ID_OPACITY_80:
    case ID_OPACITY_90:
    case ID_OPACITY_OPAQUE:
        {
            ULONG opacity;

            opacity = PH_ID_TO_OPACITY(Id);
            PhSetIntegerSetting(SETTING_MAIN_WINDOW_OPACITY, opacity);
            PhSetWindowOpacity(WindowHandle, opacity);
        }
        break;
    case ID_VIEW_REFRESH:
        {
            PhBoostProvider(&PhMwpProcessProviderRegistration, NULL);
            PhBoostProvider(&PhMwpServiceProviderRegistration, NULL);

            // Note: Don't boost the network provider unless it's currently enabled. (dmex)
            if (PhGetEnabledProvider(&PhMwpNetworkProviderRegistration))
            {
                PhBoostProvider(&PhMwpNetworkProviderRegistration, NULL);
            }
        }
        break;
    case ID_UPDATEINTERVAL_FAST:
    case ID_UPDATEINTERVAL_NORMAL:
    case ID_UPDATEINTERVAL_BELOWNORMAL:
    case ID_UPDATEINTERVAL_SLOW:
    case ID_UPDATEINTERVAL_VERYSLOW:
        {
            ULONG interval;

            switch (Id)
            {
            case ID_UPDATEINTERVAL_FAST:
                interval = 500;
                break;
            case ID_UPDATEINTERVAL_NORMAL:
                interval = 1000;
                break;
            case ID_UPDATEINTERVAL_BELOWNORMAL:
                interval = 2000;
                break;
            case ID_UPDATEINTERVAL_SLOW:
                interval = 5000;
                break;
            case ID_UPDATEINTERVAL_VERYSLOW:
                interval = 10000;
                break;
            default:
                interval = 1000;
                break;
            }

            PH_SET_INTEGER_CACHED_SETTING(UpdateInterval, interval);
            PhMwpApplyUpdateInterval(interval);
        }
        break;
    case ID_VIEW_UPDATEAUTOMATICALLY:
        {
            PhMwpUpdateAutomatically = !PhMwpUpdateAutomatically;

            PhMwpNotifyAllPages(MainTabPageUpdateAutomaticallyChanged, UlongToPtr(PhMwpUpdateAutomatically), NULL);

            if (PhPluginsEnabled)
            {
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackUpdateAutomatically), UlongToPtr(PhMwpUpdateAutomatically));
            }
        }
        break;
    case ID_TOOLS_USER_LIST:
        {
            PhShowUserListDialog(WindowHandle);
        }
        break;
    case ID_TOOLS_THREADSTACKS:
        {
            PhShowThreadStacksDialog(WindowHandle);
        }
        break;
    case ID_TOOLS_CREATESERVICE:
        {
            PhShowCreateServiceDialog(WindowHandle);
        }
        break;
    case ID_TOOLS_ZOMBIEPROCESSES:
        {
            PhShowZombieProcessesDialog();
        }
        break;
    case ID_TOOLS_INSPECTEXECUTABLEFILE:
        {
            static PH_FILETYPE_FILTER filters[] =
            {
                { L"Executable files (*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.mui;*.mun;*.efi;*.pdb)", L"*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.mui;*.mun;*.efi;*.pdb" },
                { L"All files (*.*)", L"*.*" }
            };
            PVOID fileDialog = PhCreateOpenFileDialog();

            PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
            PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_NOPATHVALIDATE | PH_FILEDIALOG_DONTADDTORECENT);

            if (PhShowFileDialog(WindowHandle, fileDialog))
            {
                PhShellExecuteUserString(
                    WindowHandle,
                    SETTING_PROGRAM_INSPECT_EXECUTABLES,
                    PH_AUTO_T(PH_STRING, PhGetFileDialogFileName(fileDialog))->Buffer,
                    FALSE,
                    L"Make sure the PE Viewer executable file is present."
                    );
            }

            PhFreeFileDialog(fileDialog);
        }
        break;
    case ID_TOOLS_PAGEFILES:
        {
            PhShowPagefilesDialog(WindowHandle);
        }
        break;
    case ID_TOOLS_LIVEDUMP:
        {
            PhShowLiveDumpDialog(WindowHandle);
        }
        break;
    case ID_TOOLS_STARTTASKMANAGER:
        {
            extern BOOLEAN PhpIsDefaultTaskManager(VOID); // options.c (dmex)
            PPH_STRING taskmgrFileName;
            PWSTR taskmgrCommandLine;

            taskmgrFileName = PH_AUTO(PhGetSystemDirectoryWin32Z(L"\\taskmgr.exe"));
            taskmgrCommandLine = PhGetIntegerSetting(SETTING_TASKMGR_WINDOW_STATE) ? L" -d" : NULL;

            if (!PhpIsDefaultTaskManager() && PhGetIntegerSetting(SETTING_ENABLE_SHELL_EXECUTE_SKIP_IFEO_DEBUGGER))
            {
                PhShellExecuteEx(
                    WindowHandle,
                    PhGetString(taskmgrFileName),
                    taskmgrCommandLine,
                    NULL,
                    SW_SHOW,
                    0,
                    0,
                    NULL
                    );
            }
            else
            {
                if (WindowsVersion >= WINDOWS_8 && !PhGetOwnTokenAttributes().Elevated)
                {
                    if (PhUiConnectToPhSvc(WindowHandle, FALSE))
                    {
                        PhSvcCallCreateProcessIgnoreIfeoDebugger(PhGetString(taskmgrFileName), taskmgrCommandLine);
                        PhUiDisconnectFromPhSvc();
                    }
                }
                else
                {
                    PhCreateProcessIgnoreIfeoDebugger(PhGetString(taskmgrFileName), taskmgrCommandLine);
                }
            }
        }
        break;
    case ID_TOOLS_STARTRESOURCEMONITOR:
        {
            PPH_STRING perfmonFileName;

            perfmonFileName = PH_AUTO(PhGetSystemDirectoryWin32Z(L"\\perfmon.exe"));

            if (PhGetIntegerSetting(SETTING_ENABLE_SHELL_EXECUTE_SKIP_IFEO_DEBUGGER))
            {
                PhShellExecuteEx(
                    WindowHandle,
                    PhGetString(perfmonFileName),
                    L" /res",
                    NULL,
                    SW_SHOW,
                    0,
                    0,
                    NULL
                    );
            }
            else
            {
                if (!PhGetOwnTokenAttributes().Elevated)
                {
                    if (PhUiConnectToPhSvc(WindowHandle, FALSE))
                    {
                        PhSvcCallCreateProcessIgnoreIfeoDebugger(PhGetString(perfmonFileName), L" /res");
                        PhUiDisconnectFromPhSvc();
                    }
                }
                else
                {
                    PhCreateProcessIgnoreIfeoDebugger(PhGetString(perfmonFileName), L" /res");
                }
            }
        }
        break;
    case ID_TOOLS_SHUTDOWNWSLPROCESSES:
        {
            NTSTATUS status;
            PPH_STRING perfmonFileName;

            perfmonFileName = PH_AUTO(PhGetSystemDirectoryWin32Z(L"\\wsl.exe"));

            status = PhShellExecuteEx(
                WindowHandle,
                PhGetString(perfmonFileName),
                L" --shutdown",
                NULL,
                SW_SHOW,
                0,
                0,
                NULL
                );

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(WindowHandle, L"Unable to shutdown WSL instances.", status, 0);
            }
        }
        break;
    case ID_TOOLS_SCM_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"Service Control Manager",
                L"SCManager",
                PhpOpenServiceControlManager,
                PhpCloseServiceControlManager,
                NULL
                );
        }
        break;
    case ID_TOOLS_PWR_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"Current Power Scheme",
                L"PowerDefault",
                PhpOpenSecurityDummyHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_RDP_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"Terminal Server Listener",
                L"RdpDefault",
                PhpOpenSecurityDummyHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_COM_ACCESS_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"COM Access Permissions",
                L"ComAccess",
                PhpOpenComDummyAccessPermissionsHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_COM_ACCESS_RESTRICTIONS:
        {
            PhEditSecurity(
                NULL,
                L"COM Access Restrictions",
                L"ComAccess",
                PhpOpenComDummyAccessRestrictionsHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_COM_LAUNCH_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"COM Launch Permissions",
                L"ComLaunch",
                PhpOpenComDummyLaunchPermissionsHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_COM_LAUNCH_RESTRICTIONS:
        {
            PhEditSecurity(
                NULL,
                L"COM Launch Restrictions",
                L"ComLaunch",
                PhpOpenComDummyLaunchRestrictionsHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_WMI_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"WMI Root Namespace",
                L"WmiDefault",
                PhpOpenSecurityDummyHandle,
                NULL,
                NULL
                );
        }
        break;
    case ID_TOOLS_DESKTOP_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"Current Window Desktop",
                L"Desktop",
                PhpOpenSecurityDesktopHandle,
                PhpCloseSecurityDesktopHandle,
                NULL
                );
        }
        break;
    case ID_TOOLS_STATION_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"Current Window Station",
                L"WindowStation",
                PhpOpenSecurityStationHandle,
                PhpCloseSecurityStationHandle,
                NULL
                );
        }
        break;
    case ID_USER_CONNECT:
        {
            PhUiConnectSession(WindowHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_DISCONNECT:
        {
            PhUiDisconnectSession(WindowHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_LOGOFF:
        {
            PhUiLogoffSession(WindowHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_REMOTECONTROL:
        {
            PhShowSessionShadowDialog(WindowHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_SENDMESSAGE:
        {
            PhShowSessionSendMessageDialog(WindowHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_PROPERTIES:
        {
            PhShowSessionProperties(WindowHandle, SelectedUserSessionId);
        }
        break;
    case ID_HELP_LOG:
        {
            PhShowLogDialog();
        }
        break;
    case ID_HELP_DONATE:
        {
            //PhShellExecute(WindowHandle, L"https://sourceforge.net/project/project_donations.php?group_id=242527", NULL);
        }
        break;
    case ID_HELP_DEBUGCONSOLE:
        {
            PhShowDebugConsole();
        }
        break;
    case ID_HELP_ABOUT:
        {
            PhShowAboutDialog(WindowHandle);
        }
        break;
    case ID_PROCESS_TERMINATE:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);

                if (PhUiTerminateProcesses(WindowHandle, processes, numberOfProcesses))
                    PhDeselectAllProcessNodes();

                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_PROCESS_TERMINATETREE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);

                if (PhUiTerminateTreeProcess(WindowHandle, processItem))
                    PhDeselectAllProcessNodes();

                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_SUSPEND:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);
                PhUiSuspendProcesses(WindowHandle, processes, numberOfProcesses);
                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_PROCESS_SUSPENDTREE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiSuspendTreeProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_RESUME:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);
                PhUiResumeProcesses(WindowHandle, processes, numberOfProcesses);
                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_PROCESS_RESUMETREE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiResumeTreeProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_FREEZE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiFreezeTreeProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_THAW:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiThawTreeProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_RESTART:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);

                // Note: The current process is a special case (dmex)
                if (processItem->ProcessId == NtCurrentProcessId())
                {
                    SystemInformer_PrepareForEarlyShutdown();

                    if (NT_SUCCESS(PhShellProcessHacker(
                        WindowHandle,
                        NULL,
                        SW_SHOW,
                        PH_SHELL_EXECUTE_DEFAULT,
                        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                        0,
                        NULL
                        )))
                    {
                        SystemInformer_Destroy();
                    }
                    else
                    {
                        SystemInformer_CancelEarlyShutdown();
                    }
                }
                else
                {
                    if (PhUiRestartProcess(WindowHandle, processItem))
                    {
                        PhDeselectAllProcessNodes();
                    }
                }

                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_DUMP_MINIMAL:
    case ID_PROCESS_DUMP_LIMITED:
    case ID_PROCESS_DUMP_NORMAL:
    case ID_PROCESS_DUMP_FULL:
    case ID_PROCESS_DUMP_CUSTOM:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhpMwpOnDumpCommand(WindowHandle, Id, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_DEBUG:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiDebugProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_VIRTUALIZATION:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiSetVirtualizationProcess(
                    WindowHandle,
                    processItem,
                    !PhMwpSelectedProcessVirtualizationEnabled
                    );
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_AFFINITY:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhShowProcessAffinityDialog(WindowHandle, processItem, NULL);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_ACTIVITY:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiSetActivityModeration(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_SETCRITICAL:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiSetCriticalProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_DETACHFROMDEBUGGER:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiDetachFromDebuggerProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_GDIHANDLES:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhShowGdiHandlesDialog(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_HEAPS:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhShowProcessHeapsDialog(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_LOCKS:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhShowProcessLocksDialog(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PAGEPRIORITY_VERYLOW:
    case ID_PAGEPRIORITY_LOW:
    case ID_PAGEPRIORITY_MEDIUM:
    case ID_PAGEPRIORITY_BELOWNORMAL:
    case ID_PAGEPRIORITY_NORMAL:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                ULONG pagePriority;

                switch (Id)
                {
                    case ID_PAGEPRIORITY_VERYLOW:
                        pagePriority = MEMORY_PRIORITY_VERY_LOW;
                        break;
                    case ID_PAGEPRIORITY_LOW:
                        pagePriority = MEMORY_PRIORITY_LOW;
                        break;
                    case ID_PAGEPRIORITY_MEDIUM:
                        pagePriority = MEMORY_PRIORITY_MEDIUM;
                        break;
                    case ID_PAGEPRIORITY_BELOWNORMAL:
                        pagePriority = MEMORY_PRIORITY_BELOW_NORMAL;
                        break;
                    case ID_PAGEPRIORITY_NORMAL:
                        pagePriority = MEMORY_PRIORITY_NORMAL;
                        break;
                }

                PhReferenceObject(processItem);
                PhUiSetPagePriorityProcess(WindowHandle, processItem, pagePriority);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_REDUCEWORKINGSET:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);
                PhUiReduceWorkingSetProcesses(WindowHandle, processes, numberOfProcesses);
                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_MISCELLANEOUS_RUNAS:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                //PPH_STRING fileNameWin32;
                //
                //fileNameWin32 = PhGetFileName(processItem->FileName);
                //
                //if (!PhIsNullOrEmptyString(fileNameWin32))
                //{
                //    PhSetStringSetting2(SETTING_RUN_AS_PROGRAM, &fileNameWin32->sr);
                //    PhDereferenceObject(fileNameWin32);
                //
                //    PhShowRunAsDialog(WindowHandle, NULL);
                //}
                //else
                //{
                //    PhShowStatus(WindowHandle, L"Unable to locate the file.", STATUS_NOT_FOUND, 0);
                //}

                PhShowRunAsDialog(WindowHandle, NULL);
            }
        }
        break;
    case ID_MISCELLANEOUS_RUNASTHISUSER:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhShowRunAsDialog(WindowHandle, processItem->ProcessId);
            }
        }
        break;
    case ID_MISCELLANEOUS_PAGESMODIFIED:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhShowImagePageModifiedDialog(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_FLUSHHEAPS:
        {
            PPH_PROCESS_ITEM* processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);
                PhUiFlushHeapProcesses(WindowHandle, processes, numberOfProcesses);
                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_PRIORITY_REALTIME:
    case ID_PRIORITY_HIGH:
    case ID_PRIORITY_ABOVENORMAL:
    case ID_PRIORITY_NORMAL:
    case ID_PRIORITY_BELOWNORMAL:
    case ID_PRIORITY_IDLE:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);
                PhMwpExecuteProcessPriorityClassCommand(WindowHandle, Id, processes, numberOfProcesses);
                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_IOPRIORITY_VERYLOW:
    case ID_IOPRIORITY_LOW:
    case ID_IOPRIORITY_NORMAL:
    case ID_IOPRIORITY_HIGH:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            if (PhGetSelectedProcessItems(&processes, &numberOfProcesses))
            {
                PhReferenceObjects(processes, numberOfProcesses);
                PhMwpExecuteProcessIoPriorityCommand(WindowHandle, Id, processes, numberOfProcesses);
                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
        }
        break;
    case ID_MISCELLANEOUS_ECOMODE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiSetEcoModeProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_EXECUTIONREQUIRED:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiSetExecutionRequiredProcess(WindowHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_WINDOW_BRINGTOFRONT:
        {
            if (IsWindow(PhMwpSelectedProcessWindowHandle))
            {
                WINDOWPLACEMENT placement = { sizeof(placement) };

                GetWindowPlacement(PhMwpSelectedProcessWindowHandle, &placement);

                if (placement.showCmd == SW_MINIMIZE)
                    ShowWindowAsync(PhMwpSelectedProcessWindowHandle, SW_RESTORE);
                else
                    ShowWindowAsync(PhMwpSelectedProcessWindowHandle, SW_SHOW);

                SetForegroundWindow(PhMwpSelectedProcessWindowHandle);
            }
        }
        break;
    case ID_WINDOW_RESTORE:
        {
            if (IsWindow(PhMwpSelectedProcessWindowHandle))
            {
                ShowWindowAsync(PhMwpSelectedProcessWindowHandle, SW_RESTORE);
            }
        }
        break;
    case ID_WINDOW_MINIMIZE:
        {
            if (IsWindow(PhMwpSelectedProcessWindowHandle))
            {
                ShowWindowAsync(PhMwpSelectedProcessWindowHandle, SW_MINIMIZE);
            }
        }
        break;
    case ID_WINDOW_MAXIMIZE:
        {
            if (IsWindow(PhMwpSelectedProcessWindowHandle))
            {
                ShowWindowAsync(PhMwpSelectedProcessWindowHandle, SW_MAXIMIZE);
            }
        }
        break;
    case ID_WINDOW_CLOSE:
        {
            if (IsWindow(PhMwpSelectedProcessWindowHandle))
            {
                NTSTATUS status;

                status = PhTerminateWindow(PhMwpSelectedProcessWindowHandle, TRUE);
                //PostMessage(PhMwpSelectedProcessWindowHandle, WM_CLOSE, 0, 0);

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(WindowHandle, L"Unable to close the window.", status, 0);
                }
            }
        }
        break;
    case ID_PROCESS_OPENFILELOCATION:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                NTSTATUS status;
                PPH_STRING fileName;

                status = PhGetProcessItemFileNameWin32(processItem, &fileName);

                if (NT_SUCCESS(status))
                {
                    PhShellExecuteUserString(
                        WindowHandle,
                        SETTING_FILE_BROWSE_EXECUTABLE,
                        PhGetString(fileName),
                        FALSE,
                        L"Make sure the Explorer executable file is present."
                        );

                    PhDereferenceObject(fileName);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"Unable to locate the file.", status, 0);
                }
            }
        }
        break;
    case ID_PROCESS_SEARCHONLINE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhSearchOnlineString(WindowHandle, PhGetString(processItem->ProcessName));
            }
        }
        break;
    case ID_PROCESS_PROPERTIES:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                // No reference needed; no messages pumped.
                PhMwpShowProcessProperties(processItem);
            }
        }
        break;
    case ID_PROCESS_COPY:
        {
            PhCopyProcessTree();
        }
        break;
    case ID_SERVICE_GOTOPROCESS:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();
            PPH_PROCESS_NODE processNode;

            if (serviceItem)
            {
                if (processNode = PhFindProcessNode(serviceItem->ProcessId))
                {
                    PhMwpSelectPage(PhMwpProcessesPage->Index);
                    SetFocus(PhMwpProcessTreeNewHandle);
                    PhSelectAndEnsureVisibleProcessNode(processNode);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"The process does not exist.", STATUS_INVALID_CID, 0);
                }
            }
        }
        break;
    case ID_SERVICE_START:
        {
            PPH_SERVICE_ITEM* serviceItems;
            ULONG numberOfServiceItems;

            PhGetSelectedServiceItems(&serviceItems, &numberOfServiceItems);
            PhReferenceObjects(serviceItems, numberOfServiceItems);
            PhUiStartServices(WindowHandle, serviceItems, numberOfServiceItems);
            PhDereferenceObjects(serviceItems, numberOfServiceItems);
            PhFree(serviceItems);
        }
        break;
    case ID_SERVICE_CONTINUE:
        {
            PPH_SERVICE_ITEM* serviceItems;
            ULONG numberOfServiceItems;

            PhGetSelectedServiceItems(&serviceItems, &numberOfServiceItems);
            PhReferenceObjects(serviceItems, numberOfServiceItems);
            PhUiContinueServices(WindowHandle, serviceItems, numberOfServiceItems);
            PhDereferenceObjects(serviceItems, numberOfServiceItems);
            PhFree(serviceItems);
        }
        break;
    case ID_SERVICE_PAUSE:
        {
            PPH_SERVICE_ITEM* serviceItems;
            ULONG numberOfServiceItems;

            PhGetSelectedServiceItems(&serviceItems, &numberOfServiceItems);
            PhReferenceObjects(serviceItems, numberOfServiceItems);
            PhUiPauseServices(WindowHandle, serviceItems, numberOfServiceItems);
            PhDereferenceObjects(serviceItems, numberOfServiceItems);
            PhFree(serviceItems);
        }
        break;
    case ID_SERVICE_STOP:
        {
            PPH_SERVICE_ITEM* serviceItems;
            ULONG numberOfServiceItems;

            PhGetSelectedServiceItems(&serviceItems, &numberOfServiceItems);
            PhReferenceObjects(serviceItems, numberOfServiceItems);
            PhUiStopServices(WindowHandle, serviceItems, numberOfServiceItems);
            PhDereferenceObjects(serviceItems, numberOfServiceItems);
            PhFree(serviceItems);
        }
        break;
    case ID_SERVICE_DELETE:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                PhReferenceObject(serviceItem);

                if (PhUiDeleteService(WindowHandle, serviceItem))
                    PhDeselectAllServiceNodes();

                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_OPENKEY:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                HANDLE keyHandle;

                if (NT_SUCCESS(PhOpenServiceKey(
                    &keyHandle,
                    KEY_READ,
                    &serviceItem->Name->sr
                    )))
                {
                    PPH_STRING hklmServiceKeyName;

                    if (NT_SUCCESS(PhQueryObjectName(keyHandle, &hklmServiceKeyName)))
                    {
                        PhMoveReference(&hklmServiceKeyName, PhFormatNativeKeyName(hklmServiceKeyName));

                        PhShellOpenKey2(WindowHandle, hklmServiceKeyName);

                        PhDereferenceObject(hklmServiceKeyName);
                    }
                    else
                    {
                        PhShowStatus(WindowHandle, L"The service does not exist.", STATUS_OBJECT_NAME_NOT_FOUND, 0);
                    }

                    NtClose(keyHandle);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"The service does not exist.", STATUS_OBJECT_NAME_NOT_FOUND, 0);
                }
            }
        }
        break;
    case ID_SERVICE_OPENFILELOCATION:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();
            NTSTATUS status;
            SC_HANDLE serviceHandle;
            PPH_STRING fileName;

            if (serviceItem)
            {
                status = PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(serviceItem->Name));

                if (NT_SUCCESS(status))
                {
                    status = PhGetServiceHandleFileName(serviceHandle, &serviceItem->Name->sr, &fileName);

                    if (NT_SUCCESS(status))
                    {
                        PhShellExecuteUserString(
                            WindowHandle,
                            SETTING_FILE_BROWSE_EXECUTABLE,
                            fileName->Buffer,
                            FALSE,
                            L"Make sure the Explorer executable file is present."
                            );
                        PhDereferenceObject(fileName);
                    }
                    else
                    {
                        PhShowStatus(WindowHandle, L"Unable to locate the file.", status, 0);
                    }

                    PhCloseServiceHandle(serviceHandle);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"Unable to locate the file.", status, 0);
                }
            }
        }
        break;
    case ID_SERVICE_PROPERTIES:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                // The object relies on the list view reference, which could
                // disappear if we don't reference the object here.
                PhReferenceObject(serviceItem);
                PhShowServiceProperties(WindowHandle, serviceItem);
                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_COPY:
        {
            PhCopyServiceList();
        }
        break;
    case ID_NETWORK_GOTOPROCESS:
        {
            PPH_NETWORK_ITEM networkItem = PhGetSelectedNetworkItem();
            PPH_PROCESS_NODE processNode;

            if (networkItem)
            {
                if (processNode = PhFindProcessNode(networkItem->ProcessId))
                {
                    PhMwpSelectPage(PhMwpProcessesPage->Index);
                    SetFocus(PhMwpProcessTreeNewHandle);
                    PhSelectAndEnsureVisibleProcessNode(processNode);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"The process does not exist.", STATUS_INVALID_CID, 0);
                }
            }
        }
        break;
    case ID_NETWORK_GOTOSERVICE:
        {
            PPH_NETWORK_ITEM networkItem = PhGetSelectedNetworkItem();
            PPH_SERVICE_ITEM serviceItem;

            if (networkItem && networkItem->OwnerName)
            {
                if (serviceItem = PhReferenceServiceItem(&networkItem->OwnerName->sr))
                {
                    PhMwpSelectPage(PhMwpServicesPage->Index);
                    SetFocus(PhMwpServiceTreeNewHandle);
                    SystemInformer_SelectServiceItem(serviceItem);

                    PhDereferenceObject(serviceItem);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"The service does not exist.", STATUS_INVALID_CID, 0);
                }
            }
        }
        break;
    case ID_NETWORK_CLOSE:
        {
            PPH_NETWORK_ITEM *networkItems;
            ULONG numberOfNetworkItems;

            PhGetSelectedNetworkItems(&networkItems, &numberOfNetworkItems);
            PhReferenceObjects(networkItems, numberOfNetworkItems);

            if (PhUiCloseConnections(WindowHandle, networkItems, numberOfNetworkItems))
                PhDeselectAllNetworkNodes();

            PhDereferenceObjects(networkItems, numberOfNetworkItems);
            PhFree(networkItems);
        }
        break;
    case ID_NETWORK_COPY:
        {
            PhCopyNetworkList();
        }
        break;
    case ID_TAB_NEXT:
        {
            ULONG selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

            if (selectedIndex != PageList->Count - 1)
                selectedIndex++;
            else
                selectedIndex = 0;

            PhMwpSelectPage(selectedIndex);
        }
        break;
    case ID_TAB_PREV:
        {
            ULONG selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

            if (selectedIndex != 0)
                selectedIndex--;
            else
                selectedIndex = PageList->Count - 1;

            PhMwpSelectPage(selectedIndex);
        }
        break;
    }
}

/**
 * Handles the ShowWindow event for the main window.
 *
 * \param WindowHandle Handle to the window receiving the event.
 * \param Showing Indicates whether the window is being shown (TRUE) or hidden (FALSE).
 * \param State Specifies the state of the window (e.g., minimized, maximized, normal).
 */
VOID PhMwpOnShowWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    if (NeedsMaximize)
    {
        ShowWindow(WindowHandle, SW_MAXIMIZE);
        NeedsMaximize = FALSE;
    }
}

/**
 * Handles system command messages for the main window.
 *
 * \param WindowHandle Handle to the window receiving the system command.
 * \param Type The type of system command (e.g., SC_CLOSE, SC_MINIMIZE).
 * \param CursorScreenX The X coordinate of the cursor in screen coordinates.
 * \param CursorScreenY The Y coordinate of the cursor in screen coordinates.
 * \return TRUE if the system command was handled; otherwise, FALSE.
 */
BOOLEAN PhMwpOnSysCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    )
{
    switch (Type)
    {
    case SC_CLOSE:
        {
            if (PhGetIntegerSetting(SETTING_HIDE_ON_CLOSE) && PhNfIconsEnabled())
            {
                ShowWindow(WindowHandle, SW_HIDE);
                return TRUE;
            }
        }
        break;
    case SC_MINIMIZE:
        {
            // Save the current window state because we may not have a chance to later.
            PhMwpSaveWindowState(WindowHandle);

            if (PhGetIntegerSetting(SETTING_HIDE_ON_MINIMIZE) && PhNfIconsEnabled())
            {
                ShowWindow(WindowHandle, SW_HIDE);
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

/**
 * Handles menu command events for the main window.
 *
 * \param WindowHandle Handle to the window receiving the menu command.
 * \param Index The index of the selected menu item.
 * \param Menu Handle to the menu from which the command originated.
 */
VOID PhMwpOnMenuCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _In_ HMENU Menu
    )
{
    MENUITEMINFO menuItemInfo;

    memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
    menuItemInfo.cbSize = sizeof(MENUITEMINFO);
    menuItemInfo.fMask = MIIM_ID | MIIM_DATA;

    if (GetMenuItemInfo(Menu, Index, TRUE, &menuItemInfo))
    {
        PhMwpDispatchMenuCommand(
            WindowHandle,
            Menu,
            Index,
            menuItemInfo.wID,
            menuItemInfo.dwItemData
            );
    }
}

/**
 * Handles the initialization of a menu popup.
 *
 * This function is called when a menu popup is about to be displayed. It allows for
 * customization of the menu items based on the current state of the application.
 *
 * \param WindowHandle Handle to the window associated with the menu.
 * \param Menu Handle to the menu being initialized.
 * \param Index The zero-based index of the menu in the menu bar.
 * \param IsWindowMenu TRUE if the menu is a window menu; otherwise, FALSE.
 */
VOID PhMwpOnInitMenuPopup(
    _In_ HWND WindowHandle,
    _In_ HMENU Menu,
    _In_ ULONG Index,
    _In_ BOOLEAN IsWindowMenu
    )
{
    ULONG i;
    BOOLEAN found;
    PPH_EMENU menu;

    found = FALSE;

    for (i = 0; i < sizeof(SubMenuHandles) / sizeof(HMENU); i++)
    {
        if (Menu == SubMenuHandles[i])
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
        return;

    // Delete all items in this submenu.
    PhDeleteHMenu(Menu);

    // Delete the previous EMENU for this submenu.
    if (SubMenuObjects[Index])
        PhDestroyEMenu(SubMenuObjects[Index]);

    // Make sure the menu style is set correctly.
    PhSetHMenuStyle(Menu, TRUE);

    menu = PhpCreateMainMenu(Index);
    PhMwpInitializeSubMenu(WindowHandle, menu, Index);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_MENU_INFORMATION pluginMenuInfo;

        PhPluginInitializeMenuInfo(&pluginMenuInfo, menu, WindowHandle, PH_PLUGIN_MENU_DISALLOW_HOOKS);
        pluginMenuInfo.u.MainMenu.SubMenuIndex = Index;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainMenuInitializing), &pluginMenuInfo);
    }

    PhEMenuToHMenu2(Menu, menu, 0, NULL);
    SubMenuObjects[Index] = menu;
}

/**
 * Handles the WM_SIZE message for the main window.
 *
 * This function is called when the main window is resized. It processes the new size and state,
 * allowing the application to adjust its layout or controls accordingly.
 *
 * \param WindowHandle Handle to the window receiving the resize event.
 * \param State Specifies the type of resizing (e.g., SIZE_MINIMIZED, SIZE_MAXIMIZED, SIZE_RESTORED).
 * \param Width The new width of the client area, in pixels.
 * \param Height The new height of the client area, in pixels.
 */
VOID PhMwpOnSize(
    _In_ HWND WindowHandle,
    _In_ UINT State,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    //if (!(Width && Height))
    //    return;

    if (State != SIZE_MINIMIZED)
    {
        HDWP deferHandle;

        deferHandle = BeginDeferWindowPos(2);
        PhMwpLayout(&deferHandle);
        EndDeferWindowPos(deferHandle);
    }
}

/**
 * Handles the window sizing event.
 *
 * \param Edge The edge of the window being resized (e.g., left, right, top, bottom).
 * \param DragRectangle Pointer to a RECT structure that specifies the new size and position.
 */
VOID PhMwpOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    )
{
    PhResizingMinimumSize(DragRectangle, Edge, 400, 175);
}

/**
 * Handles the WM_SETFOCUS message for the main window.
 *
 * \param WindowHandle Handle to the window that received focus.
 */
VOID PhMwpOnSetFocus(
    _In_ HWND WindowHandle
    )
{
    // Update the window focus.

    if (CurrentPage->WindowHandle)
    {
        SetFocus(CurrentPage->WindowHandle);
    }

    // Update the window status.

    {
        KPH_LEVEL status;
        PPH_STRING windowTitle;

        if (DelayedLoadCompleted && PhMainWndLevel != (status = KphLevelEx(FALSE)))
        {
            PhMainWndLevel = status;

            if (windowTitle = PhMwpInitializeWindowTitle(PhMainWndLevel))
            {
                PhSetWindowText(WindowHandle, PhGetString(windowTitle));
                PhDereferenceObject(windowTitle);
            }
        }
    }
}

/**
 * Handles timer events for the main window.
 *
 * \param WindowHandle Handle to the window receiving the timer event.
 * \param wParam The timer identifier.
 * \param lParam Additional message information (unused).
 */
VOID PhMwpOnTimer(
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LONG timerId = (LONG)(ULONG_PTR)wParam;
    TIMERPROC timerProc = (TIMERPROC)lParam;
}

/**
 * Handles notification messages for the main window.
 *
 * \param Header Pointer to an NMHDR structure containing notification information.
 * \param Result Pointer to a variable that receives the result of the notification processing.
 * \return Returns TRUE if the notification was handled successfully, otherwise FALSE.
 */
_Success_(return)
BOOLEAN PhMwpOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    )
{
    if (Header->hwndFrom == TabControlHandle)
    {
        PhMwpNotifyTabControl(Header);
    }

    return FALSE;
}

/**
 * Handles device change notifications for the main window.
 *
 * This function is called when the system detects a change in the device configuration,
 * such as when a device is added or removed. It processes the WM_DEVICECHANGE message
 * and performs any necessary updates or actions in response to the change.
 *
 * \param WindowHandle Handle to the window receiving the device change notification.
 * \param wParam Additional message information. Specifies the event that occurred.
 * \param lParam Additional message information. Pointer to a structure with event-specific data.
 */
VOID PhMwpOnDeviceChanged(
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (wParam)
    {
    case DBT_DEVICEARRIVAL: // Drive letter added
    //case DBT_DEVICEREMOVECOMPLETE: // Drive letter removed
        {
            PDEV_BROADCAST_HDR deviceBroadcast = (PDEV_BROADCAST_HDR)lParam;

            if (deviceBroadcast->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PhUpdateDosDevicePrefixes();
            }
        }
        break;
    }

    if (PhPluginsEnabled)
    {
        MSG message;

        memset(&message, 0, sizeof(MSG));
        message.hwnd = WindowHandle;
        message.message = WM_DEVICECHANGE;
        message.wParam = wParam;
        message.lParam = lParam;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &message);
    }
}

/**
 * Handles the DPI change event for the specified window.
 *
 * \param WindowHandle Handle to the window whose DPI has changed.
 * \param WindowDpi The new DPI value for the window.
 */
VOID PhMwpOnDpiChanged(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    )
{
    PhGuiSupportUpdateSystemMetrics(WindowHandle, WindowDpi);

    PhMwpInitializeMetrics(WindowHandle, WindowDpi);

    if (PhEnableWindowText)
        PhSetApplicationWindowIconEx(WindowHandle, LayoutWindowDpi);

    PhMwpOnSettingChange(WindowHandle, 0, NULL);

    PhMwpInvokeUpdateWindowFont(NULL);

    if (PhGetIntegerSetting(SETTING_ENABLE_MONOSPACE_FONT))
        PhMwpInvokeUpdateWindowFontMonospace(WindowHandle, NULL);

    PhMwpNotifyAllPages(MainTabPageDpiChanged, NULL, NULL);

    if (PhPluginsEnabled)
    {
        MSG message;

        memset(&message, 0, sizeof(MSG));
        message.hwnd = WindowHandle;
        message.message = WM_DPICHANGED;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &message);
    }

    InvalidateRect(WindowHandle, NULL, TRUE);
}

/**
 * Handles custom user messages sent to the main window.
 *
 * \param WindowHandle Handle to the window receiving the message.
 * \param Message The user-defined message identifier.
 * \param WParam Additional message-specific information.
 * \param LParam Additional message-specific information.
 * \return The result of message processing (LRESULT).
 */
LRESULT PhMwpOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case WM_PH_ACTIVATE:
        {
            if (!PhMainWndEarlyExit && !PhMainWndExiting)
            {
                if (WParam != 0)
                {
                    PPH_PROCESS_NODE processNode;

                    if (processNode = PhFindProcessNode((HANDLE)WParam))
                        PhSelectAndEnsureVisibleProcessNode(processNode);
                }

                if (!IsWindowVisible(WindowHandle))
                {
                    ShowWindow(WindowHandle, SW_SHOW);
                }

                if (IsMinimized(WindowHandle))
                {
                    ShowWindow(WindowHandle, SW_RESTORE);
                }

                return PH_ACTIVATE_REPLY;
            }
            else
            {
                return 0;
            }
        }
        break;
    case WM_PH_NOTIFY_ICON_MESSAGE:
        {
            PhNfForwardMessage(WindowHandle, WParam, LParam);
        }
        break;
    case WM_PH_DESTROY:
        {
            DestroyWindow(PhMainWndHandle);
        }
        break;
    case WM_PH_PREPARE_FOR_EARLY_SHUTDOWN:
        {
            PhMwpInvokePrepareEarlyExit(PhMainWndHandle);
        }
        break;
    case WM_PH_SAVE_ALL_SETTINGS:
        {
            PhMwpSaveSettings(PhMainWndHandle);
        }
        break;
    case WM_PH_SHOW_PROPERTIES:
        {
            PhMwpShowProcessProperties((PPH_PROCESS_ITEM)LParam);
        }
        break;
    case WM_PH_ACTIVATE_WINDOW:
        {
            BOOLEAN visibility = (BOOLEAN)(ULONG_PTR)LParam;

            PhMwpInvokeActivateWindow(visibility);
        }
        break;
    case WM_PH_SELECT_NODE:
        {
            switch ((ULONG)WParam)
            {
            case 1:
                PhMwpInvokeSelectTabPage((PVOID)LParam);
                break;
            case 2:
                PhSelectAndEnsureVisibleProcessNode((PVOID)LParam);
                break;
            case 3:
                PhMwpInvokeSelectServiceItem((PVOID)LParam);
                break;
            case 4:
                PhMwpInvokeSelectNetworkItem((PVOID)LParam);
                break;
            case 5:
                PhMwpInvokeUpdateWindowFont((PVOID)LParam);
                break;
            }
        }
        break;
    case WM_PH_SHOW_EDITOR:
        {
            PPH_SHOW_MEMORY_EDITOR showMemoryEditor = (PPH_SHOW_MEMORY_EDITOR)WParam;

            PhShowMemoryEditorDialog(
                showMemoryEditor->OwnerWindow,
                showMemoryEditor->ProcessId,
                showMemoryEditor->BaseAddress,
                showMemoryEditor->RegionSize,
                showMemoryEditor->SelectOffset,
                showMemoryEditor->SelectLength,
                showMemoryEditor->Title,
                showMemoryEditor->Flags
                );
            PhClearReference(&showMemoryEditor->Title);
            PhFree(showMemoryEditor);
        }
        break;
    case WM_PH_SHOW_RESULT:
        {
            PPH_SHOW_MEMORY_RESULTS showMemoryResults = (PPH_SHOW_MEMORY_RESULTS)WParam;

            PhShowMemoryResultsDialog(
                showMemoryResults->ProcessId,
                showMemoryResults->Results
                );
            PhDereferenceMemoryResults(
                (PPH_MEMORY_RESULT*)showMemoryResults->Results->Items,
                showMemoryResults->Results->Count
                );
            PhDereferenceObject(showMemoryResults->Results);
            PhFree(showMemoryResults);
        }
        break;
    case WM_PH_INVOKE:
        {
            PhProcessInvokeQueue();
        }
        break;
    }

    return 0;
}

/**
 * Loads the main window settings for the application.
 *
 * \param WindowHandle Handle to the main window for which settings are to be loaded.
 */
VOID PhMwpLoadSettings(
    _In_ HWND WindowHandle
    )
{
    ULONG opacity;

    opacity = PhGetIntegerSetting(SETTING_MAIN_WINDOW_OPACITY);
    PhStatisticsSampleCount = PhGetIntegerSetting(SETTING_SAMPLE_COUNT);
    PhEnablePurgeProcessRecords = !PhGetIntegerSetting(SETTING_NO_PURGE_PROCESS_RECORDS);
    PhEnableCycleCpuUsage = !!PhGetIntegerSetting(SETTING_ENABLE_CYCLE_CPU_USAGE);
    PhEnableNetworkBoundConnections = !!PhGetIntegerSetting(SETTING_ENABLE_NETWORK_BOUND_CONNECTIONS);
    PhEnableNetworkProviderResolve = !!PhGetIntegerSetting(SETTING_ENABLE_NETWORK_RESOLVE);
    PhEnableProcessQueryStage2 = !!PhGetIntegerSetting(SETTING_ENABLE_STAGE2);
    PhEnableServiceQueryStage2 = !!PhGetIntegerSetting(SETTING_ENABLE_SERVICE_STAGE2);
    PhEnableTooltipSupport = !!PhGetIntegerSetting(SETTING_ENABLE_TOOLTIP_SUPPORT);
    PhEnableImageCoherencySupport = !!PhGetIntegerSetting(SETTING_ENABLE_IMAGE_COHERENCY_SUPPORT);
    PhEnableLinuxSubsystemSupport = !!PhGetIntegerSetting(SETTING_ENABLE_LINUX_SUBSYSTEM_SUPPORT);
    PhEnablePackageIconSupport = !!PhGetIntegerSetting(SETTING_ENABLE_PACKAGE_ICON_SUPPORT);
    PhEnableSecurityAdvancedDialog = !!PhGetIntegerSetting(SETTING_ENABLE_SECURITY_ADVANCED_DIALOG);
    PhEnableProcessHandlePnPDeviceNameSupport = !!PhGetIntegerSetting(SETTING_ENABLE_PROCESS_HANDLE_PNP_DEVICE_NAME_SUPPORT);
    PhMwpNotifyIconNotifyMask = PhGetIntegerSetting(SETTING_ICON_NOTIFY_MASK);

    if (PhGetIntegerSetting(SETTING_MAIN_WINDOW_ALWAYS_ON_TOP))
    {
        AlwaysOnTop = TRUE;
        SetWindowPos(WindowHandle, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
    }

    if (opacity != 0)
        PhSetWindowOpacity(WindowHandle, opacity);

    PhMwpInvokeUpdateWindowFont(NULL);

    if (PhGetIntegerSetting(SETTING_ENABLE_MONOSPACE_FONT))
    {
        PhMwpInvokeUpdateWindowFontMonospace(WindowHandle, NULL);
    }

    PhNfLoadStage1();

    PhMwpNotifyAllPages(MainTabPageLoadSettings, NULL, NULL);

    PhLogInitialization();
}

/**
 * Saves the current settings of the main window.
 *
 * \param WindowHandle Handle to the main window whose settings are to be saved.
 */
VOID PhMwpSaveSettings(
    _In_ HWND WindowHandle
    )
{
    PhMwpNotifyAllPages(MainTabPageSaveSettings, NULL, NULL);

    PhSaveWindowPlacementToSetting(SETTING_MAIN_WINDOW_POSITION, SETTING_MAIN_WINDOW_SIZE, WindowHandle);
    PhMwpSaveWindowState(WindowHandle);

    if (!PhIsNullOrEmptyString(PhSettingsFileName))
    {
        NTSTATUS status;

        status = PhSaveSettings(&PhSettingsFileName->sr);

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(NULL, L"Unable to save application settings.", status, 0);
        }
    }
}

/**
 * Saves the current state of the main window.
 * \param WindowHandle Handle to the window whose state is to be saved.
 */
VOID PhMwpSaveWindowState(
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };

    GetWindowPlacement(WindowHandle, &placement);

    if (placement.showCmd == SW_NORMAL)
        PhSetIntegerSetting(SETTING_MAIN_WINDOW_STATE, SW_NORMAL);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetIntegerSetting(SETTING_MAIN_WINDOW_STATE, SW_MAXIMIZE);
}

/**
 * Updates the layout padding for the main window.
 *
 * This function recalculates and applies the necessary padding values
 * to the main window layout, ensuring proper spacing and alignment
 * of UI elements. It should be called whenever the window layout
 * or padding requirements change.
 */
VOID PhMwpUpdateLayoutPadding(
    VOID
    )
{
    PH_LAYOUT_PADDING_DATA data;

    memset(&data, 0, sizeof(PH_LAYOUT_PADDING_DATA));
    PhInvokeCallback(&LayoutPaddingCallback, &data);

    LayoutPadding = data.Padding;
}

/**
 * Applies padding to the specified rectangle.
 *
 * \param Rect A pointer to a RECT structure that will be modified to include the specified padding.
 * \param Padding A pointer to a RECT structure specifying the padding to apply (left, top, right, bottom).
 */
VOID PhMwpApplyLayoutPadding(
    _Inout_ PRECT Rect,
    _In_ PRECT Padding
    )
{
    Rect->left += Padding->left;
    Rect->top += Padding->top;
    Rect->right -= Padding->right;
    Rect->bottom -= Padding->bottom;
}

/**
 * Adjusts the layout of the main window using a deferred window positioning handle.
 *
 * \param DeferHandle A pointer to an HDWP handle used for batching window position changes.
 */
VOID PhMwpLayout(
    _Inout_ HDWP *DeferHandle
    )
{
    RECT rect;

    // Resize the tab control.
    // Don't defer the resize. The tab control doesn't repaint properly.

    if (!LayoutPaddingValid)
    {
        PhMwpUpdateLayoutPadding();
        LayoutPaddingValid = TRUE;
    }

    if (!PhGetClientRect(PhMainWndHandle, &rect))
        return;

    PhMwpApplyLayoutPadding(&rect, &LayoutPadding);

    if (PhEnableDeferredLayout)
    {
        *DeferHandle = DeferWindowPos(
            *DeferHandle,
            TabControlHandle,
            HWND_BOTTOM,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
    else
    {
        SetWindowPos(
            TabControlHandle,
            NULL,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        UpdateWindow(TabControlHandle);
    }

    PhMwpLayoutTabControl(DeferHandle);
}

/**
 * Sets up the computer menu for the main window.
 *
 * \param Root Pointer to the root menu item to be configured.
 */
VOID PhMwpSetupComputerMenu(
    _In_ PPH_EMENU_ITEM Root
    )
{
    PPH_EMENU_ITEM menuItem;

    if (WindowsVersion < WINDOWS_8)
    {
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_RESTARTADVOPTIONS))
            PhDestroyEMenuItem(menuItem);
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_RESTARTBOOTOPTIONS))
            PhDestroyEMenuItem(menuItem);
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_SHUTDOWNHYBRID))
            PhDestroyEMenuItem(menuItem);
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_RESTARTFWDEVICE))
            PhDestroyEMenuItem(menuItem);
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_RESTARTBOOTDEVICE))
            PhDestroyEMenuItem(menuItem);
    }
}

/**
 * Executes a computer-related command based on the specified command ID.
 *
 * \param WindowHandle Handle to the window that will receive any notifications or messages.
 * \param Id Identifier of the computer command to execute.
 * \return TRUE if the command was executed successfully, FALSE otherwise.
 */
BOOLEAN PhMwpExecuteComputerCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_COMPUTER_LOCK:
        PhUiLockComputer(WindowHandle);
        return TRUE;
    case ID_COMPUTER_LOGOFF:
        PhUiLogoffComputer(WindowHandle);
        return TRUE;
    case ID_COMPUTER_SLEEP:
        PhUiSleepComputer(WindowHandle);
        return TRUE;
    case ID_COMPUTER_HIBERNATE:
        PhUiHibernateComputer(WindowHandle);
        return TRUE;
    case ID_COMPUTER_RESTART:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_WIN32, 0);
        return TRUE;
    case ID_COMPUTER_RESTARTADVOPTIONS:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_ADVANCEDBOOT, 0);
        return TRUE;
    case ID_COMPUTER_RESTARTBOOTOPTIONS:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_WIN32, PH_SHUTDOWN_RESTART_BOOTOPTIONS);
        return TRUE;
    case ID_COMPUTER_RESTARTFWOPTIONS:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_FIRMWAREBOOT, 0);
        return TRUE;
    case ID_COMPUTER_SHUTDOWN:
        PhUiShutdownComputer(WindowHandle, PH_POWERACTION_TYPE_WIN32, 0);
        return TRUE;
    case ID_COMPUTER_SHUTDOWNHYBRID:
        PhUiShutdownComputer(WindowHandle, PH_POWERACTION_TYPE_WIN32, PH_SHUTDOWN_HYBRID);
        return TRUE;
    case ID_COMPUTER_RESTART_NATIVE:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_NATIVE, 0);
        return TRUE;
    case ID_COMPUTER_SHUTDOWN_NATIVE:
        PhUiShutdownComputer(WindowHandle, PH_POWERACTION_TYPE_NATIVE, 0);
        return TRUE;
    case ID_COMPUTER_RESTART_CRITICAL:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_CRITICAL, 0);
        return TRUE;
    case ID_COMPUTER_SHUTDOWN_CRITICAL:
        PhUiShutdownComputer(WindowHandle, PH_POWERACTION_TYPE_CRITICAL, 0);
        return TRUE;
    case ID_COMPUTER_RESTART_UPDATE:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_UPDATE, 0);
        return TRUE;
    case ID_COMPUTER_SHUTDOWN_UPDATE:
        PhUiShutdownComputer(WindowHandle, PH_POWERACTION_TYPE_UPDATE, 0);
        return TRUE;
    case ID_COMPUTER_RESTARTWDOSCAN:
        PhUiRestartComputer(WindowHandle, PH_POWERACTION_TYPE_WDOSCAN, 0);
        return TRUE;
    }

    return FALSE;
}

/**
 * Determines whether the specified window is overlapped by other windows.
 *
 * \param WindowHandle Handle to the window to check for overlap.
 * \return TRUE if the window is overlapped by other windows; FALSE otherwise.
 */
BOOLEAN PhMwpIsWindowOverlapped(
    _In_ HWND WindowHandle
    )
{
    RECT rectThisWindow = { 0 };
    RECT rectOtherWindow = { 0 };
    RECT rectIntersection = { 0 };
    HWND windowHandle = WindowHandle;

    if (!PhGetWindowRect(WindowHandle, &rectThisWindow))
        return FALSE;

    while ((windowHandle = GetWindow(windowHandle, GW_HWNDPREV)) && windowHandle != WindowHandle)
    {
        if (!(PhGetWindowStyle(windowHandle) & WS_VISIBLE))
            continue;

        if (!PhGetWindowRect(windowHandle, &rectOtherWindow))
            continue;

        if (!(PhGetWindowStyleEx(windowHandle) & WS_EX_TOPMOST) &&
            IntersectRect(&rectIntersection, &rectThisWindow, &rectOtherWindow))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Activates the specified window.
 *
 * \param WindowHandle Handle to the window to be activated.
 * \param Toggle If TRUE, toggles the activation state; if FALSE, activates the window without toggling.
 */
VOID PhMwpActivateWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Toggle
    )
{
    if (IsMinimized(WindowHandle))
    {
        ShowWindow(WindowHandle, SW_RESTORE);
        SetForegroundWindow(WindowHandle);
    }
    else if (IsWindowVisible(WindowHandle))
    {
        if (Toggle && !PhMwpIsWindowOverlapped(WindowHandle))
            ShowWindow(WindowHandle, SW_HIDE);
        else
            SetForegroundWindow(WindowHandle);
    }
    else
    {
        ShowWindow(WindowHandle, SW_SHOW);
        SetForegroundWindow(WindowHandle);
    }
}

/**
 * Creates a computer menu.
 *
 * \param DelayLoadMenu Specifies whether the menu should be loaded with a delay.
 * \return A pointer to the created PPH_EMENU structure.
 */
PPH_EMENU PhpCreateComputerMenu(
    _In_ BOOLEAN DelayLoadMenu
    )
{
    PPH_EMENU_ITEM menuItem;

    menuItem = PhCreateEMenuItem(0, 0, L"&Computer", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_LOCK, L"&Lock", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_LOGOFF, L"Log o&ff", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SLEEP, L"&Sleep", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_HIBERNATE, L"&Hibernate", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTART_UPDATE, L"Update and restart", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWN_UPDATE, L"Update and shut down", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTART, L"R&estart", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(PhGetOwnTokenAttributes().Elevated ? 0 : PH_EMENU_DISABLED, ID_COMPUTER_RESTARTADVOPTIONS, L"Restart to advanced options", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTARTBOOTOPTIONS, L"Restart to boot options", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(PhGetOwnTokenAttributes().Elevated ? 0 : PH_EMENU_DISABLED, ID_COMPUTER_RESTARTFWOPTIONS, L"Restart to firmware options", NULL, NULL), ULONG_MAX);
    if (PhGetIntegerSetting(SETTING_ENABLE_SHUTDOWN_BOOT_MENU))
    {
        PVOID bootApplicationMenu = PhUiCreateComputerBootDeviceMenu(DelayLoadMenu);
        if (WindowsVersion >= WINDOWS_10 && PhGetOwnTokenAttributes().Elevated)
            PhInsertEMenuItem(bootApplicationMenu, PhCreateEMenuItem(0, ID_COMPUTER_RESTARTWDOSCAN, L"Windows Defender Offline Scan", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menuItem, bootApplicationMenu, ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhUiCreateComputerFirmwareDeviceMenu(DelayLoadMenu), ULONG_MAX);
    }
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWN, L"Shu&t down", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWNHYBRID, L"H&ybrid shut down", NULL, NULL), ULONG_MAX);
    if (PhGetIntegerSetting(SETTING_ENABLE_SHUTDOWN_CRITICAL_MENU))
    {
        PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTART_NATIVE, L"R&estart (Native)", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWN_NATIVE, L"Shu&t down (Native)", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTART_CRITICAL, L"R&estart (Critical)", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWN_CRITICAL, L"Shu&t down (Critical)", NULL, NULL), ULONG_MAX);
    }

    return menuItem;
}

/**
 * Creates the system menu for the application.
 *
 * \param HackerMenu A pointer to the base menu structure to be used for creating the system menu.
 * \param DelayLoadMenu If TRUE, the menu will be loaded with a delay; if FALSE, it will be loaded immediately.
 * \return A pointer to the newly created system menu (PPH_EMENU).
 */
PPH_EMENU PhpCreateSystemMenu(
    _In_ PPH_EMENU HackerMenu,
    _In_ BOOLEAN DelayLoadMenu
    )
{
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUN, L"&Run...\bCtrl+R", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUNAS, L"Run &as...\bCtrl+Shift+R", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUNASPACKAGE, L"Run as &package...\bCtrl+Shift+P", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_SHOWDETAILSFORALLPROCESSES, L"Show &details for all processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_SAVE, L"&Save...\bCtrl+S", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_FINDHANDLESORDLLS, L"&Find handles or DLLs...\bCtrl+F", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_OPTIONS, L"&Options...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhpCreateComputerMenu(DelayLoadMenu), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_EXIT, L"E&xit", NULL, NULL), ULONG_MAX);

    return HackerMenu;
}

/**
 * Creates and initializes a view menu.
 *
 * \param ViewMenu A pointer to an existing PPH_EMENU structure representing the view menu to be created or modified.
 * \return A pointer to the newly created or updated PPH_EMENU structure.
 */
PPH_EMENU PhpCreateViewMenu(
    _In_ PPH_EMENU ViewMenu
    )
{
    PPH_EMENU_ITEM menuItem;

    PhInsertEMenuItem(ViewMenu, PhCreateEMenuItem(0, ID_VIEW_SYSTEMINFORMATION, L"System &information\bCtrl+I", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, PhCreateEMenuItem(0, ID_VIEW_TRAYICONS, L"&Tray icons", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, PhCreateEMenuItem(0, ID_VIEW_SECTIONPLACEHOLDER, L"<section placeholder>", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, PhCreateEMenuItem(0, ID_VIEW_ALWAYSONTOP, L"&Always on top", NULL, NULL), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"&Opacity", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_10, L"&10%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_20, L"&20%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_30, L"&30%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_40, L"&40%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_50, L"&50%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_60, L"&60%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_70, L"&70%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_80, L"&80%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_90, L"&90%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_OPAQUE, L"&Opaque", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, menuItem, ULONG_MAX);

    PhInsertEMenuItem(ViewMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, PhCreateEMenuItem(0, ID_VIEW_REFRESH, L"&Refresh\bF5", NULL, NULL), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"Refresh i&nterval", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_UPDATEINTERVAL_FAST, L"&Fast (0.5s)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_UPDATEINTERVAL_NORMAL, L"&Normal (1s)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_UPDATEINTERVAL_BELOWNORMAL, L"&Below normal (2s)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_UPDATEINTERVAL_SLOW, L"&Slow (5s)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_UPDATEINTERVAL_VERYSLOW, L"&Very slow (10s)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ViewMenu, menuItem, ULONG_MAX);

    PhInsertEMenuItem(ViewMenu, PhCreateEMenuItem(0, ID_VIEW_UPDATEAUTOMATICALLY, L"Refresh a&utomatically\bF6", NULL, NULL), ULONG_MAX);

    return ViewMenu;
}

/**
 * Creates or initializes the Tools menu.
 *
 * \param ToolsMenu A pointer to an existing PPH_EMENU structure representing the Tools menu.
 * \return A pointer to the created or modified PPH_EMENU structure.
 */
PPH_EMENU PhpCreateToolsMenu(
    _In_ PPH_EMENU ToolsMenu
    )
{
    PPH_EMENU_ITEM menuItem;

    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_CREATESERVICE, L"&Create service...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_LIVEDUMP, L"&Create live dump...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_INSPECTEXECUTABLEFILE, L"Inspect e&xecutable file...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_THREADSTACKS, L"&Search thread stacks", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_ZOMBIEPROCESSES, L"&Zombie processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_PAGEFILES, L"&Pagefiles", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_STARTTASKMANAGER, L"Start &Task Manager", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_STARTRESOURCEMONITOR, L"Start &Resource Monitor", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_SHUTDOWNWSLPROCESSES, L"T&erminate WSL processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuSeparator(), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"&Permissions", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_PWR_PERMISSIONS, L"Current Power Scheme", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_SCM_PERMISSIONS, L"Service Control Manager", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_RDP_PERMISSIONS, L"Terminal Server Listener", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_WMI_PERMISSIONS, L"WMI Root Namespace", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_COM_ACCESS_PERMISSIONS, L"COM Access Permissions", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_COM_ACCESS_RESTRICTIONS, L"COM Access Restrictions", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_COM_LAUNCH_PERMISSIONS, L"COM Launch Permissions", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_COM_LAUNCH_RESTRICTIONS, L"COM Launch Restrictions", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_DESKTOP_PERMISSIONS, L"Current Window Desktop", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_STATION_PERMISSIONS, L"Current Window Station", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, menuItem, ULONG_MAX);

    return ToolsMenu;
}

/**
 * Creates or initializes a users menu.
 *
 * \param UsersMenu Pointer to an existing PPH_EMENU structure to be used or modified.
 * \param DelayLoadMenu If TRUE, the menu will be loaded with a delay; if FALSE, it will be loaded immediately.
 * \return Pointer to the created or initialized PPH_EMENU structure representing the users menu.
 */
PPH_EMENU PhpCreateUsersMenu(
    _In_ PPH_EMENU UsersMenu,
    _In_ BOOLEAN DelayLoadMenu
    )
{
    if (DelayLoadMenu)
    {
        // Insert a dummy menu so we're able to receive menu events and delay load winsta.dll functions. (dmex)
        PhInsertEMenuItem(UsersMenu, PhCreateEMenuItem(0, USHRT_MAX, L" ", NULL, NULL), ULONG_MAX);
        return UsersMenu;
    }

    PhUiCreateSessionMenu(UsersMenu);

    PhInsertEMenuItem(UsersMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(UsersMenu, PhCreateEMenuItem(PhGetOwnTokenAttributes().Elevated ? 0 : PH_EMENU_DISABLED, ID_TOOLS_USER_LIST, L"User list...", NULL, NULL), ULONG_MAX);

    return UsersMenu;
}

/**
 * Creates or initializes a Help menu.
 *
 * \param HelpMenu A pointer to a PPH_EMENU structure representing the Help menu to be created or initialized.
 * \return A pointer to the created or initialized PPH_EMENU structure.
 */
PPH_EMENU PhpCreateHelpMenu(
    _In_ PPH_EMENU HelpMenu
    )
{
    PhInsertEMenuItem(HelpMenu, PhCreateEMenuItem(0, ID_HELP_LOG, L"&Log\bCtrl+L", NULL, NULL), ULONG_MAX);
    //PhInsertEMenuItem(HelpMenu, PhCreateEMenuItem(0, ID_HELP_DONATE, L"&Donate", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HelpMenu, PhCreateEMenuItem(0, ID_HELP_DEBUGCONSOLE, L"Debu&g console", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HelpMenu, PhCreateEMenuItem(0, ID_HELP_ABOUT, L"&About", NULL, NULL), ULONG_MAX);

    return HelpMenu;
}

/**
 * Creates the main menu for the application.
 *
 * \param SubMenuIndex The index of the submenu to be created or selected.
 * \return A pointer to the created PPH_EMENU structure representing the main menu.
 */
PPH_EMENU PhpCreateMainMenu(
    _In_ ULONG SubMenuIndex
    )
{
    PPH_EMENU menu = PhCreateEMenu();

    switch (SubMenuIndex)
    {
    case PH_MENU_ITEM_LOCATION_SYSTEM:
        return PhpCreateSystemMenu(menu, FALSE);
    case PH_MENU_ITEM_LOCATION_VIEW:
        return PhpCreateViewMenu(menu);
    case PH_MENU_ITEM_LOCATION_TOOLS:
        return PhpCreateToolsMenu(menu);
    case PH_MENU_ITEM_LOCATION_USERS:
        return PhpCreateUsersMenu(menu, FALSE);
    case PH_MENU_ITEM_LOCATION_HELP:
        return PhpCreateHelpMenu(menu);
    case ULONG_MAX:
        {
            PPH_EMENU_ITEM menuItem;
            menu->Flags |= PH_EMENU_MAINMENU;

            menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_SYSTEM, L"&System", NULL, NULL);
            // Insert an empty menuitem so we're able to delay load the submenu. (dmex)
            PhInsertEMenuItem(menuItem, PhCreateEMenuItemEmpty(), ULONG_MAX);
            PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
            //PhInsertEMenuItem(menu, PhpCreateSystemMenu(menuItem, TRUE), ULONG_MAX);

            menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_VIEW, L"&View", NULL, NULL);
            // Insert an empty menuitem so we're able to delay load the submenu. (dmex)
            PhInsertEMenuItem(menuItem, PhCreateEMenuItemEmpty(), ULONG_MAX);
            PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
            //PhInsertEMenuItem(menu, PhpCreateViewMenu(menuItem), ULONG_MAX);

            menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_TOOLS, L"&Tools", NULL, NULL);
            // Insert an empty menuitem so we're able to delay load the submenu. (dmex)
            PhInsertEMenuItem(menuItem, PhCreateEMenuItemEmpty(), ULONG_MAX);
            PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
            //PhInsertEMenuItem(menu, PhpCreateToolsMenu(menuItem), ULONG_MAX);

            menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_USERS, L"&Users", NULL, NULL);
            // Insert an empty menuitem so we're able to delay load the submenu. (dmex)
            PhInsertEMenuItem(menuItem, PhCreateEMenuItemEmpty(), ULONG_MAX);
            PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
            //PhInsertEMenuItem(menu, PhpCreateUsersMenu(menuItem, TRUE), ULONG_MAX);

            menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_HELP, L"&Help", NULL, NULL);
            // Insert an empty menuitem so we're able to delay load the submenu. (dmex)
            PhInsertEMenuItem(menuItem, PhCreateEMenuItemEmpty(), ULONG_MAX);
            PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
            //PhInsertEMenuItem(menu, PhpCreateHelpMenu(menuItem), ULONG_MAX);
        }
        break;
    }

    return menu;
}

/**
 * Initializes the main menu for the specified window.
 *
 * \param WindowHandle Handle to the window for which the main menu is to be initialized.
 */
VOID PhMwpInitializeMainMenu(
    _In_ HWND WindowHandle
    )
{
    HMENU menuHandle;

    // Initialize the menu.

    if (!(menuHandle = CreateMenu()))
        return;

    PhEMenuToHMenu2(menuHandle, PhpCreateMainMenu(ULONG_MAX), 0, NULL);
    PhInitializeWindowThemeMainMenu(menuHandle);
    PhSetHMenuNotify(menuHandle);
    SetMenu(WindowHandle, menuHandle);

    // Initialize the submenu.

    for (LONG i = 0; i < RTL_NUMBER_OF(SubMenuHandles); i++)
    {
        SubMenuHandles[i] = GetSubMenu(menuHandle, i);
    }
}

/**
 * Dispatches a menu command based on the provided parameters.
 *
 * \param WindowHandle Handle to the window receiving the menu command.
 * \param MenuHandle Handle to the menu containing the command.
 * \param ItemIndex Zero-based index of the menu item.
 * \param ItemId Identifier of the menu item.
 * \param ItemData Additional data associated with the menu item.
 */
VOID PhMwpDispatchMenuCommand(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ ULONG ItemIndex,
    _In_ ULONG ItemId,
    _In_ ULONG_PTR ItemData
    )
{
    switch (ItemId)
    {
    case ID_PLUGIN_MENU_ITEM:
        {
            PPH_EMENU_ITEM menuItem;
            PH_PLUGIN_MENU_INFORMATION menuInfo;

            menuItem = (PPH_EMENU_ITEM)ItemData;

            if (menuItem)
            {
                PhPluginInitializeMenuInfo(&menuInfo, NULL, WindowHandle, 0);
                PhPluginTriggerEMenuItem(&menuInfo, menuItem);
            }

            return;
        }
        break;
    case ID_TRAYICONS_REGISTERED:
        {
            PPH_EMENU_ITEM menuItem;

            menuItem = (PPH_EMENU_ITEM)ItemData;

            if (menuItem)
            {
                PPH_NF_ICON icon;

                icon = menuItem->Context;
                PhNfSetVisibleIcon(icon, !(icon->Flags & PH_NF_ICON_ENABLED));
                PhNfSaveSettings();
            }

            return;
        }
        break;
    case ID_USER_CONNECT:
    case ID_USER_DISCONNECT:
    case ID_USER_LOGOFF:
    case ID_USER_REMOTECONTROL:
    case ID_USER_SENDMESSAGE:
    case ID_USER_PROPERTIES:
        {
            PPH_EMENU_ITEM menuItem;

            menuItem = (PPH_EMENU_ITEM)ItemData;

            if (menuItem && menuItem->Parent)
            {
                SelectedUserSessionId = PtrToUlong(menuItem->Parent->Context);
            }
        }
        break;
    case ID_VIEW_ORGANIZECOLUMNSETS:
        {
            PhShowColumnSetEditorDialog(WindowHandle, SETTING_PROCESS_TREE_COLUMN_SET_CONFIG);
        }
        return;
    case ID_VIEW_SAVECOLUMNSET:
        {
            PPH_EMENU_ITEM menuItem;
            PPH_STRING columnSetName = NULL;

            menuItem = (PPH_EMENU_ITEM)ItemData;

            while (PhaChoiceDialog(
                WindowHandle,
                L"Column Set Name",
                L"Enter a name for this column set:",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &columnSetName,
                NULL,
                NULL
                ))
            {
                if (!PhIsNullOrEmptyString(columnSetName))
                    break;
            }

            if (!PhIsNullOrEmptyString(columnSetName))
            {
                PPH_STRING treeSettings;
                PPH_STRING sortSettings;

                // Query the current column configuration.
                PhSaveSettingsProcessTreeListEx(&treeSettings, &sortSettings);
                // Create the column set for this column configuration.
                PhSaveSettingsColumnSet(SETTING_PROCESS_TREE_COLUMN_SET_CONFIG, columnSetName, treeSettings, sortSettings);

                PhDereferenceObject(treeSettings);
                PhDereferenceObject(sortSettings);
            }
        }
        return;
    case ID_VIEW_LOADCOLUMNSET:
        {
            PPH_EMENU_ITEM menuItem;
            PPH_STRING columnSetName;
            PPH_STRING treeSettings;
            PPH_STRING sortSettings;

            menuItem = (PPH_EMENU_ITEM)ItemData;
            columnSetName = PhCreateString(menuItem->Text);

            // Query the selected column set.
            if (PhLoadSettingsColumnSet(SETTING_PROCESS_TREE_COLUMN_SET_CONFIG, columnSetName, &treeSettings, &sortSettings))
            {
                // Load the column configuration from the selected column set.
                PhLoadSettingsProcessTreeListEx(treeSettings, sortSettings);

                PhDereferenceObject(treeSettings);
                PhDereferenceObject(sortSettings);
            }

            PhDereferenceObject(columnSetName);
        }
        return;
    case ID_COMPUTER_RESTARTBOOTDEVICE:
        {
            PPH_EMENU_ITEM menuItem = (PPH_EMENU_ITEM)ItemData;

            PhUiHandleComputerBootApplicationMenu(WindowHandle, PtrToUlong(menuItem->Context));
        }
        return;
    case ID_COMPUTER_RESTARTFWDEVICE:
        {
            PPH_EMENU_ITEM menuItem = (PPH_EMENU_ITEM)ItemData;

            PhUiHandleComputerFirmwareApplicationMenu(WindowHandle, PtrToUlong(menuItem->Context));
        }
        return;
    }

    SendMessage(WindowHandle, WM_COMMAND, ItemId, 0);
}

/**
 * Creates and returns a notification menu for the system tray or notification area.
 * \return A pointer to a PPH_EMENU structure representing the created notification menu.
 */
PPH_EMENU PhpCreateNotificationMenu(
    VOID
    )
{
    PPH_EMENU_ITEM menuItem;
    ULONG i;
    ULONG id = ULONG_MAX;

    menuItem = PhCreateEMenuItem(0, 0, L"N&otifications", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_ENABLEALL, L"&Enable all", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_DISABLEALL, L"&Disable all", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_NEWPROCESSES, L"New &processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_TERMINATEDPROCESSES, L"T&erminated processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_NEWSERVICES, L"New &services", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_STARTEDSERVICES, L"St&arted services", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_STOPPEDSERVICES, L"St&opped services", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_DELETEDSERVICES, L"&Deleted services", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_MODIFIEDSERVICES, L"&Modified services", NULL, NULL), ULONG_MAX);
    if (WindowsVersion >= WINDOWS_10)
    {
        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_ARRIVEDDEVICES, L"&Arrived devices", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_REMOVEDDEVICES, L"&Removed devices", NULL, NULL), ULONG_MAX);
    }

    for (i = PH_NOTIFY_MINIMUM; i != PH_NOTIFY_MAXIMUM; i <<= 1)
    {
        if (PhMwpNotifyIconNotifyMask & i)
        {
            switch (i)
            {
            case PH_NOTIFY_PROCESS_CREATE:
                id = ID_NOTIFICATIONS_NEWPROCESSES;
                break;
            case PH_NOTIFY_PROCESS_DELETE:
                id = ID_NOTIFICATIONS_TERMINATEDPROCESSES;
                break;
            case PH_NOTIFY_SERVICE_CREATE:
                id = ID_NOTIFICATIONS_NEWSERVICES;
                break;
            case PH_NOTIFY_SERVICE_DELETE:
                id = ID_NOTIFICATIONS_DELETEDSERVICES;
                break;
            case PH_NOTIFY_SERVICE_START:
                id = ID_NOTIFICATIONS_STARTEDSERVICES;
                break;
            case PH_NOTIFY_SERVICE_STOP:
                id = ID_NOTIFICATIONS_STOPPEDSERVICES;
                break;
            case PH_NOTIFY_SERVICE_MODIFIED:
                id = ID_NOTIFICATIONS_MODIFIEDSERVICES;
                break;
            case PH_NOTIFY_DEVICE_ARRIVED:
                id = ID_NOTIFICATIONS_ARRIVEDDEVICES;
                break;
            case PH_NOTIFY_DEVICE_REMOVED:
                id = ID_NOTIFICATIONS_REMOVEDDEVICES;
                break;
            }

            PhSetFlagsEMenuItem(menuItem, id, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }
    }

    return menuItem;
}

BOOLEAN PhMwpExecuteNotificationMenuCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_NOTIFICATIONS_ENABLEALL:
        SetFlag(PhMwpNotifyIconNotifyMask, PH_NOTIFY_VALID_MASK);
        PhSetIntegerSetting(SETTING_ICON_NOTIFY_MASK, PhMwpNotifyIconNotifyMask);
        return TRUE;
    case ID_NOTIFICATIONS_DISABLEALL:
        ClearFlag(PhMwpNotifyIconNotifyMask, PH_NOTIFY_VALID_MASK);
        PhSetIntegerSetting(SETTING_ICON_NOTIFY_MASK, PhMwpNotifyIconNotifyMask);
        return TRUE;
    case ID_NOTIFICATIONS_NEWPROCESSES:
    case ID_NOTIFICATIONS_TERMINATEDPROCESSES:
    case ID_NOTIFICATIONS_NEWSERVICES:
    case ID_NOTIFICATIONS_STARTEDSERVICES:
    case ID_NOTIFICATIONS_STOPPEDSERVICES:
    case ID_NOTIFICATIONS_DELETEDSERVICES:
    case ID_NOTIFICATIONS_MODIFIEDSERVICES:
    case ID_NOTIFICATIONS_ARRIVEDDEVICES:
    case ID_NOTIFICATIONS_REMOVEDDEVICES:
        {
            ULONG bit = 0;

            switch (Id)
            {
            case ID_NOTIFICATIONS_NEWPROCESSES:
                bit = PH_NOTIFY_PROCESS_CREATE;
                break;
            case ID_NOTIFICATIONS_TERMINATEDPROCESSES:
                bit = PH_NOTIFY_PROCESS_DELETE;
                break;
            case ID_NOTIFICATIONS_NEWSERVICES:
                bit = PH_NOTIFY_SERVICE_CREATE;
                break;
            case ID_NOTIFICATIONS_STARTEDSERVICES:
                bit = PH_NOTIFY_SERVICE_START;
                break;
            case ID_NOTIFICATIONS_STOPPEDSERVICES:
                bit = PH_NOTIFY_SERVICE_STOP;
                break;
            case ID_NOTIFICATIONS_DELETEDSERVICES:
                bit = PH_NOTIFY_SERVICE_DELETE;
                break;
            case ID_NOTIFICATIONS_MODIFIEDSERVICES:
                bit = PH_NOTIFY_SERVICE_MODIFIED;
                break;
            case ID_NOTIFICATIONS_ARRIVEDDEVICES:
                bit = PH_NOTIFY_DEVICE_ARRIVED;
                break;
            case ID_NOTIFICATIONS_REMOVEDDEVICES:
                bit = PH_NOTIFY_DEVICE_REMOVED;
                break;
            }

            PhMwpNotifyIconNotifyMask ^= bit;
            PhSetIntegerSetting(SETTING_ICON_NOTIFY_MASK, PhMwpNotifyIconNotifyMask);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Creates and returns a notification settings menu.
 * \return A pointer to a PPH_EMENU structure representing the notification settings menu.
 */
PPH_EMENU PhpCreateNotificationSettingsMenu(
    VOID
    )
{
    PPH_EMENU_ITEM menuItem;

    menuItem = PhCreateEMenuItem(0, 0, L"Settings", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_ENABLEDELAYSTART, L"Enable initialization delay", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_ENABLEPERSISTLAYOUT, L"Enable persistent layout", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_ENABLETRANSPARENTICONS, L"Enable transparent icons", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_ENABLESINGLECLICKICONS, L"Enable single click icons", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_NOTIFICATIONS_RESETPERSISTLAYOUT, L"Reset persistent layout", NULL, NULL), ULONG_MAX);

    if (PhGetIntegerSetting(SETTING_ICON_TRAY_LAZY_START_DELAY))
    {
        PhSetFlagsEMenuItem(menuItem, ID_NOTIFICATIONS_ENABLEDELAYSTART, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (PhGetIntegerSetting(SETTING_ICON_TRAY_PERSIST_GUID_ENABLED))
    {
        PhSetFlagsEMenuItem(menuItem, ID_NOTIFICATIONS_ENABLEPERSISTLAYOUT, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (PhGetIntegerSetting(SETTING_ICON_TRANSPARENCY_ENABLED))
    {
        PhSetFlagsEMenuItem(menuItem, ID_NOTIFICATIONS_ENABLETRANSPARENTICONS, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (PhGetIntegerSetting(SETTING_ICON_SINGLE_CLICK))
    {
        PhSetFlagsEMenuItem(menuItem, ID_NOTIFICATIONS_ENABLESINGLECLICKICONS, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    return menuItem;
}

/**
 * Executes a command from the notification settings menu.
 *
 * \param WindowHandle Handle to the window that receives the command.
 * \param Id Identifier of the notification menu command to execute.
 * \return TRUE if the command was executed successfully, FALSE otherwise.
 */
BOOLEAN PhMwpExecuteNotificationSettingsMenuCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_NOTIFICATIONS_RESETPERSISTLAYOUT:
        {
            EXTERN_C VOID PhNfLoadGuids(VOID);

            PhSetStringSetting(SETTING_ICON_TRAY_GUIDS, L"");

            PhNfLoadGuids();

            PhShowOptionsRestartRequired(WindowHandle);
        }
        return TRUE;
    case ID_NOTIFICATIONS_ENABLEDELAYSTART:
        {
            BOOLEAN lazyTrayIconStartDelayEnabled = !!PhGetIntegerSetting(SETTING_ICON_TRAY_LAZY_START_DELAY);

            PhSetIntegerSetting(SETTING_ICON_TRAY_LAZY_START_DELAY, !lazyTrayIconStartDelayEnabled);

            PhShowOptionsRestartRequired(WindowHandle);
        }
        return TRUE;
    case ID_NOTIFICATIONS_ENABLEPERSISTLAYOUT:
        {
            BOOLEAN persistentTrayIconLayoutEnabled = !!PhGetIntegerSetting(SETTING_ICON_TRAY_PERSIST_GUID_ENABLED);

            PhSetIntegerSetting(SETTING_ICON_TRAY_PERSIST_GUID_ENABLED, !persistentTrayIconLayoutEnabled);

            PhShowOptionsRestartRequired(WindowHandle);
        }
        return TRUE;
    case ID_NOTIFICATIONS_ENABLETRANSPARENTICONS:
        {
            BOOLEAN transparentTrayIconsEnabled = !!PhGetIntegerSetting(SETTING_ICON_TRANSPARENCY_ENABLED);

            EXTERN_C BOOLEAN PhNfTransparencyEnabled;
            PhNfTransparencyEnabled = !transparentTrayIconsEnabled;

            PhSetIntegerSetting(SETTING_ICON_TRANSPARENCY_ENABLED, !transparentTrayIconsEnabled);

            PhShowOptionsRestartRequired(WindowHandle);
        }
        return TRUE;
    case ID_NOTIFICATIONS_ENABLESINGLECLICKICONS:
        {
            BOOLEAN singleClickTrayIconsEnabled = !!PhGetIntegerSetting(SETTING_ICON_SINGLE_CLICK);

            PhSetIntegerSetting(SETTING_ICON_SINGLE_CLICK, !singleClickTrayIconsEnabled);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Creates and returns a pointer to an icon menu.
 * \return Pointer to the created PPH_EMENU structure representing the icon menu.
 */
PPH_EMENU PhpCreateIconMenu(
    VOID
    )
{
    PPH_EMENU menu;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ICON_SHOWHIDEPROCESSHACKER, L"&Show/Hide System Informer", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ICON_SYSTEMINFORMATION, L"System &information", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhpCreateNotificationMenu(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhpCreateNotificationSettingsMenu(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESSES_DUMMY, L"&Processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhpCreateComputerMenu(FALSE), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ICON_EXIT, L"E&xit", NULL, NULL), ULONG_MAX);

    return menu;
}

/**
 * Initializes a submenu in the main window.
 *
 * \param hwnd Handle to the window that owns the menu.
 * \param Menu Pointer to the menu structure to be initialized.
 * \param Index Index specifying which submenu to initialize.
 */
VOID PhMwpInitializeSubMenu(
    _In_ HWND hwnd,
    _In_ PPH_EMENU Menu,
    _In_ ULONG Index
    )
{
    PPH_EMENU_ITEM menuItem;

    if (Index == PH_MENU_ITEM_LOCATION_SYSTEM)
    {
        if (WindowsVersion < WINDOWS_10)
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_RUNASPACKAGE))
                PhDestroyEMenuItem(menuItem);
        }

        if (PhGetOwnTokenAttributes().Elevated)
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_RUNASADMINISTRATOR))
                PhDestroyEMenuItem(menuItem);
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_SHOWDETAILSFORALLPROCESSES))
                PhDestroyEMenuItem(menuItem);
        }
        else
        {
            if (PhGetIntegerSetting(SETTING_ENABLE_BITMAP_SUPPORT))
            {
                HBITMAP shieldBitmap;

                if (shieldBitmap = PhGetShieldBitmap(LayoutWindowDpi, PhSmallIconSize.X, PhSmallIconSize.Y))
                {
                    if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_SHOWDETAILSFORALLPROCESSES))
                        menuItem->Bitmap = shieldBitmap;
                }
            }
        }

        PhMwpSetupComputerMenu(Menu);
    }
    else if (Index == PH_MENU_ITEM_LOCATION_VIEW)
    {
        PPH_EMENU_ITEM trayIconsMenuItem;
        ULONG id = ULONG_MAX;
        ULONG placeholderIndex = ULONG_MAX;

        if (trayIconsMenuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, NULL, ID_VIEW_TRAYICONS))
        {
            // Add menu items for the registered tray icons.

            PhInsertEMenuItem(trayIconsMenuItem, PhpCreateNotificationMenu(), ULONG_MAX);
            PhInsertEMenuItem(trayIconsMenuItem, PhpCreateNotificationSettingsMenu(), ULONG_MAX);
            PhInsertEMenuItem(trayIconsMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);

            for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
            {
                PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

                menuItem = PhCreateEMenuItem(0, ID_TRAYICONS_REGISTERED, icon->Text, NULL, icon);
                PhInsertEMenuItem(trayIconsMenuItem, menuItem, ULONG_MAX);

                // Update the text and check marks on the menu items.

                if (icon->Flags & PH_NF_ICON_ENABLED)
                {
                    menuItem->Flags |= PH_EMENU_CHECKED;
                }

                if (icon->Flags & PH_NF_ICON_UNAVAILABLE)
                {
                    PPH_STRING newText;

                    newText = PhaConcatStrings2(icon->Text, L" (Unavailable)");
                    PhModifyEMenuItem(menuItem, PH_EMENU_MODIFY_TEXT, PH_EMENU_TEXT_OWNED,
                        PhAllocateCopy(newText->Buffer, newText->Length + sizeof(WCHAR)), NULL);
                }
            }
        }

        if (menuItem = PhFindEMenuItemEx(Menu, 0, NULL, ID_VIEW_SECTIONPLACEHOLDER, NULL, &placeholderIndex))
        {
            PhDestroyEMenuItem(menuItem);
            PhMwpInitializeSectionMenuItems(Menu, placeholderIndex);
        }

        if (AlwaysOnTop && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_ALWAYSONTOP)))
            menuItem->Flags |= PH_EMENU_CHECKED;

        id = PH_OPACITY_TO_ID(PhGetIntegerSetting(SETTING_MAIN_WINDOW_OPACITY));

        if (menuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, NULL, id))
            menuItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;

        switch (PhGetIntegerSetting(SETTING_UPDATE_INTERVAL))
        {
        case 500:
            id = ID_UPDATEINTERVAL_FAST;
            break;
        case 1000:
            id = ID_UPDATEINTERVAL_NORMAL;
            break;
        case 2000:
            id = ID_UPDATEINTERVAL_BELOWNORMAL;
            break;
        case 5000:
            id = ID_UPDATEINTERVAL_SLOW;
            break;
        case 10000:
            id = ID_UPDATEINTERVAL_VERYSLOW;
            break;
        default:
            id = ULONG_MAX;
            break;
        }

        if (id != ULONG_MAX && (menuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, NULL, id)))
            menuItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;

        if (PhMwpUpdateAutomatically && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_UPDATEAUTOMATICALLY)))
            menuItem->Flags |= PH_EMENU_CHECKED;
    }
    else if (Index == PH_MENU_ITEM_LOCATION_TOOLS)
    {
        if (WindowsVersion < WINDOWS_8_1)
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_LIVEDUMP))
                PhDestroyEMenuItem(menuItem);
        }

        if (PhGetIntegerSetting(SETTING_ENABLE_BITMAP_SUPPORT))
        {
            HBITMAP shieldBitmap;

            if (shieldBitmap = PhGetShieldBitmap(LayoutWindowDpi, PhSmallIconSize.X, PhSmallIconSize.Y))
            {
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_STARTTASKMANAGER))
                    menuItem->Bitmap = shieldBitmap;
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_STARTRESOURCEMONITOR))
                    menuItem->Bitmap = shieldBitmap;
            }
        }
    }
}

/**
 * Initializes section-related menu items in the specified menu.
 *
 * \param Menu Pointer to the menu structure where section items will be added.
 * \param StartIndex The starting index in the menu where section items should be inserted.
 */
VOID PhMwpInitializeSectionMenuItems(
    _In_ PPH_EMENU Menu,
    _In_ ULONG StartIndex
    )
{
    BOOLEAN removeSeparator = TRUE;
    PH_MAIN_TAB_PAGE_MENU_INFORMATION menuInfo;

    menuInfo.Menu = Menu;
    menuInfo.StartIndex = StartIndex;

    for (ULONG i = PageList->Count; i > 0; i--)
    {
        PPH_MAIN_TAB_PAGE page = PageList->Items[i - 1];

        if (page->Callback(CurrentPage, MainTabPageInitializeSectionMenuItems, &menuInfo, NULL))
            removeSeparator = FALSE;
    }

    if (removeSeparator)
        PhRemoveEMenuItem(Menu, NULL, StartIndex);
}

/**
 * Adjusts the layout of the tab control in the main window by updating the deferred window positioning handle.
 * \param DeferHandle Pointer to a handle used for deferred window positioning (HDWP).
 */
VOID PhMwpLayoutTabControl(
    _Inout_ HDWP *DeferHandle
    )
{
    RECT clientRect;
    RECT tabRect;

    if (!LayoutPaddingValid)
    {
        PhMwpUpdateLayoutPadding();
        LayoutPaddingValid = TRUE;
    }

    if (!PhGetClientRect(PhMainWndHandle, &clientRect))
        return;

    PhMwpApplyLayoutPadding(&clientRect, &LayoutPadding);
    tabRect = clientRect;
    TabCtrl_AdjustRect(TabControlHandle, FALSE, &tabRect);

    if (CurrentPage && CurrentPage->WindowHandle)
    {
        // Remove the tabctrl padding (dmex)
        *DeferHandle = DeferWindowPos(
            *DeferHandle,
            CurrentPage->WindowHandle,
            HWND_TOP,
            clientRect.left,
            tabRect.top - LayoutBorderSize,
            clientRect.right - clientRect.left,
            (tabRect.bottom - tabRect.top) + (clientRect.bottom - tabRect.bottom),
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

/**
 * Handles notifications from a tab control.
 * \param Header Pointer to an NMHDR structure containing information about the notification.
 */
VOID PhMwpNotifyTabControl(
    _In_ NMHDR *Header
    )
{
    if (Header->code == TCN_SELCHANGING)
    {
        OldTabIndex = TabCtrl_GetCurSel(TabControlHandle);
    }
    else if (Header->code == TCN_SELCHANGE)
    {
        PhMwpSelectionChangedTabControl(OldTabIndex);
    }
}

/**
 * Called when the selection in a tab control has changed.
 * \param OldIndex The index of the previously selected tab.
 */
VOID PhMwpSelectionChangedTabControl(
    _In_ LONG OldIndex
    )
{
    LONG selectedIndex;
    HDWP deferHandle;
    ULONG i;

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == OldIndex)
        return;

    deferHandle = BeginDeferWindowPos(3);

    for (i = 0; i < PageList->Count; i++)
    {
        PPH_MAIN_TAB_PAGE page = PageList->Items[i];

        page->Selected = page->Index == selectedIndex;

        if (page->Index == selectedIndex)
        {
            CurrentPage = page;

            // Create the tab page window if it doesn't exist. (wj32)
            if (!page->WindowHandle && !page->CreateWindowCalled)
            {
                if (page->Callback(page, MainTabPageCreateWindow, &page->WindowHandle, PhMainWndHandle))
                    page->CreateWindowCalled = TRUE;

                if (page->WindowHandle)
                    BringWindowToTop(page->WindowHandle);
                if (PhTreeWindowFont)
                    page->Callback(page, MainTabPageFontChanged, PhTreeWindowFont, NULL);
            }

            page->Callback(page, MainTabPageSelected, UlongToPtr(TRUE), NULL);

            if (page->WindowHandle)
            {
                deferHandle = DeferWindowPos(deferHandle, page->WindowHandle, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY);
                SetFocus(page->WindowHandle);
            }
        }
        else if (page->Index == OldIndex)
        {
            page->Callback(page, MainTabPageSelected, UlongToPtr(FALSE), NULL);

            if (page->WindowHandle)
            {
                deferHandle = DeferWindowPos(deferHandle, page->WindowHandle, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY);
            }
        }
    }

    PhMwpLayoutTabControl(&deferHandle);

    EndDeferWindowPos(deferHandle);

    if (OldIndex != INT_ERROR && PhGetIntegerSetting(SETTING_MAIN_WINDOW_TAB_RESTORE_ENABLED) && IsWindowVisible(TabControlHandle))
    {
        PhSetIntegerSetting(SETTING_MAIN_WINDOW_TAB_RESTORE_INDEX, selectedIndex);
    }

    if (PhPluginsEnabled)
    {
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowTabChanged), LongToPtr(selectedIndex));
    }
}

/**
 * Creates a new main tab page based on the specified template.
 *
 * \param Template Pointer to a PPH_MAIN_TAB_PAGE structure that serves as the template for the new page.
 * \return Pointer to the newly created PPH_MAIN_TAB_PAGE structure.
 */
PPH_MAIN_TAB_PAGE PhMwpCreatePage(
    _In_ PPH_MAIN_TAB_PAGE Template
    )
{
    PPH_MAIN_TAB_PAGE page;
    PPH_STRING name;
    //HDWP deferHandle;

    page = PhAllocateZero(sizeof(PH_MAIN_TAB_PAGE));
    page->Name = Template->Name;
    page->Flags = Template->Flags;
    page->Callback = Template->Callback;
    page->Context = Template->Context;

    PhAddItemList(PageList, page);

    name = PhCreateString2(&page->Name);
    page->Index = PhAddTabControlTab(TabControlHandle, MAXINT, name->Buffer);
    PhDereferenceObject(name);

    page->Callback(page, MainTabPageCreate, NULL, NULL);

    // The tab control might need multiple lines, so we need to refresh the layout.
    //deferHandle = BeginDeferWindowPos(1);
    //PhMwpLayoutTabControl(&deferHandle);
    //EndDeferWindowPos(deferHandle);

    return page;
}

/**
 * Selects a page in the main window by its index.
 * \param Index The zero-based index of the page to select.
 */
VOID PhMwpSelectPage(
    _In_ ULONG Index
    )
{
    LONG oldIndex;

    oldIndex = TabCtrl_GetCurSel(TabControlHandle);
    TabCtrl_SetCurSel(TabControlHandle, Index);
    PhMwpSelectionChangedTabControl(oldIndex);
}

/**
 * Finds a main tab page by its name.
 *
 * \param Name A pointer to a PPH_STRINGREF structure containing the name of the page to find.
 * \return A pointer to the PPH_MAIN_TAB_PAGE structure if found, otherwise NULL.
 */
PPH_MAIN_TAB_PAGE PhMwpFindPage(
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;

    for (i = 0; i < PageList->Count; i++)
    {
        PPH_MAIN_TAB_PAGE page = PageList->Items[i];

        if (PhEqualStringRef(&page->Name, Name, TRUE))
            return page;
    }

    return NULL;
}

/**
 * Creates an internal main tab page.
 *
 * \param Name The name of the tab page.
 * \param Flags Flags that specify options for the tab page.
 * \param Callback Pointer to a callback function for the tab page.
 * \return A pointer to the newly created PPH_MAIN_TAB_PAGE structure, or NULL on failure.
 */
PPH_MAIN_TAB_PAGE PhMwpCreateInternalPage(
    _In_ PCWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MAIN_TAB_PAGE_CALLBACK Callback
    )
{
    PH_MAIN_TAB_PAGE page;

    memset(&page, 0, sizeof(PH_MAIN_TAB_PAGE));
    PhInitializeStringRefLongHint(&page.Name, Name);
    page.Flags = Flags;
    page.Callback = Callback;

    return PhMwpCreatePage(&page);
}

/**
 * Notifies all main tab pages of a specified message.
 *
 * \param Message The message to send to all main tab pages.
 * \param Parameter1 Optional parameter to pass with the message.
 * \param Parameter2 Optional parameter to pass with the message.
 */
VOID PhMwpNotifyAllPages(
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    ULONG i;
    PPH_MAIN_TAB_PAGE page;

    if (PageList)
    {
        for (i = 0; i < PageList->Count; i++)
        {
            page = PageList->Items[i];
            page->Callback(page, Message, Parameter1, Parameter2);
        }
    }
}

static int __cdecl IconProcessesCpuUsageCompare(
    _In_ void const* elem1,
    _In_ void const* elem2
    )
{
    PPH_PROCESS_ITEM processItem1 = *(PPH_PROCESS_ITEM *)elem1;
    PPH_PROCESS_ITEM processItem2 = *(PPH_PROCESS_ITEM *)elem2;

    return -singlecmp(processItem1->CpuUsage, processItem2->CpuUsage);
}

static int __cdecl IconProcessesNameCompare(
    _In_ void const* elem1,
    _In_ void const* elem2
    )
{
    PPH_PROCESS_ITEM processItem1 = *(PPH_PROCESS_ITEM *)elem1;
    PPH_PROCESS_ITEM processItem2 = *(PPH_PROCESS_ITEM *)elem2;

    return PhCompareString(processItem1->ProcessName, processItem2->ProcessName, TRUE);
}

VOID PhAddMiniProcessMenuItems(
    _Inout_ PPH_EMENU_ITEM Menu,
    _In_ HANDLE ProcessId
    )
{
    PPH_EMENU_ITEM priorityMenu;
    PPH_EMENU_ITEM ioPriorityMenu = NULL;
    PPH_PROCESS_ITEM processItem;
    BOOLEAN isSuspended = FALSE;
    BOOLEAN isPartiallySuspended = TRUE;

    // Priority

    priorityMenu = PhCreateEMenuItem(0, ID_PROCESS_PRIORITYCLASS, L"&Priority class", NULL, ProcessId);

    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_REALTIME, L"&Real time", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_HIGH, L"&High", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_ABOVENORMAL, L"&Above normal", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_NORMAL, L"&Normal", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_BELOWNORMAL, L"&Below normal", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_IDLE, L"&Idle", NULL, ProcessId), ULONG_MAX);

    // I/O priority

    ioPriorityMenu = PhCreateEMenuItem(0, ID_PROCESS_IOPRIORITY, L"&I/O priority", NULL, ProcessId);

    PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_IOPRIORITY_HIGH, L"&High", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_IOPRIORITY_NORMAL, L"&Normal", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_IOPRIORITY_LOW, L"&Low", NULL, ProcessId), ULONG_MAX);
    PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_IOPRIORITY_VERYLOW, L"&Very low", NULL, ProcessId), ULONG_MAX);

    // Menu

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_TERMINATE, L"T&erminate", NULL, ProcessId), ULONG_MAX);

    if (processItem = PhReferenceProcessItem(ProcessId))
    {
        isSuspended = (BOOLEAN)processItem->IsSuspended;
        isPartiallySuspended = (BOOLEAN)processItem->IsPartiallySuspended;
        PhDereferenceObject(processItem);
    }

    if (!isSuspended)
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_SUSPEND, L"&Suspend", NULL, ProcessId), ULONG_MAX);
    if (isSuspended || isPartiallySuspended)
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_RESUME, L"Res&ume", NULL, ProcessId), ULONG_MAX);

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_RESTART, L"Res&tart", NULL, ProcessId), ULONG_MAX);

    PhInsertEMenuItem(Menu, priorityMenu, ULONG_MAX);

    if (ioPriorityMenu)
        PhInsertEMenuItem(Menu, ioPriorityMenu, ULONG_MAX);

    PhMwpSetProcessMenuPriorityChecks(Menu, ProcessId, TRUE, TRUE, FALSE);

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_PROPERTIES, L"P&roperties", NULL, ProcessId), ULONG_MAX);
}

/**
 * Handles a menu item event for the mini-info-window.
 *
 * \param MenuItem Pointer to a PH_EMENU_ITEM structure representing the selected menu item.
 * \return Returns TRUE if the menu item was handled successfully, otherwise FALSE.
 */
BOOLEAN PhHandleMiniProcessMenuItem(
    _Inout_ PPH_EMENU_ITEM MenuItem
    )
{
    switch (MenuItem->Id)
    {
    case ID_PROCESS_TERMINATE:
    case ID_PROCESS_SUSPEND:
    case ID_PROCESS_RESUME:
    case ID_PROCESS_RESTART:
    case ID_PROCESS_PROPERTIES:
        {
            HANDLE processId = MenuItem->Context;
            PPH_PROCESS_ITEM processItem;

            if (processItem = PhReferenceProcessItem(processId))
            {
                switch (MenuItem->Id)
                {
                case ID_PROCESS_TERMINATE:
                    PhUiTerminateProcesses(PhMainWndHandle, &processItem, 1);
                    break;
                case ID_PROCESS_SUSPEND:
                    PhUiSuspendProcesses(PhMainWndHandle, &processItem, 1);
                    break;
                case ID_PROCESS_RESUME:
                    PhUiResumeProcesses(PhMainWndHandle, &processItem, 1);
                    break;
                case ID_PROCESS_RESTART:
                    PhUiRestartProcess(PhMainWndHandle, processItem);
                    break;
                case ID_PROCESS_PROPERTIES:
                    SystemInformer_ShowProcessProperties(processItem);
                    break;
                }

                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowStatus(PhMainWndHandle, L"The process does not exist.", STATUS_INVALID_CID, 0);
            }
        }
        break;
    case ID_PRIORITY_REALTIME:
    case ID_PRIORITY_HIGH:
    case ID_PRIORITY_ABOVENORMAL:
    case ID_PRIORITY_NORMAL:
    case ID_PRIORITY_BELOWNORMAL:
    case ID_PRIORITY_IDLE:
        {
            HANDLE processId = MenuItem->Context;
            PPH_PROCESS_ITEM processItem;

            if (processItem = PhReferenceProcessItem(processId))
            {
                PhMwpExecuteProcessPriorityClassCommand(PhMainWndHandle, MenuItem->Id, &processItem, 1);
                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowStatus(PhMainWndHandle, L"The process does not exist.", STATUS_INVALID_CID, 0);
            }
        }
        break;
    case ID_IOPRIORITY_HIGH:
    case ID_IOPRIORITY_NORMAL:
    case ID_IOPRIORITY_LOW:
    case ID_IOPRIORITY_VERYLOW:
        {
            HANDLE processId = MenuItem->Context;
            PPH_PROCESS_ITEM processItem;

            if (processItem = PhReferenceProcessItem(processId))
            {
                PhMwpExecuteProcessIoPriorityCommand(PhMainWndHandle, MenuItem->Id, &processItem, 1);
                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowStatus(PhMainWndHandle, L"The process does not exist.", STATUS_INVALID_CID, 0);
            }
        }
        break;
    }

    return FALSE;
}

/**
 * Adds a specified number of icon processes to the given menu.
 *
 * \param Menu Pointer to the menu item where icon processes will be added.
 * \param NumberOfProcesses The number of icon processes to add.
 */
VOID PhMwpAddIconProcesses(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG NumberOfProcesses
    )
{
    ULONG i;
    PPH_PROCESS_ITEM *processItems;
    ULONG numberOfProcessItems;
    PPH_LIST processList;
    PPH_PROCESS_ITEM processItem;

    PhEnumProcessItems(&processItems, &numberOfProcessItems);
    processList = PhCreateList(numberOfProcessItems);
    PhAddItemsList(processList, processItems, numberOfProcessItems);

    // Remove non-real processes.
    for (i = 0; i < processList->Count; i++)
    {
        processItem = processList->Items[i];

        if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
        {
            PhRemoveItemList(processList, i);
            i--;
        }
    }

    // Remove processes with zero CPU usage and those running as other users.
    for (i = 0; i < processList->Count && processList->Count > NumberOfProcesses; i++)
    {
        processItem = processList->Items[i];

        if (
            processItem->CpuUsage == 0 ||
            (processItem->Sid && !PhEqualSid(processItem->Sid, PhGetOwnTokenAttributes().TokenSid))
            )
        {
            PhRemoveItemList(processList, i);
            i--;
        }
    }

    // Sort the processes by CPU usage and remove the extra processes at the end of the list.
    qsort(processList->Items, processList->Count, sizeof(PVOID), IconProcessesCpuUsageCompare);

    if (processList->Count > NumberOfProcesses)
    {
        PhRemoveItemsList(processList, NumberOfProcesses, processList->Count - NumberOfProcesses);
    }

    // Lastly, sort by name.
    qsort(processList->Items, processList->Count, sizeof(PVOID), IconProcessesNameCompare);

    // Delete all menu items.
    PhRemoveAllEMenuItems(Menu);

    // Add the processes.

    for (i = 0; i < processList->Count; i++)
    {
        PPH_EMENU_ITEM subMenu;
        HBITMAP iconBitmap = NULL;
        CLIENT_ID clientId;
        PPH_STRING clientIdName;
        PPH_STRING escapedName;
        HICON icon;

        processItem = processList->Items[i];

        // Process

        clientId.UniqueProcess = processItem->ProcessId;
        clientId.UniqueThread = NULL;

        clientIdName = PH_AUTO(PhGetClientIdName(&clientId));
        escapedName = PH_AUTO(PhEscapeStringForMenuPrefix(&clientIdName->sr));

        subMenu = PhCreateEMenuItem(
            0,
            0,
            escapedName->Buffer,
            NULL,
            processItem->ProcessId
            );

        if (icon = PhGetImageListIcon(processItem->SmallIconIndex, FALSE))
        {
            iconBitmap = PhIconToBitmap(icon, PhSmallIconSize.X, PhSmallIconSize.Y);
            DestroyIcon(icon);
        }

        subMenu->Bitmap = iconBitmap;
        subMenu->Flags |= PH_EMENU_BITMAP_OWNED; // automatically destroy the bitmap when necessary

        PhAddMiniProcessMenuItems(subMenu, processItem->ProcessId);
        PhInsertEMenuItem(Menu, subMenu, ULONG_MAX);
    }

    PhDereferenceObject(processList);
    PhDereferenceObjects(processItems, numberOfProcessItems);
    PhFree(processItems);
}

/**
 * Displays the context menu for the application's icon at the specified location.
 *
 * \param WindowHandle Handle to the window that owns the icon.
 * \param Location Screen coordinates where the context menu should appear.
 */
VOID PhShowIconContextMenu(
    _In_ HWND WindowHandle,
    _In_ PPOINT Location
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;
    PH_PLUGIN_MENU_INFORMATION menuInfo;
    ULONG numberOfProcesses;

    // This function seems to be called recursively under some circumstances.
    // To reproduce:
    // 1. Hold right mouse button on tray icon, then left click.
    // 2. Make the menu disappear by clicking on the menu then clicking somewhere else.
    // So, don't store any global state or bad things will happen.

    menu = PhpCreateIconMenu();

    // Add processes to the menu.

    numberOfProcesses = PhGetIntegerSetting(SETTING_ICON_PROCESSES);
    item = PhFindEMenuItem(menu, 0, 0, ID_PROCESSES_DUMMY);

    if (item)
        PhMwpAddIconProcesses(item, numberOfProcesses);

    // Fix up the Computer menu.
    PhMwpSetupComputerMenu(menu);

    // Give plugins a chance to modify the menu.

    if (PhPluginsEnabled)
    {
        PhPluginInitializeMenuInfo(&menuInfo, menu, WindowHandle, 0);
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackIconMenuInitializing), &menuInfo);
    }

    SetForegroundWindow(WindowHandle); // window must be foregrounded so menu will disappear properly (wj32)
    item = PhShowEMenu(
        menu,
        WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        Location->x,
        Location->y
        );

    if (item)
    {
        BOOLEAN handled = FALSE;

        if (PhPluginsEnabled && !handled)
            handled = PhPluginTriggerEMenuItem(&menuInfo, item);

        if (!handled)
            handled = PhHandleMiniProcessMenuItem(item);

        if (!handled)
            handled = PhMwpExecuteComputerCommand(WindowHandle, item->Id);

        if (!handled)
            handled = PhMwpExecuteNotificationMenuCommand(WindowHandle, item->Id);

        if (!handled)
            handled = PhMwpExecuteNotificationSettingsMenuCommand(WindowHandle, item->Id);

        if (!handled)
        {
            switch (item->Id)
            {
            case ID_ICON_SHOWHIDEPROCESSHACKER:
                SystemInformer_ToggleVisible(FALSE);
                break;
            case ID_ICON_SYSTEMINFORMATION:
                SendMessage(WindowHandle, WM_COMMAND, ID_VIEW_SYSTEMINFORMATION, 0);
                break;
            case ID_ICON_EXIT:
                SendMessage(WindowHandle, WM_COMMAND, ID_HACKER_EXIT, 0);
                break;
            }
        }
    }

    PhDestroyEMenu(menu);
}

VOID PhShowIconNotification(
    _In_ PCWSTR Title,
    _In_ PCWSTR Text
    )
{
    PhNfShowBalloonTip(Title, Text, 10);
}

HRESULT PhShowIconNotificationEx(
    _In_ PCWSTR Title,
    _In_ PCWSTR Text,
    _In_ ULONG Timeout,
    _In_opt_ PPH_TOAST_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    return PhNfShowBalloonTipEx(Title, Text, Timeout, Callback, Context);
}

VOID PhShowDetailsForIconNotification(
    VOID
    )
{
    switch (PhMwpLastNotificationType)
    {
    case PH_NOTIFY_PROCESS_CREATE:
        {
            PPH_PROCESS_NODE processNode;

            if (processNode = PhFindProcessNode(PhMwpLastNotificationDetails.ProcessId))
            {
                SystemInformer_SelectTabPage(PhMwpProcessesPage->Index);
                SystemInformer_SelectProcessNode(processNode);
                SystemInformer_ToggleVisible(TRUE);
            }
            else
            {
                PhShowStatus(PhMainWndHandle, L"The process does not exist.", STATUS_INVALID_CID, 0);
            }
        }
        break;
    case PH_NOTIFY_SERVICE_CREATE:
    case PH_NOTIFY_SERVICE_START:
    case PH_NOTIFY_SERVICE_STOP:
        {
            PPH_SERVICE_ITEM serviceItem;

            if (PhMwpLastNotificationDetails.ServiceName &&
                (serviceItem = PhReferenceServiceItem(&PhMwpLastNotificationDetails.ServiceName->sr)))
            {
                SystemInformer_SelectTabPage(PhMwpServicesPage->Index);
                SystemInformer_SelectServiceItem(serviceItem);
                SystemInformer_ToggleVisible(TRUE);

                PhDereferenceObject(serviceItem);
            }
            else
            {
                PhShowStatus(PhMainWndHandle, L"The service does not exist.", STATUS_INVALID_CID, 0);
            }
        }
        break;
    }
}

VOID PhMwpClearLastNotificationDetails(
    VOID
    )
{
    switch (PhMwpLastNotificationType)
    {
    case PH_NOTIFY_SERVICE_CREATE:
    case PH_NOTIFY_SERVICE_START:
    case PH_NOTIFY_SERVICE_STOP:
        PhClearReference(&PhMwpLastNotificationDetails.ServiceName);
        break;
    }

    PhMwpLastNotificationType = 0;
    memset(&PhMwpLastNotificationDetails, 0, sizeof(PhMwpLastNotificationDetails));
}

// Window plugin extensions (dmex)

VOID PhMwpInvokeShowMemoryEditorDialog(
    _In_ PVOID Parameter
    )
{
    PPH_SHOW_MEMORY_EDITOR showMemoryEditor = (PPH_SHOW_MEMORY_EDITOR)Parameter;

    PhShowMemoryEditorDialog(
        showMemoryEditor->OwnerWindow,
        showMemoryEditor->ProcessId,
        showMemoryEditor->BaseAddress,
        showMemoryEditor->RegionSize,
        showMemoryEditor->SelectOffset,
        showMemoryEditor->SelectLength,
        showMemoryEditor->Title,
        showMemoryEditor->Flags
        );
    PhClearReference(&showMemoryEditor->Title);
    PhFree(showMemoryEditor);
}

VOID PhMwpInvokeShowMemoryResultsDialog(
    _In_ PVOID Parameter
    )
{
    PPH_SHOW_MEMORY_RESULTS showMemoryResults = (PPH_SHOW_MEMORY_RESULTS)Parameter;

    PhShowMemoryResultsDialog(
        showMemoryResults->ProcessId,
        showMemoryResults->Results
        );
    PhDereferenceMemoryResults(
        (PPH_MEMORY_RESULT*)showMemoryResults->Results->Items,
        showMemoryResults->Results->Count
        );
    PhDereferenceObject(showMemoryResults->Results);
    PhFree(showMemoryResults);
}

VOID PhMwpInvokeUpdateWindowFont(
    _In_opt_ PVOID Parameter
    )
{
    HFONT oldFont = PhTreeWindowFont;
    HFONT newFont;
    PPH_STRING fontHexString;
    LOGFONT font;

    fontHexString = PhaGetStringSetting(SETTING_FONT);

    if (
        fontHexString->Length / sizeof(WCHAR) / 2 == sizeof(LOGFONT) &&
        PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)&font)
        )
    {
        if (!(newFont = CreateFontIndirect(&font)))
            return;
    }
    else
    {
        if (!(newFont = PhCreateIconTitleFont(LayoutWindowDpi)))
            return;
    }

    PhTreeWindowFont = newFont;
    SetWindowFont(TabControlHandle, PhTreeWindowFont, TRUE);
    PhMwpNotifyAllPages(MainTabPageFontChanged, newFont, NULL);

    if (oldFont) DeleteFont(oldFont);
}

VOID PhMwpInvokeUpdateWindowFontMonospace(
    _In_ HWND hwnd,
    _In_opt_ PVOID Parameter
    )
{
    HFONT oldFont = PhMonospaceFont;
    HFONT newFont;
    PPH_STRING fontHexString;
    LOGFONT font;

    fontHexString = PhaGetStringSetting(SETTING_FONT_MONOSPACE);

    if (
        fontHexString->Length / sizeof(WCHAR) / 2 == sizeof(LOGFONT) &&
        PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)&font)
        )
    {
        if (!(newFont = CreateFontIndirect(&font)))
            return;
    }
    else
    {
        PhMonospaceFont = PhInitializeMonospaceFont(LayoutWindowDpi);
        if (oldFont) DeleteFont(oldFont);
        return;
    }

    PhMonospaceFont = newFont;
    if (oldFont) DeleteFont(oldFont);
}

VOID PhMwpInvokePrepareEarlyExit(
    _In_ HWND WindowHandle
    )
{
    PhMwpSaveSettings(WindowHandle);
    PhMainWndEarlyExit = TRUE;
}

VOID PhMwpInvokeActivateWindow(
    _In_ BOOLEAN Toggle
    )
{
    PhMwpActivateWindow(PhMainWndHandle, Toggle);
}

VOID PhMwpInvokeSelectTabPage(
    _In_ PVOID Parameter
    )
{
    ULONG index = PtrToUlong(Parameter);

    PhMwpSelectPage(index);

    if (CurrentPage->WindowHandle)
        SetFocus(CurrentPage->WindowHandle);
}

VOID PhMwpInvokeSelectServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PPH_SERVICE_NODE serviceNode;

    PhMwpNeedServiceTreeList();

    // For compatibility, LParam is a service item, not node.
    if (serviceNode = PhFindServiceNode(ServiceItem))
    {
        PhSelectAndEnsureVisibleServiceNode(serviceNode);
    }
}

VOID PhMwpInvokeSelectNetworkItem(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PPH_NETWORK_NODE networkNode;

    PhMwpNeedNetworkTreeList();

    // For compatibility, LParam is a service item, not node.
    if (networkNode = PhFindNetworkNode(NetworkItem))
    {
        PhSelectAndEnsureVisibleNetworkNode(networkNode);
    }
}

BOOLEAN PhMwpPluginNotifyEvent(
    _In_ ULONG Type,
    _In_ PVOID Parameter
    )
{
    PH_PLUGIN_NOTIFY_EVENT notifyEvent;

    notifyEvent.Type = Type;
    notifyEvent.Handled = FALSE;
    notifyEvent.Parameter = Parameter;

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNotifyEvent), &notifyEvent);

    return notifyEvent.Handled;
}

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _PH_INVOKE_ENTRY
{
    SLIST_ENTRY ListEntry;
    PVOID Command;
    PVOID Parameter;
    //HANDLE ThreadId;
    //ULONG64 SubmitTime;
} PH_INVOKE_ENTRY, * PPH_INVOKE_ENTRY;

SLIST_HEADER PhMainThreadInvokeQueue;
PH_FREE_LIST PhMainThreadInvokeQueueFreeList;

NTSTATUS PhInvokeOnMainThread(
    _In_opt_ PVOID Command,
    _In_opt_ PVOID Parameter
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_INVOKE_ENTRY entry;

    if (PhBeginInitOnce(&initOnce))
    {
        PhInitializeSListHead(&PhMainThreadInvokeQueue);
        PhInitializeFreeList(&PhMainThreadInvokeQueueFreeList, sizeof(PH_INVOKE_ENTRY), 5);
        PhEndInitOnce(&initOnce);
    }

    entry = PhAllocateFromFreeList(&PhMainThreadInvokeQueueFreeList);
    entry->Command = Command;
    entry->Parameter = Parameter;
    //entry->ThreadId = NtCurrentThreadId();
    //entry->SubmitTime = NtGetTickCount64();

    RtlInterlockedPushEntrySList(&PhMainThreadInvokeQueue, &entry->ListEntry);

    //static ULONG64 LastInvokeTicks = 0;
    //ULONG64 currentTicks;
    //currentTicks = NtGetTickCount64();
    //if ((currentTicks - LastInvokeTicks) < 100)
    //    dprintf("Coalesced invoke message (%llu)\n", (currentTicks - LastInvokeTicks));
    //else LastInvokeTicks = currentTicks;
    PostMessage(PhMainWndHandle, WM_PH_INVOKE, 0, 0);

    return STATUS_SUCCESS;
}

VOID PhProcessInvokeQueue(
    VOID
    )
{
    PSLIST_ENTRY listEntry;
    PPH_INVOKE_ENTRY entry;

    while ((listEntry = RtlInterlockedPopEntrySList(&PhMainThreadInvokeQueue)) != NULL)
    {
        entry = CONTAINING_RECORD(listEntry, PH_INVOKE_ENTRY, ListEntry);

        {
            VOID (NTAPI* function)(PVOID);

            function = entry->Command;
            function(entry->Parameter);
        }

        //PH_PLUGIN_INVOKE_EVENT event;
        //
        //memset(&event, 0, sizeof(PH_PLUGIN_INVOKE_EVENT));
        //event.Id = entry->Command;
        //event.Parameter = entry->Parameter;
        //
        //PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowThread), &event);

        PhFreeToFreeList(&PhMainThreadInvokeQueueFreeList, entry);
    }
}

// Exports for plugin support (dmex)

PVOID PhPluginInvokeWindowCallback(
    _In_ PH_MAINWINDOW_CALLBACK_TYPE Event,
    _In_opt_ PVOID wparam,
    _In_opt_ PVOID lparam
    )
{
    switch (Event)
    {
    case PH_MAINWINDOW_CALLBACK_TYPE_DESTROY:
        {
            SendMessage(PhMainWndHandle, WM_PH_DESTROY, 0, 0);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SHOW_PROPERTIES:
        {
            SendMessage(PhMainWndHandle, WM_PH_SHOW_PROPERTIES, 0, (LPARAM)lparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SAVE_ALL_SETTINGS:
        {
            SendMessage(PhMainWndHandle, WM_PH_SAVE_ALL_SETTINGS, 0, 0);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_PREPARE_FOR_EARLY_SHUTDOWN:
        {
            SendMessage(PhMainWndHandle, WM_PH_PREPARE_FOR_EARLY_SHUTDOWN, 0, 0);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_CANCEL_EARLY_SHUTDOWN:
        {
            PhMainWndEarlyExit = FALSE;
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_TOGGLE_VISIBLE:
        {
            BOOLEAN visibility = !(BOOLEAN)(ULONG_PTR)wparam;

            SendMessage(PhMainWndHandle, WM_PH_ACTIVATE_WINDOW, 0, (LPARAM)visibility);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_ICON_CLICK:
        {
            BOOLEAN visibility = !!PhGetIntegerSetting(SETTING_ICON_TOGGLES_VISIBILITY);

            SendMessage(PhMainWndHandle, WM_PH_ACTIVATE_WINDOW, 0, (LPARAM)visibility);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_EDITOR:
        {
            PPH_SHOW_MEMORY_EDITOR showMemoryEditor = (PPH_SHOW_MEMORY_EDITOR)lparam;

            PostMessage(PhMainWndHandle, WM_PH_SHOW_EDITOR, (WPARAM)showMemoryEditor, 0);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_RESULTS:
        {
            PPH_SHOW_MEMORY_RESULTS showMemoryResults = (PPH_SHOW_MEMORY_RESULTS)lparam;

            PostMessage(PhMainWndHandle, WM_PH_SHOW_RESULT, (WPARAM)showMemoryResults, 0);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_TAB_PAGE:
        {
            SendMessage(PhMainWndHandle, WM_PH_SELECT_NODE, (WPARAM)1, (LPARAM)lparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_GET_CALLBACK_LAYOUT_PADDING:
        {
            return (PVOID)&LayoutPaddingCallback;
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_INVALIDATE_LAYOUT_PADDING:
        {
            LayoutPaddingValid = FALSE;
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_PROCESS_NODE:
        {
            SendMessage(PhMainWndHandle, WM_PH_SELECT_NODE, (WPARAM)2, (LPARAM)lparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_SERVICE_ITEM:
        {
            SendMessage(PhMainWndHandle, WM_PH_SELECT_NODE, (WPARAM)3, (LPARAM)lparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_NETWORK_ITEM:
        {
            SendMessage(PhMainWndHandle, WM_PH_SELECT_NODE, (WPARAM)4, (LPARAM)lparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_UPDATE_FONT:
        {
            SendMessage(PhMainWndHandle, WM_PH_SELECT_NODE, (WPARAM)5, (LPARAM)lparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_GET_FONT:
        {
            return PhTreeWindowFont; // (PVOID)GetWindowFont(PhMwpProcessTreeNewHandle);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_INVOKE:
        {
            PhInvokeOnMainThread((PVOID)(ULONG_PTR)lparam, (PVOID)wparam);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_REFRESH:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_REFRESH, 0);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_CREATE_TAB_PAGE:
        {
            return NULL; //(PVOID)SendMessage(PhMainWndHandle, WM_PH_CALLBACK, (WPARAM)lparam, (LPARAM)PhMwpCreatePage);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_GET_UPDATE_AUTOMATICALLY:
        {
            return (PVOID)UlongToPtr(PhMwpUpdateAutomatically);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SET_UPDATE_AUTOMATICALLY:
        {
            BOOLEAN updateAutomatically = !!(BOOLEAN)(ULONG_PTR)wparam;

            if (updateAutomatically != PhMwpUpdateAutomatically)
            {
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_UPDATEAUTOMATICALLY, 0);
            }
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_BASE:
        {
            return (PVOID)NtCurrentImageBase();
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_GETWINDOW_PROCEDURE:
        {
            return (PVOID)PhMwpWndProc; // (WNDPROC)GetWindowLongPtr(PhMainWndHandle, GWLP_WNDPROC);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SETWINDOW_PROCEDURE:
        {
            PhMainWndProc = (WNDPROC)wparam; // (WNDPROC)GetWindowLongPtr(PhMainWndHandle, GWLP_WNDPROC);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_HANDLE:
        {
            return (PVOID)PhMainWndHandle;
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_VERSION:
        {
            return UlongToPtr(WindowsVersion);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_PORTABLE:
        {
            return (PVOID)UlongToPtr(PhPortableEnabled);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_WINDOWDPI:
        {
            return (PVOID)UlongToPtr(LayoutWindowDpi);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_WINDOWNAME:
        {
            return (PVOID)PhApplicationName;
        }
        break;
    }

    return NULL;
}

PVOID PhPluginCreateTabPage(
    _In_ PVOID Page
    )
{
    return PhMwpCreatePage(Page);
}

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
#include <phplug.h>
#include <phsvccl.h>
#include <procprv.h>
#include <proctree.h>
#include <secedit.h>
#include <settings.h>
#include <srvlist.h>
#include <srvprv.h>

#include <mainwndp.h>

PHAPPAPI HWND PhMainWndHandle = NULL;
BOOLEAN PhMainWndExiting = FALSE;
BOOLEAN PhMainWndEarlyExit = FALSE;

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
static INT OldTabIndex = 0;

static HMENU SubMenuHandles[5];
static PPH_EMENU SubMenuObjects[5];
static ULONG SelectedUserSessionId = ULONG_MAX;

PPH_STRING PhGetMainWindowTitle(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING currentUserName;

    if (!PhEnableWindowText)
        return NULL;

    PhInitializeStringBuilder(&stringBuilder, 100);
    PhAppendStringBuilder2(&stringBuilder, PhApplicationName);

    if (currentUserName = PhGetSidFullName(PhGetOwnTokenAttributes().TokenSid, TRUE, NULL))
    {
        PhAppendStringBuilder2(&stringBuilder, L" [");
        PhAppendStringBuilder(&stringBuilder, &currentUserName->sr);
        PhAppendCharStringBuilder(&stringBuilder, L']');
        PhDereferenceObject(currentUserName);
    }

    switch (KphLevel())
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

BOOLEAN PhMainWndInitialization(
    _In_ INT ShowCommand
    )
{
    RTL_ATOM windowAtom;
    PPH_STRING windowName;
    PH_RECTANGLE windowRectangle = {0};
    RECT rect;
    HMENU windowMenuHandle = NULL;
    LONG dpiValue;

    // Set FirstRun default settings.

    if (PhGetIntegerSetting(L"FirstRun"))
        PhSetIntegerSetting(L"FirstRun", FALSE);

    // Initialize the window.

    if ((windowAtom = PhMwpInitializeWindowClass()) == INVALID_ATOM)
        return FALSE;

    windowRectangle.Position = PhGetIntegerPairSetting(L"MainWindowPosition");

    rect = PhRectangleToRect(windowRectangle);
    dpiValue = PhGetMonitorDpi(&rect);

    windowRectangle.Size = PhGetScalableIntegerPairSetting(L"MainWindowSize", TRUE, dpiValue).Pair;

    // Create the window title.
    if (!(windowName = PhGetMainWindowTitle()))
    {
        PhApplicationName = L" "; // Remove dialog window title when disabled (dmex)
    }

    // Create the window.
    PhMainWndHandle = CreateWindow(
        MAKEINTATOM(windowAtom),
        PhGetString(windowName),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
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

    PhClearReference(&windowName);

    // Choose a more appropriate rectangle for the window.
    PhAdjustRectangleToWorkingArea(PhMainWndHandle, &windowRectangle);
    MoveWindow(
        PhMainWndHandle,
        windowRectangle.Left, windowRectangle.Top,
        windowRectangle.Width, windowRectangle.Height,
        FALSE
        );
    UpdateWindow(PhMainWndHandle);

    // Allow WM_PH_ACTIVATE to pass through UIPI. (wj32)
    if (PhGetOwnTokenAttributes().Elevated)
    {
        ChangeWindowMessageFilterEx(PhMainWndHandle, WM_PH_ACTIVATE, MSGFLT_ADD, NULL);
    }

    // Initialize window metrics. (dmex)
    PhMwpInitializeMetrics(PhMainWndHandle);

    // Initialize child controls.
    PhMwpInitializeControls(PhMainWndHandle);

    PhMwpOnSettingChange(PhMainWndHandle);

    PhMwpLoadSettings(PhMainWndHandle);
    PhLogInitialization();

    PhInitializeWindowTheme(PhMainWndHandle, PhEnableThemeSupport); // HACK

    // Create the main menu. This has to be done after initializing the window theme
    // (subclassing the main window) because Windows 10 doesn't forward WM_MEASUREITEM
    // messages to window subclasses for menus created with CreateMenu and set with SetMenu
    // (and only when the DPI is above 100%) unlike previous versions of Windows. (dmex)
    if (windowMenuHandle = CreateMenu())
    {
        SetMenu(PhMainWndHandle, windowMenuHandle);
        PhEMenuToHMenu2(windowMenuHandle, PhpCreateMainMenu(ULONG_MAX), 0, NULL);
        PhMwpInitializeMainMenu(windowMenuHandle);
        if (PhEnableThemeSupport) PhInitializeWindowThemeMainMenu(windowMenuHandle);
    }

    // Initialize the main providers.
    PhMwpInitializeProviders();

    // Queue delayed init functions.
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhMwpLoadStage1Worker, PhMainWndHandle);

    // Perform a layout.
    PhMwpSelectionChangedTabControl(ULONG_MAX);
    PhMwpOnSize(PhMainWndHandle);

    if ((PhStartupParameters.ShowHidden || PhGetIntegerSetting(L"StartHidden")) && PhNfIconsEnabled())
        ShowCommand = SW_HIDE;
    if (PhStartupParameters.ShowVisible)
        ShowCommand = SW_SHOW;

    if (PhGetIntegerSetting(L"MainWindowState") == SW_MAXIMIZE)
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
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowShowing), IntToPtr(ShowCommand));

    if (PhStartupParameters.SelectTab)
    {
        PPH_MAIN_TAB_PAGE page = PhMwpFindPage(&PhStartupParameters.SelectTab->sr);

        if (page)
            PhMwpSelectPage(page->Index);
    }
    else
    {
        if (PhGetIntegerSetting(L"MainWindowTabRestoreEnabled"))
            PhMwpSelectPage(PhGetIntegerSetting(L"MainWindowTabRestoreIndex"));
    }

    if (PhStartupParameters.SysInfo)
        PhShowSystemInformationDialog(PhStartupParameters.SysInfo->Buffer);

    if (ShowCommand != SW_HIDE)
    {
        ShowWindow(PhMainWndHandle, ShowCommand);
        SetForegroundWindow(PhMainWndHandle);
    }

    if (PhGetIntegerSetting(L"MiniInfoWindowPinned"))
        PhPinMiniInformation(MiniInfoManualPinType, 1, 0, PH_MINIINFO_LOAD_POSITION, NULL, NULL);

    return TRUE;
}

LRESULT CALLBACK PhMwpWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            PhProcessImageListInitialization(hWnd);
        }
        break;
    case WM_DESTROY:
        {
            PhMwpOnDestroy(hWnd);
        }
        break;
    case WM_DPICHANGED:
        {
            PhMwpInitializeMetrics(hWnd);

            PhGuiSupportUpdateSystemMetrics(hWnd);

            if (PhEnableWindowText)
            {
                PhSetApplicationWindowIcon(PhMainWndHandle);
            }

            PhProcessImageListInitialization(hWnd);
            TreeNew_SetImageList(PhMwpProcessTreeNewHandle, PhProcessSmallImageList);
            TreeNew_SetImageList(PhMwpServiceTreeNewHandle, PhProcessSmallImageList);
            TreeNew_SetImageList(PhMwpNetworkTreeNewHandle, PhProcessSmallImageList);

            PhMwpOnSettingChange(hWnd);

            PhMwpInvokeUpdateWindowFont(NULL);
            if (PhGetIntegerSetting(L"EnableMonospaceFont"))
                PhMwpInvokeUpdateWindowFontMonospace(hWnd, NULL);
            InvalidateRect(hWnd, NULL, TRUE);

            PhMwpNotifyAllPages(MainTabPageDpiChanged, NULL, NULL);
        }
        break;
    case WM_ENDSESSION:
        {
            PhMwpOnEndSession(hWnd);
        }
        break;
    case WM_SETTINGCHANGE:
        {
            PhMwpOnSettingChange(hWnd);
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
            PhMwpOnSize(hWnd);
        }
        break;
    case WM_SIZING:
        {
            PhMwpOnSizing((ULONG)wParam, (PRECT)lParam);
        }
        break;
    case WM_SETFOCUS:
        {
            PhMwpOnSetFocus();
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
            MSG message;

            memset(&message, 0, sizeof(MSG));
            message.hwnd = hWnd;
            message.message = uMsg;
            message.wParam = wParam;
            message.lParam = lParam;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &message);
        }
        break;
    }

    if (uMsg >= WM_PH_FIRST && uMsg <= WM_PH_LAST)
    {
        return PhMwpOnUserMessage(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

RTL_ATOM PhMwpInitializeWindowClass(
    VOID
    )
{
    WNDCLASSEX wcex;
    PPH_STRING className;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = PhMwpWndProc;
    wcex.hInstance = PhInstanceHandle;
    className = PhaGetStringSetting(L"MainWindowClassName");
    wcex.lpszClassName = PhGetStringOrDefault(className, L"MainWindowClassName");
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (PhEnableWindowText)
    {
        wcex.hIcon = PhGetApplicationIcon(FALSE);
        wcex.hIconSm = PhGetApplicationIcon(TRUE);
    }

    return RegisterClassEx(&wcex);
}

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
    PhRegisterProvider(&PhPrimaryProviderThread, PhNetworkProviderUpdate, NULL, &PhMwpNetworkProviderRegistration);

    PhSetEnabledProvider(&PhMwpProcessProviderRegistration, TRUE);
    PhSetEnabledProvider(&PhMwpServiceProviderRegistration, TRUE);

    PhStartProviderThread(&PhPrimaryProviderThread);
    PhStartProviderThread(&PhSecondaryProviderThread);
    PhStartProviderThread(&PhTertiaryProviderThread);
}

VOID PhMwpApplyUpdateInterval(
    _In_ ULONG Interval
    )
{
    PhSetIntervalProviderThread(&PhPrimaryProviderThread, Interval);
    PhSetIntervalProviderThread(&PhSecondaryProviderThread, Interval);
    PhSetIntervalProviderThread(&PhTertiaryProviderThread, Interval);
}

VOID PhMwpInitializeMetrics(
    _In_ HWND WindowHandle
    )
{
    LayoutWindowDpi = PhGetWindowDpi(WindowHandle);
    LayoutBorderSize = PhGetSystemMetrics(SM_CXBORDER, LayoutWindowDpi);
}

VOID PhMwpInitializeControls(
    _In_ HWND WindowHandle
    )
{
    ULONG thinRows;
    ULONG treelistBorder;
    ULONG treelistCustomColors;
    ULONG treelistCustomHeaderDraw;
    PH_TREENEW_CREATEPARAMS treelistCreateParams = { 0 };

    thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
    treelistBorder = (PhGetIntegerSetting(L"TreeListBorderEnable") && !PhEnableThemeSupport) ? WS_BORDER : 0;
    treelistCustomColors = PhGetIntegerSetting(L"TreeListCustomColorsEnable") ? TN_STYLE_CUSTOM_COLORS : 0;
    treelistCustomHeaderDraw = PhGetIntegerSetting(L"TreeListEnableHeaderTotals") ? TN_STYLE_CUSTOM_HEADERDRAW : 0;

    if (treelistCustomColors)
    {
        treelistCreateParams.TextColor = PhGetIntegerSetting(L"TreeListCustomColorText");
        treelistCreateParams.FocusColor = PhGetIntegerSetting(L"TreeListCustomColorFocus");
        treelistCreateParams.SelectionColor = PhGetIntegerSetting(L"TreeListCustomColorSelection");
    }

    TabControlHandle = CreateWindow(
        WC_TABCONTROL,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_MULTILINE,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        PhInstanceHandle,
        NULL
        );

    PhMwpProcessTreeNewHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | TN_STYLE_ANIMATE_DIVIDER | thinRows | treelistBorder | treelistCustomColors | treelistCustomHeaderDraw,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        PhInstanceHandle,
        &treelistCreateParams
        );

    PhMwpServiceTreeNewHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows | treelistBorder | treelistCustomColors,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        PhInstanceHandle,
        &treelistCreateParams
        );

    PhMwpNetworkTreeNewHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows | treelistBorder | treelistCustomColors,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        PhInstanceHandle,
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

    if (PhGetIntegerSetting(L"TreeListCustomRowSize"))
    {
        ULONG treelistCustomRowSize = PhGetIntegerSetting(L"TreeListCustomRowSize");

        if (treelistCustomRowSize < 15)
            treelistCustomRowSize = 15;

        TreeNew_SetRowHeight(PhMwpProcessTreeNewHandle, treelistCustomRowSize);
        TreeNew_SetRowHeight(PhMwpServiceTreeNewHandle, treelistCustomRowSize);
        TreeNew_SetRowHeight(PhMwpNetworkTreeNewHandle, treelistCustomRowSize);
    }

    CurrentPage = PageList->Items[0];
}

NTSTATUS PhMwpLoadStage1Worker(
    _In_ PVOID Parameter
    )
{
    // If the update interval is too large, the user might have to wait a while before seeing some types of
    // process-related data. We force an update by boosting the provider shortly after the program
    // starts up to make things appear more quickly.

    if (PhCsUpdateInterval > PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM)
    {
        PhBoostProvider(&PhMwpProcessProviderRegistration, NULL);
        PhBoostProvider(&PhMwpServiceProviderRegistration, NULL);
    }

    PhNfLoadStage2();

    if (PhGetIntegerSetting(L"EnableLastProcessShutdown"))
    {
        // Make sure we get closed late in the shutdown process.
        // This is needed for the shutdown cancel debugging scenario included with Task Manager.
        // Note: Windows excludes system binaries from the shutdown dialog while not excluding
        // programs registered for late shutdown, so when services delay shutdown they won't be shown
        // to the user while we are shown and this has caused some users to blame us instead. (dmex)

        //SetProcessShutdownParameters(0x100, SHUTDOWN_NORETRY);
        SetProcessShutdownParameters(0x1, SHUTDOWN_NORETRY);
    }

    DelayedLoadCompleted = TRUE;
    //PostMessage((HWND)Parameter, WM_PH_DELAYED_LOAD_COMPLETED, 0, 0);

    //if (PhEnableThemeSupport)
    DrawMenuBar(PhMainWndHandle);

    return STATUS_SUCCESS;
}

VOID PhMwpOnDestroy(
    _In_ HWND WindowHandle
    )
{
    PhMainWndExiting = TRUE;

    // Notify pages and plugins that we are shutting down.

    PhMwpNotifyAllPages(MainTabPageUpdateAutomaticallyChanged, UlongToPtr(FALSE), NULL); // disable providers (dmex)
    PhMwpNotifyAllPages(MainTabPageDestroy, NULL, NULL);

    if (PhPluginsEnabled)
        PhUnloadPlugins();

    if (!PhMainWndEarlyExit)
        PhMwpSaveSettings(WindowHandle);

    PhNfUninitialization();

    PostQuitMessage(0);
}

VOID PhMwpOnEndSession(
    _In_ HWND WindowHandle
    )
{
    PhMwpOnDestroy(WindowHandle);
}

VOID PhMwpOnSettingChange(
    _In_ HWND hwnd
    )
{
    PhInitializeFont(hwnd);

    if (PhGetIntegerSetting(L"EnableMonospaceFont"))
    {
        PhInitializeMonospaceFont(hwnd);
    }

    //if (TabControlHandle)
    //{
    //    SetWindowFont(TabControlHandle, PhApplicationFont, TRUE);
    //}
}

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

static NTSTATUS PhpOpenSecurityDummyHandle(
    _Inout_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return STATUS_SUCCESS;
}

VOID PhMwpOnCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_ESC_EXIT:
        {
            if (PhGetIntegerSetting(L"HideOnClose"))
            {
                if (PhNfIconsEnabled())
                    ShowWindow(WindowHandle, SW_HIDE);
            }
            else if (PhGetIntegerSetting(L"CloseOnEscape"))
            {
                ProcessHacker_Destroy();
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
    case ID_HACKER_SHOWDETAILSFORALLPROCESSES:
        {
            ProcessHacker_PrepareForEarlyShutdown();

            if (PhShellProcessHacker(
                WindowHandle,
                L"-v -newinstance",
                SW_SHOW,
                PH_SHELL_EXECUTE_ADMIN | PH_SHELL_EXECUTE_NOASYNC,
                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                0,
                NULL
                ))
            {
                ProcessHacker_Destroy();
            }
            else
            {
                ProcessHacker_CancelEarlyShutdown();
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

                    PhWriteStringAsUtf8FileStream(fileStream, &PhUnicodeByteOrderMark);
                    PhWritePhTextHeader(fileStream);

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
            PhShowFindObjectsDialog();
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
        ProcessHacker_Destroy();
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
        {
            PhMwpExecuteNotificationMenuCommand(WindowHandle, Id);
        }
        break;
    case ID_VIEW_HIDEPROCESSESFROMOTHERUSERS:
        {
            PhMwpToggleCurrentUserProcessTreeFilter();
        }
        break;
    case ID_VIEW_HIDESIGNEDPROCESSES:
        {
            PhMwpToggleSignedProcessTreeFilter();
        }
        break;
    case ID_VIEW_SCROLLTONEWPROCESSES:
        {
            PH_SET_INTEGER_CACHED_SETTING(ScrollToNewProcesses, !PhCsScrollToNewProcesses);
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
            PhSetIntegerSetting(L"MainWindowAlwaysOnTop", AlwaysOnTop);

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
            PhSetIntegerSetting(L"MainWindowOpacity", opacity);
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
            }

            PH_SET_INTEGER_CACHED_SETTING(UpdateInterval, interval);
            PhMwpApplyUpdateInterval(interval);
        }
        break;
    case ID_VIEW_UPDATEAUTOMATICALLY:
        {
            PhMwpUpdateAutomatically = !PhMwpUpdateAutomatically;
            PhMwpNotifyAllPages(MainTabPageUpdateAutomaticallyChanged, (PVOID)PhMwpUpdateAutomatically, NULL);
        }
        break;
    case ID_TOOLS_CREATESERVICE:
        {
            PhShowCreateServiceDialog(WindowHandle);
        }
        break;
    case ID_TOOLS_HIDDENPROCESSES:
        {
            PhShowHiddenProcessesDialog();
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
                    L"ProgramInspectExecutables",
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
            PPH_STRING systemDirectory;
            PPH_STRING taskmgrFileName;

            systemDirectory = PH_AUTO(PhGetSystemDirectory());
            taskmgrFileName = PH_AUTO(PhConcatStringRefZ(&systemDirectory->sr, L"\\taskmgr.exe"));

            if (!PhpIsDefaultTaskManager() && PhGetIntegerSetting(L"EnableShellExecuteSkipIfeoDebugger"))
            {
                PhShellExecuteEx(
                    WindowHandle,
                    PhGetString(taskmgrFileName),
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
                        PhSvcCallCreateProcessIgnoreIfeoDebugger(PhGetString(taskmgrFileName), NULL);
                        PhUiDisconnectFromPhSvc();
                    }
                }
                else
                {
                    PhCreateProcessIgnoreIfeoDebugger(PhGetString(taskmgrFileName), NULL);
                }
            }
        }
        break;
    case ID_TOOLS_STARTRESOURCEMONITOR:
        {
            PPH_STRING systemDirectory;
            PPH_STRING perfmonFileName;

            systemDirectory = PH_AUTO(PhGetSystemDirectory());
            perfmonFileName = PH_AUTO(PhConcatStringRefZ(&systemDirectory->sr, L"\\perfmon.exe"));

            if (PhGetIntegerSetting(L"EnableShellExecuteSkipIfeoDebugger"))
            {
                PhShellExecuteEx(
                    WindowHandle,
                    PhGetString(perfmonFileName),
                    L" /res",
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
    case ID_TOOLS_SCM_PERMISSIONS:
        {
            PhEditSecurity(
                NULL,
                L"Service Control Manager",
                L"SCManager",
                PhpOpenServiceControlManager,
                NULL,
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

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);

            if (PhUiTerminateProcesses(WindowHandle, processes, numberOfProcesses))
                PhDeselectAllProcessNodes();

            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
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

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);
            PhUiSuspendProcesses(WindowHandle, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
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

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);
            PhUiResumeProcesses(WindowHandle, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
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
                    ProcessHacker_PrepareForEarlyShutdown();

                    if (PhShellProcessHacker(
                        WindowHandle,
                        NULL,
                        SW_SHOW,
                        PH_SHELL_EXECUTE_NOZONECHECKS | PH_SHELL_EXECUTE_NOASYNC,
                        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                        0,
                        NULL
                        ))
                    {
                        ProcessHacker_Destroy();
                    }
                    else
                    {
                        ProcessHacker_CancelEarlyShutdown();
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
    case ID_PROCESS_CREATEDUMPFILE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiCreateDumpFileProcess(WindowHandle, processItem);
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
                PhShowProcessHeapsDialog(PhMainWndHandle, processItem);
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

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);
            PhUiReduceWorkingSetProcesses(WindowHandle, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
        }
        break;
    case ID_MISCELLANEOUS_RUNAS:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem && processItem->FileNameWin32)
            {
                PhSetStringSetting2(L"RunAsProgram", &processItem->FileNameWin32->sr);
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
    case ID_PRIORITY_REALTIME:
    case ID_PRIORITY_HIGH:
    case ID_PRIORITY_ABOVENORMAL:
    case ID_PRIORITY_NORMAL:
    case ID_PRIORITY_BELOWNORMAL:
    case ID_PRIORITY_IDLE:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);
            PhMwpExecuteProcessPriorityCommand(Id, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
        }
        break;
    case ID_IOPRIORITY_VERYLOW:
    case ID_IOPRIORITY_LOW:
    case ID_IOPRIORITY_NORMAL:
    case ID_IOPRIORITY_HIGH:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);
            PhMwpExecuteProcessIoPriorityCommand(Id, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
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
    case ID_WINDOW_BRINGTOFRONT:
        {
            if (IsWindow(PhMwpSelectedProcessWindowHandle))
            {
                WINDOWPLACEMENT placement = { sizeof(placement) };

                GetWindowPlacement(PhMwpSelectedProcessWindowHandle, &placement);

                if (placement.showCmd == SW_MINIMIZE)
                    ShowWindowAsync(PhMwpSelectedProcessWindowHandle, SW_RESTORE);
                else
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
                PostMessage(PhMwpSelectedProcessWindowHandle, WM_CLOSE, 0, 0);
            }
        }
        break;
    case ID_PROCESS_OPENFILELOCATION:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem &&
                !PhIsNullOrEmptyString(processItem->FileNameWin32) &&
                PhDoesFileExistWin32(PhGetString(processItem->FileNameWin32)
                ))
            {
                PhReferenceObject(processItem);
                PhShellExecuteUserString(
                    WindowHandle,
                    L"FileBrowseExecutable",
                    processItem->FileNameWin32->Buffer,
                    FALSE,
                    L"Make sure the Explorer executable file is present."
                    );
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_SEARCHONLINE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhSearchOnlineString(WindowHandle, processItem->ProcessName->Buffer);
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
            }
        }
        break;
    case ID_SERVICE_START:
        {
            PPH_SERVICE_ITEM* serviceItems;
            ULONG numberOfServiceItems;

            PhGetSelectedServiceItems(&serviceItems, &numberOfServiceItems);
            PhReferenceObjects(serviceItems, numberOfServiceItems);

            if (numberOfServiceItems == 1) // phsvc elevation (dmex)
                PhUiStartService(WindowHandle, serviceItems[0]);
            else
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

            if (numberOfServiceItems == 1) // phsvc elevation (dmex)
                PhUiContinueService(WindowHandle, serviceItems[0]);
            else
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

            if (numberOfServiceItems == 1) // phsvc elevation (dmex)
                PhUiPauseService(WindowHandle, serviceItems[0]);
            else
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

            if (numberOfServiceItems == 1) // phsvc elevation (dmex)
                PhUiStopService(WindowHandle, serviceItems[0]);
            else
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
            static PH_STRINGREF hklm = PH_STRINGREF_INIT(L"HKLM\\");

            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                HANDLE keyHandle;
                PPH_STRING serviceKeyName = PH_AUTO(PhGetServiceKeyName(&serviceItem->Name->sr));

                if (NT_SUCCESS(PhOpenKey(
                    &keyHandle,
                    KEY_READ,
                    PH_KEY_LOCAL_MACHINE,
                    &serviceKeyName->sr,
                    0
                    )))
                {
                    PPH_STRING hklmServiceKeyName;

                    hklmServiceKeyName = PH_AUTO(PhConcatStringRef2(&hklm, &serviceKeyName->sr));
                    PhShellOpenKey2(WindowHandle, hklmServiceKeyName);
                    NtClose(keyHandle);
                }
            }
        }
        break;
    case ID_SERVICE_OPENFILELOCATION:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();
            SC_HANDLE serviceHandle;

            if (serviceItem && (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG)))
            {
                PPH_STRING fileName;

                if (fileName = PhGetServiceHandleFileName(serviceHandle, &serviceItem->Name->sr))
                {
                    PhShellExecuteUserString(
                        WindowHandle,
                        L"FileBrowseExecutable",
                        fileName->Buffer,
                        FALSE,
                        L"Make sure the Explorer executable file is present."
                        );
                    PhDereferenceObject(fileName);
                }

                CloseServiceHandle(serviceHandle);
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
            }
        }
        break;
    case ID_NETWORK_GOTOSERVICE:
        {
            PPH_NETWORK_ITEM networkItem = PhGetSelectedNetworkItem();
            PPH_SERVICE_ITEM serviceItem;

            if (networkItem && networkItem->OwnerName)
            {
                if (serviceItem = PhReferenceServiceItem(networkItem->OwnerName->Buffer))
                {
                    PhMwpSelectPage(PhMwpServicesPage->Index);
                    SetFocus(PhMwpServiceTreeNewHandle);
                    ProcessHacker_SelectServiceItem(serviceItem);

                    PhDereferenceObject(serviceItem);
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
            if (PhGetIntegerSetting(L"HideOnClose") && PhNfIconsEnabled())
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

            if (PhGetIntegerSetting(L"HideOnMinimize") && PhNfIconsEnabled())
            {
                ShowWindow(WindowHandle, SW_HIDE);
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

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

VOID PhMwpOnInitMenuPopup(
    _In_ HWND WindowHandle,
    _In_ HMENU Menu,
    _In_ ULONG Index,
    _In_ BOOLEAN IsWindowMenu
    )
{
    ULONG i;
    BOOLEAN found;
    MENUINFO menuInfo;
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
    while (DeleteMenu(Menu, 0, MF_BYPOSITION))
        NOTHING;

    // Delete the previous EMENU for this submenu.
    if (SubMenuObjects[Index])
        PhDestroyEMenu(SubMenuObjects[Index]);

    // Make sure the menu style is set correctly.
    memset(&menuInfo, 0, sizeof(MENUINFO));
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_CHECKORBMP;

    if (PhEnableThemeSupport)
    {
        menuInfo.fMask |= MIM_BACKGROUND;
        menuInfo.hbrBack = PhThemeWindowBackgroundBrush;
    }

    SetMenuInfo(Menu, &menuInfo);

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

VOID PhMwpOnSize(
    _In_ HWND WindowHandle
    )
{
    if (!IsMinimized(WindowHandle))
    {
        HDWP deferHandle;

        deferHandle = BeginDeferWindowPos(2);
        PhMwpLayout(&deferHandle);
        EndDeferWindowPos(deferHandle);
    }
}

VOID PhMwpOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    )
{
    PhResizingMinimumSize(DragRectangle, Edge, 400, 175);
}

VOID PhMwpOnSetFocus(
    VOID
    )
{
    PPH_STRING windowTitle = PhGetMainWindowTitle();

    SetWindowText(PhMainWndHandle, PhGetString(windowTitle));

    if (windowTitle)
        PhDereferenceObject(windowTitle);

    if (CurrentPage->WindowHandle)
        SetFocus(CurrentPage->WindowHandle);
}

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

ULONG_PTR PhMwpOnUserMessage(
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
    case WM_PH_INVOKE:
        {
            VOID (NTAPI *function)(PVOID);

            function = (PVOID)LParam;
            function((PVOID)WParam);
        }
        break;
    }

    return 0;
}

VOID PhMwpLoadSettings(
    _In_ HWND WindowHandle
    )
{
    ULONG opacity;

    opacity = PhGetIntegerSetting(L"MainWindowOpacity");
    PhStatisticsSampleCount = PhGetIntegerSetting(L"SampleCount");
    PhEnablePurgeProcessRecords = !PhGetIntegerSetting(L"NoPurgeProcessRecords");
#ifdef _ARM64_
    PhEnableCycleCpuUsage = !!PhGetIntegerSetting(L"EnableArmCycleCpuUsage");
#else
    PhEnableCycleCpuUsage = !!PhGetIntegerSetting(L"EnableCycleCpuUsage");
#endif
    PhEnableServiceNonPoll = !!PhGetIntegerSetting(L"EnableServiceNonPoll");
    PhEnableNetworkBoundConnections = !!PhGetIntegerSetting(L"EnableNetworkBoundConnections");
    PhEnableNetworkProviderResolve = !!PhGetIntegerSetting(L"EnableNetworkResolve");
    PhEnableProcessQueryStage2 = !!PhGetIntegerSetting(L"EnableStage2");
    PhEnableServiceQueryStage2 = !!PhGetIntegerSetting(L"EnableServiceStage2");
    PhEnableTooltipSupport = !!PhGetIntegerSetting(L"EnableTooltipSupport");
    PhEnableImageCoherencySupport = !!PhGetIntegerSetting(L"EnableImageCoherencySupport");
    PhEnableLinuxSubsystemSupport = !!PhGetIntegerSetting(L"EnableLinuxSubsystemSupport");
    PhEnablePackageIconSupport = !!PhGetIntegerSetting(L"EnablePackageIconSupport");
    PhEnableSecurityAdvancedDialog = !!PhGetIntegerSetting(L"EnableSecurityAdvancedDialog");
    PhEnableProcessHandlePnPDeviceNameSupport = !!PhGetIntegerSetting(L"EnableProcessHandlePnPDeviceNameSupport");
    PhMwpNotifyIconNotifyMask = PhGetIntegerSetting(L"IconNotifyMask");

    if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
    {
        AlwaysOnTop = TRUE;
        SetWindowPos(WindowHandle, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
    }

    if (opacity != 0)
        PhSetWindowOpacity(WindowHandle, opacity);

    PhMwpInvokeUpdateWindowFont(NULL);

    if (PhGetIntegerSetting(L"EnableMonospaceFont"))
    {
        PhMwpInvokeUpdateWindowFontMonospace(WindowHandle, NULL);
    }

    PhNfLoadStage1();

    PhMwpNotifyAllPages(MainTabPageLoadSettings, NULL, NULL);
}

VOID PhMwpSaveSettings(
    _In_ HWND WindowHandle
    )
{
    PhMwpNotifyAllPages(MainTabPageSaveSettings, NULL, NULL);

    PhNfSaveSettings();
    PhSetIntegerSetting(L"IconNotifyMask", PhMwpNotifyIconNotifyMask);

    if (PhGetIntegerSetting(L"MainWindowTabRestoreEnabled"))
    {
        PhSetIntegerSetting(L"MainWindowTabRestoreIndex", TabCtrl_GetCurSel(TabControlHandle));
    }

    PhSaveWindowPlacementToSetting(L"MainWindowPosition", L"MainWindowSize", WindowHandle);
    PhMwpSaveWindowState(WindowHandle);

    if (!PhIsNullOrEmptyString(PhSettingsFileName))
        PhSaveSettings(&PhSettingsFileName->sr);
}

VOID PhMwpSaveWindowState(
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };

    GetWindowPlacement(WindowHandle, &placement);

    if (placement.showCmd == SW_NORMAL)
        PhSetIntegerSetting(L"MainWindowState", SW_NORMAL);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetIntegerSetting(L"MainWindowState", SW_MAXIMIZE);
}

VOID PhMwpUpdateLayoutPadding(
    VOID
    )
{
    PH_LAYOUT_PADDING_DATA data;

    memset(&data, 0, sizeof(PH_LAYOUT_PADDING_DATA));
    PhInvokeCallback(&LayoutPaddingCallback, &data);

    LayoutPadding = data.Padding;
}

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

    GetClientRect(PhMainWndHandle, &rect);
    PhMwpApplyLayoutPadding(&rect, &LayoutPadding);

    SetWindowPos(TabControlHandle, NULL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOACTIVATE | SWP_NOZORDER);
    UpdateWindow(TabControlHandle);

    PhMwpLayoutTabControl(DeferHandle);
}

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

BOOL PhMwpIsWindowOverlapped(
    _In_ HWND WindowHandle
    )
{
    RECT rectThisWindow = { 0 };
    RECT rectOtherWindow = { 0 };
    RECT rectIntersection = { 0 };
    HWND windowHandle = WindowHandle;

    if (!GetWindowRect(WindowHandle, &rectThisWindow))
        return FALSE;

    while ((windowHandle = GetWindow(windowHandle, GW_HWNDPREV)) && windowHandle != WindowHandle)
    {
        if (!(PhGetWindowStyle(windowHandle) & WS_VISIBLE))
            continue;

        if (!GetWindowRect(windowHandle, &rectOtherWindow))
            continue;

        if (!(PhGetWindowStyleEx(windowHandle) & WS_EX_TOPMOST) &&
            IntersectRect(&rectIntersection, &rectThisWindow, &rectOtherWindow))
        {
            return TRUE;
        }
    }

    return FALSE;
}

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
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTARTADVOPTIONS, L"Restart to advanced options", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTARTBOOTOPTIONS, L"Restart to boot options", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(PhGetOwnTokenAttributes().Elevated ? 0 : PH_EMENU_DISABLED, ID_COMPUTER_RESTARTFWOPTIONS, L"Restart to firmware options", NULL, NULL), ULONG_MAX);
    if (PhGetIntegerSetting(L"EnableShutdownBootMenu"))
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
    if (PhGetIntegerSetting(L"EnableShutdownCriticalMenu"))
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

PPH_EMENU PhpCreateHackerMenu(
    _In_ PPH_EMENU HackerMenu,
    _In_ BOOLEAN DelayLoadMenu
    )
{
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUN, L"&Run...\bCtrl+R", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUNAS, L"Run &as...\bCtrl+Shift+R", NULL, NULL), ULONG_MAX);
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

PPH_EMENU PhpCreateToolsMenu(
    _In_ PPH_EMENU ToolsMenu
    )
{
    PPH_EMENU_ITEM menuItem;

    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_CREATESERVICE, L"&Create service...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_LIVEDUMP, L"&Create live dump...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_INSPECTEXECUTABLEFILE, L"Inspect e&xecutable file...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_HIDDENPROCESSES, L"&Hidden processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_PAGEFILES, L"&Pagefiles", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_STARTTASKMANAGER, L"Start &Task Manager", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuItem(0, ID_TOOLS_STARTRESOURCEMONITOR, L"Start &Resource Monitor", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, PhCreateEMenuSeparator(), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"&Permissions", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_PWR_PERMISSIONS, L"Current Power Scheme", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_SCM_PERMISSIONS, L"Service Control Manager", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_RDP_PERMISSIONS, L"Terminal Server Listener", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_TOOLS_WMI_PERMISSIONS, L"WMI Root Namespace", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(ToolsMenu, menuItem, ULONG_MAX);

    return ToolsMenu;
}

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

    return UsersMenu;
}

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

PPH_EMENU PhpCreateMainMenu(
    _In_ ULONG SubMenuIndex
    )
{
    PPH_EMENU menu = PhCreateEMenu();

    switch (SubMenuIndex)
    {
    case PH_MENU_ITEM_LOCATION_HACKER:
        return PhpCreateHackerMenu(menu, FALSE);
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

            menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_HACKER, L"&System", NULL, NULL);
            // Insert an empty menuitem so we're able to delay load the submenu. (dmex)
            PhInsertEMenuItem(menuItem, PhCreateEMenuItemEmpty(), ULONG_MAX);
            PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
            //PhInsertEMenuItem(menu, PhpCreateHackerMenu(menuItem, TRUE), ULONG_MAX);

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

VOID PhMwpInitializeMainMenu(
    _In_ HMENU Menu
    )
{
    MENUINFO menuInfo;
    ULONG i;

    memset(&menuInfo, 0, sizeof(MENUINFO));
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_NOTIFYBYPOS; //| MNS_AUTODISMISS; Flag is unusable on Win10 - Github #547 (dmex).

    SetMenuInfo(Menu, &menuInfo);

    for (i = 0; i < RTL_NUMBER_OF(SubMenuHandles); i++)
    {
        SubMenuHandles[i] = GetSubMenu(Menu, i);
    }
}

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
            PhShowColumnSetEditorDialog(WindowHandle, L"ProcessTreeColumnSetConfig");
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
                PhSaveSettingsColumnSet(L"ProcessTreeColumnSetConfig", columnSetName, treeSettings, sortSettings);

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
            if (PhLoadSettingsColumnSet(L"ProcessTreeColumnSetConfig", columnSetName, &treeSettings, &sortSettings))
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
        PhMwpNotifyIconNotifyMask |= PH_NOTIFY_VALID_MASK;
        return TRUE;
    case ID_NOTIFICATIONS_DISABLEALL:
        PhMwpNotifyIconNotifyMask &= ~PH_NOTIFY_VALID_MASK;
        return TRUE;
    case ID_NOTIFICATIONS_NEWPROCESSES:
    case ID_NOTIFICATIONS_TERMINATEDPROCESSES:
    case ID_NOTIFICATIONS_NEWSERVICES:
    case ID_NOTIFICATIONS_STARTEDSERVICES:
    case ID_NOTIFICATIONS_STOPPEDSERVICES:
    case ID_NOTIFICATIONS_DELETEDSERVICES:
    case ID_NOTIFICATIONS_MODIFIEDSERVICES:
        {
            ULONG bit;

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
            }

            PhMwpNotifyIconNotifyMask ^= bit;
        }
        return TRUE;
    }

    return FALSE;
}

PPH_EMENU PhpCreateIconMenu(
    VOID
    )
{
    PPH_EMENU menu;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ICON_SHOWHIDEPROCESSHACKER, L"&Show/Hide System Informer", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ICON_SYSTEMINFORMATION, L"System &information", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhpCreateNotificationMenu(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESSES_DUMMY, L"&Processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhpCreateComputerMenu(FALSE), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ICON_EXIT, L"E&xit", NULL, NULL), ULONG_MAX);

    return menu;
}

VOID PhMwpInitializeSubMenu(
    _In_ HWND hwnd,
    _In_ PPH_EMENU Menu,
    _In_ ULONG Index
    )
{
    PPH_EMENU_ITEM menuItem;

    if (Index == PH_MENU_ITEM_LOCATION_HACKER) // Hacker
    {
        // Fix some menu items.
        if (PhGetOwnTokenAttributes().Elevated)
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_RUNASADMINISTRATOR))
                PhDestroyEMenuItem(menuItem);
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_SHOWDETAILSFORALLPROCESSES))
                PhDestroyEMenuItem(menuItem);
        }
        else
        {
            HBITMAP shieldBitmap;

            if (shieldBitmap = PhGetShieldBitmap(LayoutWindowDpi))
            {
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_SHOWDETAILSFORALLPROCESSES))
                    menuItem->Bitmap = shieldBitmap;
            }
        }

        // Fix up the Computer menu.
        PhMwpSetupComputerMenu(Menu);
    }
    else if (Index == PH_MENU_ITEM_LOCATION_VIEW) // View
    {
        PPH_EMENU_ITEM trayIconsMenuItem;
        PPH_EMENU_ITEM menuItem;
        ULONG id = ULONG_MAX;
        ULONG placeholderIndex = ULONG_MAX;

        if (trayIconsMenuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, NULL, ID_VIEW_TRAYICONS))
        {
            // Add menu items for the registered tray icons.

            PhInsertEMenuItem(trayIconsMenuItem, PhpCreateNotificationMenu(), ULONG_MAX);
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

        id = PH_OPACITY_TO_ID(PhGetIntegerSetting(L"MainWindowOpacity"));

        if (menuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, NULL, id))
            menuItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;

        switch (PhGetIntegerSetting(L"UpdateInterval"))
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
    else if (Index == PH_MENU_ITEM_LOCATION_TOOLS) // Tools
    {
        if (WindowsVersion < WINDOWS_8_1)
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_LIVEDUMP))
                PhDestroyEMenuItem(menuItem);
        }

        if (!PhGetIntegerSetting(L"HiddenProcessesMenuEnabled"))
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_HIDDENPROCESSES))
                PhDestroyEMenuItem(menuItem);
        }

        if (PhGetIntegerSetting(L"EnableBitmapSupport"))
        {
            HBITMAP shieldBitmap;

            if (shieldBitmap = PhGetShieldBitmap(LayoutWindowDpi))
            {
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_STARTTASKMANAGER))
                    menuItem->Bitmap = shieldBitmap;
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_STARTRESOURCEMONITOR))
                    menuItem->Bitmap = shieldBitmap;
            }
        }
    }
}

VOID PhMwpInitializeSectionMenuItems(
    _In_ PPH_EMENU Menu,
    _In_ ULONG StartIndex
    )
{
    if (CurrentPage)
    {
        PH_MAIN_TAB_PAGE_MENU_INFORMATION menuInfo;

        menuInfo.Menu = Menu;
        menuInfo.StartIndex = StartIndex;

        if (!CurrentPage->Callback(CurrentPage, MainTabPageInitializeSectionMenuItems, &menuInfo, NULL))
        {
            // Remove the extra separator.
            PhRemoveEMenuItem(Menu, NULL, StartIndex);
        }
    }
}

VOID PhMwpLayoutTabControl(
    _Inout_ HDWP *DeferHandle
    )
{
    RECT rect;
    RECT tabrect;

    if (!LayoutPaddingValid)
    {
        PhMwpUpdateLayoutPadding();
        LayoutPaddingValid = TRUE;
    }

    GetClientRect(PhMainWndHandle, &rect);
    PhMwpApplyLayoutPadding(&rect, &LayoutPadding);
    tabrect = rect;
    TabCtrl_AdjustRect(TabControlHandle, FALSE, &tabrect);

    if (CurrentPage && CurrentPage->WindowHandle)
    {
        // Remove the tabctrl padding (dmex)
        *DeferHandle = DeferWindowPos(
            *DeferHandle,
            CurrentPage->WindowHandle,
            NULL,
            rect.left,
            tabrect.top - LayoutBorderSize,
            rect.right - rect.left,
            (tabrect.bottom - tabrect.top) + (rect.bottom - tabrect.bottom),
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

#pragma warning(push)
#pragma warning(disable:26454) // The TCN_SEL definitions are negative unsigned (disable useless warning) (dmex)
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
#pragma warning(pop)

VOID PhMwpSelectionChangedTabControl(
    _In_ ULONG OldIndex
    )
{
    ULONG selectedIndex;
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
                if (page->Callback(page, MainTabPageCreateWindow, &page->WindowHandle, NULL))
                    page->CreateWindowCalled = TRUE;

                if (page->WindowHandle)
                    BringWindowToTop(page->WindowHandle);
                if (PhTreeWindowFont)
                    page->Callback(page, MainTabPageFontChanged, PhTreeWindowFont, NULL);
            }

            page->Callback(page, MainTabPageSelected, (PVOID)TRUE, NULL);

            if (page->WindowHandle)
            {
                deferHandle = DeferWindowPos(deferHandle, page->WindowHandle, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY);
                SetFocus(page->WindowHandle);
            }
        }
        else if (page->Index == OldIndex)
        {
            page->Callback(page, MainTabPageSelected, (PVOID)FALSE, NULL);

            if (page->WindowHandle)
            {
                deferHandle = DeferWindowPos(deferHandle, page->WindowHandle, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY);
            }
        }
    }

    PhMwpLayoutTabControl(&deferHandle);

    EndDeferWindowPos(deferHandle);

    if (PhPluginsEnabled)
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowTabChanged), IntToPtr(selectedIndex));
}

PPH_MAIN_TAB_PAGE PhMwpCreatePage(
    _In_ PPH_MAIN_TAB_PAGE Template
    )
{
    PPH_MAIN_TAB_PAGE page;
    PPH_STRING name;
    HDWP deferHandle;

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
    deferHandle = BeginDeferWindowPos(1);
    PhMwpLayoutTabControl(&deferHandle);
    EndDeferWindowPos(deferHandle);

    return page;
}

VOID PhMwpSelectPage(
    _In_ ULONG Index
    )
{
    INT oldIndex;

    oldIndex = TabCtrl_GetCurSel(TabControlHandle);
    TabCtrl_SetCurSel(TabControlHandle, Index);
    PhMwpSelectionChangedTabControl(oldIndex);
}

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

PPH_MAIN_TAB_PAGE PhMwpCreateInternalPage(
    _In_ PWSTR Name,
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
    _Inout_ struct _PH_EMENU_ITEM *Menu,
    _In_ HANDLE ProcessId
    )
{
    PPH_EMENU_ITEM priorityMenu;
    PPH_EMENU_ITEM ioPriorityMenu = NULL;
    PPH_PROCESS_ITEM processItem;
    BOOLEAN isSuspended = FALSE;
    BOOLEAN isPartiallySuspended = TRUE;

    // Priority

    priorityMenu = PhCreateEMenuItem(0, ID_PROCESS_PRIORITY, L"&Priority", NULL, ProcessId);

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
    if (isPartiallySuspended)
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_RESUME, L"Res&ume", NULL, ProcessId), ULONG_MAX);

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_RESTART, L"Res&tart", NULL, ProcessId), ULONG_MAX);

    PhInsertEMenuItem(Menu, priorityMenu, ULONG_MAX);

    if (ioPriorityMenu)
        PhInsertEMenuItem(Menu, ioPriorityMenu, ULONG_MAX);

    PhMwpSetProcessMenuPriorityChecks(Menu, ProcessId, TRUE, TRUE, FALSE);

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_PROPERTIES, L"P&roperties", NULL, ProcessId), ULONG_MAX);
}

BOOLEAN PhHandleMiniProcessMenuItem(
    _Inout_ struct _PH_EMENU_ITEM *MenuItem
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
                    ProcessHacker_ShowProcessProperties(processItem);
                    break;
                }

                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowError(PhMainWndHandle, L"%s", L"The process does not exist.");
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
                PhMwpExecuteProcessPriorityCommand(MenuItem->Id, &processItem, 1);
                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowError(PhMainWndHandle, L"%s", L"The process does not exist.");
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
                PhMwpExecuteProcessIoPriorityCommand(MenuItem->Id, &processItem, 1);
                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowError(PhMainWndHandle, L"%s", L"The process does not exist.");
            }
        }
        break;
    }

    return FALSE;
}

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

VOID PhShowIconContextMenu(
    _In_ POINT Location
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

    numberOfProcesses = PhGetIntegerSetting(L"IconProcesses");
    item = PhFindEMenuItem(menu, 0, 0, ID_PROCESSES_DUMMY);

    if (item)
        PhMwpAddIconProcesses(item, numberOfProcesses);

    // Fix up the Computer menu.
    PhMwpSetupComputerMenu(menu);

    // Give plugins a chance to modify the menu.

    if (PhPluginsEnabled)
    {
        PhPluginInitializeMenuInfo(&menuInfo, menu, PhMainWndHandle, 0);
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackIconMenuInitializing), &menuInfo);
    }

    SetForegroundWindow(PhMainWndHandle); // window must be foregrounded so menu will disappear properly
    item = PhShowEMenu(
        menu,
        PhMainWndHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        Location.x,
        Location.y
        );

    if (item)
    {
        BOOLEAN handled = FALSE;

        if (PhPluginsEnabled && !handled)
            handled = PhPluginTriggerEMenuItem(&menuInfo, item);

        if (!handled)
            handled = PhHandleMiniProcessMenuItem(item);

        if (!handled)
            handled = PhMwpExecuteComputerCommand(PhMainWndHandle, item->Id);

        if (!handled)
            handled = PhMwpExecuteNotificationMenuCommand(PhMainWndHandle, item->Id);

        if (!handled)
        {
            switch (item->Id)
            {
            case ID_ICON_SHOWHIDEPROCESSHACKER:
                ProcessHacker_ToggleVisible(FALSE);
                break;
            case ID_ICON_SYSTEMINFORMATION:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_SYSTEMINFORMATION, 0);
                break;
            case ID_ICON_EXIT:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_HACKER_EXIT, 0);
                break;
            }
        }
    }

    PhDestroyEMenu(menu);
}

VOID PhShowIconNotification(
    _In_ PWSTR Title,
    _In_ PWSTR Text
    )
{
    PhNfShowBalloonTip(Title, Text, 10);
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
                ProcessHacker_SelectTabPage(PhMwpProcessesPage->Index);
                ProcessHacker_SelectProcessNode(processNode);
                ProcessHacker_ToggleVisible(TRUE);
            }
        }
        break;
    case PH_NOTIFY_SERVICE_CREATE:
    case PH_NOTIFY_SERVICE_START:
    case PH_NOTIFY_SERVICE_STOP:
        {
            PPH_SERVICE_ITEM serviceItem;

            if (PhMwpLastNotificationDetails.ServiceName &&
                (serviceItem = PhReferenceServiceItem(PhMwpLastNotificationDetails.ServiceName->Buffer)))
            {
                ProcessHacker_SelectTabPage(PhMwpServicesPage->Index);
                ProcessHacker_SelectServiceItem(serviceItem);
                ProcessHacker_ToggleVisible(TRUE);

                PhDereferenceObject(serviceItem);
            }
        }
        break;
    }
}

VOID PhMwpClearLastNotificationDetails(
    VOID
    )
{
    if (PhMwpLastNotificationType &
        (PH_NOTIFY_SERVICE_CREATE | PH_NOTIFY_SERVICE_DELETE | PH_NOTIFY_SERVICE_START | PH_NOTIFY_SERVICE_STOP))
    {
        PhClearReference(&PhMwpLastNotificationDetails.ServiceName);
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

    fontHexString = PhaGetStringSetting(L"Font");

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
    SendMessage(PhMainWndHandle, WM_PH_UPDATE_FONT, 0, 0); // notify plugins of font change. (dmex)

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

    fontHexString = PhaGetStringSetting(L"FontMonospace");

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
        PhInitializeMonospaceFont(hwnd);
        return;
    }

    PhMonospaceFont = newFont;

    if (oldFont) DeleteFont(oldFont);
}

VOID PhMwpInvokePrepareEarlyExit(
    _In_ PVOID Parameter
    )
{
    PhMwpSaveSettings(PhMainWndHandle);
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

// Exports for plugin support (dmex)
HWND PhGetMainWindowHandle(
    VOID
    )
{
    return PhMainWndHandle;
}

ULONG PhGetWindowsVersion(
    VOID
    )
{
    return WindowsVersion;
}

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
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)PhMainWndHandle, (LPARAM)DestroyWindow);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SHOW_PROPERTIES:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)lparam, (LPARAM)PhMwpShowProcessProperties);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SAVE_ALL_SETTINGS:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)PhMainWndHandle, (LPARAM)PhMwpSaveSettings);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_PREPARE_FOR_EARLY_SHUTDOWN:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, 0, (LPARAM)PhMwpInvokePrepareEarlyExit);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_CANCEL_EARLY_SHUTDOWN:
        {
            PhMainWndEarlyExit = FALSE;
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_TOGGLE_VISIBLE:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)!(BOOLEAN)wparam, (LPARAM)PhMwpInvokeActivateWindow);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_ICON_CLICK:
        {
            BOOLEAN visibility = !!PhGetIntegerSetting(L"IconTogglesVisibility");

            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)visibility, (LPARAM)PhMwpInvokeActivateWindow);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_EDITOR:
        {
            PPH_SHOW_MEMORY_EDITOR showMemoryEditor = (PPH_SHOW_MEMORY_EDITOR)lparam;

            PostMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)showMemoryEditor, (LPARAM)PhMwpInvokeShowMemoryEditorDialog);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_RESULTS:
        {
            PPH_SHOW_MEMORY_RESULTS showMemoryResults = (PPH_SHOW_MEMORY_RESULTS)lparam;

            PostMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)showMemoryResults, (LPARAM)PhMwpInvokeShowMemoryResultsDialog);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_TAB_PAGE:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)wparam, (LPARAM)PhMwpInvokeSelectTabPage);
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
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)lparam, (LPARAM)PhSelectAndEnsureVisibleProcessNode);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_SERVICE_ITEM:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)lparam, (LPARAM)PhMwpInvokeSelectServiceItem);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SELECT_NETWORK_ITEM:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)lparam, (LPARAM)PhMwpInvokeSelectNetworkItem);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_UPDATE_FONT:
        {
            SendMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)lparam, (LPARAM)PhMwpInvokeUpdateWindowFont);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_GET_FONT:
        {
            return PhTreeWindowFont; // (PVOID)GetWindowFont(PhMwpProcessTreeNewHandle);
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_INVOKE:
        {
            PostMessage(PhMainWndHandle, WM_PH_INVOKE, (WPARAM)wparam, (LPARAM)lparam);
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
            return (PVOID)PhMwpUpdateAutomatically;
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_SET_UPDATE_AUTOMATICALLY:
        {
            if (!!((BOOLEAN)wparam) != PhMwpUpdateAutomatically)
            {
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_UPDATEAUTOMATICALLY, 0);
            }
        }
        break;
    case PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_PROCEDURE:
        {
            return (PVOID)PhMwpWndProc; // (WNDPROC)GetWindowLongPtr(PhMainWndHandle, GWLP_WNDPROC);
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
    }

    return NULL;
}

PVOID PhPluginCreateTabPage(
    _In_ PVOID Page
    )
{
    return PhMwpCreatePage(Page);
}

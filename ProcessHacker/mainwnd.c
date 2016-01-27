/*
 * Process Hacker -
 *   main window
 *
 * Copyright (C) 2009-2015 wj32
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

#define PH_MAINWND_PRIVATE
#include <phapp.h>
#include <kphuser.h>
#include <settings.h>
#include <emenu.h>
#include <verify.h>
#include <phsvccl.h>
#include <phplug.h>
#include <cpysave.h>
#include <notifico.h>
#include <memsrch.h>
#include <symprv.h>
#include <sysinfo.h>
#include <miniinfo.h>
#include <mainwndp.h>
#include <windowsx.h>
#include <shlobj.h>
#include <winsta.h>
#include <iphlpapi.h>

#define RUNAS_MODE_ADMIN 1
#define RUNAS_MODE_LIMITED 2

typedef HRESULT (WINAPI *_LoadIconMetric)(
    _In_ HINSTANCE hinst,
    _In_ PCWSTR pszName,
    _In_ int lims,
    _Out_ HICON *phico
    );

PHAPPAPI HWND PhMainWndHandle;
BOOLEAN PhMainWndExiting = FALSE;
HMENU PhMainWndMenuHandle;

static BOOLEAN NeedsMaximize = FALSE;
static ULONG NeedsSelectPid = 0;
static BOOLEAN AlwaysOnTop = FALSE;

static BOOLEAN DelayedLoadCompleted = FALSE;
static ULONG NotifyIconNotifyMask;

static PH_CALLBACK_DECLARE(LayoutPaddingCallback);
static RECT LayoutPadding = { 0, 0, 0, 0 };
static BOOLEAN LayoutPaddingValid = TRUE;
static HWND TabControlHandle;
static INT ProcessesTabIndex;
static INT ServicesTabIndex;
static INT NetworkTabIndex;
static INT MaxTabIndex;
static INT OldTabIndex;
static PPH_LIST AdditionalTabPageList = NULL;
static HWND ProcessTreeListHandle;
static HWND ServiceTreeListHandle;
static HWND NetworkTreeListHandle;
static HFONT CurrentCustomFont;

static BOOLEAN NetworkFirstTime = TRUE;
static BOOLEAN ServiceTreeListLoaded = FALSE;
static BOOLEAN NetworkTreeListLoaded = FALSE;
static HMENU SubMenuHandles[5];
static PPH_EMENU SubMenuObjects[5];
static PPH_LIST LegacyAddMenuItemList;
static BOOLEAN UsersMenuInitialized = FALSE;
static BOOLEAN UpdateAutomatically = TRUE;

static PH_CALLBACK_REGISTRATION SymInitRegistration;

static PH_PROVIDER_REGISTRATION ProcessProviderRegistration;
static PH_CALLBACK_REGISTRATION ProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION ProcessModifiedRegistration;
static PH_CALLBACK_REGISTRATION ProcessRemovedRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
static BOOLEAN ProcessesNeedsRedraw = FALSE;
static PPH_PROCESS_NODE ProcessToScrollTo = NULL;

static PH_PROVIDER_REGISTRATION ServiceProviderRegistration;
static PH_CALLBACK_REGISTRATION ServiceAddedRegistration;
static PH_CALLBACK_REGISTRATION ServiceModifiedRegistration;
static PH_CALLBACK_REGISTRATION ServiceRemovedRegistration;
static PH_CALLBACK_REGISTRATION ServicesUpdatedRegistration;
static PPH_POINTER_LIST ServicesPendingList;
static BOOLEAN ServicesNeedsRedraw = FALSE;

static PH_PROVIDER_REGISTRATION NetworkProviderRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemAddedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemsUpdatedRegistration;
static BOOLEAN NetworkNeedsRedraw = FALSE;

static ULONG SelectedRunAsMode;
static HWND SelectedProcessWindowHandle;
static BOOLEAN SelectedProcessVirtualizationEnabled;
static ULONG SelectedUserSessionId;

static PPH_TN_FILTER_ENTRY CurrentUserFilterEntry = NULL;
static PPH_TN_FILTER_ENTRY SignedFilterEntry = NULL;
//static PPH_TN_FILTER_ENTRY CurrentUserNetworkFilterEntry = NULL;
//static PPH_TN_FILTER_ENTRY SignedNetworkFilterEntry = NULL;
static PPH_TN_FILTER_ENTRY DriverFilterEntry = NULL;

static ULONG LastNotificationType;
static union
{
    HANDLE ProcessId;
    PPH_STRING ServiceName;
} LastNotificationDetails;

BOOLEAN PhMainWndInitialization(
    _In_ INT ShowCommand
    )
{
    PH_STRING_BUILDER stringBuilder;
    PH_RECTANGLE windowRectangle;

    if (PhGetIntegerSetting(L"FirstRun"))
    {
        PPH_STRING autoDbghelpPath;

        // Try to set up the dbghelp path automatically if this is the first run.

        autoDbghelpPath = PhMwpFindDbghelpPath();

        if (autoDbghelpPath)
        {
            PhSetStringSetting2(L"DbgHelpPath", &autoDbghelpPath->sr);
            PhDereferenceObject(autoDbghelpPath);
        }

        PhSetIntegerSetting(L"FirstRun", FALSE);
    }

    // This was added to be able to delay-load dbghelp.dll and symsrv.dll.
    PhRegisterCallback(&PhSymInitCallback, PhMwpSymInitHandler, NULL, &SymInitRegistration);

    PhMwpInitializeProviders();

    if (!PhMwpInitializeWindowClass())
        return FALSE;

    windowRectangle.Position = PhGetIntegerPairSetting(L"MainWindowPosition");
    windowRectangle.Size = PhGetIntegerPairSetting(L"MainWindowSize");

    // Create the window title.

    PhInitializeStringBuilder(&stringBuilder, 100);
    PhAppendStringBuilder2(&stringBuilder, L"Process Hacker");

    if (PhCurrentUserName)
    {
        PhAppendStringBuilder2(&stringBuilder, L" [");
        PhAppendStringBuilder(&stringBuilder, &PhCurrentUserName->sr);
        PhAppendCharStringBuilder(&stringBuilder, ']');
        if (KphIsConnected()) PhAppendCharStringBuilder(&stringBuilder, '+');
    }

    if (WINDOWS_HAS_UAC && PhElevationType == TokenElevationTypeFull)
        PhAppendStringBuilder2(&stringBuilder, L" (Administrator)");

    // Create the window.

    PhMainWndHandle = CreateWindow(
        PH_MAINWND_CLASSNAME,
        stringBuilder.String->Buffer,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        windowRectangle.Left,
        windowRectangle.Top,
        windowRectangle.Width,
        windowRectangle.Height,
        NULL,
        NULL,
        PhInstanceHandle,
        NULL
        );
    PhDeleteStringBuilder(&stringBuilder);
    PhMainWndMenuHandle = GetMenu(PhMainWndHandle);

    if (!PhMainWndHandle)
        return FALSE;

    PhMwpInitializeMainMenu(PhMainWndMenuHandle);

    // Choose a more appropriate rectangle for the window.
    PhAdjustRectangleToWorkingArea(
        PhMainWndHandle,
        &windowRectangle
        );
    MoveWindow(PhMainWndHandle, windowRectangle.Left, windowRectangle.Top,
        windowRectangle.Width, windowRectangle.Height, FALSE);

    // Allow WM_PH_ACTIVATE to pass through UIPI.
    if (WINDOWS_HAS_UAC)
        ChangeWindowMessageFilter_I(WM_PH_ACTIVATE, MSGFLT_ADD);

    PhMwpOnSettingChange();

    // Initialize child controls.
    PhMwpInitializeControls();

    PhMwpLoadSettings();
    PhLogInitialization();
    PhQueueItemGlobalWorkQueue(PhMwpDelayedLoadFunction, NULL);

    PhMwpSelectionChangedTabControl(-1);

    // Perform a layout.
    PhMwpOnSize();

    PhStartProviderThread(&PhPrimaryProviderThread);
    PhStartProviderThread(&PhSecondaryProviderThread);

    // See PhMwpOnTimer for more details.
    if (PhCsUpdateInterval > PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_1)
        SetTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA, PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_1, NULL);

    UpdateWindow(PhMainWndHandle);

    if ((PhStartupParameters.ShowHidden || PhGetIntegerSetting(L"StartHidden")) && PhNfTestIconMask(PH_ICON_ALL))
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

    NeedsSelectPid = PhStartupParameters.SelectPid;

    if (PhPluginsEnabled)
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowShowing), IntToPtr(ShowCommand));

    if (PhStartupParameters.SelectTab)
    {
        INT tabIndex = PhMwpFindTabPageIndex(PhStartupParameters.SelectTab->Buffer);

        if (tabIndex != -1)
            PhMwpSelectTabPage(tabIndex);
    }

    if (ShowCommand != SW_HIDE)
        ShowWindow(PhMainWndHandle, ShowCommand);

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
    case WM_DESTROY:
        {
            PhMwpOnDestroy();
        }
        break;
    case WM_ENDSESSION:
        {
            PhMwpOnEndSession();
        }
        break;
    case WM_SETTINGCHANGE:
        {
            PhMwpOnSettingChange();
        }
        break;
    case WM_COMMAND:
        {
            PhMwpOnCommand(LOWORD(wParam));
        }
        break;
    case WM_SHOWWINDOW:
        {
            PhMwpOnShowWindow(!!wParam, (ULONG)lParam);
        }
        break;
    case WM_SYSCOMMAND:
        {
            if (PhMwpOnSysCommand((ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
        }
        break;
    case WM_MENUCOMMAND:
        {
            PhMwpOnMenuCommand((ULONG)wParam, (HMENU)lParam);
        }
        break;
    case WM_INITMENUPOPUP:
        {
            PhMwpOnInitMenuPopup((HMENU)wParam, LOWORD(lParam), !!HIWORD(lParam));
        }
        break;
    case WM_SIZE:
        {
            PhMwpOnSize();
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
    case WM_TIMER:
        {
            PhMwpOnTimer((ULONG)wParam);
        }
        break;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhMwpOnNotify((NMHDR *)lParam, &result))
                return result;
        }
        break;
    case WM_WTSSESSION_CHANGE:
        {
            PhMwpOnWtsSessionChange((ULONG)wParam, (ULONG)lParam);
        }
        break;
    }

    if (uMsg >= WM_PH_FIRST && uMsg <= WM_PH_LAST)
    {
        return PhMwpOnUserMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOLEAN PhMwpInitializeWindowClass(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc = PhMwpWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = PhInstanceHandle;
    wcex.hIcon = LoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAINWND);
    wcex.lpszClassName = PH_MAINWND_CLASSNAME;
    wcex.hIconSm = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER), IMAGE_ICON, 16, 16, 0);

    if (!RegisterClassEx(&wcex))
        return FALSE;

    return TRUE;
}

VOID PhMwpInitializeProviders(
    VOID
    )
{
    ULONG interval;

    interval = PhGetIntegerSetting(L"UpdateInterval");

    if (interval == 0)
    {
        interval = 1000;
        PH_SET_INTEGER_CACHED_SETTING(UpdateInterval, interval);
    }

    PhInitializeProviderThread(&PhPrimaryProviderThread, interval);
    PhInitializeProviderThread(&PhSecondaryProviderThread, interval);

    PhRegisterProvider(&PhPrimaryProviderThread, PhProcessProviderUpdate, NULL, &ProcessProviderRegistration);
    PhSetEnabledProvider(&ProcessProviderRegistration, TRUE);
    PhRegisterProvider(&PhPrimaryProviderThread, PhServiceProviderUpdate, NULL, &ServiceProviderRegistration);
    PhSetEnabledProvider(&ServiceProviderRegistration, TRUE);
    PhRegisterProvider(&PhPrimaryProviderThread, PhNetworkProviderUpdate, NULL, &NetworkProviderRegistration);
}

VOID PhMwpApplyUpdateInterval(
    _In_ ULONG Interval
    )
{
    PhSetIntervalProviderThread(&PhPrimaryProviderThread, Interval);
    PhSetIntervalProviderThread(&PhSecondaryProviderThread, Interval);

    if (Interval > PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM)
        SetTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA, PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM, NULL);
    else
        KillTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA); // Might not exist
}

VOID PhMwpInitializeControls(
    VOID
    )
{
    ULONG thinRows;

    TabControlHandle = CreateWindow(
        WC_TABCONTROL,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_MULTILINE,
        0,
        0,
        3,
        3,
        PhMainWndHandle,
        NULL,
        PhInstanceHandle,
        NULL
        );
    SendMessage(TabControlHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);
    BringWindowToTop(TabControlHandle);
    ProcessesTabIndex = PhAddTabControlTab(TabControlHandle, 0, L"Processes");
    ServicesTabIndex = PhAddTabControlTab(TabControlHandle, 1, L"Services");
    NetworkTabIndex = PhAddTabControlTab(TabControlHandle, 2, L"Network");
    MaxTabIndex = NetworkTabIndex;

    thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;

    ProcessTreeListHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | TN_STYLE_ANIMATE_DIVIDER | thinRows,
        0,
        0,
        3,
        3,
        PhMainWndHandle,
        (HMENU)ID_MAINWND_PROCESSTL,
        PhLibImageBase,
        NULL
        );
    BringWindowToTop(ProcessTreeListHandle);

    ServiceTreeListHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows,
        0,
        0,
        3,
        3,
        PhMainWndHandle,
        (HMENU)ID_MAINWND_SERVICETL,
        PhLibImageBase,
        NULL
        );
    BringWindowToTop(ServiceTreeListHandle);

    NetworkTreeListHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows,
        0,
        0,
        3,
        3,
        PhMainWndHandle,
        (HMENU)ID_MAINWND_NETWORKTL,
        PhLibImageBase,
        NULL
        );
    BringWindowToTop(NetworkTreeListHandle);

    PhProcessTreeListInitialization();
    PhInitializeProcessTreeList(ProcessTreeListHandle);

    PhServiceTreeListInitialization();
    PhInitializeServiceTreeList(ServiceTreeListHandle);

    PhNetworkTreeListInitialization();
    PhInitializeNetworkTreeList(NetworkTreeListHandle);

    PhRegisterCallback(
        &PhProcessAddedEvent,
        PhMwpProcessAddedHandler,
        NULL,
        &ProcessAddedRegistration
        );
    PhRegisterCallback(
        &PhProcessModifiedEvent,
        PhMwpProcessModifiedHandler,
        NULL,
        &ProcessModifiedRegistration
        );
    PhRegisterCallback(
        &PhProcessRemovedEvent,
        PhMwpProcessRemovedHandler,
        NULL,
        &ProcessRemovedRegistration
        );
    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        PhMwpProcessesUpdatedHandler,
        NULL,
        &ProcessesUpdatedRegistration
        );

    PhRegisterCallback(
        &PhServiceAddedEvent,
        PhMwpServiceAddedHandler,
        NULL,
        &ServiceAddedRegistration
        );
    PhRegisterCallback(
        &PhServiceModifiedEvent,
        PhMwpServiceModifiedHandler,
        NULL,
        &ServiceModifiedRegistration
        );
    PhRegisterCallback(
        &PhServiceRemovedEvent,
        PhMwpServiceRemovedHandler,
        NULL,
        &ServiceRemovedRegistration
        );
    PhRegisterCallback(
        &PhServicesUpdatedEvent,
        PhMwpServicesUpdatedHandler,
        NULL,
        &ServicesUpdatedRegistration
        );

    PhRegisterCallback(
        &PhNetworkItemAddedEvent,
        PhMwpNetworkItemAddedHandler,
        NULL,
        &NetworkItemAddedRegistration
        );
    PhRegisterCallback(
        &PhNetworkItemModifiedEvent,
        PhMwpNetworkItemModifiedHandler,
        NULL,
        &NetworkItemModifiedRegistration
        );
    PhRegisterCallback(
        &PhNetworkItemRemovedEvent,
        PhMwpNetworkItemRemovedHandler,
        NULL,
        &NetworkItemRemovedRegistration
        );
    PhRegisterCallback(
        &PhNetworkItemsUpdatedEvent,
        PhMwpNetworkItemsUpdatedHandler,
        NULL,
        &NetworkItemsUpdatedRegistration
        );
}

NTSTATUS PhMwpDelayedLoadFunction(
    _In_ PVOID Parameter
    )
{
    // Register for window station notifications.
    WinStationRegisterConsoleNotification(NULL, PhMainWndHandle, WNOTIFY_ALL_SESSIONS);

    PhNfLoadStage2();

    // Make sure we get closed late in the shutdown process.
    SetProcessShutdownParameters(0x100, 0);

    DelayedLoadCompleted = TRUE;
    //PostMessage(PhMainWndHandle, WM_PH_DELAYED_LOAD_COMPLETED, 0, 0);

    return STATUS_SUCCESS;
}

PPH_STRING PhMwpFindDbghelpPath(
    VOID
    )
{
    static struct
    {
        ULONG Folder;
        PWSTR AppendPath;
    } locations[] =
    {
#ifdef _WIN64
        { CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\10\\Debuggers\\x64\\dbghelp.dll" },
        { CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\8.1\\Debuggers\\x64\\dbghelp.dll" },
        { CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\8.0\\Debuggers\\x64\\dbghelp.dll" },
        { CSIDL_PROGRAM_FILES, L"\\Debugging Tools for Windows (x64)\\dbghelp.dll" }
#else
        { CSIDL_PROGRAM_FILES, L"\\Windows Kits\\10\\Debuggers\\x86\\dbghelp.dll" },
        { CSIDL_PROGRAM_FILES, L"\\Windows Kits\\8.1\\Debuggers\\x86\\dbghelp.dll" },
        { CSIDL_PROGRAM_FILES, L"\\Windows Kits\\8.0\\Debuggers\\x86\\dbghelp.dll" },
        { CSIDL_PROGRAM_FILES, L"\\Debugging Tools for Windows (x86)\\dbghelp.dll" }
#endif
    };

    PPH_STRING path;
    ULONG i;

    for (i = 0; i < sizeof(locations) / sizeof(locations[0]); i++)
    {
        path = PhGetKnownLocation(locations[i].Folder, locations[i].AppendPath);

        if (path)
        {
            if (RtlDoesFileExists_U(path->Buffer))
                return path;

            PhDereferenceObject(path);
        }
    }

    return NULL;
}

VOID PhMwpOnDestroy(
    VOID
    )
{
    // Notify plugins that we are shutting down.

    if (PhPluginsEnabled)
        PhUnloadPlugins();

    if (!PhMainWndExiting)
        ProcessHacker_SaveAllSettings(PhMainWndHandle);

    PhNfUninitialization();

    PostQuitMessage(0);
}

VOID PhMwpOnEndSession(
    VOID
    )
{
    PhMwpOnDestroy();
}

VOID PhMwpOnSettingChange(
    VOID
    )
{
    if (PhApplicationFont)
        DeleteObject(PhApplicationFont);

    PhInitializeFont(PhMainWndHandle);

    SendMessage(TabControlHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);
}

VOID PhMwpOnCommand(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_ESC_EXIT:
        {
            if (PhGetIntegerSetting(L"HideOnClose"))
            {
                if (PhNfTestIconMask(PH_ICON_ALL))
                    ShowWindow(PhMainWndHandle, SW_HIDE);
            }
            else if (PhGetIntegerSetting(L"CloseOnEscape"))
            {
                ProcessHacker_Destroy(PhMainWndHandle);
            }
        }
        break;
    case ID_HACKER_RUN:
        {
            if (RunFileDlg)
            {
                SelectedRunAsMode = 0;
                RunFileDlg(PhMainWndHandle, NULL, NULL, NULL, NULL, 0);
            }
        }
        break;
    case ID_HACKER_RUNASADMINISTRATOR:
        {
            if (RunFileDlg)
            {
                SelectedRunAsMode = RUNAS_MODE_ADMIN;
                RunFileDlg(
                    PhMainWndHandle,
                    NULL,
                    NULL,
                    NULL,
                    L"Type the name of a program that will be opened under alternate credentials.",
                    0
                    );
            }
        }
        break;
    case ID_HACKER_RUNASLIMITEDUSER:
        {
            if (RunFileDlg)
            {
                SelectedRunAsMode = RUNAS_MODE_LIMITED;
                RunFileDlg(
                    PhMainWndHandle,
                    NULL,
                    NULL,
                    NULL,
                    L"Type the name of a program that will be opened under standard user privileges.",
                    0
                    );
            }
        }
        break;
    case ID_HACKER_RUNAS:
        {
            PhShowRunAsDialog(PhMainWndHandle, NULL);
        }
        break;
    case ID_HACKER_SHOWDETAILSFORALLPROCESSES:
        {
            ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

            if (PhShellProcessHacker(
                PhMainWndHandle,
                L"-v",
                SW_SHOW,
                PH_SHELL_EXECUTE_ADMIN,
                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                0,
                NULL
                ))
            {
                ProcessHacker_Destroy(PhMainWndHandle);
            }
            else
            {
                ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);
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
            ULONG selectedTab = TabCtrl_GetCurSel(TabControlHandle);
            PWSTR tabText = L"Output";
            PPH_ADDITIONAL_TAB_PAGE selectedTabPage = NULL;

            if (selectedTab == ProcessesTabIndex)
            {
                tabText = L"Processes";
            }
            else if (selectedTab == ServicesTabIndex)
            {
                tabText = L"Services";
            }
            else if (selectedTab == NetworkTabIndex)
            {
                tabText = L"Network";
            }
            else if (AdditionalTabPageList)
            {
                ULONG i;

                for (i = 0; i < AdditionalTabPageList->Count; i++)
                {
                    PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

                    if (tabPage->Index == selectedTab)
                    {
                        selectedTabPage = tabPage;
                        tabText = selectedTabPage->Text;
                        break;
                    }
                }
            }

            PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
            PhSetFileDialogFileName(fileDialog, PhaFormatString(L"Process Hacker %s.txt", tabText)->Buffer);

            if (PhShowFileDialog(PhMainWndHandle, fileDialog))
            {
                NTSTATUS status;
                PPH_STRING fileName;
                ULONG filterIndex;
                PPH_FILE_STREAM fileStream;

                fileName = PhGetFileDialogFileName(fileDialog);
                PhAutoDereferenceObject(fileName);
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

                    if (filterIndex == 2)
                        mode = PH_EXPORT_MODE_CSV;
                    else
                        mode = PH_EXPORT_MODE_TABS;

                    PhWriteStringAsUtf8FileStream(fileStream, &PhUnicodeByteOrderMark);
                    PhWritePhTextHeader(fileStream);

                    if (selectedTab == ProcessesTabIndex)
                    {
                        PhWriteProcessTree(fileStream, mode);
                    }
                    else if (selectedTab == ServicesTabIndex)
                    {
                        PhWriteServiceList(fileStream, mode);
                    }
                    else if (selectedTab == NetworkTabIndex)
                    {
                        PhWriteNetworkList(fileStream, mode);
                    }
                    else if (selectedTabPage)
                    {
                        if (selectedTabPage->SaveContentCallback)
                        {
                            selectedTabPage->SaveContentCallback(fileStream, UlongToPtr(mode), NULL, selectedTabPage->Context);
                        }
                    }

                    PhDereferenceObject(fileStream);
                }

                if (!NT_SUCCESS(status))
                    PhShowStatus(PhMainWndHandle, L"Unable to create the file", status, 0);
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
            PhShowOptionsDialog(PhMainWndHandle);
        }
        break;
    case ID_HACKER_PLUGINS:
        {
            PhShowPluginsDialog(PhMainWndHandle);
        }
        break;
    case ID_COMPUTER_LOCK:
    case ID_COMPUTER_LOGOFF:
    case ID_COMPUTER_SLEEP:
    case ID_COMPUTER_HIBERNATE:
    case ID_COMPUTER_RESTART:
    case ID_COMPUTER_RESTARTBOOTOPTIONS:
    case ID_COMPUTER_SHUTDOWN:
    case ID_COMPUTER_SHUTDOWNHYBRID:
        PhMwpExecuteComputerCommand(Id);
        break;
    case ID_HACKER_EXIT:
        ProcessHacker_Destroy(PhMainWndHandle);
        break;
    case ID_VIEW_SYSTEMINFORMATION:
        PhShowSystemInformationDialog(NULL);
        break;
    case ID_TRAYICONS_CPUHISTORY:
    case ID_TRAYICONS_CPUUSAGE:
    case ID_TRAYICONS_IOHISTORY:
    case ID_TRAYICONS_COMMITHISTORY:
    case ID_TRAYICONS_PHYSICALMEMORYHISTORY:
        {
            ULONG i;

            switch (Id)
            {
            case ID_TRAYICONS_CPUHISTORY:
                i = PH_ICON_CPU_HISTORY;
                break;
            case ID_TRAYICONS_CPUUSAGE:
                i = PH_ICON_CPU_USAGE;
                break;
            case ID_TRAYICONS_IOHISTORY:
                i = PH_ICON_IO_HISTORY;
                break;
            case ID_TRAYICONS_COMMITHISTORY:
                i = PH_ICON_COMMIT_HISTORY;
                break;
            case ID_TRAYICONS_PHYSICALMEMORYHISTORY:
                i = PH_ICON_PHYSICAL_HISTORY;
                break;
            }

            PhNfSetVisibleIcon(i, !PhNfTestIconMask(i));
        }
        break;
    case ID_VIEW_HIDEPROCESSESFROMOTHERUSERS:
        {
            if (!CurrentUserFilterEntry)
            {
                CurrentUserFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpCurrentUserProcessTreeFilter, NULL);
            }
            else
            {
                PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), CurrentUserFilterEntry);
                CurrentUserFilterEntry = NULL;
            }

            PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());

            PhSetIntegerSetting(L"HideOtherUserProcesses", !!CurrentUserFilterEntry);
        }
        break;
    case ID_VIEW_HIDESIGNEDPROCESSES:
        {
            if (!SignedFilterEntry)
            {
                if (!PhEnableProcessQueryStage2)
                {
                    PhShowInformation(
                        PhMainWndHandle,
                        L"This filter cannot function because digital signature checking is not enabled. "
                        L"Enable it in Options > Advanced and restart Process Hacker."
                        );
                }

                SignedFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpSignedProcessTreeFilter, NULL);
            }
            else
            {
                PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), SignedFilterEntry);
                SignedFilterEntry = NULL;
            }

            PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());

            PhSetIntegerSetting(L"HideSignedProcesses", !!SignedFilterEntry);
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
            if (!DriverFilterEntry)
            {
                DriverFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), PhMwpDriverServiceTreeFilter, NULL);
            }
            else
            {
                PhRemoveTreeNewFilter(PhGetFilterSupportServiceTreeList(), DriverFilterEntry);
                DriverFilterEntry = NULL;
            }

            PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());

            PhSetIntegerSetting(L"HideDriverServices", !!DriverFilterEntry);
        }
        break;
    case ID_VIEW_ALWAYSONTOP:
        {
            AlwaysOnTop = !AlwaysOnTop;
            SetWindowPos(PhMainWndHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            PhSetIntegerSetting(L"MainWindowAlwaysOnTop", AlwaysOnTop);
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
            PhSetWindowOpacity(PhMainWndHandle, opacity);
        }
        break;
    case ID_VIEW_REFRESH:
        {
            PhBoostProvider(&ProcessProviderRegistration, NULL);
            PhBoostProvider(&ServiceProviderRegistration, NULL);
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
            UpdateAutomatically = !UpdateAutomatically;

            PhSetEnabledProvider(&ProcessProviderRegistration, UpdateAutomatically);
            PhSetEnabledProvider(&ServiceProviderRegistration, UpdateAutomatically);

            if (TabCtrl_GetCurSel(TabControlHandle) == NetworkTabIndex)
                PhSetEnabledProvider(&NetworkProviderRegistration, UpdateAutomatically);
        }
        break;
    case ID_TOOLS_CREATESERVICE:
        {
            PhShowCreateServiceDialog(PhMainWndHandle);
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
                { L"Executable files (*.exe;*.dll;*.ocx;*.sys;*.scr;*.cpl)", L"*.exe;*.dll;*.ocx;*.sys;*.scr;*.cpl" },
                { L"All files (*.*)", L"*.*" }
            };
            PVOID fileDialog = PhCreateOpenFileDialog();

            PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

            if (PhShowFileDialog(PhMainWndHandle, fileDialog))
            {
                PPH_STRING fileName;

                fileName = PhGetFileDialogFileName(fileDialog);
                PhShellExecuteUserString(
                    PhMainWndHandle,
                    L"ProgramInspectExecutables",
                    fileName->Buffer,
                    FALSE,
                    L"Make sure the PE Viewer executable file is present."
                    );
                PhDereferenceObject(fileName);
            }

            PhFreeFileDialog(fileDialog);
        }
        break;
    case ID_TOOLS_PAGEFILES:
        {
            PhShowPagefilesDialog(PhMainWndHandle);
        }
        break;
    case ID_TOOLS_STARTTASKMANAGER:
        {
            PPH_STRING systemDirectory;
            PPH_STRING taskmgrFileName;

            systemDirectory = PhGetSystemDirectory();
            taskmgrFileName = PhConcatStrings2(systemDirectory->Buffer, L"\\taskmgr.exe");
            PhDereferenceObject(systemDirectory);

            if (WindowsVersion >= WINDOWS_8 && !PhElevated)
            {
                if (PhUiConnectToPhSvc(PhMainWndHandle, FALSE))
                {
                    PhSvcCallCreateProcessIgnoreIfeoDebugger(taskmgrFileName->Buffer);
                    PhUiDisconnectFromPhSvc();
                }
            }
            else
            {
                PhCreateProcessIgnoreIfeoDebugger(taskmgrFileName->Buffer);
            }

            PhDereferenceObject(taskmgrFileName);
        }
        break;
    case ID_USER_CONNECT:
        {
            PhUiConnectSession(PhMainWndHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_DISCONNECT:
        {
            PhUiDisconnectSession(PhMainWndHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_LOGOFF:
        {
            PhUiLogoffSession(PhMainWndHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_REMOTECONTROL:
        {
            PhShowSessionShadowDialog(PhMainWndHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_SENDMESSAGE:
        {
            PhShowSessionSendMessageDialog(PhMainWndHandle, SelectedUserSessionId);
        }
        break;
    case ID_USER_PROPERTIES:
        {
            PhShowSessionProperties(PhMainWndHandle, SelectedUserSessionId);
        }
        break;
    case ID_HELP_LOG:
        {
            PhShowLogDialog();
        }
        break;
    case ID_HELP_DONATE:
        {
            PhShellExecute(PhMainWndHandle, L"https://sourceforge.net/project/project_donations.php?group_id=242527", NULL);
        }
        break;
    case ID_HELP_DEBUGCONSOLE:
        {
            PhShowDebugConsole();
        }
        break;
    case ID_HELP_ABOUT:
        {
            PhShowAboutDialog(PhMainWndHandle);
        }
        break;
    case ID_PROCESS_TERMINATE:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);

            if (PhUiTerminateProcesses(PhMainWndHandle, processes, numberOfProcesses))
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

                if (PhUiTerminateTreeProcess(PhMainWndHandle, processItem))
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
            PhUiSuspendProcesses(PhMainWndHandle, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
        }
        break;
    case ID_PROCESS_RESUME:
        {
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            PhReferenceObjects(processes, numberOfProcesses);
            PhUiResumeProcesses(PhMainWndHandle, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
        }
        break;
    case ID_PROCESS_RESTART:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);

                if (PhUiRestartProcess(PhMainWndHandle, processItem))
                    PhDeselectAllProcessNodes();

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
                PhUiCreateDumpFileProcess(PhMainWndHandle, processItem);
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
                PhUiDebugProcess(PhMainWndHandle, processItem);
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
                    PhMainWndHandle,
                    processItem,
                    !SelectedProcessVirtualizationEnabled
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
                PhShowProcessAffinityDialog(PhMainWndHandle, processItem, NULL);
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
                PhUiDetachFromDebuggerProcess(PhMainWndHandle, processItem);
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
                PhShowGdiHandlesDialog(PhMainWndHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_MISCELLANEOUS_INJECTDLL:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhReferenceObject(processItem);
                PhUiInjectDllProcess(PhMainWndHandle, processItem);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PAGEPRIORITY_1:
    case ID_PAGEPRIORITY_2:
    case ID_PAGEPRIORITY_3:
    case ID_PAGEPRIORITY_4:
    case ID_PAGEPRIORITY_5:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                ULONG pagePriority;

                switch (Id)
                {
                    case ID_PAGEPRIORITY_1:
                        pagePriority = 1;
                        break;
                    case ID_PAGEPRIORITY_2:
                        pagePriority = 2;
                        break;
                    case ID_PAGEPRIORITY_3:
                        pagePriority = 3;
                        break;
                    case ID_PAGEPRIORITY_4:
                        pagePriority = 4;
                        break;
                    case ID_PAGEPRIORITY_5:
                        pagePriority = 5;
                        break;
                }

                PhReferenceObject(processItem);
                PhUiSetPagePriorityProcess(PhMainWndHandle, processItem, pagePriority);
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
            PhUiReduceWorkingSetProcesses(PhMainWndHandle, processes, numberOfProcesses);
            PhDereferenceObjects(processes, numberOfProcesses);
            PhFree(processes);
        }
        break;
    case ID_MISCELLANEOUS_RUNAS:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem && processItem->FileName)
            {
                PhSetStringSetting2(L"RunAsProgram", &processItem->FileName->sr);
                PhShowRunAsDialog(PhMainWndHandle, NULL);
            }
        }
        break;
    case ID_MISCELLANEOUS_RUNASTHISUSER:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhShowRunAsDialog(PhMainWndHandle, processItem->ProcessId);
            }
        }
        break;
    case ID_MISCELLANEOUS_TERMINATOR:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                // The object relies on the list view reference, which could
                // disappear if we don't reference the object here.
                PhReferenceObject(processItem);
                PhShowProcessTerminatorDialog(PhMainWndHandle, processItem);
                PhDereferenceObject(processItem);
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
    case ID_I_0:
    case ID_I_1:
    case ID_I_2:
    case ID_I_3:
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
    case ID_WINDOW_BRINGTOFRONT:
        {
            if (IsWindow(SelectedProcessWindowHandle))
            {
                WINDOWPLACEMENT placement = { sizeof(placement) };

                GetWindowPlacement(SelectedProcessWindowHandle, &placement);

                if (placement.showCmd == SW_MINIMIZE)
                    ShowWindowAsync(SelectedProcessWindowHandle, SW_RESTORE);
                else
                    SetForegroundWindow(SelectedProcessWindowHandle);
            }
        }
        break;
    case ID_WINDOW_RESTORE:
        {
            if (IsWindow(SelectedProcessWindowHandle))
            {
                ShowWindowAsync(SelectedProcessWindowHandle, SW_RESTORE);
            }
        }
        break;
    case ID_WINDOW_MINIMIZE:
        {
            if (IsWindow(SelectedProcessWindowHandle))
            {
                ShowWindowAsync(SelectedProcessWindowHandle, SW_MINIMIZE);
            }
        }
        break;
    case ID_WINDOW_MAXIMIZE:
        {
            if (IsWindow(SelectedProcessWindowHandle))
            {
                ShowWindowAsync(SelectedProcessWindowHandle, SW_MAXIMIZE);
            }
        }
        break;
    case ID_WINDOW_CLOSE:
        {
            if (IsWindow(SelectedProcessWindowHandle))
            {
                PostMessage(SelectedProcessWindowHandle, WM_CLOSE, 0, 0);
            }
        }
        break;
    case ID_PROCESS_OPENFILELOCATION:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem && processItem->FileName)
            {
                PhReferenceObject(processItem);
                PhShellExploreFile(PhMainWndHandle, processItem->FileName->Buffer);
                PhDereferenceObject(processItem);
            }
        }
        break;
    case ID_PROCESS_SEARCHONLINE:
        {
            PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

            if (processItem)
            {
                PhSearchOnlineString(PhMainWndHandle, processItem->ProcessName->Buffer);
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
                    PhMwpSelectTabPage(ProcessesTabIndex);
                    SetFocus(ProcessTreeListHandle);
                    PhSelectAndEnsureVisibleProcessNode(processNode);
                }
            }
        }
        break;
    case ID_SERVICE_START:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                PhReferenceObject(serviceItem);
                PhUiStartService(PhMainWndHandle, serviceItem);
                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_CONTINUE:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                PhReferenceObject(serviceItem);
                PhUiContinueService(PhMainWndHandle, serviceItem);
                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_PAUSE:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                PhReferenceObject(serviceItem);
                PhUiPauseService(PhMainWndHandle, serviceItem);
                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_STOP:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                PhReferenceObject(serviceItem);
                PhUiStopService(PhMainWndHandle, serviceItem);
                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_DELETE:
        {
            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                PhReferenceObject(serviceItem);

                if (PhUiDeleteService(PhMainWndHandle, serviceItem))
                    PhDeselectAllServiceNodes();

                PhDereferenceObject(serviceItem);
            }
        }
        break;
    case ID_SERVICE_OPENKEY:
        {
            static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
            static PH_STRINGREF hklm = PH_STRINGREF_INIT(L"HKLM\\");

            PPH_SERVICE_ITEM serviceItem = PhGetSelectedServiceItem();

            if (serviceItem)
            {
                HANDLE keyHandle;
                PPH_STRING serviceKeyName = PhConcatStringRef2(&servicesKeyName, &serviceItem->Name->sr);

                if (NT_SUCCESS(PhOpenKey(
                    &keyHandle,
                    KEY_READ,
                    PH_KEY_LOCAL_MACHINE,
                    &serviceKeyName->sr,
                    0
                    )))
                {
                    PPH_STRING hklmServiceKeyName;

                    hklmServiceKeyName = PhConcatStringRef2(&hklm, &serviceKeyName->sr);
                    PhShellOpenKey2(PhMainWndHandle, hklmServiceKeyName);
                    PhDereferenceObject(hklmServiceKeyName);

                    NtClose(keyHandle);
                }

                PhDereferenceObject(serviceKeyName);
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

                if (fileName = PhGetServiceRelevantFileName(&serviceItem->Name->sr, serviceHandle))
                {
                    PhShellExploreFile(PhMainWndHandle, fileName->Buffer);
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
                PhShowServiceProperties(PhMainWndHandle, serviceItem);
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
                    PhMwpSelectTabPage(ProcessesTabIndex);
                    SetFocus(ProcessTreeListHandle);
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
                    PhMwpSelectTabPage(ServicesTabIndex);
                    SetFocus(ServiceTreeListHandle);
                    ProcessHacker_SelectServiceItem(PhMainWndHandle, serviceItem);

                    PhDereferenceObject(serviceItem);
                }
            }
        }
        break;
    case ID_NETWORK_VIEWSTACK:
        {
            PPH_NETWORK_ITEM networkItem = PhGetSelectedNetworkItem();

            if (networkItem)
            {
                PhReferenceObject(networkItem);
                PhShowNetworkStackDialog(PhMainWndHandle, networkItem);
                PhDereferenceObject(networkItem);
            }
        }
        break;
    case ID_NETWORK_CLOSE:
        {
            PPH_NETWORK_ITEM *networkItems;
            ULONG numberOfNetworkItems;

            PhGetSelectedNetworkItems(&networkItems, &numberOfNetworkItems);
            PhReferenceObjects(networkItems, numberOfNetworkItems);

            if (PhUiCloseConnections(PhMainWndHandle, networkItems, numberOfNetworkItems))
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

            if (selectedIndex != MaxTabIndex)
                selectedIndex++;
            else
                selectedIndex = 0;

            PhMwpSelectTabPage(selectedIndex);
        }
        break;
    case ID_TAB_PREV:
        {
            ULONG selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

            if (selectedIndex != 0)
                selectedIndex--;
            else
                selectedIndex = MaxTabIndex;

            PhMwpSelectTabPage(selectedIndex);
        }
        break;
    }
}

VOID PhMwpOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    if (NeedsMaximize)
    {
        ShowWindow(PhMainWndHandle, SW_MAXIMIZE);
        NeedsMaximize = FALSE;
    }
}

BOOLEAN PhMwpOnSysCommand(
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    )
{
    switch (Type)
    {
    case SC_CLOSE:
        {
            if (PhGetIntegerSetting(L"HideOnClose") && PhNfTestIconMask(PH_ICON_ALL))
            {
                ShowWindow(PhMainWndHandle, SW_HIDE);
                return TRUE;
            }
        }
        break;
    case SC_MINIMIZE:
        {
            // Save the current window state because we
            // may not have a chance to later.
            PhMwpSaveWindowSettings();

            if (PhGetIntegerSetting(L"HideOnMinimize") && PhNfTestIconMask(PH_ICON_ALL))
            {
                ShowWindow(PhMainWndHandle, SW_HIDE);
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhMwpOnMenuCommand(
    _In_ ULONG Index,
    _In_ HMENU Menu
    )
{
    MENUITEMINFO menuItemInfo;

    menuItemInfo.cbSize = sizeof(MENUITEMINFO);
    menuItemInfo.fMask = MIIM_ID | MIIM_DATA;

    if (GetMenuItemInfo(Menu, Index, TRUE, &menuItemInfo))
    {
        PhMwpDispatchMenuCommand(Menu, Index, menuItemInfo.wID, menuItemInfo.dwItemData);
    }
}

VOID PhMwpOnInitMenuPopup(
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

    for (i = 0; i < sizeof(SubMenuHandles) / sizeof(HWND); i++)
    {
        if (Menu == SubMenuHandles[i])
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
        return;

    if (Index == 3)
    {
        // Special case for Users menu.
        if (!UsersMenuInitialized)
        {
            PhMwpUpdateUsersMenu();
            UsersMenuInitialized = TRUE;
        }

        return;
    }

    // Delete all items in this submenu.
    while (DeleteMenu(Menu, 0, MF_BYPOSITION)) ;

    // Delete the previous EMENU for this submenu.
    if (SubMenuObjects[Index])
        PhDestroyEMenu(SubMenuObjects[Index]);

    // Make sure the menu style is set correctly.
    memset(&menuInfo, 0, sizeof(MENUINFO));
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_CHECKORBMP;
    SetMenuInfo(Menu, &menuInfo);

    menu = PhCreateEMenu();
    PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND), Index);

    PhMwpInitializeSubMenu(menu, Index);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        PhPluginInitializeMenuInfo(&menuInfo, menu, PhMainWndHandle, PH_PLUGIN_MENU_DISALLOW_HOOKS);
        menuInfo.u.MainMenu.SubMenuIndex = Index;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainMenuInitializing), &menuInfo);
    }

    PhEMenuToHMenu2(Menu, menu, 0, NULL);
    SubMenuObjects[Index] = menu;
}

VOID PhMwpOnSize(
    VOID
    )
{
    if (!IsIconic(PhMainWndHandle))
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
    PhResizingMinimumSize(DragRectangle, Edge, 400, 340);
}

VOID PhMwpOnSetFocus(
    VOID
    )
{
    INT selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == ProcessesTabIndex)
        SetFocus(ProcessTreeListHandle);
    else if (selectedIndex == ServicesTabIndex)
        SetFocus(ServiceTreeListHandle);
    else if (selectedIndex == NetworkTabIndex)
        SetFocus(NetworkTreeListHandle);
}

VOID PhMwpOnTimer(
    _In_ ULONG Id
    )
{
    if (Id == TIMER_FLUSH_PROCESS_QUERY_DATA)
    {
        static ULONG state = 1;

        // If the update interval is too large, the user might have to wait a while before seeing some types of
        // process-related data. Here we force an update.
        //
        // In addition, we force updates shortly after the program starts up to make things appear more quickly.

        switch (state)
        {
        case 1:
            state = 2;

            if (PhCsUpdateInterval > PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_2)
                SetTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA, PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_2, NULL);
            else
                KillTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA);

            break;
        case 2:
            state = 3;

            if (PhCsUpdateInterval > PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM)
                SetTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA, PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM, NULL);
            else
                KillTimer(PhMainWndHandle, TIMER_FLUSH_PROCESS_QUERY_DATA);

            break;
        default:
            NOTHING;
            break;
        }

        PhFlushProcessQueryData(TRUE);
    }
}

BOOLEAN PhMwpOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    )
{
    if (Header->hwndFrom == TabControlHandle)
    {
        PhMwpNotifyTabControl(Header);
    }
    else if (Header->code == RFN_VALIDATE)
    {
        LPNMRUNFILEDLG runFileDlg = (LPNMRUNFILEDLG)Header;

        if (SelectedRunAsMode == RUNAS_MODE_ADMIN)
        {
            PH_STRINGREF string;
            PH_STRINGREF fileName;
            PH_STRINGREF arguments;
            PPH_STRING fullFileName;
            PPH_STRING argumentsString;

            PhInitializeStringRefLongHint(&string, (PWSTR)runFileDlg->lpszFile);
            PhParseCommandLineFuzzy(&string, &fileName, &arguments, &fullFileName);

            if (!fullFileName)
                fullFileName = PhCreateString2(&fileName);

            argumentsString = PhCreateString2(&arguments);

            if (PhShellExecuteEx(PhMainWndHandle, fullFileName->Buffer, argumentsString->Buffer,
                runFileDlg->nShow, PH_SHELL_EXECUTE_ADMIN, 0, NULL))
            {
                *Result = RF_CANCEL;
            }
            else
            {
                *Result = RF_RETRY;
            }

            PhDereferenceObject(fullFileName);
            PhDereferenceObject(argumentsString);

            return TRUE;
        }
        else if (SelectedRunAsMode == RUNAS_MODE_LIMITED)
        {
            NTSTATUS status;
            HANDLE tokenHandle;
            HANDLE newTokenHandle;

            if (NT_SUCCESS(status = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ADJUST_GROUPS |
                TOKEN_ADJUST_DEFAULT | READ_CONTROL | WRITE_DAC,
                &tokenHandle
                )))
            {
                if (NT_SUCCESS(status = PhFilterTokenForLimitedUser(
                    tokenHandle,
                    &newTokenHandle
                    )))
                {
                    status = PhCreateProcessWin32(
                        NULL,
                        (PWSTR)runFileDlg->lpszFile,
                        NULL,
                        NULL,
                        0,
                        newTokenHandle,
                        NULL,
                        NULL
                        );

                    NtClose(newTokenHandle);
                }

                NtClose(tokenHandle);
            }

            if (NT_SUCCESS(status))
            {
                *Result = RF_CANCEL;
            }
            else
            {
                PhShowStatus(PhMainWndHandle, L"Unable to execute the program", status, 0);
                *Result = RF_RETRY;
            }

            return TRUE;
        }
    }

    return FALSE;
}

VOID PhMwpOnWtsSessionChange(
    _In_ ULONG Reason,
    _In_ ULONG SessionId
    )
{
    if (Reason == WTS_SESSION_LOGON || Reason == WTS_SESSION_LOGOFF)
    {
        if (UsersMenuInitialized)
        {
            PhMwpUpdateUsersMenu();
        }
    }
}

ULONG_PTR PhMwpOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case WM_PH_ACTIVATE:
        {
            if (!PhMainWndExiting)
            {
                if (WParam != 0)
                {
                    PPH_PROCESS_NODE processNode;

                    if (processNode = PhFindProcessNode((HANDLE)WParam))
                        PhSelectAndEnsureVisibleProcessNode(processNode);
                }

                if (!IsWindowVisible(PhMainWndHandle))
                {
                    ShowWindow(PhMainWndHandle, SW_SHOW);
                }

                if (IsIconic(PhMainWndHandle))
                {
                    ShowWindow(PhMainWndHandle, SW_RESTORE);
                }

                return PH_ACTIVATE_REPLY;
            }
            else
            {
                return 0;
            }
        }
        break;
    case WM_PH_SHOW_PROCESS_PROPERTIES:
        {
            PhMwpShowProcessProperties((PPH_PROCESS_ITEM)LParam);
        }
        break;
    case WM_PH_DESTROY:
        {
            DestroyWindow(PhMainWndHandle);
        }
        break;
    case WM_PH_SAVE_ALL_SETTINGS:
        {
            PhMwpSaveSettings();
        }
        break;
    case WM_PH_PREPARE_FOR_EARLY_SHUTDOWN:
        {
            PhMwpSaveSettings();
            PhMainWndExiting = TRUE;
        }
        break;
    case WM_PH_CANCEL_EARLY_SHUTDOWN:
        {
            PhMainWndExiting = FALSE;
        }
        break;
    case WM_PH_DELAYED_LOAD_COMPLETED:
        {
            // Nothing
        }
        break;
    case WM_PH_NOTIFY_ICON_MESSAGE:
        {
            PhNfForwardMessage(WParam, LParam);
        }
        break;
    case WM_PH_TOGGLE_VISIBLE:
        {
            PhMwpActivateWindow(!WParam);
        }
        break;
    case WM_PH_SHOW_MEMORY_EDITOR:
        {
            PPH_SHOWMEMORYEDITOR showMemoryEditor = (PPH_SHOWMEMORYEDITOR)LParam;

            PhShowMemoryEditorDialog(
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
    case WM_PH_SHOW_MEMORY_RESULTS:
        {
            PPH_SHOWMEMORYRESULTS showMemoryResults = (PPH_SHOWMEMORYRESULTS)LParam;

            PhShowMemoryResultsDialog(
                showMemoryResults->ProcessId,
                showMemoryResults->Results
                );
            PhDereferenceMemoryResults(
                (PPH_MEMORY_RESULT *)showMemoryResults->Results->Items,
                showMemoryResults->Results->Count
                );
            PhDereferenceObject(showMemoryResults->Results);
            PhFree(showMemoryResults);
        }
        break;
    case WM_PH_SELECT_TAB_PAGE:
        {
            ULONG index = (ULONG)WParam;

            PhMwpSelectTabPage(index);

            if (index == ProcessesTabIndex)
                SetFocus(ProcessTreeListHandle);
            else if (index == ServicesTabIndex)
                SetFocus(ServiceTreeListHandle);
            else if (index == NetworkTabIndex)
                SetFocus(NetworkTreeListHandle);
        }
        break;
    case WM_PH_GET_CALLBACK_LAYOUT_PADDING:
        {
            return (ULONG_PTR)&LayoutPaddingCallback;
        }
        break;
    case WM_PH_INVALIDATE_LAYOUT_PADDING:
        {
            LayoutPaddingValid = FALSE;
        }
        break;
    case WM_PH_SELECT_PROCESS_NODE:
        {
            PhSelectAndEnsureVisibleProcessNode((PPH_PROCESS_NODE)LParam);
        }
        break;
    case WM_PH_SELECT_SERVICE_ITEM:
        {
            PPH_SERVICE_NODE serviceNode;

            PhMwpNeedServiceTreeList();

            // For compatibility, LParam is a service item, not node.
            if (serviceNode = PhFindServiceNode((PPH_SERVICE_ITEM)LParam))
            {
                PhSelectAndEnsureVisibleServiceNode(serviceNode);
            }
        }
        break;
    case WM_PH_SELECT_NETWORK_ITEM:
        {
            PPH_NETWORK_NODE networkNode;

            PhMwpNeedNetworkTreeList();

            // For compatibility, LParam is a network item, not node.
            if (networkNode = PhFindNetworkNode((PPH_NETWORK_ITEM)LParam))
            {
                PhSelectAndEnsureVisibleNetworkNode(networkNode);
            }
        }
        break;
    case WM_PH_UPDATE_FONT:
        {
            PPH_STRING fontHexString;
            LOGFONT font;

            fontHexString = PhGetStringSetting(L"Font");

            if (
                fontHexString->Length / 2 / 2 == sizeof(LOGFONT) &&
                PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)&font)
                )
            {
                HFONT newFont;

                newFont = CreateFontIndirect(&font);

                if (newFont)
                {
                    if (CurrentCustomFont)
                        DeleteObject(CurrentCustomFont);

                    CurrentCustomFont = newFont;

                    SendMessage(ProcessTreeListHandle, WM_SETFONT, (WPARAM)newFont, TRUE);
                    SendMessage(ServiceTreeListHandle, WM_SETFONT, (WPARAM)newFont, TRUE);
                    SendMessage(NetworkTreeListHandle, WM_SETFONT, (WPARAM)newFont, TRUE);

                    if (AdditionalTabPageList)
                    {
                        ULONG i;

                        for (i = 0; i < AdditionalTabPageList->Count; i++)
                        {
                            PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

                            if (tabPage->FontChangedCallback)
                            {
                                tabPage->FontChangedCallback((PVOID)newFont, NULL, NULL, tabPage->Context);
                            }
                        }
                    }
                }
            }

            PhDereferenceObject(fontHexString);
        }
        break;
    case WM_PH_GET_FONT:
        return SendMessage(ProcessTreeListHandle, WM_GETFONT, 0, 0);
    case WM_PH_INVOKE:
        {
            VOID (NTAPI *function)(PVOID);

            function = (PVOID)LParam;
            function((PVOID)WParam);
        }
        break;
    case WM_PH_ADD_MENU_ITEM:
        {
            PPH_ADDMENUITEM addMenuItem = (PPH_ADDMENUITEM)LParam;

            return PhMwpLegacyAddPluginMenuItem(addMenuItem);
        }
        break;
    case WM_PH_ADD_TAB_PAGE:
        {
            return (ULONG_PTR)PhMwpAddTabPage((PPH_ADDITIONAL_TAB_PAGE)LParam);
        }
        break;
    case WM_PH_REFRESH:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_REFRESH, 0);
        }
        break;
    case WM_PH_GET_UPDATE_AUTOMATICALLY:
        {
            return UpdateAutomatically;
        }
        break;
    case WM_PH_SET_UPDATE_AUTOMATICALLY:
        {
            if (!!WParam != UpdateAutomatically)
            {
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_UPDATEAUTOMATICALLY, 0);
            }
        }
        break;
    case WM_PH_ICON_CLICK:
        {
            PhMwpActivateWindow(!!PhGetIntegerSetting(L"IconTogglesVisibility"));
        }
        break;
    case WM_PH_PROCESS_ADDED:
        {
            ULONG runId = (ULONG)WParam;
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)LParam;

            PhMwpOnProcessAdded(processItem, runId);
        }
        break;
    case WM_PH_PROCESS_MODIFIED:
        {
            PhMwpOnProcessModified((PPH_PROCESS_ITEM)LParam);
        }
        break;
    case WM_PH_PROCESS_REMOVED:
        {
            PhMwpOnProcessRemoved((PPH_PROCESS_ITEM)LParam);
        }
        break;
    case WM_PH_PROCESSES_UPDATED:
        {
            PhMwpOnProcessesUpdated();
        }
        break;
    case WM_PH_SERVICE_ADDED:
        {
            ULONG runId = (ULONG)WParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)LParam;

            PhMwpOnServiceAdded(serviceItem, runId);
        }
        break;
    case WM_PH_SERVICE_MODIFIED:
        {
            PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)LParam;

            PhMwpOnServiceModified(serviceModifiedData);
            PhFree(serviceModifiedData);
        }
        break;
    case WM_PH_SERVICE_REMOVED:
        {
            PhMwpOnServiceRemoved((PPH_SERVICE_ITEM)LParam);
        }
        break;
    case WM_PH_SERVICES_UPDATED:
        {
            PhMwpOnServicesUpdated();
        }
        break;
    case WM_PH_NETWORK_ITEM_ADDED:
        {
            ULONG runId = (ULONG)WParam;
            PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)LParam;

            PhMwpOnNetworkItemAdded(runId, networkItem);
        }
        break;
    case WM_PH_NETWORK_ITEM_MODIFIED:
        {
            PhMwpOnNetworkItemModified((PPH_NETWORK_ITEM)LParam);
        }
        break;
    case WM_PH_NETWORK_ITEM_REMOVED:
        {
            PhMwpOnNetworkItemRemoved((PPH_NETWORK_ITEM)LParam);
        }
        break;
    case WM_PH_NETWORK_ITEMS_UPDATED:
        {
            PhMwpOnNetworkItemsUpdated();
        }
        break;
    }

    return 0;
}

VOID NTAPI PhMwpProcessAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // Reference the process item so it doesn't get deleted before
    // we handle the event in the main thread.
    PhReferenceObject(processItem);
    PostMessage(
        PhMainWndHandle,
        WM_PH_PROCESS_ADDED,
        (WPARAM)PhGetRunIdProvider(&ProcessProviderRegistration),
        (LPARAM)processItem
        );
}

VOID NTAPI PhMwpProcessModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_PROCESS_MODIFIED, 0, (LPARAM)processItem);
}

VOID NTAPI PhMwpProcessRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // We already have a reference to the process item, so we don't need to
    // reference it here.
    PostMessage(PhMainWndHandle, WM_PH_PROCESS_REMOVED, 0, (LPARAM)processItem);
}

VOID NTAPI PhMwpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_PROCESSES_UPDATED, 0, 0);
}

VOID NTAPI PhMwpServiceAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Parameter;

    PhReferenceObject(serviceItem);
    PostMessage(
        PhMainWndHandle,
        WM_PH_SERVICE_ADDED,
        PhGetRunIdProvider(&ServiceProviderRegistration),
        (LPARAM)serviceItem
        );
}

VOID NTAPI PhMwpServiceModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)Parameter;
    PPH_SERVICE_MODIFIED_DATA copy;

    copy = PhAllocateCopy(serviceModifiedData, sizeof(PH_SERVICE_MODIFIED_DATA));

    PostMessage(PhMainWndHandle, WM_PH_SERVICE_MODIFIED, 0, (LPARAM)copy);
}

VOID NTAPI PhMwpServiceRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_SERVICE_REMOVED, 0, (LPARAM)serviceItem);
}

VOID NTAPI PhMwpServicesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_SERVICES_UPDATED, 0, 0);
}

VOID NTAPI PhMwpNetworkItemAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PhReferenceObject(networkItem);
    PostMessage(
        PhMainWndHandle,
        WM_PH_NETWORK_ITEM_ADDED,
        PhGetRunIdProvider(&NetworkProviderRegistration),
        (LPARAM)networkItem
        );
}

VOID NTAPI PhMwpNetworkItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_NETWORK_ITEM_MODIFIED, 0, (LPARAM)networkItem);
}

VOID NTAPI PhMwpNetworkItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_NETWORK_ITEM_REMOVED, 0, (LPARAM)networkItem);
}

VOID NTAPI PhMwpNetworkItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_NETWORK_ITEMS_UPDATED, 0, 0);
}

VOID PhMwpLoadSettings(
    VOID
    )
{
    ULONG opacity;
    PPH_STRING customFont;

    if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
    {
        AlwaysOnTop = TRUE;
        SetWindowPos(PhMainWndHandle, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
    }

    opacity = PhGetIntegerSetting(L"MainWindowOpacity");

    if (opacity != 0)
        PhSetWindowOpacity(PhMainWndHandle, opacity);

    PhStatisticsSampleCount = PhGetIntegerSetting(L"SampleCount");
    PhEnableProcessQueryStage2 = !!PhGetIntegerSetting(L"EnableStage2");
    PhEnablePurgeProcessRecords = !PhGetIntegerSetting(L"NoPurgeProcessRecords");
    PhEnableCycleCpuUsage = !!PhGetIntegerSetting(L"EnableCycleCpuUsage");
    PhEnableServiceNonPoll = !!PhGetIntegerSetting(L"EnableServiceNonPoll");
    PhEnableNetworkProviderResolve = !!PhGetIntegerSetting(L"EnableNetworkResolve");

    PhNfLoadStage1();

    NotifyIconNotifyMask = PhGetIntegerSetting(L"IconNotifyMask");

    if (PhGetIntegerSetting(L"HideOtherUserProcesses"))
    {
        CurrentUserFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpCurrentUserProcessTreeFilter, NULL);
    }

    if (PhGetIntegerSetting(L"HideSignedProcesses"))
    {
        SignedFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpSignedProcessTreeFilter, NULL);
    }

    customFont = PhGetStringSetting(L"Font");

    if (customFont->Length / 2 / 2 == sizeof(LOGFONT))
        SendMessage(PhMainWndHandle, WM_PH_UPDATE_FONT, 0, 0);

    PhDereferenceObject(customFont);

    PhLoadSettingsProcessTreeList();
    // Service and network list settings are loaded on demand.
}

VOID PhMwpSaveSettings(
    VOID
    )
{
    PhSaveSettingsProcessTreeList();
    if (ServiceTreeListLoaded)
        PhSaveSettingsServiceTreeList();
    if (NetworkTreeListLoaded)
        PhSaveSettingsNetworkTreeList();

    PhNfSaveSettings();

    PhSetIntegerSetting(L"IconNotifyMask", NotifyIconNotifyMask);

    PhSaveWindowPlacementToSetting(L"MainWindowPosition", L"MainWindowSize", PhMainWndHandle);

    PhMwpSaveWindowSettings();

    if (PhSettingsFileName)
        PhSaveSettings(PhSettingsFileName->Buffer);
}

VOID PhMwpSaveWindowSettings(
    VOID
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };

    GetWindowPlacement(PhMainWndHandle, &placement);

    if (placement.showCmd == SW_NORMAL)
        PhSetIntegerSetting(L"MainWindowState", SW_NORMAL);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetIntegerSetting(L"MainWindowState", SW_MAXIMIZE);
}

VOID PhLoadDbgHelpFromPath(
    _In_ PWSTR DbgHelpPath
    )
{
    HMODULE dbghelpModule;

    if (dbghelpModule = LoadLibrary(DbgHelpPath))
    {
        PPH_STRING fullDbghelpPath;
        ULONG indexOfFileName;
        PH_STRINGREF dbghelpFolder;
        PPH_STRING symsrvPath;

        fullDbghelpPath = PhGetDllFileName(dbghelpModule, &indexOfFileName);

        if (fullDbghelpPath)
        {
            if (indexOfFileName != 0)
            {
                static PH_STRINGREF symsrvString = PH_STRINGREF_INIT(L"\\symsrv.dll");

                dbghelpFolder.Buffer = fullDbghelpPath->Buffer;
                dbghelpFolder.Length = indexOfFileName * sizeof(WCHAR);

                symsrvPath = PhConcatStringRef2(&dbghelpFolder, &symsrvString);
                LoadLibrary(symsrvPath->Buffer);
                PhDereferenceObject(symsrvPath);
            }

            PhDereferenceObject(fullDbghelpPath);
        }
    }
    else
    {
        dbghelpModule = LoadLibrary(L"dbghelp.dll");
    }

    PhSymbolProviderCompleteInitialization(dbghelpModule);
}

VOID PhMwpSymInitHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING dbghelpPath;

    dbghelpPath = PhGetStringSetting(L"DbgHelpPath");
    PhLoadDbgHelpFromPath(dbghelpPath->Buffer);
    PhDereferenceObject(dbghelpPath);
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
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_RESTARTBOOTOPTIONS))
            PhDestroyEMenuItem(menuItem);
        if (menuItem = PhFindEMenuItem(Root, PH_EMENU_FIND_DESCEND, NULL, ID_COMPUTER_SHUTDOWNHYBRID))
            PhDestroyEMenuItem(menuItem);
    }
}

BOOLEAN PhMwpExecuteComputerCommand(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_COMPUTER_LOCK:
        PhUiLockComputer(PhMainWndHandle);
        return TRUE;
    case ID_COMPUTER_LOGOFF:
        PhUiLogoffComputer(PhMainWndHandle);
        return TRUE;
    case ID_COMPUTER_SLEEP:
        PhUiSleepComputer(PhMainWndHandle);
        return TRUE;
    case ID_COMPUTER_HIBERNATE:
        PhUiHibernateComputer(PhMainWndHandle);
        return TRUE;
    case ID_COMPUTER_RESTART:
        PhUiRestartComputer(PhMainWndHandle, 0);
        return TRUE;
    case ID_COMPUTER_RESTARTBOOTOPTIONS:
        PhUiRestartComputer(PhMainWndHandle, EWX_BOOTOPTIONS);
        return TRUE;
    case ID_COMPUTER_SHUTDOWN:
        PhUiShutdownComputer(PhMainWndHandle, 0);
        return TRUE;
    case ID_COMPUTER_SHUTDOWNHYBRID:
        PhUiShutdownComputer(PhMainWndHandle, EWX_HYBRID_SHUTDOWN);
        return TRUE;
    }

    return FALSE;
}

VOID PhMwpActivateWindow(
    _In_ BOOLEAN Toggle
    )
{
    if (IsIconic(PhMainWndHandle))
    {
        ShowWindow(PhMainWndHandle, SW_RESTORE);
        SetForegroundWindow(PhMainWndHandle);
    }
    else if (IsWindowVisible(PhMainWndHandle))
    {
        if (Toggle)
            ShowWindow(PhMainWndHandle, SW_HIDE);
        else
            SetForegroundWindow(PhMainWndHandle);
    }
    else
    {
        ShowWindow(PhMainWndHandle, SW_SHOW);
        SetForegroundWindow(PhMainWndHandle);
    }
}

VOID PhMwpInitializeMainMenu(
    _In_ HMENU Menu
    )
{
    MENUINFO menuInfo;
    ULONG i;

    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_NOTIFYBYPOS;
    SetMenuInfo(Menu, &menuInfo);

    for (i = 0; i < sizeof(SubMenuHandles) / sizeof(HMENU); i++)
    {
        SubMenuHandles[i] = GetSubMenu(PhMainWndMenuHandle, i);
    }
}

VOID PhMwpDispatchMenuCommand(
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
                PhPluginInitializeMenuInfo(&menuInfo, NULL, PhMainWndHandle, 0);
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
                PhNfSetVisibleIcon(icon->IconId, !PhNfTestIconMask(icon->IconId));
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
            SelectedUserSessionId = (ULONG)ItemData;
        }
        break;
    }

    SendMessage(PhMainWndHandle, WM_COMMAND, ItemId, 0);
}

ULONG_PTR PhMwpLegacyAddPluginMenuItem(
    _In_ PPH_ADDMENUITEM AddMenuItem
    )
{
    PPH_ADDMENUITEM addMenuItem;
    PPH_PLUGIN_MENU_ITEM pluginMenuItem;

    if (!LegacyAddMenuItemList)
        LegacyAddMenuItemList = PhCreateList(8);

    addMenuItem = PhAllocateCopy(AddMenuItem, sizeof(PH_ADDMENUITEM));
    PhAddItemList(LegacyAddMenuItemList, addMenuItem);

    pluginMenuItem = PhAllocate(sizeof(PH_PLUGIN_MENU_ITEM));
    memset(pluginMenuItem, 0, sizeof(PH_PLUGIN_MENU_ITEM));
    pluginMenuItem->Plugin = AddMenuItem->Plugin;
    pluginMenuItem->Id = AddMenuItem->Id;
    pluginMenuItem->Context = AddMenuItem->Context;

    addMenuItem->Context = pluginMenuItem;

    return TRUE;
}

HBITMAP PhMwpGetShieldBitmap(
    VOID
    )
{
    static HBITMAP shieldBitmap = NULL;

    if (!shieldBitmap)
    {
        _LoadIconMetric loadIconMetric;
        HICON shieldIcon = NULL;

        // It is necessary to use LoadIconMetric because otherwise the icons are at the wrong
        // resolution and look very bad when scaled down to the small icon size.

        loadIconMetric = (_LoadIconMetric)PhGetModuleProcAddress(L"comctl32.dll", "LoadIconMetric");

        if (loadIconMetric)
        {
            if (SUCCEEDED(loadIconMetric(NULL, IDI_SHIELD, LIM_SMALL, &shieldIcon)))
            {
                shieldBitmap = PhIconToBitmap(shieldIcon, PhSmallIconSize.X, PhSmallIconSize.Y);
                DestroyIcon(shieldIcon);
            }
        }
    }

    return shieldBitmap;
}

VOID PhMwpInitializeSubMenu(
    _In_ PPH_EMENU Menu,
    _In_ ULONG Index
    )
{
    PPH_EMENU_ITEM menuItem;

    if (Index == 0) // Hacker
    {
        // Fix some menu items.
        if (PhElevated)
        {
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_RUNASADMINISTRATOR))
                PhDestroyEMenuItem(menuItem);
            if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_SHOWDETAILSFORALLPROCESSES))
                PhDestroyEMenuItem(menuItem);
        }
        else
        {
            HBITMAP shieldBitmap;

            if (shieldBitmap = PhMwpGetShieldBitmap())
            {
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_HACKER_SHOWDETAILSFORALLPROCESSES))
                    menuItem->Bitmap = shieldBitmap;
            }
        }

        // Fix up the Computer menu.
        PhMwpSetupComputerMenu(Menu);
    }
    else if (Index == 1) // View
    {
        PPH_EMENU_ITEM trayIconsMenuItem;
        ULONG i;
        PPH_EMENU_ITEM menuItem;
        ULONG id;
        ULONG placeholderIndex;

        trayIconsMenuItem = PhMwpFindTrayIconsMenuItem(Menu);

        if (trayIconsMenuItem)
        {
            ULONG maximum;
            PPH_NF_ICON icon;

            // Add menu items for the registered tray icons.

            id = PH_ICON_DEFAULT_MAXIMUM;
            maximum = PhNfGetMaximumIconId();

            for (; id != maximum; id <<= 1)
            {
                if (icon = PhNfGetIconById(id))
                {
                    PhInsertEMenuItem(trayIconsMenuItem, PhCreateEMenuItem(0, ID_TRAYICONS_REGISTERED, icon->Text, NULL, icon), -1);
                }
            }

            // Update the text and check marks on the menu items.

            for (i = 0; i < trayIconsMenuItem->Items->Count; i++)
            {
                menuItem = trayIconsMenuItem->Items->Items[i];

                id = -1;
                icon = NULL;

                switch (menuItem->Id)
                {
                case ID_TRAYICONS_CPUHISTORY:
                    id = PH_ICON_CPU_HISTORY;
                    break;
                case ID_TRAYICONS_IOHISTORY:
                    id = PH_ICON_IO_HISTORY;
                    break;
                case ID_TRAYICONS_COMMITHISTORY:
                    id = PH_ICON_COMMIT_HISTORY;
                    break;
                case ID_TRAYICONS_PHYSICALMEMORYHISTORY:
                    id = PH_ICON_PHYSICAL_HISTORY;
                    break;
                case ID_TRAYICONS_CPUUSAGE:
                    id = PH_ICON_CPU_USAGE;
                    break;
                case ID_TRAYICONS_REGISTERED:
                    icon = menuItem->Context;
                    id = icon->IconId;
                    break;
                }

                if (id != -1)
                {
                    if (PhNfTestIconMask(id))
                        menuItem->Flags |= PH_EMENU_CHECKED;

                    if (icon && (icon->Flags & PH_NF_ICON_UNAVAILABLE))
                    {
                        PPH_STRING newText;

                        newText = PhaConcatStrings2(icon->Text, L" (Unavailable)");
                        PhModifyEMenuItem(menuItem, PH_EMENU_MODIFY_TEXT, PH_EMENU_TEXT_OWNED,
                            PhAllocateCopy(newText->Buffer, newText->Length + sizeof(WCHAR)), NULL);
                    }
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
            id = -1;
            break;
        }

        if (id != -1 && (menuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, NULL, id)))
            menuItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;

        if (UpdateAutomatically && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_UPDATEAUTOMATICALLY)))
            menuItem->Flags |= PH_EMENU_CHECKED;
    }
    else if (Index == 2) // Tools
    {
#ifdef _WIN64
        if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_HIDDENPROCESSES))
            PhDestroyEMenuItem(menuItem);
#endif

        // Windows 8 Task Manager requires elevation.
        if (WindowsVersion >= WINDOWS_8 && !PhElevated)
        {
            HBITMAP shieldBitmap;

            if (shieldBitmap = PhMwpGetShieldBitmap())
            {
                if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_TOOLS_STARTTASKMANAGER))
                    menuItem->Bitmap = shieldBitmap;
            }
        }
    }

    if (LegacyAddMenuItemList)
    {
        ULONG i;
        PPH_ADDMENUITEM addMenuItem;

        for (i = 0; i < LegacyAddMenuItemList->Count; i++)
        {
            addMenuItem = LegacyAddMenuItemList->Items[i];

            if (addMenuItem->Location == Index)
            {
                ULONG insertIndex;

                if (addMenuItem->InsertAfter)
                {
                    for (insertIndex = 0; insertIndex < Menu->Items->Count; insertIndex++)
                    {
                        menuItem = Menu->Items->Items[insertIndex];

                        if (!(menuItem->Flags & PH_EMENU_SEPARATOR) && (PhCompareUnicodeStringZIgnoreMenuPrefix(
                            addMenuItem->InsertAfter,
                            menuItem->Text,
                            TRUE,
                            TRUE
                            ) == 0))
                        {
                            insertIndex++;
                            break;
                        }
                    }
                }
                else
                {
                    insertIndex = 0;
                }

                if (addMenuItem->Text[0] == '-' && addMenuItem->Text[1] == 0)
                    PhInsertEMenuItem(Menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, L"", NULL, NULL), insertIndex);
                else
                    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PLUGIN_MENU_ITEM, addMenuItem->Text, NULL, addMenuItem->Context), insertIndex);
            }
        }
    }
}

PPH_EMENU_ITEM PhMwpFindTrayIconsMenuItem(
    _In_ PPH_EMENU Menu
    )
{
    ULONG i;
    PPH_EMENU_ITEM menuItem;

    for (i = 0; i < Menu->Items->Count; i++)
    {
        menuItem = Menu->Items->Items[i];

        if (PhFindEMenuItem(menuItem, 0, NULL, ID_TRAYICONS_CPUHISTORY))
            return menuItem;
    }

    return NULL;
}

VOID PhMwpInitializeSectionMenuItems(
    _In_ PPH_EMENU Menu,
    _In_ ULONG StartIndex
    )
{
    INT selectedIndex;
    PPH_EMENU_ITEM menuItem;

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == ProcessesTabIndex)
    {
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS, L"Hide Processes From Other Users", NULL, NULL), StartIndex);
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_HIDESIGNEDPROCESSES, L"Hide Signed Processes", NULL, NULL), StartIndex + 1);
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_SCROLLTONEWPROCESSES, L"Scroll to New Processes", NULL, NULL), StartIndex + 2);
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_SHOWCPUBELOW001, L"Show CPU Below 0.01", NULL, NULL), StartIndex + 3);

        if (CurrentUserFilterEntry && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS)))
            menuItem->Flags |= PH_EMENU_CHECKED;
        if (SignedFilterEntry && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_HIDESIGNEDPROCESSES)))
            menuItem->Flags |= PH_EMENU_CHECKED;
        if (PhCsScrollToNewProcesses && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_SCROLLTONEWPROCESSES)))
            menuItem->Flags |= PH_EMENU_CHECKED;

        if (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_SHOWCPUBELOW001))
        {
            if (WindowsVersion >= WINDOWS_7 && PhEnableCycleCpuUsage)
            {
                if (PhCsShowCpuBelow001)
                    menuItem->Flags |= PH_EMENU_CHECKED;
            }
            else
            {
                menuItem->Flags |= PH_EMENU_DISABLED;
            }
        }
    }
    else if (selectedIndex == ServicesTabIndex)
    {
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_HIDEDRIVERSERVICES, L"Hide Driver Services", NULL, NULL), StartIndex);

        if (DriverFilterEntry && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_HIDEDRIVERSERVICES)))
            menuItem->Flags |= PH_EMENU_CHECKED;
    }
    else if (selectedIndex == NetworkTabIndex)
    {
        // Remove the extra separator.
        PhRemoveEMenuItem(Menu, NULL, StartIndex);

        // Disabled for now - may cause user confusion.

        //PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS, L"Hide Processes From Other Users", NULL, NULL), StartIndex);
        //PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_VIEW_HIDESIGNEDPROCESSES, L"Hide Signed Processes", NULL, NULL), StartIndex + 1);

        //if (CurrentUserFilterEntry && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS)))
        //    menuItem->Flags |= PH_EMENU_CHECKED;
        //if (SignedFilterEntry && (menuItem = PhFindEMenuItem(Menu, 0, NULL, ID_VIEW_HIDESIGNEDPROCESSES)))
        //    menuItem->Flags |= PH_EMENU_CHECKED;
    }
    else if (AdditionalTabPageList)
    {
        ULONG i;

        for (i = 0; i < AdditionalTabPageList->Count; i++)
        {
            PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

            if (selectedIndex == tabPage->Index)
            {
                if (tabPage->InitializeSectionMenuItemsCallback)
                {
                    tabPage->InitializeSectionMenuItemsCallback(Menu, UlongToPtr(StartIndex), NULL, tabPage->Context);
                }
                else
                {
                    // Remove the extra separator.
                    PhRemoveEMenuItem(Menu, NULL, StartIndex);
                }

                break;
            }
        }
    }
}

VOID PhMwpLayoutTabControl(
    _Inout_ HDWP *DeferHandle
    )
{
    RECT rect;
    INT selectedIndex;

    if (!LayoutPaddingValid)
    {
        PhMwpUpdateLayoutPadding();
        LayoutPaddingValid = TRUE;
    }

    GetClientRect(PhMainWndHandle, &rect);
    PhMwpApplyLayoutPadding(&rect, &LayoutPadding);
    TabCtrl_AdjustRect(TabControlHandle, FALSE, &rect);

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == ProcessesTabIndex)
    {
        *DeferHandle = DeferWindowPos(*DeferHandle, ProcessTreeListHandle, NULL,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else if (selectedIndex == ServicesTabIndex)
    {
        *DeferHandle = DeferWindowPos(*DeferHandle, ServiceTreeListHandle, NULL,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else if (selectedIndex == NetworkTabIndex)
    {
        *DeferHandle = DeferWindowPos(*DeferHandle, NetworkTreeListHandle, NULL,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else if (AdditionalTabPageList)
    {
        ULONG i;

        for (i = 0; i < AdditionalTabPageList->Count; i++)
        {
            PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

            if (selectedIndex == tabPage->Index)
            {
                // Create the tab page window if it doesn't exist.
                if (!tabPage->WindowHandle)
                {
                    tabPage->WindowHandle = tabPage->CreateFunction(tabPage->Context);
                    BringWindowToTop(tabPage->WindowHandle);

                    if (CurrentCustomFont && tabPage->FontChangedCallback)
                        tabPage->FontChangedCallback((PVOID)CurrentCustomFont, NULL, NULL, tabPage->Context);
                }

                if (tabPage->WindowHandle)
                {
                    *DeferHandle = DeferWindowPos(*DeferHandle, tabPage->WindowHandle, NULL,
                        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                        SWP_NOACTIVATE | SWP_NOZORDER);
                }

                break;
            }
        }
    }
}

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

VOID PhMwpSelectionChangedTabControl(
    _In_ ULONG OldIndex
    )
{
    INT selectedIndex;
    HDWP deferHandle;

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    deferHandle = BeginDeferWindowPos(1);
    PhMwpLayoutTabControl(&deferHandle);
    EndDeferWindowPos(deferHandle);

    // Built-in tabs

    if (selectedIndex == ServicesTabIndex)
        PhMwpNeedServiceTreeList();

    if (selectedIndex == NetworkTabIndex)
    {
        PhMwpNeedNetworkTreeList();

        PhSetEnabledProvider(&NetworkProviderRegistration, UpdateAutomatically);

        if (UpdateAutomatically || NetworkFirstTime)
        {
            PhBoostProvider(&NetworkProviderRegistration, NULL);
            NetworkFirstTime = FALSE;
        }
    }
    else
    {
        PhSetEnabledProvider(&NetworkProviderRegistration, FALSE);
    }

    ShowWindow(ProcessTreeListHandle, selectedIndex == ProcessesTabIndex ? SW_SHOW : SW_HIDE);
    ShowWindow(ServiceTreeListHandle, selectedIndex == ServicesTabIndex ? SW_SHOW : SW_HIDE);
    ShowWindow(NetworkTreeListHandle, selectedIndex == NetworkTabIndex ? SW_SHOW : SW_HIDE);

    // Additional tabs

    if (AdditionalTabPageList)
    {
        ULONG i;

        for (i = 0; i < AdditionalTabPageList->Count; i++)
        {
            PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

            if (tabPage->SelectionChangedCallback)
            {
                if (tabPage->Index == OldIndex)
                {
                    tabPage->SelectionChangedCallback((PVOID)FALSE, 0, 0, tabPage->Context);
                }
                else if (tabPage->Index == selectedIndex)
                {
                    tabPage->SelectionChangedCallback((PVOID)TRUE, 0, 0, tabPage->Context);
                }
            }

            ShowWindow(tabPage->WindowHandle, selectedIndex == tabPage->Index ? SW_SHOW : SW_HIDE);
        }
    }

    if (PhPluginsEnabled)
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowTabChanged), IntToPtr(selectedIndex));
}

PPH_ADDITIONAL_TAB_PAGE PhMwpAddTabPage(
    _In_ PPH_ADDITIONAL_TAB_PAGE TabPage
    )
{
    PPH_ADDITIONAL_TAB_PAGE newTabPage;
    HDWP deferHandle;

    if (!AdditionalTabPageList)
        AdditionalTabPageList = PhCreateList(2);

    newTabPage = PhAllocateCopy(TabPage, sizeof(PH_ADDITIONAL_TAB_PAGE));
    PhAddItemList(AdditionalTabPageList, newTabPage);

    newTabPage->Index = PhAddTabControlTab(TabControlHandle, MAXINT, newTabPage->Text);
    MaxTabIndex = newTabPage->Index;

    if (newTabPage->WindowHandle)
        BringWindowToTop(newTabPage->WindowHandle);

    // The tab control might need multiple lines, so we need to refresh the layout.
    deferHandle = BeginDeferWindowPos(1);
    PhMwpLayoutTabControl(&deferHandle);
    EndDeferWindowPos(deferHandle);

    return newTabPage;
}

VOID PhMwpSelectTabPage(
    _In_ ULONG Index
    )
{
    INT oldIndex;

    oldIndex = TabCtrl_GetCurSel(TabControlHandle);
    TabCtrl_SetCurSel(TabControlHandle, Index);
    PhMwpSelectionChangedTabControl(oldIndex);
}

INT PhMwpFindTabPageIndex(
    _In_ PWSTR Text
    )
{
    if (PhEqualStringZ(Text, L"Processes", TRUE))
    {
        return ProcessesTabIndex;
    }
    else if (PhEqualStringZ(Text, L"Services", TRUE))
    {
        return ServicesTabIndex;
    }
    else if (PhEqualStringZ(Text, L"Network", TRUE))
    {
        return NetworkTabIndex;
    }
    else if (AdditionalTabPageList)
    {
        ULONG i;

        for (i = 0; i < AdditionalTabPageList->Count; i++)
        {
            PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

            if (PhEqualStringZ(tabPage->Text, Text, TRUE))
                return tabPage->Index;
        }
    }

    return -1;
}

static int __cdecl IconProcessesCpuUsageCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_PROCESS_ITEM processItem1 = *(PPH_PROCESS_ITEM *)elem1;
    PPH_PROCESS_ITEM processItem2 = *(PPH_PROCESS_ITEM *)elem2;

    return -singlecmp(processItem1->CpuUsage, processItem2->CpuUsage);
}

static int __cdecl IconProcessesNameCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
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

    priorityMenu = PhCreateEMenuItem(0, 0, L"Priority", NULL, ProcessId);

    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_REALTIME, L"Real Time", NULL, ProcessId), -1);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_HIGH, L"High", NULL, ProcessId), -1);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_ABOVENORMAL, L"Above Normal", NULL, ProcessId), -1);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_NORMAL, L"Normal", NULL, ProcessId), -1);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_BELOWNORMAL, L"Below Normal", NULL, ProcessId), -1);
    PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_IDLE, L"Idle", NULL, ProcessId), -1);

    // I/O Priority

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        ioPriorityMenu = PhCreateEMenuItem(0, 0, L"I/O Priority", NULL, ProcessId);

        PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_I_3, L"High", NULL, ProcessId), -1);
        PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_I_2, L"Normal", NULL, ProcessId), -1);
        PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_I_1, L"Low", NULL, ProcessId), -1);
        PhInsertEMenuItem(ioPriorityMenu, PhCreateEMenuItem(0, ID_I_0, L"Very Low", NULL, ProcessId), -1);
    }

    // Menu

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_TERMINATE, L"T&erminate", NULL, ProcessId), -1);

    if (processItem = PhReferenceProcessItem(ProcessId))
    {
        isSuspended = (BOOLEAN)processItem->IsSuspended;
        isPartiallySuspended = (BOOLEAN)processItem->IsPartiallySuspended;
        PhDereferenceObject(processItem);
    }

    if (!isSuspended)
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_SUSPEND, L"&Suspend", NULL, ProcessId), -1);
    if (isPartiallySuspended)
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_RESUME, L"Res&ume", NULL, ProcessId), -1);

    PhInsertEMenuItem(Menu, priorityMenu, -1);

    if (ioPriorityMenu)
        PhInsertEMenuItem(Menu, ioPriorityMenu, -1);

    PhMwpSetProcessMenuPriorityChecks(Menu, ProcessId, TRUE, TRUE, FALSE);

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_PROCESS_PROPERTIES, L"P&roperties", NULL, ProcessId), -1);
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
                case ID_PROCESS_PROPERTIES:
                    ProcessHacker_ShowProcessProperties(PhMainWndHandle, processItem);
                    break;
                }

                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowError(PhMainWndHandle, L"The process does not exist.");
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
                PhShowError(PhMainWndHandle, L"The process does not exist.");
            }
        }
        break;
    case ID_I_3:
    case ID_I_2:
    case ID_I_1:
    case ID_I_0:
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
                PhShowError(PhMainWndHandle, L"The process does not exist.");
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
            !processItem->UserName ||
            (PhCurrentUserName && !PhEqualString(processItem->UserName, PhCurrentUserName, TRUE))
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
        HBITMAP iconBitmap;
        CLIENT_ID clientId;
        PPH_STRING clientIdName;
        PPH_STRING escapedName;

        processItem = processList->Items[i];

        // Process

        clientId.UniqueProcess = processItem->ProcessId;
        clientId.UniqueThread = NULL;

        clientIdName = PhGetClientIdName(&clientId);
        escapedName = PhEscapeStringForMenuPrefix(&clientIdName->sr);
        PhDereferenceObject(clientIdName);
        PhAutoDereferenceObject(escapedName);

        subMenu = PhCreateEMenuItem(
            0,
            0,
            escapedName->Buffer,
            NULL,
            processItem->ProcessId
            );

        if (processItem->SmallIcon)
        {
            iconBitmap = PhIconToBitmap(processItem->SmallIcon, PhSmallIconSize.X, PhSmallIconSize.Y);
        }
        else
        {
            HICON icon;

            PhGetStockApplicationIcon(&icon, NULL);
            iconBitmap = PhIconToBitmap(icon, PhSmallIconSize.X, PhSmallIconSize.Y);
        }

        subMenu->Bitmap = iconBitmap;
        subMenu->Flags |= PH_EMENU_BITMAP_OWNED; // automatically destroy the bitmap when necessary

        PhAddMiniProcessMenuItems(subMenu, processItem->ProcessId);
        PhInsertEMenuItem(Menu, subMenu, -1);
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
    ULONG id;
    ULONG i;

    // This function seems to be called recursively under some circumstances.
    // To reproduce:
    // 1. Hold right mouse button on tray icon, then left click.
    // 2. Make the menu disappear by clicking on the menu then clicking somewhere else.
    // So, don't store any global state or bad things will happen.

    menu = PhCreateEMenu();
    PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_ICON), 0);

    // Check the Notifications menu items.
    for (i = PH_NOTIFY_MINIMUM; i != PH_NOTIFY_MAXIMUM; i <<= 1)
    {
        if (NotifyIconNotifyMask & i)
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
            }

            PhSetFlagsEMenuItem(menu, id, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }
    }

    // Add processes to the menu.

    numberOfProcesses = PhGetIntegerSetting(L"IconProcesses");
    item = PhFindEMenuItem(menu, 0, L"Processes", 0);

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
            handled = PhMwpExecuteComputerCommand(item->Id);

        if (!handled)
        {
            switch (item->Id)
            {
            case ID_ICON_SHOWHIDEPROCESSHACKER:
                SendMessage(PhMainWndHandle, WM_PH_TOGGLE_VISIBLE, 0, 0);
                break;
            case ID_ICON_SYSTEMINFORMATION:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_VIEW_SYSTEMINFORMATION, 0);
                break;
            case ID_NOTIFICATIONS_ENABLEALL:
                NotifyIconNotifyMask |= PH_NOTIFY_VALID_MASK;
                break;
            case ID_NOTIFICATIONS_DISABLEALL:
                NotifyIconNotifyMask &= ~PH_NOTIFY_VALID_MASK;
                break;
            case ID_NOTIFICATIONS_NEWPROCESSES:
            case ID_NOTIFICATIONS_TERMINATEDPROCESSES:
            case ID_NOTIFICATIONS_NEWSERVICES:
            case ID_NOTIFICATIONS_STARTEDSERVICES:
            case ID_NOTIFICATIONS_STOPPEDSERVICES:
            case ID_NOTIFICATIONS_DELETEDSERVICES:
                {
                    ULONG bit;

                    switch (item->Id)
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
                    }

                    NotifyIconNotifyMask ^= bit;
                }
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
    _In_ PWSTR Text,
    _In_ ULONG Flags
    )
{
    PhNfShowBalloonTip(0, Title, Text, 10, Flags);
}

VOID PhShowDetailsForIconNotification(
    VOID
    )
{
    switch (LastNotificationType)
    {
    case PH_NOTIFY_PROCESS_CREATE:
        {
            PPH_PROCESS_NODE processNode;

            if (processNode = PhFindProcessNode(LastNotificationDetails.ProcessId))
            {
                ProcessHacker_SelectTabPage(PhMainWndHandle, ProcessesTabIndex);
                ProcessHacker_SelectProcessNode(PhMainWndHandle, processNode);
                ProcessHacker_ToggleVisible(PhMainWndHandle, TRUE);
            }
        }
        break;
    case PH_NOTIFY_SERVICE_CREATE:
    case PH_NOTIFY_SERVICE_START:
    case PH_NOTIFY_SERVICE_STOP:
        {
            PPH_SERVICE_ITEM serviceItem;

            if (LastNotificationDetails.ServiceName &&
                (serviceItem = PhReferenceServiceItem(LastNotificationDetails.ServiceName->Buffer)))
            {
                ProcessHacker_SelectTabPage(PhMainWndHandle, ServicesTabIndex);
                ProcessHacker_SelectServiceItem(PhMainWndHandle, serviceItem);
                ProcessHacker_ToggleVisible(PhMainWndHandle, TRUE);

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
    if (LastNotificationType &
        (PH_NOTIFY_SERVICE_CREATE | PH_NOTIFY_SERVICE_DELETE | PH_NOTIFY_SERVICE_START | PH_NOTIFY_SERVICE_STOP))
    {
        PhClearReference(&LastNotificationDetails.ServiceName);
    }

    LastNotificationType = 0;
    memset(&LastNotificationDetails, 0, sizeof(LastNotificationDetails));
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

VOID PhMwpShowProcessProperties(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_PROPCONTEXT propContext;

    propContext = PhCreateProcessPropContext(
        PhMainWndHandle,
        ProcessItem
        );

    if (propContext)
    {
        PhShowProcessProperties(propContext);
        PhDereferenceObject(propContext);
    }
}

BOOLEAN PhMwpCurrentUserProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (!processNode->ProcessItem->UserName)
        return FALSE;

    if (!PhCurrentUserName)
        return FALSE;

    if (!PhEqualString(processNode->ProcessItem->UserName, PhCurrentUserName, TRUE))
        return FALSE;

    return TRUE;
}

BOOLEAN PhMwpSignedProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (processNode->ProcessItem->VerifyResult == VrTrusted)
        return FALSE;

    return TRUE;
}

BOOLEAN PhMwpExecuteProcessPriorityCommand(
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    ULONG priorityClass;

    switch (Id)
    {
    case ID_PRIORITY_REALTIME:
        priorityClass = PROCESS_PRIORITY_CLASS_REALTIME;
        break;
    case ID_PRIORITY_HIGH:
        priorityClass = PROCESS_PRIORITY_CLASS_HIGH;
        break;
    case ID_PRIORITY_ABOVENORMAL:
        priorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
        break;
    case ID_PRIORITY_NORMAL:
        priorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
        break;
    case ID_PRIORITY_BELOWNORMAL:
        priorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
        break;
    case ID_PRIORITY_IDLE:
        priorityClass = PROCESS_PRIORITY_CLASS_IDLE;
        break;
    default:
        return FALSE;
    }

    PhUiSetPriorityProcesses(PhMainWndHandle, Processes, NumberOfProcesses, priorityClass);

    return TRUE;
}

BOOLEAN PhMwpExecuteProcessIoPriorityCommand(
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    ULONG ioPriority;

    switch (Id)
    {
    case ID_I_0:
        ioPriority = 0;
        break;
    case ID_I_1:
        ioPriority = 1;
        break;
    case ID_I_2:
        ioPriority = 2;
        break;
    case ID_I_3:
        ioPriority = 3;
        break;
    default:
        return FALSE;
    }

    PhUiSetIoPriorityProcesses(PhMainWndHandle, Processes, NumberOfProcesses, ioPriority);

    return TRUE;
}

VOID PhMwpSetProcessMenuPriorityChecks(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN SetPriority,
    _In_ BOOLEAN SetIoPriority,
    _In_ BOOLEAN SetPagePriority
    )
{
    HANDLE processHandle;
    PROCESS_PRIORITY_CLASS priorityClass = { 0 };
    ULONG ioPriority = -1;
    ULONG pagePriority = -1;
    ULONG id = 0;

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        ProcessQueryAccess,
        ProcessId
        )))
    {
        if (SetPriority)
        {
            NtQueryInformationProcess(processHandle, ProcessPriorityClass, &priorityClass, sizeof(PROCESS_PRIORITY_CLASS), NULL);
        }

        if (SetIoPriority && WindowsVersion >= WINDOWS_VISTA)
        {
            if (!NT_SUCCESS(PhGetProcessIoPriority(
                processHandle,
                &ioPriority
                )))
            {
                ioPriority = -1;
            }
        }

        if (SetPagePriority && WindowsVersion >= WINDOWS_VISTA)
        {
            if (!NT_SUCCESS(PhGetProcessPagePriority(
                processHandle,
                &pagePriority
                )))
            {
                pagePriority = -1;
            }
        }

        NtClose(processHandle);
    }

    if (SetPriority)
    {
        switch (priorityClass.PriorityClass)
        {
        case PROCESS_PRIORITY_CLASS_REALTIME:
            id = ID_PRIORITY_REALTIME;
            break;
        case PROCESS_PRIORITY_CLASS_HIGH:
            id = ID_PRIORITY_HIGH;
            break;
        case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
            id = ID_PRIORITY_ABOVENORMAL;
            break;
        case PROCESS_PRIORITY_CLASS_NORMAL:
            id = ID_PRIORITY_NORMAL;
            break;
        case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
            id = ID_PRIORITY_BELOWNORMAL;
            break;
        case PROCESS_PRIORITY_CLASS_IDLE:
            id = ID_PRIORITY_IDLE;
            break;
        }

        if (id != 0)
        {
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
        }
    }

    if (SetIoPriority && ioPriority != -1)
    {
        id = 0;

        switch (ioPriority)
        {
        case 0:
            id = ID_I_0;
            break;
        case 1:
            id = ID_I_1;
            break;
        case 2:
            id = ID_I_2;
            break;
        case 3:
            id = ID_I_3;
            break;
        }

        if (id != 0)
        {
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
        }
    }

    if (SetPagePriority && pagePriority != -1)
    {
        id = 0;

        switch (pagePriority)
        {
        case 1:
            id = ID_PAGEPRIORITY_1;
            break;
        case 2:
            id = ID_PAGEPRIORITY_2;
            break;
        case 3:
            id = ID_PAGEPRIORITY_3;
            break;
        case 4:
            id = ID_PAGEPRIORITY_4;
            break;
        case 5:
            id = ID_PAGEPRIORITY_5;
            break;
        }

        if (id != 0)
        {
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
        }
    }
}

VOID PhMwpInitializeProcessMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfProcesses == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfProcesses == 1)
    {
        // All menu items are enabled by default.

        // If the user selected a fake process, disable all but
        // a few menu items.
        if (PH_IS_FAKE_PROCESS_ID(Processes[0]->ProcessId))
        {
            PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
            PhEnableEMenuItem(Menu, ID_PROCESS_PROPERTIES, TRUE);
            PhEnableEMenuItem(Menu, ID_PROCESS_SEARCHONLINE, TRUE);
        }
    }
    else
    {
        ULONG menuItemsMultiEnabled[] =
        {
            ID_PROCESS_TERMINATE,
            ID_PROCESS_SUSPEND,
            ID_PROCESS_RESUME,
            ID_MISCELLANEOUS_REDUCEWORKINGSET,
            ID_PROCESS_COPY
        };
        ULONG i;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        // Enable the Miscellaneous menu item but disable its children.
        if (item = PhFindEMenuItem(Menu, 0, L"Miscellaneous", 0))
        {
            item->Flags &= ~PH_EMENU_DISABLED;
            PhSetFlagsAllEMenuItems(item, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        }

        // Enable the Priority menu item.
        if (item = PhFindEMenuItem(Menu, 0, L"Priority", 0))
            item->Flags &= ~PH_EMENU_DISABLED;

        // Enable the I/O Priority menu item.
        if (item = PhFindEMenuItem(Menu, 0, L"I/O Priority", 0))
            item->Flags &= ~PH_EMENU_DISABLED;

        // These menu items are capable of manipulating
        // multiple processes.
        for (i = 0; i < sizeof(menuItemsMultiEnabled) / sizeof(ULONG); i++)
        {
            PhSetFlagsEMenuItem(Menu, menuItemsMultiEnabled[i], PH_EMENU_DISABLED, 0);
        }
    }

    // Remove irrelevant menu items.
    if (WindowsVersion < WINDOWS_VISTA)
    {
        // Remove I/O priority.
        if (item = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, L"I/O Priority", 0))
            PhDestroyEMenuItem(item);
        // Remove page priority.
        if (item = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, L"Page Priority", 0))
            PhDestroyEMenuItem(item);
    }

    // Suspend/Resume
    if (NumberOfProcesses == 1)
    {
        if (Processes[0]->IsSuspended)
        {
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_SUSPEND))
                PhDestroyEMenuItem(item);
        }

        if (!Processes[0]->IsPartiallySuspended)
        {
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_RESUME))
                PhDestroyEMenuItem(item);
        }
    }

    // Virtualization
    if (NumberOfProcesses == 1)
    {
        HANDLE processHandle;
        HANDLE tokenHandle;
        BOOLEAN allowed = FALSE;
        BOOLEAN enabled = FALSE;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            ProcessQueryAccess,
            Processes[0]->ProcessId
            )))
        {
            if (NT_SUCCESS(PhOpenProcessToken(
                &tokenHandle,
                TOKEN_QUERY,
                processHandle
                )))
            {
                PhGetTokenIsVirtualizationAllowed(tokenHandle, &allowed);
                PhGetTokenIsVirtualizationEnabled(tokenHandle, &enabled);
                SelectedProcessVirtualizationEnabled = enabled;

                NtClose(tokenHandle);
            }

            NtClose(processHandle);
        }

        if (!allowed)
            PhSetFlagsEMenuItem(Menu, ID_PROCESS_VIRTUALIZATION, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        else if (enabled)
            PhSetFlagsEMenuItem(Menu, ID_PROCESS_VIRTUALIZATION, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    // Priority
    if (NumberOfProcesses == 1)
    {
        PhMwpSetProcessMenuPriorityChecks(Menu, Processes[0]->ProcessId, TRUE, TRUE, TRUE);
    }

    item = PhFindEMenuItem(Menu, 0, L"Window", 0);

    if (item)
    {
        // Window menu
        if (NumberOfProcesses == 1)
        {
            WINDOWPLACEMENT placement = { sizeof(placement) };

            // Get a handle to the process' top-level window (if any).
            SelectedProcessWindowHandle = PhGetProcessMainWindow(Processes[0]->ProcessId, Processes[0]->QueryHandle);

            if (!SelectedProcessWindowHandle)
                item->Flags |= PH_EMENU_DISABLED;

            GetWindowPlacement(SelectedProcessWindowHandle, &placement);

            if (placement.showCmd == SW_MINIMIZE)
                PhEnableEMenuItem(item, ID_WINDOW_MINIMIZE, FALSE);
            else if (placement.showCmd == SW_MAXIMIZE)
                PhEnableEMenuItem(item, ID_WINDOW_MAXIMIZE, FALSE);
            else if (placement.showCmd == SW_NORMAL)
                PhEnableEMenuItem(item, ID_WINDOW_RESTORE, FALSE);
        }
        else
        {
            item->Flags |= PH_EMENU_DISABLED;
        }
    }

    // Remove irrelevant menu items (continued)
    if (!WINDOWS_HAS_UAC)
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_VIRTUALIZATION))
            PhDestroyEMenuItem(item);
    }
}

VOID PhShowProcessContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PH_PLUGIN_MENU_INFORMATION menuInfo;
    PPH_PROCESS_ITEM *processes;
    ULONG numberOfProcesses;

    PhGetSelectedProcessItems(&processes, &numberOfProcesses);

    if (numberOfProcesses != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_PROCESS), 0);
        PhSetFlagsEMenuItem(menu, ID_PROCESS_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhMwpInitializeProcessMenu(menu, processes, numberOfProcesses);
        PhInsertCopyCellEMenuItem(menu, ID_PROCESS_COPY, ProcessTreeListHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, PhMainWndHandle, 0);
            menuInfo.u.Process.Processes = processes;
            menuInfo.u.Process.NumberOfProcesses = numberOfProcesses;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(processes);
}

VOID PhMwpOnProcessAdded(
    _In_ _Assume_refs_(1) PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG RunId
    )
{
    PPH_PROCESS_NODE processNode;

    if (!ProcessesNeedsRedraw)
    {
        TreeNew_SetRedraw(ProcessTreeListHandle, FALSE);
        ProcessesNeedsRedraw = TRUE;
    }

    processNode = PhAddProcessNode(ProcessItem, RunId);

    if (RunId != 1)
    {
        PPH_PROCESS_ITEM parentProcess;
        HANDLE parentProcessId = NULL;
        PPH_STRING parentName = NULL;

        if (parentProcess = PhReferenceProcessItemForParent(
            ProcessItem->ParentProcessId,
            ProcessItem->ProcessId,
            &ProcessItem->CreateTime
            ))
        {
            parentProcessId = parentProcess->ProcessId;
            parentName = parentProcess->ProcessName;
        }

        PhLogProcessEntry(
            PH_LOG_ENTRY_PROCESS_CREATE,
            ProcessItem->ProcessId,
            NULL,
            ProcessItem->ProcessName,
            parentProcessId,
            parentName
            );

        if (NotifyIconNotifyMask & PH_NOTIFY_PROCESS_CREATE)
        {
            if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_PROCESS_CREATE, ProcessItem))
            {
                PhMwpClearLastNotificationDetails();
                LastNotificationType = PH_NOTIFY_PROCESS_CREATE;
                LastNotificationDetails.ProcessId = ProcessItem->ProcessId;

                PhShowIconNotification(L"Process Created", PhaFormatString(
                    L"The process %s (%u) was created by %s (%u)",
                    ProcessItem->ProcessName->Buffer,
                    HandleToUlong(ProcessItem->ProcessId),
                    PhGetStringOrDefault(parentName, L"Unknown Process"),
                    HandleToUlong(ProcessItem->ParentProcessId)
                    )->Buffer, NIIF_INFO);
            }
        }

        if (parentProcess)
            PhDereferenceObject(parentProcess);

        if (PhCsScrollToNewProcesses)
            ProcessToScrollTo = processNode;
    }
    else
    {
        if (NeedsSelectPid != 0)
        {
            if (processNode->ProcessId == UlongToHandle(NeedsSelectPid))
                ProcessToScrollTo = processNode;
        }
    }

    // PhCreateProcessNode has its own reference.
    PhDereferenceObject(ProcessItem);
}

VOID PhMwpOnProcessModified(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhUpdateProcessNode(PhFindProcessNode(ProcessItem->ProcessId));

    if (SignedFilterEntry)
        PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
}

VOID PhMwpOnProcessRemoved(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_NODE processNode;

    if (!ProcessesNeedsRedraw)
    {
        TreeNew_SetRedraw(ProcessTreeListHandle, FALSE);
        ProcessesNeedsRedraw = TRUE;
    }

    PhLogProcessEntry(PH_LOG_ENTRY_PROCESS_DELETE, ProcessItem->ProcessId, ProcessItem->QueryHandle, ProcessItem->ProcessName, NULL, NULL);

    if (NotifyIconNotifyMask & PH_NOTIFY_PROCESS_DELETE)
    {
        if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_PROCESS_DELETE, ProcessItem))
        {
            PhMwpClearLastNotificationDetails();
            LastNotificationType = PH_NOTIFY_PROCESS_DELETE;
            LastNotificationDetails.ProcessId = ProcessItem->ProcessId;

            PhShowIconNotification(L"Process Terminated", PhaFormatString(
                L"The process %s (%u) was terminated.",
                ProcessItem->ProcessName->Buffer,
                HandleToUlong(ProcessItem->ProcessId)
                )->Buffer, NIIF_INFO);
        }
    }

    processNode = PhFindProcessNode(ProcessItem->ProcessId);
    PhRemoveProcessNode(processNode);

    if (ProcessToScrollTo == processNode) // shouldn't happen, but just in case
        ProcessToScrollTo = NULL;
}

VOID PhMwpOnProcessesUpdated(
    VOID
    )
{
    // The modified notification is only sent for special cases.
    // We have to invalidate the text on each update.
    PhTickProcessNodes();

    if (PhPluginsEnabled)
    {
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessesUpdated), NULL);
    }

    if (ProcessesNeedsRedraw)
    {
        TreeNew_SetRedraw(ProcessTreeListHandle, TRUE);
        ProcessesNeedsRedraw = FALSE;
    }

    if (NeedsSelectPid != 0)
    {
        if (ProcessToScrollTo)
        {
            PhSelectAndEnsureVisibleProcessNode(ProcessToScrollTo);
            ProcessToScrollTo = NULL;
        }

        NeedsSelectPid = 0;
    }

    if (ProcessToScrollTo)
    {
        TreeNew_EnsureVisible(ProcessTreeListHandle, &ProcessToScrollTo->Node);
        ProcessToScrollTo = NULL;
    }
}

VOID PhMwpNeedServiceTreeList(
    VOID
    )
{
    if (!ServiceTreeListLoaded)
    {
        ServiceTreeListLoaded = TRUE;

        PhLoadSettingsServiceTreeList();

        if (PhGetIntegerSetting(L"HideDriverServices"))
        {
            DriverFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), PhMwpDriverServiceTreeFilter, NULL);
        }

        if (ServicesPendingList)
        {
            PPH_SERVICE_ITEM serviceItem;
            ULONG enumerationKey = 0;

            while (PhEnumPointerList(ServicesPendingList, &enumerationKey, (PVOID *)&serviceItem))
            {
                PhMwpOnServiceAdded(serviceItem, 1);
            }

            // Force a re-draw.
            PhMwpOnServicesUpdated();

            PhClearReference(&ServicesPendingList);
        }
    }
}

BOOLEAN PhMwpDriverServiceTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    if (serviceNode->ServiceItem->Type & SERVICE_DRIVER)
        return FALSE;

    return TRUE;
}

VOID PhMwpInitializeServiceMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_SERVICE_ITEM *Services,
    _In_ ULONG NumberOfServices
    )
{
    if (NumberOfServices == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfServices == 1)
    {
        if (!Services[0]->ProcessId)
            PhEnableEMenuItem(Menu, ID_SERVICE_GOTOPROCESS, FALSE);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_SERVICE_COPY, TRUE);
    }

    if (NumberOfServices == 1)
    {
        switch (Services[0]->State)
        {
        case SERVICE_RUNNING:
            {
                PhEnableEMenuItem(Menu, ID_SERVICE_START, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_CONTINUE, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_PAUSE,
                    Services[0]->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
                PhEnableEMenuItem(Menu, ID_SERVICE_STOP,
                    Services[0]->ControlsAccepted & SERVICE_ACCEPT_STOP);
            }
            break;
        case SERVICE_PAUSED:
            {
                PhEnableEMenuItem(Menu, ID_SERVICE_START, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_CONTINUE,
                    Services[0]->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
                PhEnableEMenuItem(Menu, ID_SERVICE_PAUSE, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_STOP,
                    Services[0]->ControlsAccepted & SERVICE_ACCEPT_STOP);
            }
            break;
        case SERVICE_STOPPED:
            {
                PhEnableEMenuItem(Menu, ID_SERVICE_CONTINUE, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_PAUSE, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_STOP, FALSE);
            }
            break;
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_STOP_PENDING:
            {
                PhEnableEMenuItem(Menu, ID_SERVICE_START, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_CONTINUE, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_PAUSE, FALSE);
                PhEnableEMenuItem(Menu, ID_SERVICE_STOP, FALSE);
            }
            break;
        }

        if (!(Services[0]->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE))
        {
            PPH_EMENU_ITEM item;

            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_SERVICE_CONTINUE))
                PhDestroyEMenuItem(item);
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_SERVICE_PAUSE))
                PhDestroyEMenuItem(item);
        }
    }
}

VOID PhShowServiceContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PH_PLUGIN_MENU_INFORMATION menuInfo;
    PPH_SERVICE_ITEM *services;
    ULONG numberOfServices;

    PhGetSelectedServiceItems(&services, &numberOfServices);

    if (numberOfServices != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_SERVICE), 0);
        PhSetFlagsEMenuItem(menu, ID_SERVICE_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhMwpInitializeServiceMenu(menu, services, numberOfServices);
        PhInsertCopyCellEMenuItem(menu, ID_SERVICE_COPY, ServiceTreeListHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, PhMainWndHandle, 0);
            menuInfo.u.Service.Services = services;
            menuInfo.u.Service.NumberOfServices = numberOfServices;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(services);
}

VOID PhMwpOnServiceAdded(
    _In_ _Assume_refs_(1) PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG RunId
    )
{
    PPH_SERVICE_NODE serviceNode;

    if (ServiceTreeListLoaded)
    {
        if (!ServicesNeedsRedraw)
        {
            TreeNew_SetRedraw(ServiceTreeListHandle, FALSE);
            ServicesNeedsRedraw = TRUE;
        }

        serviceNode = PhAddServiceNode(ServiceItem, RunId);
        // ServiceItem dereferenced below
    }
    else
    {
        if (!ServicesPendingList)
            ServicesPendingList = PhCreatePointerList(100);

        PhAddItemPointerList(ServicesPendingList, ServiceItem);
    }

    if (RunId != 1)
    {
        PhLogServiceEntry(PH_LOG_ENTRY_SERVICE_CREATE, ServiceItem->Name, ServiceItem->DisplayName);

        if (NotifyIconNotifyMask & PH_NOTIFY_SERVICE_CREATE)
        {
            if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_SERVICE_CREATE, ServiceItem))
            {
                PhMwpClearLastNotificationDetails();
                LastNotificationType = PH_NOTIFY_SERVICE_CREATE;
                PhSwapReference(&LastNotificationDetails.ServiceName, ServiceItem->Name);

                PhShowIconNotification(L"Service Created", PhaFormatString(
                    L"The service %s (%s) has been created.",
                    ServiceItem->Name->Buffer,
                    ServiceItem->DisplayName->Buffer
                    )->Buffer, NIIF_INFO);
            }
        }
    }

    if (ServiceTreeListLoaded)
        PhDereferenceObject(ServiceItem);
}

VOID PhMwpOnServiceModified(
    _In_ PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    )
{
    PH_SERVICE_CHANGE serviceChange;
    UCHAR logEntryType;

    if (ServiceTreeListLoaded)
    {
        //if (!ServicesNeedsRedraw)
        //{
        //    TreeNew_SetRedraw(ServiceTreeListHandle, FALSE);
        //    ServicesNeedsRedraw = TRUE;
        //}

        PhUpdateServiceNode(PhFindServiceNode(ServiceModifiedData->Service));

        if (DriverFilterEntry)
            PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
    }

    serviceChange = PhGetServiceChange(ServiceModifiedData);

    switch (serviceChange)
    {
    case ServiceStarted:
        logEntryType = PH_LOG_ENTRY_SERVICE_START;
        break;
    case ServiceStopped:
        logEntryType = PH_LOG_ENTRY_SERVICE_STOP;
        break;
    case ServiceContinued:
        logEntryType = PH_LOG_ENTRY_SERVICE_CONTINUE;
        break;
    case ServicePaused:
        logEntryType = PH_LOG_ENTRY_SERVICE_PAUSE;
        break;
    default:
        logEntryType = 0;
        break;
    }

    if (logEntryType != 0)
        PhLogServiceEntry(logEntryType, ServiceModifiedData->Service->Name, ServiceModifiedData->Service->DisplayName);

    if (NotifyIconNotifyMask & (PH_NOTIFY_SERVICE_START | PH_NOTIFY_SERVICE_STOP))
    {
        PPH_SERVICE_ITEM serviceItem;

        serviceItem = ServiceModifiedData->Service;

        if (serviceChange == ServiceStarted && (NotifyIconNotifyMask & PH_NOTIFY_SERVICE_START))
        {
            if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_SERVICE_START, serviceItem))
            {
                PhMwpClearLastNotificationDetails();
                LastNotificationType = PH_NOTIFY_SERVICE_START;
                PhSwapReference(&LastNotificationDetails.ServiceName, serviceItem->Name);

                PhShowIconNotification(L"Service Started", PhaFormatString(
                    L"The service %s (%s) has been started.",
                    serviceItem->Name->Buffer,
                    serviceItem->DisplayName->Buffer
                    )->Buffer, NIIF_INFO);
            }
        }
        else if (serviceChange == ServiceStopped && (NotifyIconNotifyMask & PH_NOTIFY_SERVICE_STOP))
        {
            PhMwpClearLastNotificationDetails();
            LastNotificationType = PH_NOTIFY_SERVICE_STOP;
            PhSwapReference(&LastNotificationDetails.ServiceName, serviceItem->Name);

            if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_SERVICE_STOP, serviceItem))
            {
                PhShowIconNotification(L"Service Stopped", PhaFormatString(
                    L"The service %s (%s) has been stopped.",
                    serviceItem->Name->Buffer,
                    serviceItem->DisplayName->Buffer
                    )->Buffer, NIIF_INFO);
            }
        }
    }
}

VOID PhMwpOnServiceRemoved(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    if (ServiceTreeListLoaded)
    {
        if (!ServicesNeedsRedraw)
        {
            TreeNew_SetRedraw(ServiceTreeListHandle, FALSE);
            ServicesNeedsRedraw = TRUE;
        }
    }

    PhLogServiceEntry(PH_LOG_ENTRY_SERVICE_DELETE, ServiceItem->Name, ServiceItem->DisplayName);

    if (NotifyIconNotifyMask & PH_NOTIFY_SERVICE_CREATE)
    {
        if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_SERVICE_DELETE, ServiceItem))
        {
            PhMwpClearLastNotificationDetails();
            LastNotificationType = PH_NOTIFY_SERVICE_DELETE;
            PhSwapReference(&LastNotificationDetails.ServiceName, ServiceItem->Name);

            PhShowIconNotification(L"Service Deleted", PhaFormatString(
                L"The service %s (%s) has been deleted.",
                ServiceItem->Name->Buffer,
                ServiceItem->DisplayName->Buffer
                )->Buffer, NIIF_INFO);
        }
    }

    if (ServiceTreeListLoaded)
    {
        PhRemoveServiceNode(PhFindServiceNode(ServiceItem));
    }
    else
    {
        if (ServicesPendingList)
        {
            HANDLE pointerHandle;

            // Remove the service from the pending list so we don't try to add it
            // later.

            if (pointerHandle = PhFindItemPointerList(ServicesPendingList, ServiceItem))
                PhRemoveItemPointerList(ServicesPendingList, pointerHandle);

            PhDereferenceObject(ServiceItem);
        }
    }
}

VOID PhMwpOnServicesUpdated(
    VOID
    )
{
    if (ServiceTreeListLoaded)
    {
        PhTickServiceNodes();

        if (ServicesNeedsRedraw)
        {
            TreeNew_SetRedraw(ServiceTreeListHandle, TRUE);
            ServicesNeedsRedraw = FALSE;
        }
    }
}

VOID PhMwpNeedNetworkTreeList(
    VOID
    )
{
    if (!NetworkTreeListLoaded)
    {
        NetworkTreeListLoaded = TRUE;

        PhLoadSettingsNetworkTreeList();
    }
}

BOOLEAN PhMwpCurrentUserNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;
    PPH_PROCESS_NODE processNode;

    processNode = PhFindProcessNode(networkNode->NetworkItem->ProcessId);

    if (processNode)
        return PhMwpCurrentUserProcessTreeFilter(&processNode->Node, NULL);

    return TRUE;
}

BOOLEAN PhMwpSignedNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;
    PPH_PROCESS_NODE processNode;

    processNode = PhFindProcessNode(networkNode->NetworkItem->ProcessId);

    if (processNode)
        return PhMwpSignedProcessTreeFilter(&processNode->Node, NULL);

    return TRUE;
}

VOID PhMwpInitializeNetworkMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_NETWORK_ITEM *NetworkItems,
    _In_ ULONG NumberOfNetworkItems
    )
{
    ULONG i;
    PPH_EMENU_ITEM item;

    if (NumberOfNetworkItems == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfNetworkItems == 1)
    {
        if (!NetworkItems[0]->ProcessId)
            PhEnableEMenuItem(Menu, ID_NETWORK_GOTOPROCESS, FALSE);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_NETWORK_CLOSE, TRUE);
        PhEnableEMenuItem(Menu, ID_NETWORK_COPY, TRUE);
    }

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_NETWORK_VIEWSTACK))
            PhDestroyEMenuItem(item);
    }

    // Go to Service
    if (NumberOfNetworkItems != 1 || !NetworkItems[0]->OwnerName)
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_NETWORK_GOTOSERVICE))
            PhDestroyEMenuItem(item);
    }

    // Close
    if (NumberOfNetworkItems != 0)
    {
        BOOLEAN closeOk = TRUE;

        for (i = 0; i < NumberOfNetworkItems; i++)
        {
            if (
                NetworkItems[i]->ProtocolType != PH_TCP4_NETWORK_PROTOCOL ||
                NetworkItems[i]->State != MIB_TCP_STATE_ESTAB
                )
            {
                closeOk = FALSE;
                break;
            }
        }

        if (!closeOk)
            PhEnableEMenuItem(Menu, ID_NETWORK_CLOSE, FALSE);
    }
}

VOID PhShowNetworkContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PH_PLUGIN_MENU_INFORMATION menuInfo;
    PPH_NETWORK_ITEM *networkItems;
    ULONG numberOfNetworkItems;

    PhGetSelectedNetworkItems(&networkItems, &numberOfNetworkItems);

    if (numberOfNetworkItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_NETWORK), 0);
        PhSetFlagsEMenuItem(menu, ID_NETWORK_GOTOPROCESS, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhMwpInitializeNetworkMenu(menu, networkItems, numberOfNetworkItems);
        PhInsertCopyCellEMenuItem(menu, ID_NETWORK_COPY, NetworkTreeListHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, PhMainWndHandle, 0);
            menuInfo.u.Network.NetworkItems = networkItems;
            menuInfo.u.Network.NumberOfNetworkItems = numberOfNetworkItems;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(networkItems);
}

VOID PhMwpOnNetworkItemAdded(
    _In_ ULONG RunId,
    _In_ _Assume_refs_(1) PPH_NETWORK_ITEM NetworkItem
    )
{
    PPH_NETWORK_NODE networkNode;

    if (!NetworkNeedsRedraw)
    {
        TreeNew_SetRedraw(NetworkTreeListHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    networkNode = PhAddNetworkNode(NetworkItem, RunId);
    PhDereferenceObject(NetworkItem);
}

VOID PhMwpOnNetworkItemModified(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PhUpdateNetworkNode(PhFindNetworkNode(NetworkItem));
}

VOID PhMwpOnNetworkItemRemoved(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    if (!NetworkNeedsRedraw)
    {
        TreeNew_SetRedraw(NetworkTreeListHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    PhRemoveNetworkNode(PhFindNetworkNode(NetworkItem));
}

VOID PhMwpOnNetworkItemsUpdated(
    VOID
    )
{
    PhTickNetworkNodes();

    if (NetworkNeedsRedraw)
    {
        TreeNew_SetRedraw(NetworkTreeListHandle, TRUE);
        NetworkNeedsRedraw = FALSE;
    }
}

VOID PhMwpUpdateUsersMenu(
    VOID
    )
{
    HMENU menu;
    PSESSIONIDW sessions;
    ULONG numberOfSessions;
    ULONG i;
    ULONG j;
    MENUITEMINFO menuItemInfo = { sizeof(MENUITEMINFO) };

    menu = SubMenuHandles[3];

    // Delete all items in the Users menu.
    while (DeleteMenu(menu, 0, MF_BYPOSITION)) ;

    if (WinStationEnumerateW(NULL, &sessions, &numberOfSessions))
    {
        for (i = 0; i < numberOfSessions; i++)
        {
            HMENU userMenu;
            PPH_STRING menuText;
            PPH_STRING escapedMenuText;
            WINSTATIONINFORMATION winStationInfo;
            ULONG returnLength;
            ULONG numberOfItems;

            if (!WinStationQueryInformationW(
                NULL,
                sessions[i].SessionId,
                WinStationInformation,
                &winStationInfo,
                sizeof(WINSTATIONINFORMATION),
                &returnLength
                ))
            {
                winStationInfo.Domain[0] = 0;
                winStationInfo.UserName[0] = 0;
            }

            if (winStationInfo.Domain[0] == 0 || winStationInfo.UserName[0] == 0)
            {
                // Probably the Services or RDP-Tcp session.
                continue;
            }

            menuText = PhFormatString(
                L"%u: %s\\%s",
                sessions[i].SessionId,
                winStationInfo.Domain,
                winStationInfo.UserName
                );
            escapedMenuText = PhEscapeStringForMenuPrefix(&menuText->sr);
            PhDereferenceObject(menuText);

            userMenu = GetSubMenu(LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_USER)), 0);
            AppendMenu(
                menu,
                MF_STRING | MF_POPUP,
                (UINT_PTR)userMenu,
                escapedMenuText->Buffer
                );

            PhDereferenceObject(escapedMenuText);

            menuItemInfo.fMask = MIIM_DATA;
            menuItemInfo.dwItemData = sessions[i].SessionId;

            numberOfItems = GetMenuItemCount(userMenu);

            if (numberOfItems != -1)
            {
                for (j = 0; j < numberOfItems; j++)
                    SetMenuItemInfo(userMenu, j, TRUE, &menuItemInfo);
            }
        }

        WinStationFreeMemory(sessions);
    }

    DrawMenuBar(PhMainWndHandle);
}

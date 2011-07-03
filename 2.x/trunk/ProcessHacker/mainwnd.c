/*
 * Process Hacker - 
 *   main window
 * 
 * Copyright (C) 2009-2011 wj32
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
#include <settings.h>
#include <cpysave.h>
#include <memsrch.h>
#include <symprv.h>
#include <emenu.h>
#include <phplug.h>
#include <windowsx.h>
#include <shlobj.h>
#include <winsta.h>
#include <iphlpapi.h>

#define RUNAS_MODE_ADMIN 1
#define RUNAS_MODE_LIMITED 2

typedef HRESULT (WINAPI *_LoadIconMetric)(
    __in HINSTANCE hinst,
    __in PCWSTR pszName,
    __in int lims,
    __out HICON *phico
    );

VOID PhMainWndOnCreate();

ULONG_PTR PhpMainWndAddMenuItem(
    __in PPH_PLUGIN Plugin,
    __in HMENU ParentMenu,
    __in_opt PWSTR InsertAfter,
    __in ULONG Flags,
    __in ULONG Id,
    __in PWSTR Text,
    __in_opt PVOID Context
    );

PPH_ADDITIONAL_TAB_PAGE PhpAddTabPage(
    __in PPH_ADDITIONAL_TAB_PAGE TabPage
    );

VOID PhMainWndOnLayout(
    __inout HDWP *deferHandle
    );

VOID PhMainWndTabControlOnLayout(
    __inout HDWP *deferHandle
    );

VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    );

VOID PhMainWndTabControlOnSelectionChanged();

VOID PhMainWndNetworkListViewOnNotify(
    __in LPNMHDR Header
    );

VOID PhMainWndNetworkListViewOnContextMenu(
    __in LPARAM lParam
    );

VOID PhReloadSysParameters();

VOID PhpInitialLoadSettings();

VOID PhMainSymInitHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

NTSTATUS PhpDelayedLoadFunction(
    __in PVOID Parameter
    );

VOID PhpSaveWindowState();

VOID PhpSaveAllSettings();

VOID PhpSetCheckOpacityMenu(
    __in BOOLEAN AssumeAllUnchecked,
    __in ULONG Opacity
    );

VOID PhpSetWindowOpacity(
    __in ULONG Opacity
    );

VOID PhpSelectTabPage(
    __in ULONG Index
    );

VOID PhpPrepareForEarlyShutdown();

VOID PhpCancelEarlyShutdown();

VOID PhpNeedServiceTreeList();

BOOLEAN PhpProcessComputerCommand(
    __in ULONG Id
    );

BOOLEAN PhpProcessProcessPriorityCommand(
    __in ULONG Id,
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpRefreshUsersMenu();

VOID PhpAddIconProcesses(
    __in PPH_EMENU_ITEM Menu,
    __in ULONG NumberOfProcesses
    );

VOID PhpShowIconContextMenu(
    __in POINT Location
    );

BOOLEAN PhpCurrentUserProcessTreeFilter(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    );

BOOLEAN PhpSignedProcessTreeFilter(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    );

PPH_PROCESS_ITEM PhpGetSelectedProcess();

VOID PhpGetSelectedProcesses(
    __out PPH_PROCESS_ITEM **Processes,
    __out PULONG NumberOfProcesses
    );

PPH_SERVICE_ITEM PhpGetSelectedService();

VOID PhpGetSelectedServices(
    __out PPH_SERVICE_ITEM **Services,
    __out PULONG NumberOfServices
    );

PPH_NETWORK_ITEM PhpGetSelectedNetworkItem();

VOID PhpGetSelectedNetworkItems(
    __out PPH_NETWORK_ITEM **NetworkItems,
    __out PULONG NumberOfNetworkItems
    );

VOID PhpShowProcessProperties(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpSetProcessMenuPriorityChecks(
    __in PPH_EMENU Menu,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN SetPriority,
    __in BOOLEAN SetIoPriority,
    __in BOOLEAN SetPagePriority
    );

VOID PhMainWndOnProcessAdded(
    __in __assumeRefs(1) PPH_PROCESS_ITEM ProcessItem,
    __in ULONG RunId
    );

VOID PhMainWndOnProcessModified(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnProcessesUpdated();

VOID PhMainWndOnServiceAdded(
    __in __assumeRefs(1) PPH_SERVICE_ITEM ServiceItem,
    __in ULONG RunId
    );

VOID PhMainWndOnServiceModified(
    __in PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    );

VOID PhMainWndOnServiceRemoved(
    __in PPH_SERVICE_ITEM ServiceItem
    );

VOID PhMainWndOnServicesUpdated();

VOID PhMainWndOnNetworkItemAdded(
    __in ULONG RunId,
    __in PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMainWndOnNetworkItemModified(
    __in PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMainWndOnNetworkItemRemoved(
    __in PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMainWndOnNetworkItemsUpdated();

PHAPPAPI HWND PhMainWndHandle;
BOOLEAN PhMainWndExiting = FALSE;
HMENU PhMainWndMenuHandle;

static BOOLEAN NeedsMaximize = FALSE;

static BOOLEAN DelayedLoadCompleted = FALSE;
static ULONG NotifyIconMask;
static ULONG NotifyIconNotifyMask;

static RECT LayoutPadding = { 0, 0, 0, 0 };
static HWND TabControlHandle;
static INT ProcessesTabIndex;
static INT ServicesTabIndex;
static INT NetworkTabIndex;
static INT MaxTabIndex;
static PPH_LIST AdditionalTabPageList = NULL;
static HWND ProcessTreeListHandle;
static HWND ServiceTreeListHandle;
static HWND NetworkListViewHandle;
static HFONT CurrentCustomFont;

static BOOLEAN NetworkFirstTime = TRUE;
static BOOLEAN ServiceTreeListLoaded = FALSE;
static BOOLEAN UpdateAutomatically = TRUE;

static PH_CALLBACK_REGISTRATION SymInitRegistration;

static PH_PROVIDER_REGISTRATION ProcessProviderRegistration;
static PH_CALLBACK_REGISTRATION ProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION ProcessModifiedRegistration;
static PH_CALLBACK_REGISTRATION ProcessRemovedRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedForIconsRegistration;
static BOOLEAN ProcessesNeedsRedraw = FALSE;

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
static BOOLEAN NetworkNeedsSort = FALSE;
static PH_IMAGE_LIST_WRAPPER NetworkImageListWrapper;

static PPH_PLUGIN_MENU_ITEM SelectedMenuItemPlugin;
static ULONG SelectedRunAsMode;
static HWND SelectedProcessWindowHandle;
static BOOLEAN SelectedProcessVirtualizationEnabled;
static ULONG SelectedUserSessionId;

static PPH_PROCESS_TREE_FILTER_ENTRY CurrentUserFilterEntry = NULL;
static PPH_PROCESS_TREE_FILTER_ENTRY SignedFilterEntry = NULL;

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    )
{
    PH_RECTANGLE windowRectangle;

    if (PhGetIntegerSetting(L"FirstRun"))
    {
        PPH_STRING autoDbghelpPath;

        // Try to set up the dbghelp path automatically if this is the first run.

        autoDbghelpPath = PHA_DEREFERENCE(PhGetKnownLocation(
            CSIDL_PROGRAM_FILES,
#ifdef _M_IX86
            L"\\Debugging Tools for Windows (x86)\\dbghelp.dll"
#else
            L"\\Debugging Tools for Windows (x64)\\dbghelp.dll"
#endif
            ));

        if (autoDbghelpPath)
        {
            if (RtlDoesFileExists_U(autoDbghelpPath->Buffer))
            {
                PhSetStringSetting2(L"DbgHelpPath", &autoDbghelpPath->sr);
            }
        }

        PhSetIntegerSetting(L"FirstRun", FALSE);
    }

    // This was added to be able to delay-load dbghelp.dll and symsrv.dll.
    PhRegisterCallback(&PhSymInitCallback, PhMainSymInitHandler, NULL, &SymInitRegistration);

    // Initialize the providers.
    {
        ULONG interval = PhGetIntegerSetting(L"UpdateInterval");

        if (interval == 0)
        {
            interval = 1000;
            PH_SET_INTEGER_CACHED_SETTING(UpdateInterval, interval);
        }

        PhInitializeProviderThread(&PhPrimaryProviderThread, interval);
        PhInitializeProviderThread(&PhSecondaryProviderThread, interval);
    }

    PhRegisterProvider(&PhPrimaryProviderThread, PhProcessProviderUpdate, NULL, &ProcessProviderRegistration);
    PhSetEnabledProvider(&ProcessProviderRegistration, TRUE);
    PhRegisterProvider(&PhPrimaryProviderThread, PhServiceProviderUpdate, NULL, &ServiceProviderRegistration);
    PhSetEnabledProvider(&ServiceProviderRegistration, TRUE);
    PhRegisterProvider(&PhPrimaryProviderThread, PhNetworkProviderUpdate, NULL, &NetworkProviderRegistration);

    windowRectangle.Position = PhGetIntegerPairSetting(L"MainWindowPosition");
    windowRectangle.Size = PhGetIntegerPairSetting(L"MainWindowSize");

    PhMainWndHandle = CreateWindow(
        PhWindowClassName,
        PhApplicationName,
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
    PhMainWndMenuHandle = GetMenu(PhMainWndHandle);

    if (!PhMainWndHandle)
        return FALSE;

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

    // Create the window title.
    {
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 100);
        PhAppendStringBuilder2(&stringBuilder, L"Process Hacker");

        if (PhCurrentUserName)
        {
            PhAppendStringBuilder2(&stringBuilder, L" [");
            PhAppendStringBuilder(&stringBuilder, PhCurrentUserName);
            PhAppendCharStringBuilder(&stringBuilder, ']');
            if (PhKphHandle) PhAppendCharStringBuilder(&stringBuilder, '+');
        }

        if (WINDOWS_HAS_UAC && PhElevationType == TokenElevationTypeFull)
            PhAppendStringBuilder2(&stringBuilder, L" (Administrator)");

        SetWindowText(PhMainWndHandle, stringBuilder.String->Buffer);

        PhDeleteStringBuilder(&stringBuilder);
    }

    // Fix some menu items.
    if (PhElevated)
    {
        DeleteMenu(PhMainWndMenuHandle, ID_HACKER_RUNASADMINISTRATOR, 0);
        DeleteMenu(PhMainWndMenuHandle, ID_HACKER_SHOWDETAILSFORALLPROCESSES, 0);
    }
    else
    {
        _LoadIconMetric loadIconMetric;
        HICON shieldIcon;
        MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };

        // It is necessary to use LoadIconMetric because otherwise the icons are at the wrong 
        // resolution and look very bad when scaled down to the small icon size.

        loadIconMetric = (_LoadIconMetric)PhGetProcAddress(L"comctl32.dll", "LoadIconMetric");

        if (loadIconMetric && SUCCEEDED(loadIconMetric(NULL, IDI_SHIELD, LIM_SMALL, &shieldIcon)))
        {
            menuItemInfo.fMask = MIIM_BITMAP;
            menuItemInfo.hbmpItem = PhIconToBitmap(shieldIcon, PhSmallIconSize.X, PhSmallIconSize.Y);
            DestroyIcon(shieldIcon);

            SetMenuItemInfo(PhMainWndMenuHandle, ID_HACKER_SHOWDETAILSFORALLPROCESSES, FALSE, &menuItemInfo);
        }
    }

    DrawMenuBar(PhMainWndHandle);

    PhReloadSysParameters();

    // Initialize child controls.
    PhMainWndOnCreate();

    PhpInitialLoadSettings();
    PhLogInitialization();
    PhQueueItemGlobalWorkQueue(PhpDelayedLoadFunction, NULL);

    PhMainWndTabControlOnSelectionChanged();

    // Perform a layout.
    SendMessage(PhMainWndHandle, WM_SIZE, 0, 0);

    PhStartProviderThread(&PhPrimaryProviderThread);
    PhStartProviderThread(&PhSecondaryProviderThread);

    UpdateWindow(PhMainWndHandle);

    if ((PhStartupParameters.ShowHidden || PhGetIntegerSetting(L"StartHidden")) && NotifyIconMask != 0)
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
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMainWindowShowing), (PVOID)ShowCommand);

    if (ShowCommand != SW_HIDE)
        ShowWindow(PhMainWndHandle, ShowCommand);

    // TS notification registration is done in the delayed load function. It's therefore 
    // possible that we miss a notification, but it's a trade-off.
    PhpRefreshUsersMenu();

    return TRUE;
}

LRESULT CALLBACK PhMainWndProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {
            ULONG mask;
            ULONG i;

            if (!PhMainWndExiting)
                ProcessHacker_SaveAllSettings(hWnd);

            // Remove all icons to prevent them hanging around after we exit.

            mask = NotifyIconMask;
            NotifyIconMask = 0; // prevent further icon updating

            for (i = PH_ICON_MINIMUM; i != PH_ICON_MAXIMUM; i <<= 1)
            {
                if (mask & i)
                    PhRemoveNotifyIcon(i);
            }

            // Notify plugins that we are shutting down.

            if (PhPluginsEnabled)
                PhUnloadPlugins();

            PostQuitMessage(0);
        }
        break;
    case WM_SETTINGCHANGE:
        {
            PhReloadSysParameters();
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_ESC_EXIT:
                {
                    if (PhGetIntegerSetting(L"HideOnClose") && NotifyIconMask != 0)
                        ShowWindow(hWnd, SW_HIDE);
                }
                break;
            case ID_HACKER_RUN:
                {
                    if (RunFileDlg)
                    {
                        SelectedRunAsMode = 0;
                        RunFileDlg(hWnd, NULL, NULL, NULL, NULL, 0);
                    }
                }
                break;
            case ID_HACKER_RUNASADMINISTRATOR:
                {
                    if (RunFileDlg)
                    {
                        SelectedRunAsMode = RUNAS_MODE_ADMIN;
                        RunFileDlg(
                            hWnd,
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
                            hWnd,
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
                    PhShowRunAsDialog(hWnd, NULL);
                }
                break;
            case ID_HACKER_SHOWDETAILSFORALLPROCESSES:
                {
                    ProcessHacker_PrepareForEarlyShutdown(hWnd);

                    if (PhShellProcessHacker(hWnd, L"-v", SW_SHOW, PH_SHELL_EXECUTE_ADMIN, TRUE, 0, NULL))
                    {
                        ProcessHacker_Destroy(hWnd);
                    }
                    else
                    {
                        ProcessHacker_CancelEarlyShutdown(hWnd);
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

                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                    PhSetFileDialogFileName(fileDialog, L"Process Hacker.txt");

                    if (PhShowFileDialog(hWnd, fileDialog))
                    {
                        NTSTATUS status;
                        PPH_STRING fileName;
                        PPH_FILE_STREAM fileStream;

                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhaDereferenceObject(fileName);

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
                            ULONG selectedTab;
                            PPH_LIST lines = NULL;

                            if (PhEndsWithString2(fileName, L".csv", TRUE))
                                mode = PH_EXPORT_MODE_CSV;
                            else
                                mode = PH_EXPORT_MODE_TABS;

                            PhWritePhTextHeader(fileStream);

                            selectedTab = TabCtrl_GetCurSel(TabControlHandle);

                            if (selectedTab == ProcessesTabIndex)
                                PhWriteProcessTree(fileStream, mode);
                            else if (selectedTab == ServicesTabIndex)
                                PhWriteServiceList(fileStream, mode);
                            else if (selectedTab == NetworkTabIndex)
                                lines = PhGetListViewLines(NetworkListViewHandle, mode);

                            if (lines)
                            {
                                ULONG i;

                                for (i = 0; i < lines->Count; i++)
                                {
                                    PPH_STRING line;

                                    line = lines->Items[i];
                                    PhWriteStringAsAnsiFileStream(fileStream, &line->sr);
                                    PhDereferenceObject(line);
                                    PhWriteStringAsAnsiFileStream2(fileStream, L"\r\n");
                                }

                                PhDereferenceObject(lines);
                            }

                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hWnd, L"Unable to create the file", status, 0);
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
                    PhShowOptionsDialog(hWnd);
                }
                break;
            case ID_HACKER_PLUGINS:
                {
                    PhShowPluginsDialog(hWnd);
                }
                break;
            case ID_COMPUTER_LOCK:
            case ID_COMPUTER_LOGOFF:
            case ID_COMPUTER_SLEEP:
            case ID_COMPUTER_HIBERNATE:
            case ID_COMPUTER_RESTART:
            case ID_COMPUTER_SHUTDOWN:
            case ID_COMPUTER_POWEROFF:
                PhpProcessComputerCommand(LOWORD(wParam));
                break;
            case ID_HACKER_EXIT:
                ProcessHacker_Destroy(hWnd);
                break;
            case ID_VIEW_SYSTEMINFORMATION:
                PhShowSystemInformationDialog();
                break;
            case ID_TRAYICONS_CPUHISTORY:
            case ID_TRAYICONS_CPUUSAGE:
            case ID_TRAYICONS_IOHISTORY:
            case ID_TRAYICONS_COMMITHISTORY:
            case ID_TRAYICONS_PHYSICALMEMORYHISTORY:
                {
                    ULONG i;
                    BOOLEAN enable;

                    switch (LOWORD(wParam))
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

                    enable = !(GetMenuState(PhMainWndMenuHandle, LOWORD(wParam), 0) & MF_CHECKED);

                    if (enable)
                    {
                        NotifyIconMask |= i;
                        PhAddNotifyIcon(i);
                    }
                    else
                    {
                        NotifyIconMask &= ~i;
                        PhRemoveNotifyIcon(i);
                    }

                    CheckMenuItem(
                        PhMainWndMenuHandle,
                        LOWORD(wParam),
                        enable ? MF_CHECKED : MF_UNCHECKED
                        );
                }
                break;
            case ID_VIEW_HIDEPROCESSESFROMOTHERUSERS:
                {
                    if (!CurrentUserFilterEntry)
                    {
                        CurrentUserFilterEntry = PhAddProcessTreeFilter(PhpCurrentUserProcessTreeFilter, NULL);
                    }
                    else
                    {
                        PhRemoveProcessTreeFilter(CurrentUserFilterEntry);
                        CurrentUserFilterEntry = NULL;
                    }

                    PhApplyProcessTreeFilters();

                    PhSetIntegerSetting(L"HideOtherUserProcesses", !!CurrentUserFilterEntry);
                    CheckMenuItem(
                        PhMainWndMenuHandle,
                        ID_VIEW_HIDEPROCESSESFROMOTHERUSERS,
                        CurrentUserFilterEntry ? MF_CHECKED : MF_UNCHECKED
                        );
                }
                break;
            case ID_VIEW_HIDESIGNEDPROCESSES:
                {
                    if (!SignedFilterEntry)
                    {
                        if (!PhEnableProcessQueryStage2)
                        {
                            PhShowInformation(
                                hWnd,
                                L"This filter cannot function because digital signature checking is not enabled. "
                                L"Enable it in Options > Advanced and restart Process Hacker."
                                );
                        }

                        SignedFilterEntry = PhAddProcessTreeFilter(PhpSignedProcessTreeFilter, NULL);
                    }
                    else
                    {
                        PhRemoveProcessTreeFilter(SignedFilterEntry);
                        SignedFilterEntry = NULL;
                    }

                    PhApplyProcessTreeFilters();

                    PhSetIntegerSetting(L"HideSignedProcesses", !!SignedFilterEntry);
                    CheckMenuItem(
                        PhMainWndMenuHandle,
                        ID_VIEW_HIDESIGNEDPROCESSES,
                        SignedFilterEntry ? MF_CHECKED : MF_UNCHECKED
                        );
                }
                break;
            case ID_VIEW_ALWAYSONTOP:
                {
                    BOOLEAN topMost;

                    topMost = !(GetMenuState(PhMainWndMenuHandle, ID_VIEW_ALWAYSONTOP, 0) & MF_CHECKED);
                    SetWindowPos(PhMainWndHandle, topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                        0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                    PhSetIntegerSetting(L"MainWindowAlwaysOnTop", topMost);
                    CheckMenuItem(
                        PhMainWndMenuHandle,
                        ID_VIEW_ALWAYSONTOP,
                        topMost ? MF_CHECKED : MF_UNCHECKED
                        );
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

                    opacity = 100 - (((ULONG)LOWORD(wParam) - ID_OPACITY_10) + 1) * 10;
                    PhSetIntegerSetting(L"MainWindowOpacity", opacity);
                    PhpSetWindowOpacity(opacity);
                    PhpSetCheckOpacityMenu(TRUE, opacity);
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

                    switch (LOWORD(wParam))
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
                    PhApplyUpdateInterval(interval);
                    CheckMenuRadioItem(
                        PhMainWndMenuHandle,
                        ID_UPDATEINTERVAL_FAST,
                        ID_UPDATEINTERVAL_VERYSLOW,
                        LOWORD(wParam),
                        MF_BYCOMMAND
                        );
                }
                break;
            case ID_VIEW_UPDATEAUTOMATICALLY:
                {
                    UpdateAutomatically = !UpdateAutomatically;

                    PhSetEnabledProvider(&ProcessProviderRegistration, UpdateAutomatically);
                    PhSetEnabledProvider(&ServiceProviderRegistration, UpdateAutomatically);

                    if (TabCtrl_GetCurSel(TabControlHandle) == NetworkTabIndex)
                        PhSetEnabledProvider(&NetworkProviderRegistration, UpdateAutomatically);

                    CheckMenuItem(
                        PhMainWndMenuHandle,
                        ID_VIEW_UPDATEAUTOMATICALLY,
                        UpdateAutomatically ? MF_CHECKED : MF_UNCHECKED
                        );
                }
                break;
            case ID_TOOLS_CREATESERVICE:
                {
                    PhShowCreateServiceDialog(hWnd);
                }
                break;
            case ID_TOOLS_HIDDENPROCESSES:
                {
                    PhShowHiddenProcessesDialog();
                }
                break;
            case ID_TOOLS_PAGEFILES:
                {
                    PhShowPagefilesDialog(hWnd);
                }
                break;
            case ID_TOOLS_VERIFYFILESIGNATURE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Executable files (*.exe;*.dll;*.ocx;*.sys;*.scr;*.cpl)", L"*.exe;*.dll;*.ocx;*.sys;*.scr;*.cpl" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog = PhCreateOpenFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    if (PhShowFileDialog(hWnd, fileDialog))
                    {
                        PPH_STRING fileName;
                        VERIFY_RESULT result;
                        PPH_STRING signerName;

                        fileName = PhGetFileDialogFileName(fileDialog);
                        result = PhVerifyFile(fileName->Buffer, &signerName);

                        if (result == VrTrusted)
                        {
                            PhShowInformation(hWnd, L"\"%s\" is trusted and signed by \"%s\".",
                                fileName->Buffer, signerName->Buffer);
                        }
                        else if (result == VrNoSignature)
                        {
                            PhShowInformation(hWnd, L"\"%s\" does not have a digital signature.",
                                fileName->Buffer);
                        }
                        else
                        {
                            PhShowInformation(hWnd, L"\"%s\" is not trusted.",
                                fileName->Buffer);
                        }

                        if (signerName)
                            PhDereferenceObject(signerName);

                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case ID_USER_CONNECT:
                {
                    PhUiConnectSession(hWnd, SelectedUserSessionId);
                }
                break;
            case ID_USER_DISCONNECT:
                {
                    PhUiDisconnectSession(hWnd, SelectedUserSessionId);
                }
                break;
            case ID_USER_LOGOFF:
                {
                    PhUiLogoffSession(hWnd, SelectedUserSessionId);
                }
                break;
            case ID_USER_REMOTECONTROL:
                {
                    PhShowSessionShadowDialog(hWnd, SelectedUserSessionId);
                }
                break;
            case ID_USER_SENDMESSAGE:
                {
                    PhShowSessionSendMessageDialog(hWnd, SelectedUserSessionId);
                }
                break;
            case ID_USER_PROPERTIES:
                {
                    PhShowSessionProperties(hWnd, SelectedUserSessionId);
                }
                break;
            case ID_HELP_LOG:
                {
                    PhShowLogDialog();
                }
                break;
            case ID_HELP_DONATE:
                {
                    PhShellExecute(hWnd, L"https://sourceforge.net/project/project_donations.php?group_id=242527", NULL);
                }
                break;
            case ID_HELP_DEBUGCONSOLE:
                {
                    PhShowDebugConsole();
                }
                break;
            case ID_HELP_ABOUT:
                {
                    PhShowAboutDialog(hWnd);
                }
                break;
            case ID_PROCESS_TERMINATE:
                {
                    PPH_PROCESS_ITEM *processes;
                    ULONG numberOfProcesses;

                    PhpGetSelectedProcesses(&processes, &numberOfProcesses);
                    PhReferenceObjects(processes, numberOfProcesses);

                    if (PhUiTerminateProcesses(hWnd, processes, numberOfProcesses))
                        PhDeselectAllProcessNodes();

                    PhDereferenceObjects(processes, numberOfProcesses);
                    PhFree(processes);
                }
                break;
            case ID_PROCESS_TERMINATETREE:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);

                        if (PhUiTerminateTreeProcess(hWnd, processItem))
                            PhDeselectAllProcessNodes();

                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_PROCESS_SUSPEND:
                {
                    PPH_PROCESS_ITEM *processes;
                    ULONG numberOfProcesses;

                    PhpGetSelectedProcesses(&processes, &numberOfProcesses);
                    PhReferenceObjects(processes, numberOfProcesses);
                    PhUiSuspendProcesses(hWnd, processes, numberOfProcesses);
                    PhDereferenceObjects(processes, numberOfProcesses);
                    PhFree(processes);
                }
                break;
            case ID_PROCESS_RESUME:
                {
                    PPH_PROCESS_ITEM *processes;
                    ULONG numberOfProcesses;

                    PhpGetSelectedProcesses(&processes, &numberOfProcesses);
                    PhReferenceObjects(processes, numberOfProcesses);
                    PhUiResumeProcesses(hWnd, processes, numberOfProcesses);
                    PhDereferenceObjects(processes, numberOfProcesses);
                    PhFree(processes);
                }
                break;
            case ID_PROCESS_RESTART:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);

                        if (PhUiRestartProcess(hWnd, processItem))
                            PhDeselectAllProcessNodes();

                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_PROCESS_DEBUG:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhUiDebugProcess(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_PROCESS_REDUCEWORKINGSET:
                {
                    PPH_PROCESS_ITEM *processes;
                    ULONG numberOfProcesses;

                    PhpGetSelectedProcesses(&processes, &numberOfProcesses);
                    PhReferenceObjects(processes, numberOfProcesses);
                    PhUiReduceWorkingSetProcesses(hWnd, processes, numberOfProcesses);
                    PhDereferenceObjects(processes, numberOfProcesses);
                    PhFree(processes);
                }
                break;
            case ID_PROCESS_VIRTUALIZATION:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhUiSetVirtualizationProcess(
                            hWnd,
                            processItem,
                            !SelectedProcessVirtualizationEnabled
                            );
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_PROCESS_AFFINITY:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhShowProcessAffinityDialog(hWnd, processItem, NULL);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_PROCESS_CREATEDUMPFILE:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhUiCreateDumpFileProcess(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_PROCESS_TERMINATOR:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        // The object relies on the list view reference, which could 
                        // disappear if we don't reference the object here.
                        PhReferenceObject(processItem);
                        PhShowProcessTerminatorDialog(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_MISCELLANEOUS_DETACHFROMDEBUGGER:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhUiDetachFromDebuggerProcess(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_MISCELLANEOUS_GDIHANDLES:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhShowGdiHandlesDialog(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_MISCELLANEOUS_HEAPS:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhShowProcessHeapsDialog(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_MISCELLANEOUS_INJECTDLL:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhUiInjectDllProcess(hWnd, processItem);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_I_0:
            case ID_I_1:
            case ID_I_2:
            case ID_I_3:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        ULONG ioPriority;

                        switch (id)
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
                        }

                        PhReferenceObject(processItem);
                        PhUiSetIoPriorityProcess(hWnd, processItem, ioPriority);
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
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        ULONG pagePriority;

                        switch (id)
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
                        PhUiSetPagePriorityProcess(hWnd, processItem, pagePriority);
                        PhDereferenceObject(processItem);
                    }
                }
                break;
            case ID_MISCELLANEOUS_RUNAS:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem && processItem->FileName)
                    {
                        PhSetStringSetting2(L"RunAsProgram", &processItem->FileName->sr);
                        PhShowRunAsDialog(hWnd, NULL);
                    }
                }
                break;
            case ID_MISCELLANEOUS_RUNASTHISUSER:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhShowRunAsDialog(hWnd, processItem->ProcessId);
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
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhReferenceObject(processItem);
                        PhpProcessProcessPriorityCommand(id, processItem);
                        PhDereferenceObject(processItem);
                    }
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
            case ID_PROCESS_PROPERTIES:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        // No reference needed; no messages pumped.
                        PhpShowProcessProperties(processItem);
                    }
                }
                break;
            case ID_PROCESS_SEARCHONLINE:
                {
                    PPH_PROCESS_ITEM processItem = PhpGetSelectedProcess();

                    if (processItem)
                    {
                        PhSearchOnlineString(hWnd, processItem->ProcessName->Buffer);
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
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();
                    PPH_PROCESS_NODE processNode;

                    if (serviceItem)
                    {
                        if (processNode = PhFindProcessNode(serviceItem->ProcessId))
                        {
                            PhpSelectTabPage(ProcessesTabIndex);
                            SetFocus(ProcessTreeListHandle);
                            PhSelectAndEnsureVisibleProcessNode(processNode);
                        }
                    }
                }
                break;
            case ID_SERVICE_START:
                {
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();

                    if (serviceItem)
                    {
                        PhReferenceObject(serviceItem);
                        PhUiStartService(hWnd, serviceItem);
                        PhDereferenceObject(serviceItem);
                    }
                }
                break;
            case ID_SERVICE_CONTINUE:
                {
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();

                    if (serviceItem)
                    {
                        PhReferenceObject(serviceItem);
                        PhUiContinueService(hWnd, serviceItem);
                        PhDereferenceObject(serviceItem);
                    }
                }
                break;
            case ID_SERVICE_PAUSE:
                {
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();

                    if (serviceItem)
                    {
                        PhReferenceObject(serviceItem);
                        PhUiPauseService(hWnd, serviceItem);
                        PhDereferenceObject(serviceItem);
                    }
                }
                break;
            case ID_SERVICE_STOP:
                {
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();

                    if (serviceItem)
                    {
                        PhReferenceObject(serviceItem);
                        PhUiStopService(hWnd, serviceItem);
                        PhDereferenceObject(serviceItem);
                    }
                }
                break;
            case ID_SERVICE_DELETE:
                {
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();

                    if (serviceItem)
                    {
                        PhReferenceObject(serviceItem);

                        if (PhUiDeleteService(hWnd, serviceItem))
                            PhDeselectAllServiceNodes();

                        PhDereferenceObject(serviceItem);
                    }
                }
                break;
            case ID_SERVICE_PROPERTIES:
                {
                    PPH_SERVICE_ITEM serviceItem = PhpGetSelectedService();

                    if (serviceItem)
                    {
                        // The object relies on the list view reference, which could 
                        // disappear if we don't reference the object here.
                        PhReferenceObject(serviceItem);
                        PhShowServiceProperties(hWnd, serviceItem);
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
                    PPH_NETWORK_ITEM networkItem = PhpGetSelectedNetworkItem();
                    PPH_PROCESS_NODE processNode;

                    if (networkItem)
                    {
                        if (processNode = PhFindProcessNode(networkItem->ProcessId))
                        {
                            PhpSelectTabPage(ProcessesTabIndex);
                            SetFocus(ProcessTreeListHandle);
                            PhSelectAndEnsureVisibleProcessNode(processNode);
                        }
                    }
                }
                break;
            case ID_NETWORK_VIEWSTACK:
                {
                    PPH_NETWORK_ITEM networkItem = PhpGetSelectedNetworkItem();

                    if (networkItem)
                    {
                        PhReferenceObject(networkItem);
                        PhShowNetworkStackDialog(hWnd, networkItem);
                        PhDereferenceObject(networkItem);
                    }
                }
                break;
            case ID_NETWORK_CLOSE:
                {
                    PPH_NETWORK_ITEM *networkItems;
                    ULONG numberOfNetworkItems;

                    PhpGetSelectedNetworkItems(&networkItems, &numberOfNetworkItems);
                    PhReferenceObjects(networkItems, numberOfNetworkItems);

                    if (PhUiCloseConnections(hWnd, networkItems, numberOfNetworkItems))
                        PhSetStateAllListViewItems(NetworkListViewHandle, 0, LVIS_SELECTED);

                    PhDereferenceObjects(networkItems, numberOfNetworkItems);
                    PhFree(networkItems);
                }
                break;
            case ID_NETWORK_COPY:
                {
                    PhCopyListView(NetworkListViewHandle);
                }
                break;
            case ID_TAB_NEXT:
                {
                    ULONG selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

                    if (selectedIndex != MaxTabIndex)
                        selectedIndex++;
                    else
                        selectedIndex = 0;

                    PhpSelectTabPage(selectedIndex);
                    SetFocus(TabControlHandle);
                }
                break;
            case ID_TAB_PREV:
                {
                    ULONG selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

                    if (selectedIndex != 0)
                        selectedIndex--;
                    else
                        selectedIndex = MaxTabIndex;

                    PhpSelectTabPage(selectedIndex);
                    SetFocus(TabControlHandle);
                }
                break;
            }

            if (PhPluginsEnabled)
            {
                if (id >= IDPLUGINS && id < IDPLUGINS_END)
                {
                    if (SelectedMenuItemPlugin && SelectedMenuItemPlugin->RealId == (ULONG)id)
                    {
                        SelectedMenuItemPlugin->OwnerWindow = hWnd;

                        PhInvokeCallback(
                            PhGetPluginCallback(SelectedMenuItemPlugin->Plugin, PluginCallbackMenuItem),
                            SelectedMenuItemPlugin
                            );
                    }
                }
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (NeedsMaximize)
            {
                ShowWindow(hWnd, SW_MAXIMIZE);
                NeedsMaximize = FALSE;
            }
        }
        break;
    case WM_SYSCOMMAND:
        {
            switch (wParam)
            {
            case SC_CLOSE:
                {
                    if (PhGetIntegerSetting(L"HideOnClose") && NotifyIconMask != 0)
                    {
                        ShowWindow(hWnd, SW_HIDE);
                        return 0;
                    }
                }
                break;
            case SC_MINIMIZE:
                {
                    // Save the current window state because we 
                    // may not have a chance to later.
                    PhpSaveWindowState();

                    if (PhGetIntegerSetting(L"HideOnMinimize") && NotifyIconMask != 0)
                    {
                        ShowWindow(hWnd, SW_HIDE);
                        return 0;
                    }
                }
                break;
            }
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    case WM_MENUSELECT:
        {
            switch (LOWORD(wParam))
            {
            case ID_USER_CONNECT:
            case ID_USER_DISCONNECT:
            case ID_USER_LOGOFF:
            case ID_USER_SENDMESSAGE:
            case ID_USER_PROPERTIES:
                {
                    MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };

                    menuItemInfo.fMask = MIIM_DATA;

                    if (GetMenuItemInfo((HMENU)lParam, LOWORD(wParam), FALSE, &menuItemInfo))
                    {
                        SelectedUserSessionId = (ULONG)menuItemInfo.dwItemData;
                    }
                }
                break;
            }

            if (PhPluginsEnabled)
            {
                USHORT id = LOWORD(wParam);

                if (id >= IDPLUGINS && id < IDPLUGINS_END)
                {
                    MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };

                    menuItemInfo.fMask = MIIM_DATA;

                    if (GetMenuItemInfo((HMENU)lParam, LOWORD(wParam), FALSE, &menuItemInfo))
                    {
                        SelectedMenuItemPlugin = (PPH_PLUGIN_MENU_ITEM)menuItemInfo.dwItemData;
                    }
                }
            }
        }
        break;
    case WM_SIZE:
        {
            if (!IsIconic(hWnd))
            {
                HDWP deferHandle = BeginDeferWindowPos(2);
                PhMainWndOnLayout(&deferHandle);
                EndDeferWindowPos(deferHandle);
            }
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 400, 340);
        }
        break;
    case WM_SETFOCUS:
        {
            INT selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

            if (selectedIndex == ProcessesTabIndex)
                SetFocus(ProcessTreeListHandle);
            else if (selectedIndex == ServicesTabIndex)
                SetFocus(ServiceTreeListHandle);
            else if (selectedIndex == NetworkTabIndex)
                SetFocus(NetworkListViewHandle);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == TabControlHandle)
            {
                PhMainWndTabControlOnNotify(header);
            }
            else if (header->hwndFrom == NetworkListViewHandle)
            {
                PhMainWndNetworkListViewOnNotify(header);
            }
            else if (header->code == RFN_VALIDATE)
            {
                LPNMRUNFILEDLG runFileDlg = (LPNMRUNFILEDLG)header;

                if (SelectedRunAsMode == RUNAS_MODE_ADMIN)
                {
                    if (PhShellExecuteEx(hWnd, (PWSTR)runFileDlg->lpszFile,
                        NULL, runFileDlg->nShow, PH_SHELL_EXECUTE_ADMIN, 0, NULL))
                    {
                        return RF_CANCEL;
                    }
                    else
                    {
                        return RF_RETRY;
                    }
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
                        return RF_CANCEL;
                    }
                    else
                    {
                        PhShowStatus(hWnd, L"Unable to execute the program", status, 0);
                        return RF_RETRY;
                    }
                }
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == NetworkListViewHandle)
            {
                PhMainWndNetworkListViewOnContextMenu(lParam);
            }
        }
        break;
    case WM_WTSSESSION_CHANGE:
        {
            if (wParam == WTS_SESSION_LOGON || wParam == WTS_SESSION_LOGOFF)
            {
                PhpRefreshUsersMenu();
            }
        }
        break;
    case WM_PH_ACTIVATE:
        {
            if (!PhMainWndExiting)
            {
                if (!IsWindowVisible(hWnd))
                {
                    ShowWindow(hWnd, SW_SHOW);
                }

                if (IsIconic(hWnd))
                {
                    ShowWindow(hWnd, SW_RESTORE);
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
            PhpShowProcessProperties((PPH_PROCESS_ITEM)lParam);
        }
        break;
    case WM_PH_DESTROY:
        {
            DestroyWindow(hWnd);
        }
        break;
    case WM_PH_SAVE_ALL_SETTINGS:
        {
            PhpSaveAllSettings();
        }
        break;
    case WM_PH_PREPARE_FOR_EARLY_SHUTDOWN:
        {
            PhpPrepareForEarlyShutdown();
        }
        break;
    case WM_PH_CANCEL_EARLY_SHUTDOWN:
        {
            PhpCancelEarlyShutdown();
        }
        break;
    case WM_PH_DELAYED_LOAD_COMPLETED:
        {
            // Nothing
        }
        break;
    case WM_PH_NOTIFY_ICON_MESSAGE:
        {
            switch (LOWORD(lParam))
            {
            case WM_LBUTTONDOWN:
                {
                    if (PhGetIntegerSetting(L"IconSingleClick"))
                        SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, 0, 0);
                }
                break;
            case WM_LBUTTONDBLCLK:
                {
                    if (!PhGetIntegerSetting(L"IconSingleClick"))
                        SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, 0, 0);
                }
                break;
            case WM_RBUTTONUP:
                {
                    POINT location;

                    GetCursorPos(&location);
                    PhpShowIconContextMenu(location);
                }
                break;
            }
        }
        break;
    case WM_PH_TOGGLE_VISIBLE:
        {
            if (IsIconic(PhMainWndHandle))
            {
                ShowWindow(PhMainWndHandle, SW_RESTORE);
                SetForegroundWindow(PhMainWndHandle);
            }
            else if (IsWindowVisible(PhMainWndHandle))
            {
                ShowWindow(PhMainWndHandle, SW_HIDE);
            }
            else
            {
                ShowWindow(PhMainWndHandle, SW_SHOW);
                SetForegroundWindow(PhMainWndHandle);
            }
        }
        break;
    case WM_PH_SHOW_MEMORY_EDITOR:
        {
            PPH_SHOWMEMORYEDITOR showMemoryEditor = (PPH_SHOWMEMORYEDITOR)lParam;

            PhShowMemoryEditorDialog(
                showMemoryEditor->ProcessId,
                showMemoryEditor->BaseAddress,
                showMemoryEditor->RegionSize,
                showMemoryEditor->SelectOffset,
                showMemoryEditor->SelectLength
                );
            PhFree(showMemoryEditor);
        }
        break;
    case WM_PH_SHOW_MEMORY_RESULTS:
        {
            PPH_SHOWMEMORYRESULTS showMemoryResults = (PPH_SHOWMEMORYRESULTS)lParam;

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
            ULONG index = (ULONG)wParam;

            PhpSelectTabPage(index);

            if (index == ProcessesTabIndex)
                SetFocus(ProcessTreeListHandle);
            else if (index == ServicesTabIndex)
                SetFocus(ServiceTreeListHandle);
            else if (index == NetworkTabIndex)
                SetFocus(NetworkListViewHandle);
        }
        break;
    case WM_PH_GET_LAYOUT_PADDING:
        {
            PRECT rect = (PRECT)lParam;

            *rect = LayoutPadding;
        }
        break;
    case WM_PH_SET_LAYOUT_PADDING:
        {
            PRECT rect = (PRECT)lParam;

            LayoutPadding = *rect;
        }
        break;
    case WM_PH_SELECT_PROCESS_NODE:
        {
            PhSelectAndEnsureVisibleProcessNode((PPH_PROCESS_NODE)lParam);
        }
        break;
    case WM_PH_SELECT_SERVICE_ITEM:
        {
            PPH_SERVICE_NODE serviceNode;

            PhpNeedServiceTreeList();

            // For compatibility, lParam is a service item, not node.
            if (serviceNode = PhFindServiceNode((PPH_SERVICE_ITEM)lParam))
            {
                PhSelectAndEnsureVisibleServiceNode(serviceNode);
            }
        }
        break;
    case WM_PH_SELECT_NETWORK_ITEM:
        {
            INT lvItemIndex;

            PhSetStateAllListViewItems(NetworkListViewHandle, 0, LVIS_SELECTED);
            lvItemIndex = PhFindListViewItemByParam(NetworkListViewHandle, -1, (PVOID)lParam);

            if (lvItemIndex != -1)
            {
                ListView_SetItemState(NetworkListViewHandle, lvItemIndex,
                    LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
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
                    SendMessage(NetworkListViewHandle, WM_SETFONT, (WPARAM)newFont, TRUE);
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

            function = (PVOID)lParam;
            function((PVOID)wParam);
        }
        break;
    case WM_PH_ADD_MENU_ITEM:
        {
            PPH_ADDMENUITEM addMenuItem = (PPH_ADDMENUITEM)lParam;

            return (LRESULT)PhpMainWndAddMenuItem(
                addMenuItem->Plugin,
                addMenuItem->ParentMenu,
                addMenuItem->InsertAfter,
                addMenuItem->Flags,
                addMenuItem->Id,
                addMenuItem->Text,
                addMenuItem->Context
                );
        }
        break;
    case WM_PH_ADD_TAB_PAGE:
        {
            return (LRESULT)PhpAddTabPage((PPH_ADDITIONAL_TAB_PAGE)lParam);
        }
        break;
    case WM_PH_PROCESS_ADDED:
        {
            ULONG runId = (ULONG)wParam;
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)lParam;

            PhMainWndOnProcessAdded(processItem, runId);
        }
        break;
    case WM_PH_PROCESS_MODIFIED:
        {
            PhMainWndOnProcessModified((PPH_PROCESS_ITEM)lParam);
        }
        break;
    case WM_PH_PROCESS_REMOVED:
        {
            PhMainWndOnProcessRemoved((PPH_PROCESS_ITEM)lParam);
        }
        break;
    case WM_PH_PROCESSES_UPDATED:
        {
            PhMainWndOnProcessesUpdated();
        }
        break;
    case WM_PH_SERVICE_ADDED:
        {
            ULONG runId = (ULONG)wParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)lParam;

            PhMainWndOnServiceAdded(serviceItem, runId);
        }
        break;
    case WM_PH_SERVICE_MODIFIED:
        {
            PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)lParam;

            PhMainWndOnServiceModified(serviceModifiedData);
            PhFree(serviceModifiedData);
        }
        break;
    case WM_PH_SERVICE_REMOVED:
        {
            PhMainWndOnServiceRemoved((PPH_SERVICE_ITEM)lParam);
        }
        break;
    case WM_PH_SERVICES_UPDATED:
        {
            PhMainWndOnServicesUpdated();
        }
        break;
    case WM_PH_NETWORK_ITEM_ADDED:
        {
            ULONG runId = (ULONG)wParam;
            PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)lParam;

            PhMainWndOnNetworkItemAdded(runId, networkItem);
            PhDereferenceObject(networkItem);
        }
        break;
    case WM_PH_NETWORK_ITEM_MODIFIED:
        {
            PhMainWndOnNetworkItemModified((PPH_NETWORK_ITEM)lParam);
        }
        break;
    case WM_PH_NETWORK_ITEM_REMOVED:
        {
            PhMainWndOnNetworkItemRemoved((PPH_NETWORK_ITEM)lParam);
        }
        break;
    case WM_PH_NETWORK_ITEMS_UPDATED:
        {
            PhMainWndOnNetworkItemsUpdated();
        }
        break;
    }

    REFLECT_MESSAGE(NetworkListViewHandle, uMsg, wParam, lParam);

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID PhpReloadListViewFont()
{
    HFONT fontHandle;
    LOGFONT font;

    if (NetworkListViewHandle && (fontHandle = (HFONT)SendMessage(NetworkListViewHandle, WM_GETFONT, 0, 0)))
    {
        if (GetObject(fontHandle, sizeof(LOGFONT), &font))
        {
            font.lfWeight = FW_BOLD;
            fontHandle = CreateFontIndirect(&font);

            if (fontHandle)
            {
                if (PhBoldListViewFont)
                    DeleteObject(PhBoldListViewFont);

                PhBoldListViewFont = fontHandle;
            }
        }
    }
}

VOID PhReloadSysParameters()
{
    PhSysWindowColor = GetSysColor(COLOR_WINDOW);

    if (PhApplicationFont)
        DeleteObject(PhApplicationFont);
    if (PhBoldMessageFont)
        DeleteObject(PhBoldMessageFont);
    PhInitializeFont(PhMainWndHandle);
    SendMessage(TabControlHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);

    PhpReloadListViewFont();
}

VOID PhpInitialLoadSettings()
{
    ULONG opacity;
    ULONG id;
    PPH_STRING customFont;
    ULONG i;

    if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
    {
        CheckMenuItem(PhMainWndMenuHandle, ID_VIEW_ALWAYSONTOP, MF_CHECKED);
        SetWindowPos(PhMainWndHandle, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
    }

    opacity = PhGetIntegerSetting(L"MainWindowOpacity");

    if (opacity != 0)
        PhpSetWindowOpacity(opacity);

    PhpSetCheckOpacityMenu(TRUE, opacity);

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

    if (id != -1)
    {
        CheckMenuRadioItem(
            PhMainWndMenuHandle,
            ID_UPDATEINTERVAL_FAST,
            ID_UPDATEINTERVAL_VERYSLOW,
            id,
            MF_BYCOMMAND
            );
    }

    PhStatisticsSampleCount = PhGetIntegerSetting(L"SampleCount");
    PhEnableProcessQueryStage2 = !!PhGetIntegerSetting(L"EnableStage2");
    PhEnablePurgeProcessRecords = !PhGetIntegerSetting(L"NoPurgeProcessRecords");
    PhEnableServiceNonPoll = !!PhGetIntegerSetting(L"EnableServiceNonPoll");
    PhEnableNetworkProviderResolve = !!PhGetIntegerSetting(L"EnableNetworkResolve");

    NotifyIconMask = PhGetIntegerSetting(L"IconMask");

    for (i = PH_ICON_MINIMUM; i != PH_ICON_MAXIMUM; i <<= 1)
    {
        if (NotifyIconMask & i)
        {
            switch (i)
            {
            case PH_ICON_CPU_HISTORY:
                id = ID_TRAYICONS_CPUHISTORY;
                break;
            case PH_ICON_IO_HISTORY:
                id = ID_TRAYICONS_IOHISTORY;
                break;
            case PH_ICON_COMMIT_HISTORY:
                id = ID_TRAYICONS_COMMITHISTORY;
                break;
            case PH_ICON_PHYSICAL_HISTORY:
                id = ID_TRAYICONS_PHYSICALMEMORYHISTORY;
                break;
            case PH_ICON_CPU_USAGE:
                id = ID_TRAYICONS_CPUUSAGE;
                break;
            }

            CheckMenuItem(PhMainWndMenuHandle, id, MF_CHECKED);
        }
    }

    NotifyIconNotifyMask = PhGetIntegerSetting(L"IconNotifyMask");

    if (PhGetIntegerSetting(L"HideOtherUserProcesses"))
    {
        CurrentUserFilterEntry = PhAddProcessTreeFilter(PhpCurrentUserProcessTreeFilter, NULL);
        CheckMenuItem(PhMainWndMenuHandle, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS, MF_CHECKED);
    }

    if (PhGetIntegerSetting(L"HideSignedProcesses"))
    {
        SignedFilterEntry = PhAddProcessTreeFilter(PhpSignedProcessTreeFilter, NULL);
        CheckMenuItem(PhMainWndMenuHandle, ID_VIEW_HIDESIGNEDPROCESSES, MF_CHECKED);
    }

    customFont = PhGetStringSetting(L"Font");

    if (customFont->Length / 2 / 2 == sizeof(LOGFONT))
        SendMessage(PhMainWndHandle, WM_PH_UPDATE_FONT, 0, 0);

    PhDereferenceObject(customFont);

    PhLoadSettingsProcessTreeList();
    // Service list settings are loaded on demand.
    PhLoadListViewColumnsFromSetting(L"NetworkListViewColumns", NetworkListViewHandle);
}

VOID PhMainSymInitHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_STRING dbghelpPath;
    HMODULE dbghelpModule;

    dbghelpPath = PhGetStringSetting(L"DbgHelpPath");

    if (dbghelpModule = LoadLibrary(dbghelpPath->Buffer))
    {
        PPH_STRING fullDbghelpPath;
        ULONG indexOfFileName;
        PH_STRINGREF dbghelpFolder;
        PPH_STRING symsrvPath;

        fullDbghelpPath = PhGetDllFileName(dbghelpModule, &indexOfFileName);

        if (fullDbghelpPath)
        {
            if (indexOfFileName != -1)
            {
                static PH_STRINGREF symsrvString = PH_STRINGREF_INIT(L"\\symsrv.dll");

                dbghelpFolder.Buffer = fullDbghelpPath->Buffer;
                dbghelpFolder.Length = (USHORT)(indexOfFileName * sizeof(WCHAR));

                symsrvPath = PhConcatStringRef2(&dbghelpFolder, &symsrvString);
                LoadLibrary(symsrvPath->Buffer);
                PhDereferenceObject(symsrvPath);
            }

            PhDereferenceObject(fullDbghelpPath);
        }
    }
    else
    {
        LoadLibrary(L"dbghelp.dll");
    }

    PhDereferenceObject(dbghelpPath);

    PhSymbolProviderDynamicImport();
}

static NTSTATUS PhpDelayedLoadFunction(
    __in PVOID Parameter
    )
{
    ULONG i;

    // Register for window station notifications.
    WinStationRegisterConsoleNotification(NULL, PhMainWndHandle, WNOTIFY_ALL_SESSIONS);

    for (i = PH_ICON_MINIMUM; i != PH_ICON_MAXIMUM; i <<= 1)
    {
        if (NotifyIconMask & i)
            PhAddNotifyIcon(i);
    }

    // Make sure we get closed late in the shutdown process.
    SetProcessShutdownParameters(0x100, 0);

    DelayedLoadCompleted = TRUE;
    //PostMessage(PhMainWndHandle, WM_PH_DELAYED_LOAD_COMPLETED, 0, 0);

    return STATUS_SUCCESS;
}

VOID PhpSaveWindowState()
{
    WINDOWPLACEMENT placement = { sizeof(placement) };

    GetWindowPlacement(PhMainWndHandle, &placement);

    if (placement.showCmd == SW_NORMAL)
        PhSetIntegerSetting(L"MainWindowState", SW_NORMAL);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetIntegerSetting(L"MainWindowState", SW_MAXIMIZE);
}

VOID PhpSaveAllSettings()
{
    PhSaveSettingsProcessTreeList();
    if (ServiceTreeListLoaded)
        PhSaveSettingsServiceTreeList();
    PhSaveListViewColumnsToSetting(L"NetworkListViewColumns", NetworkListViewHandle);

    PhSetIntegerSetting(L"IconMask", NotifyIconMask);
    PhSetIntegerSetting(L"IconNotifyMask", NotifyIconNotifyMask);

    PhSaveWindowPlacementToSetting(L"MainWindowPosition", L"MainWindowSize", PhMainWndHandle);

    PhpSaveWindowState();

    if (PhSettingsFileName)
        PhSaveSettings(PhSettingsFileName->Buffer);
}

VOID PhpSetCheckOpacityMenu(
    __in BOOLEAN AssumeAllUnchecked,
    __in ULONG Opacity
    )
{
    // The setting is stored backwards - 0 means opaque, 100 means transparent.
    CheckMenuRadioItem(
        PhMainWndMenuHandle,
        ID_OPACITY_10,
        ID_OPACITY_OPAQUE,
        ID_OPACITY_10 + (10 - Opacity / 10) - 1,
        MF_BYCOMMAND
        );
}

VOID PhpSetWindowOpacity(
    __in ULONG Opacity
    )
{
    if (Opacity == 0)
    {
        // Make things a bit faster by removing the WS_EX_LAYERED bit.
        PhSetWindowExStyle(PhMainWndHandle, WS_EX_LAYERED, 0);
        RedrawWindow(PhMainWndHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        return;
    }

    PhSetWindowExStyle(PhMainWndHandle, WS_EX_LAYERED, WS_EX_LAYERED);

    // Disallow opacity values of less than 10%.
    if (Opacity > 90)
        return;

    SetLayeredWindowAttributes(
        PhMainWndHandle,
        0,
        (BYTE)(255 * (100 - Opacity) / 100),
        LWA_ALPHA
        );
}

VOID PhpSelectTabPage(
    __in ULONG Index
    )
{
    TabCtrl_SetCurSel(TabControlHandle, Index);
    PhMainWndTabControlOnSelectionChanged();
}

VOID PhpPrepareForEarlyShutdown()
{
    PhpSaveAllSettings();
    PhMainWndExiting = TRUE;
}

VOID PhpCancelEarlyShutdown()
{
    PhMainWndExiting = FALSE;
}

VOID PhpNeedServiceTreeList()
{
    if (!ServiceTreeListLoaded)
    {
        ServiceTreeListLoaded = TRUE;

        PhLoadSettingsServiceTreeList();

        if (ServicesPendingList)
        {
            PPH_SERVICE_ITEM serviceItem;
            ULONG enumerationKey = 0;

            while (PhEnumPointerList(ServicesPendingList, &enumerationKey, (PPVOID)&serviceItem))
            {
                PhMainWndOnServiceAdded(serviceItem, 1);
            }

            // Force a re-draw.
            PhMainWndOnServicesUpdated();

            PhSwapReference(&ServicesPendingList, NULL);
        }
    }
}

BOOLEAN PhpProcessComputerCommand(
    __in ULONG Id
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
        PhUiRestartComputer(PhMainWndHandle);
        return TRUE;
    case ID_COMPUTER_SHUTDOWN:
        PhUiShutdownComputer(PhMainWndHandle);
        return TRUE;
    case ID_COMPUTER_POWEROFF:
        PhUiPoweroffComputer(PhMainWndHandle);
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhpProcessProcessPriorityCommand(
    __in ULONG Id,
    __in PPH_PROCESS_ITEM ProcessItem
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

    PhUiSetPriorityProcess(PhMainWndHandle, ProcessItem, priorityClass);

    return TRUE;
}

VOID PhpRefreshUsersMenu()
{
    HMENU menu;
    PSESSIONIDW sessions;
    ULONG numberOfSessions;
    ULONG i;
    ULONG j;
    MENUITEMINFO menuItemInfo = { sizeof(MENUITEMINFO) };

    menu = GetSubMenu(PhMainWndMenuHandle, 3);

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

static int __cdecl IconProcessesCpuUsageCompare(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_PROCESS_ITEM processItem1 = *(PPH_PROCESS_ITEM *)elem1;
    PPH_PROCESS_ITEM processItem2 = *(PPH_PROCESS_ITEM *)elem2;

    return -singlecmp(processItem1->CpuUsage, processItem2->CpuUsage);
}

static int __cdecl IconProcessesNameCompare(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_PROCESS_ITEM processItem1 = *(PPH_PROCESS_ITEM *)elem1;
    PPH_PROCESS_ITEM processItem2 = *(PPH_PROCESS_ITEM *)elem2;

    return PhCompareString(processItem1->ProcessName, processItem2->ProcessName, TRUE);
}

VOID PhpAddIconProcesses(
    __in PPH_EMENU_ITEM Menu,
    __in ULONG NumberOfProcesses
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
        PPH_EMENU_ITEM priorityMenu;
        HBITMAP iconBitmap;
        CLIENT_ID clientId;
        PPH_STRING clientIdName;
        PPH_STRING escapedName;

        processItem = processList->Items[i];

        // Priority

        priorityMenu = PhCreateEMenuItem(0, 0, L"Priority", NULL, NULL);

        PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_REALTIME, L"Real Time", NULL, NULL), -1);
        PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_HIGH, L"High", NULL, NULL), -1);
        PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_ABOVENORMAL, L"Above Normal", NULL, NULL), -1);
        PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_NORMAL, L"Normal", NULL, NULL), -1);
        PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_BELOWNORMAL, L"Below Normal", NULL, NULL), -1);
        PhInsertEMenuItem(priorityMenu, PhCreateEMenuItem(0, ID_PRIORITY_IDLE, L"Idle", NULL, NULL), -1);

        PhpSetProcessMenuPriorityChecks(priorityMenu, processItem, TRUE, FALSE, FALSE);

        // Process

        clientId.UniqueProcess = processItem->ProcessId;
        clientId.UniqueThread = NULL;

        clientIdName = PhGetClientIdName(&clientId);
        escapedName = PhEscapeStringForMenuPrefix(&clientIdName->sr);
        PhDereferenceObject(clientIdName);
        PhaDereferenceObject(escapedName);

        subMenu = PhCreateEMenuItem(
            0,
            0,
            escapedName->Buffer,
            NULL,
            processItem->ProcessId
            );

        // Menu icons only work properly on Vista and above.
        if (WindowsVersion >= WINDOWS_VISTA)
        {
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
        }
        else
        {
            iconBitmap = NULL;
        }

        subMenu->Bitmap = iconBitmap;
        subMenu->Flags |= PH_EMENU_BITMAP_OWNED; // automatically destroy the bitmap when necessary

        PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, ID_PROCESS_TERMINATE, L"Terminate", NULL, NULL), -1);
        PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, ID_PROCESS_SUSPEND, L"Suspend", NULL, NULL), -1);
        PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, ID_PROCESS_RESUME, L"Resume", NULL, NULL), -1);
        PhInsertEMenuItem(subMenu, priorityMenu, -1);
        PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, ID_PROCESS_PROPERTIES, L"Properties", NULL, NULL), -1);

        PhInsertEMenuItem(Menu, subMenu, -1);
    }

    PhDereferenceObject(processList);
    PhDereferenceObjects(processItems, numberOfProcessItems);
    PhFree(processItems);
}

VOID PhpShowIconContextMenu(
    __in POINT Location
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;
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
        PhpAddIconProcesses(item, numberOfProcesses);

    // Give plugins a chance to modify the menu.

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menuInfo.Menu = menu;
        menuInfo.OwnerWindow = PhMainWndHandle;

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
        PPH_EMENU_ITEM parentItem;
        HANDLE processId;
        BOOLEAN handled = FALSE;

        parentItem = item->Parent;

        while (parentItem)
        {
            if (parentItem->Context)
                processId = parentItem->Context;

            parentItem = parentItem->Parent;
        }

        if (PhPluginsEnabled && !handled)
            handled = PhPluginTriggerEMenuItem(PhMainWndHandle, item);

        if (!handled)
            handled = PhpProcessComputerCommand(item->Id);

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
            case ID_PROCESS_TERMINATE:
            case ID_PROCESS_SUSPEND:
            case ID_PROCESS_RESUME:
            case ID_PROCESS_PROPERTIES:
                {
                    PPH_PROCESS_ITEM processItem;

                    if (processItem = PhReferenceProcessItem(processId))
                    {
                        switch (item->Id)
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
                    PPH_PROCESS_ITEM processItem;

                    if (processItem = PhReferenceProcessItem(processId))
                    {
                        PhpProcessProcessPriorityCommand(item->Id, processItem);
                        PhDereferenceObject(processItem);
                    }
                    else
                    {
                        PhShowError(PhMainWndHandle, L"The process does not exist.");
                    }
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
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Flags
    )
{
    ULONG i;

    // Find a visible icon to display the balloon tip on.
    for (i = PH_ICON_MINIMUM; i != PH_ICON_MAXIMUM; i <<= 1)
    {
        if (NotifyIconMask & i)
        {
            PhShowBalloonTipNotifyIcon(i, Title, Text, 10, Flags);
            break;
        }
    }
}

BOOLEAN PhpCurrentUserProcessTreeFilter(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    )
{
    if (!ProcessNode->ProcessItem->UserName)
        return FALSE;

    if (!PhCurrentUserName)
        return FALSE;

    if (!PhEqualString(ProcessNode->ProcessItem->UserName, PhCurrentUserName, TRUE))
        return FALSE;

    return TRUE;
}

BOOLEAN PhpSignedProcessTreeFilter(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    )
{
    if (ProcessNode->ProcessItem->VerifyResult == VrTrusted)
        return FALSE;

    return TRUE;
}

PPH_PROCESS_ITEM PhpGetSelectedProcess()
{
    return PhGetSelectedProcessItem();
}

VOID PhpGetSelectedProcesses(
    __out PPH_PROCESS_ITEM **Processes,
    __out PULONG NumberOfProcesses
    )
{
    PhGetSelectedProcessItems(Processes, NumberOfProcesses);
}

VOID PhpShowProcessProperties(
    __in PPH_PROCESS_ITEM ProcessItem
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

PPH_SERVICE_ITEM PhpGetSelectedService()
{
    return PhGetSelectedServiceItem();
}

VOID PhpGetSelectedServices(
    __out PPH_SERVICE_ITEM **Services,
    __out PULONG NumberOfServices
    )
{
    PhGetSelectedServiceItems(Services, NumberOfServices);
}

PPH_NETWORK_ITEM PhpGetSelectedNetworkItem()
{
    return PhGetSelectedListViewItemParam(
        NetworkListViewHandle
        );
}

VOID PhpGetSelectedNetworkItems(
    __out PPH_NETWORK_ITEM **NetworkItems,
    __out PULONG NumberOfNetworkItems
    )
{
    PhGetSelectedListViewItemParams(
        NetworkListViewHandle,
        NetworkItems,
        NumberOfNetworkItems
        );
}

static VOID NTAPI ProcessAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

static VOID NTAPI ProcessModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_PROCESS_MODIFIED, 0, (LPARAM)processItem);
}

static VOID NTAPI ProcessRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // We already have a reference to the process item, so we don't need to 
    // reference it here.
    PostMessage(PhMainWndHandle, WM_PH_PROCESS_REMOVED, 0, (LPARAM)processItem);
}

static VOID NTAPI ProcessesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_PROCESSES_UPDATED, 0, 0);
}

static VOID NTAPI ProcessesUpdatedForIconsHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // We do icon updating on the provider thread so we don't block the main GUI when 
    // explorer is not responding.

    if (NotifyIconMask & PH_ICON_CPU_HISTORY)
        PhUpdateIconCpuHistory();
    if (NotifyIconMask & PH_ICON_IO_HISTORY)
        PhUpdateIconIoHistory();
    if (NotifyIconMask & PH_ICON_COMMIT_HISTORY)
        PhUpdateIconCommitHistory();
    if (NotifyIconMask & PH_ICON_PHYSICAL_HISTORY)
        PhUpdateIconPhysicalHistory();
    if (NotifyIconMask & PH_ICON_CPU_USAGE)
        PhUpdateIconCpuUsage();
}

static VOID NTAPI ServiceAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

static VOID NTAPI ServiceModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)Parameter;
    PPH_SERVICE_MODIFIED_DATA copy;

    copy = PhAllocateCopy(serviceModifiedData, sizeof(PH_SERVICE_MODIFIED_DATA));

    PostMessage(PhMainWndHandle, WM_PH_SERVICE_MODIFIED, 0, (LPARAM)copy);
}

static VOID NTAPI ServiceRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_SERVICE_REMOVED, 0, (LPARAM)serviceItem);
}

static VOID NTAPI ServicesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_SERVICES_UPDATED, 0, 0);
}

static VOID NTAPI NetworkItemAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

static VOID NTAPI NetworkItemModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_NETWORK_ITEM_MODIFIED, 0, (LPARAM)networkItem);
}

static VOID NTAPI NetworkItemRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_NETWORK_ITEM_REMOVED, 0, (LPARAM)networkItem);
}

static VOID NTAPI NetworkItemsUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_NETWORK_ITEMS_UPDATED, 0, 0);
}

VOID PhMainWndOnCreate()
{
    TabControlHandle = CreateWindow(
        WC_TABCONTROL,
        L"",
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

    ProcessTreeListHandle = CreateWindow(
        PH_TREENEW_CLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED,
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

    ServiceTreeListHandle = PhCreateTreeListControlEx(PhMainWndHandle, ID_MAINWND_SERVICETL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TLSTYLE_BORDER | TLSTYLE_ICONS);
    BringWindowToTop(ServiceTreeListHandle);

    NetworkListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_NETWORKLV);
    PhSetListViewStyle(NetworkListViewHandle, TRUE, TRUE);
    SendMessage(ListView_GetToolTips(NetworkListViewHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, 0x7fff);
    BringWindowToTop(NetworkListViewHandle);
    PhpReloadListViewFont();

    PhSetControlTheme(NetworkListViewHandle, L"explorer");

    PhAddListViewColumn(NetworkListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Process");
    PhAddListViewColumn(NetworkListViewHandle, 1, 1, 1, LVCFMT_LEFT, 120, L"Local Address");
    PhAddListViewColumn(NetworkListViewHandle, 2, 2, 2, LVCFMT_RIGHT, 50, L"Local Port");
    PhAddListViewColumn(NetworkListViewHandle, 3, 3, 3, LVCFMT_LEFT, 120, L"Remote Address");
    PhAddListViewColumn(NetworkListViewHandle, 4, 4, 4, LVCFMT_RIGHT, 50, L"Remote Port");
    PhAddListViewColumn(NetworkListViewHandle, 5, 5, 5, LVCFMT_LEFT, 45, L"Protocol");
    PhAddListViewColumn(NetworkListViewHandle, 6, 6, 6, LVCFMT_LEFT, 70, L"State");

    if (WINDOWS_HAS_SERVICE_TAGS)
        PhAddListViewColumn(NetworkListViewHandle, 7, 7, 7, LVCFMT_LEFT, 80, L"Owner");

    PhProcessTreeListInitialization();
    PhInitializeProcessTreeList(ProcessTreeListHandle);

    PhServiceTreeListInitialization();
    PhInitializeServiceTreeList(ServiceTreeListHandle);

    PhSetExtendedListViewWithSettings(NetworkListViewHandle);
    ExtendedListView_SetStateHighlighting(NetworkListViewHandle, TRUE);

    PhRegisterCallback(
        &PhProcessAddedEvent,
        ProcessAddedHandler,
        NULL,
        &ProcessAddedRegistration
        );
    PhRegisterCallback(
        &PhProcessModifiedEvent,
        ProcessModifiedHandler,
        NULL,
        &ProcessModifiedRegistration
        );
    PhRegisterCallback(
        &PhProcessRemovedEvent,
        ProcessRemovedHandler,
        NULL,
        &ProcessRemovedRegistration
        );
    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        ProcessesUpdatedHandler,
        NULL,
        &ProcessesUpdatedRegistration
        );
    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        ProcessesUpdatedForIconsHandler,
        NULL,
        &ProcessesUpdatedForIconsRegistration
        );

    PhRegisterCallback(
        &PhServiceAddedEvent,
        ServiceAddedHandler,
        NULL,
        &ServiceAddedRegistration
        );
    PhRegisterCallback(
        &PhServiceModifiedEvent,
        ServiceModifiedHandler,
        NULL,
        &ServiceModifiedRegistration
        );
    PhRegisterCallback(
        &PhServiceRemovedEvent,
        ServiceRemovedHandler,
        NULL,
        &ServiceRemovedRegistration
        );
    PhRegisterCallback(
        &PhServicesUpdatedEvent,
        ServicesUpdatedHandler,
        NULL,
        &ServicesUpdatedRegistration
        );

    PhRegisterCallback(
        &PhNetworkItemAddedEvent,
        NetworkItemAddedHandler,
        NULL,
        &NetworkItemAddedRegistration
        );
    PhRegisterCallback(
        &PhNetworkItemModifiedEvent,
        NetworkItemModifiedHandler,
        NULL,
        &NetworkItemModifiedRegistration
        );
    PhRegisterCallback(
        &PhNetworkItemRemovedEvent,
        NetworkItemRemovedHandler,
        NULL,
        &NetworkItemRemovedRegistration
        );
    PhRegisterCallback(
        &PhNetworkItemsUpdatedEvent,
        NetworkItemsUpdatedHandler,
        NULL,
        &NetworkItemsUpdatedRegistration
        );
}

VOID PhpApplyLayoutPadding(
    __inout PRECT Rect,
    __in PRECT Padding
    )
{
    Rect->left += Padding->left;
    Rect->top += Padding->top;
    Rect->right -= Padding->right;
    Rect->bottom -= Padding->bottom;
}

ULONG_PTR PhpMainWndAddMenuItem(
    __in PPH_PLUGIN Plugin,
    __in HMENU ParentMenu,
    __in_opt PWSTR InsertAfter,
    __in ULONG Flags,
    __in ULONG Id,
    __in PWSTR Text,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem;
    HMENU menu;
    ULONG insertIndex;
    ULONG textCount;
    WCHAR textBuffer[256];
    MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };
    HMENU subMenu;

    textCount = (ULONG)wcslen(Text);

    if (textCount > 128)
        return FALSE;

    menuItem = PhAllocate(sizeof(PH_PLUGIN_MENU_ITEM));
    memset(menuItem, 0, sizeof(PH_PLUGIN_MENU_ITEM));
    menuItem->Plugin = Plugin;
    menuItem->Id = Id;
    menuItem->Context = Context;
    menuItem->ParentMenu = ParentMenu;
    menuItem->SubMenu = NULL;

    menu = ParentMenu;

    if (InsertAfter)
    {
        ULONG count;

        menuItemInfo.fMask = MIIM_STRING;
        menuItemInfo.dwTypeData = textBuffer;
        menuItemInfo.cch = sizeof(textBuffer) / sizeof(WCHAR);

        insertIndex = 0;
        count = GetMenuItemCount(menu);

        if (count == -1)
            return FALSE;

        for (insertIndex = 0; insertIndex < count; insertIndex++)
        {
            if (GetMenuItemInfo(menu, insertIndex, TRUE, &menuItemInfo) && menuItemInfo.dwTypeData)
            {
                if (PhCompareUnicodeStringZIgnoreMenuPrefix(InsertAfter, menuItemInfo.dwTypeData, TRUE, TRUE) == 0)
                {
                    insertIndex++;
                    break;
                }
            }
        }
    }
    else
    {
        insertIndex = 0;
    }

    if (textCount == 1 && Text[0] == '-')
    {
        menuItem->RealId = 0;

        menuItemInfo.fMask = 0;
        menuItemInfo.fType = MFT_SEPARATOR;
    }
    else
    {
        menuItem->RealId = PhPluginReserveIds(1);

        menuItemInfo.fMask = MIIM_DATA | MIIM_ID | MIIM_STRING;
        menuItemInfo.wID = menuItem->RealId;
        menuItemInfo.dwItemData = (ULONG_PTR)menuItem;
        menuItemInfo.dwTypeData = Text;

        if (Flags & PH_MENU_ITEM_SUB_MENU)
        {
            subMenu = CreatePopupMenu();
            menuItemInfo.fMask |= MIIM_SUBMENU;
            menuItemInfo.hSubMenu = subMenu;
            menuItem->SubMenu = subMenu;
        }
    }

    InsertMenuItem(menu, insertIndex, TRUE, &menuItemInfo);

    if (Flags & PH_MENU_ITEM_RETURN_MENU)
    {
        return (ULONG_PTR)menuItem;
    }
    else
    {
        if (Flags & PH_MENU_ITEM_SUB_MENU)
            return (ULONG_PTR)subMenu;
        else
            return TRUE;
    }
}

PPH_ADDITIONAL_TAB_PAGE PhpAddTabPage(
    __in PPH_ADDITIONAL_TAB_PAGE TabPage
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
    PhMainWndTabControlOnLayout(&deferHandle);
    EndDeferWindowPos(deferHandle);

    return newTabPage;
}

VOID PhMainWndOnLayout(
    __inout HDWP *deferHandle
    )
{
    RECT rect;

    // Resize the tab control.
    // Don't defer the resize. The tab control doesn't repaint properly.

    GetClientRect(PhMainWndHandle, &rect);
    PhpApplyLayoutPadding(&rect, &LayoutPadding);

    SetWindowPos(TabControlHandle, NULL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOACTIVATE | SWP_NOZORDER);
    UpdateWindow(TabControlHandle);

    PhMainWndTabControlOnLayout(deferHandle);
}

VOID PhMainWndTabControlOnLayout(
    __inout HDWP *deferHandle
    )
{
    RECT rect;
    INT selectedIndex;

    GetClientRect(PhMainWndHandle, &rect);
    PhpApplyLayoutPadding(&rect, &LayoutPadding);
    TabCtrl_AdjustRect(TabControlHandle, FALSE, &rect);

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == ProcessesTabIndex)
    {
        *deferHandle = DeferWindowPos(*deferHandle, ProcessTreeListHandle, NULL, 
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else if (selectedIndex == ServicesTabIndex)
    {
        *deferHandle = DeferWindowPos(*deferHandle, ServiceTreeListHandle, NULL,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else if (selectedIndex == NetworkTabIndex)
    {
        *deferHandle = DeferWindowPos(*deferHandle, NetworkListViewHandle, NULL,
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
                }

                if (tabPage->WindowHandle)
                {
                    *deferHandle = DeferWindowPos(*deferHandle, tabPage->WindowHandle, NULL,
                        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                        SWP_NOACTIVATE | SWP_NOZORDER);
                }

                break;
            }
        }
    }
}

VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    )
{
    if (Header->code == TCN_SELCHANGE)
    {
        PhMainWndTabControlOnSelectionChanged();
    }
}

VOID PhMainWndTabControlOnSelectionChanged()
{
    INT selectedIndex;
    HDWP deferHandle;

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    deferHandle = BeginDeferWindowPos(1);
    PhMainWndTabControlOnLayout(&deferHandle);
    EndDeferWindowPos(deferHandle);

    // Built-in tabs

    if (selectedIndex == ServicesTabIndex)
        PhpNeedServiceTreeList();

    if (selectedIndex == NetworkTabIndex)
    {
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
    ShowWindow(NetworkListViewHandle, selectedIndex == NetworkTabIndex ? SW_SHOW : SW_HIDE);

    // Additional tabs

    if (AdditionalTabPageList)
    {
        ULONG i;

        for (i = 0; i < AdditionalTabPageList->Count; i++)
        {
            PPH_ADDITIONAL_TAB_PAGE tabPage = AdditionalTabPageList->Items[i];

            ShowWindow(tabPage->WindowHandle, selectedIndex == tabPage->Index ? SW_SHOW : SW_HIDE);
        }
    }
}

BOOL CALLBACK PhpEnumProcessWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    ULONG processId;
    HWND parentWindow;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    GetWindowThreadProcessId(hwnd, &processId);

    if (
        processId == (ULONG)lParam &&
        !((parentWindow = GetParent(hwnd)) && IsWindowVisible(parentWindow)) && // skip windows with a visible parent
        GetWindowTextLength(hwnd) != 0
        )
    {
        SelectedProcessWindowHandle = hwnd;
        return FALSE;
    }

    return TRUE;
}

VOID PhpSetProcessMenuPriorityChecks(
    __in PPH_EMENU Menu,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN SetPriority,
    __in BOOLEAN SetIoPriority,
    __in BOOLEAN SetPagePriority
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
        Process->ProcessId
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

VOID PhpInitializeProcessMenu(
    __in PPH_EMENU Menu,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
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
            ID_PROCESS_REDUCEWORKINGSET,
            ID_PROCESS_COPY
        };
        ULONG i;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

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
        PhpSetProcessMenuPriorityChecks(Menu, Processes[0], TRUE, TRUE, TRUE);
    }

    item = PhFindEMenuItem(Menu, 0, L"Window", 0);

    if (item)
    {
        // Window menu
        if (NumberOfProcesses == 1)
        {
            WINDOWPLACEMENT placement = { sizeof(placement) };

            // Get a handle to the process' top-level window (if any).
            SelectedProcessWindowHandle = NULL;
            EnumWindows(PhpEnumProcessWindowsProc, (ULONG)Processes[0]->ProcessId);

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
    __in POINT Location
    )
{
    PPH_PROCESS_ITEM *processes;
    ULONG numberOfProcesses;

    PhpGetSelectedProcesses(&processes, &numberOfProcesses);

    if (numberOfProcesses != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_PROCESS), 0);
        PhSetFlagsEMenuItem(menu, ID_PROCESS_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeProcessMenu(menu, processes, numberOfProcesses);

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_MENU_INFORMATION menuInfo;

            menuInfo.Menu = menu;
            menuInfo.OwnerWindow = PhMainWndHandle;
            menuInfo.u.Process.Processes = processes;
            menuInfo.u.Process.NumberOfProcesses = numberOfProcesses;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing), &menuInfo);
        }

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

            if (PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(PhMainWndHandle, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(processes);
}

VOID PhpInitializeServiceMenu(
    __in PPH_EMENU Menu,
    __in PPH_SERVICE_ITEM *Services,
    __in ULONG NumberOfServices
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
    __in POINT Location
    )
{
    PPH_SERVICE_ITEM *services;
    ULONG numberOfServices;

    PhpGetSelectedServices(&services, &numberOfServices);

    if (numberOfServices != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_SERVICE), 0);
        PhSetFlagsEMenuItem(menu, ID_SERVICE_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeServiceMenu(menu, services, numberOfServices);

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_MENU_INFORMATION menuInfo;

            menuInfo.Menu = menu;
            menuInfo.OwnerWindow = PhMainWndHandle;
            menuInfo.u.Service.Services = services;
            menuInfo.u.Service.NumberOfServices = numberOfServices;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceMenuInitializing), &menuInfo);
        }

        ClientToScreen(ServiceTreeListHandle, &Location);

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

            if (PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(PhMainWndHandle, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(services);
}

VOID PhpInitializeNetworkMenu(
    __in PPH_EMENU Menu,
    __in PPH_NETWORK_ITEM *NetworkItems,
    __in ULONG NumberOfNetworkItems
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

VOID PhMainWndNetworkListViewOnNotify(
    __in LPNMHDR Header
    )
{
    switch (Header->code)
    {
    case NM_DBLCLK:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_GOTOPROCESS, 0);
        }
        break;
    case LVN_KEYDOWN:
        {
            LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)Header;

            switch (keyDown->wVKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_COPY, 0);
                break;
            case VK_RETURN:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_GOTOPROCESS, 0);
                break;
            }
        }
        break;
    case LVN_GETINFOTIP:
        {
            LPNMLVGETINFOTIP getInfoTip = (LPNMLVGETINFOTIP)Header;
            PPH_NETWORK_ITEM networkItem;

            if (getInfoTip->iSubItem == 0 && PhGetListViewItemParam(
                NetworkListViewHandle,
                getInfoTip->iItem,
                &networkItem
                ))
            {
                PPH_PROCESS_ITEM processItem;
                PPH_STRING tip;

                if (processItem = PhReferenceProcessItem(networkItem->ProcessId))
                {
                    tip = PhGetProcessTooltipText(processItem);
                    PhCopyListViewInfoTip(getInfoTip, &tip->sr);  
                    PhDereferenceObject(tip);
                    PhDereferenceObject(processItem);
                }
            }
        }
        break;
    }
}

VOID PhMainWndNetworkListViewOnContextMenu(
    __in LPARAM lParam
    )
{
    POINT point;
    PPH_NETWORK_ITEM *networkItems;
    ULONG numberOfNetworkItems;

    point.x = (SHORT)LOWORD(lParam);
    point.y = (SHORT)HIWORD(lParam);

    if (point.x == -1 && point.y == -1)
        PhGetListViewContextMenuPoint(NetworkListViewHandle, &point);

    PhpGetSelectedNetworkItems(&networkItems, &numberOfNetworkItems);

    if (numberOfNetworkItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_NETWORK), 0);
        PhSetFlagsEMenuItem(menu, ID_NETWORK_GOTOPROCESS, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeNetworkMenu(menu, networkItems, numberOfNetworkItems);

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_MENU_INFORMATION menuInfo;

            menuInfo.Menu = menu;
            menuInfo.OwnerWindow = PhMainWndHandle;
            menuInfo.u.Network.NetworkItems = networkItems;
            menuInfo.u.Network.NumberOfNetworkItems = numberOfNetworkItems;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            point.x,
            point.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            if (PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(PhMainWndHandle, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(networkItems);
}

BOOLEAN PhpPluginNotifyEvent(
    __in ULONG Type,
    __in PVOID Parameter
    )
{
    PH_PLUGIN_NOTIFY_EVENT notifyEvent;

    notifyEvent.Type = Type;
    notifyEvent.Handled = FALSE;
    notifyEvent.Parameter = Parameter;

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNotifyEvent), &notifyEvent);

    return notifyEvent.Handled;
}

VOID PhMainWndOnProcessAdded(
    __in __assumeRefs(1) PPH_PROCESS_ITEM ProcessItem,
    __in ULONG RunId
    )
{
    PPH_PROCESS_NODE processNode;

    if (!ProcessesNeedsRedraw)
    {
        TreeList_SetRedraw(ProcessTreeListHandle, FALSE);
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
            ProcessItem->ProcessName,
            parentProcessId,
            parentName
            );

        if (NotifyIconNotifyMask & PH_NOTIFY_PROCESS_CREATE)
        {
            if (!PhPluginsEnabled || !PhpPluginNotifyEvent(PH_NOTIFY_PROCESS_CREATE, ProcessItem))
            {
                PhShowIconNotification(L"Process Created", PhaFormatString(
                    L"The process %s (%u) was created by %s (%u)",
                    ProcessItem->ProcessName->Buffer,
                    (ULONG)ProcessItem->ProcessId,
                    PhGetStringOrDefault(parentName, L"Unknown Process"),
                    (ULONG)ProcessItem->ParentProcessId
                    )->Buffer, NIIF_INFO);
            }
        }

        if (parentProcess)
            PhDereferenceObject(parentProcess);
    }

    // PhCreateProcessNode has its own reference.
    PhDereferenceObject(ProcessItem);
}

VOID PhMainWndOnProcessModified(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhUpdateProcessNode(PhFindProcessNode(ProcessItem->ProcessId));

    if (SignedFilterEntry)
        PhApplyProcessTreeFilters();
}

VOID PhMainWndOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    if (!ProcessesNeedsRedraw)
    {
        TreeList_SetRedraw(ProcessTreeListHandle, FALSE);
        ProcessesNeedsRedraw = TRUE;
    }

    PhLogProcessEntry(PH_LOG_ENTRY_PROCESS_DELETE, ProcessItem->ProcessId, ProcessItem->ProcessName, NULL, NULL);

    if (NotifyIconNotifyMask & PH_NOTIFY_PROCESS_DELETE)
    {
        if (!PhPluginsEnabled || !PhpPluginNotifyEvent(PH_NOTIFY_PROCESS_DELETE, ProcessItem))
        {
            PhShowIconNotification(L"Process Terminated", PhaFormatString(
                L"The process %s (%u) was terminated.",
                ProcessItem->ProcessName->Buffer,
                (ULONG)ProcessItem->ProcessId
                )->Buffer, NIIF_INFO);
        }
    }

    PhRemoveProcessNode(PhFindProcessNode(ProcessItem->ProcessId));
}

VOID PhMainWndOnProcessesUpdated()
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
        TreeList_SetRedraw(ProcessTreeListHandle, TRUE);
        ProcessesNeedsRedraw = FALSE;
    }
}

VOID PhMainWndOnServiceAdded(
    __in __assumeRefs(1) PPH_SERVICE_ITEM ServiceItem,
    __in ULONG RunId
    )
{
    PPH_SERVICE_NODE serviceNode;

    if (ServiceTreeListLoaded)
    {
        if (!ServicesNeedsRedraw)
        {
            TreeList_SetRedraw(ServiceTreeListHandle, FALSE);
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
            if (!PhPluginsEnabled || !PhpPluginNotifyEvent(PH_NOTIFY_SERVICE_CREATE, ServiceItem))
            {
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

VOID PhMainWndOnServiceModified(
    __in PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    )
{
    PH_SERVICE_CHANGE serviceChange;
    UCHAR logEntryType;

    if (ServiceTreeListLoaded)
    {
        //if (!ServicesNeedsRedraw)
        //{
        //    TreeList_SetRedraw(ServiceTreeListHandle, FALSE);
        //    ServicesNeedsRedraw = TRUE;
        //}

        PhUpdateServiceNode(PhFindServiceNode(ServiceModifiedData->Service));
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
            if (!PhPluginsEnabled || !PhpPluginNotifyEvent(PH_NOTIFY_SERVICE_START, serviceItem))
            {
                PhShowIconNotification(L"Service Started", PhaFormatString(
                    L"The service %s (%s) has been started.",
                    serviceItem->Name->Buffer,
                    serviceItem->DisplayName->Buffer
                    )->Buffer, NIIF_INFO);
            }
        }
        else if (serviceChange == ServiceStopped && (NotifyIconNotifyMask & PH_NOTIFY_SERVICE_STOP))
        {
            if (!PhPluginsEnabled || !PhpPluginNotifyEvent(PH_NOTIFY_SERVICE_STOP, serviceItem))
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

VOID PhMainWndOnServiceRemoved(
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    if (ServiceTreeListLoaded)
    {
        if (!ServicesNeedsRedraw)
        {
            TreeList_SetRedraw(ServiceTreeListHandle, FALSE);
            ServicesNeedsRedraw = TRUE;
        }
    }

    PhLogServiceEntry(PH_LOG_ENTRY_SERVICE_DELETE, ServiceItem->Name, ServiceItem->DisplayName);

    if (NotifyIconNotifyMask & PH_NOTIFY_SERVICE_CREATE)
    {
        if (!PhPluginsEnabled || !PhpPluginNotifyEvent(PH_NOTIFY_SERVICE_DELETE, ServiceItem))
        {
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

VOID PhMainWndOnServicesUpdated()
{
    if (ServiceTreeListLoaded)
    {
        PhTickServiceNodes();

        if (ServicesNeedsRedraw)
        {
            TreeList_SetRedraw(ServiceTreeListHandle, TRUE);
            ServicesNeedsRedraw = FALSE;
        }
    }
}

PWSTR PhpGetNetworkItemProcessName(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    PPH_STRING string;
    PH_FORMAT format[4];

    if (!NetworkItem->ProcessId)
        return L"Waiting Connections";

    PhInitFormatS(&format[1], L" (");
    PhInitFormatU(&format[2], (ULONG)NetworkItem->ProcessId);
    PhInitFormatC(&format[3], ')');

    if (NetworkItem->ProcessName)
        PhInitFormatSR(&format[0], NetworkItem->ProcessName->sr);
    else
        PhInitFormatS(&format[0], L"Unknown Process");

    string = PhFormat(format, 4, 96);
    PhaDereferenceObject(string);

    return string->Buffer;
}

VOID PhpSetNetworkItemImageIndex(
    __in INT ItemIndex,
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    INT imageIndex;

    if (NetworkItem->ProcessIconValid && NetworkItem->ProcessIcon)
    {
        imageIndex = PhImageListWrapperAddIcon(&NetworkImageListWrapper, NetworkItem->ProcessIcon);
    }
    else
    {
        imageIndex = 0;
    }

    PhSetListViewItemImageIndex(NetworkListViewHandle, ItemIndex, imageIndex);
}

VOID PhMainWndOnNetworkItemAdded(
    __in ULONG RunId,
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    INT lvItemIndex;

    if (!NetworkNeedsRedraw)
    {
        ExtendedListView_SetRedraw(NetworkListViewHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    // Add a reference for the pointer being stored in the list view item.
    PhReferenceObject(NetworkItem);

    if (RunId == 1) ExtendedListView_SetStateHighlighting(NetworkListViewHandle, FALSE);
    lvItemIndex = PhAddListViewItem(
        NetworkListViewHandle,
        MAXINT,
        PhpGetNetworkItemProcessName(NetworkItem),
        NetworkItem
        );
    if (RunId == 1) ExtendedListView_SetStateHighlighting(NetworkListViewHandle, TRUE);
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 1,
        PhGetStringOrDefault(NetworkItem->LocalHostString, NetworkItem->LocalAddressString));
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 2, NetworkItem->LocalPortString);
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 3,
        PhGetStringOrDefault(NetworkItem->RemoteHostString, NetworkItem->RemoteAddressString));
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 4, NetworkItem->RemotePortString);
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 5, PhGetProtocolTypeName(NetworkItem->ProtocolType));
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 6,
        (NetworkItem->ProtocolType & PH_TCP_PROTOCOL_TYPE) ? PhGetTcpStateName(NetworkItem->State) : NULL);

    if (WINDOWS_HAS_SERVICE_TAGS)
        PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 7, PhGetString(NetworkItem->OwnerName));

    if (!NetworkImageListWrapper.Handle)
    {
        HICON icon;

        PhGetStockApplicationIcon(&icon, NULL);
        PhInitializeImageListWrapper(&NetworkImageListWrapper, PhSmallIconSize.X, PhSmallIconSize.Y, ILC_COLOR32 | ILC_MASK);
        PhImageListWrapperAddIcon(&NetworkImageListWrapper, icon);
        ListView_SetImageList(NetworkListViewHandle, NetworkImageListWrapper.Handle, LVSIL_SMALL);
    }

    PhpSetNetworkItemImageIndex(lvItemIndex, NetworkItem);

    NetworkNeedsSort = TRUE;
}

VOID PhMainWndOnNetworkItemModified(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    INT lvItemIndex;
    INT imageIndex;

    if (!NetworkNeedsRedraw)
    {
        ExtendedListView_SetRedraw(NetworkListViewHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    lvItemIndex = PhFindListViewItemByParam(NetworkListViewHandle, -1, NetworkItem);
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 0,
        PhpGetNetworkItemProcessName(NetworkItem));
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 1,
        PhGetStringOrDefault(NetworkItem->LocalHostString, NetworkItem->LocalAddressString));
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 3,
        PhGetStringOrDefault(NetworkItem->RemoteHostString, NetworkItem->RemoteAddressString));
    PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 6,
        (NetworkItem->ProtocolType & PH_TCP_PROTOCOL_TYPE) ? PhGetTcpStateName(NetworkItem->State) : NULL);

    if (WINDOWS_HAS_SERVICE_TAGS)
        PhSetListViewSubItem(NetworkListViewHandle, lvItemIndex, 7, PhGetString(NetworkItem->OwnerName));

    // Only set a new icon if we didn't have a proper one before.
    if (PhGetListViewItemImageIndex(
        NetworkListViewHandle,
        lvItemIndex,
        &imageIndex
        ) && imageIndex == 0)
    {
        PhpSetNetworkItemImageIndex(lvItemIndex, NetworkItem);
    }

    NetworkNeedsSort = TRUE;
}

VOID PhMainWndOnNetworkItemRemoved(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    INT lvItemIndex;
    INT imageIndex;

    if (!NetworkNeedsRedraw)
    {
        ExtendedListView_SetRedraw(NetworkListViewHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    lvItemIndex = PhFindListViewItemByParam(NetworkListViewHandle, -1, NetworkItem);

    if (PhGetListViewItemImageIndex(
        NetworkListViewHandle,
        lvItemIndex,
        &imageIndex
        ) && imageIndex != 0)
    {
        PhImageListWrapperRemove(&NetworkImageListWrapper, imageIndex);
        // Set to a generic icon so we don't get random icons popping up due to 
        // the delay caused by state highlighting.
        PhSetListViewItemImageIndex(NetworkListViewHandle, lvItemIndex, 0);
    }

    PhRemoveListViewItem(
        NetworkListViewHandle,
        lvItemIndex
        );
    // Remove the reference we added in PhMainWndOnNetworkItemAdded.
    PhDereferenceObject(NetworkItem);
}

VOID PhMainWndOnNetworkItemsUpdated()
{
    ExtendedListView_Tick(NetworkListViewHandle);

    if (NetworkNeedsSort)
    {
        ExtendedListView_SortItems(NetworkListViewHandle);
        NetworkNeedsSort = FALSE;
    }

    if (NetworkNeedsRedraw)
    {
        ExtendedListView_SetRedraw(NetworkListViewHandle, TRUE);
        NetworkNeedsRedraw = FALSE;
    }
}

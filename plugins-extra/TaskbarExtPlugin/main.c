/*
 * Process Hacker Extra Plugins -
 *   Taskbar Extensions
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

static TASKBAR_ICON TaskbarIconType = TASKBAR_ICON_NONE;
static ULONG ProcessesUpdatedCount = 0;
static BOOLEAN TaskbarButtonsCreated = FALSE;
static UINT TaskbarButtonCreatedMsgId = 0;
static ITaskbarList3* TaskbarListClass = NULL;
static HICON BlackIcon = NULL;
static HIMAGELIST ButtonsImageList = NULL;
static THUMBBUTTON ButtonsArray[4] = { 0 }; // maximum 8

static VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    HICON overlayIcon = NULL;
    ULONG taskbarIconType = TASKBAR_ICON_NONE;
    PH_PLUGIN_SYSTEM_STATISTICS statistics;

    ProcessesUpdatedCount++;
    if (ProcessesUpdatedCount < 2)
        return;

    taskbarIconType = PhGetIntegerSetting(SETTING_NAME_TASKBAR_ICON_TYPE);

    // Check if we need to clear the icon...
    if (taskbarIconType != TaskbarIconType && taskbarIconType == TASKBAR_ICON_NONE)
    {
        // Clear the icon...
        if (TaskbarListClass)
        {
            ITaskbarList3_SetOverlayIcon(TaskbarListClass, PhMainWndHandle, NULL, NULL);
        }
    }

    TaskbarIconType = taskbarIconType;

    PhPluginGetSystemStatistics(&statistics);

    switch (TaskbarIconType)
    {
    case TASKBAR_ICON_CPU_HISTORY:
        overlayIcon = PhUpdateIconCpuHistory(statistics);
        break;
    case TASKBAR_ICON_IO_HISTORY:
        overlayIcon = PhUpdateIconIoHistory(statistics);
        break;
    case TASKBAR_ICON_COMMIT_HISTORY:
        overlayIcon = PhUpdateIconCommitHistory(statistics);
        break;
    case TASKBAR_ICON_PHYSICAL_HISTORY:
        overlayIcon = PhUpdateIconPhysicalHistory(statistics);
        break;
    case TASKBAR_ICON_CPU_USAGE:
        overlayIcon = PhUpdateIconCpuUsage(statistics);
        break;
    }

    if (overlayIcon)
    {
        if (TaskbarListClass)
        {
            ITaskbarList3_SetOverlayIcon(TaskbarListClass, PhMainWndHandle, overlayIcon, NULL);
        }

        DestroyIcon(overlayIcon);
    }
}

static LRESULT CALLBACK MainWndSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    if (uMsg == TaskbarButtonCreatedMsgId)
    {
        if (!TaskbarButtonsCreated)
        {
            BlackIcon = PhGetBlackIcon();
            ButtonsImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
            ImageList_SetImageCount(ButtonsImageList, 4);
            PhSetImageListBitmap(ButtonsImageList, 0, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE_BMP));
            PhSetImageListBitmap(ButtonsImageList, 1, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND_BMP));
            PhSetImageListBitmap(ButtonsImageList, 2, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_BMP));
            PhSetImageListBitmap(ButtonsImageList, 3, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO_BMP));

            ButtonsArray[0].dwMask = THB_FLAGS | THB_BITMAP | THB_TOOLTIP;
            ButtonsArray[0].dwFlags = THBF_ENABLED | THBF_DISMISSONCLICK;
            ButtonsArray[0].iId = PHAPP_ID_VIEW_SYSTEMINFORMATION;
            ButtonsArray[0].iBitmap = 0;
            wcscpy_s(ButtonsArray[0].szTip, _countof(ButtonsArray[0].szTip), L"System Information");

            ButtonsArray[1].dwMask = THB_FLAGS | THB_BITMAP | THB_TOOLTIP;
            ButtonsArray[1].dwFlags = THBF_ENABLED | THBF_DISMISSONCLICK;
            ButtonsArray[1].iId = PHAPP_ID_HACKER_FINDHANDLESORDLLS;
            ButtonsArray[1].iBitmap = 1;
            wcscpy_s(ButtonsArray[1].szTip, _countof(ButtonsArray[1].szTip), L"Find Handles or DLLs");

            ButtonsArray[2].dwMask = THB_FLAGS | THB_BITMAP | THB_TOOLTIP;
            ButtonsArray[2].dwFlags = THBF_ENABLED | THBF_DISMISSONCLICK;
            ButtonsArray[2].iId = PHAPP_ID_HELP_LOG;
            ButtonsArray[2].iBitmap = 2;
            wcscpy_s(ButtonsArray[2].szTip, _countof(ButtonsArray[2].szTip), L"Application Log");

            ButtonsArray[3].dwMask = THB_FLAGS | THB_BITMAP | THB_TOOLTIP;
            ButtonsArray[3].dwFlags = THBF_ENABLED | THBF_DISMISSONCLICK;
            ButtonsArray[3].iId = PHAPP_ID_TOOLS_INSPECTEXECUTABLEFILE;
            ButtonsArray[3].iBitmap = 3;
            wcscpy_s(ButtonsArray[3].szTip, _countof(ButtonsArray[3].szTip), L"Inspect Executable File");

            TaskbarButtonsCreated = TRUE;
        }

        if (TaskbarListClass)
        {
            // Set the ThumbBar image list
            ITaskbarList3_ThumbBarSetImageList(TaskbarListClass, PhMainWndHandle, ButtonsImageList);
            // Set the ThumbBar buttons array
            ITaskbarList3_ThumbBarAddButtons(TaskbarListClass, PhMainWndHandle, _countof(ButtonsArray), ButtonsArray);

            if (TaskbarIconType != TASKBAR_ICON_NONE)
            {
                // Set the initial ThumbBar icon
                ITaskbarList3_SetOverlayIcon(TaskbarListClass, PhMainWndHandle, BlackIcon, NULL);
            }
        }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (WindowsVersion < WINDOWS_7)
        return;

    // Update settings
    TaskbarIconType = PhGetIntegerSetting(SETTING_NAME_TASKBAR_ICON_TYPE);

    // Get the TaskbarButtonCreated Id
    TaskbarButtonCreatedMsgId = RegisterWindowMessage(L"TaskbarButtonCreated");

    // Allow the TaskbarButtonCreated message to pass through UIPI.
    ChangeWindowMessageFilter(TaskbarButtonCreatedMsgId, MSGFLT_ALLOW);
    // Allow WM_COMMAND messages to pass through UIPI (Required for ThumbBar buttons if elevated...TODO: Review security.)
    ChangeWindowMessageFilter(WM_COMMAND, MSGFLT_ALLOW);

    // Set the process-wide AppUserModelID
    SetCurrentProcessExplicitAppUserModelID(L"ProcessHacker2");

    if (SUCCEEDED(CoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, &TaskbarListClass)))
    {
        if (!SUCCEEDED(ITaskbarList3_HrInit(TaskbarListClass)))
        {
            ITaskbarList3_Release(TaskbarListClass);
            TaskbarListClass = NULL;
        }
    }

    PhRegisterCallback(&PhProcessesUpdatedEvent, ProcessesUpdatedCallback, NULL, &ProcessesUpdatedCallbackRegistration);
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

static VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ShowOptionsDialog((HWND)Parameter);
}

static VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    if (WindowsVersion < WINDOWS_7)
        return;

    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0); 
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
                { IntegerSettingType, SETTING_NAME_TASKBAR_ICON_TYPE, L"1" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Taskbar Extensions";
            info->Author = L"dmex";
            info->Description = L"Plugin for Taskbar ThumbBar buttons and Overlay icon support.";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
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

            PhAddSettings(settings, _countof(settings));
        }
        break;
    }

    return TRUE;
}
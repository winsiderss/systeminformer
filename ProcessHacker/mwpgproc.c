/*
 * Process Hacker -
 *   Main window: Processes tab
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2018 dmex
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

#include <phapp.h>
#include <mainwnd.h>

#include <shellapi.h>

#include <cpysave.h>
#include <emenu.h>
#include <verify.h>
#include <phsettings.h>

#include <actions.h>
#include <colsetmgr.h>
#include <phplug.h>
#include <procprp.h>
#include <procprv.h>
#include <proctree.h>
#include <settings.h>

#include <mainwndp.h>

PPH_MAIN_TAB_PAGE PhMwpProcessesPage;
HWND PhMwpProcessTreeNewHandle;
HWND PhMwpSelectedProcessWindowHandle;
BOOLEAN PhMwpSelectedProcessVirtualizationEnabled;
PH_PROVIDER_EVENT_QUEUE PhMwpProcessEventQueue;

static PH_CALLBACK_REGISTRATION ProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION ProcessModifiedRegistration;
static PH_CALLBACK_REGISTRATION ProcessRemovedRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static ULONG NeedsSelectPid = 0;
static PPH_PROCESS_NODE ProcessToScrollTo = NULL;
static PPH_TN_FILTER_ENTRY CurrentUserFilterEntry = NULL;
static PPH_TN_FILTER_ENTRY SignedFilterEntry = NULL;

BOOLEAN PhMwpProcessesPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MainTabPageCreate:
        {
            PhMwpProcessesPage = Page;

            PhInitializeProviderEventQueue(&PhMwpProcessEventQueue, 100);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent),
                PhMwpProcessAddedHandler,
                NULL,
                &ProcessAddedRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderModifiedEvent),
                PhMwpProcessModifiedHandler,
                NULL,
                &ProcessModifiedRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent),
                PhMwpProcessRemovedHandler,
                NULL,
                &ProcessRemovedRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PhMwpProcessesUpdatedHandler,
                NULL,
                &ProcessesUpdatedRegistration
                );

            NeedsSelectPid = PhStartupParameters.SelectPid;
        }
        break;
    case MainTabPageCreateWindow:
        {
            *(HWND *)Parameter1 = PhMwpProcessTreeNewHandle;
        }
        return TRUE;
    case MainTabPageInitializeSectionMenuItems:
        {
            PPH_MAIN_TAB_PAGE_MENU_INFORMATION menuInfo = Parameter1;
            PPH_EMENU menu = menuInfo->Menu;
            ULONG startIndex = menuInfo->StartIndex;
            PPH_EMENU_ITEM menuItem;
            PPH_EMENU_ITEM columnSetMenuItem;

            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS, L"&Hide processes from other users", NULL, NULL), startIndex);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_HIDESIGNEDPROCESSES, L"Hide si&gned processes", NULL, NULL), startIndex + 1);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_SCROLLTONEWPROCESSES, L"Scrol&l to new processes", NULL, NULL), startIndex + 2);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_SHOWCPUBELOW001, L"Show CPU &below 0.01", NULL, NULL), startIndex + 3);

            if (CurrentUserFilterEntry && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS)))
                menuItem->Flags |= PH_EMENU_CHECKED;
            if (SignedFilterEntry && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_HIDESIGNEDPROCESSES)))
                menuItem->Flags |= PH_EMENU_CHECKED;
            if (PhCsScrollToNewProcesses && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_SCROLLTONEWPROCESSES)))
                menuItem->Flags |= PH_EMENU_CHECKED;

            if (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_SHOWCPUBELOW001))
            {
                if (PhEnableCycleCpuUsage)
                {
                    if (PhCsShowCpuBelow001)
                        menuItem->Flags |= PH_EMENU_CHECKED;
                }
                else
                {
                    menuItem->Flags |= PH_EMENU_DISABLED;
                }
            }

            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), startIndex + 4);
            PhInsertEMenuItem(menu, menuItem = PhCreateEMenuItem(0, ID_VIEW_ORGANIZECOLUMNSETS, L"Organi&ze column sets...", NULL, NULL), startIndex + 5);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_SAVECOLUMNSET, L"Sa&ve column set...", NULL, NULL), startIndex + 6);
            PhInsertEMenuItem(menu, columnSetMenuItem = PhCreateEMenuItem(0, 0, L"Loa&d column set", NULL, NULL), startIndex + 7);

            // Add column set sub menu entries.
            {
                ULONG index;
                PPH_LIST columnSetList;

                columnSetList = PhInitializeColumnSetList(L"ProcessTreeColumnSetConfig");

                if (!columnSetList->Count)
                {
                    menuItem->Flags |= PH_EMENU_DISABLED;
                    columnSetMenuItem->Flags |= PH_EMENU_DISABLED;
                }
                else
                {
                    for (index = 0; index < columnSetList->Count; index++)
                    {
                        PPH_COLUMN_SET_ENTRY entry = columnSetList->Items[index];

                        menuItem = PhCreateEMenuItem(PH_EMENU_TEXT_OWNED, ID_VIEW_LOADCOLUMNSET, 
                            PhAllocateCopy(entry->Name->Buffer, entry->Name->Length + sizeof(WCHAR)), NULL, NULL);
                        PhInsertEMenuItem(columnSetMenuItem, menuItem, -1);
                    }
                }

                PhDeleteColumnSetList(columnSetList);
            }
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            PhLoadSettingsProcessTreeList();

            if (PhGetIntegerSetting(L"HideOtherUserProcesses"))
                CurrentUserFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpCurrentUserProcessTreeFilter, NULL);
            if (PhGetIntegerSetting(L"HideSignedProcesses"))
                SignedFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpSignedProcessTreeFilter, NULL);
        }
        return TRUE;
    case MainTabPageSaveSettings:
        {
            PhSaveSettingsProcessTreeList();
        }
        return TRUE;
    case MainTabPageExportContent:
        {
            PPH_MAIN_TAB_PAGE_EXPORT_CONTENT exportContent = Parameter1;

            PhWriteProcessTree(exportContent->FileStream, exportContent->Mode);
        }
        return TRUE;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            SendMessage(PhMwpProcessTreeNewHandle, WM_SETFONT, (WPARAM)font, TRUE);
        }
        break;
    case MainTabPageUpdateAutomaticallyChanged:
        {
            BOOLEAN updateAutomatically = (BOOLEAN)PtrToUlong(Parameter1);

            PhSetEnabledProvider(&PhMwpProcessProviderRegistration, updateAutomatically);
        }
        break;
    }

    return FALSE;
}

VOID PhMwpShowProcessProperties(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_PROPCONTEXT propContext;

    propContext = PhCreateProcessPropContext(
        NULL,
        ProcessItem
        );

    if (propContext)
    {
        PhShowProcessProperties(propContext);
        PhDereferenceObject(propContext);
    }
}

VOID PhMwpToggleCurrentUserProcessTreeFilter(
    VOID
    )
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

BOOLEAN PhMwpCurrentUserProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (!processNode->ProcessItem->Sid)
        return FALSE;

    if (!RtlEqualSid(processNode->ProcessItem->Sid, PhGetOwnTokenAttributes().TokenSid))
        return FALSE;

    return TRUE;
}

VOID PhMwpToggleSignedProcessTreeFilter(
    VOID
    )
{
    if (!SignedFilterEntry)
    {
        if (!PhEnableProcessQueryStage2)
        {
            PhShowInformation2(
                PhMainWndHandle,
                NULL,
                L"This filter cannot function because digital signature checking is not enabled. "
                L"Enable it in Options > General and restart Process Hacker."
                );
            return;
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
    IO_PRIORITY_HINT ioPriority;

    switch (Id)
    {
    case ID_IOPRIORITY_VERYLOW:
        ioPriority = IoPriorityVeryLow;
        break;
    case ID_IOPRIORITY_LOW:
        ioPriority = IoPriorityLow;
        break;
    case ID_IOPRIORITY_NORMAL:
        ioPriority = IoPriorityNormal;
        break;
    case ID_IOPRIORITY_HIGH:
        ioPriority = IoPriorityHigh;
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
    IO_PRIORITY_HINT ioPriority = ULONG_MAX;
    ULONG pagePriority = ULONG_MAX;
    ULONG id = 0;

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        ProcessId
        )))
    {
        if (SetPriority)
        {
            PhGetProcessPriority(processHandle, &priorityClass);
        }

        if (SetIoPriority)
        {
            if (!NT_SUCCESS(PhGetProcessIoPriority(
                processHandle,
                &ioPriority
                )))
            {
                ioPriority = ULONG_MAX;
            }
        }

        if (SetPagePriority)
        {
            if (!NT_SUCCESS(PhGetProcessPagePriority(
                processHandle,
                &pagePriority
                )))
            {
                pagePriority = ULONG_MAX;
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

    if (SetIoPriority && ioPriority != ULONG_MAX)
    {
        id = 0;

        switch (ioPriority)
        {
        case IoPriorityVeryLow:
            id = ID_IOPRIORITY_VERYLOW;
            break;
        case IoPriorityLow:
            id = ID_IOPRIORITY_LOW;
            break;
        case IoPriorityNormal:
            id = ID_IOPRIORITY_NORMAL;
            break;
        case IoPriorityHigh:
            id = ID_IOPRIORITY_HIGH;
            break;
        }

        if (id != 0)
        {
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
        }
    }

    if (SetPagePriority && pagePriority != ULONG_MAX)
    {
        id = 0;

        switch (pagePriority)
        {
        case MEMORY_PRIORITY_VERY_LOW:
            id = ID_PAGEPRIORITY_VERYLOW;
            break;
        case MEMORY_PRIORITY_LOW:
            id = ID_PAGEPRIORITY_LOW;
            break;
        case MEMORY_PRIORITY_MEDIUM:
            id = ID_PAGEPRIORITY_MEDIUM;
            break;
        case MEMORY_PRIORITY_BELOW_NORMAL:
            id = ID_PAGEPRIORITY_BELOWNORMAL;
            break;
        case MEMORY_PRIORITY_NORMAL:
            id = ID_PAGEPRIORITY_NORMAL;
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

        // If the user selected a fake process, disable all but a few menu items.
        if (
            PH_IS_FAKE_PROCESS_ID(Processes[0]->ProcessId) || 
            Processes[0]->ProcessId == SYSTEM_IDLE_PROCESS_ID ||
            Processes[0]->ProcessId == SYSTEM_PROCESS_ID // TODO: Some menu entires could be enabled for the system process?
            )
        {
            PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
            PhEnableEMenuItem(Menu, ID_PROCESS_PROPERTIES, TRUE);
            PhEnableEMenuItem(Menu, ID_PROCESS_SEARCHONLINE, TRUE);
        }

        if (PhIsNullOrEmptyString(Processes[0]->FileName) || !RtlDoesFileExists_U(PhGetString(Processes[0]->FileName)))
        {
            PhEnableEMenuItem(Menu, ID_PROCESS_OPENFILELOCATION, FALSE);  
        }

        // Critical
        if (Processes[0]->QueryHandle)
        {
            BOOLEAN breakOnTermination;

            if (NT_SUCCESS(PhGetProcessBreakOnTermination(
                Processes[0]->QueryHandle,
                &breakOnTermination
                )))
            {
                if (breakOnTermination)
                {
                    PhSetFlagsEMenuItem(Menu, ID_MISCELLANEOUS_SETCRITICAL, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
                }
            }
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
        HANDLE tokenHandle;
        BOOLEAN allowed = FALSE;
        BOOLEAN enabled = FALSE;

        if (Processes[0]->QueryHandle)
        {
            if (NT_SUCCESS(PhOpenProcessToken(
                Processes[0]->QueryHandle,
                TOKEN_QUERY,
                &tokenHandle
                )))
            {
                PhGetTokenIsVirtualizationAllowed(tokenHandle, &allowed);
                PhGetTokenIsVirtualizationEnabled(tokenHandle, &enabled);
                PhMwpSelectedProcessVirtualizationEnabled = enabled;

                NtClose(tokenHandle);
            }
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
            PhMwpSelectedProcessWindowHandle = PhGetProcessMainWindow(Processes[0]->ProcessId, Processes[0]->QueryHandle);

            if (!PhMwpSelectedProcessWindowHandle)
                item->Flags |= PH_EMENU_DISABLED;

            GetWindowPlacement(PhMwpSelectedProcessWindowHandle, &placement);

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
        PhInsertCopyCellEMenuItem(menu, ID_PROCESS_COPY, PhMwpProcessTreeNewHandle, ContextMenu->Column);

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

VOID NTAPI PhMwpProcessAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // Reference the process item so it doesn't get deleted before
    // we handle the event in the main thread.
    PhReferenceObject(processItem);
    PhPushProviderEventQueue(&PhMwpProcessEventQueue, ProviderAddedEvent, Parameter, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
}

VOID NTAPI PhMwpProcessModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    PhPushProviderEventQueue(&PhMwpProcessEventQueue, ProviderModifiedEvent, Parameter, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
}

VOID NTAPI PhMwpProcessRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // We already have a reference to the process item, so we don't need to
    // reference it here.
    PhPushProviderEventQueue(&PhMwpProcessEventQueue, ProviderRemovedEvent, Parameter, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
}

VOID NTAPI PhMwpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhMainWndHandle, WM_PH_PROCESSES_UPDATED, PhGetRunIdProvider(&PhMwpProcessProviderRegistration), 0);
}

VOID PhMwpOnProcessAdded(
    _In_ _Assume_refs_(1) PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG RunId
    )
{
    PPH_PROCESS_NODE processNode;

    processNode = PhAddProcessNode(ProcessItem, RunId);

    if (RunId != 1)
    {
        PPH_PROCESS_ITEM parentProcess;
        HANDLE parentProcessId = NULL;
        PPH_STRING parentName = NULL;

        if (parentProcess = PhReferenceProcessItemForParent(ProcessItem))
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

        if (PhMwpNotifyIconNotifyMask & PH_NOTIFY_PROCESS_CREATE)
        {
            if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_PROCESS_CREATE, ProcessItem))
            {
                PhMwpClearLastNotificationDetails();
                PhMwpLastNotificationType = PH_NOTIFY_PROCESS_CREATE;
                PhMwpLastNotificationDetails.ProcessId = ProcessItem->ProcessId;

                PhShowIconNotification(L"Process Created", PhaFormatString(
                    L"The process %s (%u) was created by %s (%u)",
                    ProcessItem->ProcessName->Buffer,
                    HandleToUlong(ProcessItem->ProcessId),
                    PhGetStringOrDefault(parentName, L"Unknown process"),
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

    PhLogProcessEntry(PH_LOG_ENTRY_PROCESS_DELETE, ProcessItem->ProcessId, ProcessItem->QueryHandle, ProcessItem->ProcessName, NULL, NULL);

    if (PhMwpNotifyIconNotifyMask & PH_NOTIFY_PROCESS_DELETE)
    {
        if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_PROCESS_DELETE, ProcessItem))
        {
            PhMwpClearLastNotificationDetails();
            PhMwpLastNotificationType = PH_NOTIFY_PROCESS_DELETE;
            PhMwpLastNotificationDetails.ProcessId = ProcessItem->ProcessId;

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
    _In_ ULONG RunId
    )
{
    PPH_PROVIDER_EVENT events;
    ULONG count;
    ULONG i;

    events = PhFlushProviderEventQueue(&PhMwpProcessEventQueue, RunId, &count);

    if (events)
    {
        TreeNew_SetRedraw(PhMwpProcessTreeNewHandle, FALSE);

        for (i = 0; i < count; i++)
        {
            PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
            PPH_PROCESS_ITEM processItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

            switch (type)
            {
            case ProviderAddedEvent:
                PhMwpOnProcessAdded(processItem, events[i].RunId);
                break;
            case ProviderModifiedEvent:
                PhMwpOnProcessModified(processItem);
                break;
            case ProviderRemovedEvent:
                PhMwpOnProcessRemoved(processItem);
                break;
            }
        }

        PhFree(events);
    }

    // The modified notification is only sent for special cases.
    // We have to invalidate the text on each update.
    PhTickProcessNodes();

    if (PhPluginsEnabled)
    {
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessesUpdated), NULL);
    }

    if (count != 0)
        TreeNew_SetRedraw(PhMwpProcessTreeNewHandle, TRUE);

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
        TreeNew_EnsureVisible(PhMwpProcessTreeNewHandle, &ProcessToScrollTo->Node);
        ProcessToScrollTo = NULL;
    }
}

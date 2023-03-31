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
static ULONG ProcessEndToScrollTo = 0;
static PPH_PROCESS_NODE ProcessToScrollTo = NULL;
static PPH_TN_FILTER_ENTRY CurrentUserFilterEntry = NULL;
static PPH_TN_FILTER_ENTRY SignedFilterEntry = NULL;
static PPH_TN_FILTER_ENTRY MicrosoftSignedFilterEntry = NULL;

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
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_HIDEMICROSOFTPROCESSES, L"Hide &system processes", NULL, NULL), startIndex + 2);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_SCROLLTONEWPROCESSES, L"Scrol&l to new processes", NULL, NULL), startIndex + 3);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_SHOWCPUBELOW001, L"Show CPU &below 0.01", NULL, NULL), startIndex + 4);

            if (PhCsHideOtherUserProcesses && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_HIDEPROCESSESFROMOTHERUSERS)))
                menuItem->Flags |= PH_EMENU_CHECKED;
            if (PhGetIntegerSetting(L"HideSignedProcesses") && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_HIDESIGNEDPROCESSES)))
                menuItem->Flags |= PH_EMENU_CHECKED;
            if (PhGetIntegerSetting(L"HideMicrosoftProcesses") && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_HIDEMICROSOFTPROCESSES)))
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

            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), startIndex + 5);
            PhInsertEMenuItem(menu, menuItem = PhCreateEMenuItem(0, ID_VIEW_ORGANIZECOLUMNSETS, L"Organi&ze column sets...", NULL, NULL), startIndex + 6);
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
                            PhAllocateCopy(entry->Name->Buffer, entry->Name->Length + sizeof(UNICODE_NULL)), NULL, NULL);
                        PhInsertEMenuItem(columnSetMenuItem, menuItem, ULONG_MAX);
                    }
                }

                PhDeleteColumnSetList(columnSetList);
            }
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            PhLoadSettingsProcessTreeList();

            if (PhCsHideOtherUserProcesses)
                CurrentUserFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpCurrentUserProcessTreeFilter, NULL);

            if (PhGetIntegerSetting(L"HideSignedProcesses"))
                SignedFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpSignedProcessTreeFilter, NULL);

            if (PhGetIntegerSetting(L"HideMicrosoftProcesses"))
                MicrosoftSignedFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpMicrosoftProcessTreeFilter, NULL);
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

            SetWindowFont(PhMwpProcessTreeNewHandle, font, TRUE);
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

    PhCsHideOtherUserProcesses = !PhCsHideOtherUserProcesses;
    PhSetIntegerSetting(L"HideOtherUserProcesses", PhCsHideOtherUserProcesses);
}

BOOLEAN PhMwpCurrentUserProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (!processNode->ProcessItem->Sid)
        return FALSE;

    if (!PhEqualSid(processNode->ProcessItem->Sid, PhGetOwnTokenAttributes().TokenSid))
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
                L"This filter cannot function because digital signature checking is not enabled.\r\n%s",
                L"Enable it in Options > General and restart System Informer."
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

VOID PhMwpToggleMicrosoftProcessTreeFilter(
    VOID
    )
{
    if (!MicrosoftSignedFilterEntry)
    {
        MicrosoftSignedFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), PhMwpMicrosoftProcessTreeFilter, NULL);
    }
    else
    {
        PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), MicrosoftSignedFilterEntry);
        MicrosoftSignedFilterEntry = NULL;
    }

    PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());

    PhSetIntegerSetting(L"HideMicrosoftProcesses", !!MicrosoftSignedFilterEntry);
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

BOOLEAN PhMwpMicrosoftProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (PhEnableProcessQueryStage2)
    {
        if (!PhIsNullOrEmptyString(processNode->ProcessItem->VerifySignerName))
        {
            static PH_STRINGREF microsoftSignerNameSr = PH_STRINGREF_INIT(L"Microsoft Windows");

            if (PhEqualStringRef(&processNode->ProcessItem->VerifySignerName->sr, &microsoftSignerNameSr, TRUE))
                return FALSE;
        }
    }
    else
    {
        if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.CompanyName))
        {
            static PH_STRINGREF microsoftCompanyNameSr = PH_STRINGREF_INIT(L"Microsoft");

            if (PhStartsWithStringRef(&processNode->ProcessItem->VersionInfo.CompanyName->sr, &microsoftCompanyNameSr, TRUE))
                return FALSE;
        }
    }

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
    _In_opt_ HANDLE ProcessId,
    _In_ BOOLEAN SetPriority,
    _In_ BOOLEAN SetIoPriority,
    _In_ BOOLEAN SetPagePriority
    )
{
    HANDLE processHandle;
    UCHAR priorityClass = PROCESS_PRIORITY_CLASS_UNKNOWN;
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
            if (!NT_SUCCESS(PhGetProcessPriority(
                processHandle,
                &priorityClass
                )))
            {
                priorityClass = PROCESS_PRIORITY_CLASS_UNKNOWN;
            }
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

    if (SetPriority && priorityClass != PROCESS_PRIORITY_CLASS_UNKNOWN)
    {
        switch (priorityClass)
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
            Processes[0]->ProcessId == SYSTEM_IDLE_PROCESS_ID
            //Processes[0]->ProcessId == SYSTEM_PROCESS_ID // (dmex)
            )
        {
            PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
            PhEnableEMenuItem(Menu, ID_PROCESS_PROPERTIES, TRUE);
            PhEnableEMenuItem(Menu, ID_PROCESS_SEARCHONLINE, TRUE);

            // Enable the Miscellaneous menu item but disable its children.
            if (item = PhFindEMenuItem(Menu, 0, 0, ID_PROCESS_MISCELLANEOUS))
            {
                PhSetEnabledEMenuItem(item, TRUE);
                PhSetFlagsAllEMenuItems(item, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
            }
        }

        // Disable the restart menu for service host processes. (dmex)
        if (
            Processes[0]->ServiceList &&
            Processes[0]->ServiceList->Count != 0
            )
        {
            PhEnableEMenuItem(Menu, ID_PROCESS_RESTART, FALSE);
        }

        if (
            PhIsNullOrEmptyString(Processes[0]->FileName) ||
            !PhDoesFileExist(&Processes[0]->FileName->sr)
            )
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

        // Disable Terminate/Suspend/Resume tree when the process has no children.
        {
            PPH_PROCESS_NODE node = PhFindProcessNode(Processes[0]->ProcessId);

            if (node && !(node->Children && node->Children->Count))
            {
                PhEnableEMenuItem(Menu, ID_PROCESS_TERMINATETREE, FALSE);
                PhEnableEMenuItem(Menu, ID_PROCESS_SUSPENDTREE, FALSE);
                PhEnableEMenuItem(Menu, ID_PROCESS_RESUMETREE, FALSE);
            }
        }

        // Eco mode
        if (WindowsVersion < WINDOWS_10_RS4)
        {
            PhEnableEMenuItem(Menu, ID_MISCELLANEOUS_ECOMODE, FALSE);
        }
        else
        {
            if (Processes[0]->QueryHandle)
            {
                POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

                if (NT_SUCCESS(PhGetProcessPowerThrottlingState(
                    Processes[0]->QueryHandle,
                    &powerThrottlingState
                    )))
                {
                    if (
                        powerThrottlingState.ControlMask & POWER_THROTTLING_PROCESS_EXECUTION_SPEED &&
                        powerThrottlingState.StateMask & POWER_THROTTLING_PROCESS_EXECUTION_SPEED
                        )
                    {
                        PhSetFlagsEMenuItem(Menu, ID_MISCELLANEOUS_ECOMODE, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
                    }
                }
            }
        }

        if (WindowsVersion < WINDOWS_11)
        {
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_FREEZE))
                PhDestroyEMenuItem(item);
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_THAW))
                PhDestroyEMenuItem(item);
        }
    }
    else
    {
        ULONG menuItemsMultiEnabled[] =
        {
            ID_PROCESS_TERMINATE,
            ID_PROCESS_SUSPEND,
            ID_PROCESS_RESUME,
            //ID_PROCESS_THAW,
            //ID_PROCESS_FREEZE,
            ID_MISCELLANEOUS_REDUCEWORKINGSET,
            ID_PROCESS_COPY
        };
        ULONG i;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        // Enable the Miscellaneous menu item but disable its children.
        if (item = PhFindEMenuItem(Menu, 0, 0, ID_PROCESS_MISCELLANEOUS))
        {
            item->Flags &= ~PH_EMENU_DISABLED;
            PhSetFlagsAllEMenuItems(item, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        }

        // Enable the Priority menu item.
        if (item = PhFindEMenuItem(Menu, 0, 0, ID_PROCESS_PRIORITY))
            item->Flags &= ~PH_EMENU_DISABLED;

        // Enable the I/O Priority menu item.
        if (item = PhFindEMenuItem(Menu, 0, 0, ID_PROCESS_IOPRIORITY))
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
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_SUSPENDTREE))
                PhDestroyEMenuItem(item);
        }

        if (!Processes[0]->IsPartiallySuspended)
        {
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_RESUME))
                PhDestroyEMenuItem(item);
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_RESUMETREE))
                PhDestroyEMenuItem(item);
        }

        if (WindowsVersion >= WINDOWS_11 && PH_IS_REAL_PROCESS_ID(Processes[0]->ProcessId))
        {
            if (PhIsProcessStateFrozen(Processes[0]->ProcessId))
            {
                if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_FREEZE))
                    PhDestroyEMenuItem(item);
            }
            else
            {
                if (item = PhFindEMenuItem(Menu, 0, NULL, ID_PROCESS_THAW))
                    PhDestroyEMenuItem(item);
            }
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

    item = PhFindEMenuItem(Menu, 0, 0, ID_PROCESS_WINDOW);

    if (item)
    {
        // Window menu
        if (NumberOfProcesses == 1)
        {
            // Get a handle to the process' top-level window (if any).
            PhMwpSelectedProcessWindowHandle = PhGetProcessMainWindow(Processes[0]->ProcessId, Processes[0]->QueryHandle);

            if (!PhMwpSelectedProcessWindowHandle)
                item->Flags |= PH_EMENU_DISABLED;

            if (PhMwpSelectedProcessWindowHandle)
            {
                WINDOWPLACEMENT placement = { sizeof(placement) };

                GetWindowPlacement(PhMwpSelectedProcessWindowHandle, &placement);

                if (placement.showCmd == SW_MINIMIZE)
                    PhEnableEMenuItem(item, ID_WINDOW_MINIMIZE, FALSE);
                else if (placement.showCmd == SW_MAXIMIZE)
                    PhEnableEMenuItem(item, ID_WINDOW_MAXIMIZE, FALSE);
                else if (placement.showCmd == SW_NORMAL)
                    PhEnableEMenuItem(item, ID_WINDOW_RESTORE, FALSE);
            }
        }
        else
        {
            item->Flags |= PH_EMENU_DISABLED;
        }
    }
}

PPH_EMENU PhpCreateProcessMenu(
    VOID
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM menuItem;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_TERMINATE, L"T&erminate\bDel", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_TERMINATETREE, L"Terminate tree\bShift+Del", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_SUSPEND, L"&Suspend", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_SUSPENDTREE, L"Suspend tree", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_RESUME, L"Res&ume", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_RESUMETREE, L"Resume tree", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_FREEZE, L"Freeze", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_THAW, L"Thaw", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_RESTART, L"Res&tart", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_CREATEDUMPFILE, L"Create dump fi&le...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_DEBUG, L"De&bug", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_AFFINITY, L"&Affinity", NULL, NULL), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, ID_PROCESS_PRIORITY, L"&Priority", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_REALTIME, L"&Real time", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_HIGH, L"&High", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_ABOVENORMAL, L"&Above normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_NORMAL, L"&Normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_BELOWNORMAL, L"&Below normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_IDLE, L"&Idle", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, ID_PROCESS_IOPRIORITY, L"&I/O priority", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_HIGH, L"&High", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_NORMAL, L"&Normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_LOW, L"&Low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_VERYLOW, L"&Very low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, ID_PROCESS_PAGEPRIORITY, L"Pa&ge priority", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_NORMAL, L"&Normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_BELOWNORMAL, L"&Below normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_MEDIUM, L"&Medium", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_LOW, L"&Low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_VERYLOW, L"&Very low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, ID_PROCESS_MISCELLANEOUS, L"&Miscellaneous", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_SETCRITICAL, L"&Critical", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_DETACHFROMDEBUGGER, L"&Detach from debugger", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_ECOMODE, L"Efficiency mode", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_GDIHANDLES, L"GDI &handles", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_HEAPS, L"Heaps", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_REDUCEWORKINGSET, L"Reduce working &set", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_RUNAS, L"&Run as...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_RUNASTHISUSER, L"Run &as this user...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_VIRTUALIZATION, L"Virtuali&zation", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, ID_PROCESS_WINDOW, L"&Window", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_WINDOW_BRINGTOFRONT, L"Bring to &front", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_WINDOW_RESTORE, L"&Restore", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_WINDOW_MINIMIZE, L"M&inimize", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_WINDOW_MAXIMIZE, L"M&aximize", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_WINDOW_CLOSE, L"&Close", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_SEARCHONLINE, L"Search &online\bCtrl+M", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_OPENFILELOCATION, L"Open &file location\bCtrl+Enter", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_PROPERTIES, L"P&roperties\bEnter", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);

    return menu;
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

        menu = PhpCreateProcessMenu();
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
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // Reference the process item so it doesn't get deleted before
    // we handle the event in the main thread.
    PhReferenceObject(processItem);
    PhPushProviderEventQueue(&PhMwpProcessEventQueue, ProviderAddedEvent, Parameter, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
}

VOID NTAPI PhMwpProcessModifiedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    PhPushProviderEventQueue(&PhMwpProcessEventQueue, ProviderModifiedEvent, Parameter, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
}

VOID NTAPI PhMwpProcessRemovedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // We already have a reference to the process item, so we don't need to
    // reference it here.
    PhPushProviderEventQueue(&PhMwpProcessEventQueue, ProviderRemovedEvent, Parameter, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
}

VOID NTAPI PhMwpProcessesUpdatedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ProcessHacker_Invoke(PhMwpOnProcessesUpdated, PhGetRunIdProvider(&PhMwpProcessProviderRegistration));
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
            ProcessItem->ProcessName,
            parentProcessId,
            parentName,
            0
            );

        if (PhMwpNotifyIconNotifyMask & PH_NOTIFY_PROCESS_CREATE)
        {
            if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_PROCESS_CREATE, ProcessItem))
            {
                PH_FORMAT format[9];
                WCHAR formatBuffer[260];

                PhMwpClearLastNotificationDetails();
                PhMwpLastNotificationType = PH_NOTIFY_PROCESS_CREATE;
                PhMwpLastNotificationDetails.ProcessId = ProcessItem->ProcessId;

                // The process %s (%lu) was created by %s (%lu)
                PhInitFormatS(&format[0], L"The process ");
                PhInitFormatSR(&format[1], ProcessItem->ProcessName->sr);
                PhInitFormatS(&format[2], L" (");
                PhInitFormatU(&format[3], HandleToUlong(ProcessItem->ProcessId));
                PhInitFormatS(&format[4], L") was created by ");
                PhInitFormatS(&format[5], PhGetStringOrDefault(parentName, L"Unknown process")); // todo: SR type (dmex)
                PhInitFormatS(&format[6], L" (");
                PhInitFormatU(&format[7], HandleToUlong(ProcessItem->ParentProcessId));
                PhInitFormatC(&format[8], L')');

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
                {
                    PhShowIconNotification(L"Process Created", formatBuffer);
                }
                else
                {
                    PhShowIconNotification(L"Process Created",
                        PH_AUTO_T(PH_STRING, PhFormat(format, RTL_NUMBER_OF(format), 0))->Buffer);
                }
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

    if (SignedFilterEntry || MicrosoftSignedFilterEntry) // HACK: Invalidate filters when modified (dmex)
        PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
}

VOID PhMwpOnProcessRemoved(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_NODE processNode;
    ULONG processNodeIndex = 0;
    ULONG exitStatus = 0;

    if (ProcessItem->QueryHandle)
    {
        PROCESS_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetProcessBasicInformation(ProcessItem->QueryHandle, &basicInfo)))
        {
            exitStatus = basicInfo.ExitStatus;
        }
    }

    PhLogProcessEntry(PH_LOG_ENTRY_PROCESS_DELETE, ProcessItem->ProcessId, ProcessItem->ProcessName, NULL, NULL, exitStatus);

    if (PhMwpNotifyIconNotifyMask & PH_NOTIFY_PROCESS_DELETE)
    {
        if (!PhPluginsEnabled || !PhMwpPluginNotifyEvent(PH_NOTIFY_PROCESS_DELETE, ProcessItem))
        {
            PH_FORMAT format[6];
            WCHAR formatBuffer[260];

            PhMwpClearLastNotificationDetails();
            PhMwpLastNotificationType = PH_NOTIFY_PROCESS_DELETE;
            PhMwpLastNotificationDetails.ProcessId = ProcessItem->ProcessId;

            // The process %s (%lu) was terminated with status 0x%x
            PhInitFormatS(&format[0], L"The process ");
            PhInitFormatSR(&format[1], ProcessItem->ProcessName->sr);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatU(&format[3], HandleToUlong(ProcessItem->ProcessId));
            PhInitFormatS(&format[4], L") was terminated with status 0x");
            PhInitFormatX(&format[5], exitStatus);

            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            {
                PhShowIconNotification(L"Process Terminated", formatBuffer);
            }
            else
            {
                PhShowIconNotification(L"Process Terminated",
                    PH_AUTO_T(PH_STRING, PhFormat(format, RTL_NUMBER_OF(format), 0))->Buffer);
            }
        }
    }

    processNode = PhFindProcessNode(ProcessItem->ProcessId);
    processNodeIndex = processNode->Node.Index;
    PhRemoveProcessNode(processNode);

    if (ProcessToScrollTo == processNode) // shouldn't happen, but just in case
        ProcessToScrollTo = NULL;

    if (PhCsScrollToRemovedProcesses)
    {
        ProcessEndToScrollTo = processNodeIndex;
    }
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

    if (ProcessEndToScrollTo)
    {
        ULONG count = TreeNew_GetFlatNodeCount(PhMwpProcessTreeNewHandle);

        if (ProcessEndToScrollTo > count)
            ProcessEndToScrollTo = count;

        TreeNew_EnsureVisibleIndex(PhMwpProcessTreeNewHandle, ProcessEndToScrollTo);
        ProcessEndToScrollTo = 0;
    }
}

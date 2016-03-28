/*
 * Process Hacker -
 *   Process properties: Threads page
 *
 * Copyright (C) 2009-2016 wj32
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
#include <cpysave.h>
#include <emenu.h>
#include <secedit.h>
#include <procprv.h>
#include <thrdprv.h>
#include <thrdlist.h>
#include <actions.h>
#include <phplug.h>
#include <extmgri.h>
#include <procprpp.h>

static PH_STRINGREF EmptyThreadsText = PH_STRINGREF_INIT(L"There are no threads to display.");

static VOID NTAPI ThreadAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    // Parameter contains a pointer to the added thread item.
    PhReferenceObject(Parameter);
    PhPushProviderEventQueue(&threadsContext->EventQueue, ProviderAddedEvent, Parameter, (ULONG)threadsContext->Provider->RunId);
}

static VOID NTAPI ThreadModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PhPushProviderEventQueue(&threadsContext->EventQueue, ProviderModifiedEvent, Parameter, (ULONG)threadsContext->Provider->RunId);
}

static VOID NTAPI ThreadRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PhPushProviderEventQueue(&threadsContext->EventQueue, ProviderRemovedEvent, Parameter, (ULONG)threadsContext->Provider->RunId);
}

static VOID NTAPI ThreadsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREADS_UPDATED, (ULONG)threadsContext->Provider->RunId, threadsContext->Provider->RunId == 1);
}

static VOID NTAPI ThreadsLoadingStateChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(
        threadsContext->ListContext.TreeNewHandle,
        TNM_SETCURSOR,
        0,
        // Parameter contains TRUE if loading symbols
        (LPARAM)(Parameter ? LoadCursor(NULL, IDC_APPSTARTING) : NULL)
        );
}

VOID PhpInitializeThreadMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfThreads == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfThreads == 1)
    {
        // All menu items are enabled by default.
    }
    else
    {
        ULONG menuItemsMultiEnabled[] =
        {
            ID_THREAD_TERMINATE,
            ID_THREAD_SUSPEND,
            ID_THREAD_RESUME,
            ID_THREAD_COPY
        };
        ULONG i;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        // These menu items are capable of manipulating
        // multiple threads.
        for (i = 0; i < sizeof(menuItemsMultiEnabled) / sizeof(ULONG); i++)
        {
            PhEnableEMenuItem(Menu, menuItemsMultiEnabled[i], TRUE);
        }
    }

    // Remove irrelevant menu items.

    if (WindowsVersion < WINDOWS_VISTA)
    {
        // Remove I/O priority.
        if (item = PhFindEMenuItem(Menu, 0, L"I/O Priority", 0))
            PhDestroyEMenuItem(item);
        // Remove page priority.
        if (item = PhFindEMenuItem(Menu, 0, L"Page Priority", 0))
            PhDestroyEMenuItem(item);
    }

    PhEnableEMenuItem(Menu, ID_THREAD_TOKEN, FALSE);

    // Priority
    if (NumberOfThreads == 1)
    {
        HANDLE threadHandle;
        ULONG threadPriority = THREAD_PRIORITY_ERROR_RETURN;
        IO_PRIORITY_HINT ioPriority = -1;
        ULONG pagePriority = -1;
        ULONG id = 0;

        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            ThreadQueryAccess,
            Threads[0]->ThreadId
            )))
        {
            THREAD_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(PhGetThreadBasicInformation(threadHandle, &basicInfo)))
            {
                threadPriority = basicInfo.BasePriority;
            }

            if (WindowsVersion >= WINDOWS_VISTA)
            {
                PhGetThreadIoPriority(threadHandle, &ioPriority);
                PhGetThreadPagePriority(threadHandle, &pagePriority);
            }

            // Token
            {
                HANDLE tokenHandle;

                if (NT_SUCCESS(NtOpenThreadToken(
                    threadHandle,
                    TOKEN_QUERY,
                    TRUE,
                    &tokenHandle
                    )))
                {
                    PhEnableEMenuItem(Menu, ID_THREAD_TOKEN, TRUE);
                    NtClose(tokenHandle);
                }
            }

            NtClose(threadHandle);
        }

        switch (threadPriority)
        {
        case THREAD_PRIORITY_TIME_CRITICAL + 1:
        case THREAD_PRIORITY_TIME_CRITICAL:
            id = ID_PRIORITY_TIMECRITICAL;
            break;
        case THREAD_PRIORITY_HIGHEST:
            id = ID_PRIORITY_HIGHEST;
            break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
            id = ID_PRIORITY_ABOVENORMAL;
            break;
        case THREAD_PRIORITY_NORMAL:
            id = ID_PRIORITY_NORMAL;
            break;
        case THREAD_PRIORITY_BELOW_NORMAL:
            id = ID_PRIORITY_BELOWNORMAL;
            break;
        case THREAD_PRIORITY_LOWEST:
            id = ID_PRIORITY_LOWEST;
            break;
        case THREAD_PRIORITY_IDLE:
        case THREAD_PRIORITY_IDLE - 1:
            id = ID_PRIORITY_IDLE;
            break;
        }

        if (id != 0)
        {
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
        }

        if (ioPriority != -1)
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

        if (pagePriority != -1)
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
}

static NTSTATUS NTAPI PhpThreadPermissionsOpenThread(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return PhOpenThread(Handle, DesiredAccess, (HANDLE)Context);
}

static NTSTATUS NTAPI PhpOpenThreadTokenObject(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return NtOpenThreadToken(
        (HANDLE)Context,
        DesiredAccess,
        TRUE,
        Handle
        );
}

VOID PhpUpdateThreadDetails(
    _In_ HWND hwndDlg,
    _In_ PPH_THREADS_CONTEXT Context,
    _In_ BOOLEAN Force
    )
{
    PPH_THREAD_ITEM *threads;
    ULONG numberOfThreads;
    PPH_THREAD_ITEM threadItem;
    PPH_STRING startModule = NULL;
    PPH_STRING started = NULL;
    WCHAR kernelTime[PH_TIMESPAN_STR_LEN_1] = L"N/A";
    WCHAR userTime[PH_TIMESPAN_STR_LEN_1] = L"N/A";
    PPH_STRING contextSwitches = NULL;
    PPH_STRING cycles = NULL;
    PPH_STRING state = NULL;
    WCHAR priority[PH_INT32_STR_LEN_1] = L"N/A";
    WCHAR basePriority[PH_INT32_STR_LEN_1] = L"N/A";
    PWSTR ioPriority = L"N/A";
    PWSTR pagePriority = L"N/A";
    WCHAR idealProcessor[PH_INT32_STR_LEN + 1 + PH_INT32_STR_LEN + 1] = L"N/A";
    HANDLE threadHandle;
    SYSTEMTIME time;
    IO_PRIORITY_HINT ioPriorityInteger;
    ULONG pagePriorityInteger;
    PROCESSOR_NUMBER idealProcessorNumber;
    ULONG suspendCount;

    PhGetSelectedThreadItems(&Context->ListContext, &threads, &numberOfThreads);

    if (numberOfThreads == 1)
        threadItem = threads[0];
    else
        threadItem = NULL;

    PhFree(threads);

    if (numberOfThreads != 1 && !Force)
        return;

    if (numberOfThreads == 1)
    {
        startModule = threadItem->StartAddressFileName;

        PhLargeIntegerToLocalSystemTime(&time, &threadItem->CreateTime);
        started = PhaFormatDateTime(&time);

        PhPrintTimeSpan(kernelTime, threadItem->KernelTime.QuadPart, PH_TIMESPAN_HMSM);
        PhPrintTimeSpan(userTime, threadItem->UserTime.QuadPart, PH_TIMESPAN_HMSM);

        contextSwitches = PhaFormatUInt64(threadItem->ContextSwitchesDelta.Value, TRUE);

        if (WINDOWS_HAS_CYCLE_TIME)
            cycles = PhaFormatUInt64(threadItem->CyclesDelta.Value, TRUE);

        if (threadItem->State != Waiting)
        {
            if ((ULONG)threadItem->State < MaximumThreadState)
                state = PhaCreateString(PhKThreadStateNames[(ULONG)threadItem->State]);
            else
                state = PhaCreateString(L"Unknown");
        }
        else
        {
            if ((ULONG)threadItem->WaitReason < MaximumWaitReason)
                state = PhaConcatStrings2(L"Wait:", PhKWaitReasonNames[(ULONG)threadItem->WaitReason]);
            else
                state = PhaCreateString(L"Waiting");
        }

        PhPrintInt32(priority, threadItem->Priority);
        PhPrintInt32(basePriority, threadItem->BasePriority);

        if (NT_SUCCESS(PhOpenThread(&threadHandle, ThreadQueryAccess, threadItem->ThreadId)))
        {
            if (NT_SUCCESS(PhGetThreadIoPriority(threadHandle, &ioPriorityInteger)) &&
                ioPriorityInteger < MaxIoPriorityTypes)
            {
                ioPriority = PhIoPriorityHintNames[ioPriorityInteger];
            }

            if (NT_SUCCESS(PhGetThreadPagePriority(threadHandle, &pagePriorityInteger)) &&
                pagePriorityInteger <= MEMORY_PRIORITY_NORMAL)
            {
                pagePriority = PhPagePriorityNames[pagePriorityInteger];
            }

            if (NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadIdealProcessorEx, &idealProcessorNumber, sizeof(PROCESSOR_NUMBER), NULL)))
            {
                PH_FORMAT format[3];

                PhInitFormatU(&format[0], idealProcessorNumber.Group);
                PhInitFormatC(&format[1], ':');
                PhInitFormatU(&format[2], idealProcessorNumber.Number);
                PhFormatToBuffer(format, 3, idealProcessor, sizeof(idealProcessor), NULL);
            }

            if (threadItem->WaitReason == Suspended && NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadSuspendCount, &suspendCount, sizeof(ULONG), NULL)))
            {
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], state->sr);
                PhInitFormatS(&format[1], L" (");
                PhInitFormatU(&format[2], suspendCount);
                PhInitFormatS(&format[3], L")");
                state = PH_AUTO(PhFormat(format, 4, 30));
            }

            NtClose(threadHandle);
        }
    }

    if (Force)
    {
        // These don't change...

        SetDlgItemText(hwndDlg, IDC_STARTMODULE, PhGetStringOrEmpty(startModule));
        EnableWindow(GetDlgItem(hwndDlg, IDC_OPENSTARTMODULE), !!startModule);

        SetDlgItemText(hwndDlg, IDC_STARTED, PhGetStringOrDefault(started, L"N/A"));
    }

    SetDlgItemText(hwndDlg, IDC_KERNELTIME, kernelTime);
    SetDlgItemText(hwndDlg, IDC_USERTIME, userTime);
    SetDlgItemText(hwndDlg, IDC_CONTEXTSWITCHES, PhGetStringOrDefault(contextSwitches, L"N/A"));
    SetDlgItemText(hwndDlg, IDC_CYCLES, PhGetStringOrDefault(cycles, L"N/A"));
    SetDlgItemText(hwndDlg, IDC_STATE, PhGetStringOrDefault(state, L"N/A"));
    SetDlgItemText(hwndDlg, IDC_PRIORITY, priority);
    SetDlgItemText(hwndDlg, IDC_BASEPRIORITY, basePriority);
    SetDlgItemText(hwndDlg, IDC_IOPRIORITY, ioPriority);
    SetDlgItemText(hwndDlg, IDC_PAGEPRIORITY, pagePriority);
    SetDlgItemText(hwndDlg, IDC_IDEALPROCESSOR, idealProcessor);
}

VOID PhShowThreadContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_THREADS_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_THREAD_ITEM *threads;
    ULONG numberOfThreads;

    PhGetSelectedThreadItems(&Context->ListContext, &threads, &numberOfThreads);

    if (numberOfThreads != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_THREAD), 0);
        PhSetFlagsEMenuItem(menu, ID_THREAD_INSPECT, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeThreadMenu(menu, ProcessItem->ProcessId, threads, numberOfThreads);
        PhInsertCopyCellEMenuItem(menu, ID_THREAD_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Thread.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Thread.Threads = threads;
            menuInfo.u.Thread.NumberOfThreads = numberOfThreads;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
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
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(threads);
}

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_THREADS_CONTEXT threadsContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        threadsContext = (PPH_THREADS_CONTEXT)propPageContext->Context;

        if (threadsContext)
            tnHandle = threadsContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            threadsContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmThreadsContextType, sizeof(PH_THREADS_CONTEXT)));

            // The thread provider has a special registration mechanism.
            threadsContext->Provider = PhCreateThreadProvider(
                processItem->ProcessId
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadAddedEvent,
                ThreadAddedHandler,
                threadsContext,
                &threadsContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                ThreadModifiedHandler,
                threadsContext,
                &threadsContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadRemovedEvent,
                ThreadRemovedHandler,
                threadsContext,
                &threadsContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->UpdatedEvent,
                ThreadsUpdatedHandler,
                threadsContext,
                &threadsContext->UpdatedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->LoadingStateChangedEvent,
                ThreadsLoadingStateChangedHandler,
                threadsContext,
                &threadsContext->LoadingStateChangedEventRegistration
                );
            threadsContext->WindowHandle = hwndDlg;

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeThreadList(hwndDlg, tnHandle, &threadsContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &EmptyThreadsText, 0);
            PhInitializeProviderEventQueue(&threadsContext->EventQueue, 100);

            // Use Cycles instead of Context Switches on Vista and above, but only when we can
            // open the process, since cycle time information requires sufficient access to the
            // threads.
            if (WINDOWS_HAS_CYCLE_TIME)
            {
                HANDLE processHandle;
                PROCESS_EXTENDED_BASIC_INFORMATION extendedBasicInfo;

                // We make a distinction between PROCESS_QUERY_INFORMATION and PROCESS_QUERY_LIMITED_INFORMATION since
                // the latter can be used when opening audiodg.exe even though we can't access its threads using
                // THREAD_QUERY_LIMITED_INFORMATION.

                if (processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;
                }
                else if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId)))
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;
                    NtClose(processHandle);
                }
                else if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, processItem->ProcessId)))
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;

                    // We can't use cycle time for protected processes (without KProcessHacker).
                    if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &extendedBasicInfo)) && extendedBasicInfo.IsProtectedProcess)
                    {
                        threadsContext->ListContext.UseCycleTime = FALSE;
                    }

                    NtClose(processHandle);
                }
            }

            if (processItem->ServiceList && processItem->ServiceList->Count != 0 && WINDOWS_HAS_SERVICE_TAGS)
                threadsContext->ListContext.HasServices = TRUE;

            PhEmCallObjectOperation(EmThreadsContextType, threadsContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &threadsContext->ListContext.Cm;
                treeNewInfo.SystemContext = threadsContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsThreadList(&threadsContext->ListContext);

            PhThreadProviderInitialUpdate(threadsContext->Provider);
            PhRegisterThreadProvider(threadsContext->Provider, &threadsContext->ProviderRegistration);

            SET_BUTTON_ICON(IDC_OPENSTARTMODULE, PH_LOAD_SHARED_ICON_SMALL(MAKEINTRESOURCE(IDI_FOLDER)));
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmThreadsContextType, threadsContext, EmObjectDelete);

            PhUnregisterCallback(
                &threadsContext->Provider->ThreadAddedEvent,
                &threadsContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                &threadsContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadRemovedEvent,
                &threadsContext->RemovedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->UpdatedEvent,
                &threadsContext->UpdatedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->LoadingStateChangedEvent,
                &threadsContext->LoadingStateChangedEventRegistration
                );
            PhUnregisterThreadProvider(threadsContext->Provider, &threadsContext->ProviderRegistration);
            PhSetTerminatingThreadProvider(threadsContext->Provider);
            PhDereferenceObject(threadsContext->Provider);
            PhDeleteProviderEventQueue(&threadsContext->EventQueue);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &threadsContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsThreadList(&threadsContext->ListContext);
            PhDeleteThreadList(&threadsContext->ListContext);

            PhFree(threadsContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

#define ADD_BL_ITEM(Id) \
    PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, Id), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM)

                // Thread details area
                {
                    ULONG id;

                    for (id = IDC_STATICBL1; id <= IDC_STATICBL11; id++)
                        ADD_BL_ITEM(id);

                    // Not in sequence
                    ADD_BL_ITEM(IDC_STATICBL12);
                }

                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STARTMODULE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_OPENSTARTMODULE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                ADD_BL_ITEM(IDC_STARTED);
                ADD_BL_ITEM(IDC_KERNELTIME);
                ADD_BL_ITEM(IDC_USERTIME);
                ADD_BL_ITEM(IDC_CONTEXTSWITCHES);
                ADD_BL_ITEM(IDC_CYCLES);
                ADD_BL_ITEM(IDC_STATE);
                ADD_BL_ITEM(IDC_PRIORITY);
                ADD_BL_ITEM(IDC_BASEPRIORITY);
                ADD_BL_ITEM(IDC_IOPRIORITY);
                ADD_BL_ITEM(IDC_PAGEPRIORITY);
                ADD_BL_ITEM(IDC_IDEALPROCESSOR);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowThreadContextMenu(hwndDlg, processItem, threadsContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_THREAD_INSPECT:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadsContext->Provider);
                        PhShowThreadStackDialog(
                            hwndDlg,
                            threadsContext->Provider->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider
                            );
                        PhDereferenceObject(threadsContext->Provider);
                    }
                }
                break;
            case ID_THREAD_TERMINATE:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);

                    if (PhUiTerminateThreads(hwndDlg, threads, numberOfThreads))
                        PhDeselectAllThreadNodes(&threadsContext->ListContext);

                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_SUSPEND:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);
                    PhUiSuspendThreads(hwndDlg, threads, numberOfThreads);
                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_RESUME:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);
                    PhUiResumeThreads(hwndDlg, threads, numberOfThreads);
                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_AFFINITY:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadItem);
                        PhShowProcessAffinityDialog(hwndDlg, NULL, threadItem);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_THREAD_PERMISSIONS:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);
                    PH_STD_OBJECT_SECURITY stdObjectSecurity;
                    PPH_ACCESS_ENTRY accessEntries;
                    ULONG numberOfAccessEntries;

                    if (threadItem)
                    {
                        stdObjectSecurity.OpenObject = PhpThreadPermissionsOpenThread;
                        stdObjectSecurity.ObjectType = L"Thread";
                        stdObjectSecurity.Context = threadItem->ThreadId;

                        if (PhGetAccessEntries(L"Thread", &accessEntries, &numberOfAccessEntries))
                        {
                            PhEditSecurity(
                                hwndDlg,
                                PhaFormatString(L"Thread %u", HandleToUlong(threadItem->ThreadId))->Buffer,
                                PhStdGetObjectSecurity,
                                PhStdSetObjectSecurity,
                                &stdObjectSecurity,
                                accessEntries,
                                numberOfAccessEntries
                                );
                            PhFree(accessEntries);
                        }
                    }
                }
                break;
            case ID_THREAD_TOKEN:
                {
                    NTSTATUS status;
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);
                    HANDLE threadHandle;

                    if (threadItem)
                    {
                        if (NT_SUCCESS(status = PhOpenThread(
                            &threadHandle,
                            ThreadQueryAccess,
                            threadItem->ThreadId
                            )))
                        {
                            PhShowTokenProperties(
                                hwndDlg,
                                PhpOpenThreadTokenObject,
                                (PVOID)threadHandle,
                                NULL
                                );

                            NtClose(threadHandle);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to open the thread", status, 0);
                        }
                    }
                }
                break;
            case ID_ANALYZE_WAIT:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadsContext->Provider->SymbolProvider);
                        PhUiAnalyzeWaitThread(
                            hwndDlg,
                            processItem->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider->SymbolProvider
                            );
                        PhDereferenceObject(threadsContext->Provider->SymbolProvider);
                    }
                }
                break;
            case ID_PRIORITY_TIMECRITICAL:
            case ID_PRIORITY_HIGHEST:
            case ID_PRIORITY_ABOVENORMAL:
            case ID_PRIORITY_NORMAL:
            case ID_PRIORITY_BELOWNORMAL:
            case ID_PRIORITY_LOWEST:
            case ID_PRIORITY_IDLE:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        ULONG threadPriorityWin32;

                        switch (id)
                        {
                        case ID_PRIORITY_TIMECRITICAL:
                            threadPriorityWin32 = THREAD_PRIORITY_TIME_CRITICAL;
                            break;
                        case ID_PRIORITY_HIGHEST:
                            threadPriorityWin32 = THREAD_PRIORITY_HIGHEST;
                            break;
                        case ID_PRIORITY_ABOVENORMAL:
                            threadPriorityWin32 = THREAD_PRIORITY_ABOVE_NORMAL;
                            break;
                        case ID_PRIORITY_NORMAL:
                            threadPriorityWin32 = THREAD_PRIORITY_NORMAL;
                            break;
                        case ID_PRIORITY_BELOWNORMAL:
                            threadPriorityWin32 = THREAD_PRIORITY_BELOW_NORMAL;
                            break;
                        case ID_PRIORITY_LOWEST:
                            threadPriorityWin32 = THREAD_PRIORITY_LOWEST;
                            break;
                        case ID_PRIORITY_IDLE:
                            threadPriorityWin32 = THREAD_PRIORITY_IDLE;
                            break;
                        }

                        PhReferenceObject(threadItem);
                        PhUiSetPriorityThread(hwndDlg, threadItem, threadPriorityWin32);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_IOPRIORITY_VERYLOW:
            case ID_IOPRIORITY_LOW:
            case ID_IOPRIORITY_NORMAL:
            case ID_IOPRIORITY_HIGH:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        IO_PRIORITY_HINT ioPriority;

                        switch (id)
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
                        }

                        PhReferenceObject(threadItem);
                        PhUiSetIoPriorityThread(hwndDlg, threadItem, ioPriority);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_PAGEPRIORITY_VERYLOW:
            case ID_PAGEPRIORITY_LOW:
            case ID_PAGEPRIORITY_MEDIUM:
            case ID_PAGEPRIORITY_BELOWNORMAL:
            case ID_PAGEPRIORITY_NORMAL:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        ULONG pagePriority;

                        switch (id)
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

                        PhReferenceObject(threadItem);
                        PhUiSetPagePriorityThread(hwndDlg, threadItem, pagePriority);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_THREAD_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_OPENSTARTMODULE:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem && threadItem->StartAddressFileName)
                    {
                        PhShellExploreFile(hwndDlg, threadItem->StartAddressFileName->Buffer);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                break;
            case PSN_KILLACTIVE:
                // Can't disable, it screws up the deltas.
                break;
            }
        }
        break;
    case WM_PH_THREADS_UPDATED:
        {
            ULONG upToRunId = (ULONG)wParam;
            BOOLEAN firstRun = !!lParam;
            PPH_PROVIDER_EVENT events;
            ULONG count;
            ULONG i;

            events = PhFlushProviderEventQueue(&threadsContext->EventQueue, upToRunId, &count);

            if (events)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);

                for (i = 0; i < count; i++)
                {
                    PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
                    PPH_THREAD_ITEM threadItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

                    switch (type)
                    {
                    case ProviderAddedEvent:
                        PhAddThreadNode(&threadsContext->ListContext, threadItem, firstRun);
                        PhDereferenceObject(threadItem);
                        break;
                    case ProviderModifiedEvent:
                        PhUpdateThreadNode(&threadsContext->ListContext, PhFindThreadNode(&threadsContext->ListContext, threadItem->ThreadId));
                        break;
                    case ProviderRemovedEvent:
                        PhRemoveThreadNode(&threadsContext->ListContext, PhFindThreadNode(&threadsContext->ListContext, threadItem->ThreadId));
                        break;
                    }
                }

                PhFree(events);
            }

            PhTickThreadNodes(&threadsContext->ListContext);

            if (count != 0)
                TreeNew_SetRedraw(tnHandle, TRUE);

            if (propPageContext->PropContext->SelectThreadId)
            {
                PPH_THREAD_NODE threadNode;

                if (threadNode = PhFindThreadNode(&threadsContext->ListContext, propPageContext->PropContext->SelectThreadId))
                {
                    if (threadNode->Node.Visible)
                    {
                        TreeNew_SetFocusNode(tnHandle, &threadNode->Node);
                        TreeNew_SetMarkNode(tnHandle, &threadNode->Node);
                        TreeNew_SelectRange(tnHandle, threadNode->Node.Index, threadNode->Node.Index);
                        TreeNew_EnsureVisible(tnHandle, &threadNode->Node);
                    }
                }

                propPageContext->PropContext->SelectThreadId = NULL;
            }

            PhpUpdateThreadDetails(hwndDlg, threadsContext, FALSE);
        }
        break;
    case WM_PH_THREAD_SELECTION_CHANGED:
        {
            PhpUpdateThreadDetails(hwndDlg, threadsContext, TRUE);
        }
        break;
    }

    return FALSE;
}

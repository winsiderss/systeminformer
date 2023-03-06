/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2018-2019
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>

#include <emenu.h>
#include <hndlinfo.h>
#include <secedit.h>
#include <settings.h>

#define MSG_UPDATE (WM_APP + 1)

typedef struct _JOB_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
    DLGPROC HookProc;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
} JOB_PAGE_CONTEXT, *PJOB_PAGE_CONTEXT;

INT CALLBACK PhpJobPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpJobPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpShowJobAdvancedProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PJOB_PAGE_CONTEXT Context
    );

INT_PTR CALLBACK PhpJobStatisticsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowJobProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_opt_ PWSTR Title
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[1];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = Title ? Title : L"Job";
    propSheetHeader.nPages = 1;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    pages[0] = PhCreateJobPage(OpenObject, Context, NULL);

    PhModalPropertySheet(&propSheetHeader);
}

HPROPSHEETPAGE PhCreateJobPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_opt_ DLGPROC HookProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PJOB_PAGE_CONTEXT jobPageContext;

    jobPageContext = PhCreateAlloc(sizeof(JOB_PAGE_CONTEXT));
    memset(jobPageContext, 0, sizeof(JOB_PAGE_CONTEXT));
    jobPageContext->OpenObject = OpenObject;
    jobPageContext->Context = Context;
    jobPageContext->HookProc = HookProc;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OBJJOB);
    propSheetPage.hInstance = PhInstanceHandle;
    propSheetPage.pfnDlgProc = PhpJobPageProc;
    propSheetPage.lParam = (LPARAM)jobPageContext;
    propSheetPage.pfnCallback = PhpJobPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    // CreatePropertySheetPage would have sent PSPCB_ADDREF (below),
    // which would have added a reference.
    PhDereferenceObject(jobPageContext);

    return propSheetPageHandle;
}

INT CALLBACK PhpJobPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PJOB_PAGE_CONTEXT jobPageContext;

    jobPageContext = (PJOB_PAGE_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
    {
        PhReferenceObject(jobPageContext);
    }
    else if (uMsg == PSPCB_RELEASE)
    {
        PhDereferenceObject(jobPageContext);
    }

    return 1;
}

FORCEINLINE PJOB_PAGE_CONTEXT PhpJobPageHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return PhpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 1);
}

static VOID PhpAddLimit(
    _In_ HWND Handle,
    _In_ PWSTR Name,
    _In_ PWSTR Value
    )
{
    INT lvItemIndex;

    lvItemIndex = PhAddListViewItem(Handle, MAXINT, Name, NULL);
    PhSetListViewSubItem(Handle, lvItemIndex, 1, Value);
}

static VOID PhpAddJobProcesses(
    _In_ HWND hwndDlg,
    _In_ HANDLE JobHandle
    )
{
    PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;
    HWND processesLv;

    processesLv = GetDlgItem(hwndDlg, IDC_PROCESSES);

    if (NT_SUCCESS(PhGetJobProcessIdList(JobHandle, &processIdList)))
    {
        ULONG i;
        CLIENT_ID clientId;
        PPH_STRING name;

        clientId.UniqueThread = NULL;

        for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
        {
            clientId.UniqueProcess = (HANDLE)processIdList->ProcessIdList[i];
            name = PH_AUTO(PhGetClientIdName(&clientId));

            PhAddListViewItem(processesLv, MAXINT, PhGetString(name), NULL);
        }

        PhFree(processIdList);
    }
}

INT_PTR CALLBACK PhpJobPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PJOB_PAGE_CONTEXT jobPageContext;

    jobPageContext = PhpJobPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!jobPageContext)
        return FALSE;

    if (jobPageContext->HookProc)
    {
        if (jobPageContext->HookProc(hwndDlg, uMsg, wParam, lParam))
            return TRUE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE jobHandle;
            HWND processesLv;
            HWND limitsLv;

            processesLv = GetDlgItem(hwndDlg, IDC_PROCESSES);
            limitsLv = GetDlgItem(hwndDlg, IDC_LIMITS);
            PhSetListViewStyle(processesLv, FALSE, TRUE);
            PhSetListViewStyle(limitsLv, FALSE, TRUE);
            PhSetControlTheme(processesLv, L"explorer");
            PhSetControlTheme(limitsLv, L"explorer");
            PhSetExtendedListView(processesLv);
            PhSetExtendedListView(limitsLv);

            PhAddListViewColumn(processesLv, 0, 0, 0, LVCFMT_LEFT, 240, L"Name");
            PhAddListViewColumn(limitsLv, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(limitsLv, 1, 1, 1, LVCFMT_LEFT, 160, L"Value");
            PhLoadListViewColumnsFromSetting(L"JobListViewColumns", limitsLv);

            PhSetDialogItemText(hwndDlg, IDC_NAME, L"Unknown");

            if (NT_SUCCESS(jobPageContext->OpenObject(
                &jobHandle,
                JOB_OBJECT_QUERY,
                jobPageContext->Context
                )))
            {
                PPH_STRING jobObjectName = NULL;
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedLimits;
                JOBOBJECT_BASIC_UI_RESTRICTIONS basicUiRestrictions;

                // Name

                PhGetHandleInformation(
                    NtCurrentProcess(),
                    jobHandle,
                    ULONG_MAX,
                    NULL,
                    NULL,
                    NULL,
                    &jobObjectName
                    );
                PH_AUTO(jobObjectName);

                if (jobObjectName && jobObjectName->Length == 0)
                    jobObjectName = NULL;

                PhSetDialogItemText(hwndDlg, IDC_NAME, PhGetStringOrDefault(jobObjectName, L"(unnamed job)"));

                // Processes
                PhpAddJobProcesses(hwndDlg, jobHandle);

                // Limits

                if (NT_SUCCESS(PhGetJobExtendedLimits(jobHandle, &extendedLimits)))
                {
                    ULONG flags = extendedLimits.BasicLimitInformation.LimitFlags;

                    if (flags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
                    {
                        WCHAR value[PH_INT32_STR_LEN_1];
                        PhPrintUInt32(value, extendedLimits.BasicLimitInformation.ActiveProcessLimit);
                        PhpAddLimit(limitsLv, L"Active processes", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_AFFINITY)
                    {
                        WCHAR value[PH_PTR_STR_LEN_1];
                        PhPrintPointer(value, (PVOID)extendedLimits.BasicLimitInformation.Affinity);
                        PhpAddLimit(limitsLv, L"Affinity", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_BREAKAWAY_OK)
                    {
                        PhpAddLimit(limitsLv, L"Breakaway OK", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION)
                    {
                        PhpAddLimit(limitsLv, L"Die on unhandled exception", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_JOB_MEMORY)
                    {
                        PPH_STRING value = PhaFormatSize(extendedLimits.JobMemoryLimit, -1);
                        PhpAddLimit(limitsLv, L"Job memory", value->Buffer);
                    }

                    if (flags & JOB_OBJECT_LIMIT_JOB_TIME)
                    {
                        WCHAR value[PH_TIMESPAN_STR_LEN_1];
                        PhPrintTimeSpan(value, extendedLimits.BasicLimitInformation.PerJobUserTimeLimit.QuadPart,
                            PH_TIMESPAN_DHMS);
                        PhpAddLimit(limitsLv, L"Job time", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
                    {
                        PhpAddLimit(limitsLv, L"Kill on job close", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
                    {
                        PhpAddLimit(limitsLv, L"Priority class",
                            PhGetProcessPriorityClassString(extendedLimits.BasicLimitInformation.PriorityClass));
                    }

                    if (flags & JOB_OBJECT_LIMIT_PROCESS_MEMORY)
                    {
                        PPH_STRING value = PhaFormatSize(extendedLimits.ProcessMemoryLimit, -1);
                        PhpAddLimit(limitsLv, L"Process memory", value->Buffer);
                    }

                    if (flags & JOB_OBJECT_LIMIT_PROCESS_TIME)
                    {
                        WCHAR value[PH_TIMESPAN_STR_LEN_1];
                        PhPrintTimeSpan(value, extendedLimits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart,
                            PH_TIMESPAN_DHMS);
                        PhpAddLimit(limitsLv, L"Process time", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
                    {
                        WCHAR value[PH_INT32_STR_LEN_1];
                        PhPrintUInt32(value, extendedLimits.BasicLimitInformation.SchedulingClass);
                        PhpAddLimit(limitsLv, L"Scheduling class", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)
                    {
                        PhpAddLimit(limitsLv, L"Silent breakaway OK", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_WORKINGSET)
                    {
                        PPH_STRING value;

                        value = PhaFormatSize(extendedLimits.BasicLimitInformation.MinimumWorkingSetSize, -1);
                        PhpAddLimit(limitsLv, L"Working set minimum", value->Buffer);

                        value = PhaFormatSize(extendedLimits.BasicLimitInformation.MaximumWorkingSetSize, -1);
                        PhpAddLimit(limitsLv, L"Working set maximum", value->Buffer);
                    }
                }

                if (NT_SUCCESS(PhGetJobBasicUiRestrictions(jobHandle, &basicUiRestrictions)))
                {
                    ULONG flags = basicUiRestrictions.UIRestrictionsClass;

                    if (flags & JOB_OBJECT_UILIMIT_DESKTOP)
                        PhpAddLimit(limitsLv, L"Desktop", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_DISPLAYSETTINGS)
                        PhpAddLimit(limitsLv, L"Display settings", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_EXITWINDOWS)
                        PhpAddLimit(limitsLv, L"Exit windows", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_GLOBALATOMS)
                        PhpAddLimit(limitsLv, L"Global atoms", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_HANDLES)
                        PhpAddLimit(limitsLv, L"Handles", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_READCLIPBOARD)
                        PhpAddLimit(limitsLv, L"Read clipboard", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS)
                        PhpAddLimit(limitsLv, L"System parameters", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_WRITECLIPBOARD)
                        PhpAddLimit(limitsLv, L"Write clipboard", L"Limited");
                }

                NtClose(jobHandle);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        PhSaveListViewColumnsToSetting(L"JobListViewColumns", GetDlgItem(hwndDlg, IDC_LIMITS));
        break;
    case WM_SHOWWINDOW:
        {
            ExtendedListView_SetColumnWidth(GetDlgItem(hwndDlg, IDC_PROCESSES), 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_TERMINATE:
                {
                    if (PhShowConfirmMessage(
                        hwndDlg,
                        L"terminate",
                        L"the job",
                        L"Terminating a job will terminate all processes assigned to it.",
                        TRUE
                        ))
                    {
                        NTSTATUS status;
                        HANDLE jobHandle;

                        if (NT_SUCCESS(status = jobPageContext->OpenObject(
                            &jobHandle,
                            JOB_OBJECT_TERMINATE,
                            jobPageContext->Context
                            )))
                        {
                            status = NtTerminateJobObject(jobHandle, STATUS_SUCCESS);
                            NtClose(jobHandle);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to terminate the job", status, 0);
                    }
                }
                break;
            case IDC_ADD:
                {
                    NTSTATUS status;
                    HANDLE processId;
                    HANDLE processHandle;
                    HANDLE jobHandle;

                    while (PhShowChooseProcessDialog(
                        hwndDlg,
                        L"Select a process to add to the job permanently.",
                        &processId
                        ))
                    {
                        if (NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            PROCESS_TERMINATE | PROCESS_SET_QUOTA,
                            processId
                            )))
                        {
                            if (NT_SUCCESS(status = jobPageContext->OpenObject(
                                &jobHandle,
                                JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_QUERY,
                                jobPageContext->Context
                                )))
                            {
                                status = NtAssignProcessToJobObject(jobHandle, processHandle);

                                if (NT_SUCCESS(status))
                                {
                                    ListView_DeleteAllItems(GetDlgItem(hwndDlg, IDC_PROCESSES));
                                    PhpAddJobProcesses(hwndDlg, jobHandle);
                                }

                                NtClose(jobHandle);
                            }

                            NtClose(processHandle);
                        }

                        if (NT_SUCCESS(status))
                            break;
                        else
                            PhShowStatus(hwndDlg, L"Unable to add the process to the job", status, 0);
                    }
                }
                break;
            case IDC_ADVANCED:
                {
                    PhpShowJobAdvancedProperties(hwndDlg, jobPageContext);
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
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_PROCESSES));
                return TRUE;
            }

            PhHandleListViewNotifyBehaviors(lParam, GetDlgItem(hwndDlg, IDC_PROCESSES), PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
            PhHandleListViewNotifyBehaviors(lParam, GetDlgItem(hwndDlg, IDC_LIMITS), PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_SIZE:
        {
            ExtendedListView_SetColumnWidth(GetDlgItem(hwndDlg, IDC_PROCESSES), 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            HWND listViewHandle = NULL;

            if ((HWND)wParam == GetDlgItem(hwndDlg, IDC_PROCESSES))
                listViewHandle = GetDlgItem(hwndDlg, IDC_PROCESSES);
            else if ((HWND)wParam == GetDlgItem(hwndDlg, IDC_LIMITS))
                listViewHandle = GetDlgItem(hwndDlg, IDC_LIMITS);

            if (listViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(listViewHandle, &point);

                PhGetSelectedListViewItemParams(listViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();

                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, listViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(listViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}

VOID PhpShowJobAdvancedProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PJOB_PAGE_CONTEXT Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[2];
    PROPSHEETPAGE statisticsPage;

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Job";
    propSheetHeader.nPages = 2;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General

    memset(&statisticsPage, 0, sizeof(PROPSHEETPAGE));
    statisticsPage.dwSize = sizeof(PROPSHEETPAGE);
    statisticsPage.pszTemplate = MAKEINTRESOURCE(IDD_JOBSTATISTICS);
    statisticsPage.hInstance = PhInstanceHandle;
    statisticsPage.pfnDlgProc = PhpJobStatisticsPageProc;
    statisticsPage.lParam = (LPARAM)Context;
    pages[0] = CreatePropertySheetPage(&statisticsPage);

    // Security

    pages[1] = PhCreateSecurityPage(
        L"Job",
        L"Job",
        Context->OpenObject,
        NULL,
        Context->Context
        );

    PhModalPropertySheet(&propSheetHeader);
}

static VOID PhpRefreshJobStatisticsInfo(
    _In_ HWND hwndDlg,
    _In_ PJOB_PAGE_CONTEXT Context
    )
{
    HANDLE jobHandle = NULL;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION basicAndIo;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedLimitInfo;

    Context->OpenObject(
        &jobHandle,
        JOB_OBJECT_QUERY,
        Context->Context
        );

    if (jobHandle && NT_SUCCESS(PhGetJobBasicAndIoAccounting(
        jobHandle,
        &basicAndIo
        )))
    {
        WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1];

        PhSetDialogItemValue(hwndDlg, IDC_ZACTIVEPROCESSES_V, basicAndIo.BasicInfo.ActiveProcesses, FALSE);
        PhSetDialogItemValue(hwndDlg, IDC_ZTOTALPROCESSES_V, basicAndIo.BasicInfo.TotalProcesses, FALSE);
        PhSetDialogItemValue(hwndDlg, IDC_ZTERMINATEDPROCESSES_V, basicAndIo.BasicInfo.TotalTerminatedProcesses, FALSE);

        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.TotalUserTime.QuadPart, PH_TIMESPAN_HMSM);
        PhSetDialogItemText(hwndDlg, IDC_ZUSERTIME_V, timeSpan);
        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.TotalKernelTime.QuadPart, PH_TIMESPAN_HMSM);
        PhSetDialogItemText(hwndDlg, IDC_ZKERNELTIME_V, timeSpan);
        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.ThisPeriodTotalUserTime.QuadPart, PH_TIMESPAN_HMSM);
        PhSetDialogItemText(hwndDlg, IDC_ZUSERTIMEPERIOD_V, timeSpan);
        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.ThisPeriodTotalKernelTime.QuadPart, PH_TIMESPAN_HMSM);
        PhSetDialogItemText(hwndDlg, IDC_ZKERNELTIMEPERIOD_V, timeSpan);

        PhSetDialogItemText(hwndDlg, IDC_ZPAGEFAULTS_V, PhaFormatUInt64(basicAndIo.BasicInfo.TotalPageFaultCount, TRUE)->Buffer);

        PhSetDialogItemText(hwndDlg, IDC_ZIOREADS_V, PhaFormatUInt64(basicAndIo.IoInfo.ReadOperationCount, TRUE)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZIOREADBYTES_V, PhaFormatSize(basicAndIo.IoInfo.ReadTransferCount, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZIOWRITES_V, PhaFormatUInt64(basicAndIo.IoInfo.WriteOperationCount, TRUE)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZIOWRITEBYTES_V, PhaFormatSize(basicAndIo.IoInfo.WriteTransferCount, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZIOOTHER_V, PhaFormatUInt64(basicAndIo.IoInfo.OtherOperationCount, TRUE)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZIOOTHERBYTES_V, PhaFormatSize(basicAndIo.IoInfo.OtherTransferCount, ULONG_MAX)->Buffer);
    }
    else
    {
        PhSetDialogItemText(hwndDlg, IDC_ZACTIVEPROCESSES_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZTOTALPROCESSES_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZTERMINATEDPROCESSES_V, L"Unknown");

        PhSetDialogItemText(hwndDlg, IDC_ZUSERTIME_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZKERNELTIME_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZUSERTIMEPERIOD_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZKERNELTIMEPERIOD_V, L"Unknown");

        PhSetDialogItemText(hwndDlg, IDC_ZPAGEFAULTS_V, L"Unknown");

        PhSetDialogItemText(hwndDlg, IDC_ZIOREADS_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZIOREADBYTES_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZIOWRITES_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZIOWRITEBYTES_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZIOOTHER_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZIOOTHERBYTES_V, L"Unknown");
    }

    if (jobHandle && NT_SUCCESS(PhGetJobExtendedLimits(
        jobHandle,
        &extendedLimitInfo
        )))
    {
        PhSetDialogItemText(hwndDlg, IDC_ZPEAKPROCESSUSAGE_V, PhaFormatSize(extendedLimitInfo.PeakProcessMemoryUsed, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZPEAKJOBUSAGE_V, PhaFormatSize(extendedLimitInfo.PeakJobMemoryUsed, ULONG_MAX)->Buffer);
    }
    else
    {
        PhSetDialogItemText(hwndDlg, IDC_ZPEAKPROCESSUSAGE_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZPEAKJOBUSAGE_V, L"Unknown");
    }

    if (jobHandle)
        NtClose(jobHandle);
}

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(Context, MSG_UPDATE, 0, 0);
}

INT_PTR CALLBACK PhpJobStatisticsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PJOB_PAGE_CONTEXT jobPageContext;

    jobPageContext = PhpJobPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!jobPageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg))); // HACK

            PhpRefreshJobStatisticsInfo(hwndDlg, jobPageContext);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                ProcessesUpdatedCallback,
                hwndDlg,
                &jobPageContext->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &jobPageContext->ProcessesUpdatedRegistration);
        }
        break;
    case MSG_UPDATE:
        {
            PhpRefreshJobStatisticsInfo(hwndDlg, jobPageContext);
        }
        break;
    }

    return FALSE;
}

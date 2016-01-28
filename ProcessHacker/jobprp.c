/*
 * Process Hacker -
 *   job properties
 *
 * Copyright (C) 2010 wj32
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
#include <secedit.h>
#include <settings.h>

typedef struct _JOB_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
    DLGPROC HookProc;
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
    return (PJOB_PAGE_CONTEXT)PhpGenericPropertyPageHeader(
        hwndDlg, uMsg, wParam, lParam, L"JobPageContext");
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
            name = PhAutoDereferenceObject(PhGetClientIdName(&clientId));

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

            PhAddListViewColumn(processesLv, 0, 0, 0, LVCFMT_LEFT, 240, L"Name");

            PhAddListViewColumn(limitsLv, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(limitsLv, 1, 1, 1, LVCFMT_LEFT, 160, L"Value");

            SetDlgItemText(hwndDlg, IDC_NAME, L"Unknown");

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
                    -1,
                    NULL,
                    NULL,
                    NULL,
                    &jobObjectName
                    );
                PhAutoDereferenceObject(jobObjectName);

                if (jobObjectName && jobObjectName->Length == 0)
                    jobObjectName = NULL;

                SetDlgItemText(hwndDlg, IDC_NAME, PhGetStringOrDefault(jobObjectName, L"(unnamed job)"));

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
                        PhpAddLimit(limitsLv, L"Active Processes", value);
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
                        PhpAddLimit(limitsLv, L"Die on Unhandled Exception", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_JOB_MEMORY)
                    {
                        PPH_STRING value = PhFormatSize(extendedLimits.JobMemoryLimit, -1);
                        PhpAddLimit(limitsLv, L"Job Memory", value->Buffer);
                        PhDereferenceObject(value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_JOB_TIME)
                    {
                        WCHAR value[PH_TIMESPAN_STR_LEN_1];
                        PhPrintTimeSpan(value, extendedLimits.BasicLimitInformation.PerJobUserTimeLimit.QuadPart,
                            PH_TIMESPAN_DHMS);
                        PhpAddLimit(limitsLv, L"Job Time", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
                    {
                        PhpAddLimit(limitsLv, L"Kill on Job Close", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
                    {
                        PhpAddLimit(limitsLv, L"Priority Class",
                            PhGetProcessPriorityClassString(extendedLimits.BasicLimitInformation.PriorityClass));
                    }

                    if (flags & JOB_OBJECT_LIMIT_PROCESS_MEMORY)
                    {
                        PPH_STRING value = PhFormatSize(extendedLimits.ProcessMemoryLimit, -1);
                        PhpAddLimit(limitsLv, L"Process Memory", value->Buffer);
                        PhDereferenceObject(value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_PROCESS_TIME)
                    {
                        WCHAR value[PH_TIMESPAN_STR_LEN_1];
                        PhPrintTimeSpan(value, extendedLimits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart,
                            PH_TIMESPAN_DHMS);
                        PhpAddLimit(limitsLv, L"Process Time", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
                    {
                        WCHAR value[PH_INT32_STR_LEN_1];
                        PhPrintUInt32(value, extendedLimits.BasicLimitInformation.SchedulingClass);
                        PhpAddLimit(limitsLv, L"Scheduling Class", value);
                    }

                    if (flags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)
                    {
                        PhpAddLimit(limitsLv, L"Silent Breakaway OK", L"Enabled");
                    }

                    if (flags & JOB_OBJECT_LIMIT_WORKINGSET)
                    {
                        PPH_STRING value;

                        value = PhFormatSize(extendedLimits.BasicLimitInformation.MinimumWorkingSetSize, -1);
                        PhpAddLimit(limitsLv, L"Working Set Minimum", value->Buffer);
                        PhDereferenceObject(value);

                        value = PhFormatSize(extendedLimits.BasicLimitInformation.MaximumWorkingSetSize, -1);
                        PhpAddLimit(limitsLv, L"Working Set Maximum", value->Buffer);
                        PhDereferenceObject(value);
                    }
                }

                if (NT_SUCCESS(PhGetJobBasicUiRestrictions(jobHandle, &basicUiRestrictions)))
                {
                    ULONG flags = basicUiRestrictions.UIRestrictionsClass;

                    if (flags & JOB_OBJECT_UILIMIT_DESKTOP)
                        PhpAddLimit(limitsLv, L"Desktop", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_DISPLAYSETTINGS)
                        PhpAddLimit(limitsLv, L"Display Settings", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_EXITWINDOWS)
                        PhpAddLimit(limitsLv, L"Exit Windows", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_GLOBALATOMS)
                        PhpAddLimit(limitsLv, L"Global Atoms", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_HANDLES)
                        PhpAddLimit(limitsLv, L"Handles", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_READCLIPBOARD)
                        PhpAddLimit(limitsLv, L"Read Clipboard", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS)
                        PhpAddLimit(limitsLv, L"System Parameters", L"Limited");
                    if (flags & JOB_OBJECT_UILIMIT_WRITECLIPBOARD)
                        PhpAddLimit(limitsLv, L"Write Clipboard", L"Limited");
                }

                NtClose(jobHandle);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
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
            PhHandleListViewNotifyBehaviors(lParam, GetDlgItem(hwndDlg, IDC_PROCESSES), PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
            PhHandleListViewNotifyBehaviors(lParam, GetDlgItem(hwndDlg, IDC_LIMITS), PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
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
    PH_STD_OBJECT_SECURITY stdObjectSecurity;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Job";
    propSheetHeader.nPages = 2;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General

    memset(&statisticsPage, 0, sizeof(PROPSHEETPAGE));
    statisticsPage.dwSize = sizeof(PROPSHEETPAGE);
    statisticsPage.pszTemplate = MAKEINTRESOURCE(IDD_JOBSTATISTICS);
    statisticsPage.pfnDlgProc = PhpJobStatisticsPageProc;
    statisticsPage.lParam = (LPARAM)Context;
    pages[0] = CreatePropertySheetPage(&statisticsPage);

    // Security

    stdObjectSecurity.OpenObject = Context->OpenObject;
    stdObjectSecurity.ObjectType = L"Job";
    stdObjectSecurity.Context = Context->Context;

    if (PhGetAccessEntries(L"Job", &accessEntries, &numberOfAccessEntries))
    {
        pages[1] = PhCreateSecurityPage(
            L"Job",
            PhStdGetObjectSecurity,
            PhStdSetObjectSecurity,
            &stdObjectSecurity,
            accessEntries,
            numberOfAccessEntries
            );
        PhFree(accessEntries);
    }

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

        SetDlgItemInt(hwndDlg, IDC_ZACTIVEPROCESSES_V, basicAndIo.BasicInfo.ActiveProcesses, FALSE);
        SetDlgItemInt(hwndDlg, IDC_ZTOTALPROCESSES_V, basicAndIo.BasicInfo.TotalProcesses, FALSE);
        SetDlgItemInt(hwndDlg, IDC_ZTERMINATEDPROCESSES_V, basicAndIo.BasicInfo.TotalTerminatedProcesses, FALSE);

        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.TotalUserTime.QuadPart, PH_TIMESPAN_HMSM);
        SetDlgItemText(hwndDlg, IDC_ZUSERTIME_V, timeSpan);
        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.TotalKernelTime.QuadPart, PH_TIMESPAN_HMSM);
        SetDlgItemText(hwndDlg, IDC_ZKERNELTIME_V, timeSpan);
        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.ThisPeriodTotalUserTime.QuadPart, PH_TIMESPAN_HMSM);
        SetDlgItemText(hwndDlg, IDC_ZUSERTIMEPERIOD_V, timeSpan);
        PhPrintTimeSpan(timeSpan, basicAndIo.BasicInfo.ThisPeriodTotalKernelTime.QuadPart, PH_TIMESPAN_HMSM);
        SetDlgItemText(hwndDlg, IDC_ZKERNELTIMEPERIOD_V, timeSpan);

        SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTS_V, PhaFormatUInt64(basicAndIo.BasicInfo.TotalPageFaultCount, TRUE)->Buffer);

        SetDlgItemText(hwndDlg, IDC_ZIOREADS_V, PhaFormatUInt64(basicAndIo.IoInfo.ReadOperationCount, TRUE)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZIOREADBYTES_V, PhaFormatSize(basicAndIo.IoInfo.ReadTransferCount, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZIOWRITES_V, PhaFormatUInt64(basicAndIo.IoInfo.WriteOperationCount, TRUE)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZIOWRITEBYTES_V, PhaFormatSize(basicAndIo.IoInfo.WriteTransferCount, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZIOOTHER_V, PhaFormatUInt64(basicAndIo.IoInfo.OtherOperationCount, TRUE)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZIOOTHERBYTES_V, PhaFormatSize(basicAndIo.IoInfo.OtherTransferCount, -1)->Buffer);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_ZACTIVEPROCESSES_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZTOTALPROCESSES_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZTERMINATEDPROCESSES_V, L"Unknown");

        SetDlgItemText(hwndDlg, IDC_ZUSERTIME_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZKERNELTIME_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZUSERTIMEPERIOD_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZKERNELTIMEPERIOD_V, L"Unknown");

        SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTS_V, L"Unknown");

        SetDlgItemText(hwndDlg, IDC_ZIOREADS_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZIOREADBYTES_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZIOWRITES_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZIOWRITEBYTES_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZIOOTHER_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZIOOTHERBYTES_V, L"Unknown");
    }

    if (jobHandle && NT_SUCCESS(PhGetJobExtendedLimits(
        jobHandle,
        &extendedLimitInfo
        )))
    {
        SetDlgItemText(hwndDlg, IDC_ZPEAKPROCESSUSAGE_V, PhaFormatSize(extendedLimitInfo.PeakProcessMemoryUsed, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZPEAKJOBUSAGE_V, PhaFormatSize(extendedLimitInfo.PeakJobMemoryUsed, -1)->Buffer);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_ZPEAKPROCESSUSAGE_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZPEAKJOBUSAGE_V, L"Unknown");
    }

    if (jobHandle)
        NtClose(jobHandle);
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
            PhpRefreshJobStatisticsInfo(hwndDlg, jobPageContext);
            SetTimer(hwndDlg, 1, PhGetIntegerSetting(L"UpdateInterval"), NULL);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 1)
            {
                PhpRefreshJobStatisticsInfo(hwndDlg, jobPageContext);
            }
        }
        break;
    }

    return FALSE;
}

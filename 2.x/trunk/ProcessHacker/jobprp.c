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

typedef struct _JOB_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
    DLGPROC HookProc;
} JOB_PAGE_CONTEXT, *PJOB_PAGE_CONTEXT;

INT CALLBACK PhpJobPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpJobPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowJobProperties(
    __in HWND ParentWindowHandle,
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in_opt PWSTR Title
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

    PropertySheet(&propSheetHeader);
}

HPROPSHEETPAGE PhCreateJobPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in_opt DLGPROC HookProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PJOB_PAGE_CONTEXT jobPageContext;

    if (!NT_SUCCESS(PhCreateAlloc(&jobPageContext, sizeof(JOB_PAGE_CONTEXT))))
        return NULL;

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
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
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
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PJOB_PAGE_CONTEXT jobPageContext;

    if (uMsg != WM_INITDIALOG)
    {
        jobPageContext = (PJOB_PAGE_CONTEXT)GetProp(hwndDlg, L"JobPageContext");
    }
    else
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

        jobPageContext = (PJOB_PAGE_CONTEXT)propSheetPage->lParam;
        SetProp(hwndDlg, L"JobPageContext", (HANDLE)jobPageContext);
    }

    return jobPageContext;
}

INT_PTR CALLBACK PhpJobPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
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
                PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;

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
                PHA_DEREFERENCE(jobObjectName);

                if (jobObjectName->Length == 0)
                    jobObjectName = NULL;

                SetDlgItemText(hwndDlg, IDC_NAME, PhGetStringOrDefault(jobObjectName, L"(unnamed job)"));

                // Processes

                if (NT_SUCCESS(PhGetJobProcessIdList(jobHandle, &processIdList)))
                {
                    ULONG i;
                    CLIENT_ID clientId;
                    PPH_STRING name;

                    clientId.UniqueThread = NULL;

                    for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
                    {
                        clientId.UniqueProcess = (HANDLE)processIdList->ProcessIdList[i];
                        name = PHA_DEREFERENCE(PhGetClientIdName(&clientId));

                        PhAddListViewItem(processesLv, MAXINT, PhGetString(name), NULL);
                    }

                    PhFree(processIdList);
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
            }
        }
        break;
    }

    return FALSE;
}

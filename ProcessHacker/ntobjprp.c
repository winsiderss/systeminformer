/*
 * Process Hacker - 
 *   properties for NT objects
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

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

HPROPSHEETPAGE PhpCommonCreatePage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in PWSTR Template,
    __in DLGPROC DlgProc
    );

INT CALLBACK PhpCommonPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpEventPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpSemaphorePageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static HPROPSHEETPAGE PhpCommonCreatePage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in PWSTR Template,
    __in DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    if (!NT_SUCCESS(PhCreateAlloc(&pageContext, sizeof(COMMON_PAGE_CONTEXT))))
        return NULL;

    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->OpenObject = OpenObject;
    pageContext->Context = Context;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = Template;
    propSheetPage.pfnDlgProc = DlgProc;
    propSheetPage.lParam = (LPARAM)pageContext;
    propSheetPage.pfnCallback = PhpCommonPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    // CreatePropertySheetPage would have sent PSPCB_ADDREF (below), 
    // which would have added a reference.
    PhDereferenceObject(pageContext);

    return propSheetPageHandle;
}

INT CALLBACK PhpCommonPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = (PCOMMON_PAGE_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
    {
        PhReferenceObject(pageContext);
    }
    else if (uMsg == PSPCB_RELEASE)
    {
        PhDereferenceObject(pageContext);
    }

    return 1;
}

FORCEINLINE PCOMMON_PAGE_CONTEXT PhpCommonPageHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return (PCOMMON_PAGE_CONTEXT)PhpGenericPropertyPageHeader(
        hwndDlg, uMsg, wParam, lParam, L"PageContext");
}

HPROPSHEETPAGE PhCreateEventPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJEVENT),
        PhpEventPageProc
        );
}

static VOID PhpRefreshEventPageInfo(
    __in HWND hwndDlg,
    __in PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE eventHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &eventHandle,
        EVENT_QUERY_STATE,
        PageContext->Context
        )))
    {
        EVENT_BASIC_INFORMATION basicInfo;
        PWSTR eventType = L"Unknown";
        PWSTR eventState = L"Unknown";

        if (NT_SUCCESS(PhGetEventBasicInformation(eventHandle, &basicInfo)))
        {
            switch (basicInfo.EventType)
            {
            case NotificationEvent:
                eventType = L"Notification";
                break;
            case SynchronizationEvent:
                eventType = L"Synchronization";
                break;
            }

            eventState = basicInfo.EventState > 0 ? L"True" : L"False";
        }

        SetDlgItemText(hwndDlg, IDC_TYPE, eventType);
        SetDlgItemText(hwndDlg, IDC_SIGNALED, eventState);

        NtClose(eventHandle);
    }
}

INT_PTR CALLBACK PhpEventPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshEventPageInfo(hwndDlg, pageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_SET:
            case IDC_RESET:
            case IDC_PULSE:
                {
                    NTSTATUS status;
                    HANDLE eventHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &eventHandle,
                        EVENT_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (LOWORD(wParam))
                        {
                        case IDC_SET:
                            NtSetEvent(eventHandle, NULL);
                            break;
                        case IDC_RESET:
                            NtResetEvent(eventHandle, NULL);
                            break;
                        case IDC_PULSE:
                            NtPulseEvent(eventHandle, NULL);
                            break;
                        }

                        NtClose(eventHandle);
                    }

                    PhpRefreshEventPageInfo(hwndDlg, pageContext);

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the event", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateSemaphorePage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJSEMAPHORE),
        PhpSemaphorePageProc
        );
}

static VOID PhpRefreshSemaphorePageInfo(
    __in HWND hwndDlg,
    __in PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE semaphoreHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &semaphoreHandle,
        SEMAPHORE_QUERY_STATE,
        PageContext->Context
        )))
    {
        SEMAPHORE_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetSemaphoreBasicInformation(semaphoreHandle, &basicInfo)))
        {
            SetDlgItemInt(hwndDlg, IDC_CURRENTCOUNT, basicInfo.CurrentCount, TRUE);
            SetDlgItemInt(hwndDlg, IDC_MAXIMUMCOUNT, basicInfo.MaximumCount, TRUE);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_CURRENTCOUNT, L"Unknown");
            SetDlgItemText(hwndDlg, IDC_MAXIMUMCOUNT, L"Unknown");
        }

        NtClose(semaphoreHandle);
    }
}

INT_PTR CALLBACK PhpSemaphorePageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshSemaphorePageInfo(hwndDlg, pageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ACQUIRE:
            case IDC_RELEASE:
                {
                    NTSTATUS status;
                    HANDLE semaphoreHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &semaphoreHandle,
                        LOWORD(wParam) == IDC_ACQUIRE ? SYNCHRONIZE : SEMAPHORE_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (LOWORD(wParam))
                        {
                        case IDC_ACQUIRE:
                            {
                                LARGE_INTEGER timeout;

                                timeout.QuadPart = 0;

                                if (NtWaitForSingleObject(semaphoreHandle, FALSE, &timeout) != STATUS_WAIT_0)
                                {
                                    PhShowError(hwndDlg, L"Unable to acquire the semaphore.");
                                }
                            }
                            break;
                        case IDC_RELEASE:
                            NtReleaseSemaphore(semaphoreHandle, 1, NULL);
                            break;
                        }

                        NtClose(semaphoreHandle);
                    }

                    PhpRefreshSemaphorePageInfo(hwndDlg, pageContext);

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the semaphore", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

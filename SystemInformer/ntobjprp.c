/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *
 */

#include <phapp.h>
#include <phsettings.h>

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

HPROPSHEETPAGE PhpCommonCreatePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    );

INT CALLBACK PhpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpEventPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpEventPairPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpSemaphorePageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpTimerPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static HPROPSHEETPAGE PhpCommonCreatePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhCreateAlloc(sizeof(COMMON_PAGE_CONTEXT));
    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->OpenObject = OpenObject;
    pageContext->Context = Context;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = Template;
    propSheetPage.hInstance = PhInstanceHandle;
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
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return PhpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 2);
}

HPROPSHEETPAGE PhCreateEventPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
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
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
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

        PhSetDialogItemText(hwndDlg, IDC_TYPE, eventType);
        PhSetDialogItemText(hwndDlg, IDC_SIGNALED, eventState);

        NtClose(eventHandle);
    }
}

INT_PTR CALLBACK PhpEventPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
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

HPROPSHEETPAGE PhCreateEventPairPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJEVENTPAIR),
        PhpEventPairPageProc
        );
}

INT_PTR CALLBACK PhpEventPairPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            // Nothing
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SETLOW:
            case IDC_SETHIGH:
                {
                    NTSTATUS status;
                    HANDLE eventPairHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &eventPairHandle,
                        EVENT_PAIR_ALL_ACCESS,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_SETLOW:
                            NtSetLowEventPair(eventPairHandle);
                            break;
                        case IDC_SETHIGH:
                            NtSetHighEventPair(eventPairHandle);
                            break;
                        }

                        NtClose(eventPairHandle);
                    }

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the event pair", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateSemaphorePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
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
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
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
            PhSetDialogItemValue(hwndDlg, IDC_CURRENTCOUNT, basicInfo.CurrentCount, TRUE);
            PhSetDialogItemValue(hwndDlg, IDC_MAXIMUMCOUNT, basicInfo.MaximumCount, TRUE);
        }
        else
        {
            PhSetDialogItemText(hwndDlg, IDC_CURRENTCOUNT, L"Unknown");
            PhSetDialogItemText(hwndDlg, IDC_MAXIMUMCOUNT, L"Unknown");
        }

        NtClose(semaphoreHandle);
    }
}

INT_PTR CALLBACK PhpSemaphorePageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_ACQUIRE:
            case IDC_RELEASE:
                {
                    NTSTATUS status;
                    HANDLE semaphoreHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &semaphoreHandle,
                        GET_WM_COMMAND_ID(wParam, lParam) == IDC_ACQUIRE ? SYNCHRONIZE : SEMAPHORE_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_ACQUIRE:
                            {
                                LARGE_INTEGER timeout;

                                timeout.QuadPart = 0;
                                NtWaitForSingleObject(semaphoreHandle, FALSE, &timeout);
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

HPROPSHEETPAGE PhCreateTimerPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJTIMER),
        PhpTimerPageProc
        );
}

static VOID PhpRefreshTimerPageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE timerHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &timerHandle,
        TIMER_QUERY_STATE,
        PageContext->Context
        )))
    {
        TIMER_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetTimerBasicInformation(timerHandle, &basicInfo)))
        {
            PhSetDialogItemText(hwndDlg, IDC_SIGNALED, basicInfo.TimerState ? L"True" : L"False");
        }
        else
        {
            PhSetDialogItemText(hwndDlg, IDC_SIGNALED, L"Unknown");
        }

        NtClose(timerHandle);
    }
}

INT_PTR CALLBACK PhpTimerPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PhpRefreshTimerPageInfo(hwndDlg, pageContext);
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_CANCEL:
                {
                    NTSTATUS status;
                    HANDLE timerHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &timerHandle,
                        TIMER_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_CANCEL:
                            NtCancelTimer(timerHandle, NULL);
                            break;
                        }

                        NtClose(timerHandle);
                    }

                    PhpRefreshTimerPageInfo(hwndDlg, pageContext);

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the timer", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

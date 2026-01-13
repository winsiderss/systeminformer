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
#include <kphuser.h>
#include <ksisup.h>
#include <hndlinfo.h>
#include <emenu.h>
#include <mainwnd.h>
#include <procprv.h>
#include <phafd.h>

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PPH_CLOSE_OBJECT CloseObject;
    PVOID Context;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

HPROPSHEETPAGE PhpCommonCreatePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context,
    _In_ PCWSTR Template,
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

INT_PTR CALLBACK PhpAfdSocketPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

HPROPSHEETPAGE PhpCommonCreatePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context,
    _In_ PCWSTR Template,
    _In_ DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhCreateAlloc(sizeof(COMMON_PAGE_CONTEXT));
    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->OpenObject = OpenObject;
    pageContext->CloseObject = CloseObject;
    pageContext->Context = Context;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = Template;
    propSheetPage.hInstance = NtCurrentImageBase();
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
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        CloseObject,
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
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        CloseObject,
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
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        CloseObject,
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
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        CloseObject,
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

typedef struct _MAPPINGS_PAGE_CONTEXT
{
    HANDLE ProcessId;
    HANDLE SectionHandle;
    DLGPROC HookProc;
    HWND WindowHandle;
    HWND ListViewHandle;
    PKPH_SECTION_MAPPINGS_INFORMATION SectionInfo;

} MAPPINGS_PAGE_CONTEXT, *PMAPPINGS_PAGE_CONTEXT;

VOID PhpEnumerateMappingsEntries(
    _In_ PMAPPINGS_PAGE_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    CLIENT_ID clientId;

    if (KsiLevel() < KphLevelMed)
    {
        PPH_STRING statusText;
        HWND statusWindow;

        statusWindow = GetDlgItem(Context->WindowHandle, IDC_TEXT);
        ShowWindow(Context->ListViewHandle, SW_HIDE);
        ShowWindow(statusWindow, SW_SHOW);

        statusText = PhGetKsiNotConnectedString(
            L"Viewing active mappings requires a connection to the kernel driver."
            );
        PhSetWindowText(statusWindow, PhGetString(statusText));
        //SendMessage(statusWindow, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        PhSetDialogFocus(GetParent(Context->WindowHandle), GetDlgItem(GetParent(Context->WindowHandle), IDCANCEL));
        PhDereferenceObject(statusText);

        return;
    }

    clientId.UniqueProcess = Context->ProcessId;
    clientId.UniqueThread = NULL;

    status = KphOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, &clientId);

    if (!NT_SUCCESS(status))
        return;

    status = KphQueryObjectSectionMappingsInfo(
        processHandle,
        Context->SectionHandle,
        &Context->SectionInfo
        );

    NtClose(processHandle);

    if (!NT_SUCCESS(status))
        return;

    for (ULONG i = 0; i < Context->SectionInfo->NumberOfMappings; i++)
    {
        PKPH_SECTION_MAP_ENTRY info = &Context->SectionInfo->Mappings[i];
        INT lvItemIndex;
        WCHAR value[PH_INT64_STR_LEN_1];

        if (info->ViewMapType == VIEW_MAP_TYPE_PROCESS)
        {
            PPH_STRING string = NULL;

            clientId.UniqueProcess = info->ProcessId;
            clientId.UniqueThread = NULL;

            string = PhStdGetClientIdName(&clientId);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, string->Buffer, info);
            PhDereferenceObject(string);
        }
        else if (info->ViewMapType == VIEW_MAP_TYPE_SESSION)
        {
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, L"Session", info);
        }
        else if (info->ViewMapType == VIEW_MAP_TYPE_SYSTEM_CACHE)
        {
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, L"System", info);
        }
        else
        {
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, L"Unknown", info);
        }

        PhPrintPointer(value, info->StartVa);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, value);

        PhPrintPointer(value, info->EndVa);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, value);

        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhaFormatSize((ULONG64)info->EndVa - (ULONG64)info->StartVa, ULONG_MAX)->Buffer);
    }
}

VOID PhpShowProcessForMapping(
    _In_ HWND hwndDlg,
    _In_ PKPH_SECTION_MAP_ENTRY Entry
)
{
    PPH_PROCESS_ITEM processItem;

    assert(Entry->ViewMapType == VIEW_MAP_TYPE_PROCESS);
    if (processItem = PhReferenceProcessItem(Entry->ProcessId))
    {
        //
        // TODO would like this to show the process properties
        // memory tab and select the info->StartVa
        //
        SystemInformer_ShowProcessProperties(processItem);
        PhDereferenceObject(processItem);
    }
    else
    {
        PhShowStatus(hwndDlg, L"The process does not exist.", STATUS_INVALID_CID, 0);
    }
}

INT_PTR CALLBACK PhpMappingsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMAPPINGS_PAGE_CONTEXT context;

    context = PhpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 3);

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"View");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Start");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"End");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_RIGHT, 60, L"Size");
            PhSetExtendedListView(context->ListViewHandle);

            PhpEnumerateMappingsEntries(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            if (context->SectionInfo)
                PhFree(context->SectionInfo);

            PhFree(context);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PKPH_SECTION_MAP_ENTRY info = NULL;

            if (ListView_GetSelectedCount(context->ListViewHandle) == 1)
                info = PhGetSelectedListViewItemParam(context->ListViewHandle);

            POINT point;
            PPH_EMENU menu;
            PPH_EMENU item;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            if (point.x == -1 && point.y == -1)
                PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

            menu = PhCreateEMenu();
            if (info && info->ViewMapType == VIEW_MAP_TYPE_PROCESS)
            {
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Go to process", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            }
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
            PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

            item = PhShowEMenu(
                menu,
                hwndDlg,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                point.x,
                point.y
                );

            if (item && item->Id != ULONG_MAX && !PhHandleCopyListViewEMenuItem(item))
            {
                switch (item->Id)
                {
                case IDC_COPY:
                    {
                        PhCopyListView(context->ListViewHandle);
                    }
                    break;
                case 1:
                    {
                        assert(info);
                        PhpShowProcessForMapping(hwndDlg, info);
                    }
                    break;
                }

                PhDestroyEMenu(menu);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateMappingsPage(
    _In_ HANDLE ProcessId,
    _In_ HANDLE SectionHandle
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PMAPPINGS_PAGE_CONTEXT mappingsPageContext;

    mappingsPageContext = PhAllocateZero(sizeof(MAPPINGS_PAGE_CONTEXT));
    mappingsPageContext->ProcessId = ProcessId;
    mappingsPageContext->SectionHandle = SectionHandle;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_DEFAULT;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OBJMAPPINGS);
    propSheetPage.hInstance = NtCurrentImageBase();
    propSheetPage.pfnDlgProc = PhpMappingsPageProc;
    propSheetPage.lParam = (LPARAM)mappingsPageContext;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);

    return propSheetPageHandle;
}

typedef struct _PHP_AFD_SOCKET_PAGE_CONTEXT
{
    HANDLE ProcessId;
    HANDLE HandleValue;
    HWND ListViewHandle;
    PPH_LISTVIEW_CONTEXT ListViewContext;
    PH_LAYOUT_MANAGER LayoutManager;
} PHP_AFD_SOCKET_PAGE_CONTEXT, *PPHP_AFD_SOCKET_PAGE_CONTEXT;

typedef enum _PHP_AFD_SOCKET_GROUP
{
    PH_AFD_SOCKET_GROUP_SHARED,
    PH_AFD_SOCKET_GROUP_ADDRESSES,
    PH_AFD_SOCKET_GROUP_INFOCLASS,
    PH_AFD_SOCKET_GROUP_TDI,
    PH_AFD_SOCKET_GROUP_SO,
    PH_AFD_SOCKET_GROUP_IP,
    PH_AFD_SOCKET_GROUP_TCP,
    PH_AFD_SOCKET_GROUP_TCP_INFO,
    PH_AFD_SOCKET_GROUP_UDP,
    PH_AFD_SOCKET_GROUP_HVSOCKET,
} PHP_AFD_SOCKET_GROUP;

typedef enum _PHP_AFD_SOCKET_ITEM
{
    PH_AFD_SOCKET_ITEM_STATE,
    PH_AFD_SOCKET_ITEM_TYPE,
    PH_AFD_SOCKET_ITEM_ADDRESS_FAMILY,
    PH_AFD_SOCKET_ITEM_PROTOCOL,
    PH_AFD_SOCKET_ITEM_CATALOG_ENTRY_ID,
    PH_AFD_SOCKET_ITEM_PROVIDER_ID,
    PH_AFD_SOCKET_ITEM_PROVIDER_FLAGS,
    PH_AFD_SOCKET_ITEM_SERVICE_FLAGS,
    PH_AFD_SOCKET_ITEM_SEND_TIMEOUT,
    PH_AFD_SOCKET_ITEM_RECEIVE_TIMEOUT,
    PH_AFD_SOCKET_ITEM_SEND_BUFFER_SIZE,
    PH_AFD_SOCKET_ITEM_RECEIVE_BUFFER_SIZE,
    PH_AFD_SOCKET_ITEM_CREATION_FLAGS,
    PH_AFD_SOCKET_ITEM_FLAGS,

    PH_AFD_SOCKET_ITEM_ADDRESS,
    PH_AFD_SOCKET_ITEM_REMOTE_ADDRESS,

    PH_AFD_SOCKET_ITEM_CONNECT_TIME,
    PH_AFD_SOCKET_ITEM_DELIVERY_AVAILABLE,
    PH_AFD_SOCKET_ITEM_PENDED_RECEIVE_REQUESTS,
    PH_AFD_SOCKET_ITEM_SENDS_PENDING,
    PH_AFD_SOCKET_ITEM_RECEIVE_WINDOW_SIZE,
    PH_AFD_SOCKET_ITEM_SEND_WINDOW_SIZE,
    PH_AFD_SOCKET_ITEM_MAX_SEND_SIZE,
    PH_AFD_SOCKET_ITEM_GROUP_ID,
    PH_AFD_SOCKET_ITEM_GROUP_TYPE,

    PH_AFD_SOCKET_ITEM_TDI_ADDRESS_DEVICE,
    PH_AFD_SOCKET_ITEM_TDI_CONNECTION_DEVICE,

    PH_AFD_SOCKET_ITEM_SO_REUSEADDR,
    PH_AFD_SOCKET_ITEM_SO_KEEPALIVE,
    PH_AFD_SOCKET_ITEM_SO_DONTROUTE,
    PH_AFD_SOCKET_ITEM_SO_BROADCAST,
    PH_AFD_SOCKET_ITEM_SO_OOBINLINE,
    PH_AFD_SOCKET_ITEM_SO_RCVBUF,
    PH_AFD_SOCKET_ITEM_SO_MAX_MSG_SIZE,
    PH_AFD_SOCKET_ITEM_SO_CONDITIONAL_ACCEPT,
    PH_AFD_SOCKET_ITEM_SO_PAUSE_ACCEPT,
    PH_AFD_SOCKET_ITEM_SO_COMPARTMENT_ID,
    PH_AFD_SOCKET_ITEM_SO_RANDOMIZE_PORT,
    PH_AFD_SOCKET_ITEM_SO_PORT_SCALABILITY,
    PH_AFD_SOCKET_ITEM_SO_REUSE_UNICASTPORT,
    PH_AFD_SOCKET_ITEM_SO_EXCLUSIVEADDRUSE,

    PH_AFD_SOCKET_ITEM_IP_HDRINCL,
    PH_AFD_SOCKET_ITEM_IP_TOS,
    PH_AFD_SOCKET_ITEM_IP_TTL,
    PH_AFD_SOCKET_ITEM_IP_MULTICAST_IF,
    PH_AFD_SOCKET_ITEM_IP_MULTICAST_TTL,
    PH_AFD_SOCKET_ITEM_IP_MULTICAST_LOOP,
    PH_AFD_SOCKET_ITEM_IP_DONTFRAGMENT,
    PH_AFD_SOCKET_ITEM_IP_PKTINFO,
    PH_AFD_SOCKET_ITEM_IP_RECVTTL,
    PH_AFD_SOCKET_ITEM_IP_RECEIVE_BROADCAST,
    PH_AFD_SOCKET_ITEM_IP_PROTECTION_LEVEL,
    PH_AFD_SOCKET_ITEM_IP_RECVIF,
    PH_AFD_SOCKET_ITEM_IP_RECVDSTADDR,
    PH_AFD_SOCKET_ITEM_IP_V6ONLY,
    PH_AFD_SOCKET_ITEM_IP_IFLIST,
    PH_AFD_SOCKET_ITEM_IP_UNICAST_IF,
    PH_AFD_SOCKET_ITEM_IP_RECVRTHDR,
    PH_AFD_SOCKET_ITEM_IP_RECVTOS,
    PH_AFD_SOCKET_ITEM_IP_ORIGINAL_ARRIVAL_IF,
    PH_AFD_SOCKET_ITEM_IP_RECVECN,
    PH_AFD_SOCKET_ITEM_IP_PKTINFO_EX,
    PH_AFD_SOCKET_ITEM_IP_WFP_REDIRECT_RECORDS,
    PH_AFD_SOCKET_ITEM_IP_WFP_REDIRECT_CONTEXT,
    PH_AFD_SOCKET_ITEM_IP_MTU_DISCOVER,
    PH_AFD_SOCKET_ITEM_IP_MTU,
    PH_AFD_SOCKET_ITEM_IP_RECVERR,
    PH_AFD_SOCKET_ITEM_IP_USER_MTU,

    PH_AFD_SOCKET_ITEM_TCP_NODELAY,
    PH_AFD_SOCKET_ITEM_TCP_EXPEDITED,
    PH_AFD_SOCKET_ITEM_TCP_KEEPALIVE,
    PH_AFD_SOCKET_ITEM_TCP_MAXSEG,
    PH_AFD_SOCKET_ITEM_TCP_MAXRT,
    PH_AFD_SOCKET_ITEM_TCP_STDURG,
    PH_AFD_SOCKET_ITEM_TCP_NOURG,
    PH_AFD_SOCKET_ITEM_TCP_ATMARK,
    PH_AFD_SOCKET_ITEM_TCP_NOSYNRETRIES,
    PH_AFD_SOCKET_ITEM_TCP_TIMESTAMPS,
    PH_AFD_SOCKET_ITEM_TCP_CONGESTION_ALGORITHM,
    PH_AFD_SOCKET_ITEM_TCP_DELAY_FIN_ACK,
    PH_AFD_SOCKET_ITEM_TCP_MAXRTMS,
    PH_AFD_SOCKET_ITEM_TCP_FASTOPEN,
    PH_AFD_SOCKET_ITEM_TCP_KEEPCNT,
    PH_AFD_SOCKET_ITEM_TCP_KEEPINTVL,
    PH_AFD_SOCKET_ITEM_TCP_FAIL_CONNECT_ON_ICMP_ERROR,

    PH_AFD_SOCKET_ITEM_TCP_INFO_STATE,
    PH_AFD_SOCKET_ITEM_TCP_INFO_MSS,
    PH_AFD_SOCKET_ITEM_TCP_INFO_CONNECTION_TIME,
    PH_AFD_SOCKET_ITEM_TCP_INFO_TIMESTAMPS_ENABLED,
    PH_AFD_SOCKET_ITEM_TCP_INFO_RTT,
    PH_AFD_SOCKET_ITEM_TCP_INFO_MINRTT,
    PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_IN_FLIGHT,
    PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_WINDOW,
    PH_AFD_SOCKET_ITEM_TCP_INFO_SEND_WINDOW,
    PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVE_WINDOW,
    PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVE_BUFFER,
    PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_OUT,
    PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_IN,
    PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_REORDERED,
    PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_RETRANSMITTED,
    PH_AFD_SOCKET_ITEM_TCP_INFO_FAST_RETRANSMIT,
    PH_AFD_SOCKET_ITEM_TCP_INFO_DUPLICATE_ACKS_IN,
    PH_AFD_SOCKET_ITEM_TCP_INFO_TIMEOUT_EPISODES,
    PH_AFD_SOCKET_ITEM_TCP_INFO_SYN_RETRANSMITS,
    PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_TRANSITIONS,
    PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_TIME,
    PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_BYTES,
    PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_TRANSITIONS,
    PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_TIME,
    PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_BYTES,
    PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_TRANSITIONS,
    PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_TIME,
    PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_BYTES,
    PH_AFD_SOCKET_ITEM_TCP_INFO_OUT_OF_ORDER_PACKETS,
    PH_AFD_SOCKET_ITEM_TCP_INFO_ECN_NEGOTIATED,
    PH_AFD_SOCKET_ITEM_TCP_INFO_ECE_ACKS_IN,
    PH_AFD_SOCKET_ITEM_TCP_INFO_PTO_EPISODES,

    PH_AFD_SOCKET_ITEM_UDP_NOCHECKSUM,
    PH_AFD_SOCKET_ITEM_UDP_SEND_MSG_SIZE,
    PH_AFD_SOCKET_ITEM_UDP_RECV_MAX_COALESCED_SIZE,

    PH_AFD_SOCKET_ITEM_HVSOCKET_CONNECT_TIMEOUT,
    PH_AFD_SOCKET_ITEM_HVSOCKET_CONTAINER_PASSTHRU,
    PH_AFD_SOCKET_ITEM_HVSOCKET_CONNECTED_SUSPEND,
    PH_AFD_SOCKET_ITEM_HVSOCKET_HIGH_VTL,
} PHP_AFD_SOCKET_ITEM;

VOID PhAddSocketListViewItem(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text
    )
{
    PhListView_AddGroupItem(Context->ListViewContext, GroupId, Index, Text, UlongToPtr(Index));
}

VOID PhSetSocketListViewItem(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG Index,
    _In_ PCWSTR Text
    )
{
    LONG index = PhFindListViewItemByParam(Context->ListViewHandle, INT_ERROR, UlongToPtr(Index));

    if (index != INT_ERROR)
    {
        PhListView_SetSubItem(Context->ListViewContext, index, 1, Text);
    }
}

VOID PhSetSocketListViewItemBoolean(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG Index,
    _In_ ULONG Value
    )
{
    PhSetSocketListViewItem(Context, Index, Value ? L"True" : L"False");
}

VOID PhSetSocketListViewItemBytes(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG Index,
    _In_ ULONG64 Value
    )
{
    PPH_STRING itemString;

    itemString = PhFormatSize(Value, -1);
    PhSetSocketListViewItem(Context, Index, PhGetString(itemString));
    PhDereferenceObject(itemString);
}

VOID PhSetSocketListViewItemDecimal(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG Index,
    _In_ ULONG Value
    )
{
    PPH_STRING itemString;

    itemString = PhFormatString(L"%d", Value);
    PhSetSocketListViewItem(Context, Index, PhGetString(itemString));
    PhDereferenceObject(itemString);
}

VOID PhSetSocketListViewItemTimeSpan(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG Index,
    _In_ ULONG64 TimeSpan
    )
{
    PPH_STRING itemString;

    if (TimeSpan == 0)
    {
        PhSetSocketListViewItem(Context, Index, L"None");
        return;
    }

    itemString = PhFormatTimeSpanRelative(TimeSpan);
    PhSetSocketListViewItem(Context, Index, PhGetString(itemString));
    PhDereferenceObject(itemString);
}

VOID PhSetSocketListViewItemTimeAgo(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context,
    _In_ LONG Index,
    _In_ ULONG64 Duration
)
{
    LARGE_INTEGER absoluteTime;
    SYSTEMTIME absoluteTimeFields;
    PPH_STRING absoluteTimeString;
    PPH_STRING relativeTimeString;
    PPH_STRING itemString;

    PhQuerySystemTime(&absoluteTime);
    absoluteTime.QuadPart -= Duration;
    PhLargeIntegerToLocalSystemTime(&absoluteTimeFields, &absoluteTime);

    relativeTimeString = PhFormatTimeSpanRelative(Duration);
    absoluteTimeString = PhFormatDateTime(&absoluteTimeFields);
    itemString = PhFormatString(L"%s ago (%s)", PhGetString(relativeTimeString), PhGetString(absoluteTimeString));
    PhDereferenceObject(relativeTimeString);
    PhDereferenceObject(absoluteTimeString);

    PhSetSocketListViewItem(Context, Index, PhGetString(itemString));
    PhDereferenceObject(itemString);
}

HPROPSHEETPAGE PhCreateAfdSocketPage(
    _In_ HANDLE ProcessId,
    _In_ HANDLE HandleValue
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PPHP_AFD_SOCKET_PAGE_CONTEXT socketPageContext;

    socketPageContext = PhAllocateZero(sizeof(PHP_AFD_SOCKET_PAGE_CONTEXT));
    socketPageContext->ProcessId = ProcessId;
    socketPageContext->HandleValue = HandleValue;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_DEFAULT;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OBJAFDSOCKET);
    propSheetPage.hInstance = NtCurrentImageBase();
    propSheetPage.pfnDlgProc = PhpAfdSocketPageProc;
    propSheetPage.lParam = (LPARAM)socketPageContext;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);

    return propSheetPageHandle;
}

static VOID PhpRefreshAfdSocketPageInfo(
    _In_ PPHP_AFD_SOCKET_PAGE_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE socketHandle = NULL;
    PPH_STRING itemString;
    SOCK_SHARED_INFO sharedInfo;
    AFD_INFORMATION simpleInfo;
    ULONG optionValue;
    TCP_INFO_v2 tcpInfo;
    ULONG tcpInfoVersion;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        Context->ProcessId
        )))
    {
        status = NtDuplicateObject(
            processHandle,
            Context->HandleValue,
            NtCurrentProcess(),
            &socketHandle,
            0,
            0,
            DUPLICATE_SAME_ACCESS
            );
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
        return;

    //
    // Shared Winsock context
    //

    if (NT_SUCCESS(PhAfdQuerySharedInfo(socketHandle, &sharedInfo)))
    {
        // State
        itemString = PhAfdFormatSocketState(sharedInfo.State);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_STATE, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Type
        itemString = PhAfdFormatSocketType(sharedInfo.SocketType);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_TYPE, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Address family
        itemString = PhAfdFormatAddressFamily(sharedInfo.AddressFamily);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_ADDRESS_FAMILY, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Protocol
        itemString = PhAfdFormatProtocol(sharedInfo.AddressFamily, sharedInfo.Protocol);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_PROTOCOL, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Catalog entry ID
        PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_CATALOG_ENTRY_ID, sharedInfo.CatalogEntryId);

        // Provider ID
        itemString = PhFormatGuid(&sharedInfo.ProviderId);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_PROVIDER_ID, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Provider flags
        itemString = PhAfdFormatProviderFlags(sharedInfo.ProviderFlags);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_PROVIDER_FLAGS, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Service flags
        itemString = PhAfdFormatServiceFlags(sharedInfo.ServiceFlags1);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_SERVICE_FLAGS, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Send timeout
        switch (sharedInfo.SendTimeout)
        {
        case 0:
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_SEND_TIMEOUT, L"Unlimited");
            break;
        default:
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_SEND_TIMEOUT, PH_TICKS_PER_MS * sharedInfo.SendTimeout);
        }

        // Receive timeout
        switch (sharedInfo.ReceiveTimeout)
        {
        case 0:
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_RECEIVE_TIMEOUT, L"Unlimited");
            break;
        default:
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_RECEIVE_TIMEOUT, PH_TICKS_PER_MS * sharedInfo.ReceiveTimeout);
        }

        // Send buffer size
        PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_SEND_BUFFER_SIZE, sharedInfo.SendBufferSize);

        // Receive buffer size
        PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_RECEIVE_BUFFER_SIZE, sharedInfo.ReceiveBufferSize);

        // Creation flags
        itemString = PhAfdFormatCreationFlags(sharedInfo.CreationFlags);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_CREATION_FLAGS, PhGetString(itemString));
        PhDereferenceObject(itemString);

        // Flags
        itemString = PhAfdFormatSharedInfoFlags(&sharedInfo);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_FLAGS, PhGetString(itemString));
        PhDereferenceObject(itemString);
    }

    //
    // Addresses
    //

    // Address
    if (NT_SUCCESS(PhAfdQueryFormatAddress(socketHandle, FALSE, &itemString, 0)))
    {
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_ADDRESS, PhGetString(itemString));
        PhDereferenceObject(itemString);
    }

    // Remote address
    if (NT_SUCCESS(PhAfdQueryFormatAddress(socketHandle, TRUE, &itemString, 0)))
    {
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_REMOTE_ADDRESS, PhGetString(itemString));
        PhDereferenceObject(itemString);
    }

    //
    // AFD info classes
    //

    // Connect time
    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_CONNECT_TIME, &simpleInfo)))
    {
        switch (simpleInfo.Information.Ulong)
        {
        case ULONG_MAX:
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_CONNECT_TIME, L"N/A (not connected)");
            break;
        default:
            PhSetSocketListViewItemTimeAgo(Context, PH_AFD_SOCKET_ITEM_CONNECT_TIME, PH_TICKS_PER_SEC * simpleInfo.Information.Ulong);
        }
    }

    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_DELIVERY_STATUS, &simpleInfo)))
    {
        // Delivery available
        PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_DELIVERY_AVAILABLE, simpleInfo.Information.DeliveryStatus.DeliveryAvailable);

        // Pendeding recveives
        PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_PENDED_RECEIVE_REQUESTS, simpleInfo.Information.DeliveryStatus.PendedReceiveRequests);
    }

    // Pending sends
    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_SENDS_PENDING, &simpleInfo)))
    {
        PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_SENDS_PENDING, simpleInfo.Information.Ulong);
    }

    // Receive window size
    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_RECEIVE_WINDOW_SIZE, &simpleInfo)))
    {
        PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_RECEIVE_WINDOW_SIZE, simpleInfo.Information.Ulong);
    }

    // Send window size
    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_SEND_WINDOW_SIZE, &simpleInfo)))
    {
        PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_SEND_WINDOW_SIZE, simpleInfo.Information.Ulong);
    }

    // Maximum send size
    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_MAX_SEND_SIZE, &simpleInfo)))
    {
        PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_MAX_SEND_SIZE, simpleInfo.Information.Ulong);
    }

    if (NT_SUCCESS(PhAfdQuerySimpleInfo(socketHandle, AFD_GROUP_ID_AND_TYPE, &simpleInfo)))
    {
        // Group ID
        PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_GROUP_ID, simpleInfo.Information.GroupInfo.GroupID);

        // Group type
        itemString = PhAfdFormatGroupType(simpleInfo.Information.GroupInfo.GroupType);
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_GROUP_TYPE, PhGetString(itemString));
        PhDereferenceObject(itemString);
    }

    //
    // TDI devices
    //

    // TDI address device
    if (NT_SUCCESS(PhAfdQueryFormatTdiDeviceName(socketHandle, AFD_QUERY_ADDRESS_HANDLE, &itemString)))
    {
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_TDI_ADDRESS_DEVICE, PhGetString(itemString));
        PhDereferenceObject(itemString);
    }

    // TDI connection device
    if (NT_SUCCESS(PhAfdQueryFormatTdiDeviceName(socketHandle, AFD_QUERY_CONNECTION_HANDLE, &itemString)))
    {
        PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_TDI_CONNECTION_DEVICE, PhGetString(itemString));
        PhDereferenceObject(itemString);
    }

    // HACK: hvsocket.sys has a bug that makes connected Hyper-V sockets return
    // STATUS_SUCCESS for all option-querying request. We detect it by issuing a
    // deliberately invalid query. If it succeeds, we know we've hit the bug and
    // cannot display any meaningful option information about the socket. (diversenok)

    if (!NT_SUCCESS(PhAfdQueryOption(socketHandle, 0xDEAD, 0xDEAD, &optionValue)))
    {
        //
        // Socket-level options
        //

        // Reuse address
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_REUSEADDR, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_REUSEADDR, optionValue);
        }

        // Keep alive
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_KEEPALIVE, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_KEEPALIVE, optionValue);
        }

        // Don't route
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_DONTROUTE, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_DONTROUTE, optionValue);
        }

        // Broadcast
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_BROADCAST, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_BROADCAST, optionValue);
        }

        // OOB in line
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_OOBINLINE, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_OOBINLINE, optionValue);
        }

        // Receive buffer size
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_RCVBUF, &optionValue)))
        {
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_SO_RCVBUF, optionValue);
        }

        // Maximum message size
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_MAX_MSG_SIZE, &optionValue)))
        {
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_SO_MAX_MSG_SIZE, optionValue);
        }

        // Conditional accept
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_CONDITIONAL_ACCEPT, optionValue);
        }

        // Pause accept
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_PAUSE_ACCEPT, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_PAUSE_ACCEPT, optionValue);
        }

        // Compartment ID
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_COMPARTMENT_ID, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_SO_COMPARTMENT_ID, optionValue);
        }

        // Randomize port
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_RANDOMIZE_PORT, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_RANDOMIZE_PORT, optionValue);
        }

        // Port scalability
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_PORT_SCALABILITY, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_PORT_SCALABILITY, optionValue);
        }

        // Reuse unicast port
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_REUSE_UNICASTPORT, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_REUSE_UNICASTPORT, optionValue);
        }

        // Exclusive address use
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_SO_EXCLUSIVEADDRUSE, optionValue);
        }

        //
        // IP-level options
        //

        // Header included
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_HDRINCL, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_HDRINCL, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_HDRINCL, optionValue);
        }

        // Type-of-service
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_TOS, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_IP_TOS, optionValue);
        }

        // Unicast TTL
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_TTL, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_IP_TTL, optionValue);
        }

        // Multicast interface
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_MULTICAST_IF, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_MULTICAST_IF, &optionValue)))
        {
            itemString = PhAfdFormatInterfaceOption(optionValue);
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_IP_MULTICAST_IF, PhGetString(itemString));
            PhDereferenceObject(itemString);
        }

        // Multicast TTL
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_MULTICAST_TTL, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_IP_MULTICAST_TTL, optionValue);
        }

        // Multicast loopback
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_MULTICAST_LOOP, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_MULTICAST_LOOP, optionValue);
        }

        // Don't fragment
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_DONTFRAGMENT, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_DONTFRAG, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_DONTFRAGMENT, optionValue);
        }

        // Receive packet info
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_PKTINFO, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_PKTINFO, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_PKTINFO, optionValue);
        }

        // Receive TTL
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVTTL, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IP_HOPLIMIT, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVTTL, optionValue);
        }

        // Broadcast reception
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECEIVE_BROADCAST, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECEIVE_BROADCAST, optionValue);
        }

        // IPv6 protection level
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, &optionValue)))
        {
            itemString = PhAfdFormatProtectionLevel(optionValue);
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_IP_PROTECTION_LEVEL, PhGetString(itemString));
            PhDereferenceObject(itemString);
        }

        // Receive arrival interface
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVIF, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_RECVIF, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVIF, optionValue);
        }

        // Receive destination address
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVDSTADDR, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_RECVDSTADDR, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVDSTADDR, optionValue);
        }

        // IPv6-only
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_V6ONLY, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_V6ONLY, optionValue);
        }

        // Interface list
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_IFLIST, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_IFLIST, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_IFLIST, optionValue);
        }

        // Unicast interface
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_UNICAST_IF, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_UNICAST_IF, &optionValue)))
        {
            itemString = PhAfdFormatInterfaceOption(optionValue);
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_IP_UNICAST_IF, PhGetString(itemString));
            PhDereferenceObject(itemString);
        }

        // Receive routing header
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVRTHDR, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_RECVRTHDR, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVRTHDR, optionValue);
        }

        // Receive type-of-service
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVTCLASS, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_RECVTCLASS, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVTOS, optionValue);
        }

        // Original arrival interface
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_ORIGINAL_ARRIVAL_IF, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_ORIGINAL_ARRIVAL_IF, optionValue);
        }

        // Receive ECN
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVECN, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_RECVECN, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVECN, optionValue);
        }

        // Receive extended packet info
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_PKTINFO_EX, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_PKTINFO_EX, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_PKTINFO_EX, optionValue);
        }

        // WFP redirect records
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_WFP_REDIRECT_RECORDS, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_WFP_REDIRECT_RECORDS, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_WFP_REDIRECT_RECORDS, optionValue);
        }

        // WFP redirect context
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_WFP_REDIRECT_CONTEXT, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_WFP_REDIRECT_CONTEXT, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_WFP_REDIRECT_CONTEXT, optionValue);
        }

        // MTU discovery
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_MTU_DISCOVER, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &optionValue)))
        {
            itemString = PhAfdFormatMtuDiscoveryMode(optionValue);
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_IP_MTU_DISCOVER, PhGetString(itemString));
            PhDereferenceObject(itemString);
        }

        // Path MTU
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_MTU, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_MTU, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_IP_MTU, optionValue);
        }

        // Receive ICMP errors
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_RECVERR, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_RECVERR, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_IP_RECVERR, optionValue);
        }

        // Upper MTU bound
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IP, IP_USER_MTU, &optionValue)) ||
            NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_IPV6, IPV6_USER_MTU, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_IP_USER_MTU, optionValue);
        }

        //
        // TCP-level options
        //

        // No delay
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_NODELAY, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_NODELAY, optionValue);
        }

        // Expedited data
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_EXPEDITED_1122, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_EXPEDITED, optionValue);
        }

        // Keep alive
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_KEEPALIVE, &optionValue)))
        {
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_KEEPALIVE, PH_TICKS_PER_SEC * optionValue);
        }

        // Maximum segment size
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_MAXSEG, &optionValue)))
        {
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_MAXSEG, optionValue);
        }

        // Retry timeout
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_MAXRT, &optionValue)))
        {
            switch (optionValue)
            {
            case ULONG_MAX:
                PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_TCP_MAXRT, L"Unlimited");
                break;
            default:
                PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_MAXRT, PH_TICKS_PER_SEC * optionValue);
            }
        }

        // URG interpretation
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_STDURG, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_STDURG, optionValue);
        }

        // No URG
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_NOURG, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_NOURG, optionValue);
        }

        // At mark
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_ATMARK, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_ATMARK, optionValue);
        }

        // No SYN retries
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_NOSYNRETRIES, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_NOSYNRETRIES, optionValue);
        }

        // Timestamps
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_TIMESTAMPS, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_TIMESTAMPS, optionValue);
        }

        // Congestion algorithm
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_CONGESTION_ALGORITHM, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_CONGESTION_ALGORITHM, optionValue);
        }

        // Delay FIN ACK
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_DELAY_FIN_ACK, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_DELAY_FIN_ACK, optionValue);
        }

        // Retry timeout (precise)
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_MAXRTMS, &optionValue)))
        {
            switch (optionValue)
            {
            case ULONG_MAX:
                PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_TCP_MAXRTMS, L"Unlimited");
                break;
            default:
                PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_MAXRTMS, PH_TICKS_PER_MS * optionValue);
            }
        }

        // Fast open
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_FASTOPEN, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_FASTOPEN, optionValue);
        }

        // Keep alive count
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_KEEPCNT, &optionValue)))
        {
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_KEEPCNT, optionValue);
        }

        // Keep alive interval
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_KEEPINTVL, &optionValue)))
        {
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_KEEPINTVL, PH_TICKS_PER_SEC * optionValue);
        }

        // Fail on ICMP error
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_TCP, TCP_FAIL_CONNECT_ON_ICMP_ERROR, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_FAIL_CONNECT_ON_ICMP_ERROR, optionValue);
        }

        //
        // TCP information
        //

        if (NT_SUCCESS(PhAfdQueryTcpInfo(socketHandle, &tcpInfo, &tcpInfoVersion)))
        {
            // TCP state
            itemString = PhAfdFormatTcpState(tcpInfo.State);
            PhSetSocketListViewItem(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_STATE, PhGetString(itemString));
            PhDereferenceObject(itemString);

            // Maximum segment size
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_MSS, tcpInfo.Mss);

            // Connection time
            PhSetSocketListViewItemTimeAgo(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_CONNECTION_TIME, PH_TICKS_PER_MS * tcpInfo.ConnectionTimeMs);

            // Timestamps enabled
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_TIMESTAMPS_ENABLED, tcpInfo.TimestampsEnabled);

            // Estimated round-trip
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_RTT, RTL_TICKS_PER_MICROSEC * tcpInfo.RttUs);

            // Minimal round-trip
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_MINRTT, RTL_TICKS_PER_MICROSEC * tcpInfo.MinRttUs);

            // Bytes in flight
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_IN_FLIGHT, tcpInfo.BytesInFlight);

            // Congestion window
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_WINDOW, tcpInfo.Cwnd);

            // Send window
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_SEND_WINDOW, tcpInfo.SndWnd);

            // Receive window
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVE_WINDOW, tcpInfo.RcvWnd);

            // Receive buffer
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVE_BUFFER, tcpInfo.RcvBuf);

            // Bytes sent
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_OUT, tcpInfo.BytesOut);

            // Bytes received
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_IN, tcpInfo.BytesIn);

            // Bytes reordered
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_REORDERED, tcpInfo.BytesReordered);

            // Bytes retransmitted
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_RETRANSMITTED, tcpInfo.BytesRetrans);

            // Fast retransmits
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_FAST_RETRANSMIT, tcpInfo.FastRetrans);

            // Duplicate ACKs
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_DUPLICATE_ACKS_IN, tcpInfo.DupAcksIn);

            // Timeout episodes
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_TIMEOUT_EPISODES, tcpInfo.TimeoutEpisodes);

            // SYN retransmits
            PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_SYN_RETRANSMITS, tcpInfo.SynRetrans);

            if (tcpInfoVersion >= 1)
            {
                // Receiver-limited episodes
                PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_TRANSITIONS, tcpInfo.SndLimTransRwin);

                // Receiver-limited time
                PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_TIME, PH_TICKS_PER_MS * tcpInfo.SndLimTimeRwin);

                // Receiver-limited bytes
                PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_BYTES, tcpInfo.SndLimBytesRwin);

                // Congestion-limited episodes
                PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_TRANSITIONS, tcpInfo.SndLimTransCwnd);

                // Congestion-limited time
                PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_TIME, PH_TICKS_PER_MS * tcpInfo.SndLimTimeCwnd);

                // Congestion-limited bytes
                PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_BYTES, tcpInfo.SndLimBytesCwnd);

                // Sender-limited episodes
                PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_TRANSITIONS, tcpInfo.SndLimTransSnd);

                // Sender-limited time
                PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_TIME, PH_TICKS_PER_MS * tcpInfo.SndLimTimeSnd);

                // Sender-limited bytes
                PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_BYTES, tcpInfo.SndLimBytesSnd);
            }

            if (tcpInfoVersion >= 2)
            {
                // Out-of-order packets
                PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_OUT_OF_ORDER_PACKETS, tcpInfo.OutOfOrderPktsIn);

                // ECN negotiated
                PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_ECN_NEGOTIATED, tcpInfo.EcnNegotiated);

                // ECE ACKs
                PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_ECE_ACKS_IN, tcpInfo.EceAcksIn);

                // Probe timeout episodes
                PhSetSocketListViewItemDecimal(Context, PH_AFD_SOCKET_ITEM_TCP_INFO_PTO_EPISODES, tcpInfo.PtoEpisodes);
            }
        }

        //
        // UDP-level options
        //

        // No checksum
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_UDP, UDP_NOCHECKSUM, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_UDP_NOCHECKSUM, optionValue);
        }

        // Maximum message size
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_UDP, UDP_SEND_MSG_SIZE, &optionValue)))
        {
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_UDP_SEND_MSG_SIZE, optionValue);
        }

        // Maximum coalesced size
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, IPPROTO_UDP, UDP_RECV_MAX_COALESCED_SIZE, &optionValue)))
        {
            PhSetSocketListViewItemBytes(Context, PH_AFD_SOCKET_ITEM_UDP_RECV_MAX_COALESCED_SIZE, optionValue);
        }

        //
        // Hyper-V-level options
        //

        // Connect timeout
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, HV_PROTOCOL_RAW, HVSOCKET_CONNECT_TIMEOUT, &optionValue)))
        {
            PhSetSocketListViewItemTimeSpan(Context, PH_AFD_SOCKET_ITEM_HVSOCKET_CONNECT_TIMEOUT, PH_TICKS_PER_MS * optionValue);
        }

        // Container passthru
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, HV_PROTOCOL_RAW, HVSOCKET_CONTAINER_PASSTHRU, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_HVSOCKET_CONTAINER_PASSTHRU, optionValue);
        }

        // Connected suspend
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, HV_PROTOCOL_RAW, HVSOCKET_CONNECTED_SUSPEND, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_HVSOCKET_CONNECTED_SUSPEND, optionValue);
        }

        // High VTL
        if (NT_SUCCESS(PhAfdQueryOption(socketHandle, HV_PROTOCOL_RAW, HVSOCKET_HIGH_VTL, &optionValue)))
        {
            PhSetSocketListViewItemBoolean(Context, PH_AFD_SOCKET_ITEM_HVSOCKET_HIGH_VTL, optionValue);
        }
    }

    NtClose(socketHandle);
}

INT_PTR CALLBACK PhpAfdSocketPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_AFD_SOCKET_PAGE_CONTEXT context;

    context = PhpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 4);

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->ListViewContext = PhListView_Initialize(context->ListViewHandle);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);
            PhListView_AddColumn(context->ListViewContext, 0, 0, 0, LVCFMT_LEFT, 145, L"Name");
            PhListView_AddColumn(context->ListViewContext, 1, 1, 1, LVCFMT_LEFT, 225, L"Value");
            PhListView_EnableGroupView(context->ListViewContext, TRUE);

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_SHARED, L"Shared Winsock context");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_STATE, L"Socket state");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_TYPE, L"Socket type");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_ADDRESS_FAMILY, L"Address family");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_PROTOCOL, L"Protocol");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_CATALOG_ENTRY_ID, L"Catalog entry ID");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_PROVIDER_ID, L"Provider ID");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_PROVIDER_FLAGS, L"Provider flags");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_SERVICE_FLAGS, L"Service flags");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_SEND_TIMEOUT, L"Send timeout");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_RECEIVE_TIMEOUT, L"Receive timeout");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_SEND_BUFFER_SIZE, L"Send buffer size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_RECEIVE_BUFFER_SIZE, L"Receive buffer size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_CREATION_FLAGS, L"Creation flags");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SHARED, PH_AFD_SOCKET_ITEM_FLAGS, L"Flags");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_ADDRESSES, L"Addresses");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_ADDRESSES, PH_AFD_SOCKET_ITEM_ADDRESS, L"Local address");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_ADDRESSES, PH_AFD_SOCKET_ITEM_REMOTE_ADDRESS, L"Remote address");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_INFOCLASS, L"AFD info classes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_CONNECT_TIME, L"Connect time");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_DELIVERY_AVAILABLE, L"Delivery available");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_PENDED_RECEIVE_REQUESTS, L"Pending receive requests");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_SENDS_PENDING, L"Pending sends");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_RECEIVE_WINDOW_SIZE, L"Receive window size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_SEND_WINDOW_SIZE, L"Send window size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_MAX_SEND_SIZE, L"Maximum send size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_GROUP_ID, L"Group ID");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_INFOCLASS, PH_AFD_SOCKET_ITEM_GROUP_TYPE, L"Group type");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_TDI, L"TDI devices");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TDI, PH_AFD_SOCKET_ITEM_TDI_ADDRESS_DEVICE, L"TDI address device");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TDI, PH_AFD_SOCKET_ITEM_TDI_CONNECTION_DEVICE, L"TDI connection device");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_SO, L"Socket-level options");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_REUSEADDR, L"Reuse address");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_KEEPALIVE, L"Keep alive");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_DONTROUTE, L"Don't route");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_BROADCAST, L"Broadcast");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_OOBINLINE, L"OOB in line");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_RCVBUF, L"Receive buffer size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_MAX_MSG_SIZE, L"Maximum message size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_PAUSE_ACCEPT, L"Pause accept");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_COMPARTMENT_ID, L"Compartment ID");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_RANDOMIZE_PORT, L"Randomize port");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_PORT_SCALABILITY, L"Port scalability");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_REUSE_UNICASTPORT, L"Reuse unicast port");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_SO, PH_AFD_SOCKET_ITEM_SO_EXCLUSIVEADDRUSE, L"Exclusive address use");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_IP, L"IP-level options");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_HDRINCL, L"Header included");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_TOS, L"Type-of-service");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_TTL, L"Unicast TTL");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_MULTICAST_IF, L"Multicast interface");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_MULTICAST_TTL, L"Multicast TTL");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_MULTICAST_LOOP, L"Multicast loopback");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_DONTFRAGMENT, L"Don't fragment");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_PKTINFO, L"Receive packet info");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVTTL, L"Receive TTL");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECEIVE_BROADCAST, L"Broadcast reception");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_PROTECTION_LEVEL, L"IPv6 protection level");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVIF, L"Receive arrival interface");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVDSTADDR, L"Receive dest. address");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_V6ONLY, L"IPv6-only");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_IFLIST, L"Interface list");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_UNICAST_IF, L"Unicast interface");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVRTHDR, L"Receive routing header");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVTOS, L"Receive type-of-service");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_ORIGINAL_ARRIVAL_IF, L"Original arrival interface");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVECN, L"Receive ECN");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_PKTINFO_EX, L"Recveive ext. packet info");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_WFP_REDIRECT_RECORDS, L"WFP redirect records");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_WFP_REDIRECT_CONTEXT, L"WFP redirect context");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_MTU_DISCOVER, L"MTU discovery");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_MTU, L"Path MTU");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_RECVERR, L"Receive ICMP errors");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_IP, PH_AFD_SOCKET_ITEM_IP_USER_MTU, L"Upper MTU bound");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_TCP, L"TCP-level options");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_NODELAY, L"No delay");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_EXPEDITED, L"Expedited data");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_KEEPALIVE, L"Keep alive");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_MAXSEG, L"Maximum segment size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_MAXRT, L"Retry timeout");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_STDURG, L"URG interpretation");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_NOURG, L"No URG");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_ATMARK, L"At mark");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_NOSYNRETRIES, L"No SYN retries");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_TIMESTAMPS, L"Timestamps");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_CONGESTION_ALGORITHM, L"Congestion algorithm");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_DELAY_FIN_ACK, L"Delay FIN ACK");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_MAXRTMS, L"Retry timeout (precise)");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_FASTOPEN, L"Fast open");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_KEEPCNT, L"Keep alive count");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_KEEPINTVL, L"Keep alive interval");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP, PH_AFD_SOCKET_ITEM_TCP_FAIL_CONNECT_ON_ICMP_ERROR, L"Fail on ICMP error");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_TCP_INFO, L"TCP information");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_STATE, L"TCP state");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_MSS, L"Maximum segment size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_CONNECTION_TIME, L"Connection time");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_TIMESTAMPS_ENABLED, L"Timestamps enabled");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_RTT, L"Estimated round-trip");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_MINRTT, L"Minimal round-trip");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_IN_FLIGHT, L"Bytes in flight");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_WINDOW, L"Congestion window");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_SEND_WINDOW, L"Send window");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVE_WINDOW, L"Receive window");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVE_BUFFER, L"Receive buffer");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_OUT, L"Bytes sent");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_IN, L"Bytes received");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_REORDERED, L"Bytes reordered");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_BYTES_RETRANSMITTED, L"Bytes retransmitted");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_FAST_RETRANSMIT, L"Fast retransmits");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_DUPLICATE_ACKS_IN, L"Duplicate ACKs");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_TIMEOUT_EPISODES, L"Timeout episodes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_SYN_RETRANSMITS, L"SYN retransmits");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_TRANSITIONS, L"Receiver-limited episodes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_TIME, L"Receiver-limited time");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_RECEIVER_LIMITED_BYTES, L"Receiver-limited bytes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_TRANSITIONS, L"Congestion-limited episodes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_TIME, L"Congestion-limited time");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_CONGESTION_LIMITED_BYTES, L"Congestion-limited bytes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_TRANSITIONS, L"Sender-limited episodes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_TIME, L"Sender-limited time");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_SENDER_LIMITED_BYTES, L"Sender-limited bytes");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_OUT_OF_ORDER_PACKETS, L"Out-of-order packets");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_ECN_NEGOTIATED, L"ECN negotiated");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_ECE_ACKS_IN, L"ECE ACKs");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_TCP_INFO, PH_AFD_SOCKET_ITEM_TCP_INFO_PTO_EPISODES, L"Probe timeout episodes");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_UDP, L"UDP-level options");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_UDP, PH_AFD_SOCKET_ITEM_UDP_NOCHECKSUM, L"No checksum");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_UDP, PH_AFD_SOCKET_ITEM_UDP_SEND_MSG_SIZE, L"Maximum message size");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_UDP, PH_AFD_SOCKET_ITEM_UDP_RECV_MAX_COALESCED_SIZE, L"Maximum coalesced size");

            PhListView_AddGroup(context->ListViewContext, PH_AFD_SOCKET_GROUP_HVSOCKET, L"Hyper-V-level options");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_HVSOCKET, PH_AFD_SOCKET_ITEM_HVSOCKET_CONNECT_TIMEOUT, L"Connect timeout");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_HVSOCKET, PH_AFD_SOCKET_ITEM_HVSOCKET_CONTAINER_PASSTHRU, L"Container passthru");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_HVSOCKET, PH_AFD_SOCKET_ITEM_HVSOCKET_CONNECTED_SUSPEND, L"Connected suspend");
            PhAddSocketListViewItem(context, PH_AFD_SOCKET_GROUP_HVSOCKET, PH_AFD_SOCKET_ITEM_HVSOCKET_HIGH_VTL, L"High VTL");

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PhpRefreshAfdSocketPageInfo(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhListView_Destroy(context->ListViewContext);

            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

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
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

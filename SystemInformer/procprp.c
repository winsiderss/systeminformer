/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2016-2023
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>

#include <kphuser.h>
#include <settings.h>

#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>

PPH_OBJECT_TYPE PhpProcessPropContextType = NULL;
PPH_OBJECT_TYPE PhpProcessPropPageContextType = NULL;
PPH_OBJECT_TYPE PhpProcessPropPageWaitContextType = NULL;
PH_STRINGREF PhProcessPropPageLoadingText = PH_STRINGREF_INIT(L"Loading...");
static RECT MinimumSize = { -1, -1, -1, -1 };
SLIST_HEADER WaitContextQueryListHead;

PPH_PROCESS_PROPCONTEXT PhCreateProcessPropContext(
    _In_opt_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_PROCESS_PROPCONTEXT propContext;
    PROPSHEETHEADER propSheetHeader;

    if (PhBeginInitOnce(&initOnce))
    {
        PhpProcessPropContextType = PhCreateObjectType(L"ProcessPropContext", 0, PhpProcessPropContextDeleteProcedure);
        PhpProcessPropPageContextType = PhCreateObjectType(L"ProcessPropPageContext", 0, PhpProcessPropPageContextDeleteProcedure);
        PhpProcessPropPageWaitContextType = PhCreateObjectType(L"ProcessPropPageWaitContext", 0, PhpProcessPropPageWaitContextDeleteProcedure);
        RtlInitializeSListHead(&WaitContextQueryListHead);
        PhEndInitOnce(&initOnce);
    }

    propContext = PhCreateObjectZero(sizeof(PH_PROCESS_PROPCONTEXT), PhpProcessPropContextType);
    propContext->PropSheetPages = PhAllocateZero(sizeof(HPROPSHEETPAGE) * PH_PROCESS_PROPCONTEXT_MAXPAGES);

    if (!PH_IS_FAKE_PROCESS_ID(ProcessItem->ProcessId))
    {
        PH_FORMAT format[4];

        PhInitFormatSR(&format[0], ProcessItem->ProcessName->sr);
        PhInitFormatS(&format[1], L" (");
        PhInitFormatU(&format[2], HandleToUlong(ProcessItem->ProcessId));
        PhInitFormatC(&format[3], L')');

        propContext->Title = PhFormat(format, RTL_NUMBER_OF(format), 64);
    }
    else
    {
        PhSetReference(&propContext->Title, ProcessItem->ProcessName);
    }

    memset(&propSheetHeader, 0, sizeof(PROPSHEETHEADER));
    propSheetHeader.dwSize = sizeof(PROPSHEETHEADER);
    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK |
        PSH_USEHICON;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.hIcon = PhGetImageListIcon(ProcessItem->SmallIconIndex, FALSE);
    propSheetHeader.pszCaption = PhGetString(propContext->Title);
    propSheetHeader.pfnCallback = PhpPropSheetProc;

    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    if (PhCsForceNoParent)
        propSheetHeader.hwndParent = NULL;

    memcpy(&propContext->PropSheetHeader, &propSheetHeader, sizeof(PROPSHEETHEADER));

    PhSetReference(&propContext->ProcessItem, ProcessItem);

    return propContext;
}

VOID NTAPI PhpProcessPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_PROPCONTEXT propContext = (PPH_PROCESS_PROPCONTEXT)Object;

    PhFree(propContext->PropSheetPages);
    PhDereferenceObject(propContext->Title);
    PhDereferenceObject(propContext->ProcessItem);
}

VOID PhRefreshProcessPropContext(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext
    )
{
    PropContext->PropSheetHeader.hIcon = PhGetImageListIcon(PropContext->ProcessItem->SmallIconIndex, FALSE);
}

VOID PhSetSelectThreadIdProcessPropContext(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HANDLE ThreadId
    )
{
    PropContext->SelectThreadId = ThreadId;
}

INT CALLBACK PhpPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    )
{
#define PROPSHEET_ADD_STYLE (WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

    switch (uMsg)
    {
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                if (((DLGTEMPLATEEX *)lParam)->signature == USHRT_MAX)
                {
                    ((DLGTEMPLATEEX *)lParam)->style |= PROPSHEET_ADD_STYLE;
                }
                else
                {
                    ((DLGTEMPLATE *)lParam)->style |= PROPSHEET_ADD_STYLE;
                }
            }
        }
        break;
    case PSCB_INITIALIZED:
        {
            PPH_PROCESS_PROPSHEETCONTEXT propSheetContext;

            propSheetContext = PhAllocate(sizeof(PH_PROCESS_PROPSHEETCONTEXT));
            memset(propSheetContext, 0, sizeof(PH_PROCESS_PROPSHEETCONTEXT));

            PhInitializeLayoutManager(&propSheetContext->LayoutManager, hwndDlg);
            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, propSheetContext);

            propSheetContext->PropSheetWindowHookProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, 0xF, propSheetContext);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PhpPropSheetWndProc);

            if (PhEnableThemeSupport) // NOTE: Required for compatibility. (dmex)
                PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 290;
                rect.bottom = 320;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            SetTimer(hwndDlg, 2000, 2000, NULL);
        }
        break;
    }

    return 0;
}

PPH_PROCESS_PROPSHEETCONTEXT PhpGetPropSheetContext(
    _In_ HWND hwnd
    )
{
    return PhGetWindowContext(hwnd, PH_WINDOW_CONTEXT_DEFAULT);
}

LRESULT CALLBACK PhpPropSheetWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_PROPSHEETCONTEXT propSheetContext;
    WNDPROC oldWndProc;

    propSheetContext = PhGetWindowContext(hwnd, 0xF);

    if (!propSheetContext)
        return 0;

    oldWndProc = propSheetContext->PropSheetWindowHookProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            HWND tabControl;
            TCITEM tabItem;
            WCHAR text[128] = L"";

            // Save the window position and size.

            PhSaveWindowPlacementToSetting(L"ProcPropPosition", L"ProcPropSize", hwnd);

            // Save the selected tab.

            tabControl = PropSheet_GetTabControl(hwnd);

            tabItem.mask = TCIF_TEXT;
            tabItem.pszText = text;
            tabItem.cchTextMax = RTL_NUMBER_OF(text) - 1;

            if (TabCtrl_GetItem(tabControl, TabCtrl_GetCurSel(tabControl), &tabItem))
            {
                PhSetStringSetting(L"ProcPropPage", text);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhUnregisterWindowCallback(hwnd);

            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwnd, 0xF);

            PhDeleteLayoutManager(&propSheetContext->LayoutManager);
            PhFree(propSheetContext);
        }
        break;
    case WM_SYSCOMMAND:
        {
            // Note: Clicking the X on the taskbar window thumbnail preview doesn't close modeless property sheets
            // when there are more than 1 window and the window doesn't have focus... The MFC, ATL and WTL libraries
            // check if the propsheet is modeless and SendMessage WM_CLOSE and so we'll implement the same solution. (dmex)
            switch (wParam & 0xFFF0)
            {
            case SC_CLOSE:
                {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    //SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                    //return TRUE;
                }
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                // Prevent the OK button from working (even though
                // it's already hidden). This prevents the Enter
                // key from closing the dialog box.
                return 0;
            }
        }
        break;
    case WM_SIZE:
        {
            if (!IsMinimized(hwnd))
            {
                PhLayoutManagerLayout(&propSheetContext->LayoutManager);
            }
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_KEYDOWN: // forward key messages (dmex)
    //case WM_KEYUP:
        {
            HWND pageWindowHandle;

            if (pageWindowHandle = PropSheet_GetCurrentPageHwnd(hwnd))
            {
                // TODO: Add hotkey plugin support using hashlist register/callback for window handle. (dmex)
                if (SendMessage(pageWindowHandle, uMsg, wParam, lParam))
                {
                    return TRUE;
                }
            }
        }
        break;
    case WM_TIMER:
        {
            UINT id = (UINT)wParam;

            if (id == 2000)
            {
                PhpFlushProcessPropSheetWaitContextData();
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOLEAN PhpInitializePropSheetLayoutStage1(
    _In_ PPH_PROCESS_PROPSHEETCONTEXT Context,
    _In_ HWND hwnd
    )
{
    if (!Context->LayoutInitialized)
    {
        HWND tabControlHandle;
        PPH_LAYOUT_ITEM tabControlItem;
        PPH_LAYOUT_ITEM tabPageItem;

        tabControlHandle = PropSheet_GetTabControl(hwnd);
        tabControlItem = PhAddLayoutItem(&Context->LayoutManager, tabControlHandle,
            NULL, PH_ANCHOR_ALL | PH_LAYOUT_IMMEDIATE_RESIZE);
        tabPageItem = PhAddLayoutItem(&Context->LayoutManager, tabControlHandle,
            NULL, PH_LAYOUT_TAB_CONTROL); // dummy item to fix multiline tab control

        Context->TabPageItem = tabPageItem;

        PhAddLayoutItem(&Context->LayoutManager, GetDlgItem(hwnd, IDCANCEL),
            NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

        // Hide the OK button.
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        PhSetDialogItemText(hwnd, IDCANCEL, L"Close");

        Context->LayoutInitialized = TRUE;

        return TRUE;
    }

    return FALSE;
}

VOID PhpInitializePropSheetLayoutStage2(
    _In_ HWND hwnd
    )
{
    PH_RECTANGLE windowRectangle = {0};
    RECT rect;
    LONG dpiValue;

    windowRectangle.Position = PhGetIntegerPairSetting(L"ProcPropPosition");

    rect = PhRectangleToRect(windowRectangle);
    dpiValue = PhGetMonitorDpi(&rect);

    windowRectangle.Size = PhGetScalableIntegerPairSetting(L"ProcPropSize", TRUE, dpiValue).Pair;

    if (windowRectangle.Size.X < MinimumSize.right)
        windowRectangle.Size.X = MinimumSize.right;
    if (windowRectangle.Size.Y < MinimumSize.bottom)
        windowRectangle.Size.Y = MinimumSize.bottom;

    PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

    MoveWindow(hwnd, windowRectangle.Left, windowRectangle.Top,
        windowRectangle.Width, windowRectangle.Height, FALSE);

    // Implement cascading by saving an offsetted rectangle.
    windowRectangle.Left += 20;
    windowRectangle.Top += 20;

    PhSetIntegerPairSetting(L"ProcPropPosition", windowRectangle.Position);
}

BOOLEAN PhAddProcessPropPage(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    HPROPSHEETPAGE propSheetPageHandle;

    if (PropContext->PropSheetHeader.nPages == PH_PROCESS_PROPCONTEXT_MAXPAGES)
        return FALSE;

    propSheetPageHandle = CreatePropertySheetPage(
        &PropPageContext->PropSheetPage
        );
    // CreatePropertySheetPage would have sent PSPCB_ADDREF,
    // which would have added a reference.
    PhDereferenceObject(PropPageContext);

    PhSetReference(&PropPageContext->PropContext, PropContext);

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] = propSheetPageHandle;
    PropContext->PropSheetHeader.nPages++;

    return TRUE;
}

BOOLEAN PhAddProcessPropPage2(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HPROPSHEETPAGE PropSheetPageHandle
    )
{
    if (PropContext->PropSheetHeader.nPages == PH_PROCESS_PROPCONTEXT_MAXPAGES)
        return FALSE;

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] = PropSheetPageHandle;
    PropContext->PropSheetHeader.nPages++;

    return TRUE;
}

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    return PhCreateProcessPropPageContextEx(PhInstanceHandle, Template, DlgProc, Context);
}

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    propPageContext = PhCreateObjectZero(sizeof(PH_PROCESS_PROPPAGECONTEXT), PhpProcessPropPageContextType);
    propPageContext->PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propPageContext->PropSheetPage.dwFlags = PSP_USECALLBACK;
    propPageContext->PropSheetPage.hInstance = InstanceHandle;
    propPageContext->PropSheetPage.pszTemplate = Template;
    propPageContext->PropSheetPage.pfnDlgProc = DlgProc;
    propPageContext->PropSheetPage.lParam = (LPARAM)propPageContext;
    propPageContext->PropSheetPage.pfnCallback = PhpStandardPropPageProc;

    propPageContext->Context = Context;

    return propPageContext;
}

VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)Object;

    if (propPageContext->PropContext)
        PhDereferenceObject(propPageContext->PropContext);
}

INT CALLBACK PhpStandardPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        PhReferenceObject(propPageContext);
    else if (uMsg == PSPCB_RELEASE)
        PhDereferenceObject(propPageContext);

    return 1;
}

VOID NTAPI PhpProcessPropPageWaitContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_WAITPROPCONTEXT context = (PPH_PROCESS_WAITPROPCONTEXT)Object;

    if (context->ProcessWaitHandle)
        RtlDeregisterWaitEx(context->ProcessWaitHandle, RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION);
    if (context->ProcessHandle)
        NtClose(context->ProcessHandle);
    if (context->ProcessItem)
        PhDereferenceObject(context->ProcessItem);
}

VOID PhpCreateProcessPropSheetWaitContext(
    _In_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HWND WindowHandle
    )
{
    PPH_PROCESS_ITEM processItem = PropContext->ProcessItem;
    PPH_PROCESS_WAITPROPCONTEXT waitContext;
    HANDLE processHandle;

    if (!processItem->QueryHandle)
        return;
    if (processItem->ProcessId == NtCurrentProcessId())
        return;
    // On Windows 8.1 and above, processes without threads are reflected processes
    // which will not terminate if we have a handle open. (wj32)
    if (processItem->NumberOfThreads == 0)
        return;

    if (!NT_SUCCESS(PhOpenProcess(
        &processHandle,
        SYNCHRONIZE,
        processItem->ProcessId
        )))
    {
        return;
    }

    waitContext = PhCreateObjectZero(sizeof(PH_PROCESS_WAITPROPCONTEXT), PhpProcessPropPageWaitContextType);
    waitContext->ProcessItem = PhReferenceObject(processItem);
    waitContext->PropSheetWindowHandle = GetParent(WindowHandle);
    waitContext->ProcessHandle = processHandle;

    if (NT_SUCCESS(RtlRegisterWait(
        &waitContext->ProcessWaitHandle,
        processHandle,
        PhpProcessPropertiesWaitCallback,
        waitContext,
        INFINITE,
        WT_EXECUTEONLYONCE | WT_EXECUTEINIOTHREAD
        )))
    {
        PropContext->ProcessWaitContext = waitContext;
    }
    else
    {
        PhDereferenceObject(waitContext->ProcessItem);
        PhDereferenceObject(waitContext);
        NtClose(processHandle);
    }
}

VOID PhpFlushProcessPropSheetWaitContextData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PPH_PROCESS_WAITPROPCONTEXT data;
    PROCESS_BASIC_INFORMATION basicInfo;

    //if (!RtlQueryDepthSList(&WaitContextQueryListHead))
    //    return;
    //if (!RtlFirstEntrySList(&WaitContextQueryListHead))
    //    return;

    entry = RtlInterlockedFlushSList(&WaitContextQueryListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, PH_PROCESS_WAITPROPCONTEXT, ListEntry);
        entry = entry->Next;

        if (NT_SUCCESS(PhGetProcessBasicInformation(data->ProcessItem->QueryHandle, &basicInfo)))
        {
            PPH_STRING statusMessage = NULL;
            PPH_STRING errorMessage;
            PH_FORMAT format[5];

            PhInitFormatSR(&format[0], data->ProcessItem->ProcessName->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatU(&format[2], HandleToUlong(data->ProcessItem->ProcessId));

            if (basicInfo.ExitStatus < STATUS_WAIT_1 || basicInfo.ExitStatus > STATUS_WAIT_63)
            {
                if (errorMessage = PhGetStatusMessage(basicInfo.ExitStatus, 0))
                {
                    PhInitFormatS(&format[3], L") exited with ");
                    PhInitFormatSR(&format[4], errorMessage->sr);

                    statusMessage = PhFormat(format, RTL_NUMBER_OF(format), 0);
                    PhDereferenceObject(errorMessage);
                }
            }

            if (PhIsNullOrEmptyString(statusMessage))
            {
                PhInitFormatS(&format[3], L") exited with 0x");
                PhInitFormatX(&format[4], basicInfo.ExitStatus);
                //format[4].Type |= FormatPadZeros; format[4].Width = 8;
                statusMessage = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }

            if (statusMessage)
            {
                PhSetWindowText(data->PropSheetWindowHandle, PhGetString(statusMessage));
                PhDereferenceObject(statusMessage);
            }
        }

        //PostMessage(data->PropSheetWindowHandle, WM_PH_PROPPAGE_EXITSTATUS, 0, (LPARAM)data);
    }
}

VOID CALLBACK PhpProcessPropertiesWaitCallback(
    _In_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    PPH_PROCESS_WAITPROPCONTEXT waitContext = Context;

    // This avoids blocking the workqueue and avoids converting the workqueue to GUI threads. (dmex)
    RtlInterlockedPushEntrySList(&WaitContextQueryListHead, &waitContext->ListEntry);
}

_Success_(return)
BOOLEAN PhPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_opt_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_opt_ PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    _Out_opt_ PPH_PROCESS_ITEM *ProcessItem
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    if (uMsg == WM_INITDIALOG)
    {
        PPH_PROCESS_PROPCONTEXT propContext;

        propSheetPage = (LPPROPSHEETPAGE)lParam;
        propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam;
        propContext = propPageContext->PropContext;

        if (!propContext->WaitInitialized)
        {
            PhpCreateProcessPropSheetWaitContext(propContext, hwndDlg);
            propContext->WaitInitialized = TRUE;
        }

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, propSheetPage);
    }
    else
    {
        propSheetPage = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!propSheetPage)
        return FALSE;

    propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam;

    if (PropSheetPage)
        *PropSheetPage = propSheetPage;
    if (PropPageContext)
        *PropPageContext = propPageContext;
    if (ProcessItem)
        *ProcessItem = propPageContext->PropContext->ProcessItem;

    if (uMsg == WM_DESTROY)
    {
        PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (propPageContext->PropContext->ProcessWaitContext)
        {
            PhDereferenceObject(propPageContext->PropContext->ProcessWaitContext);
            propPageContext->PropContext->ProcessWaitContext = NULL;
        }
    }

    return TRUE;
}

PPH_LAYOUT_ITEM PhAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    HWND parent;
    PPH_PROCESS_PROPSHEETCONTEXT propSheetContext;
    PPH_LAYOUT_MANAGER layoutManager;
    PPH_LAYOUT_ITEM realParentItem;
    BOOLEAN doLayoutStage2;
    PPH_LAYOUT_ITEM item;

    parent = GetParent(hwnd);
    propSheetContext = PhpGetPropSheetContext(parent);
    layoutManager = &propSheetContext->LayoutManager;

    doLayoutStage2 = PhpInitializePropSheetLayoutStage1(propSheetContext, parent);

    if (ParentItem != PH_PROP_PAGE_TAB_CONTROL_PARENT)
        realParentItem = ParentItem;
    else
        realParentItem = propSheetContext->TabPageItem;

    // Use the HACK if the control is a direct child of the dialog.
    if (ParentItem && ParentItem != PH_PROP_PAGE_TAB_CONTROL_PARENT &&
        // We detect if ParentItem is the layout item for the dialog
        // by looking at its parent.
        (ParentItem->ParentItem == &layoutManager->RootItem ||
        (ParentItem->ParentItem->Anchor & PH_LAYOUT_TAB_CONTROL)))
    {
        RECT dialogRect;
        RECT dialogSize;
        RECT margin;

        // MAKE SURE THESE NUMBERS ARE CORRECT.
        dialogSize.right = 260;
        dialogSize.bottom = 260;
        MapDialogRect(hwnd, &dialogSize);

        // Get the original dialog rectangle.
        GetWindowRect(hwnd, &dialogRect);
        dialogRect.right = dialogRect.left + dialogSize.right;
        dialogRect.bottom = dialogRect.top + dialogSize.bottom;

        // Calculate the margin from the original rectangle.
        GetWindowRect(Handle, &margin);
        margin = PhMapRect(margin, dialogRect);
        PhConvertRect(&margin, &dialogRect);

        item = PhAddLayoutItemEx(layoutManager, Handle, realParentItem, Anchor, margin);
    }
    else
    {
        item = PhAddLayoutItem(layoutManager, Handle, realParentItem, Anchor);
    }

    if (doLayoutStage2)
        PhpInitializePropSheetLayoutStage2(parent);

    return item;
}

VOID PhDoPropPageLayout(
    _In_ HWND hwnd
    )
{
    HWND parent;
    PPH_PROCESS_PROPSHEETCONTEXT propSheetContext;

    parent = GetParent(hwnd);
    propSheetContext = PhpGetPropSheetContext(parent);
    PhLayoutManagerLayout(&propSheetContext->LayoutManager);
}

NTSTATUS PhpProcessPropertiesThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_PROCESS_PROPCONTEXT PropContext = (PPH_PROCESS_PROPCONTEXT)Parameter;
    PPH_PROCESS_PROPPAGECONTEXT newPage;
    PPH_STRING startPage;

    PhInitializeAutoPool(&autoPool);

    // Wait for stage 1 to be processed.
    PhWaitForEvent(&PropContext->ProcessItem->Stage1Event, NULL);
    // Refresh the icon which may have been updated due to
    // stage 1.
    PhRefreshProcessPropContext(PropContext);

    // Add the pages...

    // General
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCGENERAL),
        PhpProcessGeneralDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Statistics
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCSTATISTICS),
        PhpProcessStatisticsDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Performance
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCPERFORMANCE),
        PhpProcessPerformanceDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Threads
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCTHREADS),
        PhpProcessThreadsDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Token
    PhAddProcessPropPage2(
        PropContext,
        PhCreateTokenPage(PhpOpenProcessTokenForPage, PropContext->ProcessItem->ProcessId, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessTokenHookProc)
        );

    // Modules
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCMODULES),
        PhpProcessModulesDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Memory
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCMEMORY),
        PhpProcessMemoryDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Environment
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCENVIRONMENT),
        PhpProcessEnvironmentDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Handles
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCHANDLES),
        PhpProcessHandlesDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);

    // Job
    if (
        PropContext->ProcessItem->IsInJob &&
        // There's no way the job page can function without KPH since it needs
        // to open a handle to the job.
        (KphLevel() >= KphLevelMed)
        )
    {
        PhAddProcessPropPage2(
            PropContext,
            PhCreateJobPage(PhpOpenProcessJobForPage, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessJobHookProc)
            );
    }

    // Services
    if (PropContext->ProcessItem->ServiceList && PropContext->ProcessItem->ServiceList->Count != 0)
    {
        newPage = PhCreateProcessPropPageContext(
            MAKEINTRESOURCE(IDD_PROCSERVICES),
            PhpProcessServicesDlgProc,
            NULL
            );
        PhAddProcessPropPage(PropContext, newPage);
    }

    // WMI Provider Host
    // Note: The Winmgmt service includes WMI providers but the process doesn't get tagged with WmiProviderHostType
    // and the perf cost adding service detection is too high for just this one usage case. (dmex)
    //if ((PropContext->ProcessItem->KnownProcessType & KnownProcessTypeMask) == WmiProviderHostType)
    {
        newPage = PhCreateProcessPropPageContext(
            MAKEINTRESOURCE(IDD_PROCWMIPROVIDERS),
            PhpProcessWmiProvidersDlgProc,
            NULL
            );
        PhAddProcessPropPage(PropContext, newPage);
    }

#ifdef _M_IX86
    if ((PropContext->ProcessItem->KnownProcessType & KnownProcessTypeMask) == NtVdmHostProcessType)
    {
        newPage = PhCreateProcessPropPageContext(
            MAKEINTRESOURCE(IDD_PROCVDMHOST),
            PhpProcessVdmHostProcessDlgProc,
            NULL
            );
        PhAddProcessPropPage(PropContext, newPage);
    }
#endif

    // Plugin-supplied pages
    if (PhPluginsEnabled)
    {
        PH_PLUGIN_PROCESS_PROPCONTEXT pluginProcessPropContext;

        pluginProcessPropContext.PropContext = PropContext;
        pluginProcessPropContext.ProcessItem = PropContext->ProcessItem;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing), &pluginProcessPropContext);
    }

    // Create the property sheet

    if (PropContext->SelectThreadId)
        PhSetStringSetting(L"ProcPropPage", L"Threads");

    startPage = PhGetStringSetting(L"ProcPropPage");
    PropContext->PropSheetHeader.dwFlags |= PSH_USEPSTARTPAGE;
    PropContext->PropSheetHeader.pStartPage = startPage->Buffer;

    PhModalPropertySheet(&PropContext->PropSheetHeader);

    PhDereferenceObject(startPage);
    PhDereferenceObject(PropContext);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID PhShowProcessProperties(
    _In_ PPH_PROCESS_PROPCONTEXT Context
    )
{
    PhReferenceObject(Context);
    PhCreateThread2(PhpProcessPropertiesThreadStart, Context);
}

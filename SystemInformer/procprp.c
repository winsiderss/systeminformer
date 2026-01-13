/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2016-2026
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>
#include <proctree.h>

#include <mapldr.h>
#include <kphuser.h>
#include <settings.h>
#include <secedit.h>
#include <emenu.h>

#include <actions.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>
#include <mainwnd.h>
#include <mainwndp.h>

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
        PhInitializeSListHead(&WaitContextQueryListHead);
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

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

            propSheetContext->PropSheetWindowHookProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, 0xF, propSheetContext);
            PhSetWindowProcedure(hwndDlg, PhpPropSheetWndProc);

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

            PhSetTimer(hwndDlg, 2000, 2000, NULL);
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

            PhKillTimer(hwnd, 2000);

            // Save the window position and size.

            PhSaveWindowPlacementToSetting(SETTING_PROC_PROP_POSITION, SETTING_PROC_PROP_SIZE, hwnd);

            // Save the selected tab.

            tabControl = PropSheet_GetTabControl(hwnd);

            tabItem.mask = TCIF_TEXT;
            tabItem.pszText = text;
            tabItem.cchTextMax = RTL_NUMBER_OF(text) - 1;

            if (TabCtrl_GetItem(tabControl, TabCtrl_GetCurSel(tabControl), &tabItem))
            {
                PhSetStringSetting(SETTING_PROC_PROP_PAGE, text);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhUnregisterWindowCallback(hwnd);

            PhSetWindowProcedure(hwnd, oldWndProc);
            PhRemoveWindowContext(hwnd, 0xF);

            PhDeleteLayoutManager(&propSheetContext->LayoutManager);
            PhRemoveWindowContext(hwnd, PH_WINDOW_CONTEXT_DEFAULT);

            PhFree(propSheetContext);
        }
        break;
    case WM_SYSCOMMAND:
        {
            // Note: Clicking the X on the taskbar window thumbnail preview doesn't close modeless property sheets
            // when there are more than 1 window and the window doesn't have focus. (dmex)
            switch (wParam & 0xFFF0)
            {
            case SC_CLOSE:
                {
                    PostQuitMessage(0);
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
    case WM_TIMER:
        {
            UINT id = (UINT)wParam;

            if (id == 2000)
            {
                PhpFlushProcessPropSheetWaitContextData();
            }
        }
        break;
    case PSM_ISDIALOGMESSAGE:
        {
            PMSG dialog = (PMSG)lParam;

            if (dialog->message == WM_KEYDOWN)
            {
                switch (dialog->wParam)
                {
                case VK_F5:
                    SystemInformer_Refresh();
                    break;
                case VK_F6:
                case VK_PAUSE:
                    SystemInformer_SetUpdateAutomatically(!SystemInformer_GetUpdateAutomatically());
                    break;
                }
            }
        }
        break;
    case WM_DPICHANGED:
        {
            USHORT newDpi = HIWORD(wParam);
            PRECT CONST newRect = (PRECT)lParam;

            CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);

            PhLayoutManagerUpdate(&propSheetContext->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&propSheetContext->LayoutManager);

            {
                SetWindowPos(
                    hwnd,
                    NULL,
                    newRect->left,
                    newRect->top,
                    newRect->right - newRect->left,
                    newRect->bottom - newRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                    );
            }

            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 290;
                rect.bottom = 320;
                MapDialogRect(hwnd, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            return 0;
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

_Function_class_(PH_OPEN_OBJECT)
NTSTATUS PhOptionsButtonGeneralOpenProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    if (Context)
    {
        return PhOpenProcess(Handle, DesiredAccess, (HANDLE)Context);
    }

    return STATUS_UNSUCCESSFUL;
}

_Function_class_(PH_CLOSE_OBJECT)
NTSTATUS PhOptionsButtonGeneralCloseHandle(
    _In_opt_ HANDLE Handle,
    _In_opt_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    if (Handle)
    {
        NtClose(Handle);
    }

    return STATUS_SUCCESS;
}

LRESULT CALLBACK PhpOptionsButtonWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_PROPSHEETCONTEXT propSheetContext;
    WNDPROC oldWndProc;

    if (!(propSheetContext = PhGetWindowContext(WindowHandle, SCHAR_MAX)))
        return DefWindowProc(WindowHandle, uMsg, wParam, lParam);

    oldWndProc = propSheetContext->OldOptionsButtonWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);
        }
        break;
    case WM_COMMAND:
        {
            if (GET_WM_COMMAND_HWND(wParam, lParam) == propSheetContext->OptionsButtonWindowHandle)
            {
                RECT rect;
                PPH_EMENU menu;
                PPH_EMENU_ITEM selectedItem;
                PPH_EMENU_ITEM menuItem;
                HWND pageWindow;
                LPPROPSHEETPAGE propSheetPage;
                PPH_PROCESS_PROPPAGECONTEXT propPageContext;
                PPH_PROCESS_PROPCONTEXT propContext;

                if (!(pageWindow = PropSheet_GetCurrentPageHwnd(WindowHandle)))
                    break;
                if (!(propSheetPage = PhGetWindowContext(pageWindow, PH_WINDOW_CONTEXT_DEFAULT)))
                    break;
                if (!(propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam))
                    break;
                if (!(propContext = propPageContext->PropContext))
                    break;
                if (!PhGetWindowRect(propSheetContext->OptionsButtonWindowHandle, &rect))
                    break;

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

                if (propContext->ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
                {
                    PPH_EMENU_ITEM kernelMinimal;

                    menuItem = PhCreateEMenuItem(0, ID_PROCESS_CREATEDUMPFILE, L"Create live kernel dump fi&le", NULL, NULL);
                    PhInsertEMenuItem(menuItem, kernelMinimal = PhCreateEMenuItem(0, ID_PROCESS_DUMP_MINIMAL, L"&Minimal...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_NORMAL, L"&Normal...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_FULL, L"&Full...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_CUSTOM, L"&Custom...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

                    if (!PhGetOwnTokenAttributes().Elevated)
                    {
                        menuItem->Flags |= PH_EMENU_DISABLED;
                    }
                    else if (WindowsVersion < WINDOWS_11)
                    {
                        // Minimal only captures thread stacks, not supported before Windows 11
                        PhSetDisabledEMenuItem(kernelMinimal);
                    }
                }
                else
                {
                    menuItem = PhCreateEMenuItem(0, ID_PROCESS_CREATEDUMPFILE, L"Create dump fi&le", NULL, NULL);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_MINIMAL, L"&Minimal...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_LIMITED, L"&Limited...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_NORMAL, L"&Normal...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_FULL, L"&Full...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_DUMP_CUSTOM, L"&Custom...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
                }

                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_DEBUG, L"De&bug", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_AFFINITY, L"&Affinity", NULL, NULL), ULONG_MAX);

                menuItem = PhCreateEMenuItem(0, ID_PROCESS_PRIORITYCLASS, L"&Priority", NULL, NULL);
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
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_ACTIVITY, L"Activity moderation", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_SETCRITICAL, L"&Critical", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_DETACHFROMDEBUGGER, L"&Detach from debugger", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_ECOMODE, L"Efficiency mode", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_EXECUTIONREQUIRED, L"Execution required", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_GDIHANDLES, L"GDI &handles...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_HEAPS, L"Heaps...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_LOCKS, L"Locks...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_FLUSHHEAPS, L"Flush heaps", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_PAGESMODIFIED, L"Modified pages...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_REDUCEWORKINGSET, L"Reduce working &set", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_RUNAS, L"&Run as...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_MISCELLANEOUS_RUNASTHISUSER, L"Run &as this user...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PROCESS_VIRTUALIZATION, L"Virtuali&zation", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_SEARCHONLINE, L"Search &online\bCtrl+M", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_OPENFILELOCATION, L"Open &file location\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_SECURITY, L"Security", NULL, NULL), ULONG_MAX);

                PhMwpInitializeProcessMenu(menu, &propContext->ProcessItem, 1);

                selectedItem = PhShowEMenu(
                    menu,
                    WindowHandle,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
                    rect.left,
                    rect.top
                    );

                if (selectedItem && selectedItem->Id)
                {
                    PPH_PROCESS_ITEM processItem = propContext->ProcessItem;
                    
                    PhReferenceObject(processItem);
                    
                    switch (selectedItem->Id)
                    {
                    case ID_PROCESS_TERMINATE:
                        PhUiTerminateProcesses(WindowHandle, &processItem, 1);
                        break;
                    case ID_PROCESS_TERMINATETREE:
                        PhUiTerminateTreeProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_SUSPEND:
                        PhUiSuspendProcesses(WindowHandle, &processItem, 1);
                        break;
                    case ID_PROCESS_SUSPENDTREE:
                        PhUiSuspendTreeProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_RESUME:
                        PhUiResumeProcesses(WindowHandle, &processItem, 1);
                        break;
                    case ID_PROCESS_RESUMETREE:
                        PhUiResumeTreeProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_FREEZE:
                        PhUiFreezeTreeProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_THAW:
                        PhUiThawTreeProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_RESTART:
                        PhUiRestartProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_AFFINITY:
                        PhShowProcessAffinityDialog(WindowHandle, processItem, NULL);
                        break;
                    case ID_PRIORITY_REALTIME:
                    case ID_PRIORITY_HIGH:
                    case ID_PRIORITY_ABOVENORMAL:
                    case ID_PRIORITY_NORMAL:
                    case ID_PRIORITY_BELOWNORMAL:
                    case ID_PRIORITY_IDLE:
                        PhMwpExecuteProcessPriorityClassCommand(WindowHandle, selectedItem->Id, &processItem, 1);
                        break;
                    case ID_IOPRIORITY_VERYLOW:
                    case ID_IOPRIORITY_LOW:
                    case ID_IOPRIORITY_NORMAL:
                    case ID_IOPRIORITY_HIGH:
                        PhMwpExecuteProcessIoPriorityCommand(WindowHandle, selectedItem->Id, &processItem, 1);
                        break;
                    case ID_PAGEPRIORITY_VERYLOW:
                    case ID_PAGEPRIORITY_LOW:
                    case ID_PAGEPRIORITY_MEDIUM:
                    case ID_PAGEPRIORITY_BELOWNORMAL:
                    case ID_PAGEPRIORITY_NORMAL:
                        {
                            ULONG pagePriority;

                            switch (selectedItem->Id)
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

                            PhUiSetPagePriorityProcess(WindowHandle, processItem, pagePriority);
                        }
                        break;
                    case ID_MISCELLANEOUS_ACTIVITY:
                        PhUiSetActivityModeration(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_SETCRITICAL:
                        PhUiSetCriticalProcess(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_DETACHFROMDEBUGGER:
                        PhUiDetachFromDebuggerProcess(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_ECOMODE:
                        PhUiSetEcoModeProcess(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_EXECUTIONREQUIRED:
                        PhUiSetExecutionRequiredProcess(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_GDIHANDLES:
                        PhShowGdiHandlesDialog(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_HEAPS:
                        PhShowProcessHeapsDialog(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_LOCKS:
                        PhShowProcessLocksDialog(WindowHandle, processItem);
                        break;
                    case ID_MISCELLANEOUS_REDUCEWORKINGSET:
                        PhUiReduceWorkingSetProcesses(WindowHandle, &processItem, 1);
                        break;
                    case ID_MISCELLANEOUS_RUNAS:
                        PhShowRunAsDialog(WindowHandle, NULL);
                        break;
                    case ID_MISCELLANEOUS_RUNASTHISUSER:
                        PhShowRunAsDialog(WindowHandle, processItem->ProcessId);
                        break;
                    case ID_MISCELLANEOUS_FLUSHHEAPS:
                        PhUiFlushHeapProcesses(WindowHandle, &processItem, 1);
                        break;
                    case ID_PROCESS_DUMP_MINIMAL:
                    case ID_PROCESS_DUMP_LIMITED:
                    case ID_PROCESS_DUMP_NORMAL:
                    case ID_PROCESS_DUMP_FULL:
                    case ID_PROCESS_DUMP_CUSTOM:
                        PhUiCreateDumpFileProcess(WindowHandle, processItem, selectedItem->Id);
                        break;
                    case ID_PROCESS_DEBUG:
                        PhUiDebugProcess(WindowHandle, processItem);
                        break;
                    case ID_PROCESS_SEARCHONLINE:
                        PhSearchOnlineString(WindowHandle, processItem->ProcessName->Buffer);
                        break;
                    case ID_PROCESS_OPENFILELOCATION:
                        {
                            NTSTATUS status;
                            PPH_STRING fileName;

                            status = PhGetProcessItemFileNameWin32(processItem, &fileName);

                            if (NT_SUCCESS(status))
                            {
                                PhShellExecuteUserString(
                                    WindowHandle,
                                    SETTING_FILE_BROWSE_EXECUTABLE,
                                    PhGetString(fileName),
                                    FALSE,
                                    L"Make sure the Explorer executable file is present."
                                    );

                                PhDereferenceObject(fileName);
                            }
                            else
                            {
                                PhShowStatus(WindowHandle, L"Unable to locate the file.", status, 0);
                            }
                        }
                        break;
                    case ID_HANDLE_SECURITY:
                        {
                            PhEditSecurity(
                                PhCsForceNoParent ? NULL : pageWindow,
                                PhGetStringOrEmpty(propContext->ProcessItem->ProcessName),
                                L"Process",
                                PhOptionsButtonGeneralOpenProcess,
                                PhOptionsButtonGeneralCloseHandle,
                                propContext->ProcessItem->ProcessId
                                );
                        }
                        break;
                    }
                    
                    PhDereferenceObject(processItem);
                }

                PhDestroyEMenu(menu);
            }
            //else if (GET_WM_COMMAND_HWND(wParam, lParam) == propSheetContext->PermissionsButtonWindowHandle)
            //{
            //    HWND pageWindow;
            //    LPPROPSHEETPAGE propSheetPage;
            //    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
            //    PPH_PROCESS_PROPCONTEXT propContext;
            //
            //    if (!(pageWindow = PropSheet_GetCurrentPageHwnd(WindowHandle)))
            //        break;
            //    if (!(propSheetPage = PhGetWindowContext(pageWindow, PH_WINDOW_CONTEXT_DEFAULT)))
            //        break;
            //    if (!(propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam))
            //        break;
            //    if (!(propContext = propPageContext->PropContext))
            //        break;
            //
            //    NOTHING;
            //}
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            if (propSheetContext->ButtonsLabelWindowHandle)
            {
                static CONST PH_STRINGREF text = PH_STRINGREF_INIT(L"Protection");
                static CONST PH_STRINGREF seperator = PH_STRINGREF_INIT(L": ");
                static CONST PH_STRINGREF natext = PH_STRINGREF_INIT(L"N/A");
                HWND pageWindow;
                LPPROPSHEETPAGE propSheetPage;
                PPH_PROCESS_PROPPAGECONTEXT propPageContext;
                PPH_PROCESS_PROPCONTEXT propContext;

                if (!(pageWindow = PropSheet_GetCurrentPageHwnd(WindowHandle)))
                    break;
                if (!(propSheetPage = PhGetWindowContext(pageWindow, PH_WINDOW_CONTEXT_DEFAULT)))
                    break;
                if (!(propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam))
                    break;
                if (!(propContext = propPageContext->PropContext))
                    break;

                PPH_STRING string = PhGetProcessProtectionString(propContext->ProcessItem->Protection, (BOOLEAN)propContext->ProcessItem->IsSecureProcess);

                if (string)
                {
                    PhMoveReference(&string, PhConcatStringRef3(&text, &seperator, &string->sr));
                }
                else
                {
                    PhMoveReference(&string, PhConcatStringRef3(&text, &seperator, &natext));
                }

                PhSetWindowText(propSheetContext->ButtonsLabelWindowHandle, string->Buffer);
                PhClearReference(&string);
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID PhpCreateProcessPropButtons(
    _In_ PPH_PROCESS_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND PropSheetWindow
    )
{
    if (!PropSheetContext->OptionsButtonWindowHandle)
    {
        RECT clientRect;
        RECT rect;
        LONG buttonWidth;
        LONG buttonHeight;
        LONG buttonSpacing = 6;
        LONG labelWidth = 250;
        HFONT windowFont;

        windowFont = GetWindowFont(GetDlgItem(PropSheetWindow, IDCANCEL));

        PropSheetContext->OldOptionsButtonWndProc = PhGetWindowProcedure(PropSheetWindow);
        PhSetWindowContext(PropSheetWindow, SCHAR_MAX, PropSheetContext);
        PhSetWindowProcedure(PropSheetWindow, PhpOptionsButtonWndProc);

        PhGetClientRect(PropSheetWindow, &clientRect);
        PhGetWindowRect(GetDlgItem(PropSheetWindow, IDCANCEL), &rect);
        MapWindowRect(NULL, PropSheetWindow, &rect);

        buttonWidth = rect.right - rect.left;
        buttonHeight = rect.bottom - rect.top;

        // Create the Options button
        PropSheetContext->OptionsButtonWindowHandle = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            WC_BUTTON,
            L"Options",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            clientRect.right - rect.right,
            rect.top,
            buttonWidth,
            buttonHeight,
            PropSheetWindow,
            NULL,
            PhInstanceHandle,
            NULL
            );
        SetWindowFont(PropSheetContext->OptionsButtonWindowHandle, windowFont, TRUE);

        // Create the Permissions button
        //PropSheetContext->PermissionsButtonWindowHandle = CreateWindowEx(
        //    WS_EX_NOPARENTNOTIFY,
        //    WC_BUTTON,
        //    L"Permissions",
        //    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        //    clientRect.right - rect.right + buttonWidth + buttonSpacing,
        //    rect.top,
        //    buttonWidth,
        //    buttonHeight,
        //    PropSheetWindow,
        //    NULL,
        //    PhInstanceHandle,
        //    NULL
        //    );
        //SetWindowFont(PropSheetContext->PermissionsButtonWindowHandle, windowFont, TRUE);

        // Create the label
        PropSheetContext->ButtonsLabelWindowHandle = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            WC_STATIC,
            L"Protection",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            clientRect.right - rect.right + (buttonWidth + buttonSpacing) + 5,
            rect.top + 7,
            labelWidth,
            buttonHeight,
            PropSheetWindow,
            NULL,
            PhInstanceHandle,
            NULL
            );
        SetWindowFont(PropSheetContext->ButtonsLabelWindowHandle, windowFont, TRUE);

        PostMessage(PropSheetWindow, WM_PH_UPDATE_DIALOG, 0, 0);
    }
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
        tabControlItem = PhAddLayoutItem(&Context->LayoutManager, tabControlHandle, NULL, PH_ANCHOR_ALL | PH_LAYOUT_IMMEDIATE_RESIZE);
        tabPageItem = PhAddLayoutItem(&Context->LayoutManager, tabControlHandle, NULL, PH_LAYOUT_TAB_CONTROL); // dummy item to fix multiline tab control
        PhAddLayoutItem(&Context->LayoutManager, GetDlgItem(hwnd, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

        // Create and add the buttons to the layout
        PhpCreateProcessPropButtons(Context, hwnd);
        PhAddLayoutItem(&Context->LayoutManager, Context->OptionsButtonWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
        PhAddLayoutItem(&Context->LayoutManager, Context->ButtonsLabelWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
        //PhAddLayoutItem(&Context->LayoutManager, Context->PermissionsButtonWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

        // Hide the OK button.
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        PhSetDialogItemText(hwnd, IDCANCEL, L"Close");

        Context->TabPageItem = tabPageItem;
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

    PhLoadWindowPlacementFromSetting(SETTING_PROC_PROP_POSITION, SETTING_PROC_PROP_SIZE, hwnd);

    windowRectangle.Position = PhGetIntegerPairSetting(SETTING_PROC_PROP_POSITION);
    PhRectangleToRect(&rect, &windowRectangle);
    dpiValue = PhGetMonitorDpi(NULL, &rect);
    windowRectangle.Size = PhGetScalableIntegerPairSetting(SETTING_PROC_PROP_SIZE, TRUE, dpiValue)->Pair;

    if (windowRectangle.Size.X < MinimumSize.right)
        windowRectangle.Size.X = MinimumSize.right;
    if (windowRectangle.Size.Y < MinimumSize.bottom)
        windowRectangle.Size.Y = MinimumSize.bottom;

    PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

    // Implement cascading by saving an offsetted rectangle.
    windowRectangle.Left += 20;
    windowRectangle.Top += 20;
    PhSetIntegerPairSetting(SETTING_PROC_PROP_POSITION, windowRectangle.Position);
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

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)Object;

    if (propPageContext->PropContext)
        PhDereferenceObject(propPageContext->PropContext);
}

UINT CALLBACK PhpStandardPropPageProc(
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

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpProcessPropPageWaitContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_WAITPROPCONTEXT context = (PPH_PROCESS_WAITPROPCONTEXT)Object;

    PhpFlushProcessPropSheetWaitContextData();

    if (context->ProcessWaitHandle)
    {
        RtlDeregisterWaitEx(context->ProcessWaitHandle, RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION);
        context->ProcessWaitHandle = NULL;
    }

    if (context->ProcessHandle)
    {
        NtClose(context->ProcessHandle);
        context->ProcessHandle = NULL;
    }

    if (context->ProcessItem)
    {
        PhDereferenceObject(context->ProcessItem);
        context->ProcessItem = NULL;
    }
}

VOID PhpCreateProcessPropSheetWaitContext(
    _In_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HWND WindowHandle
    )
{
    PPH_PROCESS_ITEM processItem = PropContext->ProcessItem;
    PPH_PROCESS_WAITPROPCONTEXT waitContext;
    HANDLE processHandle;

    if (!processItem->QueryHandle || processItem->ProcessId == NtCurrentProcessId())
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
        WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD
        )))
    {
        PropContext->ProcessWaitContext = waitContext;
    }
    else
    {
        PhDereferenceObject(waitContext->ProcessItem);
        waitContext->ProcessItem = NULL;

        NtClose(waitContext->ProcessHandle);
        waitContext->ProcessHandle = NULL;

        PhDereferenceObject(waitContext);
    }
}

VOID PhpFlushProcessPropSheetWaitContextData(
    VOID
    )
{
    PSLIST_ENTRY entry = NULL;
    PPH_PROCESS_WAITPROPCONTEXT data;
    PROCESS_BASIC_INFORMATION basicInfo;

    //if (!PhQueryDepthSList(&WaitContextQueryListHead))
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

_Function_class_(WAIT_CALLBACK_ROUTINE)
VOID NTAPI PhpProcessPropertiesWaitCallback(
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

    if (uMsg == WM_NCDESTROY)
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

#ifdef DEBUG
static VOID ASSERT_DIALOGRECT(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ SHORT Width,
    _In_ USHORT Height
    )
{
    PDLGTEMPLATEEX dialogTemplate = NULL;

    PhLoadResource(DllBase, Name, RT_DIALOG, NULL, &dialogTemplate);

    assert(dialogTemplate && dialogTemplate->cx == Width && dialogTemplate->cy == Height);
}
#endif

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

#ifdef DEBUG
        ASSERT_DIALOGRECT(PhInstanceHandle, MAKEINTRESOURCE(IDD_PROCGENERAL), 260, 260);
#endif
        // MAKE SURE THESE NUMBERS ARE CORRECT.
        dialogSize.right = 260;
        dialogSize.bottom = 260;
        MapDialogRect(hwnd, &dialogSize);

        // Get the original dialog rectangle.
        PhGetWindowRect(hwnd, &dialogRect);
        dialogRect.right = dialogRect.left + dialogSize.right;
        dialogRect.bottom = dialogRect.top + dialogSize.bottom;

        // Calculate the margin from the original rectangle.
        PhGetWindowRect(Handle, &margin);
        PhMapRect(&margin, &margin, &dialogRect);
        PhConvertRect(&margin, &dialogRect);

        item = PhAddLayoutItemEx(layoutManager, Handle, realParentItem, Anchor, &margin);
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

_Function_class_(USER_THREAD_START_ROUTINE)
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
        PhCreateTokenPage(PhpOpenProcessTokenForPage, PhpCloseProcessTokenForPage, PropContext->ProcessItem->ProcessId, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessTokenHookProc)
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
        (KsiLevel() >= KphLevelMed)
        )
    {
        PhAddProcessPropPage2(
            PropContext,
            PhCreateJobPage(PhpOpenProcessJobForPage, PhpCloseProcessJobForPage, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessJobHookProc)
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
    // Note: The Winmgmt service has WMI providers but doesn't get tagged with WmiProviderHostType. (dmex)
    if (
        (PropContext->ProcessItem->KnownProcessType & KnownProcessTypeMask) == WmiProviderHostType ||
        (PropContext->ProcessItem->KnownProcessType & KnownProcessTypeMask) == ServiceHostProcessType
        )
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
    {
        PhSetStringSetting(SETTING_PROC_PROP_PAGE, L"Threads");
    }

    startPage = PhGetStringSetting(SETTING_PROC_PROP_PAGE);
    PropContext->PropSheetHeader.dwFlags |= PSH_USEPSTARTPAGE;
    PropContext->PropSheetHeader.pStartPage = PhGetString(startPage);

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

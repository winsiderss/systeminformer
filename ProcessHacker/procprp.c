/*
 * Process Hacker - 
 *   process properties
 * 
 * Copyright (C) 2009-2010 wj32
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

#include <phgui.h>
#include <procprpp.h>
#include <settings.h>

PPH_OBJECT_TYPE PhpProcessPropContextType;
PPH_OBJECT_TYPE PhpProcessPropPageContextType;

BOOLEAN PhProcessPropInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhpProcessPropContextType,
        0,
        PhpProcessPropContextDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhpProcessPropPageContextType,
        0,
        PhpProcessPropPageContextDeleteProcedure
        )))
        return FALSE;

    return TRUE;
}

PPH_PROCESS_PROPCONTEXT PhCreateProcessPropContext(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_PROPCONTEXT propContext;
    PROPSHEETHEADER propSheetHeader;

    if (!NT_SUCCESS(PhCreateObject(
        &propContext,
        sizeof(PH_PROCESS_PROPCONTEXT),
        0,
        PhpProcessPropContextType,
        0
        )))
        return NULL;

    memset(propContext, 0, sizeof(PH_PROCESS_PROPCONTEXT));

    propContext->PropSheetPages =
        PhAllocate(sizeof(HPROPSHEETPAGE) * PH_PROCESS_PROPCONTEXT_MAXPAGES);

    if (
        ProcessItem->ProcessId != DPCS_PROCESS_ID &&
        ProcessItem->ProcessId != INTERRUPTS_PROCESS_ID
        )
    {
        propContext->Title = PhFormatString(
            L"%s (%u)",
            ProcessItem->ProcessName->Buffer,
            (ULONG)ProcessItem->ProcessId
            );
    }
    else
    {
        propContext->Title = ProcessItem->ProcessName;
        PhReferenceObject(propContext->Title);
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
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.hIcon = ProcessItem->SmallIcon;
    propSheetHeader.pszCaption = propContext->Title->Buffer;
    propSheetHeader.pfnCallback = PhpPropSheetProc;

    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    memcpy(&propContext->PropSheetHeader, &propSheetHeader, sizeof(PROPSHEETHEADER));

    propContext->ProcessItem = ProcessItem;
    PhReferenceObject(ProcessItem);
    PhInitializeEvent(&propContext->CreatedEvent);

    return propContext;
}

VOID NTAPI PhpProcessPropContextDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_PROPCONTEXT propContext = (PPH_PROCESS_PROPCONTEXT)Object;

    PhFree(propContext->PropSheetPages);
    PhDereferenceObject(propContext->Title);
    PhDereferenceObject(propContext->ProcessItem);
}

VOID PhRefreshProcessPropContext(
    __inout PPH_PROCESS_PROPCONTEXT PropContext
    )
{
    PropContext->PropSheetHeader.hIcon = PropContext->ProcessItem->SmallIcon;
}

INT CALLBACK PhpPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    )
{
    const LONG addStyle = WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;

    switch (uMsg)
    {
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                if (((DLGTEMPLATEEX *)lParam)->signature == 0xffff)
                {
                    ((DLGTEMPLATEEX *)lParam)->style |= addStyle;
                }
                else
                {
                    ((DLGTEMPLATE *)lParam)->style |= addStyle;
                }
            }
        }
        break;
    case PSCB_INITIALIZED:
        {
            WNDPROC oldWndProc;
            PPH_LAYOUT_MANAGER layoutManager;

            oldWndProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PhpPropSheetWndProc);
            SetProp(hwndDlg, L"OldWndProc", (HANDLE)oldWndProc);

            layoutManager = PhAllocate(sizeof(PH_LAYOUT_MANAGER));
            PhInitializeLayoutManager(layoutManager, hwndDlg);
            SetProp(hwndDlg, L"LayoutManager", (HANDLE)layoutManager);
        }
        break;
    }

    return 0;
}

LRESULT CALLBACK PhpPropSheetWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc = (WNDPROC)GetProp(hwnd, L"OldWndProc");

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            RemoveProp(hwnd, L"OldWndProc");

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwnd, L"LayoutManager");
            PhDeleteLayoutManager(layoutManager);
            PhFree(layoutManager);
            RemoveProp(hwnd, L"LayoutManager");

            {
                WINDOWPLACEMENT placement = { sizeof(placement) };
                PH_RECTANGLE windowRectangle;
                HWND tabControl;
                TCITEM tabItem;
                WCHAR text[32];

                // Save the window position and size.

                GetWindowPlacement(hwnd, &placement);
                windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

                PhSetIntegerPairSetting(L"ProcPropPosition", windowRectangle.Position);
                PhSetIntegerPairSetting(L"ProcPropSize", windowRectangle.Size);

                // Save the selected tab.

                tabControl = PropSheet_GetTabControl(hwnd);

                tabItem.mask = TCIF_TEXT;
                tabItem.pszText = text;
                tabItem.cchTextMax = sizeof(text) / 2 - 1;

                if (TabCtrl_GetItem(tabControl, TabCtrl_GetCurSel(tabControl), &tabItem))
                {
                    PhSetStringSetting(L"ProcPropPage", text);
                }
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwnd, L"LayoutManager");

            // See PhpAddPropPageLayoutItem for an explanation of this hack.
            layoutManager->RootItem.OrigRect = layoutManager->RootItem.Rect;

            PhAddLayoutItem(layoutManager, PropSheet_GetTabControl(hwnd),
                NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwnd, IDCANCEL),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            // Hide the OK button.
            ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
            // Set the Cancel button's text to "Close".
            SetDlgItemText(hwnd, IDCANCEL, L"Close");

            {
                PH_RECTANGLE windowRectangle;

                windowRectangle.Position = PhGetIntegerPairSetting(L"ProcPropPosition");
                windowRectangle.Size = PhGetIntegerPairSetting(L"ProcPropSize");
                PhAdjustRectangleToWorkingArea(hwnd, &windowRectangle);

                MoveWindow(hwnd, windowRectangle.Left, windowRectangle.Top,
                    windowRectangle.Width, windowRectangle.Height, FALSE);

                // Implement cascading by saving an offsetted rectangle.
                windowRectangle.Left += 20;
                windowRectangle.Top += 20;

                PhSetIntegerPairSetting(L"ProcPropPosition", windowRectangle.Position);
                PhSetIntegerPairSetting(L"ProcPropSize", windowRectangle.Size);
            }
        }
        break;
    case WM_SIZE:
        {
            if (!IsIconic(hwnd))
            {
                PhLayoutManagerLayout((PPH_LAYOUT_MANAGER)GetProp(hwnd, L"LayoutManager"));
            }
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 400, 500);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOLEAN PhAddProcessPropPage(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    HPROPSHEETPAGE propSheetPageHandle;

    if (PropContext->PropSheetHeader.nPages == PH_PROCESS_PROPCONTEXT_MAXPAGES)
        return FALSE;

    propSheetPageHandle = CreatePropertySheetPage(
        &PropPageContext->PropSheetPage
        );
    PropPageContext->PropContext = PropContext;
    PhReferenceObject(PropContext);

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] =
        propSheetPageHandle;
    PropContext->PropSheetHeader.nPages++;

    return TRUE;
}

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in PVOID Context
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    if (!NT_SUCCESS(PhCreateObject(
        &propPageContext,
        sizeof(PH_PROCESS_PROPPAGECONTEXT),
        0,
        PhpProcessPropPageContextType,
        0
        )))
        return NULL;

    memset(&propPageContext->PropSheetPage, 0, sizeof(PROPSHEETPAGE));
    propPageContext->PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propPageContext->PropSheetPage.dwFlags =
        PSP_USECALLBACK;
    propPageContext->PropSheetPage.pszTemplate = Template;
    propPageContext->PropSheetPage.pfnDlgProc = DlgProc;
    propPageContext->PropSheetPage.lParam = (LPARAM)propPageContext;
    propPageContext->PropSheetPage.pfnCallback = PhpStandardPropPageProc;

    propPageContext->Context = Context;
    propPageContext->LayoutInitialized = FALSE;

    return propPageContext;
}

VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)Object;

    if (propPageContext->PropContext)
        PhDereferenceObject(propPageContext->PropContext);
}

INT CALLBACK PhpStandardPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
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

BOOLEAN PhpPropPageDlgProcHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam,
    __out LPPROPSHEETPAGE *PropSheetPage,
    __out PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    __out PPH_PROCESS_ITEM *ProcessItem
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    if (uMsg == WM_INITDIALOG)
    {
        // Save the context.
        SetProp(hwndDlg, L"PropPageContext", (HANDLE)lParam);
    }

    propSheetPage = (LPPROPSHEETPAGE)GetProp(hwndDlg, L"PropPageContext");

    if (!propSheetPage)
        return FALSE;

    *PropSheetPage = propSheetPage;
    *PropPageContext = propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam;
    *ProcessItem = propPageContext->PropContext->ProcessItem;

    return TRUE;
}

VOID PhpPropPageDlgProcDestroy(
    __in HWND hwndDlg
    )
{
    RemoveProp(hwndDlg, L"PropPageContext");
}

PPH_LAYOUT_ITEM PhpAddPropPageLayoutItem(
    __in HWND hwnd,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor
    )
{
    HWND parent;
    PPH_LAYOUT_MANAGER layoutManager;

    parent = GetParent(hwnd);
    layoutManager = (PPH_LAYOUT_MANAGER)GetProp(parent, L"LayoutManager");

    // Use the hack if the control is a child of the dialog.
    if (ParentItem && ParentItem->ParentItem == &layoutManager->RootItem)
    {
        RECT dialogSize;
        RECT dialogRect;
        RECT margin;

        GetWindowRect(hwnd, &dialogRect);
        // MAKE SURE THESE NUMBERS ARE CORRECT.
        dialogSize.right = 240;
        dialogSize.bottom = 260;
        MapDialogRect(hwnd, &dialogSize);
        dialogRect.right = dialogRect.left + dialogSize.right;
        dialogRect.bottom = dialogRect.top + dialogSize.bottom;

        GetWindowRect(Handle, &margin);
        margin = PhMapRect(margin, dialogRect);
        PhConvertRect(&margin, &dialogRect);

        return PhAddLayoutItemEx(layoutManager, Handle, ParentItem, Anchor, margin);
    }
    else
    {
        return PhAddLayoutItem(layoutManager, Handle, ParentItem, Anchor);
    }
}

VOID PhpDoPropPageLayout(
    __in HWND hwnd
    )
{
    HWND parent;
    PPH_LAYOUT_MANAGER layoutManager;

    parent = GetParent(hwnd);
    layoutManager = (PPH_LAYOUT_MANAGER)GetProp(parent, L"LayoutManager");
    PhLayoutManagerLayout(layoutManager);
}

NTSTATUS PhpProcessGeneralOpenProcess(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    )
{
    return PhOpenProcess(Handle, DesiredAccess, (HANDLE)Context);
}

PWSTR PhpGetStringOrNa(
    __in PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"N/A";
}

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SendMessage(GetDlgItem(hwndDlg, IDC_PROCGENERAL_ICON), STM_SETICON,
                (WPARAM)processItem->LargeIcon, 0);

            SetDlgItemText(hwndDlg, IDC_PROCGENERAL_NAME,
                PhpGetStringOrNa(processItem->VersionInfo.FileDescription));
            SetDlgItemText(hwndDlg, IDC_PROCGENERAL_VERSION,
                PhpGetStringOrNa(processItem->VersionInfo.FileVersion));
            SetDlgItemText(hwndDlg, IDC_PROCGENERAL_FILENAME,
                PhpGetStringOrNa(processItem->FileName));
            SetDlgItemText(hwndDlg, IDC_PROCGENERAL_CMDLINE,
                PhpGetStringOrNa(processItem->CommandLine));

            if (processItem->VerifySignerName)
            {
                SetDlgItemText(hwndDlg, IDC_PROCGENERAL_COMPANYNAME,
                    PhaConcatStrings2(L"(Verified) ", processItem->VerifySignerName->Buffer)->Buffer);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_PROCGENERAL_COMPANYNAME,
                    PhpGetStringOrNa(processItem->VersionInfo.CompanyName));
            }

            {
                HANDLE processHandle;
                PPH_STRING curdir = NULL;

                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    ProcessQueryAccess | PROCESS_VM_READ,
                    processItem->ProcessId
                    )))
                {
                    PhGetProcessPebString(
                        processHandle,
                        PhpoCurrentDirectory,
                        &curdir
                        );

                    CloseHandle(processHandle);
                }

                SetDlgItemText(hwndDlg, IDC_PROCGENERAL_CURDIR,
                    PhpGetStringOrNa(curdir));

                if (curdir)
                    PhDereferenceObject(curdir);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_FILE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_NAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_COMPANYNAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_VERSION),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_FILENAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_CMDLINE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_CURDIR),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TERMINATE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PERMISSIONS),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCGENERAL_PROCESS),
                    dialogItem, PH_ANCHOR_ALL);

                PhpDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDC_TERMINATE:
                {
                    PhUiTerminateProcesses(
                        hwndDlg,
                        &processItem,
                        1
                        );
                }
                break;
            case IDC_PERMISSIONS:
                {
                    PH_STD_OBJECT_SECURITY stdObjectSecurity;
                    PPH_ACCESS_ENTRY accessEntries;
                    ULONG numberOfAccessEntries;

                    stdObjectSecurity.OpenObject = PhpProcessGeneralOpenProcess;
                    stdObjectSecurity.Context = processItem->ProcessId;

                    if (PhGetAccessEntries(L"Process", &accessEntries, &numberOfAccessEntries))
                    {
                        PhEditSecurity(
                            hwndDlg,
                            processItem->ProcessName->Buffer,
                            PhStdGetObjectSecurity,
                            PhStdSetObjectSecurity,
                            &stdObjectSecurity,
                            accessEntries,
                            numberOfAccessEntries
                            );
                        PhFree(accessEntries);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI ThreadAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    // Parameter contains a pointer to the added thread item.
    PhReferenceObject(Parameter);
    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_ADDED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ThreadModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ThreadRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_REMOVED, 0, (LPARAM)Parameter);
}

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_THREADS_CONTEXT threadsContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        threadsContext = (PPH_THREADS_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCTHREADS_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            threadsContext = propPageContext->Context =
                PhAllocate(sizeof(PH_THREADS_CONTEXT));

            threadsContext->Provider = PhCreateThreadProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhSecondaryProviderThread,
                PhThreadProviderUpdate,
                threadsContext->Provider,
                &threadsContext->ProviderRegistration
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
            PhSetProviderEnabled(
                &threadsContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &threadsContext->ProviderRegistration
                );
            threadsContext->WindowHandle = hwndDlg;

            // Initialize the list.
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"TID");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Cycles Delta"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Start Address"); 
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 120, L"Priority"); 
        }
        break;
    case WM_DESTROY:
        {
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
            PhUnregisterProvider(
                &PhSecondaryProviderThread,
                &threadsContext->ProviderRegistration
                );

            PhDereferenceAllThreadItems(threadsContext->Provider);

            PhDereferenceObject(threadsContext->Provider);
            PhFree(threadsContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCTHREADS_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhpDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                PhSetProviderEnabled(&threadsContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&threadsContext->ProviderRegistration, FALSE);
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)header;

                        if (item->iItem != -1)
                        {
                            PPH_THREAD_ITEM threadItem;

                            if (PhGetListViewItemParam(
                                lvHandle,
                                item->iItem,
                                &threadItem
                                ))
                            {
                                PhShowThreadStackDialog(
                                    hwndDlg,
                                    threadsContext->Provider->ProcessId,
                                    threadItem->ThreadId,
                                    threadsContext->Provider->SymbolProvider
                                    );
                            }
                        }
                    }
                }
                break;
            case NM_RCLICK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;

                        if (itemActivate->iItem != -1)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_THREAD));
                            subMenu = GetSubMenu(menu, 0);

                            PhShowContextMenu(
                                hwndDlg,
                                lvHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_THREAD_ADDED:
        {
            INT lvItemIndex;
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                threadItem->ThreadIdString,
                threadItem
                );
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(threadItem->StartAddressString));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(threadItem->PriorityWin32String));
        }
        break;
    case WM_PH_THREAD_MODIFIED:
        {
            INT lvItemIndex;
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            lvItemIndex = PhFindListViewItemByParam(lvHandle, -1, threadItem);

            if (lvItemIndex != -1)
            {
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(threadItem->CyclesDeltaString));
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(threadItem->StartAddressString));
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(threadItem->PriorityWin32String));
            }
        }
        break;
    case WM_PH_THREAD_REMOVED:
        {
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, threadItem)
                );
            PhDereferenceObject(threadItem);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI ModuleAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    // Parameter contains a pointer to the added module item.
    PhReferenceObject(Parameter);
    PostMessage(modulesContext->WindowHandle, WM_PH_MODULE_ADDED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ModuleRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULE_REMOVED, 0, (LPARAM)Parameter);
}

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_MODULES_CONTEXT modulesContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        modulesContext = (PPH_MODULES_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCMODULES_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Lots of boilerplate code...

            modulesContext = propPageContext->Context =
                PhAllocate(sizeof(PH_MODULES_CONTEXT));

            modulesContext->Provider = PhCreateModuleProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhSecondaryProviderThread,
                PhModuleProviderUpdate,
                modulesContext->Provider,
                &modulesContext->ProviderRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->ModuleAddedEvent,
                ModuleAddedHandler,
                modulesContext,
                &modulesContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                ModuleRemovedHandler,
                modulesContext,
                &modulesContext->RemovedEventRegistration
                );
            PhSetProviderEnabled(
                &modulesContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &modulesContext->ProviderRegistration
                );
            modulesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 120, L"Base Address"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 200, L"File Name"); 
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleAddedEvent,
                &modulesContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                &modulesContext->RemovedEventRegistration
                );
            PhUnregisterProvider(
                &PhSecondaryProviderThread,
                &modulesContext->ProviderRegistration
                );

            PhDereferenceAllModuleItems(modulesContext->Provider);

            PhDereferenceObject(modulesContext->Provider);
            PhFree(modulesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCMODULES_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhpDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                PhSetProviderEnabled(&modulesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&modulesContext->ProviderRegistration, FALSE);
                break;
            case NM_RCLICK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;

                        if (itemActivate->iItem != -1)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_MODULE));
                            subMenu = GetSubMenu(menu, 0);

                            PhShowContextMenu(
                                hwndDlg,
                                lvHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_MODULE_ADDED:
        {
            INT lvItemIndex;
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                moduleItem->Name->Buffer,
                moduleItem
                );
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, moduleItem->BaseAddressString);
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(moduleItem->SizeString));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(moduleItem->FileName));
        }
        break;
    case WM_PH_MODULE_REMOVED:
        {
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, moduleItem)
                );
            PhDereferenceObject(moduleItem);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE processHandle;
            PPH_ENVIRONMENT_VARIABLE variables;
            ULONG numberOfVariables;
            ULONG i;
            HWND lvHandle = GetDlgItem(hwndDlg, IDC_PROCENVIRONMENT_LIST);

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                processItem->ProcessId
                )))
            {
                if (NT_SUCCESS(PhGetProcessEnvironmentVariables(
                    processHandle,
                    &variables,
                    &numberOfVariables
                    )))
                {
                    for (i = 0; i < numberOfVariables; i++)
                    {
                        INT lvItemIndex;

                        // Don't display pairs with no name.
                        if (variables[i].Name->Length == 0)
                            continue;

                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, PhGetString(variables[i].Name), NULL);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(variables[i].Value));
                    }

                    PhFreeProcessEnvironmentVariables(variables, numberOfVariables);
                }

                CloseHandle(processHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCENVIRONMENT_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhpDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI HandleAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    // Parameter contains a pointer to the added handle item.
    PhReferenceObject(Parameter);
    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_ADDED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandleModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandleRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_REMOVED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandlesUpdatedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLES_UPDATED, 0, 0);
}



INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_HANDLES_CONTEXT handlesContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        handlesContext = (PPH_HANDLES_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCHANDLES_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            handlesContext = propPageContext->Context =
                PhAllocate(sizeof(PH_HANDLES_CONTEXT));

            handlesContext->Provider = PhCreateHandleProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhSecondaryProviderThread,
                PhHandleProviderUpdate,
                handlesContext->Provider,
                &handlesContext->ProviderRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleAddedEvent,
                HandleAddedHandler,
                handlesContext,
                &handlesContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleModifiedEvent,
                HandleModifiedHandler,
                handlesContext,
                &handlesContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleRemovedEvent,
                HandleRemovedHandler,
                handlesContext,
                &handlesContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->UpdatedEvent,
                HandlesUpdatedHandler,
                handlesContext,
                &handlesContext->UpdatedEventRegistration
                );
            PhSetProviderEnabled(
                &handlesContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &handlesContext->ProviderRegistration
                );
            handlesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Handle"); 
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &handlesContext->Provider->HandleAddedEvent,
                &handlesContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &handlesContext->Provider->HandleModifiedEvent,
                &handlesContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &handlesContext->Provider->HandleRemovedEvent,
                &handlesContext->RemovedEventRegistration
                );
            PhUnregisterCallback(
                &handlesContext->Provider->UpdatedEvent,
                &handlesContext->UpdatedEventRegistration
                );
            PhUnregisterProvider(
                &PhSecondaryProviderThread,
                &handlesContext->ProviderRegistration
                );

            PhDereferenceAllHandleItems(handlesContext->Provider);

            PhDereferenceObject(handlesContext->Provider);
            PhFree(handlesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCHANDLES_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhpDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_HANDLE_CLOSE:
                {
                    NTSTATUS status;
                    HANDLE processHandle;

                    if (NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_DUP_HANDLE,
                        processItem->ProcessId
                        )))
                    {
                        PPH_HANDLE_ITEM *selectedItems;
                        ULONG numberOfItems;
                        ULONG i;

                        PhGetSelectedListViewItemParams(
                            lvHandle,
                            &selectedItems,
                            &numberOfItems
                            );

                        for (i = 0; i < numberOfItems; i++)
                        {
                            status = PhDuplicateObject(
                                processHandle,
                                selectedItems[i]->Handle,
                                NULL,
                                NULL,
                                0,
                                0,
                                DUPLICATE_CLOSE_SOURCE
                                );

                            if (!NT_SUCCESS(status))
                            {
                                if (!PhShowContinueStatus(
                                    hwndDlg,
                                    PhaFormatString(
                                    L"Unable to close the handle \"%s\" (0x%Ix)",
                                    PhGetString(selectedItems[i]->BestObjectName),
                                    selectedItems[i]->Handle
                                    )->Buffer,
                                    status,
                                    0
                                    ))
                                    break;
                            }
                        }

                        CloseHandle(processHandle);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the process", status, 0);
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
                PhSetProviderEnabled(&handlesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&handlesContext->ProviderRegistration, FALSE);
                break;
            case NM_RCLICK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;

                        if (itemActivate->iItem != -1)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_HANDLE));
                            subMenu = GetSubMenu(menu, 0);

                            SetMenuDefaultItem(subMenu, ID_HANDLE_PROPERTIES, FALSE);
                            PhShowContextMenu(
                                hwndDlg,
                                lvHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_HANDLE_ADDED:
        {
            INT lvItemIndex;
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            // Disable redraw. It will be re-enabled later.
            SendMessage(lvHandle, WM_SETREDRAW, FALSE, 0);

            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                PhGetString(handleItem->TypeName),
                handleItem
                );
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(handleItem->BestObjectName));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, handleItem->HandleString);
        }
        break;
    case WM_PH_HANDLE_MODIFIED:
        {
            INT lvItemIndex;
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            lvItemIndex = PhFindListViewItemByParam(lvHandle, -1, handleItem);

            if (lvItemIndex != -1)
            {
                // TODO when highlighting is implemented
            }
        }
        break;
    case WM_PH_HANDLE_REMOVED:
        {
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            SendMessage(lvHandle, WM_SETREDRAW, FALSE, 0);

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, handleItem)
                );
            PhDereferenceObject(handleItem);
        }
        break;
    case WM_PH_HANDLES_UPDATED:
        {
            // Enable redraw.
            SendMessage(lvHandle, WM_SETREDRAW, TRUE, 0);
            InvalidateRect(lvHandle, NULL, FALSE);
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhpProcessPropertiesThreadStart(
    __in PVOID Parameter
    )
{
    PPH_PROCESS_PROPCONTEXT PropContext = (PPH_PROCESS_PROPCONTEXT)Parameter;
    PPH_PROCESS_PROPPAGECONTEXT newPage;
    PPH_STRING startPage;
    PPH_AUTO_POOL autoPool;
    HWND hwnd;
    BOOL result;
    MSG message;

    PhBaseThreadInitialization();

    // Wait for stage 1 to be processed.
    PhWaitForEvent(&PropContext->ProcessItem->Stage1Event, INFINITE);
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
    PhDereferenceObject(newPage);

    // Threads
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCTHREADS),
        PhpProcessThreadsDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Modules
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCMODULES),
        PhpProcessModulesDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Environment
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCENVIRONMENT),
        PhpProcessEnvironmentDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Handles
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCHANDLES),
        PhpProcessHandlesDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    autoPool = PhCreateAutoPool();

    // Create the property sheet

    startPage = PhGetStringSetting(L"ProcPropPage");
    PropContext->PropSheetHeader.dwFlags |= PSH_USEPSTARTPAGE;
    PropContext->PropSheetHeader.pStartPage = startPage->Buffer;

    hwnd = (HWND)PropertySheet(&PropContext->PropSheetHeader);

    PhDereferenceObject(startPage);

    PropContext->WindowHandle = hwnd;
    PhSetEvent(&PropContext->CreatedEvent);

    // Main event loop

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            return STATUS_UNSUCCESSFUL;

        if (!PropSheet_IsDialogMessage(hwnd, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(autoPool);

        // Destroy the window when necessary.
        if (!PropSheet_GetCurrentPageHwnd(hwnd))
        {
            DestroyWindow(hwnd);
            break;
        }
    }

    PhDereferenceObject(PropContext);

    PhFreeAutoPool(autoPool);

    return STATUS_SUCCESS;
}

BOOLEAN PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    )
{
    HANDLE threadHandle;

    PhReferenceObject(Context);
    threadHandle = CreateThread(NULL, 0, PhpProcessPropertiesThreadStart, Context, 0, NULL);

    if (threadHandle)
    {
        CloseHandle(threadHandle);
        return TRUE;
    }
    else
    {
        PhDereferenceObject(Context);
        return FALSE;
    }
}

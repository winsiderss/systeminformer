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
#include <kph.h>
#include <settings.h>

PPH_OBJECT_TYPE PhpProcessPropContextType;
PPH_OBJECT_TYPE PhpProcessPropPageContextType;

BOOLEAN PhProcessPropInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhpProcessPropContextType,
        L"ProcessPropContext",
        0,
        PhpProcessPropContextDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhpProcessPropPageContextType,
        L"ProcessPropPageContext",
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
#define PROPSHEET_ADD_STYLE (WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

    switch (uMsg)
    {
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                if (((DLGTEMPLATEEX *)lParam)->signature == 0xffff)
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
            WNDPROC oldWndProc;
            PPH_LAYOUT_MANAGER layoutManager;

            // Disable multiple rows.
            // TODO: Add support for multiple rows
            PhSetWindowStyle(
                PropSheet_GetTabControl(hwndDlg),
                TCS_MULTILINE | TCS_SINGLELINE,
                TCS_SINGLELINE
                );

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
            if (!GetProp(hwnd, L"LayoutInitialized"))
            {
                PPH_LAYOUT_MANAGER layoutManager;

                layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwnd, L"LayoutManager");

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

                SetProp(hwnd, L"LayoutInitialized", (HANDLE)TRUE);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                // Disable the OK button from working (even though 
                // it's already hidden). This prevents the Enter 
                // key from closing the dialog box.
                return 0;
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

BOOLEAN PhAddProcessPropPage2(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in HPROPSHEETPAGE PropSheetPageHandle
    )
{
    if (PropContext->PropSheetHeader.nPages == PH_PROCESS_PROPCONTEXT_MAXPAGES)
        return FALSE;

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] =
        PropSheetPageHandle;
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
        SetProp(hwndDlg, L"PropSheetPage", (HANDLE)lParam);
    }

    propSheetPage = (LPPROPSHEETPAGE)GetProp(hwndDlg, L"PropSheetPage");

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
    RemoveProp(hwndDlg, L"PropSheetPage");
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
            HANDLE processHandle = NULL;
            PPH_STRING curdir = NULL;
            PROCESS_BASIC_INFORMATION basicInfo;
            SYSTEMTIME startTime;
            CLIENT_ID clientId;
            ULONG executeFlags;
            BOOLEAN isProtected;

            // File

            SendMessage(GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON,
                (WPARAM)processItem->LargeIcon, 0);

            SetDlgItemText(hwndDlg, IDC_NAME, PhpGetStringOrNa(processItem->VersionInfo.FileDescription));
            SetDlgItemText(hwndDlg, IDC_VERSION, PhpGetStringOrNa(processItem->VersionInfo.FileVersion));
            SetDlgItemText(hwndDlg, IDC_FILENAME, PhpGetStringOrNa(processItem->FileName));

            if (processItem->VerifyResult == VrTrusted)
            {
                if (processItem->VerifySignerName)
                {
                    SetDlgItemText(hwndDlg, IDC_COMPANYNAME,
                        PhaConcatStrings2(L"(Verified) ", processItem->VerifySignerName->Buffer)->Buffer);
                }
                else
                {
                    SetDlgItemText(hwndDlg, IDC_COMPANYNAME,
                        PhaConcatStrings2(
                        L"(Verified) ",
                        PhGetStringOrEmpty(processItem->VersionInfo.CompanyName)
                        )->Buffer);
                }
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_COMPANYNAME,
                    PhpGetStringOrNa(processItem->VersionInfo.CompanyName));
            }

            // Command Line

            SetDlgItemText(hwndDlg, IDC_CMDLINE, PhpGetStringOrNa(processItem->CommandLine));

            // Current Directory

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

                NtClose(processHandle);
                processHandle = NULL;
            }

            SetDlgItemText(hwndDlg, IDC_CURDIR,
                PhpGetStringOrNa(curdir));

            if (curdir)
                PhDereferenceObject(curdir);

            // Started

            PhLargeIntegerToLocalSystemTime(&startTime, &processItem->CreateTime);
            SetDlgItemText(hwndDlg, IDC_STARTED,
                PhaConcatStrings(
                3,
                ((PPH_STRING)PHA_DEREFERENCE(PhFormatTime(&startTime, NULL)))->Buffer,
                L" ",
                ((PPH_STRING)PHA_DEREFERENCE(PhFormatDate(&startTime, NULL)))->Buffer
                )->Buffer);

            // Parent

            clientId.UniqueProcess = processItem->ParentProcessId;
            clientId.UniqueThread = NULL;

            SetDlgItemText(hwndDlg, IDC_PARENTPROCESS,
                ((PPH_STRING)PHA_DEREFERENCE(PhGetClientIdName(&clientId)))->Buffer);

            // DEP

            SetDlgItemText(hwndDlg, IDC_DEP, L"N/A");

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_INFORMATION,
                processItem->ProcessId
                )))
            {
                if (NT_SUCCESS(PhGetProcessExecuteFlags(processHandle, &executeFlags)))
                {
                    PPH_STRING depString;

                    if (executeFlags & MEM_EXECUTE_OPTION_DISABLE)
                        depString = PhaCreateString(L"Enabled");
                    else
                        depString = PhaCreateString(L"Disabled");

                    if (executeFlags & MEM_EXECUTE_OPTION_PERMANENT)
                    {
                        depString = PhaConcatStrings2(depString->Buffer, L", Permanent");
                    }

                    if (executeFlags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
                    {
                        depString = PhaConcatStrings2(depString->Buffer, L", DEP-ATL thunk emulation disabled");
                    }

                    SetDlgItemText(hwndDlg, IDC_DEP, depString->Buffer);
                }

                NtClose(processHandle);
                processHandle = NULL;
            }

            // PEB address

            SetDlgItemText(hwndDlg, IDC_PEBADDRESS, L"N/A");

            PhOpenProcess(
                &processHandle,
                ProcessQueryAccess,
                processItem->ProcessId
                );

            if (processHandle)
            {
                PhGetProcessBasicInformation(processHandle, &basicInfo);
                SetDlgItemText(hwndDlg, IDC_PEBADDRESS,
                    PhaFormatString(L"0x%Ix", basicInfo.PebBaseAddress)->Buffer);
            }

            // Protected

            SetDlgItemText(hwndDlg, IDC_PROTECTION, L"N/A");

            if (WINDOWS_HAS_LIMITED_ACCESS)
            {
                if (PhKphHandle)
                {
                    if (NT_SUCCESS(KphGetProcessProtected(PhKphHandle, processItem->ProcessId, &isProtected)))
                    {
                        if (isProtected)
                            SetDlgItemText(hwndDlg, IDC_PROTECTION, L"Protected");
                        else
                            SetDlgItemText(hwndDlg, IDC_PROTECTION, L"Not Protected");
                    }
                }
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROTECTION), FALSE);
            }

            if (processHandle)
                NtClose(processHandle);

#ifdef _M_X64
            if (processItem->IsWow64)
                SetDlgItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"32-bit");
            else
                SetDlgItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"64-bit");
#else
            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESSTYPELABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESSTYPETEXT), SW_HIDE);
#endif
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
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMPANYNAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VERSION),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILENAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CMDLINE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CURDIR),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STARTED),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PEBADDRESS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PARENTPROCESS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DEP),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROTECTION),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TERMINATE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PERMISSIONS),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESS),
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
                    stdObjectSecurity.ObjectType = L"Process";
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

static VOID NTAPI ThreadsUpdatedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREADS_UPDATED, 0, 0);
}

static VOID NTAPI ThreadsLoadingStateChangedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(
        GetDlgItem(threadsContext->WindowHandle, IDC_PROCTHREADS_LIST),
        ELVM_SETCURSOR,
        0,
        // Parameter contains TRUE if loading symbols
        (LPARAM)(Parameter ? LoadCursor(NULL, IDC_APPSTARTING) : NULL)
        );
}

VOID PhpInitializeThreadMenu(
    __in HMENU Menu,
    __in HANDLE ProcessId,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    )
{
#define ANALYZE_MENU_INDEX 8
#define IOPRIORITY_MENU_INDEX 10

    if (NumberOfThreads == 0)
    {
        PhEnableAllMenuItems(Menu, FALSE);
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
            ID_THREAD_FORCETERMINATE,
            ID_THREAD_SUSPEND,
            ID_THREAD_RESUME
        };
        ULONG i;

        PhEnableAllMenuItems(Menu, FALSE);

        // These menu items are capable of manipulating 
        // multiple threads.
        for (i = 0; i < sizeof(menuItemsMultiEnabled) / sizeof(ULONG); i++)
        {
            EnableMenuItem(Menu, menuItemsMultiEnabled[i], MF_ENABLED);
        }
    }

    // Remove irrelevant menu items.

    if (WindowsVersion < WINDOWS_VISTA)
    {
        // Remove I/O priority.
        DeleteMenu(Menu, IOPRIORITY_MENU_INDEX, MF_BYPOSITION);
    }

#ifndef _M_IX86
    // Remove Analyze.
    DeleteMenu(Menu, ANALYZE_MENU_INDEX, MF_BYPOSITION);
#endif

    if (!PhKphHandle)
    {
        // Remove Force Terminate.
        DeleteMenu(Menu, ID_THREAD_FORCETERMINATE, 0);
    }
    else
    {
        if (ProcessId == SYSTEM_PROCESS_ID)
        {
            // Remove Force Terminate because Terminate does 
            // the same job.
            DeleteMenu(Menu, ID_THREAD_FORCETERMINATE, 0);
        }
    }

    PhEnableMenuItem(Menu, ID_THREAD_TOKEN, FALSE);

    // Priority
    if (NumberOfThreads == 1)
    {
        HANDLE threadHandle;
        ULONG ioPriority = -1;
        ULONG threadPriority = THREAD_PRIORITY_ERROR_RETURN;
        ULONG id = 0;

        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            ThreadQueryAccess,
            Threads[0]->ThreadId
            )))
        {
            threadPriority = GetThreadPriority(threadHandle);

            if (WindowsVersion >= WINDOWS_VISTA)
            {
                if (!NT_SUCCESS(PhGetThreadIoPriority(
                    threadHandle,
                    &ioPriority
                    )))
                {
                    ioPriority = -1;
                }
            }

            // Token
            {
                HANDLE tokenHandle;

                if (NT_SUCCESS(PhOpenThreadToken(
                    &tokenHandle,
                    TOKEN_QUERY,
                    threadHandle,
                    TRUE
                    )))
                {
                    PhEnableMenuItem(Menu, ID_THREAD_TOKEN, TRUE);
                    NtClose(tokenHandle);
                }
            }

            NtClose(threadHandle);
        }

        switch (threadPriority)
        {
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
            id = ID_PRIORITY_IDLE;
            break;
        }

        if (id != 0)
        {
            CheckMenuItem(Menu, id, MF_CHECKED);
            PhSetRadioCheckMenuItem(Menu, id, TRUE);
        }

        if (ioPriority != -1)
        {
            id = 0;

            switch (ioPriority)
            {
            case 0:
                id = ID_I_0;
                break;
            case 1:
                id = ID_I_1;
                break;
            case 2:
                id = ID_I_2;
                break;
            case 3:
                id = ID_I_3;
                break;
            }

            if (id != 0)
            {
                CheckMenuItem(Menu, id, MF_CHECKED);
                PhSetRadioCheckMenuItem(Menu, id, TRUE);
            }
        }
    }
}

NTSTATUS NTAPI PhpThreadPermissionsOpenThread(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    )
{
    return PhOpenThread(Handle, DesiredAccess, (HANDLE)Context);
}

INT NTAPI PhpThreadTidCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_THREAD_ITEM item1 = Item1;
    PPH_THREAD_ITEM item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->ThreadId, (ULONG_PTR)item2->ThreadId);
}

INT NTAPI PhpThreadCyclesCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_THREAD_ITEM item1 = Item1;
    PPH_THREAD_ITEM item2 = Item2;
    PPH_THREADS_CONTEXT threadsContext = Context;

    if (threadsContext->UseCycleTime)
        return uint64cmp(item1->CyclesDelta.Delta, item2->CyclesDelta.Delta);
    else
        return uint64cmp(item1->ContextSwitchesDelta.Delta, item2->ContextSwitchesDelta.Delta);
}

INT NTAPI PhpThreadPriorityCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_THREAD_ITEM item1 = Item1;
    PPH_THREAD_ITEM item2 = Item2;

    return intcmp(item1->PriorityWin32, item2->PriorityWin32);
}

COLORREF NTAPI PhpThreadColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PPH_THREAD_ITEM item = Param;

    if (PhCsUseColorSuspended && item->WaitReason == Suspended)
        return PhCsColorSuspended;
    if (PhCsUseColorGuiThreads && item->IsGuiThread)
        return PhCsColorGuiThreads;

    return PhSysWindowColor;
}

NTSTATUS NTAPI PhpOpenThreadTokenObject(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    )
{
    return PhOpenThreadToken(
        Handle,
        DesiredAccess,
        (HANDLE)Context,
        TRUE
        );
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
            PhRegisterCallbackEx(
                &threadsContext->Provider->ThreadAddedEvent,
                ThreadAddedHandler,
                threadsContext,
                PH_CALLBACK_SYNC_WITH_UNREGISTER,
                &threadsContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                ThreadModifiedHandler,
                threadsContext,
                &threadsContext->ModifiedEventRegistration
                );
            PhRegisterCallbackEx(
                &threadsContext->Provider->ThreadRemovedEvent,
                ThreadRemovedHandler,
                threadsContext,
                PH_CALLBACK_SYNC_WITH_UNREGISTER,
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
            threadsContext->UseCycleTime = FALSE;
            threadsContext->NeedsRedraw = FALSE;
            threadsContext->NeedsSort = FALSE;

            if (processItem->ProcessId != SYSTEM_IDLE_PROCESS_ID)
            {
                // Use Cycles instead of Context Switches on Vista.
                if (WINDOWS_HAS_THREAD_CYCLES)
                    threadsContext->UseCycleTime = TRUE;
            }

            // Initialize the list.
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"TID");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80,
                threadsContext->UseCycleTime ? L"Cycles Delta" : L"Context Switches Delta"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 180, L"Start Address"); 
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Priority");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetContext(lvHandle, threadsContext);
            ExtendedListView_SetCompareFunction(lvHandle, 0, PhpThreadTidCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 1, PhpThreadCyclesCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 3, PhpThreadPriorityCompareFunction);
            ExtendedListView_SetSort(lvHandle, 1, DescendingSortOrder);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpThreadColorFunction);
            ExtendedListView_SetStateHighlighting(lvHandle, TRUE);

            // Sort by TID, Start Address, Priority, then Cycles/Context Switches Delta.
            {
                ULONG fallbackColumns[] = { 0, 2, 3, 1 };

                ExtendedListView_AddFallbackColumns(lvHandle,
                    sizeof(fallbackColumns) / sizeof(ULONG), fallbackColumns);
            }

            PhSetProviderEnabled(
                &threadsContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &threadsContext->ProviderRegistration
                );
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
            PhUnregisterCallback(
                &threadsContext->Provider->UpdatedEvent,
                &threadsContext->UpdatedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->LoadingStateChangedEvent,
                &threadsContext->LoadingStateChangedEventRegistration
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
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_THREAD_INSPECT:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (threadItem)
                    {
                        PhShowThreadStackDialog(
                            hwndDlg,
                            threadsContext->Provider->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider->SymbolProvider
                            );
                    }
                }
                break;
            case ID_THREAD_TERMINATE:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedListViewItemParams(lvHandle, &threads, &numberOfThreads);

                    if (
                        processItem->ProcessId != SYSTEM_PROCESS_ID ||
                        !PhKphHandle
                        )
                    {
                        PhUiTerminateThreads(hwndDlg, threads, numberOfThreads);
                    }
                    else
                    {
                        PhUiForceTerminateThreads(hwndDlg, processItem->ProcessId, threads, numberOfThreads);
                    }

                    PhFree(threads);
                }
                break;
            case ID_THREAD_FORCETERMINATE:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedListViewItemParams(lvHandle, &threads, &numberOfThreads);
                    PhUiForceTerminateThreads(hwndDlg, processItem->ProcessId, threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_SUSPEND:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedListViewItemParams(lvHandle, &threads, &numberOfThreads);
                    PhUiSuspendThreads(hwndDlg, threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_RESUME:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedListViewItemParams(lvHandle, &threads, &numberOfThreads);
                    PhUiResumeThreads(hwndDlg, threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_PERMISSIONS:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedListViewItemParam(lvHandle);
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
                                PhaFormatString(L"Thread %u", (ULONG)threadItem->ThreadId)->Buffer,
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
                    PPH_THREAD_ITEM threadItem = PhGetSelectedListViewItemParam(lvHandle);
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
                    PPH_THREAD_ITEM threadItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (threadItem)
                    {
                        PhUiAnalyzeWaitThread(
                            hwndDlg,
                            processItem->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider->SymbolProvider
                            );
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
                    PPH_THREAD_ITEM threadItem = PhGetSelectedListViewItemParam(lvHandle);

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

                        PhUiSetPriorityThread(hwndDlg, threadItem, threadPriorityWin32);
                    }
                }
                break;
            case ID_I_0:
            case ID_I_1:
            case ID_I_2:
            case ID_I_3:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (threadItem)
                    {
                        ULONG ioPriority;

                        switch (id)
                        {
                            case ID_I_0:
                                ioPriority = 0;
                                break;
                            case ID_I_1:
                                ioPriority = 1;
                                break;
                            case ID_I_2:
                                ioPriority = 2;
                                break;
                            case ID_I_3:
                                ioPriority = 3;
                                break;
                        }

                        PhUiSetIoPriorityThread(hwndDlg, threadItem, ioPriority);
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
                PhSetProviderEnabled(&threadsContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&threadsContext->ProviderRegistration, FALSE);
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, ID_THREAD_INSPECT, 0);
                    }
                }
                break;
            case NM_RCLICK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                        PPH_THREAD_ITEM *threads;
                        ULONG numberOfThreads;

                        PhGetSelectedListViewItemParams(lvHandle, &threads, &numberOfThreads);

                        if (numberOfThreads != 0)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_THREAD));
                            subMenu = GetSubMenu(menu, 0);

                            SetMenuDefaultItem(subMenu, ID_THREAD_INSPECT, FALSE);
                            PhpInitializeThreadMenu(subMenu, processItem->ProcessId, threads, numberOfThreads);

                            PhShowContextMenu(
                                hwndDlg,
                                lvHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }

                        PhFree(threads);
                    }
                }
                break;
            case LVN_KEYDOWN:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)header;

                        switch (keyDown->wVKey)
                        {
                        case VK_DELETE:
                            SendMessage(hwndDlg, WM_COMMAND, ID_THREAD_TERMINATE, 0);
                            break;
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

            if (!threadsContext->NeedsRedraw)
            {
                // Disable redraw. It will be re-enabled later.
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                threadsContext->NeedsRedraw = TRUE;
            }

            if (threadItem->RunId == 0) ExtendedListView_SetStateHighlighting(lvHandle, FALSE);
            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                threadItem->ThreadIdString,
                threadItem
                );
            if (threadItem->RunId == 0) ExtendedListView_SetStateHighlighting(lvHandle, TRUE);
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(threadItem->StartAddressString));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(threadItem->PriorityWin32String));

            threadsContext->NeedsSort = TRUE;
        }
        break;
    case WM_PH_THREAD_MODIFIED:
        {
            INT lvItemIndex;
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            if (!threadsContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                threadsContext->NeedsRedraw = TRUE;
            }

            lvItemIndex = PhFindListViewItemByParam(lvHandle, -1, threadItem);

            if (lvItemIndex != -1)
            {
                if (threadsContext->UseCycleTime)
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(threadItem->CyclesDeltaString));
                else
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(threadItem->ContextSwitchesDeltaString));

                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(threadItem->StartAddressString));
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(threadItem->PriorityWin32String));

                threadsContext->NeedsSort = TRUE;
            }
        }
        break;
    case WM_PH_THREAD_REMOVED:
        {
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            if (!threadsContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                threadsContext->NeedsRedraw = TRUE;
            }

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, threadItem)
                );
            PhDereferenceObject(threadItem);
        }
        break;
    case WM_PH_THREADS_UPDATED:
        {
            ExtendedListView_Tick(lvHandle);

            if (threadsContext->NeedsSort)
            {
                ExtendedListView_SortItems(lvHandle);
                threadsContext->NeedsSort = FALSE;
            }

            if (threadsContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, TRUE);
                threadsContext->NeedsRedraw = FALSE;
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, lvHandle, uMsg, wParam, lParam);

    return FALSE;
}

static NTSTATUS NTAPI PhpOpenProcessToken(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        ProcessQueryAccess,
        (HANDLE)Context
        )))
        return status;

    status = PhOpenProcessToken(Handle, DesiredAccess, processHandle);
    NtClose(processHandle);

    return status;
}

INT_PTR CALLBACK PhpProcessTokenHookProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"LayoutInitialized");
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!GetProp(hwndDlg, L"LayoutInitialized"))
            {
                PPH_LAYOUT_ITEM dialogItem;

                // This is a big violation of abstraction...

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_USER),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_USERSID),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIRTUALIZED),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_GROUPS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PRIVILEGES),
                    dialogItem, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADVANCED),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                PhpDoPropPageLayout(hwndDlg);

                SetProp(hwndDlg, L"LayoutInitialized", (HANDLE)TRUE);
            }
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

static VOID NTAPI ModulesUpdatedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULES_UPDATED, 0, 0);
}

VOID PhpInitializeModuleMenu(
    __in HMENU Menu,
    __in HANDLE ProcessId,
    __in PPH_MODULE_ITEM *Modules,
    __in ULONG NumberOfModules
    )
{
    if (NumberOfModules == 0)
    {
        PhEnableAllMenuItems(Menu, FALSE);
    }
    else if (NumberOfModules == 1)
    {
        // Nothing
    }
    else
    {
        // None of the menu items work with multiple items.
        PhEnableAllMenuItems(Menu, FALSE);
    }
}

INT NTAPI PhpModuleTriStateCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_MODULE_ITEM item1 = Item1;
    PPH_MODULE_ITEM item2 = Item2;

    // Place the primary module above the others.
    if (item1->IsFirst)
        return -1;
    else if (item2->IsFirst)
        return 1;

    return 0;
}

INT NTAPI PhpModuleBaseAddressCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_MODULE_ITEM item1 = Item1;
    PPH_MODULE_ITEM item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->BaseAddress, (ULONG_PTR)item2->BaseAddress);
}

INT NTAPI PhpModuleSizeCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_MODULE_ITEM item1 = Item1;
    PPH_MODULE_ITEM item2 = Item2;

    return uintcmp(item1->Size, item2->Size);
}

COLORREF NTAPI PhpModuleColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PPH_MODULE_ITEM item = Param;

    if (PhCsUseColorDotNet && (item->Flags & LDRP_COR_IMAGE))
        return PhCsColorDotNet;
    if (PhCsUseColorRelocatedModules && (item->Flags & LDRP_IMAGE_NOT_AT_BASE))
        return PhCsColorRelocatedModules;

    return PhSysWindowColor;
}

HFONT NTAPI PhpModuleFontFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PPH_MODULE_ITEM item = Param;

    if (item->IsFirst)
        return PhBoldMessageFont;

    return NULL;
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
            PhRegisterCallbackEx(
                &modulesContext->Provider->ModuleAddedEvent,
                ModuleAddedHandler,
                modulesContext,
                PH_CALLBACK_SYNC_WITH_UNREGISTER,
                &modulesContext->AddedEventRegistration
                );
            PhRegisterCallbackEx(
                &modulesContext->Provider->ModuleRemovedEvent,
                ModuleRemovedHandler,
                modulesContext,
                PH_CALLBACK_SYNC_WITH_UNREGISTER,
                &modulesContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->UpdatedEvent,
                ModulesUpdatedHandler,
                modulesContext,
                &modulesContext->UpdatedEventRegistration
                );
            modulesContext->WindowHandle = hwndDlg;
            modulesContext->NeedsRedraw = FALSE;
            modulesContext->NeedsSort = FALSE;

            // Initialize the list.
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Base Address"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 60, L"Size");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 160, L"Description");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetTriState(lvHandle, TRUE);
            ExtendedListView_SetTriStateCompareFunction(lvHandle, PhpModuleTriStateCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 1, PhpModuleBaseAddressCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 2, PhpModuleSizeCompareFunction);
            ExtendedListView_SetSort(lvHandle, 0, NoSortOrder);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpModuleColorFunction);
            ExtendedListView_SetItemFontFunction(lvHandle, PhpModuleFontFunction);
            ExtendedListView_SetStateHighlighting(lvHandle, TRUE);

            // Sort by Name, Base Address, Size.
            {
                ULONG fallbackColumns[] = { 0, 1, 2 };

                ExtendedListView_AddFallbackColumns(lvHandle,
                    sizeof(fallbackColumns) / sizeof(ULONG), fallbackColumns);
            }

            PhSetProviderEnabled(
                &modulesContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &modulesContext->ProviderRegistration
                );
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
            PhUnregisterCallback(
                &modulesContext->Provider->UpdatedEvent,
                &modulesContext->UpdatedEventRegistration
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
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_MODULE_UNLOAD:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (moduleItem)
                    {
                        PhUiUnloadModule(hwndDlg, processItem->ProcessId, moduleItem);
                    }
                }
                break;
            case ID_MODULE_OPENCONTAININGFOLDER:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (moduleItem)
                    {
                        PhShellExploreFile(hwndDlg, moduleItem->FileName->Buffer);
                    }
                }
                break;
            case ID_MODULE_PROPERTIES:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (moduleItem)
                    {
                        PhShellProperties(hwndDlg, moduleItem->FileName->Buffer);
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
                        PPH_MODULE_ITEM *modules;
                        ULONG numberOfModules;

                        PhGetSelectedListViewItemParams(lvHandle, &modules, &numberOfModules);

                        if (numberOfModules != 0)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_MODULE));
                            subMenu = GetSubMenu(menu, 0);

                            PhpInitializeModuleMenu(subMenu, processItem->ProcessId, modules, numberOfModules);

                            PhShowContextMenu(
                                hwndDlg,
                                lvHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }

                        PhFree(modules);
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

            if (!modulesContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                modulesContext->NeedsRedraw = TRUE;
            }

            if (moduleItem->RunId == 0) ExtendedListView_SetStateHighlighting(lvHandle, FALSE);
            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                moduleItem->Name->Buffer,
                moduleItem
                );
            if (moduleItem->RunId == 0) ExtendedListView_SetStateHighlighting(lvHandle, TRUE);
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, moduleItem->BaseAddressString);
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(moduleItem->SizeString));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(moduleItem->VersionInfo.FileDescription));

            modulesContext->NeedsSort = TRUE;
        }
        break;
    case WM_PH_MODULE_REMOVED:
        {
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            if (!modulesContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                modulesContext->NeedsRedraw = TRUE;
            }

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, moduleItem)
                );
            PhDereferenceObject(moduleItem);
        }
        break;
    case WM_PH_MODULES_UPDATED:
        {
            ExtendedListView_Tick(lvHandle);

            if (modulesContext->NeedsSort)
            {
                ExtendedListView_SortItems(lvHandle);
                modulesContext->NeedsSort = FALSE;
            }

            if (modulesContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, TRUE);
                modulesContext->NeedsRedraw = FALSE;
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, lvHandle, uMsg, wParam, lParam);

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

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");

            PhSetExtendedListView(lvHandle);

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

                NtClose(processHandle);
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

VOID PhpInitializeHandleMenu(
    __in HMENU Menu,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM *Handles,
    __in ULONG NumberOfHandles,
    __inout PPH_HANDLES_CONTEXT HandlesContext
    )
{
    if (NumberOfHandles == 0)
    {
        PhEnableAllMenuItems(Menu, FALSE);
    }
    else if (NumberOfHandles == 1)
    {
        // Nothing
    }
    else
    {
        PhEnableAllMenuItems(Menu, FALSE);

        EnableMenuItem(Menu, ID_HANDLE_CLOSE, MF_ENABLED);
    }

    // Remove irrelevant menu items.

    if (!PhKphHandle)
    {
        DeleteMenu(Menu, ID_HANDLE_PROTECTED, 0);
        DeleteMenu(Menu, ID_HANDLE_INHERIT, 0);
    }

    // Protected, Inherit
    if (NumberOfHandles == 1 && PhKphHandle)
    {
        HandlesContext->SelectedHandleProtected = FALSE;
        HandlesContext->SelectedHandleInherit = FALSE;

        if (Handles[0]->Attributes & OBJ_PROTECT_CLOSE)
        {
            HandlesContext->SelectedHandleProtected = TRUE;
            CheckMenuItem(Menu, ID_HANDLE_PROTECTED, MF_CHECKED);
        }

        if (Handles[0]->Attributes & OBJ_INHERIT)
        {
            HandlesContext->SelectedHandleInherit = TRUE;
            CheckMenuItem(Menu, ID_HANDLE_INHERIT, MF_CHECKED);
        }
    }
}

INT NTAPI PhpHandleHandleCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_HANDLE_ITEM item1 = Item1;
    PPH_HANDLE_ITEM item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->Handle, (ULONG_PTR)item2->Handle);
}

COLORREF NTAPI PhpHandleColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PPH_HANDLE_ITEM item = Param;

    if (PhCsUseColorProtectedHandles && (item->Attributes & OBJ_PROTECT_CLOSE))
        return PhCsColorProtectedHandles;
    if (PhCsUseColorInheritHandles && (item->Attributes & OBJ_INHERIT))
        return PhCsColorInheritHandles;

    return PhSysWindowColor;
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
            PhRegisterCallbackEx(
                &handlesContext->Provider->HandleAddedEvent,
                HandleAddedHandler,
                handlesContext,
                PH_CALLBACK_SYNC_WITH_UNREGISTER,
                &handlesContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleModifiedEvent,
                HandleModifiedHandler,
                handlesContext,
                &handlesContext->ModifiedEventRegistration
                );
            PhRegisterCallbackEx(
                &handlesContext->Provider->HandleRemovedEvent,
                HandleRemovedHandler,
                handlesContext,
                PH_CALLBACK_SYNC_WITH_UNREGISTER,
                &handlesContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->UpdatedEvent,
                HandlesUpdatedHandler,
                handlesContext,
                &handlesContext->UpdatedEventRegistration
                );
            handlesContext->WindowHandle = hwndDlg;
            handlesContext->NeedsRedraw = FALSE;
            handlesContext->NeedsSort = FALSE;
            handlesContext->HideUnnamedHandles = !!PhGetIntegerSetting(L"HideUnnamedHandles");

            // Initialize the list.
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Handle");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetCompareFunction(lvHandle, 2, PhpHandleHandleCompareFunction);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpHandleColorFunction);
            ExtendedListView_SetStateHighlighting(lvHandle, TRUE);

            // Sort by Type, Handle, Name.
            {
                ULONG fallbackColumns[] = { 0, 2, 1 };

                ExtendedListView_AddFallbackColumns(lvHandle,
                    sizeof(fallbackColumns) / sizeof(ULONG), fallbackColumns);
            }

            PhSetProviderEnabled(
                &handlesContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &handlesContext->ProviderRegistration
                );
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
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_HANDLE_CLOSE:
                {
                    PPH_HANDLE_ITEM *handles;
                    ULONG numberOfHandles;

                    PhGetSelectedListViewItemParams(lvHandle, &handles, &numberOfHandles);
                    PhUiNtCloses(hwndDlg, processItem->ProcessId, handles, numberOfHandles, !!lParam);
                    PhFree(handles);
                }
                break;
            case ID_HANDLE_PROTECTED:
            case ID_HANDLE_INHERIT:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (handleItem)
                    {
                        ULONG attributes = 0;

                        // Re-create the attributes.

                        if (handlesContext->SelectedHandleProtected)
                            attributes |= OBJ_PROTECT_CLOSE;
                        if (handlesContext->SelectedHandleInherit)
                            attributes |= OBJ_INHERIT;

                        // Toggle the appropriate bit.

                        if (id == ID_HANDLE_PROTECTED)
                            attributes ^= OBJ_PROTECT_CLOSE;
                        else if (id == ID_HANDLE_INHERIT)
                            attributes ^= OBJ_INHERIT;

                        PhUiSetAttributesHandle(hwndDlg, processItem->ProcessId, handleItem, attributes);
                    }
                }
                break;
            case ID_HANDLE_PROPERTIES:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (handleItem)
                    {
                        // The object relies on the list view reference, which could 
                        // disappear if we don't reference the object here.
                        PhReferenceObject(handleItem);
                        PhShowHandleProperties(hwndDlg, processItem->ProcessId, handleItem);
                        PhDereferenceObject(handleItem);
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
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, ID_HANDLE_PROPERTIES, 0);
                    }
                }
                break;
            case NM_RCLICK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                        PPH_HANDLE_ITEM *handles;
                        ULONG numberOfHandles;

                        PhGetSelectedListViewItemParams(lvHandle, &handles, &numberOfHandles);

                        if (numberOfHandles != 0)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_HANDLE));
                            subMenu = GetSubMenu(menu, 0);

                            SetMenuDefaultItem(subMenu, ID_HANDLE_PROPERTIES, FALSE);
                            PhpInitializeHandleMenu(
                                subMenu,
                                processItem->ProcessId,
                                handles,
                                numberOfHandles,
                                handlesContext
                                );

                            PhShowContextMenu(
                                hwndDlg,
                                lvHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }

                        PhFree(handles);
                    }
                }
                break;
            case LVN_KEYDOWN:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)header;

                        switch (keyDown->wVKey)
                        {
                        case VK_DELETE:
                            // Pass a 1 in lParam to indicate that warnings should be 
                            // enabled.
                            SendMessage(hwndDlg, WM_COMMAND, ID_HANDLE_CLOSE, 1);
                            break;
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

            // If we're hiding unnamed handles and this handle doesn't 
            // have a name, don't add it.
            if (
                handlesContext->HideUnnamedHandles &&
                PhIsStringNullOrEmpty(handleItem->BestObjectName)
                )
            {
                // No need to dereference; if we re-add the handle when 
                // the user changes the "hide unnamed handles" setting 
                // we can just assume we have a reference to the handle 
                // item. This is important for consistency with the 
                // PhDereferenceAllHandleItems call in WM_DESTROY.
                break;
            }

            if (!handlesContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                handlesContext->NeedsRedraw = TRUE;
            }

            if (handleItem->RunId == 0) ExtendedListView_SetStateHighlighting(lvHandle, FALSE);
            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                PhGetString(handleItem->TypeName),
                handleItem
                );
            if (handleItem->RunId == 0) ExtendedListView_SetStateHighlighting(lvHandle, TRUE);
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(handleItem->BestObjectName));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, handleItem->HandleString);

            handlesContext->NeedsSort = TRUE;
        }
        break;
    case WM_PH_HANDLE_MODIFIED:
        {
            INT lvItemIndex;
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            lvItemIndex = PhFindListViewItemByParam(lvHandle, -1, handleItem);

            if (lvItemIndex != -1)
            {
                // Force redraw of the item because its color may have 
                // changed.
                ListView_RedrawItems(lvHandle, lvItemIndex, lvItemIndex);
            }
        }
        break;
    case WM_PH_HANDLE_REMOVED:
        {
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            if (!handlesContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, FALSE);
                handlesContext->NeedsRedraw = TRUE;
            }

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, handleItem)
                );
            PhDereferenceObject(handleItem);
        }
        break;
    case WM_PH_HANDLES_UPDATED:
        {
            ExtendedListView_Tick(lvHandle);

            if (handlesContext->NeedsSort)
            {
                ExtendedListView_SortItems(lvHandle);
                handlesContext->NeedsSort = FALSE;
            }

            if (handlesContext->NeedsRedraw)
            {
                ExtendedListView_SetRedraw(lvHandle, TRUE);
                handlesContext->NeedsRedraw = FALSE;
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, lvHandle, uMsg, wParam, lParam);

    return FALSE;
}

static VOID NTAPI ServiceModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)Parameter;
    PPH_SERVICES_CONTEXT servicesContext = (PPH_SERVICES_CONTEXT)Context;
    PPH_SERVICE_MODIFIED_DATA copy;

    copy = PhAllocateCopy(serviceModifiedData, sizeof(PH_SERVICE_MODIFIED_DATA));

    PostMessage(servicesContext->WindowHandle, WM_PH_SERVICE_MODIFIED, 0, (LPARAM)copy);
}

VOID PhpFixProcessServicesControls(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    HWND startButton;
    HWND pauseButton;
    HWND descriptionLabel;

    startButton = GetDlgItem(hWnd, IDC_START);
    pauseButton = GetDlgItem(hWnd, IDC_PAUSE);
    descriptionLabel = GetDlgItem(hWnd, IDC_DESCRIPTION);

    if (ServiceItem)
    {
        SC_HANDLE serviceHandle;
        PPH_STRING description;

        switch (ServiceItem->State)
        {
        case SERVICE_RUNNING:
            {
                SetWindowText(startButton, L"Stop");
                SetWindowText(pauseButton, L"Pause");
                EnableWindow(startButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_STOP);
                EnableWindow(pauseButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
            }
            break;
        case SERVICE_PAUSED:
            {
                SetWindowText(startButton, L"Stop");
                SetWindowText(pauseButton, L"Continue");
                EnableWindow(startButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_STOP);
                EnableWindow(pauseButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
            }
            break;
        case SERVICE_STOPPED:
            {
                SetWindowText(startButton, L"Start");
                SetWindowText(pauseButton, L"Pause");
                EnableWindow(startButton, TRUE);
                EnableWindow(pauseButton, FALSE);
            }
            break;
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_STOP_PENDING:
            {
                SetWindowText(startButton, L"Start");
                SetWindowText(pauseButton, L"Pause");
                EnableWindow(startButton, FALSE);
                EnableWindow(pauseButton, FALSE);
            }
            break;
        }

        if (serviceHandle = PhOpenService(
            ServiceItem->Name->Buffer,
            SERVICE_QUERY_CONFIG
            ))
        {
            if (description = PhGetServiceDescription(serviceHandle))
            {
                SetWindowText(descriptionLabel, description->Buffer);
                PhDereferenceObject(description);
            }

            CloseServiceHandle(serviceHandle);
        }
    }
    else
    {
        SetWindowText(startButton, L"Start");
        SetWindowText(pauseButton, L"Pause");
        EnableWindow(startButton, FALSE);
        EnableWindow(pauseButton, FALSE);
        SetWindowText(descriptionLabel, L"");
    }
}

INT_PTR CALLBACK PhpProcessServicesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_SERVICES_CONTEXT servicesContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        servicesContext = (PPH_SERVICES_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCSERVICES_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG i;

            servicesContext = propPageContext->Context =
                PhAllocate(sizeof(PH_SERVICES_CONTEXT));

            // Get a copy of the process' service list.

            PhAcquireFastLockShared(&processItem->ServiceListLock);

            servicesContext->NumberOfServices = processItem->ServiceList->Count;
            servicesContext->Services = PhAllocate(servicesContext->NumberOfServices * sizeof(PPH_SERVICE_ITEM));

            {
                ULONG enumerationKey = 0;
                PPH_SERVICE_ITEM serviceItem;

                i = 0;

                while (PhEnumPointerList(processItem->ServiceList, &enumerationKey, &serviceItem))
                {
                    PhReferenceObject(serviceItem);
                    servicesContext->Services[i++] = serviceItem;
                }
            }

            PhReleaseFastLockShared(&processItem->ServiceListLock);

            PhRegisterCallback(
                &PhServiceModifiedEvent,
                ServiceModifiedHandler,
                servicesContext,
                &servicesContext->ModifiedEventRegistration
                );

            servicesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 220, L"Display Name");

            PhSetExtendedListView(lvHandle);

            for (i = 0; i < servicesContext->NumberOfServices; i++)
            {
                PPH_SERVICE_ITEM serviceItem;
                INT lvItemIndex;

                serviceItem = servicesContext->Services[i];
                lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, serviceItem->Name->Buffer, serviceItem);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, serviceItem->DisplayName->Buffer);
            }

            ExtendedListView_SortItems(lvHandle);

            PhpFixProcessServicesControls(hwndDlg, NULL);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            for (i = 0; i < servicesContext->NumberOfServices; i++)
                PhDereferenceObject(servicesContext->Services[i]);

            PhFree(servicesContext->Services);

            PhUnregisterCallback(
                &PhServiceModifiedEvent,
                &servicesContext->ModifiedEventRegistration
                );

            PhFree(servicesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhpAddPropPageLayoutItem(hwndDlg, hwndDlg, NULL, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCSERVICES_LIST),
                    dialogItem, PH_ANCHOR_ALL);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DESCRIPTION),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_START),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhpAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PAUSE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

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
            case IDC_START:
                {
                    PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (serviceItem)
                    {
                        switch (serviceItem->State)
                        {
                        case SERVICE_RUNNING:
                            PhUiStopService(hwndDlg, serviceItem);
                            break;
                        case SERVICE_PAUSED:
                            PhUiStopService(hwndDlg, serviceItem);
                            break;
                        case SERVICE_STOPPED:
                            PhUiStartService(hwndDlg, serviceItem);
                            break;
                        }
                    }
                }
                break;
            case IDC_PAUSE:
                {
                    PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(lvHandle);

                    if (serviceItem)
                    {
                        switch (serviceItem->State)
                        {
                        case SERVICE_RUNNING:
                            PhUiPauseService(hwndDlg, serviceItem);
                            break;
                        case SERVICE_PAUSED:
                            PhUiContinueService(hwndDlg, serviceItem);
                            break;
                        }
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
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(lvHandle);

                        if (serviceItem)
                        {
                            PhShowServiceProperties(hwndDlg, serviceItem);
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        //LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                        PPH_SERVICE_ITEM serviceItem = NULL;

                        if (ListView_GetSelectedCount(lvHandle) == 1)
                            serviceItem = PhGetSelectedListViewItemParam(lvHandle);

                        PhpFixProcessServicesControls(hwndDlg, serviceItem);
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_SERVICE_MODIFIED:
        {
            PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)lParam;
            PPH_SERVICE_ITEM serviceItem = NULL;

            if (ListView_GetSelectedCount(lvHandle) == 1)
                serviceItem = PhGetSelectedListViewItemParam(lvHandle);

            if (serviceModifiedData->Service == serviceItem)
            {
                PhpFixProcessServicesControls(hwndDlg, serviceItem);
            }

            PhFree(serviceModifiedData);
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhpProcessPropertiesThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_PROCESS_PROPCONTEXT PropContext = (PPH_PROCESS_PROPCONTEXT)Parameter;
    PPH_PROCESS_PROPPAGECONTEXT newPage;
    PPH_STRING startPage;
    HWND hwnd;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

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

    // Token
    PhAddProcessPropPage2(
        PropContext,
        PhCreateTokenPage(PhpOpenProcessToken, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessTokenHookProc)
        );

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

    // Services
    if (PropContext->ProcessItem->ServiceList->Count != 0)
    {
        newPage = PhCreateProcessPropPageContext(
            MAKEINTRESOURCE(IDD_PROCSERVICES),
            PhpProcessServicesDlgProc,
            NULL
            );
        PhAddProcessPropPage(PropContext, newPage);
        PhDereferenceObject(newPage);
    }

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

        PhDrainAutoPool(&autoPool);

        // Destroy the window when necessary.
        if (!PropSheet_GetCurrentPageHwnd(hwnd))
        {
            DestroyWindow(hwnd);
            break;
        }
    }

    PhDereferenceObject(PropContext);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

BOOLEAN PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    )
{
    HANDLE threadHandle;

    PhReferenceObject(Context);
    threadHandle = PhCreateThread(0, PhpProcessPropertiesThreadStart, Context);

    if (threadHandle)
    {
        NtClose(threadHandle);
        return TRUE;
    }
    else
    {
        PhDereferenceObject(Context);
        return FALSE;
    }
}

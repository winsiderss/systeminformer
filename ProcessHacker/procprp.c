/*
 * Process Hacker -
 *   process properties
 *
 * Copyright (C) 2009-2015 wj32
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
#include <kphuser.h>
#include <settings.h>
#include <cpysave.h>
#include <emenu.h>
#include <phplug.h>
#include <extmgri.h>
#include <verify.h>
#include <procprpp.h>
#include <windowsx.h>

#define SET_BUTTON_BITMAP(Id, Bitmap) \
    SendMessage(GetDlgItem(hwndDlg, (Id)), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(Bitmap))

PPH_OBJECT_TYPE PhpProcessPropContextType;
PPH_OBJECT_TYPE PhpProcessPropPageContextType;

static RECT MinimumSize = { -1, -1, -1, -1 };
static PWSTR ProtectedSignerStrings[] = { L"", L" (Authenticode)", L" (CodeGen)", L" (Antimalware)", L" (Lsa)", L" (Windows)", L" (WinTcb)" };

static PH_STRINGREF LoadingText = PH_STRINGREF_INIT(L"Loading...");
static PH_STRINGREF EmptyThreadsText = PH_STRINGREF_INIT(L"There are no threads to display.");
static PH_STRINGREF EmptyModulesText = PH_STRINGREF_INIT(L"There are no modules to display.");
static PH_STRINGREF EmptyMemoryText = PH_STRINGREF_INIT(L"There are no memory regions to display.");
static PH_STRINGREF EmptyHandlesText = PH_STRINGREF_INIT(L"There are no handles to display.");

BOOLEAN PhProcessPropInitialization(
    VOID
    )
{
    PhpProcessPropContextType = PhCreateObjectType(L"ProcessPropContext", 0, PhpProcessPropContextDeleteProcedure);
    PhpProcessPropPageContextType = PhCreateObjectType(L"ProcessPropPageContext", 0, PhpProcessPropPageContextDeleteProcedure);

    return TRUE;
}

PPH_PROCESS_PROPCONTEXT PhCreateProcessPropContext(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_PROPCONTEXT propContext;
    PROPSHEETHEADER propSheetHeader;

    propContext = PhCreateObject(sizeof(PH_PROCESS_PROPCONTEXT), PhpProcessPropContextType);
    memset(propContext, 0, sizeof(PH_PROCESS_PROPCONTEXT));

    propContext->PropSheetPages = PhAllocate(sizeof(HPROPSHEETPAGE) * PH_PROCESS_PROPCONTEXT_MAXPAGES);

    if (!PH_IS_FAKE_PROCESS_ID(ProcessItem->ProcessId))
    {
        propContext->Title = PhFormatString(
            L"%s (%u)",
            ProcessItem->ProcessName->Buffer,
            HandleToUlong(ProcessItem->ProcessId)
            );
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
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.hIcon = ProcessItem->SmallIcon;
    propSheetHeader.pszCaption = propContext->Title->Buffer;
    propSheetHeader.pfnCallback = PhpPropSheetProc;

    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    if (PhCsForceNoParent)
        propSheetHeader.hwndParent = NULL;

    memcpy(&propContext->PropSheetHeader, &propSheetHeader, sizeof(PROPSHEETHEADER));

    PhSetReference(&propContext->ProcessItem, ProcessItem);
    PhInitializeEvent(&propContext->CreatedEvent);

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
    PropContext->PropSheetHeader.hIcon = PropContext->ProcessItem->SmallIcon;
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
            PPH_PROCESS_PROPSHEETCONTEXT propSheetContext;

            propSheetContext = PhAllocate(sizeof(PH_PROCESS_PROPSHEETCONTEXT));
            memset(propSheetContext, 0, sizeof(PH_PROCESS_PROPSHEETCONTEXT));

            PhInitializeLayoutManager(&propSheetContext->LayoutManager, hwndDlg);

            propSheetContext->OldWndProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PhpPropSheetWndProc);

            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)propSheetContext);

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
        }
        break;
    }

    return 0;
}

PPH_PROCESS_PROPSHEETCONTEXT PhpGetPropSheetContext(
    _In_ HWND hwnd
    )
{
    return (PPH_PROCESS_PROPSHEETCONTEXT)GetProp(hwnd, PhMakeContextAtom());
}

LRESULT CALLBACK PhpPropSheetWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_PROPSHEETCONTEXT propSheetContext = PhpGetPropSheetContext(hwnd);
    WNDPROC oldWndProc = propSheetContext->OldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            HWND tabControl;
            TCITEM tabItem;
            WCHAR text[128];

            // Save the window position and size.

            PhSaveWindowPlacementToSetting(L"ProcPropPosition", L"ProcPropSize", hwnd);

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
        break;
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhDeleteLayoutManager(&propSheetContext->LayoutManager);

            RemoveProp(hwnd, PhMakeContextAtom());
            PhFree(propSheetContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
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
            if (!IsIconic(hwnd))
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
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOLEAN PhpInitializePropSheetLayoutStage1(
    _In_ HWND hwnd
    )
{
    PPH_PROCESS_PROPSHEETCONTEXT propSheetContext = PhpGetPropSheetContext(hwnd);

    if (!propSheetContext->LayoutInitialized)
    {
        HWND tabControlHandle;
        PPH_LAYOUT_ITEM tabControlItem;
        PPH_LAYOUT_ITEM tabPageItem;

        tabControlHandle = PropSheet_GetTabControl(hwnd);
        tabControlItem = PhAddLayoutItem(&propSheetContext->LayoutManager, tabControlHandle,
            NULL, PH_ANCHOR_ALL | PH_LAYOUT_IMMEDIATE_RESIZE);
        tabPageItem = PhAddLayoutItem(&propSheetContext->LayoutManager, tabControlHandle,
            NULL, PH_LAYOUT_TAB_CONTROL); // dummy item to fix multiline tab control

        propSheetContext->TabPageItem = tabPageItem;

        PhAddLayoutItem(&propSheetContext->LayoutManager, GetDlgItem(hwnd, IDCANCEL),
            NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

        // Hide the OK button.
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        SetDlgItemText(hwnd, IDCANCEL, L"Close");

        propSheetContext->LayoutInitialized = TRUE;

        return TRUE;
    }

    return FALSE;
}

VOID PhpInitializePropSheetLayoutStage2(
    _In_ HWND hwnd
    )
{
    PH_RECTANGLE windowRectangle;

    windowRectangle.Position = PhGetIntegerPairSetting(L"ProcPropPosition");
    windowRectangle.Size = PhGetIntegerPairSetting(L"ProcPropSize");

    if (windowRectangle.Size.X < MinimumSize.right)
        windowRectangle.Size.X = MinimumSize.right;
    if (windowRectangle.Size.Y < MinimumSize.bottom)
        windowRectangle.Size.Y = MinimumSize.bottom;

    PhAdjustRectangleToWorkingArea(hwnd, &windowRectangle);

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

    PropPageContext->PropContext = PropContext;
    PhReferenceObject(PropContext);

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] =
        propSheetPageHandle;
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

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] =
        PropSheetPageHandle;
    PropContext->PropSheetHeader.nPages++;

    return TRUE;
}

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    return PhCreateProcessPropPageContextEx(NULL, Template, DlgProc, Context);
}

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    propPageContext = PhCreateObject(sizeof(PH_PROCESS_PROPPAGECONTEXT), PhpProcessPropPageContextType);
    memset(propPageContext, 0, sizeof(PH_PROCESS_PROPPAGECONTEXT));

    propPageContext->PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propPageContext->PropSheetPage.dwFlags =
        PSP_USECALLBACK;
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

FORCEINLINE BOOLEAN PhpPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_ PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    _Out_ PPH_PROCESS_ITEM *ProcessItem
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    if (uMsg == WM_INITDIALOG)
    {
        // Save the context.
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)lParam);
    }

    propSheetPage = (LPPROPSHEETPAGE)GetProp(hwndDlg, PhMakeContextAtom());

    if (!propSheetPage)
        return FALSE;

    *PropSheetPage = propSheetPage;
    *PropPageContext = propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam;
    *ProcessItem = propPageContext->PropContext->ProcessItem;

    return TRUE;
}

BOOLEAN PhPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_ PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    _Out_ PPH_PROCESS_ITEM *ProcessItem
    )
{
    return PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam, PropSheetPage, PropPageContext, ProcessItem);
}

VOID PhpPropPageDlgProcDestroy(
    _In_ HWND hwndDlg
    )
{
    RemoveProp(hwndDlg, PhMakeContextAtom());
}

VOID PhPropPageDlgProcDestroy(
    _In_ HWND hwndDlg
    )
{
    PhpPropPageDlgProcDestroy(hwndDlg);
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

    doLayoutStage2 = PhpInitializePropSheetLayoutStage1(parent);

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

NTSTATUS PhpProcessGeneralOpenProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return PhOpenProcess(Handle, DesiredAccess, (HANDLE)Context);
}

FORCEINLINE PWSTR PhpGetStringOrNa(
    _In_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"N/A";
}

VOID PhpUpdateProcessDep(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    HANDLE processHandle;
    ULONG depStatus;

#ifdef _WIN64
    if (ProcessItem->IsWow64)
#else
    if (TRUE)
#endif
    {
        SetDlgItemText(hwndDlg, IDC_DEP, L"N/A");

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessItem->ProcessId
            )))
        {
            if (NT_SUCCESS(PhGetProcessDepStatus(processHandle, &depStatus)))
            {
                PPH_STRING depString;

                if (depStatus & PH_PROCESS_DEP_ENABLED)
                    depString = PhaCreateString(L"Enabled");
                else
                    depString = PhaCreateString(L"Disabled");

                if ((depStatus & PH_PROCESS_DEP_ENABLED) &&
                    (depStatus & PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED))
                {
                    depString = PhaConcatStrings2(depString->Buffer, L", DEP-ATL thunk emulation disabled");
                }

                if (depStatus & PH_PROCESS_DEP_PERMANENT)
                {
                    depString = PhaConcatStrings2(depString->Buffer, L", Permanent");
                }

                SetDlgItemText(hwndDlg, IDC_DEP, depString->Buffer);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EDITDEP), !(depStatus & PH_PROCESS_DEP_PERMANENT));
            }

            NtClose(processHandle);
        }
    }
    else
    {
        if (ProcessItem->QueryHandle)
            SetDlgItemText(hwndDlg, IDC_DEP, L"Enabled, Permanent");
        else
            SetDlgItemText(hwndDlg, IDC_DEP, L"N/A");
    }
}

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
#ifdef _WIN64
            PVOID peb32;
#endif
            PPH_PROCESS_ITEM parentProcess;
            CLIENT_ID clientId;

            {
                HBITMAP folder;
                HBITMAP magnifier;
                HBITMAP pencil;

                folder = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_FOLDER), IMAGE_BITMAP);
                magnifier = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_MAGNIFIER), IMAGE_BITMAP);
                pencil = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_PENCIL), IMAGE_BITMAP);

                SET_BUTTON_BITMAP(IDC_INSPECT, magnifier);
                SET_BUTTON_BITMAP(IDC_OPENFILENAME, folder);
                SET_BUTTON_BITMAP(IDC_VIEWPARENTPROCESS, magnifier);
                SET_BUTTON_BITMAP(IDC_EDITDEP, pencil);
            }

            // File

            SendMessage(GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON,
                (WPARAM)processItem->LargeIcon, 0);

            if (PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            {
                SetDlgItemText(hwndDlg, IDC_NAME, PhpGetStringOrNa(processItem->VersionInfo.FileDescription));
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_NAME, processItem->ProcessName->Buffer);
            }

            SetDlgItemText(hwndDlg, IDC_VERSION, PhpGetStringOrNa(processItem->VersionInfo.FileVersion));
            SetDlgItemText(hwndDlg, IDC_FILENAME, PhpGetStringOrNa(processItem->FileName));

            if (!processItem->FileName)
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPENFILENAME), FALSE);

            {
                PPH_STRING inspectExecutables;

                inspectExecutables = PhGetStringSetting(L"ProgramInspectExecutables");

                if (!processItem->FileName || inspectExecutables->Length == 0)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INSPECT), FALSE);
                }

                PhDereferenceObject(inspectExecutables);
            }

            if (processItem->VerifyResult == VrTrusted)
            {
                if (processItem->VerifySignerName)
                {
                    SetDlgItemText(hwndDlg, IDC_COMPANYNAME_LINK,
                        PhaFormatString(L"<a>(Verified) %s</a>", processItem->VerifySignerName->Buffer)->Buffer);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME_LINK), SW_SHOW);
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
            else if (processItem->VerifyResult != VrUnknown)
            {
                SetDlgItemText(hwndDlg, IDC_COMPANYNAME,
                    PhaConcatStrings2(
                    L"(UNVERIFIED) ",
                    PhGetStringOrEmpty(processItem->VersionInfo.CompanyName)
                    )->Buffer);
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
                PH_PEB_OFFSET pebOffset;

                pebOffset = PhpoCurrentDirectory;

#ifdef _WIN64
                // Tell the function to get the WOW64 current directory, because that's
                // the one that actually gets updated.
                if (processItem->IsWow64)
                    pebOffset |= PhpoWow64;
#endif

                PhGetProcessPebString(
                    processHandle,
                    pebOffset,
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

            if (processItem->CreateTime.QuadPart != 0)
            {
                LARGE_INTEGER startTime;
                LARGE_INTEGER currentTime;
                SYSTEMTIME startTimeFields;
                PPH_STRING startTimeRelativeString;
                PPH_STRING startTimeString;

                startTime = processItem->CreateTime;
                PhQuerySystemTime(&currentTime);
                startTimeRelativeString = PhAutoDereferenceObject(PhFormatTimeSpanRelative(currentTime.QuadPart - startTime.QuadPart));

                PhLargeIntegerToLocalSystemTime(&startTimeFields, &startTime);
                startTimeString = PhaFormatDateTime(&startTimeFields);

                SetDlgItemText(hwndDlg, IDC_STARTED,
                    PhaFormatString(L"%s ago (%s)", startTimeRelativeString->Buffer, startTimeString->Buffer)->Buffer);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_STARTED, L"N/A");
            }

            // Parent

            if (parentProcess = PhReferenceProcessItemForParent(
                processItem->ParentProcessId,
                processItem->ProcessId,
                &processItem->CreateTime
                ))
            {
                clientId.UniqueProcess = parentProcess->ProcessId;
                clientId.UniqueThread = NULL;

                SetDlgItemText(hwndDlg, IDC_PARENTPROCESS,
                    ((PPH_STRING)PhAutoDereferenceObject(PhGetClientIdNameEx(&clientId, parentProcess->ProcessName)))->Buffer);

                PhDereferenceObject(parentProcess);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_PARENTPROCESS,
                    PhaFormatString(L"Non-existent process (%u)", HandleToUlong(processItem->ParentProcessId))->Buffer);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VIEWPARENTPROCESS), FALSE);
            }

            // DEP

            PhpUpdateProcessDep(hwndDlg, processItem);

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
#ifdef _WIN64
                if (processItem->IsWow64)
                {
                    PhGetProcessPeb32(processHandle, &peb32);
                    SetDlgItemText(hwndDlg, IDC_PEBADDRESS,
                        PhaFormatString(L"0x%Ix (32-bit: 0x%x)", basicInfo.PebBaseAddress, PtrToUlong(peb32))->Buffer);
                }
                else
                {
#endif

                SetDlgItemText(hwndDlg, IDC_PEBADDRESS,
                    PhaFormatString(L"0x%Ix", basicInfo.PebBaseAddress)->Buffer);
#ifdef _WIN64
                }
#endif
            }

            // Protection

            SetDlgItemText(hwndDlg, IDC_PROTECTION, L"N/A");

            if (WINDOWS_HAS_LIMITED_ACCESS && processHandle)
            {
                if (WindowsVersion >= WINDOWS_8_1)
                {
                    PS_PROTECTION protection;

                    if (NT_SUCCESS(NtQueryInformationProcess(
                        processHandle,
                        ProcessProtectionInformation,
                        &protection,
                        sizeof(PS_PROTECTION),
                        NULL
                        )))
                    {
                        PWSTR type;
                        PWSTR signer;

                        switch (protection.Type)
                        {
                        case PsProtectedTypeNone:
                            type = L"None";
                            break;
                        case PsProtectedTypeProtectedLight:
                            type = L"Light";
                            break;
                        case PsProtectedTypeProtected:
                            type = L"Full";
                            break;
                        default:
                            type = L"Unknown";
                            break;
                        }

                        if (protection.Signer < sizeof(ProtectedSignerStrings) / sizeof(PWSTR))
                            signer = ProtectedSignerStrings[protection.Signer];
                        else
                            signer = L"";

                        SetDlgItemText(hwndDlg, IDC_PROTECTION, PhaConcatStrings2(type, signer)->Buffer);
                    }
                }
                else if (KphIsConnected())
                {
                    KPH_PROCESS_PROTECTION_INFORMATION protectionInfo;

                    if (NT_SUCCESS(KphQueryInformationProcess(
                        processHandle,
                        KphProcessProtectionInformation,
                        &protectionInfo,
                        sizeof(KPH_PROCESS_PROTECTION_INFORMATION),
                        NULL
                        )))
                    {
                        SetDlgItemText(hwndDlg, IDC_PROTECTION, protectionInfo.IsProtectedProcess ? L"Yes" : L"None");
                        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITPROTECTION), TRUE);
                    }
                }
                else
                {
                    PROCESS_EXTENDED_BASIC_INFORMATION extendedBasicInfo;

                    if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(
                        processHandle,
                        &extendedBasicInfo
                        )))
                    {
                        SetDlgItemText(hwndDlg, IDC_PROTECTION, extendedBasicInfo.IsProtectedProcess ? L"Yes" : L"None");
                    }
                }
            }

            if (processHandle)
                NtClose(processHandle);

            if (WindowsVersion >= WINDOWS_8)
            {
                PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

                SetDlgItemText(hwndDlg, IDC_ASLR, L"N/A");

                policyInfo.Policy = ProcessASLRPolicy;

                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    PROCESS_QUERY_INFORMATION,
                    processItem->ProcessId
                    )))
                {
                    if (NT_SUCCESS(NtQueryInformationProcess(
                        processHandle,
                        ProcessMitigationPolicy,
                        &policyInfo,
                        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
                        NULL
                        )))
                    {
                        PH_STRING_BUILDER sb;

                        PhInitializeStringBuilder(&sb, 40);

                        if (policyInfo.ASLRPolicy.EnableBottomUpRandomization)
                            PhAppendStringBuilder2(&sb, L"Bottom up randomization, ");
                        if (policyInfo.ASLRPolicy.EnableForceRelocateImages)
                            PhAppendStringBuilder2(&sb, L"Force relocate images, ");
                        if (policyInfo.ASLRPolicy.EnableHighEntropy)
                            PhAppendStringBuilder2(&sb, L"High entropy, ");
                        if (policyInfo.ASLRPolicy.DisallowStrippedImages)
                            PhAppendStringBuilder2(&sb, L"Disallow stripped images, ");
                        if (sb.String->Length != 0)
                            PhRemoveEndStringBuilder(&sb, 2);

                        if (sb.String->Length == 0)
                            SetDlgItemText(hwndDlg, IDC_ASLR, L"Disabled");
                        else
                            SetDlgItemText(hwndDlg, IDC_ASLR, sb.String->Buffer);

                        PhDeleteStringBuilder(&sb);
                    }

                    NtClose(processHandle);
                }
            }
            else
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_ASLRLABEL), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_ASLR), SW_HIDE);
            }

#ifdef _WIN64
            if (processItem->IsWow64Valid)
            {
                if (processItem->IsWow64)
                    SetDlgItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"32-bit");
                else
                    SetDlgItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"64-bit");
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"N/A");
            }

            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESSTYPELABEL), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESSTYPETEXT), SW_SHOW);
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

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMPANYNAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMPANYNAME_LINK),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VERSION),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILENAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INSPECT),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_OPENFILENAME),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CMDLINE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CURDIR),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STARTED),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PEBADDRESS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PARENTPROCESS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIEWPARENTPROCESS),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DEP),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_EDITDEP),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ASLR),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TERMINATE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PERMISSIONS),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESS),
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDC_INSPECT:
                {
                    if (processItem->FileName)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"ProgramInspectExecutables",
                            processItem->FileName->Buffer,
                            FALSE,
                            L"Make sure the PE Viewer executable file is present."
                            );
                    }
                }
                break;
            case IDC_OPENFILENAME:
                {
                    if (processItem->FileName)
                        PhShellExploreFile(hwndDlg, processItem->FileName->Buffer);
                }
                break;
            case IDC_VIEWPARENTPROCESS:
                {
                    PPH_PROCESS_ITEM parentProcessItem;

                    if (parentProcessItem = PhReferenceProcessItem(processItem->ParentProcessId))
                    {
                        ProcessHacker_ShowProcessProperties(PhMainWndHandle, parentProcessItem);
                        PhDereferenceObject(parentProcessItem);
                    }
                    else
                    {
                        PhShowError(hwndDlg, L"The process does not exist.");
                    }
                }
                break;
            case IDC_EDITDEP:
                {
                    if (PhUiSetDepStatusProcess(hwndDlg, processItem))
                        PhpUpdateProcessDep(hwndDlg, processItem);
                }
                break;
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_COMPANYNAME_LINK:
                        {
                            if (processItem->FileName)
                            {
                                PH_VERIFY_FILE_INFO info;

                                memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
                                info.FileName = processItem->FileName->Buffer;
                                info.Flags = PH_VERIFY_VIEW_PROPERTIES;
                                info.hWnd = hwndDlg;
                                PhVerifyFileWithAdditionalCatalog(
                                    &info,
                                    PhGetString(processItem->PackageFullName),
                                    NULL
                                    );
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI StatisticsUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STATISTICS_CONTEXT statisticsContext = (PPH_STATISTICS_CONTEXT)Context;

    if (statisticsContext->Enabled)
        PostMessage(statisticsContext->WindowHandle, WM_PH_STATISTICS_UPDATE, 0, 0);
}

VOID PhpUpdateProcessStatistics(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1];

    SetDlgItemInt(hwndDlg, IDC_ZPRIORITY_V, ProcessItem->BasePriority, TRUE); // priority
    PhPrintTimeSpan(timeSpan, ProcessItem->KernelTime.QuadPart, PH_TIMESPAN_HMSM); // kernel time
    SetDlgItemText(hwndDlg, IDC_ZKERNELTIME_V, timeSpan);
    PhPrintTimeSpan(timeSpan, ProcessItem->UserTime.QuadPart, PH_TIMESPAN_HMSM); // user time
    SetDlgItemText(hwndDlg, IDC_ZUSERTIME_V, timeSpan);
    PhPrintTimeSpan(timeSpan,
        ProcessItem->KernelTime.QuadPart + ProcessItem->UserTime.QuadPart, PH_TIMESPAN_HMSM); // total time
    SetDlgItemText(hwndDlg, IDC_ZTOTALTIME_V, timeSpan);

    SetDlgItemText(hwndDlg, IDC_ZPRIVATEBYTES_V,
        PhaFormatSize(ProcessItem->VmCounters.PagefileUsage, -1)->Buffer); // private bytes (same as PrivateUsage)
    SetDlgItemText(hwndDlg, IDC_ZPEAKPRIVATEBYTES_V,
        PhaFormatSize(ProcessItem->VmCounters.PeakPagefileUsage, -1)->Buffer); // peak private bytes
    SetDlgItemText(hwndDlg, IDC_ZVIRTUALSIZE_V,
        PhaFormatSize(ProcessItem->VmCounters.VirtualSize, -1)->Buffer); // virtual size
    SetDlgItemText(hwndDlg, IDC_ZPEAKVIRTUALSIZE_V,
        PhaFormatSize(ProcessItem->VmCounters.PeakVirtualSize, -1)->Buffer); // peak virtual size
    SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTS_V,
        PhaFormatUInt64(ProcessItem->VmCounters.PageFaultCount, TRUE)->Buffer); // page faults
    SetDlgItemText(hwndDlg, IDC_ZWORKINGSET_V,
        PhaFormatSize(ProcessItem->VmCounters.WorkingSetSize, -1)->Buffer); // working set
    SetDlgItemText(hwndDlg, IDC_ZPEAKWORKINGSET_V,
        PhaFormatSize(ProcessItem->VmCounters.PeakWorkingSetSize, -1)->Buffer); // peak working set

    SetDlgItemText(hwndDlg, IDC_ZIOREADS_V,
        PhaFormatUInt64(ProcessItem->IoCounters.ReadOperationCount, TRUE)->Buffer); // reads
    SetDlgItemText(hwndDlg, IDC_ZIOREADBYTES_V,
        PhaFormatSize(ProcessItem->IoCounters.ReadTransferCount, -1)->Buffer); // read bytes
    SetDlgItemText(hwndDlg, IDC_ZIOWRITES_V,
        PhaFormatUInt64(ProcessItem->IoCounters.WriteOperationCount, TRUE)->Buffer); // writes
    SetDlgItemText(hwndDlg, IDC_ZIOWRITEBYTES_V,
        PhaFormatSize(ProcessItem->IoCounters.WriteTransferCount, -1)->Buffer); // write bytes
    SetDlgItemText(hwndDlg, IDC_ZIOOTHER_V,
        PhaFormatUInt64(ProcessItem->IoCounters.OtherOperationCount, TRUE)->Buffer); // other
    SetDlgItemText(hwndDlg, IDC_ZIOOTHERBYTES_V,
        PhaFormatSize(ProcessItem->IoCounters.OtherTransferCount, -1)->Buffer); // read bytes

    SetDlgItemText(hwndDlg, IDC_ZHANDLES_V,
        PhaFormatUInt64(ProcessItem->NumberOfHandles, TRUE)->Buffer); // handles

    // Optional information
    if (!PH_IS_FAKE_PROCESS_ID(ProcessItem->ProcessId))
    {
        PPH_STRING peakHandles = NULL;
        PPH_STRING gdiHandles = NULL;
        PPH_STRING userHandles = NULL;
        PPH_STRING cycles = NULL;
        ULONG pagePriority = -1;
        ULONG ioPriority = -1;
        PPH_STRING privateWs = NULL;
        PPH_STRING shareableWs = NULL;
        PPH_STRING sharedWs = NULL;
        BOOLEAN gotCycles = FALSE;
        BOOLEAN gotWsCounters = FALSE;

        if (ProcessItem->QueryHandle)
        {
            ULONG64 cycleTime;

            if (WindowsVersion >= WINDOWS_7)
            {
                PROCESS_HANDLE_INFORMATION handleInfo;

                if (NT_SUCCESS(NtQueryInformationProcess(
                    ProcessItem->QueryHandle,
                    ProcessHandleCount,
                    &handleInfo,
                    sizeof(PROCESS_HANDLE_INFORMATION),
                    NULL
                    )))
                {
                    peakHandles = PhaFormatUInt64(handleInfo.HandleCountHighWatermark, TRUE);
                }
            }

            gdiHandles = PhaFormatUInt64(GetGuiResources(ProcessItem->QueryHandle, GR_GDIOBJECTS), TRUE); // GDI handles
            userHandles = PhaFormatUInt64(GetGuiResources(ProcessItem->QueryHandle, GR_USEROBJECTS), TRUE); // USER handles

            if (WINDOWS_HAS_CYCLE_TIME &&
                NT_SUCCESS(PhGetProcessCycleTime(ProcessItem->QueryHandle, &cycleTime)))
            {
                cycles = PhaFormatUInt64(cycleTime, TRUE);
                gotCycles = TRUE;
            }

            if (WindowsVersion >= WINDOWS_VISTA)
            {
                PhGetProcessPagePriority(ProcessItem->QueryHandle, &pagePriority);
                PhGetProcessIoPriority(ProcessItem->QueryHandle, &ioPriority);
            }
        }

        if (Context->ProcessHandle)
        {
            PH_PROCESS_WS_COUNTERS wsCounters;

            if (NT_SUCCESS(PhGetProcessWsCounters(Context->ProcessHandle, &wsCounters)))
            {
                privateWs = PhaFormatSize((ULONG64)wsCounters.NumberOfPrivatePages * PAGE_SIZE, -1);
                shareableWs = PhaFormatSize((ULONG64)wsCounters.NumberOfShareablePages * PAGE_SIZE, -1);
                sharedWs = PhaFormatSize((ULONG64)wsCounters.NumberOfSharedPages * PAGE_SIZE, -1);
                gotWsCounters = TRUE;
            }
        }

        if (WindowsVersion >= WINDOWS_7)
        {
            if (!gotCycles)
                cycles = PhaFormatUInt64(ProcessItem->CycleTimeDelta.Value, TRUE);
            if (!gotWsCounters)
                privateWs = PhaFormatSize(ProcessItem->WorkingSetPrivateSize, -1);
        }

        if (WindowsVersion >= WINDOWS_7)
            SetDlgItemText(hwndDlg, IDC_ZPEAKHANDLES_V, PhGetStringOrDefault(peakHandles, L"Unknown"));
        else
            SetDlgItemText(hwndDlg, IDC_ZPEAKHANDLES_V, L"N/A");

        SetDlgItemText(hwndDlg, IDC_ZGDIHANDLES_V, PhGetStringOrDefault(gdiHandles, L"Unknown"));
        SetDlgItemText(hwndDlg, IDC_ZUSERHANDLES_V, PhGetStringOrDefault(userHandles, L"Unknown"));
        SetDlgItemText(hwndDlg, IDC_ZCYCLES_V,
            PhGetStringOrDefault(cycles, WINDOWS_HAS_CYCLE_TIME ? L"Unknown" : L"N/A"));

        if (WindowsVersion >= WINDOWS_VISTA)
        {
            if (pagePriority != -1)
                SetDlgItemInt(hwndDlg, IDC_ZPAGEPRIORITY_V, pagePriority, FALSE);
            else
                SetDlgItemText(hwndDlg, IDC_ZPAGEPRIORITY_V, L"Unknown");

            if (ioPriority != -1 && ioPriority < MaxIoPriorityTypes)
                SetDlgItemText(hwndDlg, IDC_ZIOPRIORITY_V, PhIoPriorityHintNames[ioPriority]);
            else
                SetDlgItemText(hwndDlg, IDC_ZIOPRIORITY_V, L"Unknown");
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_ZPAGEPRIORITY_V, L"N/A");
            SetDlgItemText(hwndDlg, IDC_ZIOPRIORITY_V, L"N/A");
        }

        SetDlgItemText(hwndDlg, IDC_ZPRIVATEWS_V, PhGetStringOrDefault(privateWs, L"Unknown"));
        SetDlgItemText(hwndDlg, IDC_ZSHAREABLEWS_V, PhGetStringOrDefault(shareableWs, L"Unknown"));
        SetDlgItemText(hwndDlg, IDC_ZSHAREDWS_V, PhGetStringOrDefault(sharedWs, L"Unknown"));
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_ZPEAKHANDLES_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZGDIHANDLES_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZUSERHANDLES_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZCYCLES_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZPAGEPRIORITY_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZIOPRIORITY_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZPRIVATEWS_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZSHAREABLEWS_V, L"N/A");
        SetDlgItemText(hwndDlg, IDC_ZSHAREDWS_V, L"N/A");
    }
}

INT_PTR CALLBACK PhpProcessStatisticsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_STATISTICS_CONTEXT statisticsContext;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        statisticsContext = (PPH_STATISTICS_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            statisticsContext = propPageContext->Context =
                PhAllocate(sizeof(PH_STATISTICS_CONTEXT));

            statisticsContext->WindowHandle = hwndDlg;
            statisticsContext->Enabled = TRUE;
            statisticsContext->ProcessHandle = NULL;

            // Try to open a process handle with PROCESS_QUERY_INFORMATION access for
            // WS information.
            PhOpenProcess(
                &statisticsContext->ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                processItem->ProcessId
                );

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                StatisticsUpdateHandler,
                statisticsContext,
                &statisticsContext->ProcessesUpdatedRegistration
                );

            PhpUpdateProcessStatistics(hwndDlg, processItem, statisticsContext);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &statisticsContext->ProcessesUpdatedRegistration
                );

            if (statisticsContext->ProcessHandle)
                NtClose(statisticsContext->ProcessHandle);

            PhFree(statisticsContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_DETAILS:
                {
                    PhShowHandleStatisticsDialog(hwndDlg, processItem->ProcessId);
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
                statisticsContext->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                statisticsContext->Enabled = FALSE;
                break;
            }
        }
        break;
    case WM_PH_STATISTICS_UPDATE:
        {
            PhpUpdateProcessStatistics(hwndDlg, processItem, statisticsContext);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI PerformanceUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PERFORMANCE_CONTEXT performanceContext = (PPH_PERFORMANCE_CONTEXT)Context;

    PostMessage(performanceContext->WindowHandle, WM_PH_PERFORMANCE_UPDATE, 0, 0);
}

INT_PTR CALLBACK PhpProcessPerformanceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_PERFORMANCE_CONTEXT performanceContext;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        performanceContext = (PPH_PERFORMANCE_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            performanceContext = propPageContext->Context =
                PhAllocate(sizeof(PH_PERFORMANCE_CONTEXT));

            performanceContext->WindowHandle = hwndDlg;

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                PerformanceUpdateHandler,
                performanceContext,
                &performanceContext->ProcessesUpdatedRegistration
                );

            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            PhInitializeGraphState(&performanceContext->CpuGraphState);
            PhInitializeGraphState(&performanceContext->PrivateGraphState);
            PhInitializeGraphState(&performanceContext->IoGraphState);

            performanceContext->CpuGraphHandle = GetDlgItem(hwndDlg, IDC_CPU);
            PhSetWindowStyle(performanceContext->CpuGraphHandle, WS_BORDER, WS_BORDER);
            Graph_SetTooltip(performanceContext->CpuGraphHandle, TRUE);
            BringWindowToTop(performanceContext->CpuGraphHandle);

            performanceContext->PrivateGraphHandle = GetDlgItem(hwndDlg, IDC_PRIVATEBYTES);
            PhSetWindowStyle(performanceContext->PrivateGraphHandle, WS_BORDER, WS_BORDER);
            Graph_SetTooltip(performanceContext->PrivateGraphHandle, TRUE);
            BringWindowToTop(performanceContext->PrivateGraphHandle);

            performanceContext->IoGraphHandle = GetDlgItem(hwndDlg, IDC_IO);
            PhSetWindowStyle(performanceContext->IoGraphHandle, WS_BORDER, WS_BORDER);
            Graph_SetTooltip(performanceContext->IoGraphHandle, TRUE);
            BringWindowToTop(performanceContext->IoGraphHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteGraphState(&performanceContext->CpuGraphState);
            PhDeleteGraphState(&performanceContext->PrivateGraphState);
            PhDeleteGraphState(&performanceContext->IoGraphState);

            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &performanceContext->ProcessesUpdatedRegistration
                );
            PhFree(performanceContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == performanceContext->CpuGraphHandle)
                    {
                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&performanceContext->CpuGraphState.Text,
                                PhFormatString(L"%.2f%%",
                                (processItem->CpuKernelUsage + processItem->CpuUserUsage) * 100
                                ));

                            hdc = Graph_GetBufferedContext(performanceContext->CpuGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &performanceContext->CpuGraphState.Text->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->CpuGraphState,
                            getDrawInfo,
                            processItem->CpuKernelHistory.Count
                            );

                        if (!performanceContext->CpuGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(&processItem->CpuKernelHistory,
                                performanceContext->CpuGraphState.Data1, drawInfo->LineDataCount);
                            PhCopyCircularBuffer_FLOAT(&processItem->CpuUserHistory,
                                performanceContext->CpuGraphState.Data2, drawInfo->LineDataCount);
                            performanceContext->CpuGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == performanceContext->PrivateGraphHandle)
                    {
                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&performanceContext->PrivateGraphState.Text,
                                PhConcatStrings2(
                                L"Private Bytes: ",
                                PhaFormatSize(processItem->VmCounters.PagefileUsage, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(performanceContext->PrivateGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &performanceContext->PrivateGraphState.Text->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPrivate, 0);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->PrivateGraphState,
                            getDrawInfo,
                            processItem->PrivateBytesHistory.Count
                            );

                        if (!performanceContext->PrivateGraphState.Valid)
                        {
                            ULONG i;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                performanceContext->PrivateGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, i);
                            }

                            if (processItem->VmCounters.PeakPagefileUsage != 0)
                            {
                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    performanceContext->PrivateGraphState.Data1,
                                    (FLOAT)processItem->VmCounters.PeakPagefileUsage,
                                    drawInfo->LineDataCount
                                    );
                            }

                            performanceContext->PrivateGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == performanceContext->IoGraphHandle)
                    {
                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&performanceContext->IoGraphState.Text,
                                PhFormatString(
                                L"R+O: %s, W: %s",
                                PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer,
                                PhaFormatSize(processItem->IoWriteDelta.Delta, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(performanceContext->IoGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &performanceContext->IoGraphState.Text->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->IoGraphState,
                            getDrawInfo,
                            processItem->IoReadHistory.Count
                            );

                        if (!performanceContext->IoGraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = 0;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                performanceContext->IoGraphState.Data1[i] = data1 =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, i) +
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, i);
                                performanceContext->IoGraphState.Data2[i] = data2 =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            if (max != 0)
                            {
                                // Scale the data.

                                PhDivideSinglesBySingle(
                                    performanceContext->IoGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                                PhDivideSinglesBySingle(
                                    performanceContext->IoGraphState.Data2,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            performanceContext->IoGraphState.Valid = TRUE;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (
                        header->hwndFrom == performanceContext->CpuGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (performanceContext->CpuGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            FLOAT cpuKernel;
                            FLOAT cpuUser;

                            cpuKernel = PhGetItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, getTooltipText->Index);
                            cpuUser = PhGetItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, getTooltipText->Index);

                            PhMoveReference(&performanceContext->CpuGraphState.TooltipText, PhFormatString(
                                L"%.2f%%\n%s",
                                (cpuKernel + cpuUser) * 100,
                                ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(processItem, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = performanceContext->CpuGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == performanceContext->PrivateGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (performanceContext->PrivateGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            SIZE_T privateBytes;

                            privateBytes = PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, getTooltipText->Index);

                            PhMoveReference(&performanceContext->PrivateGraphState.TooltipText, PhFormatString(
                                L"Private Bytes: %s\n%s",
                                PhaFormatSize(privateBytes, -1)->Buffer,
                                ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(processItem, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = performanceContext->PrivateGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == performanceContext->IoGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (performanceContext->IoGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            ULONG64 ioRead;
                            ULONG64 ioWrite;
                            ULONG64 ioOther;

                            ioRead = PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, getTooltipText->Index);
                            ioWrite = PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, getTooltipText->Index);
                            ioOther = PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, getTooltipText->Index);

                            PhMoveReference(&performanceContext->IoGraphState.TooltipText, PhFormatString(
                                L"R: %s\nW: %s\nO: %s\n%s",
                                PhaFormatSize(ioRead, -1)->Buffer,
                                PhaFormatSize(ioWrite, -1)->Buffer,
                                PhaFormatSize(ioOther, -1)->Buffer,
                                ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(processItem, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = performanceContext->IoGraphState.TooltipText->sr;
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            HDWP deferHandle;
            HWND cpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPCPU);
            HWND privateBytesGroupBox = GetDlgItem(hwndDlg, IDC_GROUPPRIVATEBYTES);
            HWND ioGroupBox = GetDlgItem(hwndDlg, IDC_GROUPIO);
            RECT clientRect;
            RECT margin = { 13, 13, 13, 13 };
            RECT innerMargin = { 10, 20, 10, 10 };
            LONG between = 3;
            LONG width;
            LONG height;

            performanceContext->CpuGraphState.Valid = FALSE;
            performanceContext->CpuGraphState.TooltipIndex = -1;
            performanceContext->PrivateGraphState.Valid = FALSE;
            performanceContext->PrivateGraphState.TooltipIndex = -1;
            performanceContext->IoGraphState.Valid = FALSE;
            performanceContext->IoGraphState.TooltipIndex = -1;

            GetClientRect(hwndDlg, &clientRect);
            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 3;

            deferHandle = BeginDeferWindowPos(6);

            deferHandle = DeferWindowPos(deferHandle, cpuGroupBox, NULL, margin.left, margin.top,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                performanceContext->CpuGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, privateBytesGroupBox, NULL, margin.left, margin.top + height + between,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                performanceContext->PrivateGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + height + between + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, ioGroupBox, NULL, margin.left, margin.top + (height + between) * 2,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                performanceContext->IoGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + (height + between) * 2 + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_PH_PERFORMANCE_UPDATE:
        {
            if (!(processItem->State & PH_PROCESS_ITEM_REMOVED))
            {
                performanceContext->CpuGraphState.Valid = FALSE;
                Graph_MoveGrid(performanceContext->CpuGraphHandle, 1);
                Graph_Draw(performanceContext->CpuGraphHandle);
                Graph_UpdateTooltip(performanceContext->CpuGraphHandle);
                InvalidateRect(performanceContext->CpuGraphHandle, NULL, FALSE);

                performanceContext->PrivateGraphState.Valid = FALSE;
                Graph_MoveGrid(performanceContext->PrivateGraphHandle, 1);
                Graph_Draw(performanceContext->PrivateGraphHandle);
                Graph_UpdateTooltip(performanceContext->PrivateGraphHandle);
                InvalidateRect(performanceContext->PrivateGraphHandle, NULL, FALSE);

                performanceContext->IoGraphState.Valid = FALSE;
                Graph_MoveGrid(performanceContext->IoGraphHandle, 1);
                Graph_Draw(performanceContext->IoGraphHandle);
                Graph_UpdateTooltip(performanceContext->IoGraphHandle);
                InvalidateRect(performanceContext->IoGraphHandle, NULL, FALSE);
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI ThreadAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    // Parameter contains a pointer to the added thread item.
    PhReferenceObject(Parameter);
    PostMessage(
        threadsContext->WindowHandle,
        WM_PH_THREAD_ADDED,
        threadsContext->Provider->RunId == 1,
        (LPARAM)Parameter
        );
}

static VOID NTAPI ThreadModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ThreadRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_REMOVED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ThreadsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREADS_UPDATED, 0, 0);
}

static VOID NTAPI ThreadsLoadingStateChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(
        threadsContext->ListContext.TreeNewHandle,
        TNM_SETCURSOR,
        0,
        // Parameter contains TRUE if loading symbols
        (LPARAM)(Parameter ? LoadCursor(NULL, IDC_APPSTARTING) : NULL)
        );
}

VOID PhpInitializeThreadMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfThreads == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
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
            ID_THREAD_RESUME,
            ID_THREAD_COPY
        };
        ULONG i;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        // These menu items are capable of manipulating
        // multiple threads.
        for (i = 0; i < sizeof(menuItemsMultiEnabled) / sizeof(ULONG); i++)
        {
            PhEnableEMenuItem(Menu, menuItemsMultiEnabled[i], TRUE);
        }
    }

    // Remove irrelevant menu items.

    if (WindowsVersion < WINDOWS_VISTA)
    {
        // Remove I/O priority.
        if (item = PhFindEMenuItem(Menu, 0, L"I/O Priority", 0))
            PhDestroyEMenuItem(item);
        // Remove page priority.
        if (item = PhFindEMenuItem(Menu, 0, L"Page Priority", 0))
            PhDestroyEMenuItem(item);
    }

#ifndef _WIN64
    if (!KphIsConnected())
    {
#endif
        // Remove Force Terminate (this is always done on x64).
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_THREAD_FORCETERMINATE))
            PhDestroyEMenuItem(item);
#ifndef _WIN64
    }
    else
    {
        if (ProcessId == SYSTEM_PROCESS_ID)
        {
            // Remove Force Terminate because Terminate does
            // the same job.
            if (item = PhFindEMenuItem(Menu, 0, NULL, ID_THREAD_FORCETERMINATE))
                PhDestroyEMenuItem(item);
        }
    }
#endif

    PhEnableEMenuItem(Menu, ID_THREAD_TOKEN, FALSE);

    // Priority
    if (NumberOfThreads == 1)
    {
        HANDLE threadHandle;
        ULONG ioPriority = -1;
        ULONG pagePriority = -1;
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

                if (!NT_SUCCESS(PhGetThreadPagePriority(
                    threadHandle,
                    &pagePriority
                    )))
                {
                    pagePriority = -1;
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
                    PhEnableEMenuItem(Menu, ID_THREAD_TOKEN, TRUE);
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
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
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
                PhSetFlagsEMenuItem(Menu, id,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
            }
        }

        if (pagePriority != -1)
        {
            id = 0;

            switch (pagePriority)
            {
            case 1:
                id = ID_PAGEPRIORITY_1;
                break;
            case 2:
                id = ID_PAGEPRIORITY_2;
                break;
            case 3:
                id = ID_PAGEPRIORITY_3;
                break;
            case 4:
                id = ID_PAGEPRIORITY_4;
                break;
            case 5:
                id = ID_PAGEPRIORITY_5;
                break;
            }

            if (id != 0)
            {
                PhSetFlagsEMenuItem(Menu, id,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
            }
        }
    }
}

static NTSTATUS NTAPI PhpThreadPermissionsOpenThread(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return PhOpenThread(Handle, DesiredAccess, (HANDLE)Context);
}

static NTSTATUS NTAPI PhpOpenThreadTokenObject(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return PhOpenThreadToken(
        Handle,
        DesiredAccess,
        (HANDLE)Context,
        TRUE
        );
}

VOID PhpUpdateThreadDetails(
    _In_ HWND hwndDlg,
    _In_ PPH_THREADS_CONTEXT Context,
    _In_ BOOLEAN Force
    )
{
    PPH_THREAD_ITEM *threads;
    ULONG numberOfThreads;
    PPH_THREAD_ITEM threadItem;
    PPH_STRING startModule = NULL;
    PPH_STRING started = NULL;
    WCHAR kernelTime[PH_TIMESPAN_STR_LEN_1] = L"N/A";
    WCHAR userTime[PH_TIMESPAN_STR_LEN_1] = L"N/A";
    PPH_STRING contextSwitches = NULL;
    PPH_STRING cycles = NULL;
    PPH_STRING state = NULL;
    WCHAR priority[PH_INT32_STR_LEN_1] = L"N/A";
    WCHAR basePriority[PH_INT32_STR_LEN_1] = L"N/A";
    PWSTR ioPriority = L"N/A";
    WCHAR pagePriority[PH_INT32_STR_LEN_1] = L"N/A";
    WCHAR idealProcessor[PH_INT32_STR_LEN + 1 + PH_INT32_STR_LEN + 1] = L"N/A";
    HANDLE threadHandle;
    SYSTEMTIME time;
    ULONG ioPriorityInteger;
    ULONG pagePriorityInteger;
    PROCESSOR_NUMBER idealProcessorNumber;
    ULONG suspendCount;

    PhGetSelectedThreadItems(&Context->ListContext, &threads, &numberOfThreads);

    if (numberOfThreads == 1)
        threadItem = threads[0];
    else
        threadItem = NULL;

    PhFree(threads);

    if (numberOfThreads != 1 && !Force)
        return;

    if (numberOfThreads == 1)
    {
        startModule = threadItem->StartAddressFileName;

        PhLargeIntegerToLocalSystemTime(&time, &threadItem->CreateTime);
        started = PhaFormatDateTime(&time);

        PhPrintTimeSpan(kernelTime, threadItem->KernelTime.QuadPart, PH_TIMESPAN_HMSM);
        PhPrintTimeSpan(userTime, threadItem->UserTime.QuadPart, PH_TIMESPAN_HMSM);

        contextSwitches = PhaFormatUInt64(threadItem->ContextSwitchesDelta.Value, TRUE);

        if (WINDOWS_HAS_CYCLE_TIME)
            cycles = PhaFormatUInt64(threadItem->CyclesDelta.Value, TRUE);

        if (threadItem->State != Waiting)
        {
            if ((ULONG)threadItem->State < MaximumThreadState)
                state = PhaCreateString(PhKThreadStateNames[(ULONG)threadItem->State]);
            else
                state = PhaCreateString(L"Unknown");
        }
        else
        {
            if ((ULONG)threadItem->WaitReason < MaximumWaitReason)
                state = PhaConcatStrings2(L"Wait:", PhKWaitReasonNames[(ULONG)threadItem->WaitReason]);
            else
                state = PhaCreateString(L"Waiting");
        }

        PhPrintInt32(priority, threadItem->Priority);
        PhPrintInt32(basePriority, threadItem->BasePriority);

        if (NT_SUCCESS(PhOpenThread(&threadHandle, ThreadQueryAccess, threadItem->ThreadId)))
        {
            if (NT_SUCCESS(PhGetThreadIoPriority(threadHandle, &ioPriorityInteger)) &&
                ioPriorityInteger < MaxIoPriorityTypes)
            {
                ioPriority = PhIoPriorityHintNames[ioPriorityInteger];
            }

            if (NT_SUCCESS(PhGetThreadPagePriority(threadHandle, &pagePriorityInteger)))
            {
                PhPrintUInt32(pagePriority, pagePriorityInteger);
            }

            if (NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadIdealProcessorEx, &idealProcessorNumber, sizeof(PROCESSOR_NUMBER), NULL)))
            {
                PH_FORMAT format[3];

                PhInitFormatU(&format[0], idealProcessorNumber.Group);
                PhInitFormatC(&format[1], ':');
                PhInitFormatU(&format[2], idealProcessorNumber.Number);
                PhFormatToBuffer(format, 3, idealProcessor, sizeof(idealProcessor), NULL);
            }

            if (threadItem->WaitReason == Suspended && NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadSuspendCount, &suspendCount, sizeof(ULONG), NULL)))
            {
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], state->sr);
                PhInitFormatS(&format[1], L" (");
                PhInitFormatU(&format[2], suspendCount);
                PhInitFormatS(&format[3], L")");
                state = PhAutoDereferenceObject(PhFormat(format, 4, 30));
            }

            NtClose(threadHandle);
        }
    }

    if (Force)
    {
        // These don't change...

        SetDlgItemText(hwndDlg, IDC_STARTMODULE, PhGetStringOrEmpty(startModule));
        EnableWindow(GetDlgItem(hwndDlg, IDC_OPENSTARTMODULE), !!startModule);

        SetDlgItemText(hwndDlg, IDC_STARTED, PhGetStringOrDefault(started, L"N/A"));
    }

    SetDlgItemText(hwndDlg, IDC_KERNELTIME, kernelTime);
    SetDlgItemText(hwndDlg, IDC_USERTIME, userTime);
    SetDlgItemText(hwndDlg, IDC_CONTEXTSWITCHES, PhGetStringOrDefault(contextSwitches, L"N/A"));
    SetDlgItemText(hwndDlg, IDC_CYCLES, PhGetStringOrDefault(cycles, L"N/A"));
    SetDlgItemText(hwndDlg, IDC_STATE, PhGetStringOrDefault(state, L"N/A"));
    SetDlgItemText(hwndDlg, IDC_PRIORITY, priority);
    SetDlgItemText(hwndDlg, IDC_BASEPRIORITY, basePriority);
    SetDlgItemText(hwndDlg, IDC_IOPRIORITY, ioPriority);
    SetDlgItemText(hwndDlg, IDC_PAGEPRIORITY, pagePriority);
    SetDlgItemText(hwndDlg, IDC_IDEALPROCESSOR, idealProcessor);
}

VOID PhShowThreadContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_THREADS_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_THREAD_ITEM *threads;
    ULONG numberOfThreads;

    PhGetSelectedThreadItems(&Context->ListContext, &threads, &numberOfThreads);

    if (numberOfThreads != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_THREAD), 0);
        PhSetFlagsEMenuItem(menu, ID_THREAD_INSPECT, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeThreadMenu(menu, ProcessItem->ProcessId, threads, numberOfThreads);
        PhInsertCopyCellEMenuItem(menu, ID_THREAD_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Thread.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Thread.Threads = threads;
            menuInfo.u.Thread.NumberOfThreads = numberOfThreads;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(threads);
}

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_THREADS_CONTEXT threadsContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        threadsContext = (PPH_THREADS_CONTEXT)propPageContext->Context;

        if (threadsContext)
            tnHandle = threadsContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            threadsContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmThreadsContextType, sizeof(PH_THREADS_CONTEXT)));

            // The thread provider has a special registration mechanism.
            threadsContext->Provider = PhCreateThreadProvider(
                processItem->ProcessId
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

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeThreadList(hwndDlg, tnHandle, &threadsContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &EmptyThreadsText, 0);
            threadsContext->NeedsRedraw = FALSE;

            // Use Cycles instead of Context Switches on Vista and above, but only when we can
            // open the process, since cycle time information requires sufficient access to the
            // threads.
            if (WINDOWS_HAS_CYCLE_TIME)
            {
                HANDLE processHandle;
                PROCESS_EXTENDED_BASIC_INFORMATION extendedBasicInfo;

                // We make a distinction between PROCESS_QUERY_INFORMATION and PROCESS_QUERY_LIMITED_INFORMATION since
                // the latter can be used when opening audiodg.exe even though we can't access its threads using
                // THREAD_QUERY_LIMITED_INFORMATION.

                if (processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;
                }
                else if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId)))
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;
                    NtClose(processHandle);
                }
                else if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, processItem->ProcessId)))
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;

                    // We can't use cycle time for protected processes (without KProcessHacker).
                    if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &extendedBasicInfo)) && extendedBasicInfo.IsProtectedProcess)
                    {
                        threadsContext->ListContext.UseCycleTime = FALSE;
                    }

                    NtClose(processHandle);
                }
            }

            if (processItem->ServiceList && processItem->ServiceList->Count != 0 && WINDOWS_HAS_SERVICE_TAGS)
                threadsContext->ListContext.HasServices = TRUE;

            PhEmCallObjectOperation(EmThreadsContextType, threadsContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &threadsContext->ListContext.Cm;
                treeNewInfo.SystemContext = threadsContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsThreadList(&threadsContext->ListContext);

            PhThreadProviderInitialUpdate(threadsContext->Provider);
            PhRegisterThreadProvider(threadsContext->Provider, &threadsContext->ProviderRegistration);

            SET_BUTTON_BITMAP(IDC_OPENSTARTMODULE,
                PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_FOLDER), IMAGE_BITMAP));
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmThreadsContextType, threadsContext, EmObjectDelete);

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
            PhUnregisterThreadProvider(threadsContext->Provider, &threadsContext->ProviderRegistration);
            PhSetTerminatingThreadProvider(threadsContext->Provider);
            PhDereferenceObject(threadsContext->Provider);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &threadsContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsThreadList(&threadsContext->ListContext);
            PhDeleteThreadList(&threadsContext->ListContext);

            PhFree(threadsContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

#define ADD_BL_ITEM(Id) \
    PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, Id), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM)

                // Thread details area
                {
                    ULONG id;

                    for (id = IDC_STATICBL1; id <= IDC_STATICBL11; id++)
                        ADD_BL_ITEM(id);

                    // Not in sequence
                    ADD_BL_ITEM(IDC_STATICBL12);
                }

                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STARTMODULE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_OPENSTARTMODULE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                ADD_BL_ITEM(IDC_STARTED);
                ADD_BL_ITEM(IDC_KERNELTIME);
                ADD_BL_ITEM(IDC_USERTIME);
                ADD_BL_ITEM(IDC_CONTEXTSWITCHES);
                ADD_BL_ITEM(IDC_CYCLES);
                ADD_BL_ITEM(IDC_STATE);
                ADD_BL_ITEM(IDC_PRIORITY);
                ADD_BL_ITEM(IDC_BASEPRIORITY);
                ADD_BL_ITEM(IDC_IOPRIORITY);
                ADD_BL_ITEM(IDC_PAGEPRIORITY);
                ADD_BL_ITEM(IDC_IDEALPROCESSOR);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowThreadContextMenu(hwndDlg, processItem, threadsContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_THREAD_INSPECT:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadsContext->Provider);
                        PhShowThreadStackDialog(
                            hwndDlg,
                            threadsContext->Provider->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider
                            );
                        PhDereferenceObject(threadsContext->Provider);
                    }
                }
                break;
            case ID_THREAD_TERMINATE:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);

                    if (
                        processItem->ProcessId != SYSTEM_PROCESS_ID ||
                        !KphIsConnected()
                        )
                    {
                        if (PhUiTerminateThreads(hwndDlg, threads, numberOfThreads))
                            PhDeselectAllThreadNodes(&threadsContext->ListContext);
                    }
                    else
                    {
                        if (PhUiForceTerminateThreads(hwndDlg, processItem->ProcessId, threads, numberOfThreads))
                            PhDeselectAllThreadNodes(&threadsContext->ListContext);
                    }

                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_FORCETERMINATE:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);

                    if (PhUiForceTerminateThreads(hwndDlg, processItem->ProcessId, threads, numberOfThreads))
                        PhDeselectAllThreadNodes(&threadsContext->ListContext);

                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_SUSPEND:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);
                    PhUiSuspendThreads(hwndDlg, threads, numberOfThreads);
                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_RESUME:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);
                    PhUiResumeThreads(hwndDlg, threads, numberOfThreads);
                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_AFFINITY:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadItem);
                        PhShowProcessAffinityDialog(hwndDlg, NULL, threadItem);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_THREAD_PERMISSIONS:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);
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
                                PhaFormatString(L"Thread %u", HandleToUlong(threadItem->ThreadId))->Buffer,
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
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);
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
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadsContext->Provider->SymbolProvider);
                        PhUiAnalyzeWaitThread(
                            hwndDlg,
                            processItem->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider->SymbolProvider
                            );
                        PhDereferenceObject(threadsContext->Provider->SymbolProvider);
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
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

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

                        PhReferenceObject(threadItem);
                        PhUiSetPriorityThread(hwndDlg, threadItem, threadPriorityWin32);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_I_0:
            case ID_I_1:
            case ID_I_2:
            case ID_I_3:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

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

                        PhReferenceObject(threadItem);
                        PhUiSetIoPriorityThread(hwndDlg, threadItem, ioPriority);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_PAGEPRIORITY_1:
            case ID_PAGEPRIORITY_2:
            case ID_PAGEPRIORITY_3:
            case ID_PAGEPRIORITY_4:
            case ID_PAGEPRIORITY_5:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        ULONG pagePriority;

                        switch (id)
                        {
                            case ID_PAGEPRIORITY_1:
                                pagePriority = 1;
                                break;
                            case ID_PAGEPRIORITY_2:
                                pagePriority = 2;
                                break;
                            case ID_PAGEPRIORITY_3:
                                pagePriority = 3;
                                break;
                            case ID_PAGEPRIORITY_4:
                                pagePriority = 4;
                                break;
                            case ID_PAGEPRIORITY_5:
                                pagePriority = 5;
                                break;
                        }

                        PhReferenceObject(threadItem);
                        PhUiSetPagePriorityThread(hwndDlg, threadItem, pagePriority);
                        PhDereferenceObject(threadItem);
                    }
                }
                break;
            case ID_THREAD_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_OPENSTARTMODULE:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem && threadItem->StartAddressFileName)
                    {
                        PhShellExploreFile(hwndDlg, threadItem->StartAddressFileName->Buffer);
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
                break;
            case PSN_KILLACTIVE:
                // Can't disable, it screws up the deltas.
                break;
            }
        }
        break;
    case WM_PH_THREAD_ADDED:
        {
            BOOLEAN firstRun = (BOOLEAN)wParam;
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            if (!threadsContext->NeedsRedraw)
            {
                // Disable redraw. It will be re-enabled later.
                TreeNew_SetRedraw(tnHandle, FALSE);
                threadsContext->NeedsRedraw = TRUE;
            }

            PhAddThreadNode(&threadsContext->ListContext, threadItem, firstRun);
            PhDereferenceObject(threadItem);
        }
        break;
    case WM_PH_THREAD_MODIFIED:
        {
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            if (!threadsContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                threadsContext->NeedsRedraw = TRUE;
            }

            PhUpdateThreadNode(&threadsContext->ListContext, PhFindThreadNode(&threadsContext->ListContext, threadItem->ThreadId));
        }
        break;
    case WM_PH_THREAD_REMOVED:
        {
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            if (!threadsContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                threadsContext->NeedsRedraw = TRUE;
            }

            PhRemoveThreadNode(&threadsContext->ListContext, PhFindThreadNode(&threadsContext->ListContext, threadItem->ThreadId));
        }
        break;
    case WM_PH_THREADS_UPDATED:
        {
            PhTickThreadNodes(&threadsContext->ListContext);

            if (threadsContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, TRUE);
                threadsContext->NeedsRedraw = FALSE;
            }

            if (propPageContext->PropContext->SelectThreadId)
            {
                PPH_THREAD_NODE threadNode;

                if (threadNode = PhFindThreadNode(&threadsContext->ListContext, propPageContext->PropContext->SelectThreadId))
                {
                    if (threadNode->Node.Visible)
                    {
                        TreeNew_SetFocusNode(tnHandle, &threadNode->Node);
                        TreeNew_SetMarkNode(tnHandle, &threadNode->Node);
                        TreeNew_SelectRange(tnHandle, threadNode->Node.Index, threadNode->Node.Index);
                        TreeNew_EnsureVisible(tnHandle, &threadNode->Node);
                    }
                }

                propPageContext->PropContext->SelectThreadId = NULL;
            }

            PhpUpdateThreadDetails(hwndDlg, threadsContext, FALSE);
        }
        break;
    case WM_PH_THREAD_SELECTION_CHANGED:
        {
            PhpUpdateThreadDetails(hwndDlg, threadsContext, TRUE);
        }
        break;
    }

    return FALSE;
}

static NTSTATUS NTAPI PhpOpenProcessToken(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!GetProp(hwndDlg, PhMakeContextAtom())) // LayoutInitialized
            {
                PPH_LAYOUT_ITEM dialogItem;
                HWND groupsLv;
                HWND privilegesLv;

                // This is a big violation of abstraction...

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_USER),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_USERSID),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIRTUALIZED),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_APPCONTAINERSID),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_GROUPS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PRIVILEGES),
                    dialogItem, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INSTRUCTION),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INTEGRITY),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADVANCED),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                PhDoPropPageLayout(hwndDlg);

                groupsLv = GetDlgItem(hwndDlg, IDC_GROUPS);
                privilegesLv = GetDlgItem(hwndDlg, IDC_PRIVILEGES);

                if (ListView_GetItemCount(groupsLv) != 0)
                {
                    ListView_SetColumnWidth(groupsLv, 0, LVSCW_AUTOSIZE);
                    ExtendedListView_SetColumnWidth(groupsLv, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);
                }

                if (ListView_GetItemCount(privilegesLv) != 0)
                {
                    ListView_SetColumnWidth(privilegesLv, 0, LVSCW_AUTOSIZE);
                    ListView_SetColumnWidth(privilegesLv, 1, LVSCW_AUTOSIZE);
                    ExtendedListView_SetColumnWidth(privilegesLv, 2, ELVSCW_AUTOSIZE_REMAININGSPACE);
                }

                SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)TRUE);
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI ModuleAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    // Parameter contains a pointer to the added module item.
    PhReferenceObject(Parameter);
    PostMessage(
        modulesContext->WindowHandle,
        WM_PH_MODULE_ADDED,
        PhGetRunIdProvider(&modulesContext->ProviderRegistration),
        (LPARAM)Parameter
        );
}

static VOID NTAPI ModuleModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULE_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ModuleRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULE_REMOVED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ModulesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULES_UPDATED, 0, 0);
}

VOID PhpInitializeModuleMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_MODULE_ITEM *Modules,
    _In_ ULONG NumberOfModules
    )
{
    PPH_EMENU_ITEM item;
    PPH_STRING inspectExecutables;

    inspectExecutables = PhGetStringSetting(L"ProgramInspectExecutables");

    if (inspectExecutables->Length == 0)
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_MODULE_INSPECT))
            PhDestroyEMenuItem(item);
    }

    PhDereferenceObject(inspectExecutables);

    if (NumberOfModules == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfModules == 1)
    {
        // Nothing
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_MODULE_COPY, TRUE);
    }
}

VOID PhShowModuleContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_MODULES_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_MODULE_ITEM *modules;
    ULONG numberOfModules;

    PhGetSelectedModuleItems(&Context->ListContext, &modules, &numberOfModules);

    if (numberOfModules != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_MODULE), 0);
        PhSetFlagsEMenuItem(menu, ID_MODULE_INSPECT, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeModuleMenu(menu, ProcessItem->ProcessId, modules, numberOfModules);
        PhInsertCopyCellEMenuItem(menu, ID_MODULE_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Module.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Module.Modules = modules;
            menuInfo.u.Module.NumberOfModules = numberOfModules;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(modules);
}

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_MODULES_CONTEXT modulesContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        modulesContext = (PPH_MODULES_CONTEXT)propPageContext->Context;

        if (modulesContext)
            tnHandle = modulesContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Lots of boilerplate code...

            modulesContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmModulesContextType, sizeof(PH_MODULES_CONTEXT)));

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
                &modulesContext->Provider->ModuleModifiedEvent,
                ModuleModifiedHandler,
                modulesContext,
                &modulesContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                ModuleRemovedHandler,
                modulesContext,
                &modulesContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->UpdatedEvent,
                ModulesUpdatedHandler,
                modulesContext,
                &modulesContext->UpdatedEventRegistration
                );
            modulesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeModuleList(hwndDlg, tnHandle, &modulesContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &LoadingText, 0);
            modulesContext->NeedsRedraw = FALSE;
            modulesContext->LastRunStatus = -1;
            modulesContext->ErrorMessage = NULL;

            PhEmCallObjectOperation(EmModulesContextType, modulesContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &modulesContext->ListContext.Cm;
                treeNewInfo.SystemContext = modulesContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackModuleTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsModuleList(&modulesContext->ListContext);

            PhSetEnabledProvider(&modulesContext->ProviderRegistration, TRUE);
            PhBoostProvider(&modulesContext->ProviderRegistration, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmModulesContextType, modulesContext, EmObjectDelete);

            PhUnregisterCallback(
                &modulesContext->Provider->ModuleAddedEvent,
                &modulesContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleModifiedEvent,
                &modulesContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                &modulesContext->RemovedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->UpdatedEvent,
                &modulesContext->UpdatedEventRegistration
                );
            PhUnregisterProvider(&modulesContext->ProviderRegistration);
            PhDereferenceObject(modulesContext->Provider);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &modulesContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackModuleTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsModuleList(&modulesContext->ListContext);
            PhDeleteModuleList(&modulesContext->ListContext);

            PhClearReference(&modulesContext->ErrorMessage);
            PhFree(modulesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, modulesContext->ListContext.TreeNewHandle,
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowModuleContextMenu(hwndDlg, processItem, modulesContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_MODULE_UNLOAD:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhReferenceObject(moduleItem);

                        if (PhUiUnloadModule(hwndDlg, processItem->ProcessId, moduleItem))
                            PhDeselectAllModuleNodes(&modulesContext->ListContext);

                        PhDereferenceObject(moduleItem);
                    }
                }
                break;
            case ID_MODULE_INSPECT:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"ProgramInspectExecutables",
                            moduleItem->FileName->Buffer,
                            FALSE,
                            L"Make sure the PE Viewer executable file is present."
                            );
                    }
                }
                break;
            case ID_MODULE_SEARCHONLINE:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhSearchOnlineString(hwndDlg, moduleItem->Name->Buffer);
                    }
                }
                break;
            case ID_MODULE_OPENFILELOCATION:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhShellExploreFile(hwndDlg, moduleItem->FileName->Buffer);
                    }
                }
                break;
            case ID_MODULE_PROPERTIES:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhShellProperties(hwndDlg, moduleItem->FileName->Buffer);
                    }
                }
                break;
            case ID_MODULE_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
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
                PhSetEnabledProvider(&modulesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetEnabledProvider(&modulesContext->ProviderRegistration, FALSE);
                break;
            }
        }
        break;
    case WM_PH_MODULE_ADDED:
        {
            ULONG runId = (ULONG)wParam;
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            if (!modulesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                modulesContext->NeedsRedraw = TRUE;
            }

            PhAddModuleNode(&modulesContext->ListContext, moduleItem, runId);
            PhDereferenceObject(moduleItem);
        }
        break;
    case WM_PH_MODULE_MODIFIED:
        {
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            if (!modulesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                modulesContext->NeedsRedraw = TRUE;
            }

            PhUpdateModuleNode(&modulesContext->ListContext, PhFindModuleNode(&modulesContext->ListContext, moduleItem));
        }
        break;
    case WM_PH_MODULE_REMOVED:
        {
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            if (!modulesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                modulesContext->NeedsRedraw = TRUE;
            }

            PhRemoveModuleNode(&modulesContext->ListContext, PhFindModuleNode(&modulesContext->ListContext, moduleItem));
        }
        break;
    case WM_PH_MODULES_UPDATED:
        {
            PhTickModuleNodes(&modulesContext->ListContext);

            if (modulesContext->LastRunStatus != modulesContext->Provider->RunStatus)
            {
                NTSTATUS status;
                PPH_STRING message;

                status = modulesContext->Provider->RunStatus;
                modulesContext->LastRunStatus = status;

                if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
                    status = STATUS_SUCCESS;

                if (NT_SUCCESS(status))
                {
                    TreeNew_SetEmptyText(tnHandle, &EmptyModulesText, 0);
                }
                else
                {
                    message = PhGetStatusMessage(status, 0);
                    PhMoveReference(&modulesContext->ErrorMessage, PhFormatString(L"Unable to query module information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
                    PhClearReference(&message);
                    TreeNew_SetEmptyText(tnHandle, &modulesContext->ErrorMessage->sr, 0);
                }

                InvalidateRect(tnHandle, NULL, FALSE);
            }

            if (modulesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, TRUE);
                modulesContext->NeedsRedraw = FALSE;
            }
        }
        break;
    case WM_CTLCOLORDLG:
        {
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

VOID PhpRefreshProcessMemoryList(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PPH_MEMORY_CONTEXT memoryContext = PropPageContext->Context;

    if (memoryContext->MemoryItemListValid)
    {
        PhDeleteMemoryItemList(&memoryContext->MemoryItemList);
        memoryContext->MemoryItemListValid = FALSE;
    }

    memoryContext->LastRunStatus = PhQueryMemoryItemList(
        memoryContext->ProcessId,
        PH_QUERY_MEMORY_REGION_TYPE | PH_QUERY_MEMORY_WS_COUNTERS,
        &memoryContext->MemoryItemList
        );

    if (NT_SUCCESS(memoryContext->LastRunStatus))
    {
        if (PhPluginsEnabled)
        {
            PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL control;

            control.Type = PluginMemoryItemListInitialized;
            control.u.Initialized.List = &memoryContext->MemoryItemList;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryItemListControl), &control);
        }

        memoryContext->MemoryItemListValid = TRUE;
        TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &EmptyMemoryText, 0);
        PhReplaceMemoryList(&memoryContext->ListContext, &memoryContext->MemoryItemList);
    }
    else
    {
        PPH_STRING message;

        message = PhGetStatusMessage(memoryContext->LastRunStatus, 0);
        PhMoveReference(&memoryContext->ErrorMessage, PhFormatString(L"Unable to query memory information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
        PhClearReference(&message);
        TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &memoryContext->ErrorMessage->sr, 0);

        PhReplaceMemoryList(&memoryContext->ListContext, NULL);
    }
}

VOID PhpInitializeMemoryMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_NODE *MemoryNodes,
    _In_ ULONG NumberOfMemoryNodes
    )
{
    if (NumberOfMemoryNodes == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfMemoryNodes == 1 && !MemoryNodes[0]->IsAllocationBase)
    {
        if (MemoryNodes[0]->MemoryItem->State & MEM_FREE)
        {
            PhEnableEMenuItem(Menu, ID_MEMORY_CHANGEPROTECTION, FALSE);
            PhEnableEMenuItem(Menu, ID_MEMORY_FREE, FALSE);
            PhEnableEMenuItem(Menu, ID_MEMORY_DECOMMIT, FALSE);
        }
        else if (MemoryNodes[0]->MemoryItem->Type & (MEM_MAPPED | MEM_IMAGE))
        {
            PhEnableEMenuItem(Menu, ID_MEMORY_DECOMMIT, FALSE);
        }
    }
    else
    {
        ULONG i;
        ULONG numberOfAllocationBase = 0;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_MEMORY_COPY, TRUE);

        for (i = 0; i < NumberOfMemoryNodes; i++)
        {
            if (MemoryNodes[i]->IsAllocationBase)
                numberOfAllocationBase++;
        }

        if (numberOfAllocationBase == 0 || numberOfAllocationBase == NumberOfMemoryNodes)
            PhEnableEMenuItem(Menu, ID_MEMORY_SAVE, TRUE);
    }

    PhEnableEMenuItem(Menu, ID_MEMORY_READWRITEADDRESS, TRUE);
}

VOID PhShowMemoryContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_MEMORY_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_MEMORY_NODE *memoryNodes;
    ULONG numberOfMemoryNodes;

    PhGetSelectedMemoryNodes(&Context->ListContext, &memoryNodes, &numberOfMemoryNodes);

    //if (numberOfMemoryNodes != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_MEMORY), 0);
        PhSetFlagsEMenuItem(menu, ID_MEMORY_READWRITEMEMORY, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeMemoryMenu(menu, ProcessItem->ProcessId, memoryNodes, numberOfMemoryNodes);
        PhInsertCopyCellEMenuItem(menu, ID_MEMORY_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Memory.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Memory.MemoryNodes = memoryNodes;
            menuInfo.u.Memory.NumberOfMemoryNodes = numberOfMemoryNodes;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(memoryNodes);
}

INT_PTR CALLBACK PhpProcessMemoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_MEMORY_CONTEXT memoryContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        memoryContext = (PPH_MEMORY_CONTEXT)propPageContext->Context;

        if (memoryContext)
            tnHandle = memoryContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            memoryContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmMemoryContextType, sizeof(PH_MEMORY_CONTEXT)));
            memset(memoryContext, 0, sizeof(PH_MEMORY_CONTEXT));
            memoryContext->ProcessId = processItem->ProcessId;

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeMemoryList(hwndDlg, tnHandle, &memoryContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &LoadingText, 0);
            memoryContext->LastRunStatus = -1;
            memoryContext->ErrorMessage = NULL;

            PhEmCallObjectOperation(EmMemoryContextType, memoryContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &memoryContext->ListContext.Cm;
                treeNewInfo.SystemContext = memoryContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsMemoryList(&memoryContext->ListContext);
            PhSetOptionsMemoryList(&memoryContext->ListContext, TRUE);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_HIDEFREEREGIONS),
                memoryContext->ListContext.HideFreeRegions ? BST_CHECKED : BST_UNCHECKED);

            PhpRefreshProcessMemoryList(hwndDlg, propPageContext);
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmMemoryContextType, memoryContext, EmObjectDelete);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &memoryContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsMemoryList(&memoryContext->ListContext);
            PhDeleteMemoryList(&memoryContext->ListContext);

            if (memoryContext->MemoryItemListValid)
                PhDeleteMemoryItemList(&memoryContext->MemoryItemList);

            PhClearReference(&memoryContext->ErrorMessage);
            PhFree(memoryContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STRINGS),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_REFRESH),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, memoryContext->ListContext.TreeNewHandle,
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowMemoryContextMenu(hwndDlg, processItem, memoryContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_MEMORY_READWRITEMEMORY:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode && !memoryNode->IsAllocationBase)
                    {
                        if (memoryNode->MemoryItem->State & MEM_COMMIT)
                        {
                            PPH_SHOWMEMORYEDITOR showMemoryEditor = PhAllocate(sizeof(PH_SHOWMEMORYEDITOR));

                            memset(showMemoryEditor, 0, sizeof(PH_SHOWMEMORYEDITOR));
                            showMemoryEditor->ProcessId = processItem->ProcessId;
                            showMemoryEditor->BaseAddress = memoryNode->MemoryItem->BaseAddress;
                            showMemoryEditor->RegionSize = memoryNode->MemoryItem->RegionSize;
                            showMemoryEditor->SelectOffset = -1;
                            showMemoryEditor->SelectLength = 0;
                            ProcessHacker_ShowMemoryEditor(PhMainWndHandle, showMemoryEditor);
                        }
                        else
                        {
                            PhShowError(hwndDlg, L"Unable to edit the memory region because it is not committed.");
                        }
                    }
                }
                break;
            case ID_MEMORY_SAVE:
                {
                    NTSTATUS status;
                    HANDLE processHandle;
                    PPH_MEMORY_NODE *memoryNodes;
                    ULONG numberOfMemoryNodes;

                    if (!NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_VM_READ,
                        processItem->ProcessId
                        )))
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the process", status, 0);
                        break;
                    }

                    PhGetSelectedMemoryNodes(&memoryContext->ListContext, &memoryNodes, &numberOfMemoryNodes);

                    if (numberOfMemoryNodes != 0)
                    {
                        static PH_FILETYPE_FILTER filters[] =
                        {
                            { L"Binary files (*.bin)", L"*.bin" },
                            { L"All files (*.*)", L"*.*" }
                        };
                        PVOID fileDialog;

                        fileDialog = PhCreateSaveFileDialog();

                        PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                        PhSetFileDialogFileName(fileDialog, PhaConcatStrings2(processItem->ProcessName->Buffer, L".bin")->Buffer);

                        if (PhShowFileDialog(hwndDlg, fileDialog))
                        {
                            PPH_STRING fileName;
                            PPH_FILE_STREAM fileStream;
                            PVOID buffer;
                            ULONG i;
                            ULONG_PTR offset;

                            fileName = PhGetFileDialogFileName(fileDialog);
                            PhAutoDereferenceObject(fileName);

                            if (NT_SUCCESS(status = PhCreateFileStream(
                                &fileStream,
                                fileName->Buffer,
                                FILE_GENERIC_WRITE,
                                FILE_SHARE_READ,
                                FILE_OVERWRITE_IF,
                                0
                                )))
                            {
                                buffer = PhAllocatePage(PAGE_SIZE, NULL);

                                // Go through each selected memory item and append the region contents
                                // to the file.
                                for (i = 0; i < numberOfMemoryNodes; i++)
                                {
                                    PPH_MEMORY_NODE memoryNode = memoryNodes[i];
                                    PPH_MEMORY_ITEM memoryItem = memoryNode->MemoryItem;

                                    if (!memoryNode->IsAllocationBase && !(memoryItem->State & MEM_COMMIT))
                                        continue;

                                    for (offset = 0; offset < memoryItem->RegionSize; offset += PAGE_SIZE)
                                    {
                                        if (NT_SUCCESS(PhReadVirtualMemory(
                                            processHandle,
                                            PTR_ADD_OFFSET(memoryItem->BaseAddress, offset),
                                            buffer,
                                            PAGE_SIZE,
                                            NULL
                                            )))
                                        {
                                            PhWriteFileStream(fileStream, buffer, PAGE_SIZE);
                                        }
                                    }
                                }

                                PhFreePage(buffer);

                                PhDereferenceObject(fileStream);
                            }

                            if (!NT_SUCCESS(status))
                                PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                        }

                        PhFreeFileDialog(fileDialog);
                    }

                    PhFree(memoryNodes);
                    NtClose(processHandle);
                }
                break;
            case ID_MEMORY_CHANGEPROTECTION:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);

                        PhShowMemoryProtectDialog(hwndDlg, processItem, memoryNode->MemoryItem);
                        PhUpdateMemoryNode(&memoryContext->ListContext, memoryNode);

                        PhDereferenceObject(memoryNode->MemoryItem);
                    }
                }
                break;
            case ID_MEMORY_FREE:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);
                        PhUiFreeMemory(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem, TRUE);
                        PhDereferenceObject(memoryNode->MemoryItem);
                        // TODO: somehow update the list
                    }
                }
                break;
            case ID_MEMORY_DECOMMIT:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);
                        PhUiFreeMemory(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem, FALSE);
                        PhDereferenceObject(memoryNode->MemoryItem);
                    }
                }
                break;
            case ID_MEMORY_READWRITEADDRESS:
                {
                    PPH_STRING selectedChoice = NULL;

                    if (!memoryContext->MemoryItemListValid)
                        break;

                    while (PhaChoiceDialog(
                        hwndDlg,
                        L"Read/Write Address",
                        L"Enter an address:",
                        NULL,
                        0,
                        NULL,
                        PH_CHOICE_DIALOG_USER_CHOICE,
                        &selectedChoice,
                        NULL,
                        L"MemoryReadWriteAddressChoices"
                        ))
                    {
                        ULONG64 address64;
                        PVOID address;

                        if (selectedChoice->Length == 0)
                            continue;

                        if (PhStringToInteger64(&selectedChoice->sr, 0, &address64))
                        {
                            PPH_MEMORY_ITEM memoryItem;

                            address = (PVOID)address64;
                            memoryItem = PhLookupMemoryItemList(&memoryContext->MemoryItemList, address);

                            if (memoryItem)
                            {
                                PPH_SHOWMEMORYEDITOR showMemoryEditor = PhAllocate(sizeof(PH_SHOWMEMORYEDITOR));

                                memset(showMemoryEditor, 0, sizeof(PH_SHOWMEMORYEDITOR));
                                showMemoryEditor->ProcessId = processItem->ProcessId;
                                showMemoryEditor->BaseAddress = memoryItem->BaseAddress;
                                showMemoryEditor->RegionSize = memoryItem->RegionSize;
                                showMemoryEditor->SelectOffset = (ULONG)((ULONG_PTR)address - (ULONG_PTR)memoryItem->BaseAddress);
                                showMemoryEditor->SelectLength = 0;
                                ProcessHacker_ShowMemoryEditor(PhMainWndHandle, showMemoryEditor);
                                break;
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"Unable to find the memory region for the selected address.");
                            }
                        }
                    }
                }
                break;
            case ID_MEMORY_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_HIDEFREEREGIONS:
                {
                    BOOLEAN hide;

                    hide = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HIDEFREEREGIONS)) == BST_CHECKED;
                    PhSetOptionsMemoryList(&memoryContext->ListContext, hide);
                }
                break;
            case IDC_STRINGS:
                PhShowMemoryStringDialog(hwndDlg, processItem);
                break;
            case IDC_REFRESH:
                PhpRefreshProcessMemoryList(hwndDlg, propPageContext);
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PVOID environment;
            ULONG environmentLength;
            ULONG enumerationKey;
            PH_ENVIRONMENT_VARIABLE variable;
            HWND lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");

            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"EnvironmentListViewColumns", lvHandle);

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                processItem->ProcessId
                )))
            {
                ULONG flags;

                flags = 0;

#ifdef _WIN64
                if (processItem->IsWow64)
                    flags |= PH_GET_PROCESS_ENVIRONMENT_WOW64;
#endif

                if (NT_SUCCESS(PhGetProcessEnvironment(
                    processHandle,
                    flags,
                    &environment,
                    &environmentLength
                    )))
                {
                    enumerationKey = 0;

                    while (PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable))
                    {
                        INT lvItemIndex;
                        PPH_STRING nameString;
                        PPH_STRING valueString;

                        // Don't display pairs with no name.
                        if (variable.Name.Length == 0)
                            continue;

                        // The strings are not guaranteed to be null-terminated, so we need to create some temporary strings.
                        nameString = PhCreateString2(&variable.Name);
                        valueString = PhCreateString2(&variable.Value);

                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, nameString->Buffer, NULL);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, valueString->Buffer);

                        PhDereferenceObject(nameString);
                        PhDereferenceObject(valueString);
                    }

                    PhFreePage(environment);
                }

                NtClose(processHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"EnvironmentListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, GetDlgItem(hwndDlg, IDC_LIST), PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CTLCOLORDLG:
        {
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI HandleAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    // Parameter contains a pointer to the added handle item.
    PhReferenceObject(Parameter);
    PostMessage(
        handlesContext->WindowHandle,
        WM_PH_HANDLE_ADDED,
        PhGetRunIdProvider(&handlesContext->ProviderRegistration),
        (LPARAM)Parameter
        );
}

static VOID NTAPI HandleModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandleRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_REMOVED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandlesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLES_UPDATED, 0, 0);
}

static NTSTATUS PhpDuplicateHandleFromProcessItem(
    _Out_ PHANDLE NewHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId,
    _In_ HANDLE Handle
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        ProcessId
        )))
        return status;

    status = PhDuplicateObject(
        processHandle,
        Handle,
        NtCurrentProcess(),
        NewHandle,
        DesiredAccess,
        0,
        0
        );
    NtClose(processHandle);

    return status;
}

static VOID PhpShowProcessPropContext(
    _In_ PVOID Parameter
    )
{
    PhShowProcessProperties(Parameter);
    PhDereferenceObject(Parameter);
}

VOID PhInsertHandleObjectPropertiesEMenuItems(
    _In_ struct _PH_EMENU_ITEM *Menu,
    _In_ ULONG InsertBeforeId,
    _In_ BOOLEAN EnableShortcut,
    _In_ PPH_HANDLE_ITEM_INFO Info
    )
{
    PPH_EMENU_ITEM parentItem;
    ULONG indexInParent;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertBeforeId, &parentItem, &indexInParent))
        return;

    if (PhEqualString2(Info->TypeName, L"File", TRUE) || PhEqualString2(Info->TypeName, L"DLL", TRUE) ||
        PhEqualString2(Info->TypeName, L"Mapped File", TRUE) || PhEqualString2(Info->TypeName, L"Mapped Image", TRUE))
    {
        if (PhEqualString2(Info->TypeName, L"File", TRUE))
            PhInsertEMenuItem(parentItem, PhCreateEMenuItem(0, ID_HANDLE_OBJECTPROPERTIES2, L"File Properties", NULL, NULL), indexInParent);

        PhInsertEMenuItem(parentItem, PhCreateEMenuItem(0, ID_HANDLE_OBJECTPROPERTIES1, PhaAppendCtrlEnter(L"Open &File Location", EnableShortcut), NULL, NULL), indexInParent);
    }
    else if (PhEqualString2(Info->TypeName, L"Key", TRUE))
    {
        PhInsertEMenuItem(parentItem, PhCreateEMenuItem(0, ID_HANDLE_OBJECTPROPERTIES1, PhaAppendCtrlEnter(L"Open Key", EnableShortcut), NULL, NULL), indexInParent);
    }
    else if (PhEqualString2(Info->TypeName, L"Process", TRUE))
    {
        PhInsertEMenuItem(parentItem, PhCreateEMenuItem(0, ID_HANDLE_OBJECTPROPERTIES1, PhaAppendCtrlEnter(L"Process Properties", EnableShortcut), NULL, NULL), indexInParent);
    }
    else if (PhEqualString2(Info->TypeName, L"Section", TRUE))
    {
        PhInsertEMenuItem(parentItem, PhCreateEMenuItem(0, ID_HANDLE_OBJECTPROPERTIES1, PhaAppendCtrlEnter(L"Read/Write Memory", EnableShortcut), NULL, NULL), indexInParent);
    }
    else if (PhEqualString2(Info->TypeName, L"Thread", TRUE))
    {
        PhInsertEMenuItem(parentItem, PhCreateEMenuItem(0, ID_HANDLE_OBJECTPROPERTIES1, PhaAppendCtrlEnter(L"Go to Thread", EnableShortcut), NULL, NULL), indexInParent);
    }
}

VOID PhShowHandleObjectProperties1(
    _In_ HWND hWnd,
    _In_ PPH_HANDLE_ITEM_INFO Info
    )
{
    if (PhEqualString2(Info->TypeName, L"File", TRUE) || PhEqualString2(Info->TypeName, L"DLL", TRUE) ||
        PhEqualString2(Info->TypeName, L"Mapped File", TRUE) || PhEqualString2(Info->TypeName, L"Mapped Image", TRUE))
    {
        if (Info->BestObjectName)
            PhShellExploreFile(hWnd, Info->BestObjectName->Buffer);
        else
            PhShowError(hWnd, L"Unable to open file location because the object is unnamed.");
    }
    else if (PhEqualString2(Info->TypeName, L"Key", TRUE))
    {
        if (Info->BestObjectName)
            PhShellOpenKey2(hWnd, Info->BestObjectName);
        else
            PhShowError(hWnd, L"Unable to open key because the object is unnamed.");
    }
    else if (PhEqualString2(Info->TypeName, L"Process", TRUE))
    {
        HANDLE processHandle;
        HANDLE processId;
        PPH_PROCESS_ITEM targetProcessItem;

        processId = NULL;

        if (KphIsConnected())
        {
            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                ProcessQueryAccess,
                Info->ProcessId
                )))
            {
                PROCESS_BASIC_INFORMATION basicInfo;

                if (NT_SUCCESS(KphQueryInformationObject(
                    processHandle,
                    Info->Handle,
                    KphObjectProcessBasicInformation,
                    &basicInfo,
                    sizeof(PROCESS_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    processId = basicInfo.UniqueProcessId;
                }

                NtClose(processHandle);
            }
        }
        else
        {
            HANDLE handle;
            PROCESS_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(PhpDuplicateHandleFromProcessItem(
                &handle,
                ProcessQueryAccess,
                Info->ProcessId,
                Info->Handle
                )))
            {
                if (NT_SUCCESS(PhGetProcessBasicInformation(handle, &basicInfo)))
                    processId = basicInfo.UniqueProcessId;

                NtClose(handle);
            }
        }

        if (processId)
        {
            targetProcessItem = PhReferenceProcessItem(processId);

            if (targetProcessItem)
            {
                ProcessHacker_ShowProcessProperties(PhMainWndHandle, targetProcessItem);
                PhDereferenceObject(targetProcessItem);
            }
            else
            {
                PhShowError(hWnd, L"The process does not exist.");
            }
        }
    }
    else if (PhEqualString2(Info->TypeName, L"Section", TRUE))
    {
        HANDLE handle = NULL;
        BOOLEAN readOnly = FALSE;

        if (!NT_SUCCESS(PhpDuplicateHandleFromProcessItem(
            &handle,
            SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE,
            Info->ProcessId,
            Info->Handle
            )))
        {
            PhpDuplicateHandleFromProcessItem(
                &handle,
                SECTION_QUERY | SECTION_MAP_READ,
                Info->ProcessId,
                Info->Handle
                );
            readOnly = TRUE;
        }

        if (handle)
        {
            NTSTATUS status;
            PPH_STRING sectionName = NULL;
            SECTION_BASIC_INFORMATION basicInfo;
            SIZE_T viewSize = PH_MAX_SECTION_EDIT_SIZE;
            PVOID viewBase = NULL;
            BOOLEAN tooBig = FALSE;

            PhGetHandleInformation(NtCurrentProcess(), handle, -1, NULL, NULL, NULL, &sectionName);

            if (NT_SUCCESS(PhGetSectionBasicInformation(handle, &basicInfo)))
            {
                if (basicInfo.MaximumSize.QuadPart <= PH_MAX_SECTION_EDIT_SIZE)
                    viewSize = (SIZE_T)basicInfo.MaximumSize.QuadPart;
                else
                    tooBig = TRUE;

                status = NtMapViewOfSection(
                    handle,
                    NtCurrentProcess(),
                    &viewBase,
                    0,
                    0,
                    NULL,
                    &viewSize,
                    ViewShare,
                    0,
                    readOnly ? PAGE_READONLY : PAGE_READWRITE
                    );

                if (status == STATUS_SECTION_PROTECTION && !readOnly)
                {
                    status = NtMapViewOfSection(
                        handle,
                        NtCurrentProcess(),
                        &viewBase,
                        0,
                        0,
                        NULL,
                        &viewSize,
                        ViewShare,
                        0,
                        PAGE_READONLY
                        );
                }

                if (NT_SUCCESS(status))
                {
                    PPH_SHOWMEMORYEDITOR showMemoryEditor = PhAllocate(sizeof(PH_SHOWMEMORYEDITOR));

                    if (tooBig)
                        PhShowWarning(hWnd, L"The section size is greater than 32 MB. Only the first 32 MB will be available for editing.");

                    memset(showMemoryEditor, 0, sizeof(PH_SHOWMEMORYEDITOR));
                    showMemoryEditor->ProcessId = NtCurrentProcessId();
                    showMemoryEditor->BaseAddress = viewBase;
                    showMemoryEditor->RegionSize = viewSize;
                    showMemoryEditor->SelectOffset = -1;
                    showMemoryEditor->SelectLength = 0;
                    showMemoryEditor->Title = sectionName ? PhConcatStrings2(L"Section - ", sectionName->Buffer) : PhCreateString(L"Section");
                    showMemoryEditor->Flags = PH_MEMORY_EDITOR_UNMAP_VIEW_OF_SECTION;
                    ProcessHacker_ShowMemoryEditor(PhMainWndHandle, showMemoryEditor);
                }
                else
                {
                    PhShowStatus(hWnd, L"Unable to map a view of the section", status, 0);
                }
            }

            PhClearReference(&sectionName);

            NtClose(handle);
        }
    }
    else if (PhEqualString2(Info->TypeName, L"Thread", TRUE))
    {
        HANDLE processHandle;
        CLIENT_ID clientId;
        PPH_PROCESS_ITEM targetProcessItem;
        PPH_PROCESS_PROPCONTEXT propContext;

        clientId.UniqueProcess = NULL;
        clientId.UniqueThread = NULL;

        if (KphIsConnected())
        {
            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                ProcessQueryAccess,
                Info->ProcessId
                )))
            {
                THREAD_BASIC_INFORMATION basicInfo;

                if (NT_SUCCESS(KphQueryInformationObject(
                    processHandle,
                    Info->Handle,
                    KphObjectThreadBasicInformation,
                    &basicInfo,
                    sizeof(THREAD_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    clientId = basicInfo.ClientId;
                }

                NtClose(processHandle);
            }
        }
        else
        {
            HANDLE handle;
            THREAD_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(PhpDuplicateHandleFromProcessItem(
                &handle,
                ThreadQueryAccess,
                Info->ProcessId,
                Info->Handle
                )))
            {
                if (NT_SUCCESS(PhGetThreadBasicInformation(handle, &basicInfo)))
                    clientId = basicInfo.ClientId;

                NtClose(handle);
            }
        }

        if (clientId.UniqueProcess)
        {
            targetProcessItem = PhReferenceProcessItem(clientId.UniqueProcess);

            if (targetProcessItem)
            {
                propContext = PhCreateProcessPropContext(PhMainWndHandle, targetProcessItem);
                PhDereferenceObject(targetProcessItem);
                PhSetSelectThreadIdProcessPropContext(propContext, clientId.UniqueThread);
                ProcessHacker_Invoke(PhMainWndHandle, PhpShowProcessPropContext, propContext);
            }
            else
            {
                PhShowError(hWnd, L"The process does not exist.");
            }
        }
    }
}

VOID PhShowHandleObjectProperties2(
    _In_ HWND hWnd,
    _In_ PPH_HANDLE_ITEM_INFO Info
    )
{
    if (PhEqualString2(Info->TypeName, L"File", TRUE) || PhEqualString2(Info->TypeName, L"DLL", TRUE) ||
        PhEqualString2(Info->TypeName, L"Mapped File", TRUE) || PhEqualString2(Info->TypeName, L"Mapped Image", TRUE))
    {
        if (Info->BestObjectName)
            PhShellProperties(hWnd, Info->BestObjectName->Buffer);
        else
            PhShowError(hWnd, L"Unable to open file properties because the object is unnamed.");
    }
}

VOID PhpInitializeHandleMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM *Handles,
    _In_ ULONG NumberOfHandles,
    _Inout_ PPH_HANDLES_CONTEXT HandlesContext
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfHandles == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfHandles == 1)
    {
        PH_HANDLE_ITEM_INFO info;

        info.ProcessId = ProcessId;
        info.Handle = Handles[0]->Handle;
        info.TypeName = Handles[0]->TypeName;
        info.BestObjectName = Handles[0]->BestObjectName;
        PhInsertHandleObjectPropertiesEMenuItems(Menu, ID_HANDLE_COPY, TRUE, &info);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        PhEnableEMenuItem(Menu, ID_HANDLE_CLOSE, TRUE);
        PhEnableEMenuItem(Menu, ID_HANDLE_COPY, TRUE);
    }

    // Remove irrelevant menu items.

    if (!KphIsConnected())
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_HANDLE_PROTECTED))
            PhDestroyEMenuItem(item);
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_HANDLE_INHERIT))
            PhDestroyEMenuItem(item);
    }

    // Protected, Inherit
    if (NumberOfHandles == 1 && KphIsConnected())
    {
        HandlesContext->SelectedHandleProtected = FALSE;
        HandlesContext->SelectedHandleInherit = FALSE;

        if (Handles[0]->Attributes & OBJ_PROTECT_CLOSE)
        {
            HandlesContext->SelectedHandleProtected = TRUE;
            PhSetFlagsEMenuItem(Menu, ID_HANDLE_PROTECTED, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }

        if (Handles[0]->Attributes & OBJ_INHERIT)
        {
            HandlesContext->SelectedHandleInherit = TRUE;
            PhSetFlagsEMenuItem(Menu, ID_HANDLE_INHERIT, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }
    }
}

VOID PhShowHandleContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_HANDLES_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_HANDLE_ITEM *handles;
    ULONG numberOfHandles;

    PhGetSelectedHandleItems(&Context->ListContext, &handles, &numberOfHandles);

    if (numberOfHandles != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_HANDLE), 0);
        PhSetFlagsEMenuItem(menu, ID_HANDLE_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeHandleMenu(menu, ProcessItem->ProcessId, handles, numberOfHandles, Context);
        PhInsertCopyCellEMenuItem(menu, ID_HANDLE_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Handle.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Handle.Handles = handles;
            menuInfo.u.Handle.NumberOfHandles = numberOfHandles;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(handles);
}

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_HANDLES_CONTEXT handlesContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        handlesContext = (PPH_HANDLES_CONTEXT)propPageContext->Context;

        if (handlesContext)
            tnHandle = handlesContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            handlesContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmHandlesContextType, sizeof(PH_HANDLES_CONTEXT)));

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
            handlesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeHandleList(hwndDlg, tnHandle, &handlesContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &LoadingText, 0);
            handlesContext->NeedsRedraw = FALSE;
            handlesContext->LastRunStatus = -1;
            handlesContext->ErrorMessage = NULL;

            PhEmCallObjectOperation(EmHandlesContextType, handlesContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &handlesContext->ListContext.Cm;
                treeNewInfo.SystemContext = handlesContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsHandleList(&handlesContext->ListContext);

            PhSetOptionsHandleList(&handlesContext->ListContext, !!PhGetIntegerSetting(L"HideUnnamedHandles"));
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_HIDEUNNAMEDHANDLES),
                handlesContext->ListContext.HideUnnamedHandles ? BST_CHECKED : BST_UNCHECKED);

            PhSetEnabledProvider(&handlesContext->ProviderRegistration, TRUE);
            PhBoostProvider(&handlesContext->ProviderRegistration, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmHandlesContextType, handlesContext, EmObjectDelete);

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
            PhUnregisterProvider(&handlesContext->ProviderRegistration);
            PhDereferenceObject(handlesContext->Provider);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &handlesContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsHandleList(&handlesContext->ListContext);
            PhDeleteHandleList(&handlesContext->ListContext);

            PhFree(handlesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowHandleContextMenu(hwndDlg, processItem, handlesContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_HANDLE_CLOSE:
                {
                    PPH_HANDLE_ITEM *handles;
                    ULONG numberOfHandles;

                    PhGetSelectedHandleItems(&handlesContext->ListContext, &handles, &numberOfHandles);
                    PhReferenceObjects(handles, numberOfHandles);

                    if (PhUiCloseHandles(hwndDlg, processItem->ProcessId, handles, numberOfHandles, !!lParam))
                        PhDeselectAllHandleNodes(&handlesContext->ListContext);

                    PhDereferenceObjects(handles, numberOfHandles);
                    PhFree(handles);
                }
                break;
            case ID_HANDLE_PROTECTED:
            case ID_HANDLE_INHERIT:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

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

                        PhReferenceObject(handleItem);
                        PhUiSetAttributesHandle(hwndDlg, processItem->ProcessId, handleItem, attributes);
                        PhDereferenceObject(handleItem);
                    }
                }
                break;
            case ID_HANDLE_OBJECTPROPERTIES1:
            case ID_HANDLE_OBJECTPROPERTIES2:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

                    if (handleItem)
                    {
                        PH_HANDLE_ITEM_INFO info;

                        info.ProcessId = processItem->ProcessId;
                        info.Handle = handleItem->Handle;
                        info.TypeName = handleItem->TypeName;
                        info.BestObjectName = handleItem->BestObjectName;

                        if (id == ID_HANDLE_OBJECTPROPERTIES1)
                            PhShowHandleObjectProperties1(hwndDlg, &info);
                        else
                            PhShowHandleObjectProperties2(hwndDlg, &info);
                    }
                }
                break;
            case ID_HANDLE_PROPERTIES:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

                    if (handleItem)
                    {
                        PhReferenceObject(handleItem);
                        PhShowHandleProperties(hwndDlg, processItem->ProcessId, handleItem);
                        PhDereferenceObject(handleItem);
                    }
                }
                break;
            case ID_HANDLE_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_HIDEUNNAMEDHANDLES:
                {
                    BOOLEAN hide;

                    hide = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HIDEUNNAMEDHANDLES)) == BST_CHECKED;
                    PhSetOptionsHandleList(&handlesContext->ListContext, hide);
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
                PhSetEnabledProvider(&handlesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetEnabledProvider(&handlesContext->ProviderRegistration, FALSE);
                break;
            }
        }
        break;
    case WM_PH_HANDLE_ADDED:
        {
            ULONG runId = (ULONG)wParam;
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            if (!handlesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                handlesContext->NeedsRedraw = TRUE;
            }

            PhAddHandleNode(&handlesContext->ListContext, handleItem, runId);
            PhDereferenceObject(handleItem);
        }
        break;
    case WM_PH_HANDLE_MODIFIED:
        {
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            if (!handlesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                handlesContext->NeedsRedraw = TRUE;
            }

            PhUpdateHandleNode(&handlesContext->ListContext, PhFindHandleNode(&handlesContext->ListContext, handleItem->Handle));
        }
        break;
    case WM_PH_HANDLE_REMOVED:
        {
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            if (!handlesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);
                handlesContext->NeedsRedraw = TRUE;
            }

            PhRemoveHandleNode(&handlesContext->ListContext, PhFindHandleNode(&handlesContext->ListContext, handleItem->Handle));
        }
        break;
    case WM_PH_HANDLES_UPDATED:
        {
            PhTickHandleNodes(&handlesContext->ListContext);

            if (handlesContext->LastRunStatus != handlesContext->Provider->RunStatus)
            {
                NTSTATUS status;
                PPH_STRING message;

                status = handlesContext->Provider->RunStatus;
                handlesContext->LastRunStatus = status;

                if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
                    status = STATUS_SUCCESS;

                if (NT_SUCCESS(status))
                {
                    TreeNew_SetEmptyText(tnHandle, &EmptyHandlesText, 0);
                }
                else
                {
                    message = PhGetStatusMessage(status, 0);
                    PhMoveReference(&handlesContext->ErrorMessage, PhFormatString(L"Unable to query handle information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
                    PhClearReference(&message);
                    TreeNew_SetEmptyText(tnHandle, &handlesContext->ErrorMessage->sr, 0);
                }

                InvalidateRect(tnHandle, NULL, FALSE);
            }

            if (handlesContext->NeedsRedraw)
            {
                TreeNew_SetRedraw(tnHandle, TRUE);
                handlesContext->NeedsRedraw = FALSE;
            }
        }
        break;
    }

    return FALSE;
}

static NTSTATUS NTAPI PhpOpenProcessJob(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE jobHandle = NULL;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        ProcessQueryAccess,
        (HANDLE)Context
        )))
        return status;

    status = KphOpenProcessJob(processHandle, DesiredAccess, &jobHandle);
    NtClose(processHandle);

    if (NT_SUCCESS(status) && status != STATUS_PROCESS_NOT_IN_JOB && jobHandle)
    {
        *Handle = jobHandle;
    }
    else if (NT_SUCCESS(status))
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

INT_PTR CALLBACK PhpProcessJobHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!GetProp(hwndDlg, PhMakeContextAtom())) // LayoutInitialized
            {
                PPH_LAYOUT_ITEM dialogItem;

                // This is a big violation of abstraction...

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TERMINATE),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESSES),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADD),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIMITS),
                    dialogItem, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADVANCED),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                PhDoPropPageLayout(hwndDlg);

                SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)TRUE);
            }
        }
        break;
    }

    return FALSE;
}

static VOID PhpLayoutServiceListControl(
    _In_ HWND hwndDlg,
    _In_ HWND ServiceListHandle
    )
{
    RECT rect;

    GetWindowRect(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), &rect);
    MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);

    MoveWindow(
        ServiceListHandle,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        TRUE
        );
}

INT_PTR CALLBACK PhpProcessServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_SERVICE_ITEM *services;
            ULONG numberOfServices;
            ULONG i;
            HWND serviceListHandle;

            // Get a copy of the process' service list.

            PhAcquireQueuedLockShared(&processItem->ServiceListLock);

            numberOfServices = processItem->ServiceList->Count;
            services = PhAllocate(numberOfServices * sizeof(PPH_SERVICE_ITEM));

            {
                ULONG enumerationKey = 0;
                PPH_SERVICE_ITEM serviceItem;

                i = 0;

                while (PhEnumPointerList(processItem->ServiceList, &enumerationKey, &serviceItem))
                {
                    PhReferenceObject(serviceItem);
                    services[i++] = serviceItem;
                }
            }

            PhReleaseQueuedLockShared(&processItem->ServiceListLock);

            serviceListHandle = PhCreateServiceListControl(
                hwndDlg,
                services,
                numberOfServices
                );
            SendMessage(serviceListHandle, WM_PH_SET_LIST_VIEW_SETTINGS, 0, (LPARAM)L"ProcessServiceListViewColumns");
            ShowWindow(serviceListHandle, SW_SHOW);

            propPageContext->Context = serviceListHandle;
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

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT),
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);
                PhpLayoutServiceListControl(hwndDlg, (HWND)propPageContext->Context);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhpLayoutServiceListControl(hwndDlg, (HWND)propPageContext->Context);
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhpProcessPropertiesThreadStart(
    _In_ PVOID Parameter
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
        PhCreateTokenPage(PhpOpenProcessToken, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessTokenHookProc)
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
        KphIsConnected()
        )
    {
        PhAddProcessPropPage2(
            PropContext,
            PhCreateJobPage(PhpOpenProcessJob, (PVOID)PropContext->ProcessItem->ProcessId, PhpProcessJobHookProc)
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

    hwnd = (HWND)PropertySheet(&PropContext->PropSheetHeader);

    PhDereferenceObject(startPage);

    PropContext->WindowHandle = hwnd;
    PhSetEvent(&PropContext->CreatedEvent);

    // Main event loop

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!PropSheet_IsDialogMessage(hwnd, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);

        if (!PropSheet_GetCurrentPageHwnd(hwnd))
            break;
    }

    DestroyWindow(hwnd);
    PhDereferenceObject(PropContext);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

BOOLEAN PhShowProcessProperties(
    _In_ PPH_PROCESS_PROPCONTEXT Context
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

/*
 * Process Hacker -
 *   property sheet 
 *
 * Copyright (C) 2017 dmex
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

// NOTE: Copied from processhacker2\ProcessHacker\procprp.c

#include <peview.h>

PPH_OBJECT_TYPE PvpPropContextType;
PPH_OBJECT_TYPE PvpPropPageContextType;
static RECT MinimumSize = { -1, -1, -1, -1 };

VOID NTAPI PvpPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID NTAPI PvpPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

INT CALLBACK PvpPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    );

INT CALLBACK PvpStandardPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

BOOLEAN PvPropInitialization(
    VOID
    )
{
    PvpPropContextType = PhCreateObjectType(L"PvPropContext", 0, PvpPropContextDeleteProcedure);
    PvpPropPageContextType = PhCreateObjectType(L"PvPropPageContext", 0, PvpPropPageContextDeleteProcedure);

    return TRUE;
}

PPV_PROPCONTEXT PvCreatePropContext(
    _In_ PPH_STRING Caption
    )
{
    PPV_PROPCONTEXT propContext;
    PROPSHEETHEADER propSheetHeader;

    propContext = PhCreateObject(sizeof(PV_PROPCONTEXT), PvpPropContextType);
    memset(propContext, 0, sizeof(PV_PROPCONTEXT));

    propContext->Title = Caption;
    propContext->PropSheetPages = PhAllocate(sizeof(HPROPSHEETPAGE) * PV_PROPCONTEXT_MAXPAGES);

    memset(&propSheetHeader, 0, sizeof(PROPSHEETHEADER));
    propSheetHeader.dwSize = sizeof(PROPSHEETHEADER);
    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK;
    propSheetHeader.hwndParent = NULL;
    propSheetHeader.pszCaption = propContext->Title->Buffer;
    propSheetHeader.pfnCallback = PvpPropSheetProc;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    memcpy(&propContext->PropSheetHeader, &propSheetHeader, sizeof(PROPSHEETHEADER));

    return propContext;
}

VOID NTAPI PvpPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPV_PROPCONTEXT propContext = (PPV_PROPCONTEXT)Object;

    PhFree(propContext->PropSheetPages);
    PhDereferenceObject(propContext->Title);
}

INT CALLBACK PvpPropSheetProc(
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
            PPV_PROPSHEETCONTEXT propSheetContext;

            propSheetContext = PhAllocate(sizeof(PV_PROPSHEETCONTEXT));
            memset(propSheetContext, 0, sizeof(PV_PROPSHEETCONTEXT));

            PhInitializeLayoutManager(&propSheetContext->LayoutManager, hwndDlg);

            SetWindowSubclass(hwndDlg, PvpPropSheetWndProc, 0, (ULONG_PTR)propSheetContext);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 320;
                rect.bottom = 300;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }
        }
        break;
    }

    return 0;
}

LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext = dwRefData;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hWnd, PvpPropSheetWndProc, uIdSubclass);

            PhDeleteLayoutManager(&propSheetContext->LayoutManager);
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
            if (!IsIconic(hWnd))
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

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOLEAN PhpInitializePropSheetLayoutStage1(
    _In_ HWND hwnd
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext = PvpGetPropSheetContext(hwnd);

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

BOOLEAN PvAddPropPage(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPV_PROPPAGECONTEXT PropPageContext
    )
{
    HPROPSHEETPAGE propSheetPageHandle;

    if (PropContext->PropSheetHeader.nPages == PV_PROPCONTEXT_MAXPAGES)
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

BOOLEAN PvAddPropPage2(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ HPROPSHEETPAGE PropSheetPageHandle
    )
{
    if (PropContext->PropSheetHeader.nPages == PV_PROPCONTEXT_MAXPAGES)
        return FALSE;

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] = PropSheetPageHandle;
    PropContext->PropSheetHeader.nPages++;

    return TRUE;
}

PPV_PROPPAGECONTEXT PvCreatePropPageContext(
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    return PvCreatePropPageContextEx(NULL, Template, DlgProc, Context);
}

PPV_PROPPAGECONTEXT PvCreatePropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    propPageContext = PhCreateObject(sizeof(PV_PROPPAGECONTEXT), PvpPropPageContextType);
    memset(propPageContext, 0, sizeof(PV_PROPPAGECONTEXT));

    propPageContext->PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propPageContext->PropSheetPage.dwFlags =
        PSP_USECALLBACK;
    propPageContext->PropSheetPage.hInstance = InstanceHandle;
    propPageContext->PropSheetPage.pszTemplate = Template;
    propPageContext->PropSheetPage.pfnDlgProc = DlgProc;
    propPageContext->PropSheetPage.lParam = (LPARAM)propPageContext;
    propPageContext->PropSheetPage.pfnCallback = PvpStandardPropPageProc;

    propPageContext->Context = Context;

    return propPageContext;
}

VOID NTAPI PvpPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPV_PROPPAGECONTEXT propPageContext = (PPV_PROPPAGECONTEXT)Object;

    if (propPageContext->PropContext)
        PhDereferenceObject(propPageContext->PropContext);
}

INT CALLBACK PvpStandardPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    propPageContext = (PPV_PROPPAGECONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        PhReferenceObject(propPageContext);
    else if (uMsg == PSPCB_RELEASE)
        PhDereferenceObject(propPageContext);

    return 1;
}

PPH_LAYOUT_ITEM PvAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    HWND parent;
    PPV_PROPSHEETCONTEXT propSheetContext;
    PPH_LAYOUT_MANAGER layoutManager;
    PPH_LAYOUT_ITEM realParentItem;
    PPH_LAYOUT_ITEM item;

    parent = GetParent(hwnd);
    propSheetContext = PvpGetPropSheetContext(parent);
    layoutManager = &propSheetContext->LayoutManager;

    PhpInitializePropSheetLayoutStage1(parent);

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
        dialogSize.right = 300;
        dialogSize.bottom = 280;
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

    return item;
}

VOID PvDoPropPageLayout(
    _In_ HWND hwnd
    )
{
    HWND parent;
    PPV_PROPSHEETCONTEXT propSheetContext;

    parent = GetParent(hwnd);
    propSheetContext = PvpGetPropSheetContext(parent);
    PhLayoutManagerLayout(&propSheetContext->LayoutManager);
}
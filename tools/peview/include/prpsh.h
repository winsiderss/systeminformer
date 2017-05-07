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

// NOTE: Copied from processhacker2\ProcessHacker\procprp.h

#ifndef PV_PRP_H
#define PV_PRP_H

#define PV_PROPCONTEXT_MAXPAGES 20

typedef struct _PV_PROPSHEETCONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
    BOOLEAN LayoutInitialized;
} PV_PROPSHEETCONTEXT, *PPV_PROPSHEETCONTEXT;

typedef struct _PV_PROPCONTEXT
{
    PPH_STRING Title;
    PPH_STRING StartPage;
    PROPSHEETHEADER PropSheetHeader;
    HPROPSHEETPAGE *PropSheetPages;
} PV_PROPCONTEXT, *PPV_PROPCONTEXT;

typedef struct _PV_PROPPAGECONTEXT
{
    PPV_PROPCONTEXT PropContext;
    PVOID Context;
    PROPSHEETPAGE PropSheetPage;

    BOOLEAN LayoutInitialized;
} PV_PROPPAGECONTEXT, *PPV_PROPPAGECONTEXT;

BOOLEAN PvPropInitialization(
    VOID
    );

PPV_PROPCONTEXT PvCreatePropContext(
    _In_ PPH_STRING Caption
    );

BOOLEAN PvAddPropPage(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPV_PROPPAGECONTEXT PropPageContext
    );

BOOLEAN PvAddPropPage2(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ HPROPSHEETPAGE PropSheetPageHandle
    );

PPV_PROPPAGECONTEXT PvCreatePropPageContext(
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

PPV_PROPPAGECONTEXT PvCreatePropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

BOOLEAN PvPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_ PPV_PROPPAGECONTEXT *PropPageContext
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PPH_LAYOUT_ITEM PvAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

VOID PvDoPropPageLayout(
    _In_ HWND hwnd
    );

FORCEINLINE BOOLEAN PvPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_ PPV_PROPPAGECONTEXT *PropPageContext
    )
{
    LPPROPSHEETPAGE propSheetPage;

    if (uMsg == WM_INITDIALOG)
    {
        // Save the context.
        SetProp(hwndDlg, L"PvContext", (HANDLE)lParam);
    }

    propSheetPage = (LPPROPSHEETPAGE)GetProp(hwndDlg, L"PvContext");

    if (!propSheetPage)
        return FALSE;

    *PropSheetPage = propSheetPage;
    *PropPageContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;

    return TRUE;
}

#endif

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017
 *
 */

// NOTE: Copied from processhacker\ProcessHacker\procprp.h

#ifndef PV_PRP_H
#define PV_PRP_H

#define PV_PROPCONTEXT_MAXPAGES 40

typedef struct _PV_PROPSHEETCONTEXT
{
    BOOLEAN LayoutInitialized;
    WNDPROC DefaultWindowProc;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
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

_Success_(return)
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
        PhSetWindowContext(hwndDlg, ULONG_MAX, (PVOID)lParam);
    }

    propSheetPage = (LPPROPSHEETPAGE)PhGetWindowContext(hwndDlg, ULONG_MAX);

    if (!propSheetPage)
        return FALSE;

    *PropSheetPage = propSheetPage;
    *PropPageContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;

    return TRUE;
}

#endif

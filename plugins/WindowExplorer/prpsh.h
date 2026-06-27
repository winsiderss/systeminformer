/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
 *
 */

// NOTE: Copied from processhacker2\ProcessHacker\procprp.h

#ifndef PV_PRP_H
#define PV_PRP_H

#include <graphprp.h>

EXTERN_C_START

typedef struct _PV_PROPCONTEXT
{
    PPH_PROPSHEETNEW_BUILDER Builder;
} PV_PROPCONTEXT, *PPV_PROPCONTEXT;

typedef struct _PV_PROPPAGECONTEXT
{
    PVOID Context;
    PVOID Instance;
    LPCWSTR Template;
    DLGPROC DialogProc;
    PCWSTR Id;
    PCWSTR Name;

    BOOLEAN LayoutInitialized;
} PV_PROPPAGECONTEXT, *PPV_PROPPAGECONTEXT;

PPV_PROPCONTEXT HdCreatePropContext(
    _In_ PWSTR Caption,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_PROPSHEETNEW_INITIALIZED_CALLBACK InitializedCallback
    );

BOOLEAN PvAddPropPage(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPV_PROPPAGECONTEXT PropPageContext
    );

PPV_PROPPAGECONTEXT PvCreatePropPageContext(
    _In_ PCWSTR Id,
    _In_ PCWSTR Name,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

PPV_PROPPAGECONTEXT PvCreatePropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ PCWSTR Id,
    _In_ PCWSTR Name,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

_Success_(return)
BOOLEAN
NTAPI
PvPropPageDlgProcHeader(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ LPARAM lParam,
    _Out_ PPV_PROPPAGECONTEXT *PropPageContext
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PPH_LAYOUT_ITEM PvAddPropPageLayoutItem(
    _In_ HWND WindowHandle,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

VOID PvDoPropPageLayout(
    _In_ HWND WindowHandle
    );

_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PvPropPageDlgProcHeader(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ LPARAM lParam,
    _Out_ PPV_PROPPAGECONTEXT *PropPageContext
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    if (WindowMessage == WM_INITDIALOG)
    {
        propPageContext = (PPV_PROPPAGECONTEXT)lParam;
        PhSetWindowContext(WindowHandle, 0xfff, propPageContext);
    }
    else
    {
        propPageContext = (PPV_PROPPAGECONTEXT)PhGetWindowContext(WindowHandle, 0xfff);

        if (!propPageContext)
            return FALSE;

        if (WindowMessage == WM_NCDESTROY)
            PhRemoveWindowContext(WindowHandle, 0xfff);
    }

    *PropPageContext = propPageContext;

    return TRUE;
}

VOID PvRefreshChildWindows(
    _In_ HWND WindowHandle
    );

EXTERN_C_END

#endif

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

#include "wndexp.h"

BOOLEAN HdPropContextInit = FALSE;
PPH_OBJECT_TYPE PvpPropContextType = NULL;
PPH_OBJECT_TYPE PvpPropPageContextType = NULL;
static RECT MinimumSize = { -1, -1, -1, -1 };

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PvpPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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
    _In_ LPARAM lParam
    );

INT CALLBACK PvpStandardPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

PPV_PROPCONTEXT HdCreatePropContext(
    _In_ PWSTR Caption
    )
{
    PPV_PROPCONTEXT propContext;
    PROPSHEETHEADER propSheetHeader;

    if (!PvpPropContextType)
        PvpPropContextType = PhCreateObjectType(L"HdPropContext2", 0, PvpPropContextDeleteProcedure);

    propContext = PhCreateObject(sizeof(PV_PROPCONTEXT), PvpPropContextType);
    memset(propContext, 0, sizeof(PV_PROPCONTEXT));

    propContext->PropSheetPages = PhAllocate(sizeof(HPROPSHEETPAGE) * PV_PROPCONTEXT_MAXPAGES);
    memset(propContext->PropSheetPages, 0, sizeof(HPROPSHEETPAGE) * PV_PROPCONTEXT_MAXPAGES);

    memset(&propSheetHeader, 0, sizeof(PROPSHEETHEADER));
    propSheetHeader.dwSize = sizeof(PROPSHEETHEADER);
    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK;
    propSheetHeader.hInstance = PluginInstance->DllBase;
    propSheetHeader.hwndParent = NULL;
    propSheetHeader.pszCaption = Caption;
    propSheetHeader.pfnCallback = PvpPropSheetProc;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    memcpy(&propContext->PropSheetHeader, &propSheetHeader, sizeof(PROPSHEETHEADER));

    return propContext;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PvpPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPV_PROPCONTEXT propContext = (PPV_PROPCONTEXT)Object;

    PhFree(propContext->PropSheetPages);
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
            PPV_PROPSHEETCONTEXT context;

            context = PhAllocate(sizeof(PV_PROPSHEETCONTEXT));
            memset(context, 0, sizeof(PV_PROPSHEETCONTEXT));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            context->DefaultWindowProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, ULONG_MAX, context);
            PhSetWindowProcedure(hwndDlg, PvpPropSheetWndProc);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 271;
                rect.bottom = 224;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }
        }
        break;
    }

    return 0;
}

PPV_PROPSHEETCONTEXT PvpGetPropSheetContext(
    _In_ HWND hwnd
    )
{
    return PhGetWindowContext(hwnd, ULONG_MAX);
}

LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PROPSHEETCONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hWnd, ULONG_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(hWnd, oldWndProc);
            PhRemoveWindowContext(hWnd, ULONG_MAX);

            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOWS_PROPERTY_POSITION, SETTING_NAME_WINDOWS_PROPERTY_SIZE, hWnd);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
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
            if (!IsMinimized(hWnd))
            {
                PhLayoutManagerLayout(&context->LayoutManager);
            }
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PvRefreshButtonWindowEnumCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR className[256];

    if (!GetClassName(WindowHandle, className, RTL_NUMBER_OF(className)))
        className[0] = UNICODE_NULL;

    if (PhEqualStringZ(className, L"#32770", TRUE))
    {
        SendMessage(WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }

    return TRUE;
}

VOID PvRefreshChildWindows(
    _In_ HWND WindowHandle
    )
{
    HWND propSheetHandle;

    if (propSheetHandle = GetAncestor(WindowHandle, GA_ROOT))
    {
        //HWND pageHandle;
        //
        //if (pageHandle = PropSheet_GetCurrentPageHwnd(propSheetHandle))
        //{
        //    SendMessage(pageHandle, WM_PH_UPDATE_DIALOG, 0, 0);
        //}

        PhEnumChildWindows(propSheetHandle, ULONG_MAX, PvRefreshButtonWindowEnumCallback, NULL);
    }
}

LRESULT CALLBACK PvRefreshButtonWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext;
    WNDPROC oldWndProc;

    if (!(propSheetContext = PhGetWindowContext(WindowHandle, SCHAR_MAX)))
        return DefWindowProc(WindowHandle, uMsg, wParam, lParam);

    oldWndProc = propSheetContext->OldRefreshButtonWndProc;

    switch (uMsg)
    {
    case WM_COMMAND:
        {
            if (GET_WM_COMMAND_HWND(wParam, lParam) == propSheetContext->RefreshButtonWindowHandle)
            {
                PvRefreshChildWindows(WindowHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

HWND PvpCreateOptionsButton(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND PropSheetWindow
    )
{
    if (!PropSheetContext->RefreshButtonWindowHandle)
    {
        RECT clientRect;
        RECT rect;

        PropSheetContext->OldRefreshButtonWndProc = PhGetWindowProcedure(PropSheetWindow);
        PhSetWindowContext(PropSheetWindow, SCHAR_MAX, PropSheetContext);
        PhSetWindowProcedure(PropSheetWindow, PvRefreshButtonWndProc);

        // Create the refresh button.
        PhGetClientRect(PropSheetWindow, &clientRect);
        PhGetWindowRect(GetDlgItem(PropSheetWindow, IDCANCEL), &rect);
        MapWindowRect(NULL, PropSheetWindow, &rect);
        PropSheetContext->RefreshButtonWindowHandle = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            WC_BUTTON,
            L"Refresh",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            clientRect.right - rect.right,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            PropSheetWindow,
            NULL,
            PluginInstance->DllBase,
            NULL
            );
        SetWindowFont(PropSheetContext->RefreshButtonWindowHandle, GetWindowFont(GetDlgItem(PropSheetWindow, IDCANCEL)), TRUE);
    }

    return PropSheetContext->RefreshButtonWindowHandle;
}

BOOLEAN PhpInitializePropSheetLayoutStage1(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND hwnd
    )
{
    if (!PropSheetContext->LayoutInitialized)
    {
        HWND tabControlHandle;
        PPH_LAYOUT_ITEM tabControlItem;
        PPH_LAYOUT_ITEM tabPageItem;

        tabControlHandle = PropSheet_GetTabControl(hwnd);
        tabControlItem = PhAddLayoutItem(&PropSheetContext->LayoutManager, tabControlHandle,
            NULL, PH_ANCHOR_ALL | PH_LAYOUT_IMMEDIATE_RESIZE);
        tabPageItem = PhAddLayoutItem(&PropSheetContext->LayoutManager, tabControlHandle,
            NULL, PH_LAYOUT_TAB_CONTROL); // dummy item to fix multiline tab control

        PropSheetContext->TabPageItem = tabPageItem;

        PhAddLayoutItem(&PropSheetContext->LayoutManager, GetDlgItem(hwnd, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        PhAddLayoutItem(&PropSheetContext->LayoutManager, PvpCreateOptionsButton(PropSheetContext, hwnd), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

        // Hide the OK button.
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        PhSetDialogItemText(hwnd, IDCANCEL, L"Close");

        if (PhValidWindowPlacementFromSetting(SETTING_NAME_WINDOWS_PROPERTY_POSITION))
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOWS_PROPERTY_POSITION, SETTING_NAME_WINDOWS_PROPERTY_SIZE, hwnd);

        PropSheetContext->LayoutInitialized = TRUE;

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
    return PvCreatePropPageContextEx(PluginInstance->DllBase, Template, DlgProc, Context);
}

PPV_PROPPAGECONTEXT PvCreatePropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvpPropPageContextType)
        PvpPropPageContextType = PhCreateObjectType(L"HdPropPageContext2", 0, PvpPropPageContextDeleteProcedure);

    propPageContext = PhCreateObject(sizeof(PV_PROPPAGECONTEXT), PvpPropPageContextType);
    memset(propPageContext, 0, sizeof(PV_PROPPAGECONTEXT));

    propPageContext->PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propPageContext->PropSheetPage.dwFlags = PSP_USECALLBACK;
    propPageContext->PropSheetPage.hInstance = PluginInstance->DllBase;
    propPageContext->PropSheetPage.pszTemplate = Template;
    propPageContext->PropSheetPage.pfnDlgProc = DlgProc;
    propPageContext->PropSheetPage.lParam = (LPARAM)propPageContext;
    propPageContext->PropSheetPage.pfnCallback = PvpStandardPropPageProc;

    propPageContext->Context = Context;

    return propPageContext;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

    PhpInitializePropSheetLayoutStage1(propSheetContext, parent);

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
        dialogSize.right = 271;
        dialogSize.bottom = 224;
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

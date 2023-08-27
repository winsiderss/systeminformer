/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2020
 *
 */

#include "devices.h"

static PPH_OBJECT_TYPE PvpPropContextType = NULL;
static PPH_OBJECT_TYPE PvpPropPageContextType = NULL;
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
        PvpPropContextType = PhCreateObjectType(L"HdPropContext", 0, PvpPropContextDeleteProcedure);

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

            context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, ULONG_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PvpPropSheetWndProc);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 309;
                rect.bottom = 265;
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
    case WM_DESTROY:
        {
            if (context->PositionSettingName)
                PhSaveWindowPlacementToSetting(context->PositionSettingName, context->SizeSettingName, hWnd);

            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhRemoveWindowContext(hWnd, ULONG_MAX);

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

BOOLEAN CALLBACK PvUpdateButtonWindowEnumCallback(
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

VOID PvUpdateChildWindows(
    _In_ HWND WindowHandle
    )
{
    HWND propSheetHandle;

    if (propSheetHandle = GetAncestor(WindowHandle, GA_ROOT))
    {
        PhEnumChildWindows(propSheetHandle, ULONG_MAX, PvUpdateButtonWindowEnumCallback, NULL);
    }
}

LRESULT CALLBACK PvControlButtonWndProc(
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

    oldWndProc = propSheetContext->OldOptionsButtonWndProc;

    switch (uMsg)
    {
    case WM_COMMAND:
        {
            if (GET_WM_COMMAND_HWND(wParam, lParam) == propSheetContext->OptionsButtonWindowHandle)
            {
                PvUpdateChildWindows(WindowHandle);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

HWND PvpCreateControlButton(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND PropSheetWindow
    )
{
    if (!PropSheetContext->OptionsButtonWindowHandle)
    {
        RECT clientRect;
        RECT rect;

        PropSheetContext->OldOptionsButtonWndProc = PhGetWindowProcedure(PropSheetWindow);
        PhSetWindowContext(PropSheetWindow, SCHAR_MAX, PropSheetContext);
        PhSetWindowProcedure(PropSheetWindow, PvControlButtonWndProc);

        // Create the refresh button.
        GetClientRect(PropSheetWindow, &clientRect);
        GetWindowRect(GetDlgItem(PropSheetWindow, IDCANCEL), &rect);
        MapWindowPoints(NULL, PropSheetWindow, (POINT*)& rect, 2);
        PropSheetContext->OptionsButtonWindowHandle = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            WC_BUTTON,
            L"Options",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            clientRect.right - rect.right,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            PropSheetWindow,
            (HMENU)IDABORT,
            PluginInstance->DllBase,
            NULL
            );
        SetWindowFont(PropSheetContext->OptionsButtonWindowHandle, GetWindowFont(GetDlgItem(PropSheetWindow, IDCANCEL)), TRUE);
    }

    return PropSheetContext->OptionsButtonWindowHandle;
}

BOOLEAN PhpInitializePropSheetLayoutStage1(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND hwnd,
    _In_ BOOLEAN EnableOptionsButton,
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName
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

        PhAddLayoutItem(&PropSheetContext->LayoutManager, GetDlgItem(hwnd, IDCANCEL),
            NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

        if (EnableOptionsButton)
        {
            PhAddLayoutItem(
                &PropSheetContext->LayoutManager,
                PvpCreateControlButton(PropSheetContext, hwnd),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM
                );
        }

        // Hide the OK button.
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        PhSetDialogItemText(hwnd, IDCANCEL, L"Close");

        if (PositionSettingName)
        {
            PropSheetContext->PositionSettingName = PositionSettingName;
            PropSheetContext->SizeSettingName = SizeSettingName;

            if (PhGetIntegerPairSetting(PositionSettingName).X)
            {
                PhLoadWindowPlacementFromSetting(PositionSettingName, SizeSettingName, hwnd);
            }
            else
            {
                PhCenterWindow(hwnd, GetParent(hwnd));
            }

            PhSetApplicationWindowIcon(hwnd);
        }

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
        PvpPropPageContextType = PhCreateObjectType(L"HdPropPageContext", 0, PvpPropPageContextDeleteProcedure);

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

PPH_LAYOUT_ITEM PvAddPropPageLayoutItemEx(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_ BOOLEAN EnableOptionsButton,
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName
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

    PhpInitializePropSheetLayoutStage1(propSheetContext, parent, EnableOptionsButton, PositionSettingName, SizeSettingName);

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
        dialogSize.right = 309;
        dialogSize.bottom = 265;
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

PPH_LAYOUT_ITEM PvAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    return PvAddPropPageLayoutItemEx(hwnd, Handle, ParentItem, Anchor, FALSE, NULL, NULL);
}

PPH_LAYOUT_ITEM PvAddPropPageLayoutConfig(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName
    )
{
    return PvAddPropPageLayoutItemEx(hwnd, Handle, ParentItem, Anchor, FALSE, PositionSettingName, SizeSettingName);
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

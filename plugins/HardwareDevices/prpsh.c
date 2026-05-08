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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND hWnd,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

UINT CALLBACK PvpStandardPropPageProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ LPPROPSHEETPAGE ppsp
    );

PPV_PROPCONTEXT HdCreatePropContext(
    _In_ PCWSTR Caption
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
    propSheetHeader.hInstance = NtCurrentImageBase();
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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ LPARAM lParam
    )
{
#define PROPSHEET_ADD_STYLE (WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

    switch (WindowMessage)
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

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);

            context->DefaultWindowProc = PhGetWindowProcedure(WindowHandle);
            PhSetWindowContext(WindowHandle, ULONG_MAX, context);
            PhSetWindowProcedure(WindowHandle, PvpPropSheetWndProc);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 309;
                rect.bottom = 265;
                MapDialogRect(WindowHandle, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }
        }
        break;
    }

    return 0;
}

PPV_PROPSHEETCONTEXT PvpGetPropSheetContext(
    _In_ HWND WindowHandle
    )
{
    return PhGetWindowContext(WindowHandle, ULONG_MAX);
}

LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND hWnd,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PROPSHEETCONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hWnd, ULONG_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (WindowMessage)
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
            PhSetWindowProcedure(hWnd, oldWndProc);

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
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
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

    return CallWindowProc(oldWndProc, hWnd, WindowMessage, wParam, lParam);
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN CALLBACK PvUpdateButtonWindowEnumCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR className[256];

    if (NT_SUCCESS(PhGetClassName(WindowHandle, className, RTL_NUMBER_OF(className), NULL)))
    {
        if (PhEqualStringZ(className, L"#32770", TRUE))
        {
            SendMessage(WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
        }
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
        PhEnumChildWindows(propSheetHandle, PvUpdateButtonWindowEnumCallback, NULL);
    }
}

LRESULT CALLBACK PvControlButtonWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext;
    WNDPROC oldWndProc;

    if (!(propSheetContext = PhGetWindowContext(WindowHandle, SCHAR_MAX)))
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    oldWndProc = propSheetContext->OldOptionsButtonWndProc;

    switch (WindowMessage)
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
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

HWND PvpCreateControlButton(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND PropSheetWindow
    )
{
    if (!PropSheetContext->OptionsButtonWindowHandle)
    {
        HWND buttonHandle;
        RECT clientRect;
        RECT rect;

        buttonHandle = GetDlgItem(PropSheetWindow, IDCANCEL);

        // Create the refresh button.
        if (!PhGetClientRect(PropSheetWindow, &clientRect))
            return NULL;
        if (!PhGetWindowRect(buttonHandle, &rect))
            return NULL;

        PropSheetContext->OldOptionsButtonWndProc = PhGetWindowProcedure(PropSheetWindow);
        PhSetWindowContext(PropSheetWindow, SCHAR_MAX, PropSheetContext);
        PhSetWindowProcedure(PropSheetWindow, PvControlButtonWndProc);

        MapWindowRect(NULL, PropSheetWindow, &rect);
        PropSheetContext->OptionsButtonWindowHandle = PhCreateWindowEx(
            WC_BUTTON,
            L"Options",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            WS_EX_NOPARENTNOTIFY,
            clientRect.right - rect.right,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            PropSheetWindow,
            UlongToHandle(IDABORT),
            PluginInstance->DllBase,
            NULL
            );
        SetWindowFont(PropSheetContext->OptionsButtonWindowHandle, GetWindowFont(buttonHandle), TRUE);
    }

    return PropSheetContext->OptionsButtonWindowHandle;
}

BOOLEAN PhpInitializePropSheetLayoutStage1(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND WindowHandle,
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

        tabControlHandle = PropSheet_GetTabControl(WindowHandle);
        tabControlItem = PhAddLayoutItem(&PropSheetContext->LayoutManager, tabControlHandle, NULL, PH_ANCHOR_ALL | PH_LAYOUT_IMMEDIATE_RESIZE);
        tabPageItem = PhAddLayoutItem(&PropSheetContext->LayoutManager, tabControlHandle, NULL, PH_LAYOUT_TAB_CONTROL); // dummy item to fix multiline tab control
        PhAddLayoutItem(&PropSheetContext->LayoutManager, GetDlgItem(WindowHandle, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        PropSheetContext->TabPageItem = tabPageItem;

        if (EnableOptionsButton)
        {
            PhAddLayoutItem(
                &PropSheetContext->LayoutManager,
                PvpCreateControlButton(PropSheetContext, WindowHandle),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM
                );
        }

        // Hide the OK button.
        ShowWindow(GetDlgItem(WindowHandle, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        PhSetDialogItemText(WindowHandle, IDCANCEL, L"Close");

        if (PositionSettingName)
        {
            PropSheetContext->PositionSettingName = PositionSettingName;
            PropSheetContext->SizeSettingName = SizeSettingName;

            if (PhValidWindowPlacementFromSetting(PositionSettingName))
            {
                PhLoadWindowPlacementFromSetting(PositionSettingName, SizeSettingName, WindowHandle);
            }
            else
            {
                PhCenterWindow(WindowHandle, GetParent(WindowHandle));
            }

            PhSetApplicationWindowIcon(WindowHandle);
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
    propPageContext->PropSheetPage.hInstance = InstanceHandle;
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

UINT CALLBACK PvpStandardPropPageProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    propPageContext = (PPV_PROPPAGECONTEXT)ppsp->lParam;

    if (WindowMessage == PSPCB_ADDREF)
        PhReferenceObject(propPageContext);
    else if (WindowMessage == PSPCB_RELEASE)
        PhDereferenceObject(propPageContext);

    return 1;
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

PPH_LAYOUT_ITEM PvAddPropPageLayoutItemEx(
    _In_ HWND WindowHandle,
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

    parent = GetParent(WindowHandle);
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

#ifdef DEBUG
        ASSERT_DIALOGRECT(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_NETADAPTER_DETAILS), 309, 265);
#endif
        // MAKE SURE THESE NUMBERS ARE CORRECT.
        dialogSize.right = 309;
        dialogSize.bottom = 265;
        MapDialogRect(WindowHandle, &dialogSize);

        // Get the original dialog rectangle.
        PhGetWindowRect(WindowHandle, &dialogRect);
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

PPH_LAYOUT_ITEM PvAddPropPageLayoutItem(
    _In_ HWND WindowHandle,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    return PvAddPropPageLayoutItemEx(WindowHandle, Handle, ParentItem, Anchor, FALSE, NULL, NULL);
}

PPH_LAYOUT_ITEM PvAddPropPageLayoutConfig(
    _In_ HWND WindowHandle,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName
    )
{
    return PvAddPropPageLayoutItemEx(WindowHandle, Handle, ParentItem, Anchor, FALSE, PositionSettingName, SizeSettingName);
}

VOID PvDoPropPageLayout(
    _In_ HWND WindowHandle
    )
{
    HWND parent;
    PPV_PROPSHEETCONTEXT propSheetContext;

    parent = GetParent(WindowHandle);
    propSheetContext = PvpGetPropSheetContext(parent);
    PhLayoutManagerLayout(&propSheetContext->LayoutManager);
}

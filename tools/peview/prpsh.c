/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2021
 *
 */

// NOTE: Copied from processhacker\ProcessHacker\procprp.c

#include <peview.h>
#include <secedit.h>
#include <settings.h>

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
    _In_ LPARAM lParam
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
    propContext->StartPage = PhGetStringSetting(L"MainWindowPage");
    propContext->PropSheetPages = PhAllocate(sizeof(HPROPSHEETPAGE) * PV_PROPCONTEXT_MAXPAGES);

    memset(&propSheetHeader, 0, sizeof(PROPSHEETHEADER));
    propSheetHeader.dwSize = sizeof(PROPSHEETHEADER);
    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = NULL;
    propSheetHeader.pszCaption = propContext->Title->Buffer;
    propSheetHeader.pfnCallback = PvpPropSheetProc;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    propSheetHeader.dwFlags |= PSH_USEPSTARTPAGE;
    propSheetHeader.pStartPage = propContext->StartPage->Buffer;

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
    PhDereferenceObject(propContext->StartPage);
}

static HWND OptionsButton = NULL;
static HWND SecurityButton = NULL;
static WNDPROC OldOptionsWndProc = NULL;
static WNDPROC OldSecurityWndProc = NULL;

LRESULT CALLBACK PvpButtonWndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            if (GET_WM_COMMAND_HWND(wParam, lParam) == OptionsButton)
            {
                PvShowOptionsWindow(hwndDlg);
            }
            else if (GET_WM_COMMAND_HWND(wParam, lParam) == SecurityButton)
            {
                PhEditSecurity(
                    hwndDlg,
                    PhGetString(PvFileName),
                    L"FileObject",
                    PhpOpenFileSecurity,
                    NULL,
                    NULL
                    );
            }
        }
        break;
    }

    return CallWindowProc(OldOptionsWndProc, hwndDlg, uMsg, wParam, lParam);
}

static HWND PvpCreateOptionsButton(
    _In_ HWND hwndDlg
    )
{
    if (!OptionsButton)
    {
        HWND optionsWindow;
        RECT clientRect;
        RECT rect;

        optionsWindow = hwndDlg;
        OldOptionsWndProc = (WNDPROC)GetWindowLongPtr(optionsWindow, GWLP_WNDPROC);
        SetWindowLongPtr(optionsWindow, GWLP_WNDPROC, (LONG_PTR)PvpButtonWndProc);

        // Create the Reset button.
        GetClientRect(optionsWindow, &clientRect);
        GetWindowRect(GetDlgItem(optionsWindow, IDCANCEL), &rect);
        MapWindowPoints(NULL, optionsWindow, (POINT*)& rect, 2);
        OptionsButton = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            WC_BUTTON,
            L"Options",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            clientRect.right - rect.right,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            optionsWindow,
            NULL,
            PhInstanceHandle,
            NULL
            );
        SetWindowFont(OptionsButton, GetWindowFont(GetDlgItem(optionsWindow, IDCANCEL)), TRUE);
    }

    return OptionsButton;
}

static HWND PvpCreateSecurityButton(
    _In_ HWND hwndDlg
    )
{
    if (!SecurityButton)
    {
        HWND optionsWindow;
        RECT clientRect;
        RECT rect;

        optionsWindow = hwndDlg;
        OldSecurityWndProc = (WNDPROC)GetWindowLongPtr(optionsWindow, GWLP_WNDPROC);
        SetWindowLongPtr(optionsWindow, GWLP_WNDPROC, (LONG_PTR)PvpButtonWndProc);

        // Create the Reset button.
        GetClientRect(optionsWindow, &clientRect);
        GetWindowRect(OptionsButton, &rect);
        MapWindowPoints(NULL, optionsWindow, (POINT*)& rect, 2);

        SecurityButton = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            WC_BUTTON,
            L"Security",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            rect.right,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            optionsWindow,
            NULL,
            PhInstanceHandle,
            NULL
            );

        SetWindowFont(SecurityButton, GetWindowFont(GetDlgItem(optionsWindow, IDCANCEL)), TRUE);
    }

    return SecurityButton;
}


static HFONT PvpCreateFont(
    _In_ PWSTR Name,
    _In_ ULONG Size,
    _In_ ULONG Weight,
    _In_ LONG dpiValue
    )
{
    return CreateFont(
        -(LONG)PhMultiplyDivide(Size, dpiValue, 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        Name
        );
}

VOID PvpInitializeFont(
    _In_ HWND hwnd
)
{
    NONCLIENTMETRICS metrics = { sizeof(metrics) };
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(hwnd);

    if (PhApplicationFont)
        DeleteFont(PhApplicationFont);

    if (
        !(PhApplicationFont = PvpCreateFont(L"Microsoft Sans Serif", 8, FW_NORMAL, dpiValue)) &&
        !(PhApplicationFont = PvpCreateFont(L"Tahoma", 8, FW_NORMAL, dpiValue))
        )
    {
        if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
            PhApplicationFont = CreateFontIndirect(&metrics.lfMessageFont);
        else
            PhApplicationFont = NULL;
    }
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
            HICON smallIcon;
            HICON largeIcon;

            PvpInitializeFont(hwndDlg);

            PhGetStockApplicationIcon(&smallIcon, &largeIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);

            context = PhAllocateZero(sizeof(PV_PROPSHEETCONTEXT));
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PvpPropSheetWndProc);

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

            if (PhEnableThemeSupport)
                PhInitializeWindowTheme(hwndDlg, TRUE);
        }
        break;
    }

    return 0;
}

PPV_PROPSHEETCONTEXT PvpGetPropSheetContext(
    _In_ HWND hwnd
    )
{
    return PhGetWindowContext(hwnd, UCHAR_MAX);
}

LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext;
    WNDPROC oldWndProc;

    if (!(propSheetContext = PhGetWindowContext(hWnd, UCHAR_MAX)))
        return 0;

    oldWndProc = propSheetContext->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            HWND tabControl;
            TCITEM tabItem;
            WCHAR text[128] = L"";

            // Save the window position and size.

            PhSaveWindowPlacementToSetting(L"MainWindowPosition", L"MainWindowSize", hWnd);

            // Save the selected tab.

            tabControl = PropSheet_GetTabControl(hWnd);

            tabItem.mask = TCIF_TEXT;
            tabItem.pszText = text;
            tabItem.cchTextMax = RTL_NUMBER_OF(text) - sizeof(UNICODE_NULL);

            if (TabCtrl_GetItem(tabControl, TabCtrl_GetCurSel(tabControl), &tabItem))
            {
                PhSetStringSetting(L"MainWindowPage", text);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hWnd, UCHAR_MAX);

            PhDeleteLayoutManager(&propSheetContext->LayoutManager);
            PhFree(propSheetContext);
        }
        break;
    case WM_DPICHANGED:
        {
            PvpInitializeFont(hWnd);
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
            if (!IsMinimized(hWnd))
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

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
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
        tabControlItem = PhAddLayoutItem(&PropSheetContext->LayoutManager, tabControlHandle, NULL, PH_ANCHOR_ALL | PH_LAYOUT_IMMEDIATE_RESIZE);
        tabPageItem = PhAddLayoutItem(&PropSheetContext->LayoutManager, tabControlHandle, NULL, PH_LAYOUT_TAB_CONTROL); // dummy item to fix multiline tab control

        PropSheetContext->TabPageItem = tabPageItem;

        PhAddLayoutItem(&PropSheetContext->LayoutManager, GetDlgItem(hwnd, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        PhAddLayoutItem(&PropSheetContext->LayoutManager, PvpCreateOptionsButton(hwnd), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
        PhAddLayoutItem(&PropSheetContext->LayoutManager, PvpCreateSecurityButton(hwnd), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

        // Hide the OK button.
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
        // Set the Cancel button's text to "Close".
        PhSetDialogItemText(hwnd, IDCANCEL, L"Close");

        PropSheetContext->LayoutInitialized = TRUE;

        return TRUE;
    }

    return FALSE;
}

VOID PhpInitializePropSheetLayoutStage2(
    _In_ HWND hwnd
    )
{
    PH_RECTANGLE windowRectangle;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(hwnd);

    windowRectangle.Position = PhGetIntegerPairSetting(L"MainWindowPosition");
    windowRectangle.Size = PhGetScalableIntegerPairSetting(L"MainWindowSize", TRUE, dpiValue).Pair;

    if (!windowRectangle.Position.X)
        return;

    if (windowRectangle.Size.X < MinimumSize.right)
        windowRectangle.Size.X = MinimumSize.right;
    if (windowRectangle.Size.Y < MinimumSize.bottom)
        windowRectangle.Size.Y = MinimumSize.bottom;

    PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

    MoveWindow(hwnd, windowRectangle.Left, windowRectangle.Top,
        windowRectangle.Width, windowRectangle.Height, FALSE);

    // Implement cascading by saving an offsetted rectangle.
    windowRectangle.Left += 20;
    windowRectangle.Top += 20;

    PhSetIntegerPairSetting(L"MainWindowPosition", windowRectangle.Position);
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

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] = propSheetPageHandle;
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
    propPageContext->PropSheetPage.dwFlags = PSP_USECALLBACK;
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
    BOOLEAN doLayoutStage2;
    PPH_LAYOUT_ITEM item;

    parent = GetParent(hwnd);
    propSheetContext = PvpGetPropSheetContext(parent);
    layoutManager = &propSheetContext->LayoutManager;

    doLayoutStage2 = PhpInitializePropSheetLayoutStage1(propSheetContext, parent);

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

    if (doLayoutStage2)
        PhpInitializePropSheetLayoutStage2(parent);

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

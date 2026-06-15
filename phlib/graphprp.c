/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 */

#include <ph.h>
#include <guisup.h>
#include <settings.h>
#include <tabnew.h>
#include <graphprp.h>

/*
 * Custom resizable PropertySheet replacement built on PhTabNew (dmex)
 *
 *  - Tab control and page dialogs are *siblings* under the host window. The
 *    page rect is computed via PhTabNew_GetPageRect (parent-client coords).
 *  - Page dialogs are created lazily on first activation (unless the page
 *    sets PH_PROPSHEETNEW_PAGE_EAGER).
 *  - Page switching uses a single BeginDeferWindowPos batch with
 *    SWP_SHOWWINDOW_ONLY / SWP_HIDEWINDOW_ONLY, mirroring the main window's
 *    tab pattern (SystemInformer/mainwnd.c) to avoid flicker.
 *  - Min size: caller-provided, or 290x320 DLU mapped to pixels via
 *    MapDialogRect (matches procprp.c default).
 *  - Placement and active-page persistence are opt-in via flags.
 */

typedef struct _PH_PROPSHEETNEW_CONTEXT
{
    PH_PROPSHEETNEW Sheet;         // shallow copy of caller's descriptor
    HWND WindowHandle;             // host
    HWND TabControl;               // PhTabNew, sibling of pages
    HWND CloseButton;              // optional, when PH_PROPSHEETNEW_CLOSE_BUTTON set
    HFONT Font;                    // owned chrome font

    PPH_PROPSHEETNEW_PAGE Pages;   // owned mirror of Sheet.Pages
    ULONG PageCount;
    INT CurrentIndex;              // -1 until first activation

    SIZE MinimumSize96;            // 96-DPI baseline pixels
    SIZE MinimumSize;
    LONG WindowDpi;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Modal : 1;
            ULONG Initialized : 1;
            ULONG QuitRequested : 1;
            ULONG LayoutInProgress : 1;
            ULONG Spare : 28;
        };
    };

    INT_PTR ModalResult;
} PH_PROPSHEETNEW_CONTEXT, *PPH_PROPSHEETNEW_CONTEXT;

BOOLEAN PhPropSheetNewClassRegistered = FALSE;

LRESULT CALLBACK PhPropSheetNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#define PH_PROPSHEETNEW_PADDING       7   // inner padding (px @ 96 DPI) around content
#define PH_PROPSHEETNEW_BTN_HEIGHT    23
#define PH_PROPSHEETNEW_BTN_WIDTH     84

// Page-switch DeferWindowPos flag combo (phlib has no SWP_SHOWWINDOW_ONLY of its own).
#define PHP_SWP_NOMOVE_NOSIZE     (SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)
#define PHP_SWP_SHOWWINDOW_ONLY   (PHP_SWP_NOMOVE_NOSIZE | SWP_SHOWWINDOW)
#define PHP_SWP_HIDEWINDOW_ONLY   (PHP_SWP_NOMOVE_NOSIZE | SWP_HIDEWINDOW)

// ---------------------------------------------------------------------------
// Class registration
// ---------------------------------------------------------------------------

RTL_ATOM PhPropSheetNewRegisterClass(
    VOID
    )
{
    WNDCLASSEX wcex;

    if (PhPropSheetNewClassRegistered)
        return RTL_ATOM_INVALID_ATOM;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wcex.lpfnWndProc = PhPropSheetNewWndProc;
    wcex.cbWndExtra = sizeof(PVOID);  // for PhSetWindowContextEx
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = PH_PROPSHEETNEW_CLASSNAME;

    PhPropSheetNewClassRegistered = TRUE;

    return RegisterClassEx(&wcex);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

LONG PhPropSheetNewScaleDpi(
    _In_ LONG Value96,
    _In_ LONG Dpi
    )
{
    if (Dpi <= 0 || Dpi == USER_DEFAULT_SCREEN_DPI)
        return Value96;
    return PhMultiplyDivideSigned(Value96, Dpi, USER_DEFAULT_SCREEN_DPI);
}

VOID PhPropSheetNewUpdateMinimumSize(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context
    )
{
    Context->MinimumSize.cx = PhPropSheetNewScaleDpi(Context->MinimumSize96.cx, Context->WindowDpi);
    Context->MinimumSize.cy = PhPropSheetNewScaleDpi(Context->MinimumSize96.cy, Context->WindowDpi);
}

VOID PhPropSheetNewInitializeMinimumSize(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context
    )
{
    HDC hdc;
    HFONT oldFont = NULL;
    TEXTMETRIC tm;
    LONG cx = 290 * 2;   // ~520 px fallback at 96 DPI for Segoe UI 9pt
    LONG cy = 320 * 13 / 8;

    if (Context->Sheet.MinimumSize.cx > 0 && Context->Sheet.MinimumSize.cy > 0)
    {
        Context->MinimumSize96 = Context->Sheet.MinimumSize;
        PhPropSheetNewUpdateMinimumSize(Context);
        return;
    }

    // Convert the legacy 290x320 DLU baseline (matches procprp.c) to pixels
    // using the application font's metrics. MapDialogRect requires a real
    // dialog window — the host isn't one, so apply the DLU formula directly:
    //   pxX = MulDiv(dlu, avgCharWidth, 4)
    //   pxY = MulDiv(dlu, charHeight,   8)
    // against PhApplicationFont (Segoe UI 9pt typically), which gives the
    // same answer the dialog manager would.

    if (hdc = GetDC(Context->WindowHandle))
    {
        if (PhApplicationFont)
        {
            oldFont = SelectFont(hdc, PhApplicationFont);
        }

        if (GetTextMetrics(hdc, &tm))
        {
            LONG avgCharWidth = tm.tmAveCharWidth > 0 ? tm.tmAveCharWidth : 7;
            LONG charHeight = tm.tmHeight > 0 ? tm.tmHeight : 13;

            cx = PhMultiplyDivideSigned(290, avgCharWidth, 4);
            cy = PhMultiplyDivideSigned(320, charHeight, 8);

            if (Context->WindowDpi > 0 && Context->WindowDpi != USER_DEFAULT_SCREEN_DPI)
            {
                cx = PhMultiplyDivideSigned(cx, USER_DEFAULT_SCREEN_DPI, Context->WindowDpi);
                cy = PhMultiplyDivideSigned(cy, USER_DEFAULT_SCREEN_DPI, Context->WindowDpi);
            }
        }

        if (oldFont)
            SelectFont(hdc, oldFont);

        ReleaseDC(Context->WindowHandle, hdc);
    }

    Context->MinimumSize96.cx = cx;
    Context->MinimumSize96.cy = cy;
    PhPropSheetNewUpdateMinimumSize(Context);
}

VOID PhPropSheetNewUpdateFont(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context
    )
{
    HFONT newFont;

    if (!Context->CloseButton)
        return;

    newFont = PhCreateCommonFont(
        -11,
        FW_NORMAL,
        Context->WindowHandle,
        Context->WindowDpi
        );

    if (newFont)
    {
        if (Context->Font)
            DeleteFont(Context->Font);
        Context->Font = newFont;
    }

    if (Context->Font)
        SetWindowFont(Context->CloseButton, Context->Font, TRUE);
}

PPH_PROPSHEETNEW_PAGE PhPropSheetNewPageAt(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context,
    _In_ INT TabIndex
    )
{
    if (TabIndex < 0 || (ULONG)TabIndex >= Context->PageCount)
        return NULL;

    return &Context->Pages[TabIndex];
}

VOID PhPropSheetNewCreatePageDialog(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context,
    _In_ PPH_PROPSHEETNEW_PAGE Page
    )
{
    HWND windowHandle;

    if (Page->DialogHandle || !Page->DialogProc || !Page->Template)
        return;

    windowHandle = PhCreateDialogFromTemplate(
        Context->WindowHandle,
        DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD | WS_CLIPSIBLINGS,
        Page->Instance,
        Page->Template,
        Page->DialogProc,
        Page->Parameter
        );

    if (!windowHandle)
        return;

    // Reparent the dialog as a child of the host window. Dialogs
    // from PROPSHEETPAGE-style templates are typically created with
    // WS_POPUP|WS_CAPTION|WS_SYSMENU which we strip here. WS_EX_CONTROLPARENT
    // makes tab navigation traverse into the page.
    //SetWindowLongPtr(hwnd, GWL_STYLE, (GetWindowLongPtr(hwnd, GWL_STYLE) & ~(WS_POPUP | WS_CAPTION | WS_SYSMENU)) | WS_CHILD | WS_CLIPSIBLINGS);

    SetWindowLongPtr(windowHandle, GWL_EXSTYLE, GetWindowLongPtr(windowHandle, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);

    SetParent(windowHandle, Context->WindowHandle);

    Page->DialogHandle = windowHandle;

    //PhInitializeWindowTheme(windowHandle, PhEnableThemeSupport);
}

VOID PhPropSheetNewGetPageRect(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context,
    _Out_ PRECT PageRect
    )
{
    if (Context->TabControl)
    {
        PhTabNew_GetPageRect(Context->TabControl, PageRect);
    }
    else
    {
        GetClientRect(Context->WindowHandle, PageRect);
    }
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhPropSheetNewSendDpiChangedAfterParentCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PMSG message = (PMSG)Context;

    SendMessage(WindowHandle, WM_DPICHANGED_AFTERPARENT, message->wParam, message->lParam);

    return TRUE;
}

VOID PhPropSheetNewSendDpiChangedAfterParent(
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    MSG message;

    memset(&message, 0, sizeof(MSG));
    message.wParam = wParam;
    message.lParam = lParam;

    PhEnumChildWindows(WindowHandle, PhPropSheetNewSendDpiChangedAfterParentCallback, &message);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

LONG PhPropSheetNewScale(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context,
    _In_ LONG Value96
    )
{
    return PhPropSheetNewScaleDpi(Value96, Context->WindowDpi);
}

VOID PhPropSheetNewLayout(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context
    )
{
    RECT clientRect;
    RECT tabRect;
    RECT pageRect;
    PPH_PROPSHEETNEW_PAGE current;
    HDWP defer;
    LONG padding;
    LONG buttonHeight;
    LONG buttonWidth;
    BOOLEAN hasCloseButton = (Context->Sheet.Flags & PH_PROPSHEETNEW_CLOSE_BUTTON) != 0;
    BOOLEAN tabResized = FALSE;
    BOOLEAN deferFailed = FALSE;

    if (!Context->Initialized)
        return;
    if (Context->LayoutInProgress)
        return;

    padding = PhPropSheetNewScale(Context, PH_PROPSHEETNEW_PADDING);
    buttonHeight = PhPropSheetNewScale(Context, PH_PROPSHEETNEW_BTN_HEIGHT);
    buttonWidth = PhPropSheetNewScale(Context, PH_PROPSHEETNEW_BTN_WIDTH);

    GetClientRect(Context->WindowHandle, &clientRect);
    current = PhPropSheetNewPageAt(Context, Context->CurrentIndex);

    // Tab area: client rect minus padding on all sides, minus the bottom
    // button strip if any.
    tabRect = clientRect;
    tabRect.left += padding;
    tabRect.top += padding;
    tabRect.right -= padding;
    tabRect.bottom -= padding;
    if (hasCloseButton)
        tabRect.bottom -= buttonHeight + padding;

    Context->LayoutInProgress = TRUE;

    // PhTabNew_GetPageRect returns the tab control's cached page rectangle.
    // Resize the tab synchronously first so the active page never receives
    // the previous layout pass' page bounds during live resize/DPI changes.
    if (Context->TabControl)
    {
        tabResized = !!SetWindowPos(
            Context->TabControl,
            NULL,
            tabRect.left,
            tabRect.top,
            tabRect.right - tabRect.left,
            tabRect.bottom - tabRect.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW
            );
    }

    if (current && current->DialogHandle)
    {
        PhPropSheetNewGetPageRect(Context, &pageRect);
    }

    defer = BeginDeferWindowPos(2);
    deferFailed = !defer;

    if (hasCloseButton && Context->CloseButton)
    {
        LONG btnX = clientRect.right - padding - buttonWidth;
        LONG btnY = clientRect.bottom - padding - buttonHeight;

        if (defer)
        {
            HDWP nextDefer = DeferWindowPos(
                defer,
                Context->CloseButton,
                NULL,
                btnX,
                btnY,
                buttonWidth,
                buttonHeight,
                SWP_NOZORDER | SWP_NOACTIVATE
                );

            if (nextDefer)
                defer = nextDefer;
            else
            {
                defer = NULL;
                deferFailed = TRUE;
            }
        }
    }

    if (current && current->DialogHandle)
    {
        if (defer)
        {
            HDWP nextDefer = DeferWindowPos(
                defer,
                current->DialogHandle,
                NULL,
                pageRect.left,
                pageRect.top,
                pageRect.right - pageRect.left,
                pageRect.bottom - pageRect.top,
                SWP_NOZORDER | SWP_NOACTIVATE
                );

            if (nextDefer)
                defer = nextDefer;
            else
            {
                defer = NULL;
                deferFailed = TRUE;
            }
        }
    }

    if (defer && !EndDeferWindowPos(defer))
        deferFailed = TRUE;

    if (deferFailed)
    {
        if (hasCloseButton && Context->CloseButton)
        {
            LONG btnX = clientRect.right - padding - buttonWidth;
            LONG btnY = clientRect.bottom - padding - buttonHeight;

            SetWindowPos(
                Context->CloseButton,
                NULL,
                btnX,
                btnY,
                buttonWidth,
                buttonHeight,
                SWP_NOZORDER | SWP_NOACTIVATE
                );
        }

        if (current && current->DialogHandle)
        {
            SetWindowPos(
                current->DialogHandle,
                NULL,
                pageRect.left,
                pageRect.top,
                pageRect.right - pageRect.left,
                pageRect.bottom - pageRect.top,
                SWP_NOZORDER | SWP_NOACTIVATE
                );
        }
    }

    if (tabResized)
    {
        InvalidateRect(
            Context->TabControl,
            NULL,
            FALSE
            );
    }

    Context->LayoutInProgress = FALSE;
}

// ---------------------------------------------------------------------------
// Page switching — DeferWindowPos batch mirroring mainwnd.c
// ---------------------------------------------------------------------------

VOID PhPropSheetNewSelectPage(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context,
    _In_ INT NewIndex
    )
{
    HDWP defer;
    RECT pageRect;
    PPH_PROPSHEETNEW_PAGE current;
    ULONG i;

    if (NewIndex < 0 || (ULONG)NewIndex >= Context->PageCount)
        return;
    if (NewIndex == Context->CurrentIndex)
        return;

    current = &Context->Pages[NewIndex];

    // Lazy-create the incoming page's dialog before the batch so we can
    // position it inside the same DeferWindowPos pass.
    if (!current->DialogHandle)
    {
        PhPropSheetNewCreatePageDialog(Context, current);
    }

    PhPropSheetNewGetPageRect(Context, &pageRect);

    defer = BeginDeferWindowPos(Context->PageCount + 1);

    for (i = 0; i < Context->PageCount; i++)
    {
        PPH_PROPSHEETNEW_PAGE page = &Context->Pages[i];

        if (!page->DialogHandle)
            continue;

        if ((INT)i == NewIndex)
        {
            // Reposition the new page outside the deferred batch via
            // MoveWindow. Avoids cases where queuing the move alongside
            // sibling SWP_HIDEWINDOW calls in DeferWindowPos can drop the
            // reposition silently, and guarantees the page reflows before
            // the explicit ShowWindow below fires WM_SHOWWINDOW (where the
            // procprp page DlgProcs install their layout items).
            MoveWindow(
                page->DialogHandle,
                pageRect.left,
                pageRect.top,
                pageRect.right - pageRect.left,
                pageRect.bottom - pageRect.top,
                TRUE
                );
        }
        else
        {
            defer = DeferWindowPos(
                defer,
                page->DialogHandle,
                NULL,
                0, 0, 0, 0,
                PHP_SWP_HIDEWINDOW_ONLY
                );
        }
    }

    EndDeferWindowPos(defer);

    Context->CurrentIndex = NewIndex;

    // Show the now-positioned new page explicitly so WM_SHOWWINDOW fires.
    if (current->DialogHandle)
    {
        ShowWindow(current->DialogHandle, SW_SHOW);
        SetFocus(current->DialogHandle);
    }
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

VOID PhPropSheetNewRestoreState(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context
    )
{
    ULONG initialIndex = 0;

    if (FlagOn(Context->Sheet.Flags, PH_PROPSHEETNEW_SAVE_PLACEMENT) &&
        (Context->Sheet.SettingNamePosition || Context->Sheet.SettingNameSize))
    {
        PhLoadWindowPlacementFromSetting(
            Context->Sheet.SettingNamePosition,
            Context->Sheet.SettingNameSize,
            Context->WindowHandle
            );
    }
    else if (Context->Sheet.Flags & PH_PROPSHEETNEW_CENTER)
    {
        PhCenterWindow(Context->WindowHandle, Context->Sheet.ParentWindow);
    }

    if (FlagOn(Context->Sheet.Flags, PH_PROPSHEETNEW_SAVE_ACTIVE_PAGE) &&
        Context->Sheet.SettingNameActivePage)
    {
        PPH_STRING saved = PhGetStringSetting(Context->Sheet.SettingNameActivePage);

        if (saved && !PhIsNullOrEmptyString(saved))
        {
            for (ULONG i = 0; i < Context->PageCount; i++)
            {
                if (Context->Pages[i].Name &&
                    PhEqualStringZ(saved->Buffer, (PWSTR)Context->Pages[i].Name, TRUE))
                {
                    initialIndex = (INT)i;
                    break;
                }
            }
        }

        if (saved)
        {
            PhDereferenceObject(saved);
        }
    }

    // Drive the standard selection path so the page gets created/positioned.
    if (Context->PageCount > 0)
    {
        Context->CurrentIndex = -1;
        if (Context->TabControl)
            PhTabNew_SetCurSel(Context->TabControl, initialIndex);
        PhPropSheetNewSelectPage(Context, initialIndex);
    }
}

VOID PhPropSheetNewSaveState(
    _In_ PPH_PROPSHEETNEW_CONTEXT Context
    )
{
    if (FlagOn(Context->Sheet.Flags, PH_PROPSHEETNEW_SAVE_PLACEMENT) &&
        (Context->Sheet.SettingNamePosition || Context->Sheet.SettingNameSize))
    {
        PhSaveWindowPlacementToSetting(
            Context->Sheet.SettingNamePosition,
            Context->Sheet.SettingNameSize,
            Context->WindowHandle
            );
    }

    if ((FlagOn(Context->Sheet.Flags, PH_PROPSHEETNEW_SAVE_ACTIVE_PAGE)) &&
        Context->Sheet.SettingNameActivePage)
    {
        PPH_PROPSHEETNEW_PAGE current = PhPropSheetNewPageAt(Context, Context->CurrentIndex);

        if (current && current->Name)
        {
            PhSetStringSetting(Context->Sheet.SettingNameActivePage, current->Name);
        }
    }
}

LRESULT CALLBACK PhPropSheetNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROPSHEETNEW_CONTEXT context = (PPH_PROPSHEETNEW_CONTEXT)PhGetWindowContextEx(WindowHandle);

    switch (WindowMessage)
    {
    case PSM_GETTABCONTROL:
        return (LRESULT)(context ? context->TabControl : NULL);
    case PSM_GETCURRENTPAGEHWND:
        {
            PPH_PROPSHEETNEW_PAGE page;

            if (!context)
                return (LRESULT)NULL;

            page = PhPropSheetNewPageAt(context, context->CurrentIndex);
            return (LRESULT)(page ? page->DialogHandle : NULL);
        }
    case WM_NCCREATE:
        {
            CREATESTRUCT *cs = (CREATESTRUCT *)lParam;

            context = (PPH_PROPSHEETNEW_CONTEXT)cs->lpCreateParams;
            context->WindowHandle = WindowHandle;
            context->WindowDpi = PhGetWindowDpi(WindowHandle);
            context->CurrentIndex = -1;

            PhSetWindowContextEx(WindowHandle, context);
        }
        break;
    case WM_CREATE:
        {
            ULONG tabStyle;
            ULONG i;

            PhPropSheetNewInitializeMinimumSize(context);

            tabStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
            tabStyle |= (context->Sheet.Layout == PhPropSheetNewLayoutLeft) ? TNS_LEFT : TNS_TOP;

            context->TabControl = PhCreateWindow(
                PH_TABNEW_CLASSNAME,
                NULL,
                tabStyle,
                0, 0, 0, 0,
                WindowHandle,
                NULL,
                NtCurrentImageBase(),
                NULL
                );

            if (context->TabControl)
            {
                PhTabNew_SetSkin(context->TabControl, context->Sheet.Skin);
            }

            // Optional Close button (anchored bottom-right in PhPropSheetNewLayout).
            if (FlagOn(context->Sheet.Flags, PH_PROPSHEETNEW_CLOSE_BUTTON))
            {
                context->CloseButton = PhCreateWindow(
                    WC_BUTTON,
                    L"Close",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    0, 0, 0, 0,
                    WindowHandle,
                    (HMENU)IDCANCEL,
                    NtCurrentImageBase(),
                    NULL
                    );

                PhPropSheetNewUpdateFont(context);
            }

            context->Initialized = TRUE;

            // Position the tab + Close button at their initial inset rects
            // BEFORE firing the Initialized callback. Callers that register
            // these windows with a layout manager (e.g. procprp) need them
            // already sized so the captured anchor margins are correct —
            // otherwise the manager will shrink them on every resize.
            PhPropSheetNewLayout(context);

            // Fire the post-create callback BEFORE any pages are created so
            // the caller can set up shared state (layout manager, theming,
            // etc.) that page WM_INITDIALOG handlers depend on.
            if (context->Sheet.Initialized)
                context->Sheet.Initialized(WindowHandle, context->Sheet.Context);

            // Insert tab strip items (no dialogs created yet).
            if (context->TabControl)
            {
                SendMessage(context->TabControl, WM_SETREDRAW, FALSE, 0);

                for (i = 0; i < context->PageCount; i++)
                {
                    PPH_PROPSHEETNEW_PAGE page = &context->Pages[i];
                    PH_TABNEW_INSERTITEM item;

                    item.Text = (PWSTR)page->Name;
                    item.ImageIndex = -1;
                    item.Param = (LPARAM)page;

                    PhTabNew_InsertItem(context->TabControl, -1, &item);
                }

                SendMessage(context->TabControl, WM_SETREDRAW, TRUE, 0);
            }

            // Eager page dialog creation (page WM_INITDIALOG runs here).
            for (i = 0; i < context->PageCount; i++)
            {
                PPH_PROPSHEETNEW_PAGE page = &context->Pages[i];

                if (page->Flags & PH_PROPSHEETNEW_PAGE_EAGER)
                {
                    PhPropSheetNewCreatePageDialog(context, page);
                    if (page->DialogHandle)
                        ShowWindow(page->DialogHandle, SW_HIDE);
                }
            }

            PhPropSheetNewLayout(context);
            PhPropSheetNewRestoreState(context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(WindowHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhPropSheetNewSaveState(context);

            for (ULONG i = 0; i < context->PageCount; i++)
            {
                if (context->Pages[i].DialogHandle)
                {
                    DestroyWindow(context->Pages[i].DialogHandle);
                    context->Pages[i].DialogHandle = NULL;
                }
            }

            if (context->Font)
            {
                DeleteFont(context->Font);
                context->Font = NULL;
            }
        }
        break;
    case WM_SIZE:
        {
            if (!IsMinimized(WindowHandle))
            {
                PhPropSheetNewLayout(context);
            }
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize(
                (PRECT)lParam,
                wParam,
                context->MinimumSize.cx,
                context->MinimumSize.cy
                );
        }
        return TRUE;
    case WM_DPICHANGED:
        {
            PRECT newRect = (PRECT)lParam;
            LONG width;
            LONG height;

            context->WindowDpi = LOWORD(wParam);
            PhPropSheetNewUpdateMinimumSize(context);
            PhPropSheetNewUpdateFont(context);

            width = newRect->right - newRect->left;
            height = newRect->bottom - newRect->top;

            if (width < context->MinimumSize.cx)
                width = context->MinimumSize.cx;
            if (height < context->MinimumSize.cy)
                height = context->MinimumSize.cy;

            SetWindowPos(
                WindowHandle,
                NULL,
                newRect->left,
                newRect->top,
                width,
                height,
                SWP_NOZORDER | SWP_NOACTIVATE
                );

            PhPropSheetNewSendDpiChangedAfterParent(WindowHandle, wParam, lParam);
            PhPropSheetNewLayout(context);
        }
        return 0;
    case WM_NOTIFY:
        {
            NMHDR *hdr = (NMHDR *)lParam;

            if (hdr->hwndFrom == context->TabControl)
            {
                switch (hdr->code)
                {
                case PHTNN_SELCHANGED:
                    {
                        INT sel = PhTabNew_GetCurSel(context->TabControl);

                        PhPropSheetNewSelectPage(context, sel);
                    }
                    break;
                case PHTNN_LAYOUT:
                    {
                        if (!context->LayoutInProgress)
                        {
                            PhPropSheetNewLayout(context);
                        }
                    }
                    break;
                }
            }
        }
        break;
    case WM_COMMAND:
        {
            if (GET_WM_COMMAND_HWND(wParam, lParam) == context->CloseButton && context->Sheet.Flags & PH_PROPSHEETNEW_CLOSE_BUTTON)
            {
                //PostMessage(WindowHandle, WM_CLOSE, 0, 0);
                PostQuitMessage(0);
                return 0;
            }
        }
        break;
    case WM_CLOSE:
        {
            context->ModalResult = IDCANCEL;
            context->QuitRequested = TRUE;
            DestroyWindow(WindowHandle);
        }
        return 0;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

HWND PhPropSheetNewCreate(
    _In_ PPH_PROPSHEETNEW Sheet,
    _In_ BOOLEAN Modal,
    _Out_opt_ PPH_PROPSHEETNEW_CONTEXT* OutContext
    )
{
    PPH_PROPSHEETNEW_CONTEXT context;
    HWND hwnd;
    ULONG style;
    LONG initialCx;
    LONG initialCy;
    LONG initialDpi;
    ULONG i;

    if (Sheet->PageCount == 0 || !Sheet->Pages)
        return NULL;

    PhPropSheetNewRegisterClass();

    context = PhAllocateZero(sizeof(PH_PROPSHEETNEW_CONTEXT));
    context->Sheet = *Sheet;
    context->Modal = !!Modal;
    context->PageCount = Sheet->PageCount;
    context->CurrentIndex = -1;

    context->Pages = PhAllocateZero(sizeof(PH_PROPSHEETNEW_PAGE) * context->PageCount);
    for (i = 0; i < context->PageCount; i++)
        context->Pages[i] = Sheet->Pages[i];

    if (Sheet->ParentWindow)
        initialDpi = PhGetWindowDpi(Sheet->ParentWindow);
    else
        initialDpi = PhGetMonitorDpi(NULL, NULL);

    initialCx = PhPropSheetNewScaleDpi(Sheet->InitialSize.cx > 0 ? Sheet->InitialSize.cx : 520, initialDpi);
    initialCy = PhPropSheetNewScaleDpi(Sheet->InitialSize.cy > 0 ? Sheet->InitialSize.cy : 420, initialDpi);

    style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;

    if (FlagOn(Sheet->Flags, PH_PROPSHEETNEW_RESIZABLE))
    {
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
    }

    hwnd = PhCreateWindowEx(
        PH_PROPSHEETNEW_CLASSNAME,
        Sheet->Caption,
        style,
        WS_EX_CONTROLPARENT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        initialCx,
        initialCy,
        Sheet->ParentWindow,
        NULL,
        NtCurrentImageBase(),
        context
        );

    if (!hwnd)
    {
        PhFree(context->Pages);
        PhFree(context);
        if (OutContext)
            *OutContext = NULL;
        return NULL;
    }

    if (Sheet->Icon && !(Sheet->Flags & PH_PROPSHEETNEW_NOICON))
    {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)Sheet->Icon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)Sheet->Icon);
    }

    if (OutContext)
        *OutContext = context;

    return hwnd;
}

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

HWND PhPropSheetNewShow(
    _In_ PPH_PROPSHEETNEW Sheet
    )
{
    HWND windowHandle;

    if (windowHandle = PhPropSheetNewCreate(Sheet, FALSE, NULL))
    {
        ShowWindow(windowHandle, SW_SHOW);
        UpdateWindow(windowHandle);
    }

    return windowHandle;
}

INT_PTR PhPropSheetNewShowModal(
    _In_ PPH_PROPSHEETNEW Sheet
    )
{
    PPH_PROPSHEETNEW_CONTEXT context;
    HWND hwnd;
    HWND parentOwner;
    MSG msg;
    INT_PTR result;

    hwnd = PhPropSheetNewCreate(Sheet, TRUE, &context);
    if (!hwnd || !context)
        return -1;

    parentOwner = Sheet->ParentWindow;
    if (parentOwner)
        EnableWindow(parentOwner, FALSE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (context->QuitRequested && !IsWindow(hwnd))
            break;

        if (context->Sheet.PreTranslateMessage &&
            context->Sheet.PreTranslateMessage(hwnd, &msg, context->Sheet.Context))
        {
            // Message consumed by the caller.
        }
        else if (!IsDialogMessage(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (context->QuitRequested && !IsWindow(hwnd))
            break;
    }

    if (parentOwner)
    {
        EnableWindow(parentOwner, TRUE);
        SetForegroundWindow(parentOwner);
    }

    result = context->ModalResult;

    PhFree(context->Pages);
    PhFree(context);

    return result;
}

HWND PhPropSheetNewGetCurrentPage(
    _In_ HWND HostHandle
    )
{
    PPH_PROPSHEETNEW_CONTEXT context = (PPH_PROPSHEETNEW_CONTEXT)PhGetWindowContextEx(HostHandle);
    PPH_PROPSHEETNEW_PAGE page;

    if (!context)
        return NULL;

    page = PhPropSheetNewPageAt(context, context->CurrentIndex);
    return page ? page->DialogHandle : NULL;
}

HWND PhPropSheetNewGetTabControl(
    _In_ HWND HostHandle
    )
{
    PPH_PROPSHEETNEW_CONTEXT context = (PPH_PROPSHEETNEW_CONTEXT)PhGetWindowContextEx(HostHandle);

    return context ? context->TabControl : NULL;
}

BOOLEAN PhPropSheetNewSetCurrentPageByName(
    _In_ HWND HostHandle,
    _In_ PCWSTR Name
    )
{
    PPH_PROPSHEETNEW_CONTEXT context = (PPH_PROPSHEETNEW_CONTEXT)PhGetWindowContextEx(HostHandle);
    ULONG i;

    if (!context || !Name)
        return FALSE;

    for (i = 0; i < context->PageCount; i++)
    {
        if (context->Pages[i].Name &&
            PhEqualStringZ((PWSTR)context->Pages[i].Name, (PWSTR)Name, TRUE))
        {
            if (context->TabControl)
                PhTabNew_SetCurSel(context->TabControl, (INT)i);
            PhPropSheetNewSelectPage(context, (INT)i);
            return TRUE;
        }
    }

    return FALSE;
}

PVOID PhPropSheetNewGetContext(
    _In_ HWND HostHandle
    )
{
    PPH_PROPSHEETNEW_CONTEXT context = (PPH_PROPSHEETNEW_CONTEXT)PhGetWindowContextEx(HostHandle);

    return context ? context->Sheet.Context : NULL;
}

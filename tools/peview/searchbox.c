/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2022
 *     jxy-s   2023
 *
 */

#include <peview.h>
#include <vssym32.h>
//#include <wincodec.h>
#include <emenu.h>

#include "../../tools/thirdparty/pcre/pcre2.h"

typedef struct _PV_SEARCHCONTROL_BUTTON
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG Pushed : 1;
            ULONG Active : 1;
            ULONG Error : 1;
            ULONG Spare : 29;
        };
    };

    ULONG Index;
    ULONG ImageIndex;
    ULONG ActiveImageIndex;
} PV_SEARCHCONTROL_BUTTON, *PPV_SEARCHCONTROL_BUTTON;

#define PV_SC_BUTTON_COUNT 3

typedef struct _PV_SEARCHCONTROL_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG ThemeSupport : 1;
            ULONG Hot : 1;
            ULONG HotTrack : 1;
            ULONG ColorMode : 8;
            ULONG UseSearchPointer : 1;
            ULONG Spare : 20;
        };
    };

    PV_SEARCHCONTROL_BUTTON SearchButton;
    PV_SEARCHCONTROL_BUTTON RegexButton;
    PV_SEARCHCONTROL_BUTTON CaseButton;

    LONG CXWidth;
    INT CXBorder;
    INT ImageWidth;
    INT ImageHeight;
    WNDPROC DefaultWindowProc;
    HFONT WindowFont;
    HIMAGELIST ImageListHandle;
    PPH_STRING CueBannerText;

    PPV_SEARCHCONTROL_CALLBACK Callback;
    PVOID CallbackContext;

    PPH_STRING SearchboxText;
    ULONG64 SearchPointer;
    INT SearchboxRegexError;
    PCRE2_SIZE SearchboxRegexErrorOffset;
    pcre2_code* SearchboxRegexCode;
    pcre2_match_data* SearchboxRegexMatchData;
} PV_SEARCHCONTROL_CONTEXT, *PPV_SEARCHCONTROL_CONTEXT;

VOID PvpSearchFreeTheme(
    _Inout_ PPV_SEARCHCONTROL_CONTEXT Context
    )
{
    NOTHING;
}

VOID PvpSearchInitializeFont(
    _Inout_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LOGFONT logFont;
    LONG dpiValue;

    if (Context->WindowFont)
        DeleteFont(Context->WindowFont);

    dpiValue = PhGetWindowDpi(WindowHandle);

    if (!PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, dpiValue))
        return;

    Context->WindowFont = CreateFont(
        -PhMultiplyDivideSigned(10, dpiValue, 72),
        0,
        0,
        0,
        FW_MEDIUM,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
        DEFAULT_PITCH,
        logFont.lfFaceName
        );

    SetWindowFont(WindowHandle, Context->WindowFont, TRUE);
}

VOID PvpSearchInitializeTheme(
    _Inout_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LONG dpiValue;
    INT borderX;

    dpiValue = PhGetWindowDpi(WindowHandle);

    Context->CXWidth = PhGetDpi(20, dpiValue);
    Context->ColorMode = PhGetIntegerSetting(L"GraphColorMode");

    borderX = PhGetSystemMetrics(SM_CXBORDER, dpiValue);

    if (PhIsThemeActive())
    {
        HTHEME themeDataHandle;

        if (themeDataHandle = PhOpenThemeData(WindowHandle, VSCLASS_EDIT, dpiValue))
        {
            //IsThemePartDefined_I(themeDataHandle, EP_EDITBORDER_NOSCROLL, EPSHV_NORMAL);

            if (!PhGetThemeInt(
                themeDataHandle,
                EP_EDITBORDER_NOSCROLL,
                EPSHV_NORMAL,
                TMT_BORDERSIZE,
                &Context->CXBorder
                ))
            {
                Context->CXBorder = borderX * 2;
            }

            PhCloseThemeData(themeDataHandle);
        }
        else
        {
            Context->CXBorder = borderX * 2;
        }
    }
    else
    {
        Context->CXBorder = borderX * 2;
    }
}

VOID PvpSearchInitializeImages(
    _Inout_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    HBITMAP bitmap;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(WindowHandle);

    Context->ImageWidth = PhGetSystemMetrics(SM_CXSMICON, dpiValue) + PhGetDpi(4, dpiValue);
    Context->ImageHeight = PhGetSystemMetrics(SM_CYSMICON, dpiValue) + PhGetDpi(4, dpiValue);

    if (Context->ImageListHandle)
    {
        PhImageListSetIconSize(
            Context->ImageListHandle,
            Context->ImageWidth,
            Context->ImageHeight
            );
    }
    else
    {
        Context->ImageListHandle = PhImageListCreate(
            Context->ImageWidth,
            Context->ImageHeight,
            ILC_MASK | ILC_COLOR32,
            2,
            0
            );
    }
    PhImageListSetImageCount(Context->ImageListHandle, 4);

    // Search Button
    Context->SearchButton.ImageIndex = ULONG_MAX;
    Context->SearchButton.ActiveImageIndex = ULONG_MAX;

    if (Context->ThemeSupport)
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_LIGHT), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->SearchButton.ImageIndex = 0;
            PhImageListReplace(Context->ImageListHandle, 0, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }
    else
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_DARK), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->SearchButton.ImageIndex = 0;
            PhImageListReplace(Context->ImageListHandle, 0, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }

    if (Context->ThemeSupport)
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_LIGHT), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->SearchButton.ActiveImageIndex = 1;
            PhImageListReplace(Context->ImageListHandle, 1, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }
    else
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_DARK), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->SearchButton.ActiveImageIndex = 1;
            PhImageListReplace(Context->ImageListHandle, 1, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }

    //if (Context->ImageWidth == 20 && Context->ImageHeight == 20) // Avoids bitmap scaling on startup at default DPI (dmex)
    //    bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_SMALL), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    //else
    //    bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);

    //if (bitmap)
    //{
    //    Context->SearchButton.ImageIndex = 0;
    //    PhImageListReplace(Context->ImageListHandle, 0, bitmap, NULL);
    //    DeleteBitmap(bitmap);
    //}
    //else
    //{
    //    Context->SearchButton.ImageIndex = 0;
    //    PhSetImageListBitmap(Context->ImageListHandle, 0, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP));
    //}

    //if (Context->ImageWidth == 20 && Context->ImageHeight == 20) // Avoids bitmap scaling on startup at default DPI (dmex)
    //    bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_SMALL), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    //else
    //    bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);

    //if (bitmap)
    //{
    //    Context->SearchButton.ActiveImageIndex = 1;
    //    PhImageListReplace(Context->ImageListHandle, 1, bitmap, NULL);
    //    DeleteBitmap(bitmap);
    //}
    //else
    //{
    //    Context->SearchButton.ActiveImageIndex = 1;
    //    PhSetImageListBitmap(Context->ImageListHandle, 1, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP));
    //}

    // Regex Button
    Context->RegexButton.ImageIndex = ULONG_MAX;
    Context->RegexButton.ActiveImageIndex = ULONG_MAX;

    if (Context->ThemeSupport)
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_LIGHT), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->RegexButton.ImageIndex = 2;
            PhImageListReplace(Context->ImageListHandle, 2, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }
    else
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_DARK), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->RegexButton.ImageIndex = 2;
            PhImageListReplace(Context->ImageListHandle, 2, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }

    // Case-Sensitivity Button
    Context->CaseButton.ImageIndex = ULONG_MAX;
    Context->CaseButton.ActiveImageIndex = ULONG_MAX;

    if (Context->ThemeSupport)
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_LIGHT), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->CaseButton.ImageIndex = 3;
            PhImageListReplace(Context->ImageListHandle, 3, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }
    else
    {
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_DARK), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->CaseButton.ImageIndex = 3;
            PhImageListReplace(Context->ImageListHandle, 3, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }
}

VOID PhSearchControlButtonRect(
    _In_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ PPV_SEARCHCONTROL_BUTTON Button,
    _In_ RECT WindowRect,
    _Out_ PRECT ButtonRect
    )
{
    *ButtonRect = WindowRect;

    ButtonRect->left = ((ButtonRect->right - Context->CXWidth) - Context->CXBorder - 1);
    ButtonRect->top += Context->CXBorder;
    ButtonRect->right -= Context->CXBorder;
    ButtonRect->bottom -= Context->CXBorder;

    // Shift the button rect to the left based on the button index.
    ButtonRect->left -= ((Context->CXWidth + Context->CXBorder - 1) * (PV_SC_BUTTON_COUNT - 1 - Button->Index));
    ButtonRect->right -= ((Context->CXWidth + Context->CXBorder - 1) * (PV_SC_BUTTON_COUNT - 1 - Button->Index));
}

VOID PvpSearchDrawWindow(
    _In_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ RECT WindowRect
    )
{
    if (PhEnableThemeSupport)
    {
        if (GetFocus() == WindowHandle)
        {
            SetDCBrushColor(Hdc, RGB(65, 65, 65));
            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 1, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.bottom - 1, WindowRect.right - WindowRect.left, 1, PATCOPY);

            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(65, 65, 65));
            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 1, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.bottom - 1, WindowRect.right - WindowRect.left, 1, PATCOPY);

            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
    }
}

VOID PvpSearchDrawButton(
    _In_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ PPV_SEARCHCONTROL_BUTTON Button,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ RECT WindowRect
    )
{
    RECT buttonRect;

    PhSearchControlButtonRect(Context, Button, WindowRect, &buttonRect);

    if (Button->Pushed)
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(99, 99, 99));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(153, 209, 255));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
    }
    else if (Button->Hot)
    {
        if (Button->Active && Button->ActiveImageIndex == ULONG_MAX)
        {
            if (PhEnableThemeSupport)
            {
                SetDCBrushColor(Hdc, RGB(54, 54, 54));
                FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
            }
            else
            {
                SetDCBrushColor(Hdc, RGB(133, 199, 255));
                FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
            }
        }
        else
        {
            if (PhEnableThemeSupport)
            {
                SetDCBrushColor(Hdc, RGB(78, 78, 78));
                FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
            }
            else
            {
                SetDCBrushColor(Hdc, RGB(205, 232, 255));
                FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
            }
        }
    }
    else if (Button->Error)
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(100, 28, 30));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(255, 155, 155));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
    }
    else if (Button->Active && Button->ActiveImageIndex == ULONG_MAX)
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(44, 44, 44));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(123, 189, 255));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
    }
    else
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            FillRect(Hdc, &buttonRect, GetStockBrush(DC_BRUSH));
        }
        else
        {
            FillRect(Hdc, &buttonRect, GetSysColorBrush(COLOR_WINDOW));
        }
    }

    if (Button->Active && Button->ActiveImageIndex != ULONG_MAX)
    {
        PhImageListDrawIcon(
            Context->ImageListHandle,
            Button->ActiveImageIndex,
            Hdc,
            buttonRect.left + 1 /*offset*/ + ((buttonRect.right - buttonRect.left) - Context->ImageWidth) / 2,
            buttonRect.top + ((buttonRect.bottom - buttonRect.top) - Context->ImageHeight) / 2,
            ILD_TRANSPARENT,
            FALSE
            );
    }
    else
    {
        PhImageListDrawIcon(
            Context->ImageListHandle,
            Button->ImageIndex,
            Hdc,
            buttonRect.left + 1 /*offset*/ + ((buttonRect.right - buttonRect.left) - Context->ImageWidth) / 2,
            buttonRect.top +  ((buttonRect.bottom - buttonRect.top) - Context->ImageHeight) / 2,
            ILD_TRANSPARENT,
            FALSE
            );
    }
}

VOID PvpSearchUpdateRegex(
    _In_ HWND hWnd,
    _In_ PPV_SEARCHCONTROL_CONTEXT Context
    )
{
    ULONG flags;

    Context->RegexButton.Error = FALSE;
    Context->SearchboxRegexError = 0;
    Context->SearchboxRegexErrorOffset = 0;

    if (Context->SearchboxRegexCode)
    {
        pcre2_code_free(Context->SearchboxRegexCode);
        Context->SearchboxRegexCode = NULL;
    }

    if (Context->SearchboxRegexMatchData)
    {
        pcre2_match_data_free(Context->SearchboxRegexMatchData);
        Context->SearchboxRegexMatchData = NULL;
    }

    if (!Context->RegexButton.Active || PhIsNullOrEmptyString(Context->SearchboxText))
        return;

    if (Context->CaseButton.Active)
        flags = PCRE2_DOTALL;
    else
        flags = PCRE2_CASELESS | PCRE2_DOTALL;

    Context->SearchboxRegexCode = pcre2_compile(
        Context->SearchboxText->Buffer,
        Context->SearchboxText->Length / sizeof(WCHAR),
        flags,
        &Context->SearchboxRegexError,
        &Context->SearchboxRegexErrorOffset,
        NULL
        );
    if (!Context->SearchboxRegexCode)
    {
        Context->RegexButton.Error = TRUE;
        return;
    }

    Context->SearchboxRegexMatchData = pcre2_match_data_create_from_pattern(
        Context->SearchboxRegexCode,
        NULL
        );
}

BOOLEAN PvpSearchUpdateText(
    _In_ HWND hWnd,
    _In_ PPV_SEARCHCONTROL_CONTEXT Context,
    _In_ BOOLEAN Force
    )
{
    PPH_STRING newSearchboxText;
    ULONG_PTR matchHandle;

    newSearchboxText = PH_AUTO(PhGetWindowText(hWnd));

    Context->SearchButton.Active = (newSearchboxText->Length > 0);

    if (!Force && PhEqualString(newSearchboxText, Context->SearchboxText, FALSE))
        return FALSE;

    PhSwapReference(&Context->SearchboxText, newSearchboxText);

    Context->UseSearchPointer = PhStringToInteger64(&newSearchboxText->sr, 0, &Context->SearchPointer);

    PvpSearchUpdateRegex(hWnd, Context);

    if (!Context->Callback)
        return TRUE;

    if (!Context->SearchboxText->Length)
        matchHandle = 0;
    else if (Context->RegexButton.Active && !Context->SearchboxRegexCode)
        matchHandle = 0;
    else
        matchHandle = (ULONG_PTR)Context;

    Context->Callback(matchHandle, Context->CallbackContext);

    return TRUE;
}


LRESULT CALLBACK PvpSearchWndSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_SEARCHCONTROL_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hWnd, SHRT_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PvpSearchFreeTheme(context);

            if (context->WindowFont)
            {
                DeleteFont(context->WindowFont);
                context->WindowFont = NULL;
            }

            if (context->ImageListHandle)
            {
                PhImageListDestroy(context->ImageListHandle);
                context->ImageListHandle = NULL;
            }

            if (context->CueBannerText)
            {
                PhDereferenceObject(context->CueBannerText);
                context->CueBannerText = NULL;
            }

            PhDereferenceObject(context->SearchboxText);

            if (context->SearchboxRegexCode)
            {
                pcre2_code_free(context->SearchboxRegexCode);
            }

            if (context->SearchboxRegexMatchData)
            {
                pcre2_match_data_free(context->SearchboxRegexMatchData);
            }

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hWnd, SHRT_MAX);
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DPICHANGED:
        {
            PvpSearchFreeTheme(context);
            PvpSearchInitializeTheme(context, hWnd);
            PvpSearchInitializeFont(context, hWnd);
            PvpSearchInitializeImages(context, hWnd);

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= (context->CXWidth * PV_SC_BUTTON_COUNT);
        }
        return 0;
    case WM_NCPAINT:
        {
            RECT windowRect;
            HDC hdc;
            ULONG flags;
            HRGN updateRegion;

            updateRegion = (HRGN)wParam;

            if (updateRegion == (HRGN)1) // HRGN_FULL
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(hWnd, updateRegion, flags))
            {
                RECT windowRectStart;
                HDC bufferDc;
                RECT bufferRect;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                // Get the screen coordinates of the window.
                GetWindowRect(hWnd, &windowRect);
                // Adjust the coordinates (start from 0,0).
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
                windowRectStart = windowRect;

                // Exclude client area.
                ExcludeClipRect(
                    hdc,
                    windowRect.left + 2,
                    windowRect.top + 2,
                    windowRect.right - (context->CXWidth * PV_SC_BUTTON_COUNT) - 2,
                    windowRect.bottom - 2
                    );

                bufferRect.left = 0;
                bufferRect.top = 0;
                bufferRect.right = windowRect.right - windowRect.left;
                bufferRect.bottom = windowRect.bottom - windowRect.top;

                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                if (GetFocus() == hWnd)
                {
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_HOTLIGHT));
                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
                }
                else if (context->Hot)
                {
                    if (context->ThemeSupport)
                    {
                        SetDCBrushColor(bufferDc, RGB(0x8f, 0x8f, 0x8f));
                        FrameRect(bufferDc, &windowRect, GetStockBrush(DC_BRUSH));
                    }
                    else
                    {
                        SetDCBrushColor(bufferDc, RGB(43, 43, 43));
                        FrameRect(bufferDc, &windowRect, GetStockBrush(DC_BRUSH));
                    }

                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
                }
                else
                {
                    if (context->ThemeSupport)
                    {
                        //SetDCBrushColor(bufferDc, RGB(43, 43, 43));
                        FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                    }
                    else
                    {
                        FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                    }

                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
                }

                PvpSearchDrawWindow(context, hWnd, bufferDc, windowRectStart);
                PvpSearchDrawButton(context, &context->SearchButton, hWnd, bufferDc, windowRectStart);
                PvpSearchDrawButton(context, &context->RegexButton, hWnd, bufferDc, windowRectStart);
                PvpSearchDrawButton(context, &context->CaseButton, hWnd, bufferDc, windowRectStart);

                BitBlt(
                    hdc,
                    bufferRect.left,
                    bufferRect.top,
                    bufferRect.right,
                    bufferRect.bottom,
                    bufferDc,
                    0,
                    0,
                    SRCCOPY
                    );


                BitBlt(hdc, bufferRect.left, bufferRect.top, bufferRect.right, bufferRect.bottom, bufferDc, 0, 0, SRCCOPY);
                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);

                ReleaseDC(hWnd, hdc);
            }
        }
        return 0;
    case WM_NCHITTEST:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted buttons.
            PhSearchControlButtonRect(context, &context->SearchButton, windowRect, &buttonRect);
            if (PtInRect(&buttonRect, windowPoint))
                return HTBORDER;

            PhSearchControlButtonRect(context, &context->RegexButton, windowRect, &buttonRect);
            if (PtInRect(&buttonRect, windowPoint))
                return HTBORDER;

            PhSearchControlButtonRect(context, &context->CaseButton, windowRect, &buttonRect);
            if (PtInRect(&buttonRect, windowPoint))
                return HTBORDER;
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            PhSearchControlButtonRect(context, &context->SearchButton, windowRect, &buttonRect);
            context->SearchButton.Pushed = PtInRect(&buttonRect, windowPoint);

            PhSearchControlButtonRect(context, &context->RegexButton, windowRect, &buttonRect);
            context->RegexButton.Pushed = PtInRect(&buttonRect, windowPoint);

            PhSearchControlButtonRect(context, &context->CaseButton, windowRect, &buttonRect);
            context->CaseButton.Pushed = PtInRect(&buttonRect, windowPoint);

            SetCapture(hWnd);
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_LBUTTONUP:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            PhSearchControlButtonRect(context, &context->SearchButton, windowRect, &buttonRect);
            if (PtInRect(&buttonRect, windowPoint))
            {
                SetFocus(hWnd);
                PhSetWindowText(hWnd, L"");
                PvpSearchUpdateText(hWnd, context, FALSE);
            }

            PhSearchControlButtonRect(context, &context->RegexButton, windowRect, &buttonRect);
            if (PtInRect(&buttonRect, windowPoint))
            {
                context->RegexButton.Active = !context->RegexButton.Active;
                PhSetIntegerSetting(L"SearchControlRegex", context->RegexButton.Active);
                PvpSearchUpdateText(hWnd, context, TRUE);
            }

            PhSearchControlButtonRect(context, &context->CaseButton, windowRect, &buttonRect);
            if (PtInRect(&buttonRect, windowPoint))
            {
                context->CaseButton.Active = !context->CaseButton.Active;
                PhSetIntegerSetting(L"SearchControlCaseSensitive", context->CaseButton.Active);
                PvpSearchUpdateText(hWnd, context, TRUE);
            }

            if (GetCapture() == hWnd)
            {
                context->SearchButton.Pushed = FALSE;
                context->RegexButton.Pushed = FALSE;
                context->CaseButton.Pushed = FALSE;
                ReleaseCapture();
            }

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            POINT windowPoint;
            PPH_EMENU menu;
            PPH_EMENU item;
            ULONG selStart;
            ULONG selEnd;

            SendMessage(hWnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            menu = PhCreateEMenu();

            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_UNDO, L"Undo", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_CUT, L"Cut", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_PASTE, L"Paste", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_DELETE, L"Delete", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_SELECTALL, L"Select All", NULL, NULL), ULONG_MAX);

            if (selStart == selEnd)
            {
                PhEnableEMenuItem(menu, IDC_CUT, FALSE);
                PhEnableEMenuItem(menu, IDC_COPY, FALSE);
                PhEnableEMenuItem(menu, IDC_DELETE, FALSE);
            }

            item = PhShowEMenu(
                menu,
                hWnd,
                PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                windowPoint.x,
                windowPoint.y
                );

            if (item)
            {
                PPH_STRING text = PH_AUTO(PhGetWindowText(hWnd));

                switch (item->Id)
                {
                    case IDC_UNDO:
                        {
                            SendMessage(hWnd, EM_UNDO, 0, 0);
                            PvpSearchUpdateText(hWnd, context, FALSE);
                        }
                        break;
                    case IDC_CUT:
                        {
                            PPH_STRING selectedText = PH_AUTO(PhSubstring(text, selStart, selEnd - selStart));
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef2(&startText->sr, &endText->sr));
                            PhSetClipboardString(hWnd, &selectedText->sr);
                            PhSetWindowText(hWnd, newText->Buffer);
                            PvpSearchUpdateText(hWnd, context, FALSE);
                        }
                        break;
                    case IDC_COPY:
                        {
                            PPH_STRING selectedText = PH_AUTO(PhSubstring(text, selStart, selEnd - selStart));
                            PhSetClipboardString(hWnd, &selectedText->sr);
                        }
                        break;
                    case IDC_DELETE:
                        {
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef2(&startText->sr, &endText->sr));
                            PhSetWindowText(hWnd, newText->Buffer);
                            PvpSearchUpdateText(hWnd, context, FALSE);
                        }
                        break;
                    case IDC_PASTE:
                        {
                            PPH_STRING clipText = PH_AUTO(PhGetClipboardString(hWnd));
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef3(&startText->sr, &clipText->sr, &endText->sr));
                            PhSetWindowText(hWnd, newText->Buffer);
                            PvpSearchUpdateText(hWnd, context, FALSE);
                        }
                        break;
                    case IDC_SELECTALL:
                        SendMessage(hWnd, EM_SETSEL, 0, -1);
                        break;
                }
            }

            PhDestroyEMenu(menu);
        }
        return FALSE;
    case WM_CUT:
    case WM_CLEAR:
    case WM_PASTE:
    case WM_UNDO:
    case WM_KEYUP:
    case WM_SETTEXT:
        PvpSearchUpdateText(hWnd, context, FALSE);
        __fallthrough;
    case WM_KILLFOCUS:
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            PvpSearchFreeTheme(context);
            PvpSearchInitializeTheme(context, hWnd);
            PvpSearchInitializeFont(context, hWnd);

            // Reset the client area margins.
            SendMessage(hWnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            PhSearchControlButtonRect(context, &context->SearchButton, windowRect, &buttonRect);
            context->SearchButton.Hot = PtInRect(&buttonRect, windowPoint);

            PhSearchControlButtonRect(context, &context->RegexButton, windowRect, &buttonRect);
            context->RegexButton.Hot = PtInRect(&buttonRect, windowPoint);

            PhSearchControlButtonRect(context, &context->CaseButton, windowRect, &buttonRect);
            context->CaseButton.Hot = PtInRect(&buttonRect, windowPoint);

            // Check that the mouse is within the inserted button.
            if (!context->HotTrack)
            {
                TRACKMOUSEEVENT trackMouseEvent;
                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = hWnd;
                trackMouseEvent.dwHoverTime = 0;

                context->HotTrack = TRUE;

                TrackMouseEvent(&trackMouseEvent);
            }

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_MOUSELEAVE:
    case WM_NCMOUSELEAVE:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            context->HotTrack = FALSE;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            PhSearchControlButtonRect(context, &context->SearchButton, windowRect, &buttonRect);
            context->SearchButton.Hot = PtInRect(&buttonRect, windowPoint);

            PhSearchControlButtonRect(context, &context->RegexButton, windowRect, &buttonRect);
            context->RegexButton.Hot = PtInRect(&buttonRect, windowPoint);

            PhSearchControlButtonRect(context, &context->CaseButton, windowRect, &buttonRect);
            context->CaseButton.Hot = PtInRect(&buttonRect, windowPoint);

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_PAINT:
        {
            if (
                PhIsNullOrEmptyString(context->CueBannerText) ||
                GetFocus() == hWnd ||
                CallWindowProc(oldWndProc, hWnd, WM_GETTEXTLENGTH, 0, 0) > 0 // Edit_GetTextLength
                )
            {
                return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
            }

            HDC hdc = (HDC)wParam ? (HDC)wParam : GetDC(hWnd);

            if (hdc)
            {
                HDC bufferDc;
                RECT clientRect;
                HFONT oldFont;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                GetClientRect(hWnd, &clientRect);

                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                SetBkMode(bufferDc, TRANSPARENT);

                if (context->ThemeSupport)
                {
                    SetTextColor(bufferDc, RGB(170, 170, 170));
                    SetDCBrushColor(bufferDc, RGB(60, 60, 60));
                    FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));
                    FillRect(bufferDc, &clientRect, GetSysColorBrush(COLOR_WINDOW));
                }

                oldFont = SelectFont(bufferDc, GetWindowFont(hWnd));
                clientRect.left += 2;
                DrawText(
                    bufferDc,
                    context->CueBannerText->Buffer,
                    (UINT)context->CueBannerText->Length / sizeof(WCHAR),
                    &clientRect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP
                    );
                clientRect.left -= 2;
                SelectFont(bufferDc, oldFont);

                BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);

                if (!(HDC)wParam)
                {
                    ReleaseDC(hWnd, hdc);
                }
            }

            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_KEYDOWN:
        {
            // Delete previous word for ctrl+backspace (thanks to Katayama Hirofumi MZ) (modified) (dmex)
            if (wParam == VK_BACK && GetAsyncKeyState(VK_CONTROL) < 0)
            {
                UINT textStart = 0;
                UINT textEnd = 0;
                UINT textLength;

                textLength = (UINT)CallWindowProc(oldWndProc, hWnd, WM_GETTEXTLENGTH, 0, 0);
                CallWindowProc(oldWndProc, hWnd, EM_GETSEL, (WPARAM)&textStart, (LPARAM)&textEnd);

                if (textLength > 0 && textStart == textEnd)
                {
                    PWSTR textBuffer;

                    textBuffer = PhAllocateZero((textLength + 1) * sizeof(WCHAR));
                    GetWindowText(hWnd, textBuffer, textLength);

                    for (; 0 < textStart; --textStart)
                    {
                        if (textBuffer[textStart - 1] == L' ' && iswalnum(textBuffer[textStart]))
                        {
                            CallWindowProc(oldWndProc, hWnd, EM_SETSEL, textStart, textEnd);
                            CallWindowProc(oldWndProc, hWnd, EM_REPLACESEL, TRUE, (LPARAM)L"");
                            PhFree(textBuffer);
                            return 1;
                        }
                    }

                    if (textStart == 0)
                    {
                        //CallWindowProc(oldWndProc, hWnd, WM_SETTEXT, 0, (LPARAM)L"");
                        PhSetWindowText(hWnd, L"");
                        PvpSearchUpdateText(hWnd, context, FALSE);
                        PhFree(textBuffer);
                        return 1;
                    }

                    PhFree(textBuffer);
                }
            }
        }
        break;
    case WM_CHAR:
        {
            // Delete previous word for ctrl+backspace (dmex)
            if (wParam == VK_F16 && GetAsyncKeyState(VK_CONTROL) < 0)
                return 1;
        }
        break;
    case EM_SETCUEBANNER:
        {
            PWSTR text = (PWSTR)lParam;

            PhMoveReference(&context->CueBannerText, PhCreateString(text));

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID PvCreateSearchControl(
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR BannerText,
    _In_ PPV_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PPV_SEARCHCONTROL_CONTEXT context;

    context = PhAllocateZero(sizeof(PV_SEARCHCONTROL_CONTEXT));

    context->ThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport"); // HACK
    context->ColorMode = PhGetIntegerSetting(L"GraphColorMode");

    context->CaseButton.Index = 0;
    context->RegexButton.Index = 1;
    context->SearchButton.Index = 2;

    //PhpSearchInitializeTheme(context);
    PvpSearchInitializeImages(context, WindowHandle);

    context->CueBannerText = BannerText ? PhCreateString(BannerText) : NULL;

    context->Callback = Callback;
    context->CallbackContext = Context;

    context->SearchboxText = PhReferenceEmptyString();

    context->RegexButton.Active = !!PhGetIntegerSetting(L"SearchControlRegex");
    context->CaseButton.Active = !!PhGetIntegerSetting(L"SearchControlCaseSensitive");

    // Subclass the Edit control window procedure.
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
    PhSetWindowContext(WindowHandle, SHRT_MAX, context);
    SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PvpSearchWndSubclassProc);

    // Initialize the theme parameters.
    SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
}

BOOLEAN PvSearchControlMatch(
    _In_ ULONG_PTR MatchHandle,
    _In_ PPH_STRINGREF Text
    )
{
    PPV_SEARCHCONTROL_CONTEXT context;

    context = (PPV_SEARCHCONTROL_CONTEXT)MatchHandle;

    if (!context)
        return FALSE;

    if (context->RegexButton.Active)
    {
        if (pcre2_match(
            context->SearchboxRegexCode,
            Text->Buffer,
            Text->Length / sizeof(WCHAR),
            0,
            0,
            context->SearchboxRegexMatchData,
            NULL
            ) >= 0)
        {
            return TRUE;
        }
    }
    else if (context->CaseButton.Active)
    {
        if (PhFindStringInStringRef(Text, &context->SearchboxText->sr, FALSE) != MAXULONG_PTR)
            return TRUE;
    }
    else
    {
        if (PhFindStringInStringRef(Text, &context->SearchboxText->sr, TRUE) != MAXULONG_PTR)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN PvSearchControlMatchZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PvSearchControlMatch(MatchHandle, &text);
}

BOOLEAN PvSearchControlMatchLongHintZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRefLongHint(&text, Text);

    return PvSearchControlMatch(MatchHandle, &text);
}

BOOLEAN PvSearchControlMatchPointer(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer
    )
{
    PPV_SEARCHCONTROL_CONTEXT context;

    context = (PPV_SEARCHCONTROL_CONTEXT)MatchHandle;

    if (!context || !context->UseSearchPointer)
        return FALSE;

    return ((ULONG64)Pointer == context->SearchPointer);
}

BOOLEAN PvSearchControlMatchPointerRange(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer,
    _In_ SIZE_T Size
    )
{
    PPV_SEARCHCONTROL_CONTEXT context;
    PVOID pointerEnd;

    context = (PPV_SEARCHCONTROL_CONTEXT)MatchHandle;

    if (!context || !context->UseSearchPointer)
        return FALSE;

    pointerEnd = PTR_ADD_OFFSET(Pointer, Size);

    return ((context->SearchPointer >= (ULONG64)Pointer) &&
            (context->SearchPointer < (ULONG64)pointerEnd));
}



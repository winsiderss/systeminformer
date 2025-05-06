/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2023
 *     jxy-s   2023-2024
 *
 */

#include <ph.h>
#include <searchbox.h>
#include <guisup.h>
#include <settings.h>
#include <vssym32.h>
#include <emenu.h>
#include <thirdparty.h>

typedef struct _PH_SEARCHCONTROL_BUTTON
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
    HWND TooltipHandle;
} PH_SEARCHCONTROL_BUTTON, *PPH_SEARCHCONTROL_BUTTON;

#define PH_SC_BUTTON_COUNT 3

typedef struct _PH_SEARCHCONTROL_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG HotTrack : 1;
            ULONG UseSearchPointer : 1;
            ULONG Spare : 29;
        };
    };

    HWND ParentWindowHandle;
    HWND PreviousFocusWindowHandle;
    LONG WindowDpi;

    PCWSTR RegexSetting;
    PCWSTR CaseSetting;

    PVOID ImageBaseAddress;
    PCWSTR SearchButtonResource;
    PCWSTR SearchButtonActiveResource;
    PCWSTR RegexButtonResource;
    PCWSTR CaseButtonResource;

    PH_SEARCHCONTROL_BUTTON SearchButton;
    PH_SEARCHCONTROL_BUTTON RegexButton;
    PH_SEARCHCONTROL_BUTTON CaseButton;

    LONG ButtonWidth;
    LONG BorderSize;
    LONG ImageWidth;
    LONG ImageHeight;
    WNDPROC DefaultWindowProc;
    HFONT WindowFont;
    HIMAGELIST ImageListHandle;
    PPH_STRING CueBannerText;

    HDC BufferedDc;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;

    HBRUSH FrameBrush;
    HBRUSH WindowBrush;

    ULONG SearchDelayMs;

    PPH_SEARCHCONTROL_CALLBACK Callback;
    PVOID CallbackContext;

    PH_STRINGREF SearchboxText;
    WCHAR SearchboxTextBuffer[0x100];

    ULONG64 SearchPointer;
    LONG SearchboxRegexError;
    PCRE2_SIZE SearchboxRegexErrorOffset;
    pcre2_code* SearchboxRegexCode;
    pcre2_match_data* SearchboxRegexMatchData;
} PH_SEARCHCONTROL_CONTEXT, *PPH_SEARCHCONTROL_CONTEXT;

VOID PhpSearchControlCreateBufferedContext(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT BufferRect
    )
{
    Context->BufferedDc = CreateCompatibleDC(Hdc);

    if (!Context->BufferedDc)
        return;

    Context->BufferedContextRect = *BufferRect;
    Context->BufferedBitmap = CreateCompatibleBitmap(
        Hdc,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom
        );

    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedDc, Context->BufferedBitmap);
}

VOID PhpSearchControlDestroyBufferedContext(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    if (Context->BufferedDc && Context->BufferedOldBitmap)
    {
        SelectBitmap(Context->BufferedDc, Context->BufferedOldBitmap);
    }

    if (Context->BufferedBitmap)
    {
        DeleteBitmap(Context->BufferedBitmap);
        Context->BufferedBitmap = NULL;
    }

    if (Context->BufferedDc)
    {
        DeleteDC(Context->BufferedDc);
        Context->BufferedDc = NULL;
    }
}

VOID PhpSearchControlInitializeFont(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    if (Context->WindowFont)
    {
        DeleteFont(Context->WindowFont);
        Context->WindowFont = NULL;
    }

    Context->WindowFont = PhCreateCommonFont(10, FW_MEDIUM, WindowHandle, Context->WindowDpi);
}

VOID PhpSearchControlInitializeTheme(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LONG borderSize;

    borderSize = PhGetSystemMetrics(SM_CXBORDER, Context->WindowDpi);

    Context->CaseButton.Index = 0;
    Context->RegexButton.Index = 1;
    Context->SearchButton.Index = 2;

    Context->ButtonWidth = PhGetDpi(20, Context->WindowDpi);
    Context->BorderSize = borderSize;
    Context->FrameBrush = GetSysColorBrush(COLOR_WINDOWFRAME);
    Context->WindowBrush = GetSysColorBrush(COLOR_WINDOW);

    if (PhIsThemeActive())
    {
        HTHEME themeHandle;

        if (themeHandle = PhOpenThemeData(WindowHandle, VSCLASS_EDIT, Context->WindowDpi))
        {
            if (PhGetThemeInt(themeHandle, 0, 0, TMT_BORDERSIZE, &borderSize))
            {
                Context->BorderSize = borderSize;
            }

            PhCloseThemeData(themeHandle);
        }
    }
}

VOID PhpSearchControlInitializeImages(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    HBITMAP bitmap;

    Context->ImageWidth = PhGetSystemMetrics(SM_CXSMICON, Context->WindowDpi) + PhGetDpi(4, Context->WindowDpi);
    Context->ImageHeight = PhGetSystemMetrics(SM_CYSMICON, Context->WindowDpi) + PhGetDpi(4, Context->WindowDpi);

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
            2, 0
            );
    }

    PhImageListSetImageCount(Context->ImageListHandle, 4);

    // Search Button
    Context->SearchButton.ImageIndex = ULONG_MAX;
    Context->SearchButton.ActiveImageIndex = ULONG_MAX;

    bitmap = PhLoadImageFormatFromResource(Context->ImageBaseAddress, Context->SearchButtonResource, L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    if (bitmap)
    {
        Context->SearchButton.ImageIndex = 0;
        PhImageListReplace(Context->ImageListHandle, 0, bitmap, NULL);
        DeleteBitmap(bitmap);
    }

    bitmap = PhLoadImageFormatFromResource(Context->ImageBaseAddress, Context->SearchButtonActiveResource, L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    if (bitmap)
    {
        Context->SearchButton.ActiveImageIndex = 1;
        PhImageListReplace(Context->ImageListHandle, 1, bitmap, NULL);
        DeleteBitmap(bitmap);
    }

    // Regex Button
    Context->RegexButton.ImageIndex = ULONG_MAX;
    Context->RegexButton.ActiveImageIndex = ULONG_MAX;

    bitmap = PhLoadImageFormatFromResource(Context->ImageBaseAddress, Context->RegexButtonResource, L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    if (bitmap)
    {
        Context->RegexButton.ImageIndex = 2;
        PhImageListReplace(Context->ImageListHandle, 2, bitmap, NULL);
        DeleteBitmap(bitmap);
    }

    // Case-Sensitivity Button
    Context->CaseButton.ImageIndex = ULONG_MAX;
    Context->CaseButton.ActiveImageIndex = ULONG_MAX;

    bitmap = PhLoadImageFormatFromResource(Context->ImageBaseAddress, Context->CaseButtonResource, L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    if (bitmap)
    {
        Context->CaseButton.ImageIndex = 3;
        PhImageListReplace(Context->ImageListHandle, 3, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
}

VOID PhpSearchControlButtonRect(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ PPH_SEARCHCONTROL_BUTTON Button,
    _In_ PRECT WindowRect,
    _Out_ PRECT ButtonRect
    )
{
    memcpy(ButtonRect, WindowRect, sizeof(RECT));

    ButtonRect->left = ((ButtonRect->right - Context->ButtonWidth) - Context->BorderSize - 1);
    ButtonRect->top += Context->BorderSize;
    ButtonRect->right -= Context->BorderSize;
    ButtonRect->bottom -= Context->BorderSize;

    // Shift the button rect to the left based on the button index.
    ButtonRect->left -= ((Context->ButtonWidth + Context->BorderSize - 1) * (PH_SC_BUTTON_COUNT - 1 - Button->Index));
    ButtonRect->right -= ((Context->ButtonWidth + Context->BorderSize - 1) * (PH_SC_BUTTON_COUNT - 1 - Button->Index));
}

VOID PhpSearchControlCreateTooltip(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ PPH_SEARCHCONTROL_BUTTON Button,
    _In_ HWND ParentWindow,
    _In_ PRECT TooltipRect,
    _In_ PWSTR TooltipText
    )
{
    TOOLINFO toolInfo;

    if (Button->TooltipHandle)
        return;

    Button->TooltipHandle = PhCreateWindowEx(
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE,
        WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        NULL,
        NULL
        );

    SetWindowPos(
        Button->TooltipHandle,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
        );

    MapWindowRect(HWND_DESKTOP, ParentWindow, TooltipRect);
    PhInflateRect(TooltipRect, -1, -1);

    memset(&toolInfo, 0, sizeof(TOOLINFO));
    toolInfo.cbSize = sizeof(TOOLINFO);
    toolInfo.uFlags = TTF_TRANSPARENT | TTF_SUBCLASS;
    toolInfo.hwnd = ParentWindow;
    toolInfo.lpszText = TooltipText;
    toolInfo.rect = *TooltipRect;
    SendMessage(Button->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    SendMessage(Button->TooltipHandle, TTM_SETDELAYTIME, TTDT_INITIAL, 0);
    SendMessage(Button->TooltipHandle, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
    SendMessage(Button->TooltipHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT);
    SendMessage(Button->TooltipHandle, TTM_POPUP, 0, 0);
}

VOID PhpSearchControlThemeChanged(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    PhpSearchControlInitializeFont(Context, WindowHandle);
    PhpSearchControlInitializeTheme(Context, WindowHandle);
    PhpSearchControlInitializeImages(Context, WindowHandle);

    // Reset the client area margins.
    CallWindowProc(Context->DefaultWindowProc, WindowHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

    // Refresh the non-client area.
    SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    // Force the edit control to update its non-client area.
    RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
}

VOID PhpSearchDrawWindow(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect
    )
{
    if (PhEnableThemeSupport)
    {
        SetDCBrushColor(Hdc, PhThemeWindowBackground2Color);
        SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
        PatBlt(Hdc, WindowRect->left, WindowRect->top, 1, WindowRect->bottom - WindowRect->top, PATCOPY);
        PatBlt(Hdc, WindowRect->right - 1, WindowRect->top, 1, WindowRect->bottom - WindowRect->top, PATCOPY);
        PatBlt(Hdc, WindowRect->left, WindowRect->top, WindowRect->right - WindowRect->left, 1, PATCOPY);
        PatBlt(Hdc, WindowRect->left, WindowRect->bottom - 1, WindowRect->right - WindowRect->left, 1, PATCOPY);

        SetDCBrushColor(Hdc, RGB(60, 60, 60));
        SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
        PatBlt(Hdc, WindowRect->left + 1, WindowRect->top + 1, 1, WindowRect->bottom - WindowRect->top - 2, PATCOPY);
        PatBlt(Hdc, WindowRect->right - 2, WindowRect->top + 1, 1, WindowRect->bottom - WindowRect->top - 2, PATCOPY);
        PatBlt(Hdc, WindowRect->left + 1, WindowRect->top + 1, WindowRect->right - WindowRect->left - 2, 1, PATCOPY);
        PatBlt(Hdc, WindowRect->left + 1, WindowRect->bottom - 2, WindowRect->right - WindowRect->left - 2, 1, PATCOPY);
    }
}

VOID PhpSearchDrawButton(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ PPH_SEARCHCONTROL_BUTTON Button,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect
    )
{
    RECT buttonRect;

    PhpSearchControlButtonRect(Context, Button, WindowRect, &buttonRect);

    if (Button->Pushed)
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(99, 99, 99));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(153, 209, 255));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
    }
    else if (Button->Hot)
    {
        if (Button->Active && Button->ActiveImageIndex == ULONG_MAX)
        {
            if (PhEnableThemeSupport)
            {
                SetDCBrushColor(Hdc, RGB(54, 54, 54));
                FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
            }
            else
            {
                SetDCBrushColor(Hdc, RGB(133, 199, 255));
                FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
            }
        }
        else
        {
            if (PhEnableThemeSupport)
            {
                SetDCBrushColor(Hdc, RGB(78, 78, 78));
                FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
            }
            else
            {
                SetDCBrushColor(Hdc, RGB(205, 232, 255));
                FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
            }
        }
    }
    else if (Button->Error)
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(100, 28, 30));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(255, 155, 155));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
    }
    else if (Button->Active && Button->ActiveImageIndex == ULONG_MAX)
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(44, 44, 44));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(123, 189, 255));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
    }
    else
    {
        if (PhEnableThemeSupport)
        {
            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            FillRect(Hdc, &buttonRect, PhGetStockBrush(DC_BRUSH));
        }
        else
        {
            FillRect(Hdc, &buttonRect, Context->WindowBrush);
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

VOID PhpSearchUpdateRegex(
    _In_ HWND WindowHandle,
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
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

    if (!Context->RegexButton.Active || Context->SearchboxText.Length == 0)
        return;

    if (Context->CaseButton.Active)
        flags = PCRE2_DOTALL;
    else
        flags = PCRE2_CASELESS | PCRE2_DOTALL;

    Context->SearchboxRegexCode = pcre2_compile(
        Context->SearchboxText.Buffer,
        Context->SearchboxText.Length / sizeof(WCHAR),
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

BOOLEAN PhGetSearchTextToBuffer(
    _In_ HWND WindowHandle,
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ PSIZE_T ReturnLength
    )
{
    SIZE_T returnLength;

    returnLength = CallWindowProc(
        Context->DefaultWindowProc,
        WindowHandle,
        WM_GETTEXT,
        BufferLength,
        (LPARAM)Buffer
        );

    if (returnLength != 0)
    {
        *ReturnLength = returnLength;
        return TRUE;
    }

    memset(Buffer, UNICODE_NULL, sizeof(UNICODE_NULL));
    *ReturnLength = 0;
    return TRUE;
}

BOOLEAN PhpSearchUpdateText(
    _In_ HWND WindowHandle,
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ BOOLEAN Force
    )
{
    ULONG_PTR matchHandle;
    PH_STRINGREF newSearchboxText;
    SIZE_T searchboxTextBufferLength;
    WCHAR searchboxTextBuffer[0x100];

    //if (PhGetWindowTextLength(WindowHandle) == 0)
    //{
    //    return FALSE;
    //}

    if (!PhGetSearchTextToBuffer(WindowHandle, Context, searchboxTextBuffer, RTL_NUMBER_OF(searchboxTextBuffer), &searchboxTextBufferLength))
        return FALSE;

    newSearchboxText.Buffer = searchboxTextBuffer;
    newSearchboxText.Length = searchboxTextBufferLength * sizeof(WCHAR);

    Context->SearchButton.Active = (newSearchboxText.Length > 0);

    if (!Force && PhEqualStringRef(&newSearchboxText, &Context->SearchboxText, FALSE))
        return FALSE;

    if (memcpy_s(Context->SearchboxTextBuffer, sizeof(Context->SearchboxTextBuffer), newSearchboxText.Buffer, newSearchboxText.Length))
        return FALSE;
    Context->SearchboxText.Buffer = Context->SearchboxTextBuffer;
    Context->SearchboxText.Length = searchboxTextBufferLength * sizeof(WCHAR);

    Context->UseSearchPointer = PhStringToUInt64(&newSearchboxText, 0, &Context->SearchPointer);

    PhpSearchUpdateRegex(WindowHandle, Context);

    if (!Context->Callback)
        return TRUE;

    matchHandle = (Context->SearchboxText.Length == 0 || (Context->RegexButton.Active && !Context->SearchboxRegexCode)) ? 0 : (ULONG_PTR)Context;

    Context->Callback(matchHandle, Context->CallbackContext);

    return TRUE;
}

void PhpSearchRestoreFocus(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    if (Context->PreviousFocusWindowHandle)
    {
        SetFocus(Context->PreviousFocusWindowHandle);
        Context->PreviousFocusWindowHandle = NULL;
    }
}

LRESULT CALLBACK PhpSearchWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SEARCHCONTROL_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);

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

            if (context->SearchboxRegexCode)
            {
                pcre2_code_free(context->SearchboxRegexCode);
            }

            if (context->SearchboxRegexMatchData)
            {
                pcre2_match_data_free(context->SearchboxRegexMatchData);
            }

            if (context->CaseButton.TooltipHandle)
            {
                DestroyWindow(context->CaseButton.TooltipHandle);
                context->CaseButton.TooltipHandle = NULL;
            }

            if (context->RegexButton.TooltipHandle)
            {
                DestroyWindow(context->RegexButton.TooltipHandle);
                context->RegexButton.TooltipHandle = NULL;
            }

            if (context->SearchButton.TooltipHandle)
            {
                DestroyWindow(context->SearchButton.TooltipHandle);
                context->SearchButton.TooltipHandle = NULL;
            }

            PhpSearchControlDestroyBufferedContext(context);

            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= (context->ButtonWidth * PH_SC_BUTTON_COUNT);
        }
        return 0;
    case WM_NCPAINT:
        {
            RECT windowRect;
            HDC hdc;
            ULONG flags;
            HRGN updateRegion;

            updateRegion = (HRGN)wParam;

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
            {
                RECT windowRectStart;
                RECT bufferRect;

                // Get the screen coordinates of the window.
                GetWindowRect(WindowHandle, &windowRect);
                // Adjust the coordinates (start from 0,0).
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
                windowRectStart = windowRect;

                // Exclude client area.
                ExcludeClipRect(
                    hdc,
                    windowRect.left + (context->BorderSize + 1),
                    windowRect.top + (context->BorderSize + 1),
                    windowRect.right - (context->ButtonWidth * PH_SC_BUTTON_COUNT) - (context->BorderSize + 1),
                    windowRect.bottom - (context->BorderSize + 1)
                    );

                bufferRect.left = 0;
                bufferRect.top = 0;
                bufferRect.right = windowRect.right - windowRect.left;
                bufferRect.bottom = windowRect.bottom - windowRect.top;

                if (context->BufferedDc && (
                    context->BufferedContextRect.right < bufferRect.right ||
                    context->BufferedContextRect.bottom < bufferRect.bottom))
                {
                    PhpSearchControlDestroyBufferedContext(context);
                }

                if (!context->BufferedDc)
                {
                    PhpSearchControlCreateBufferedContext(context, hdc, &bufferRect);
                }

                if (!context->BufferedDc)
                    break;

                if (GetFocus() == WindowHandle)
                {
                    FrameRect(context->BufferedDc, &windowRect, GetSysColorBrush(COLOR_HOTLIGHT));
                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
                }
                else if (context->Hot)
                {
                    if (PhEnableThemeSupport)
                    {
                        SetDCBrushColor(context->BufferedDc, PhThemeWindowHighlight2Color);
                        FrameRect(context->BufferedDc, &windowRect, PhGetStockBrush(DC_BRUSH));
                    }
                    else
                    {
                        SetDCBrushColor(context->BufferedDc, RGB(43, 43, 43));
                        FrameRect(context->BufferedDc, &windowRect, PhGetStockBrush(DC_BRUSH));
                    }

                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
                }
                else
                {
                    FrameRect(context->BufferedDc, &windowRect, context->FrameBrush);
                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
                }

                PhpSearchDrawWindow(context, WindowHandle, context->BufferedDc, &windowRectStart);
                PhpSearchDrawButton(context, &context->SearchButton, WindowHandle, context->BufferedDc, &windowRectStart);
                PhpSearchDrawButton(context, &context->RegexButton, WindowHandle, context->BufferedDc, &windowRectStart);
                PhpSearchDrawButton(context, &context->CaseButton, WindowHandle, context->BufferedDc, &windowRectStart);

                BitBlt(
                    hdc,
                    bufferRect.left,
                    bufferRect.top,
                    bufferRect.right,
                    bufferRect.bottom,
                    context->BufferedDc,
                    0,
                    0,
                    SRCCOPY
                    );

                ReleaseDC(WindowHandle, hdc);
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
            GetWindowRect(WindowHandle, &windowRect);

            // Get the position of the inserted buttons.
            PhpSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, windowPoint))
                return HTBORDER;

            PhpSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, windowPoint))
                return HTBORDER;

            PhpSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, windowPoint))
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
            GetWindowRect(WindowHandle, &windowRect);

            PhpSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            context->SearchButton.Pushed = PhPtInRect(&buttonRect, windowPoint);

            PhpSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            context->RegexButton.Pushed = PhPtInRect(&buttonRect, windowPoint);

            PhpSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            context->CaseButton.Pushed = PhPtInRect(&buttonRect, windowPoint);

            SetCapture(WindowHandle);
            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
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
            GetWindowRect(WindowHandle, &windowRect);

            PhpSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, windowPoint))
            {
                SetFocus(WindowHandle);
                PhSetWindowText(WindowHandle, L"");
                PhpSearchUpdateText(WindowHandle, context, FALSE);
            }

            PhpSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, windowPoint))
            {
                context->RegexButton.Active = !context->RegexButton.Active;
                PhSetIntegerSetting(context->RegexSetting, context->RegexButton.Active);
                PhpSearchUpdateText(WindowHandle, context, TRUE);
            }

            PhpSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, windowPoint))
            {
                context->CaseButton.Active = !context->CaseButton.Active;
                PhSetIntegerSetting(context->CaseSetting, context->CaseButton.Active);
                PhpSearchUpdateText(WindowHandle, context, FALSE);
            }

            if (GetCapture() == WindowHandle)
            {
                context->SearchButton.Pushed = FALSE;
                context->RegexButton.Pushed = FALSE;
                context->CaseButton.Pushed = FALSE;
                ReleaseCapture();
            }

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            POINT windowPoint;
            PPH_EMENU menu;
            PPH_EMENU item;
            ULONG selStart;
            ULONG selEnd;

            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            CallWindowProc(oldWndProc, WindowHandle, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Undo", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Cut", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"Copy", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 4, L"Paste", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 5, L"Delete", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 6, L"Select All", NULL, NULL), ULONG_MAX);

            if (selStart == selEnd)
            {
                PhEnableEMenuItem(menu, 2, FALSE);
                PhEnableEMenuItem(menu, 3, FALSE);
                PhEnableEMenuItem(menu, 6, FALSE);
            }

            item = PhShowEMenu(
                menu,
                WindowHandle,
                PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                windowPoint.x,
                windowPoint.y
                );

            if (item)
            {
                PPH_STRING text = PhGetWindowText(WindowHandle);

                switch (item->Id)
                {
                    case 1:
                        {
                            CallWindowProc(oldWndProc, WindowHandle, EM_UNDO, 0, 0);
                            PhpSearchUpdateText(WindowHandle, context, FALSE);
                        }
                        break;
                    case 2:
                        {
                            PPH_STRING selectedText = PH_AUTO(PhSubstring(text, selStart, selEnd - selStart));
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef2(&startText->sr, &endText->sr));
                            PhSetClipboardString(WindowHandle, &selectedText->sr);
                            PhSetWindowText(WindowHandle, newText->Buffer);
                            PhpSearchUpdateText(WindowHandle, context, FALSE);
                        }
                        break;
                    case 3:
                        {
                            PPH_STRING selectedText = PH_AUTO(PhSubstring(text, selStart, selEnd - selStart));
                            PhSetClipboardString(WindowHandle, &selectedText->sr);
                        }
                        break;
                    case 4:
                        {
                            PPH_STRING clipText = PH_AUTO(PhGetClipboardString(WindowHandle));
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef3(&startText->sr, &clipText->sr, &endText->sr));
                            PhSetWindowText(WindowHandle, newText->Buffer);
                            PhpSearchUpdateText(WindowHandle, context, FALSE);
                        }
                        break;
                    case 5:
                        {
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef2(&startText->sr, &endText->sr));
                            PhSetWindowText(WindowHandle, newText->Buffer);
                            PhpSearchUpdateText(WindowHandle, context, FALSE);
                        }
                        break;
                    case 6:
                        {
                            CallWindowProc(oldWndProc, WindowHandle, EM_SETSEL, 0, -1);
                        }
                        break;
                }

                PhDereferenceObject(text);
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
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            PhpSearchUpdateText(WindowHandle, context, FALSE);
            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
        break;
    case WM_KILLFOCUS:
        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_SETFOCUS:
        context->PreviousFocusWindowHandle = (HWND)wParam;
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            PhpSearchControlThemeChanged(context, WindowHandle);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(context->ParentWindowHandle);

            PhpSearchControlThemeChanged(context, WindowHandle);
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
            GetWindowRect(WindowHandle, &windowRect);
            context->Hot = PhPtInRect(&windowRect, windowPoint);

            PhpSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            context->RegexButton.Hot = PhPtInRect(&buttonRect, windowPoint);

            if (context->RegexButton.Hot)
            {
                PhpSearchControlCreateTooltip(context, &context->RegexButton, WindowHandle, &buttonRect, L"Use Regular Expression");
            }

            PhpSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            context->CaseButton.Hot = PhPtInRect(&buttonRect, windowPoint);

            if (context->CaseButton.Hot)
            {
                PhpSearchControlCreateTooltip(context, &context->CaseButton, WindowHandle, &buttonRect, L"Match Case");
            }

            PhpSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            context->SearchButton.Hot = PhPtInRect(&buttonRect, windowPoint);

            if (context->SearchButton.Hot)
            {
                PhpSearchControlCreateTooltip(context, &context->SearchButton, WindowHandle, &buttonRect, L"Clear Search");
            }

            // Check that the mouse is within the inserted button.
            if (!context->HotTrack)
            {
                TRACKMOUSEEVENT trackMouseEvent;
                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = WindowHandle;
                trackMouseEvent.dwHoverTime = 0;

                context->HotTrack = TRUE;

                TrackMouseEvent(&trackMouseEvent);
            }

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
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
            GetWindowRect(WindowHandle, &windowRect);
            context->Hot = PhPtInRect(&windowRect, windowPoint);

            PhpSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            context->SearchButton.Hot = PhPtInRect(&buttonRect, windowPoint);

            PhpSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            context->RegexButton.Hot = PhPtInRect(&buttonRect, windowPoint);

            PhpSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            context->CaseButton.Hot = PhPtInRect(&buttonRect, windowPoint);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_PAINT:
        {
            if (
                PhIsNullOrEmptyString(context->CueBannerText) ||
                GetFocus() == WindowHandle ||
                CallWindowProc(oldWndProc, WindowHandle, WM_GETTEXTLENGTH, 0, 0) > 0 // Edit_GetTextLength
                )
            {
                return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
            }

            PAINTSTRUCT paintStruct;
            HDC hdc;

            if (hdc = BeginPaint(WindowHandle, &paintStruct))
            {
                HDC bufferDc;
                RECT clientRect;
                HFONT oldFont;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                GetClientRect(WindowHandle, &clientRect);

                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                SetBkMode(bufferDc, TRANSPARENT);

                if (PhEnableThemeSupport)
                {
                    SetTextColor(bufferDc, RGB(170, 170, 170));
                    SetDCBrushColor(bufferDc, RGB(60, 60, 60));
                    FillRect(bufferDc, &clientRect, PhGetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));
                    FillRect(bufferDc, &clientRect, context->WindowBrush);
                }

                oldFont = SelectFont(bufferDc, context->WindowFont);
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

                EndPaint(WindowHandle, &paintStruct);
            }
        }
        return 0;
    case WM_KEYDOWN:
        {
            // Delete previous word for ctrl+backspace (thanks to Katayama Hirofumi MZ) (modified) (dmex)
            if (wParam == VK_BACK && GetAsyncKeyState(VK_CONTROL) < 0)
            {
                LONG textStart = 0;
                LONG textEnd = 0;
                LONG textLength;

                textLength = (LONG)CallWindowProc(oldWndProc, WindowHandle, WM_GETTEXTLENGTH, 0, 0);
                CallWindowProc(oldWndProc, WindowHandle, EM_GETSEL, (WPARAM)&textStart, (LPARAM)&textEnd);

                if (textLength > 0 && textStart == textEnd)
                {
                    ULONG textBufferLength;
                    WCHAR textBuffer[0x100];

                    if (!NT_SUCCESS(PhGetWindowTextToBuffer(WindowHandle, 0, textBuffer, RTL_NUMBER_OF(textBuffer), &textBufferLength)))
                    {
                        break;
                    }

                    for (; 0 < textStart; --textStart)
                    {
                        if (textBuffer[textStart - 1] == L' ' && iswalnum(textBuffer[textStart]))
                        {
                            CallWindowProc(oldWndProc, WindowHandle, EM_SETSEL, textStart, textEnd);
                            CallWindowProc(oldWndProc, WindowHandle, EM_REPLACESEL, TRUE, (LPARAM)L"");
                            return 1;
                        }
                    }

                    if (textStart == 0)
                    {
                        PhSetWindowText(WindowHandle, L"");
                        PhpSearchUpdateText(WindowHandle, context, FALSE);
                        return 1;
                    }
                }
            }
            // Clear search and restore focus for esc key
            else if (wParam == VK_ESCAPE)
            {
                PhSetWindowText(WindowHandle, L"");
                PhpSearchUpdateText(WindowHandle, context, FALSE);
                PhpSearchRestoreFocus(context);
                return 1;
            }
            // Up/down arrows will just restore previous focus without clearing search
            else if (wParam == VK_DOWN || wParam == VK_UP)
            {
                PhpSearchRestoreFocus(context);
                return 1;
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
    case WM_GETDLGCODE:
        {
            // Intercept esc key (otherwise it would get sent to parent window)
            if (wParam == VK_ESCAPE && ((MSG*)lParam)->message == WM_KEYDOWN)
                return DLGC_WANTMESSAGE;
        }
        break;
    case EM_SETCUEBANNER:
        {
            PWSTR text = (PWSTR)lParam;

            PhMoveReference(&context->CueBannerText, PhCreateString(text));

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

VOID PhCreateSearchControlEx(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PVOID ImageBaseAddress,
    _In_ PCWSTR SearchButtonResource,
    _In_ PCWSTR SearchButtonActiveResource,
    _In_ PCWSTR RegexButtonResource,
    _In_ PCWSTR CaseButtonResource,
    _In_ PCWSTR RegexSetting,
    _In_ PCWSTR CaseSetting,
    _In_ PPH_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PPH_SEARCHCONTROL_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_SEARCHCONTROL_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;
    context->CueBannerText = BannerText ? PhCreateString(BannerText) : NULL;
    context->WindowDpi = PhGetWindowDpi(ParentWindowHandle);

    context->RegexSetting = RegexSetting;
    context->CaseSetting = CaseSetting;

    context->ImageBaseAddress = ImageBaseAddress;
    context->SearchButtonResource = SearchButtonResource;
    context->SearchButtonActiveResource = SearchButtonActiveResource;
    context->RegexButtonResource = RegexButtonResource;
    context->CaseButtonResource = CaseButtonResource;

    context->Callback = Callback;
    context->CallbackContext = Context;

    context->RegexButton.Active = !!PhGetIntegerSetting(context->RegexSetting);
    context->CaseButton.Active = !!PhGetIntegerSetting(context->CaseSetting);

    // Subclass the Edit control window procedure.
    context->DefaultWindowProc = PhGetWindowProcedure(WindowHandle);
    PhSetWindowContext(WindowHandle, SHRT_MAX, context);
    PhSetWindowProcedure(WindowHandle, PhpSearchWndSubclassProc);

    // Initialize the theme parameters.
    PhpSearchControlThemeChanged(context, WindowHandle);
}

BOOLEAN PhSearchControlMatch(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text
    )
{
    PPH_SEARCHCONTROL_CONTEXT context;

    context = (PPH_SEARCHCONTROL_CONTEXT)MatchHandle;

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
        if (PhFindStringInStringRef(Text, &context->SearchboxText, FALSE) != MAXULONG_PTR)
            return TRUE;
    }
    else
    {
        if (PhFindStringInStringRef(Text, &context->SearchboxText, TRUE) != MAXULONG_PTR)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN PhSearchControlMatchZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PhSearchControlMatch(MatchHandle, &text);
}

BOOLEAN PhSearchControlMatchLongHintZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRefLongHint(&text, Text);

    return PhSearchControlMatch(MatchHandle, &text);
}

BOOLEAN PhSearchControlMatchPointer(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer
    )
{
    PPH_SEARCHCONTROL_CONTEXT context;

    context = (PPH_SEARCHCONTROL_CONTEXT)MatchHandle;

    if (!Pointer || !context || !context->UseSearchPointer)
        return FALSE;

    return ((ULONG64)Pointer == context->SearchPointer);
}

BOOLEAN PhSearchControlMatchPointerRange(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer,
    _In_ SIZE_T Size
    )
{
    PPH_SEARCHCONTROL_CONTEXT context;
    PVOID pointerEnd;

    context = (PPH_SEARCHCONTROL_CONTEXT)MatchHandle;

    if (!context || !context->UseSearchPointer)
        return FALSE;

    pointerEnd = PTR_ADD_OFFSET(Pointer, Size);

    return ((context->SearchPointer >= (ULONG64)Pointer) &&
            (context->SearchPointer < (ULONG64)pointerEnd));
}


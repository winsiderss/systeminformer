/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2026
 *     jxy-s   2023-2024
 *
 */

#include <ph.h>
#include <searchbox.h>
#include <searchmatch.h>
#include <guisup.h>
#include <settings.h>
#include <vssym32.h>
#include <emenu.h>
#include <thirdparty.h>
#include <uxtheme.h>

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
            ULONG Spare : 28;
        };
    };

    ULONG Index;
    ULONG ImageIndex;
    ULONG ActiveImageIndex;
} PH_SEARCHCONTROL_BUTTON, *PPH_SEARCHCONTROL_BUTTON;

#define PH_SC_BUTTON_COUNT 4

typedef struct _PH_SEARCHCONTROL_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG HotTrack : 1;
            ULONG WindowFocus : 1;
            ULONG Spare : 29;
        };
    };

    HWND ParentWindowHandle;
    HWND PreviousFocusWindowHandle;
    LONG WindowDpi;

    PCWSTR RegexSetting;
    PCWSTR CaseSetting;
    PCWSTR FuzzySetting;

    PVOID ImageBaseAddress;
    PCWSTR SearchButtonResource;
    PCWSTR SearchButtonActiveResource;
    PCWSTR RegexButtonResource;
    PCWSTR CaseButtonResource;
    PCWSTR FuzzyButtonResource;

    PH_SEARCHCONTROL_BUTTON SearchButton;
    PH_SEARCHCONTROL_BUTTON RegexButton;
    PH_SEARCHCONTROL_BUTTON CaseButton;
    PH_SEARCHCONTROL_BUTTON FuzzyButton;

    LONG ButtonWidth;
    LONG BorderSize;
    LONG ImageWidth;
    LONG ImageHeight;
    WNDPROC DefaultWindowProc;
    HFONT WindowFont;
    HIMAGELIST ImageListHandle;
    PPH_STRING CueBannerText;
    HWND TooltipHandle;

    HBRUSH WindowBrush;
    HBRUSH DcBrush;

    COLORREF WindowBorderOuterColor;
    COLORREF WindowBorderInnerColor;
    COLORREF WindowBackgroundColor;
    COLORREF FrameDefaultColor;
    COLORREF ButtonPushedColor;
    COLORREF ButtonHotActiveColor;
    COLORREF ButtonHotColor;
    COLORREF ButtonErrorColor;
    COLORREF ButtonActiveColor;
    COLORREF ButtonDefaultColor;
    COLORREF FrameHotColor;
    COLORREF CueBannerTextColor;
    COLORREF CueBannerBackgroundColor;

    ULONG SearchDelayMs;

    PPH_SEARCHCONTROL_CALLBACK Callback;
    PVOID CallbackContext;

    PH_SEARCH_MATCH_STATE Match;
} PH_SEARCHCONTROL_CONTEXT, *PPH_SEARCHCONTROL_CONTEXT;

/**
 * Selects the appropriate color based on whether theme support is enabled.
 *
 * \param ThemeColor The color to use when a theme is active.
 * \param ClassicColor The color to use when no theme is active.
 * \return The selected color.
 */
COLORREF PhSearchControlSelectColor(
    _In_ COLORREF ThemeColor,
    _In_ COLORREF ClassicColor
    )
{
    return PhEnableThemeSupport ? ThemeColor : ClassicColor;
}

/**
 * Initializes the colors used by the search control.
 *
 * \param Context The search control context.
 */
VOID PhSearchControlInitializeColors(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    static const COLORREF WindowBorderInnerThemeColor = RGB(60, 60, 60);
    static const COLORREF WindowBorderInnerClassicColor = RGB(60, 60, 60);
    static const COLORREF ButtonPushedThemeColor = RGB(99, 99, 99);
    static const COLORREF ButtonPushedClassicColor = RGB(153, 209, 255);
    static const COLORREF ButtonHotActiveThemeColor = RGB(54, 54, 54);
    static const COLORREF ButtonHotActiveClassicColor = RGB(133, 199, 255);
    static const COLORREF ButtonHotThemeColor = RGB(78, 78, 78);
    static const COLORREF ButtonHotClassicColor = RGB(205, 232, 255);
    static const COLORREF ButtonErrorThemeColor = RGB(100, 28, 30);
    static const COLORREF ButtonErrorClassicColor = RGB(255, 155, 155);
    static const COLORREF ButtonActiveThemeColor = RGB(44, 44, 44);
    static const COLORREF ButtonActiveClassicColor = RGB(123, 189, 255);
    static const COLORREF FrameHotClassicColor = RGB(43, 43, 43);
    static const COLORREF CueBannerTextThemeColor = RGB(170, 170, 170);

    Context->DcBrush = PhGetStockBrush(DC_BRUSH);
    Context->WindowBrush = GetSysColorBrush(COLOR_WINDOW);

    Context->WindowBorderOuterColor = PhThemeWindowBackground2Color;
    Context->WindowBorderInnerColor = PhSearchControlSelectColor(WindowBorderInnerThemeColor, WindowBorderInnerClassicColor);
    Context->WindowBackgroundColor = PhSearchControlSelectColor(PhThemeWindowBackgroundColor, GetSysColor(COLOR_WINDOW));
    Context->FrameDefaultColor = PhSearchControlSelectColor(PhThemeWindowBackground2Color, GetSysColor(COLOR_WINDOWFRAME));
    Context->ButtonPushedColor = PhSearchControlSelectColor(ButtonPushedThemeColor, ButtonPushedClassicColor);
    Context->ButtonHotActiveColor = PhSearchControlSelectColor(ButtonHotActiveThemeColor, ButtonHotActiveClassicColor);
    Context->ButtonHotColor = PhSearchControlSelectColor(ButtonHotThemeColor, ButtonHotClassicColor);
    Context->ButtonErrorColor = PhSearchControlSelectColor(ButtonErrorThemeColor, ButtonErrorClassicColor);
    Context->ButtonActiveColor = PhSearchControlSelectColor(ButtonActiveThemeColor, ButtonActiveClassicColor);
    Context->ButtonDefaultColor = PhSearchControlSelectColor(WindowBorderInnerThemeColor, GetSysColor(COLOR_WINDOW));
    Context->FrameHotColor = PhSearchControlSelectColor(PhThemeWindowHighlight2Color, FrameHotClassicColor);
    Context->CueBannerTextColor = PhSearchControlSelectColor(CueBannerTextThemeColor, GetSysColor(COLOR_GRAYTEXT));
    Context->CueBannerBackgroundColor = PhSearchControlSelectColor(WindowBorderInnerThemeColor, GetSysColor(COLOR_WINDOW));
}

/**
 * Initializes the font used by the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 */
VOID PhSearchControlInitializeFont(
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

/**
 * Initializes the theme parameters for the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 */
VOID PhSearchControlInitializeTheme(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LONG borderSize;

    borderSize = PhGetSystemMetrics(SM_CXBORDER, Context->WindowDpi);

    Context->CaseButton.Index = 0;
    Context->FuzzyButton.Index = 1;
    Context->RegexButton.Index = 2;
    Context->SearchButton.Index = 3;

    Context->ButtonWidth = PhScaleToDisplay(20, Context->WindowDpi);
    Context->BorderSize = borderSize;
    PhSearchControlInitializeColors(Context);

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

/**
 * Initializes the images used for the buttons in the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 */
VOID PhSearchControlInitializeImages(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    HBITMAP bitmap;

    Context->ImageWidth = PhGetSystemMetrics(SM_CXSMICON, Context->WindowDpi) + PhScaleToDisplay(4, Context->WindowDpi);
    Context->ImageHeight = PhGetSystemMetrics(SM_CYSMICON, Context->WindowDpi) + PhScaleToDisplay(4, Context->WindowDpi);

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

    PhImageListSetImageCount(Context->ImageListHandle, 5);

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

    Context->FuzzyButton.ImageIndex = ULONG_MAX;
    Context->FuzzyButton.ActiveImageIndex = ULONG_MAX;

    if (Context->FuzzyButtonResource)
    {
        bitmap = PhLoadImageFormatFromResource(Context->ImageBaseAddress, Context->FuzzyButtonResource, L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
        if (bitmap)
        {
            Context->FuzzyButton.ImageIndex = 4;
            PhImageListReplace(Context->ImageListHandle, 4, bitmap, NULL);
            DeleteBitmap(bitmap);
        }
    }
}

/**
 * Calculates the bounding rectangle for a search control button.
 *
 * \param Context The search control context.
 * \param Button The button to calculate the rectangle for.
 * \param WindowRect The bounding rectangle of the search window.
 * \param ButtonRect A variable which receives the calculated button rectangle.
 */
VOID PhSearchControlButtonRect(
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

/**
 * Creates or updates a tooltip for the search control.
 *
 * \param Context The search control context.
 * \param ParentWindow A handle to the parent window of the tooltip.
 * \param TooltipRect The bounding rectangle of the tooltip.
 * \param TooltipText The text to display in the tooltip.
 */
VOID PhSearchControlCreateTooltip(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND ParentWindow,
    _In_ PRECT TooltipRect,
    _In_ PWSTR TooltipText
    )
{
    TOOLINFO toolInfo;

    MapWindowRect(HWND_DESKTOP, ParentWindow, TooltipRect);
    PhInflateRect(TooltipRect, -1, -1);

    if (!Context->TooltipHandle)
    {
        Context->TooltipHandle = PhCreateWindowEx(
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE,
            WS_EX_TOPMOST | WS_EX_TRANSPARENT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            ParentWindow,
            NULL,
            NULL,
            NULL
            );

        SetWindowPos(
            Context->TooltipHandle,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
            );

        memset(&toolInfo, 0, sizeof(TOOLINFO));
        toolInfo.cbSize = sizeof(TOOLINFO);
        toolInfo.uFlags = TTF_TRANSPARENT | TTF_SUBCLASS;
        toolInfo.hwnd = ParentWindow;
        toolInfo.uId = 1;
        toolInfo.lpszText = TooltipText;
        toolInfo.rect = *TooltipRect;
        SendMessage(Context->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
        SendMessage(Context->TooltipHandle, TTM_SETDELAYTIME, TTDT_INITIAL, 0);
        SendMessage(Context->TooltipHandle, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
        SendMessage(Context->TooltipHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT);
    }
    //else
    //{
    //    toolInfo.cbSize = sizeof(TOOLINFO);
    //    toolInfo.hwnd = ParentWindow;
    //    toolInfo.uId = 1;
    //    toolInfo.lpszText = TooltipText;
    //    toolInfo.rect = *TooltipRect;
    //    SendMessage(Context->TooltipHandle, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolInfo);
    //    SendMessage(Context->TooltipHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
    //}

    //SendMessage(Context->TooltipHandle, TTM_UPDATE, 0, 0);
    SendMessage(Context->TooltipHandle, TTM_POPUP, 0, 0);
}

/**
 * Handles theme change events for the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 */
VOID PhSearchControlThemeChanged(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    PhSearchControlInitializeColors(Context);
    PhSearchControlInitializeFont(Context, WindowHandle);
    PhSearchControlInitializeTheme(Context, WindowHandle);
    PhSearchControlInitializeImages(Context, WindowHandle);

    // Reset the client area margins.
    CallWindowProc(Context->DefaultWindowProc, WindowHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

    // Refresh the non-client area.
    PhSetWindowFrameChanged(WindowHandle);

    // Force the edit control to update its non-client area.
    //RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
}

/**
 * Determines the frame color for the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 * \return The frame color.
 */
COLORREF PhSearchControlFrameColor(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    if (GetFocus() == WindowHandle)
        return PhThemeWindowHighlightColor;

    if (Context->Hot)
        return Context->FrameHotColor;

    return Context->FrameDefaultColor;
}

/**
 * Determines the color for a search control button based on its state.
 *
 * \param Context The search control context.
 * \param Button The button to determine the color for.
 * \return The button color.
 */
COLORREF PhSearchControlButtonColor(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ PPH_SEARCHCONTROL_BUTTON Button
    )
{
    if (Button->Pushed)
        return Context->ButtonPushedColor;

    if (Button->Hot)
    {
        if (Button->Active && Button->ActiveImageIndex == ULONG_MAX)
            return Context->ButtonHotActiveColor;

        return Context->ButtonHotColor;
    }

    if (Button->Error)
        return Context->ButtonErrorColor;

    if (Button->Active && Button->ActiveImageIndex == ULONG_MAX)
        return Context->ButtonActiveColor;

    return Context->ButtonDefaultColor;
}

/**
 * Gets the text color for the cue banner.
 *
 * \param Context The search control context.
 * \return The cue banner text color.
 */
COLORREF PhSearchControlCueBannerTextColor(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    return Context->CueBannerTextColor;
}

/**
 * Gets the background color for the cue banner.
 *
 * \param Context The search control context.
 * \return The cue banner background color.
 */
COLORREF PhSearchControlCueBannerBackgroundColor(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    return Context->CueBannerBackgroundColor;
}

/**
 * Paints a search control button.
 *
 * \param Context The search control context.
 * \param Button The button to paint.
 * \param Hdc A handle to the device context.
 * \param WindowRect The bounding rectangle of the search window.
 */
VOID PhSearchControlPaintButton(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ PPH_SEARCHCONTROL_BUTTON Button,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect
    )
{
    RECT buttonRect;
    ULONG imageIndex;

    PhSearchControlButtonRect(Context, Button, WindowRect, &buttonRect);

    SetDCBrushColor(Hdc, PhSearchControlButtonColor(Context, Button));
    FillRect(Hdc, &buttonRect, Context->DcBrush);

    if (Button->Active && Button->ActiveImageIndex != ULONG_MAX)
        imageIndex = Button->ActiveImageIndex;
    else
        imageIndex = Button->ImageIndex;

    if (imageIndex == ULONG_MAX)
    {
        if (Button == &Context->FuzzyButton)
        {
            HFONT oldFont;
            COLORREF textColor = PhEnableThemeSupport ? RGB(222, 222, 222) : GetSysColor(COLOR_BTNTEXT);

            oldFont = SelectFont(Hdc, Context->WindowFont);
            SetBkMode(Hdc, TRANSPARENT);
            SetTextColor(Hdc, textColor);
            PhOffsetRect(&buttonRect, 0, -PhScaleToDisplay(1, Context->WindowDpi));
            DrawText(Hdc, L"Fz", -1, &buttonRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
            SelectFont(Hdc, oldFont);
        }

        return;
    }

    PhImageListDrawIcon(
        Context->ImageListHandle,
        imageIndex,
        Hdc,
        buttonRect.left + 1 /*offset*/ + ((buttonRect.right - buttonRect.left) - Context->ImageWidth) / 2,
        buttonRect.top + ((buttonRect.bottom - buttonRect.top) - Context->ImageHeight) / 2,
        ILD_TRANSPARENT,
        FALSE
        );
}

/**
 * Paints all buttons in the search control.
 *
 * \param Context The search control context.
 * \param Hdc A handle to the device context.
 * \param WindowRect The bounding rectangle of the search window.
 */
VOID PhSearchControlPaintButtons(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect
    )
{
    PhSearchControlPaintButton(Context, &Context->SearchButton, Hdc, WindowRect);
    PhSearchControlPaintButton(Context, &Context->RegexButton, Hdc, WindowRect);
    PhSearchControlPaintButton(Context, &Context->CaseButton, Hdc, WindowRect);
    PhSearchControlPaintButton(Context, &Context->FuzzyButton, Hdc, WindowRect);
}

/**
 * Paints the frame of the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 * \param Hdc A handle to the device context.
 * \param WindowRect The bounding rectangle of the search window.
 */
VOID PhSearchControlPaintFrame(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect
    )
{
    RECT frameRect = *WindowRect;

    SetDCBrushColor(Hdc, PhSearchControlFrameColor(Context, WindowHandle));
    FrameRect(Hdc, &frameRect, Context->DcBrush);

    SetDCBrushColor(Hdc, Context->WindowBackgroundColor);
    PhInflateRect(&frameRect, -1, -1);
    FrameRect(Hdc, &frameRect, Context->DcBrush);
}

/**
 * Excludes the client area of the search window from the clipping region.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 * \param Hdc A handle to the device context.
 * \param WindowRect The bounding rectangle of the search window.
 */
VOID PhSearchControlExcludeClient(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect
    )
{
    WINDOWINFO windowInfo;

    memset(&windowInfo, 0, sizeof(WINDOWINFO));
    windowInfo.cbSize = sizeof(WINDOWINFO);

    if (GetWindowInfo(WindowHandle, &windowInfo))
    {
        PhOffsetRect(
            &windowInfo.rcClient,
            -windowInfo.rcWindow.left,
            -windowInfo.rcWindow.top
            );

        ExcludeClipRect(
            Hdc,
            windowInfo.rcClient.left,
            windowInfo.rcClient.top,
            windowInfo.rcClient.right,
            windowInfo.rcClient.bottom
            );

        return;
    }

    ExcludeClipRect(
        Hdc,
        WindowRect->left + (Context->BorderSize + 1),
        WindowRect->top + (Context->BorderSize + 1),
        WindowRect->right - (Context->ButtonWidth * PH_SC_BUTTON_COUNT) - (Context->BorderSize + 1),
        WindowRect->bottom - (Context->BorderSize + 1)
        );
}

/**
 * Copies the button toggle states into the shared match state.
 *
 * \param Context The search control context.
 */
VOID PhpSearchSyncMatchFlags(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    Context->Match.CaseActive = !!Context->CaseButton.Active;
    Context->Match.RegexActive = !!Context->RegexButton.Active;
    Context->Match.FuzzyActive = !!Context->FuzzyButton.Active;
}

/**
 * Updates the regular expression for the search control.
 *
 * \param WindowHandle A handle to the search window.
 * \param Context The search control context.
 */
VOID PhpSearchUpdateRegex(
    _In_ HWND WindowHandle,
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    PhpSearchSyncMatchFlags(Context);
    PhSearchMatchUpdateRegex(&Context->Match);
    Context->RegexButton.Error = !!Context->Match.RegexError;
}

/**
 * Retrieves the search control text into a buffer.
 *
 * \param WindowHandle A handle to the search window.
 * \param Context The search control context.
 * \param Buffer The buffer that receives the text.
 * \param BufferLength The size of the buffer, in bytes.
 * \param ReturnLength A variable which receives the number of bytes written to the buffer.
 * \return TRUE if successful, otherwise FALSE.
 */
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

    Buffer[0] = UNICODE_NULL;
    *ReturnLength = 0;
    return TRUE;
}

/**
 * Updates the text for the search control.
 *
 * \param WindowHandle A handle to the search window.
 * \param Context The search control context.
 * \param Force TRUE to force an update even if the text has not changed.
 * \return TRUE if the text was updated, otherwise FALSE.
 */
BOOLEAN PhSearchUpdateText(
    _In_ HWND WindowHandle,
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ BOOLEAN Force
    )
{
    ULONG_PTR matchHandle;
    PH_STRINGREF newSearchboxText;
    PPH_STRING searchboxTextString;

    //if (PhGetWindowTextLength(WindowHandle) == 0)
    //{
    //    return FALSE;
    //}

    searchboxTextString = PhGetWindowText(WindowHandle);
    if (!searchboxTextString)
        return FALSE;

    newSearchboxText.Buffer = searchboxTextString->Buffer;
    newSearchboxText.Length = searchboxTextString->Length;

    Context->SearchButton.Active = (newSearchboxText.Length > 0);

    PhpSearchSyncMatchFlags(Context);

    if (!PhSearchMatchSetText(&Context->Match, searchboxTextString, Force))
    {
        PhDereferenceObject(searchboxTextString);
        return FALSE;
    }

    PhDereferenceObject(searchboxTextString);

    //PhSearchUpdateRegex(WindowHandle, Context);

    if (!Context->Callback)
        return TRUE;

    matchHandle = PhSearchMatchGetHandle(&Context->Match);

    Context->Callback(matchHandle, Context->CallbackContext);

    return TRUE;
}

/**
 * Restores focus to the previous window.
 *
 * \param Context The search control context.
 */
VOID PhSearchRestoreFocus(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    if (Context->PreviousFocusWindowHandle)
    {
        SetFocus(Context->PreviousFocusWindowHandle);
        Context->PreviousFocusWindowHandle = NULL;
    }
}

/**
 * Paints the non-client area of the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 * \param Hdc A handle to the device context.
 * \param WindowRect The bounding rectangle of the search window.
 * \param BufferRect The bounding rectangle of the buffer.
 */
VOID PhSearchControlPaintNonClient(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect,
    _In_ PRECT BufferRect
    )
{
    PhSearchControlExcludeClient(Context, WindowHandle, Hdc, WindowRect);

    SetDCBrushColor(Hdc, Context->WindowBackgroundColor);
    FillRect(Hdc, BufferRect, Context->DcBrush);

    PhSearchControlPaintFrame(Context, WindowHandle, Hdc, WindowRect);
    PhSearchControlPaintButtons(Context, Hdc, WindowRect);
}

/**
 * Handles painting the non-client area of the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 * \param WParam The WPARAM value from the window message.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
BOOLEAN PhSearchControlHandleNonClientPaint(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ WPARAM WParam
    )
{
    RECT windowRect;
    RECT bufferRect;
    LONG width;
    LONG height;
    HDC hdc;
    HDC bufferDc;
    HRGN updateRegion;
    ULONG flags;
    HPAINTBUFFER bufferedPaint;
    BP_PAINTPARAMS bufferParams;

    if (!PhGetWindowRect(WindowHandle, &windowRect))
        return FALSE;

    width = windowRect.right - windowRect.left;
    height = windowRect.bottom - windowRect.top;

    if (width <= 0 || height <= 0)
        return FALSE;

    updateRegion = (HRGN)WParam;

    if (updateRegion != HRGN_FULL && updateRegion != RGN_ERROR)
    {
        if (!RectInRegion(updateRegion, &windowRect))
            return FALSE;   // frame area isn't dirty at all — skip GetDCEx entirely
    }

    if (updateRegion == HRGN_FULL)
        updateRegion = NULL;

    flags = DCX_WINDOW | DCX_CACHE | DCX_USESTYLE;

    if (updateRegion)
        flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

    if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
    {
        PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);

        bufferRect.left = 0;
        bufferRect.top = 0;
        bufferRect.right = width;
        bufferRect.bottom = height;

        PhSearchControlExcludeClient(Context, WindowHandle, hdc, &windowRect);

        memset(&bufferParams, 0, sizeof(BP_PAINTPARAMS));
        bufferParams.cbSize = sizeof(BP_PAINTPARAMS);
        bufferParams.dwFlags = BPPF_NONCLIENT;

        if (bufferedPaint = BeginBufferedPaint(
            hdc,
            &bufferRect,
            BPBF_COMPATIBLEBITMAP,
            &bufferParams,
            &bufferDc
            ))
        {
            PhSearchControlPaintNonClient(Context, WindowHandle, bufferDc, &windowRect, &bufferRect);
            EndBufferedPaint(bufferedPaint, TRUE);
        }
        else
        {
            PhSearchControlPaintNonClient(Context, WindowHandle, hdc, &windowRect, &bufferRect);
        }

        ReleaseDC(WindowHandle, hdc);
        return TRUE;
    }

    return FALSE;
}

/**
 * Paints the cue banner text for the search control.
 *
 * \param Context The search control context.
 * \param Hdc A handle to the device context.
 * \param ClientRect The client rectangle of the search window.
 * \param Erase TRUE to erase the background before painting.
 */
VOID PhSearchControlPaintCueBanner(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect,
    _In_ BOOLEAN Erase
    )
{
    HFONT oldFont;
    RECT textRect;

    if (Erase)
        FillRect(Hdc, ClientRect, Context->WindowBrush);

    SetBkMode(Hdc, TRANSPARENT);
    SetTextColor(Hdc, PhSearchControlCueBannerTextColor(Context));
    SetDCBrushColor(Hdc, PhSearchControlCueBannerBackgroundColor(Context));
    FillRect(Hdc, ClientRect, Context->DcBrush);

    oldFont = SelectFont(Hdc, Context->WindowFont);

    textRect = *ClientRect;
    textRect.left += 2;

    DrawText(
        Hdc,
        Context->CueBannerText->Buffer,
        (UINT)(Context->CueBannerText->Length / sizeof(WCHAR)),
        &textRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP
        );

    SelectFont(Hdc, oldFont);
}

/**
 * Handles painting the client area of the search control.
 *
 * \param Context The search control context.
 * \param WindowHandle A handle to the search window.
 * \param OldWndProc The original window procedure.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
BOOLEAN PhSearchControlHandleClientPaint(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ WNDPROC OldWndProc
    )
{
    PAINTSTRUCT ps;
    RECT clientRect;
    PH_BUFFERED_PAINT bufferedPaint;
    HDC hdc;
    HDC bufferDc;
    LONG width;
    LONG height;

    if (PhIsNullOrEmptyString(Context->CueBannerText) ||
        Context->WindowFocus ||
        CallWindowProc(OldWndProc, WindowHandle, WM_GETTEXTLENGTH, 0, 0) > 0)
    {
        return FALSE;
    }

    if (!PhGetClientRect(WindowHandle, &clientRect))
        return FALSE;

    width = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;

    if (width <= 0 || height <= 0)
        return FALSE;

    if (hdc = BeginPaint(WindowHandle, &ps))
    {
        if (PhBeginBufferedPaint(
            hdc,
            &clientRect,
            PHBF_COMPATIBLEBITMAP,
            NULL,
            &bufferedPaint,
            &bufferDc
            ))
        {
            PhSearchControlPaintCueBanner(Context, bufferDc, &clientRect, !!ps.fErase);
            PhEndBufferedPaint(&bufferedPaint, TRUE);
        }
        else
        {
            PhSearchControlPaintCueBanner(Context, hdc, &clientRect, !!ps.fErase);
        }

        EndPaint(WindowHandle, &ps);
        return TRUE;
    }

    return FALSE;
}

/**
 * The subclass window procedure for the search control.
 *
 * \param WindowHandle A handle to the search window.
 * \param WindowMessage The window message.
 * \param wParam The WPARAM value from the window message.
 * \param lParam The LPARAM value from the window message.
 * \return The result of the message processing.
 */
LRESULT CALLBACK PhSearchWndSubclassProc(
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
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);

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

            PhSearchMatchDelete(&context->Match);

            if (context->TooltipHandle)
            {
                DestroyWindow(context->TooltipHandle);
                context->TooltipHandle = NULL;
            }

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

            // Deflate the client area to accommodate the custom buttons, plus the same
            // border inset reserved on the other three sides below - otherwise the text
            // area butts directly against the button row with no frame between them,
            // while the other sides show a proper 2px border. (dmex)
            ncCalcSize->rgrc[0].right -= (context->ButtonWidth * PH_SC_BUTTON_COUNT) + (context->BorderSize + 1);

            // Note: Also reserve the border drawn by PhSearchControlPaintFrame (a 2px-deep
            // frame: the outer edge plus an inset edge). Without this, the client edit control's
            // own rect still overlaps those pixels, so its redraws can leave stale fragments of
            // the border behind whenever the non-client area isn't repainted alongside it. (dmex)
            ncCalcSize->rgrc[0].left += (context->BorderSize + 1);
            ncCalcSize->rgrc[0].top += (context->BorderSize + 1);
            ncCalcSize->rgrc[0].bottom -= (context->BorderSize + 1);
        }
        return 0;
    case WM_NCPAINT:
        {
            if (PhSearchControlHandleNonClientPaint(context, WindowHandle, wParam))
                return 0;
        }
        break;
    case WM_NCHITTEST:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            // Get the screen coordinates of the mouse.
            //if (!PhGetCursorPos(&windowPoint))
            //    break;
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the screen coordinates of the window.
            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            // Get the position of the inserted buttons.
            PhSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
                return HTBORDER;

            PhSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
                return HTBORDER;

            PhSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
                return HTBORDER;

            PhSearchControlButtonRect(context, &context->FuzzyButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
                return HTBORDER;
        }
        break;
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        {
            // Note: The edit class has CS_DBLCLKS, so a second click shortly after the first
            // arrives as WM_NCLBUTTONDBLCLK instead of WM_NCLBUTTONDOWN. Treat it as a normal
            // press, otherwise the click is dropped. (dmex)
            UINT codeHitTest = (UINT)wParam;
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            // Get the screen coordinates of the mouse.
            //if (!PhGetMessagePos(&windowPoint))
            //    break;
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the screen coordinates of the window.
            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            PhSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            context->SearchButton.Pushed = PhPtInRect(&buttonRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            context->RegexButton.Pushed = PhPtInRect(&buttonRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            context->CaseButton.Pushed = PhPtInRect(&buttonRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->FuzzyButton, &windowRect, &buttonRect);
            context->FuzzyButton.Pushed = PhPtInRect(&buttonRect, &windowPoint);

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
            if (!PhGetMessagePos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            PhSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
            {
                SetFocus(WindowHandle);
                PhSetWindowText(WindowHandle, L"");
                PhSearchUpdateText(WindowHandle, context, FALSE);
            }

            PhSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
            {
                context->RegexButton.Active = !context->RegexButton.Active;
                PhSetIntegerSetting(context->RegexSetting, context->RegexButton.Active);
                if (context->RegexButton.Active)
                {
                    context->FuzzyButton.Active = FALSE;
                    PhSetIntegerSetting(context->FuzzySetting, FALSE);
                }
                PhSearchUpdateText(WindowHandle, context, TRUE);
            }

            PhSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
            {
                context->CaseButton.Active = !context->CaseButton.Active;
                PhSetIntegerSetting(context->CaseSetting, context->CaseButton.Active);
                PhSearchUpdateText(WindowHandle, context, FALSE);
            }

            PhSearchControlButtonRect(context, &context->FuzzyButton, &windowRect, &buttonRect);
            if (PhPtInRect(&buttonRect, &windowPoint))
            {
                context->FuzzyButton.Active = !context->FuzzyButton.Active;
                PhSetIntegerSetting(context->FuzzySetting, context->FuzzyButton.Active);
                if (context->FuzzyButton.Active)
                {
                    context->RegexButton.Active = FALSE;
                    PhSetIntegerSetting(context->RegexSetting, FALSE);
                }
                PhSearchUpdateText(WindowHandle, context, TRUE);
            }

            if (GetCapture() == WindowHandle)
            {
                context->SearchButton.Pushed = FALSE;
                context->RegexButton.Pushed = FALSE;
                context->CaseButton.Pushed = FALSE;
                context->FuzzyButton.Pushed = FALSE;
                ReleaseCapture();
            }

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_CAPTURECHANGED:
        {
            // Capture can be lost without a button-up (menus, modal dialogs);
            // clear the pushed state so the buttons don't stay painted down.
            context->SearchButton.Pushed = FALSE;
            context->RegexButton.Pushed = FALSE;
            context->CaseButton.Pushed = FALSE;
            context->FuzzyButton.Pushed = FALSE;

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
                            PhSearchUpdateText(WindowHandle, context, FALSE);
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
                            PhSearchUpdateText(WindowHandle, context, FALSE);
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
                            PhSearchUpdateText(WindowHandle, context, FALSE);
                        }
                        break;
                    case 5:
                        {
                            PPH_STRING startText = PH_AUTO(PhSubstring(text, 0, selStart));
                            PPH_STRING endText = PH_AUTO(PhSubstring(text, selEnd, text->Length / sizeof(WCHAR)));
                            PPH_STRING newText = PH_AUTO(PhConcatStringRef2(&startText->sr, &endText->sr));
                            PhSetWindowText(WindowHandle, newText->Buffer);
                            PhSearchUpdateText(WindowHandle, context, FALSE);
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

            PhSearchUpdateText(WindowHandle, context, FALSE);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
        break;
    case WM_SETFOCUS:
        {
            context->WindowFocus = TRUE;
            context->PreviousFocusWindowHandle = (HWND)wParam;

            //RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_KILLFOCUS:
        {
            context->WindowFocus = FALSE;

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            PhSearchControlThemeChanged(context, WindowHandle);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(context->ParentWindowHandle);

            PhSearchControlThemeChanged(context, WindowHandle);
        }
        break;
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;
            BOOLEAN wasHot;
            BOOLEAN oldHot;
            BOOLEAN oldSearchHot;
            BOOLEAN oldRegexHot;
            BOOLEAN oldCaseHot;
            BOOLEAN oldFuzzyHot;

            if (!PhGetMessagePos(&windowPoint))
                break;
            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            oldHot = !!context->Hot;
            oldSearchHot = !!context->SearchButton.Hot;
            oldRegexHot = !!context->RegexButton.Hot;
            oldCaseHot = !!context->CaseButton.Hot;
            oldFuzzyHot = !!context->FuzzyButton.Hot;

            context->Hot = PhPtInRect(&windowRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            wasHot = !!context->RegexButton.Hot;
            context->RegexButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            // Note: Only (re)pop the tooltip on the hot-state transition. Calling TTM_POPUP on
            // every WM_MOUSEMOVE while already hot causes the tooltip to flicker. (dmex)
            if (context->RegexButton.Hot && !wasHot)
            {
                PhSearchControlCreateTooltip(context, WindowHandle, &buttonRect, L"Regular Expression");
            }

            PhSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            wasHot = !!context->CaseButton.Hot;
            context->CaseButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            if (context->CaseButton.Hot && !wasHot)
            {
                PhSearchControlCreateTooltip(context, WindowHandle, &buttonRect, L"Match Case");
            }

            PhSearchControlButtonRect(context, &context->FuzzyButton, &windowRect, &buttonRect);
            wasHot = !!context->FuzzyButton.Hot;
            context->FuzzyButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            if (context->FuzzyButton.Hot && !wasHot)
            {
                PhSearchControlCreateTooltip(context, WindowHandle, &buttonRect, L"Fuzzy Match");
            }

            PhSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            wasHot = !!context->SearchButton.Hot;
            context->SearchButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            if (context->SearchButton.Hot && !wasHot)
            {
                PhSearchControlCreateTooltip(context, WindowHandle, &buttonRect, L"Clear Search");
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

            // Note: The buttons are painted in the non-client area, so the frame must be
            // invalidated for the hot state to become visible. Only redraw on the hot-state
            // transition since WM_MOUSEMOVE is generated for every mouse movement. (dmex)
            if (oldHot != !!context->Hot ||
                oldSearchHot != !!context->SearchButton.Hot ||
                oldRegexHot != !!context->RegexButton.Hot ||
                oldCaseHot != !!context->CaseButton.Hot ||
                oldFuzzyHot != !!context->FuzzyButton.Hot)
            {
                RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_MOUSELEAVE:
    case WM_NCMOUSELEAVE:
        {
            POINT windowPoint;
            RECT windowRect;
            RECT buttonRect;

            context->HotTrack = FALSE;

            if (!PhGetMessagePos(&windowPoint))
                break;
            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            context->Hot = PhPtInRect(&windowRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->SearchButton, &windowRect, &buttonRect);
            context->SearchButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->RegexButton, &windowRect, &buttonRect);
            context->RegexButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->CaseButton, &windowRect, &buttonRect);
            context->CaseButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            PhSearchControlButtonRect(context, &context->FuzzyButton, &windowRect, &buttonRect);
            context->FuzzyButton.Hot = PhPtInRect(&buttonRect, &windowPoint);

            if (context->TooltipHandle)
                SendMessage(context->TooltipHandle, TTM_POP, 0, 0);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_PAINT:
        {
            if (PhSearchControlHandleClientPaint(context, WindowHandle, oldWndProc))
                return 0;
        }
        break;
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
                    PWSTR textBuffer;

                    if ((ULONG64)textLength >= ((ULONG64)ULONG_MAX / sizeof(WCHAR)))
                        break;

                    textBuffer = PhAllocate((textLength + 1) * sizeof(WCHAR));

                    if (!NT_SUCCESS(PhGetWindowTextToBuffer(WindowHandle, 0, textBuffer, (ULONG)textLength + 1, &textBufferLength)))
                    {
                        PhFree(textBuffer);
                        break;
                    }

                    for (; 0 < textStart; --textStart)
                    {
                        if (textBuffer[textStart - 1] == L' ' && iswalnum(textBuffer[textStart]))
                        {
                            CallWindowProc(oldWndProc, WindowHandle, EM_SETSEL, textStart, textEnd);
                            CallWindowProc(oldWndProc, WindowHandle, EM_REPLACESEL, TRUE, (LPARAM)L"");
                            PhFree(textBuffer);
                            return 1;
                        }
                    }

                    if (textStart == 0)
                    {
                        PhSetWindowText(WindowHandle, L"");
                        PhSearchUpdateText(WindowHandle, context, FALSE);
                        PhFree(textBuffer);
                        return 1;
                    }

                    PhFree(textBuffer);
                }
            }
            // Clear search and restore focus for esc key
            else if (wParam == VK_ESCAPE)
            {
                PhSetWindowText(WindowHandle, L"");
                PhSearchUpdateText(WindowHandle, context, FALSE);
                PhSearchRestoreFocus(context);
                return 1;
            }
            // Up/down arrows will just restore previous focus without clearing search
            else if (wParam == VK_DOWN || wParam == VK_UP)
            {
                PhSearchRestoreFocus(context);
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
            // Intercept esc key only when there is text to clear or focus to restore,
            // otherwise let the dialog manager translate it to IDCANCEL. (dmex)
            if (wParam == VK_ESCAPE && lParam && ((MSG*)lParam)->message == WM_KEYDOWN)
            {
                if (GetWindowTextLength(WindowHandle) != 0 || context->PreviousFocusWindowHandle)
                    return DLGC_WANTMESSAGE;
            }
        }
        break;
    case EM_SETCUEBANNER:
        {
            PWSTR text = (PWSTR)lParam;

            PhMoveReference(&context->CueBannerText, PhCreateString(text));

            //RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
//DefaultWndProc:
//    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Creates an extended search control.
 *
 * \param ParentWindowHandle A handle to the parent window.
 * \param SearchWindowHandle A handle to the search edit control.
 * \param BannerText An optional string for the cue banner text.
 * \param ImageBaseAddress The base address of the image containing the button resources.
 * \param SearchButtonResource The resource name for the search button image.
 * \param SearchButtonActiveResource The resource name for the active search button image.
 * \param RegexButtonResource The resource name for the regular expression button image.
 * \param CaseButtonResource The resource name for the case-sensitive button image.
 * \param FuzzyButtonResource An optional resource name for the fuzzy search button image.
 * \param RegexSetting The setting name for the regular expression state.
 * \param CaseSetting The setting name for the case-sensitive state.
 * \param FuzzySetting The setting name for the fuzzy search state.
 * \param Callback A callback function that is invoked when the search text changes.
 * \param Context An optional user-defined value passed to the callback function.
 */
VOID PhCreateSearchControlEx(
    _In_ HWND ParentWindowHandle,
    _In_ HWND SearchWindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PVOID ImageBaseAddress,
    _In_ PCWSTR SearchButtonResource,
    _In_ PCWSTR SearchButtonActiveResource,
    _In_ PCWSTR RegexButtonResource,
    _In_ PCWSTR CaseButtonResource,
    _In_opt_ PCWSTR FuzzyButtonResource,
    _In_ PCWSTR RegexSetting,
    _In_ PCWSTR CaseSetting,
    _In_ PCWSTR FuzzySetting,
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
    context->FuzzySetting = FuzzySetting;

    context->ImageBaseAddress = ImageBaseAddress;
    context->SearchButtonResource = SearchButtonResource;
    context->SearchButtonActiveResource = SearchButtonActiveResource;
    context->RegexButtonResource = RegexButtonResource;
    context->CaseButtonResource = CaseButtonResource;
    context->FuzzyButtonResource = FuzzyButtonResource;

    context->Callback = Callback;
    context->CallbackContext = Context;

    context->RegexButton.Active = !!PhGetIntegerSetting(context->RegexSetting);
    context->CaseButton.Active = !!PhGetIntegerSetting(context->CaseSetting);
    context->FuzzyButton.Active = !!PhGetIntegerSetting(context->FuzzySetting);

    if (context->FuzzyButton.Active)
        context->RegexButton.Active = FALSE;

    // Subclass the Edit control window procedure.
    context->DefaultWindowProc = PhGetWindowProcedure(SearchWindowHandle);
    PhSetWindowContext(SearchWindowHandle, SHRT_MAX, context);
    PhSetWindowProcedure(SearchWindowHandle, PhSearchWndSubclassProc);

    // The control draws a 2px non-client border, so it needs the client edge to reserve that
    // non-client space. The generic edit theming path skips this control (it already owns the
    // window context), so set the extended style here. The SWP_FRAMECHANGED in
    // PhpSearchControlThemeChanged below forces WM_NCCALCSIZE to recompute. (dmex)
    PhSetWindowExStyle(SearchWindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);

    // Initialize the theme parameters.
    PhSearchControlThemeChanged(context, SearchWindowHandle);

}

/**
 * Clears the search text and restores focus to the search control.
 *
 * \param SearchWindowHandle A handle to the search window.
 */
VOID PhSearchControlClear(
    _In_ HWND SearchWindowHandle
    )
{
    SetFocus(SearchWindowHandle);
    PhSetWindowText(SearchWindowHandle, L"");
}

/**
 * Checks if a text string matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Text The text to match against the search criteria.
 * \return TRUE if the text matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchControlMatch(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text
    )
{
    return PhSearchMatchString(MatchHandle, Text);
}

/**
 * Checks if a text string matches the current search criteria and retrieves the match ranges.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Text The text to match against the search criteria.
 * \param Ranges A buffer that receives the match ranges.
 * \param MaximumRanges The maximum number of ranges that the buffer can hold.
 * \param RangeCount A variable which receives the number of match ranges.
 * \return TRUE if the text matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchControlMatchEx(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text,
    _Out_writes_to_opt_(MaximumRanges, *RangeCount) PPH_SEARCHCONTROL_MATCH_RANGE Ranges,
    _In_ ULONG MaximumRanges,
    _Out_opt_ PULONG RangeCount
    )
{
    return PhSearchMatchStringEx(MatchHandle, Text, Ranges, MaximumRanges, RangeCount);
}

/**
 * Checks if a null-terminated text string matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Text The null-terminated text to match against the search criteria.
 * \return TRUE if the text matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchControlMatchZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PhSearchControlMatch(MatchHandle, &text);
}

/**
 * Checks if a long null-terminated text string matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Text The long null-terminated text to match against the search criteria.
 * \return TRUE if the text matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchControlMatchLongHintZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRefLongHint(&text, Text);

    return PhSearchControlMatch(MatchHandle, &text);
}

/**
 * Checks if a pointer matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Pointer The pointer to match against the search criteria.
 * \return TRUE if the pointer matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchControlMatchPointer(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer
    )
{
    return PhSearchMatchPointer(MatchHandle, Pointer);
}

/**
 * Checks if any pointer in a given range matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Pointer The start of the pointer range.
 * \param Size The size of the pointer range, in bytes.
 * \return TRUE if a pointer in the range matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchControlMatchPointerRange(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer,
    _In_ SIZE_T Size
    )
{
    return PhSearchMatchPointerRange(MatchHandle, Pointer, Size);
}

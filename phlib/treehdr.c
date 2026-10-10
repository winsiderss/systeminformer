/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <ph.h>
#include <commctrl.h>
#include <guisup.h>
#include <guisupp.h>
#include <treehdrp.h>
#include <vssym32.h>

#define PhHeaderNewIsUserMessage(WindowMessage) ( \
    ((WindowMessage) >= HDM_FIRST && (WindowMessage) <= HDM_FIRST + 0x40) || \
    ((WindowMessage) >= CCM_FIRST && (WindowMessage) <= CCM_LAST) || \
    ((WindowMessage) >= PHHM_FIRST && (WindowMessage) <= PHHM_LAST))

typedef struct _PH_HEADERNEW_COLORS
{
    COLORREF Background;
    COLORREF Hot;
    COLORREF Pressed;
    COLORREF Divider;
    COLORREF Text;
    COLORREF Marker;
} PH_HEADERNEW_COLORS, *PPH_HEADERNEW_COLORS;

static BOOLEAN PhHeaderNewUseDarkTheme(
    VOID
    )
{
    if (!PhEnableThemeSupport)
        return FALSE;

    return PhGetColorBrightness(PhThemeWindowBackgroundColor) < PH_HEADERNEW_DARK_THEME_BRIGHTNESS;
}

/**
 * Registers the PhHeaderNew window class.
 *
 * \return The class atom, or RTL_ATOM_INVALID_ATOM on failure.
 */
RTL_ATOM PhHeaderNewInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wcex.lpfnWndProc = PhHeaderNewWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_HEADERNEW_CLASSNAME;
    //wcex.hbrBackground = PhThemeWindowBackgroundBrush;

    return RegisterClassEx(&wcex);
}

//
// Items
//

PPH_HEADERNEW_ITEM PhHeaderNewGetItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index
    )
{
    if (Index < 0 || (ULONG)Index >= Context->Items->Count)
        return NULL;

    return (PPH_HEADERNEW_ITEM)Context->Items->Items[Index];
}

VOID PhHeaderNewFreeItem(
    _In_ _Post_invalid_ PPH_HEADERNEW_ITEM Item
    )
{
    PhClearReference(&Item->Text);
    PhFree(Item);
}

/**
 * Maps a display order to an item index.
 *
 * \return The item index, or INT_ERROR if no item occupies that order.
 */
LONG PhHeaderNewOrderToIndex(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Order
    )
{
    ULONG i;

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_HEADERNEW_ITEM item = Context->Items->Items[i];

        if (item->Order == Order)
            return (LONG)i;
    }

    return INT_ERROR;
}

/**
 * Rewrites the order values so that they form the contiguous range
 * 0..Count-1 while preserving the relative left-to-right arrangement. Ties are
 * broken by index, which matches the way SysHeader32 settles ambiguous orders.
 */
VOID PhHeaderNewNormalizeOrder(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    PLONG ranks;
    ULONG count;
    ULONG i;
    ULONG j;

    if (!(count = Context->Items->Count))
        return;

    // Rank each item by (Order, index), then renumber. Ties are broken by index,
    // which is how SysHeader32 settles ambiguous orders. The item count is small
    // enough that anything cleverer than this is not worth the code.

    ranks = PhAllocate(count * sizeof(LONG));

    for (i = 0; i < count; i++)
    {
        PPH_HEADERNEW_ITEM item = Context->Items->Items[i];
        LONG rank = 0;

        for (j = 0; j < count; j++)
        {
            PPH_HEADERNEW_ITEM other = Context->Items->Items[j];

            if (j == i)
                continue;

            if (other->Order < item->Order || (other->Order == item->Order && j < i))
                rank++;
        }

        ranks[i] = rank;
    }

    for (i = 0; i < count; i++)
    {
        PPH_HEADERNEW_ITEM item = Context->Items->Items[i];

        item->Order = ranks[i];
    }

    PhFree(ranks);
}

/**
 * Recalculates each item's rectangle from its width and order.
 */
VOID PhHeaderNewLayoutItems(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    RECT clientRect;
    LONG bottom;
    LONG x = 0;
    LONG order;

    if (PhGetClientRect(Context->WindowHandle, &clientRect) && clientRect.bottom > 0)
        bottom = clientRect.bottom;
    else
        bottom = Context->HeaderHeight;

    for (order = 0; order < (LONG)Context->Items->Count; order++)
    {
        PPH_HEADERNEW_ITEM item;
        LONG index;

        if ((index = PhHeaderNewOrderToIndex(Context, order)) == INT_ERROR)
            continue;

        item = Context->Items->Items[index];
        item->Rect.left = x;
        item->Rect.top = 0;
        item->Rect.right = x + item->Width;
        item->Rect.bottom = bottom;
        x = item->Rect.right;
    }

    Context->NeedsLayout = FALSE;
}

VOID PhHeaderNewInvalidate(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_opt_ PRECT Rect
    )
{
    if (!Context->Redraw)
        return;

    InvalidateRect(Context->WindowHandle, Rect, FALSE);
}

VOID PhHeaderNewInvalidateItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index
    )
{
    PPH_HEADERNEW_ITEM item;

    if (item = PhHeaderNewGetItem(Context, Index))
        PhHeaderNewInvalidate(Context, &item->Rect);
}

//
// Metrics, font and theme
//

VOID PhHeaderNewUpdateDpiMetrics(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    Context->PaddingX = PhScaleToDisplay(PH_HEADERNEW_PADDING_X, Context->WindowDpi);
    Context->PaddingY = PhScaleToDisplay(PH_HEADERNEW_PADDING_Y, Context->WindowDpi);
    Context->DividerGrip = PhScaleToDisplay(PH_HEADERNEW_DIVIDER_GRIP, Context->WindowDpi);
    Context->BitmapMargin = PhScaleToDisplay(PH_HEADERNEW_BITMAP_MARGIN, Context->WindowDpi);
}

VOID PhHeaderNewUpdateHeight(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    TEXTMETRIC textMetrics;
    HDC hdc;
    LONG height = 0;

    if (Context->CustomHeight > 0)
    {
        Context->HeaderHeight = Context->CustomHeight;
        return;
    }

    if (hdc = GetDC(Context->WindowHandle))
    {
        HFONT oldFont = NULL;

        if (Context->FontHandle)
            oldFont = SelectFont(hdc, Context->FontHandle);

        if (GetTextMetrics(hdc, &textMetrics))
            height = textMetrics.tmHeight + textMetrics.tmExternalLeading;

        if (oldFont)
            SelectFont(hdc, oldFont);

        ReleaseDC(Context->WindowHandle, hdc);
    }

    height += Context->PaddingY * 2;

    Context->HeaderHeight = max(height, PhScaleToDisplay(PH_HEADERNEW_MINIMUM_HEIGHT, Context->WindowDpi));
}

VOID PhHeaderNewUpdateFont(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    HFONT fontHandle;

    // Only replace a font we created ourselves. A font handed to us by
    // WM_SETFONT belongs to the caller.

    if (Context->FontHandle && !Context->OwnFont)
        return;

    if (!(fontHandle = PhCreateCommonFont(-11, FW_NORMAL, Context->WindowHandle, Context->WindowDpi)))
        return;

    if (Context->FontHandle && Context->OwnFont)
        DeleteFont(Context->FontHandle);

    Context->FontHandle = fontHandle;
    Context->OwnFont = TRUE;
}

VOID PhHeaderNewUpdateTheme(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    if (Context->ThemeHandle)
    {
        PhCloseThemeData(Context->ThemeHandle);
        Context->ThemeHandle = NULL;
    }

    Context->ThemeDark = PhHeaderNewUseDarkTheme();
    Context->Theme = PhHeaderNewThemeUxTheme;
    Context->ThemeHandle = PhOpenThemeData(Context->WindowHandle, VSCLASS_HEADER, Context->WindowDpi);
}

VOID PhHeaderNewGetColors(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _Out_ PPH_HEADERNEW_COLORS Colors
    )
{
    if (Context->ThemeDark)
    {
        Colors->Background = PhThemeWindowBackgroundColor;
        Colors->Hot = PhThemeWindowHighlight2Color != Colors->Background ?
            PhThemeWindowHighlight2Color : PhThemeWindowHighlightColor;
        Colors->Pressed = PhThemeWindowHighlightColor;
        Colors->Divider = PhThemeWindowBorderColor;
        Colors->Text = PhThemeWindowTextColor;
        Colors->Marker = PhThemeWindowFocusBorderColor;
    }
    else
    {
        Colors->Background = PhEnableThemeSupport ?
            PhThemeWindowBackgroundColor : PH_HEADERNEW_COLOR_BACKGROUND;
        Colors->Hot = PH_HEADERNEW_COLOR_HOT;
        Colors->Pressed = PH_HEADERNEW_COLOR_PRESSED;
        Colors->Divider = PH_HEADERNEW_COLOR_DIVIDER;
        Colors->Text = PH_HEADERNEW_COLOR_TEXT;
        Colors->Marker = GetSysColor(COLOR_HOTLIGHT);
    }
}

// Notifications

LRESULT PhHeaderNewNotify(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ ULONG Code
    )
{
    NMHDR notify;

    memset(&notify, 0, sizeof(NMHDR));
    notify.hwndFrom = Context->WindowHandle;
    notify.idFrom = Context->Id;
    notify.code = Code;

    if (Context->Callback)
        Context->Callback(Context->WindowHandle, Code, &notify, NULL, Context->Context);

    if (!Context->ParentHandle)
        return 0;

    return SendMessage(Context->ParentHandle, WM_NOTIFY, (WPARAM)Context->Id, (LPARAM)&notify);
}

/**
 * Sends an item notification. \a Item is passed through to the handler as
 * NMHEADER.pitem, which handlers of HDN_ITEMCHANGING are allowed to modify in
 * place - TreeNew clamps the fixed column width that way.
 * \return The value returned by the parent.
 */
LRESULT PhHeaderNewNotifyItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ ULONG Code,
    _In_ LONG Index,
    _In_opt_ HDITEM *Item,
    _In_ LONG Button
    )
{
    NMHEADER notify;

    memset(&notify, 0, sizeof(NMHEADER));
    notify.hdr.hwndFrom = Context->WindowHandle;
    notify.hdr.idFrom = Context->Id;
    notify.hdr.code = Code;
    notify.iItem = Index;
    notify.iButton = Button;
    notify.pitem = Item;

    if (Context->Callback)
        Context->Callback(Context->WindowHandle, Code, &notify, NULL, Context->Context);

    if (!Context->ParentHandle)
        return 0;

    return SendMessage(Context->ParentHandle, WM_NOTIFY, (WPARAM)Context->Id, (LPARAM)&notify);
}

/**
 * Resolves LPSTR_TEXTCALLBACK / I_IMAGECALLBACK items through HDN_GETDISPINFO.
 * The returned text is owned by the parent and is only valid until the next
 * call, so it is copied into \a Buffer.
 */
_Success_(return != NULL)
PCWSTR PhHeaderNewGetDispInfo(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ PPH_HEADERNEW_ITEM Item,
    _In_ LONG Index,
    _Out_writes_z_(BufferLength) PWSTR Buffer,
    _In_ ULONG BufferLength
    )
{
    NMHDDISPINFO dispInfo;

    if (!Item->TextCallback || !Context->ParentHandle)
        return NULL;

    Buffer[0] = UNICODE_NULL;

    memset(&dispInfo, 0, sizeof(NMHDDISPINFO));
    dispInfo.hdr.hwndFrom = Context->WindowHandle;
    dispInfo.hdr.idFrom = Context->Id;
    dispInfo.hdr.code = HDN_GETDISPINFO;
    dispInfo.iItem = Index;
    dispInfo.mask = HDI_TEXT;
    dispInfo.lParam = Item->Param;
    dispInfo.pszText = Buffer;
    dispInfo.cchTextMax = BufferLength;

    SendMessage(Context->ParentHandle, WM_NOTIFY, (WPARAM)Context->Id, (LPARAM)&dispInfo);

    if (dispInfo.pszText && dispInfo.pszText != Buffer)
    {
        PhCopyStringZ(dispInfo.pszText, SIZE_MAX, Buffer, BufferLength, NULL);
    }

    return Buffer;
}

// Hit testing

/**
 * Locates the item and part under \a Point.
 *
 * \param Flags Receives a combination of HHT_* flags.
 * \return The item index, or INT_ERROR when the point is not over an item. For
 * HHT_ONDIVIDER the index is that of the item to the left of the divider.
 */
LONG PhHeaderNewHitTest(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ PPOINT Point,
    _Out_ PULONG Flags
    )
{
    RECT clientRect;
    LONG order;
    LONG lastIndex = INT_ERROR;
    LONG lastRight = 0;

    *Flags = HHT_NOWHERE;

    if (!PhGetClientRect(Context->WindowHandle, &clientRect))
        return INT_ERROR;

    if (Point->y < clientRect.top)
    {
        *Flags = HHT_ABOVE;
        return INT_ERROR;
    }

    if (Point->y >= clientRect.bottom)
    {
        *Flags = HHT_BELOW;
        return INT_ERROR;
    }

    for (order = 0; order < (LONG)Context->Items->Count; order++)
    {
        PPH_HEADERNEW_ITEM item;
        LONG index;

        if ((index = PhHeaderNewOrderToIndex(Context, order)) == INT_ERROR)
            continue;

        item = Context->Items->Items[index];
        lastIndex = index;
        lastRight = item->Rect.right;

        if (Point->x < item->Rect.left)
            break;

        if (Point->x >= item->Rect.right)
            continue;

        // Inside this item. Decide between the body and either divider.

        if (!FlagOn(Context->Style, HDS_NOSIZING))
        {
            if (Point->x >= item->Rect.right - Context->DividerGrip)
            {
                *Flags = HHT_ONDIVIDER;
                return index;
            }

            if (order > 0 && Point->x < item->Rect.left + Context->DividerGrip)
            {
                LONG previous = PhHeaderNewOrderToIndex(Context, order - 1);

                if (previous != INT_ERROR)
                {
                    PPH_HEADERNEW_ITEM previousItem = Context->Items->Items[previous];

                    // A zero-width neighbour cannot be grabbed by its own right
                    // edge, so its divider is reported as HHT_ONDIVOPEN.
                    *Flags = (previousItem->Width == 0) ? HHT_ONDIVOPEN : HHT_ONDIVIDER;
                    return previous;
                }
            }
        }

        *Flags = HHT_ONHEADER;
        return index;
    }

    // Past the last item: the trailing divider is still grabbable.

    if (lastIndex != INT_ERROR && !FlagOn(Context->Style, HDS_NOSIZING) &&
        Point->x >= lastRight && Point->x < lastRight + Context->DividerGrip)
    {
        *Flags = HHT_ONDIVIDER;
        return lastIndex;
    }

    *Flags = (Point->x < 0) ? HHT_TOLEFT : HHT_TORIGHT;

    return INT_ERROR;
}

// Drawing

VOID PhHeaderNewDeleteBuffer(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    if (Context->BufferedOldBitmap)
    {
        SelectBitmap(Context->BufferedDc, Context->BufferedOldBitmap);
        Context->BufferedOldBitmap = NULL;
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

    memset(&Context->BufferedRect, 0, sizeof(RECT));
}

BOOLEAN PhHeaderNewCreateBuffer(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ HDC Hdc
    )
{
    RECT clientRect;

    if (!PhGetClientRect(Context->WindowHandle, &clientRect))
        return FALSE;

    if (clientRect.right <= 0 || clientRect.bottom <= 0)
        return FALSE;

    if (Context->BufferedDc && PhEqualRect(&clientRect, &Context->BufferedRect))
        return TRUE;

    PhHeaderNewDeleteBuffer(Context);

    if (!(Context->BufferedDc = CreateCompatibleDC(Hdc)))
        return FALSE;

    if (!(Context->BufferedBitmap = PhCreateDIBSection(Hdc, PHBF_DIB, clientRect.right, clientRect.bottom, NULL)))
    {
        DeleteDC(Context->BufferedDc);
        Context->BufferedDc = NULL;
        return FALSE;
    }

    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedDc, Context->BufferedBitmap);
    Context->BufferedRect = clientRect;

    return TRUE;
}

VOID PhHeaderNewDrawSortArrow(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN Ascending,
    _In_ PPH_HEADERNEW_COLORS Colors
    )
{
    RECT arrowRect = *Rect;
    LONG state = Ascending ? HSAS_SORTEDUP : HSAS_SORTEDDOWN;

    if (Context->ThemeHandle)
    {
        SIZE arrowSize;

        if (PhGetThemePartSize(
            Context->ThemeHandle,
            Hdc,
            HP_HEADERSORTARROW,
            state,
            NULL,
            THEMEPARTSIZE_TRUE,
            &arrowSize
            ))
        {
            arrowRect.bottom = arrowRect.top + arrowSize.cy;
        }

        if (PhDrawThemeBackground(Context->ThemeHandle, Hdc, HP_HEADERSORTARROW, state, &arrowRect, NULL))
            return;
    }

    // No theme, or the theme has no sort arrow part. Draw a small triangle
    // against the right edge, the way the classic header does.
    {
        LONG size = PhScaleToDisplay(5, Context->WindowDpi);
        LONG centerX = arrowRect.right - Context->PaddingX - size;
        LONG centerY = (arrowRect.top + arrowRect.bottom) / 2;
        POINT points[3];
        HBRUSH oldBrush;
        HPEN oldPen;

        if (Ascending)
        {
            points[0].x = centerX;              points[0].y = centerY - size / 2;
            points[1].x = centerX - size;       points[1].y = centerY + size / 2;
            points[2].x = centerX + size;       points[2].y = centerY + size / 2;
        }
        else
        {
            points[0].x = centerX;              points[0].y = centerY + size / 2;
            points[1].x = centerX - size;       points[1].y = centerY - size / 2;
            points[2].x = centerX + size;       points[2].y = centerY - size / 2;
        }

        SetDCBrushColor(Hdc, Colors->Text);
        SetDCPenColor(Hdc, Colors->Text);
        oldBrush = SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
        oldPen = SelectPen(Hdc, PhGetStockPen(DC_PEN));
        Polygon(Hdc, points, (INT)RTL_NUMBER_OF(points));
        SelectPen(Hdc, oldPen);
        SelectBrush(Hdc, oldBrush);
    }
}

VOID PhHeaderNewDrawItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ LONG Index,
    _In_ PPH_HEADERNEW_ITEM Item,
    _In_ PPH_HEADERNEW_COLORS Colors
    )
{
    RECT rect = Item->Rect;
    RECT textRect;
    BOOLEAN hot = (Context->HotIndex == Index);
    BOOLEAN pressed = (Context->PressedIndex == Index);
    WCHAR textBuffer[MAX_PATH];
    PCWSTR text = NULL;
    ULONG textLength = 0;
    HFONT oldFont = NULL;

    if (rect.right <= rect.left)
        return;

    // Background.

    if (!FlagOn(Context->Flags, PHHF_NOTHEMEBACKGROUND))
    {
        BOOLEAN drawn = FALSE;

        if (!Context->ThemeDark && Context->ThemeHandle && Context->Theme != PhHeaderNewThemeWin10)
        {
            LONG state = pressed ? HIS_PRESSED : (hot ? HIS_HOT : HIS_NORMAL);

            drawn = PhDrawThemeBackground(Context->ThemeHandle, Hdc, HP_HEADERITEM, state, &rect, NULL);
        }

        if (!drawn)
        {
            SetDCBrushColor(Hdc, pressed ? Colors->Pressed : (hot ? Colors->Hot : Colors->Background));
            FillRect(Hdc, &rect, PhGetStockBrush(DC_BRUSH));
        }
    }

    // Divider along the right edge.

    if (!FlagOn(Context->Flags, PHHF_NODIVIDERS))
    {
        HBRUSH oldBrush;

        SetDCBrushColor(Hdc, Colors->Divider);
        oldBrush = SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
        PatBlt(Hdc, rect.right - 1, rect.top, 1, rect.bottom - rect.top, PATCOPY);

        if (FlagOn(Context->Flags, PHHF_BOTTOMBORDER))
            PatBlt(Hdc, rect.left, rect.bottom - 1, rect.right - rect.left, 1, PATCOPY);

        SelectBrush(Hdc, oldBrush);
    }

    textRect = rect;
    textRect.left += Context->PaddingX;
    textRect.right -= Context->PaddingX;
    textRect.top += Context->PaddingY;
    textRect.bottom -= Context->PaddingY;

    // Sort arrow. Drawn before the text so the text rect can be shortened when
    // the arrow sits beside it rather than above it.

    if (FlagOn(Item->Format, HDF_SORTUP | HDF_SORTDOWN))
    {
        PhHeaderNewDrawSortArrow(
            Context,
            Hdc,
            &rect,
            !!FlagOn(Item->Format, HDF_SORTUP),
            Colors
            );
    }

    // Image.

    if (Context->ImageListHandle && Item->ImageIndex >= 0 &&
        FlagOn(Item->Format, HDF_IMAGE | HDF_BITMAP | HDF_BITMAP_ON_RIGHT))
    {
        LONG cx = 0;
        LONG cy = 0;

        if (ImageList_GetIconSize(Context->ImageListHandle, &cx, &cy))
        {
            LONG imageY = textRect.top + ((textRect.bottom - textRect.top) - cy) / 2;
            LONG imageX;

            if (FlagOn(Item->Format, HDF_BITMAP_ON_RIGHT))
            {
                imageX = textRect.right - cx;
                textRect.right = imageX - Context->BitmapMargin;
            }
            else
            {
                imageX = textRect.left;
                textRect.left = imageX + cx + Context->BitmapMargin;
            }

            ImageList_Draw(Context->ImageListHandle, Item->ImageIndex, Hdc, imageX, imageY, ILD_TRANSPARENT);
        }
    }

    // Text.

    if (Item->TextCallback)
        text = PhHeaderNewGetDispInfo(Context, Item, Index, textBuffer, RTL_NUMBER_OF(textBuffer));
    else if (Item->Text)
        text = Item->Text->Buffer;

    if (text && (textLength = (ULONG)PhCountStringZ(text)) && textRect.right > textRect.left)
    {
        ULONG format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_HIDEPREFIX;

        if (FlagOn(Item->Format, HDF_RIGHT))
            format |= DT_RIGHT;
        else if (FlagOn(Item->Format, HDF_CENTER))
            format |= DT_CENTER;
        else
            format |= DT_LEFT;

        if (FlagOn(Item->Format, HDF_RTLREADING))
            format |= DT_RTLREADING;

        if (Context->FontHandle)
            oldFont = SelectFont(Hdc, Context->FontHandle);

        SetBkMode(Hdc, TRANSPARENT);
        SetTextColor(Hdc, Colors->Text);
        DrawText(Hdc, text, (LONG)textLength, &textRect, format);

        if (oldFont)
            SelectFont(Hdc, oldFont);
    }
}

VOID PhHeaderNewDrawInsertionMarker(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PPH_HEADERNEW_COLORS Colors
    )
{
    RECT clientRect;
    LONG width = PhScaleToDisplay(2, Context->WindowDpi);
    LONG x;
    LONG index;
    HBRUSH oldBrush;

    if (Context->HotDivider == INT_ERROR)
        return;

    if (!PhGetClientRect(Context->WindowHandle, &clientRect))
        return;

    if ((index = PhHeaderNewOrderToIndex(Context, Context->HotDivider)) != INT_ERROR)
    {
        PPH_HEADERNEW_ITEM item = Context->Items->Items[index];
        x = item->Rect.left;
    }
    else if (Context->Items->Count &&
        (index = PhHeaderNewOrderToIndex(Context, (LONG)Context->Items->Count - 1)) != INT_ERROR)
    {
        PPH_HEADERNEW_ITEM item = Context->Items->Items[index];
        x = item->Rect.right - width;
    }
    else
    {
        return;
    }

    SetDCBrushColor(Hdc, Colors->Marker);
    oldBrush = SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
    PatBlt(Hdc, x, clientRect.top, width, clientRect.bottom - clientRect.top, PATCOPY);
    SelectBrush(Hdc, oldBrush);
}

VOID PhHeaderNewDraw(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT PaintRect
    )
{
    PH_HEADERNEW_COLORS colors;
    NMCUSTOMDRAW customDraw;
    RECT clientRect;
    LRESULT result;
    BOOLEAN notifyItems = FALSE;
    LONG order;

    if (!PhGetClientRect(Context->WindowHandle, &clientRect))
        return;

    PhHeaderNewGetColors(Context, &colors);

    SetBkMode(Hdc, TRANSPARENT);
    SetDCBrushColor(Hdc, colors.Background);
    FillRect(Hdc, PaintRect, PhGetStockBrush(DC_BRUSH));

    memset(&customDraw, 0, sizeof(NMCUSTOMDRAW));
    customDraw.hdr.hwndFrom = Context->WindowHandle;
    customDraw.hdr.idFrom = Context->Id;
    customDraw.hdr.code = NM_CUSTOMDRAW;
    customDraw.hdc = Hdc;
    customDraw.dwDrawStage = CDDS_PREPAINT;
    customDraw.rc = clientRect;

    if (Context->ParentHandle)
        result = SendMessage(Context->ParentHandle, WM_NOTIFY, (WPARAM)Context->Id, (LPARAM)&customDraw);
    else
        result = CDRF_DODEFAULT;

    if (FlagOn(result, CDRF_SKIPDEFAULT))
        return;

    notifyItems = FlagOn(result, CDRF_NOTIFYITEMDRAW) ? TRUE : FALSE;

    for (order = 0; order < (LONG)Context->Items->Count; order++)
    {
        PPH_HEADERNEW_ITEM item;
        LONG index;

        if ((index = PhHeaderNewOrderToIndex(Context, order)) == INT_ERROR)
            continue;

        item = Context->Items->Items[index];

        // Skip items that fall outside the update rectangle. The buffered DC
        // carries no clip region, so this has to be an explicit test rather
        // than a RectVisible call.
        if (item->Rect.right <= PaintRect->left || item->Rect.left >= PaintRect->right)
            continue;

        if (notifyItems)
        {
            LRESULT itemResult;

            customDraw.dwDrawStage = CDDS_ITEMPREPAINT;
            customDraw.rc = item->Rect;
            customDraw.dwItemSpec = (ULONG_PTR)index;
            customDraw.lItemlParam = item->Param;
            customDraw.uItemState = 0;

            if (Context->HotIndex == index)
                customDraw.uItemState |= CDIS_HOT;
            if (Context->PressedIndex == index)
                customDraw.uItemState |= CDIS_SELECTED | CDIS_SHOWKEYBOARDCUES;
            if (Context->FocusedIndex == index)
                customDraw.uItemState |= CDIS_FOCUS;

            itemResult = SendMessage(Context->ParentHandle, WM_NOTIFY, (WPARAM)Context->Id, (LPARAM)&customDraw);

            if (FlagOn(itemResult, CDRF_SKIPDEFAULT))
                continue;
        }

        PhHeaderNewDrawItem(Context, Hdc, index, item, &colors);
    }

    if (Context->Dragging)
        PhHeaderNewDrawInsertionMarker(Context, Hdc, &colors);
}

VOID PhHeaderNewPaint(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT PaintRect
    )
{
    PH_HEADERNEW_COLORS colors;

    if (Context->NeedsLayout)
        PhHeaderNewLayoutItems(Context);

    PhHeaderNewGetColors(Context, &colors);

    if (PhHeaderNewCreateBuffer(Context, Hdc))
    {
        SetDCBrushColor(Context->BufferedDc, colors.Background);
        FillRect(Context->BufferedDc, PaintRect, PhGetStockBrush(DC_BRUSH));
        PhHeaderNewDraw(Context, Context->BufferedDc, PaintRect);

        BitBlt(
            Hdc,
            PaintRect->left,
            PaintRect->top,
            PaintRect->right - PaintRect->left,
            PaintRect->bottom - PaintRect->top,
            Context->BufferedDc,
            PaintRect->left,
            PaintRect->top,
            SRCCOPY
            );
    }
    else
    {
        SetDCBrushColor(Hdc, colors.Background);
        FillRect(Hdc, PaintRect, PhGetStockBrush(DC_BRUSH));
        PhHeaderNewDraw(Context, Hdc, PaintRect);
    }
}

// Item management

LONG PhHeaderNewInsertItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ HDITEM *Item
    )
{
    PPH_HEADERNEW_ITEM item;
    LONG index;

    if (!Item)
        return INT_ERROR;

    item = PhAllocateZero(sizeof(PH_HEADERNEW_ITEM));
    item->ImageIndex = INT_ERROR;

    index = Index;

    if (index < 0 || (ULONG)index > Context->Items->Count)
        index = (LONG)Context->Items->Count;

    if (FlagOn(Item->mask, HDI_TEXT) && Item->pszText)
    {
        if (Item->pszText == LPSTR_TEXTCALLBACK)
            item->TextCallback = TRUE;
        else
            item->Text = PhCreateString(Item->pszText);
    }

    // HDI_HEIGHT is an alias of HDI_WIDTH in commctrl.h - both name the cxy
    // field - so there is only one value to store.

    if (FlagOn(Item->mask, HDI_WIDTH))
        item->Width = Item->cxy;
    if (FlagOn(Item->mask, HDI_FORMAT))
        item->Format = Item->fmt;
    if (FlagOn(Item->mask, HDI_LPARAM))
        item->Param = Item->lParam;
    if (FlagOn(Item->mask, HDI_BITMAP))
        item->Bitmap = Item->hbm;
    if (FlagOn(Item->mask, HDI_STATE))
        item->State = Item->state;

    if (FlagOn(Item->mask, HDI_IMAGE))
    {
        if (Item->iImage == I_IMAGECALLBACK)
            item->ImageCallback = TRUE;
        else
            item->ImageIndex = Item->iImage;
    }

    // The new item takes the requested order, pushing everything at or after
    // that position one place to the right. Without HDI_ORDER it goes last.

    if (FlagOn(Item->mask, HDI_ORDER))
    {
        LONG order = Item->iOrder;
        ULONG i;

        if (order < 0)
            order = 0;
        if (order > (LONG)Context->Items->Count)
            order = (LONG)Context->Items->Count;

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_HEADERNEW_ITEM other = Context->Items->Items[i];

            if (other->Order >= order)
                other->Order++;
        }

        item->Order = order;
    }
    else
    {
        item->Order = (LONG)Context->Items->Count;
    }

    PhInsertItemList(Context->Items, (ULONG)index, item);
    PhHeaderNewNormalizeOrder(Context);

    Context->NeedsLayout = TRUE;
    PhHeaderNewLayoutItems(Context);
    PhHeaderNewInvalidate(Context, NULL);

    return index;
}

BOOLEAN PhHeaderNewSetItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ HDITEM *Item
    )
{
    PPH_HEADERNEW_ITEM item;
    BOOLEAN layout = FALSE;

    if (!Item)
        return FALSE;

    if (!(item = PhHeaderNewGetItem(Context, Index)))
        return FALSE;

    if (FlagOn(Item->mask, HDI_TEXT))
    {
        PhClearReference(&item->Text);
        item->TextCallback = FALSE;

        if (Item->pszText == LPSTR_TEXTCALLBACK)
            item->TextCallback = TRUE;
        else if (Item->pszText)
            item->Text = PhCreateString(Item->pszText);
    }

    if (FlagOn(Item->mask, HDI_WIDTH))
    {
        if (item->Width != Item->cxy)
            layout = TRUE;

        item->Width = Item->cxy;
    }

    if (FlagOn(Item->mask, HDI_FORMAT))
        item->Format = Item->fmt;
    if (FlagOn(Item->mask, HDI_LPARAM))
        item->Param = Item->lParam;
    if (FlagOn(Item->mask, HDI_BITMAP))
        item->Bitmap = Item->hbm;
    if (FlagOn(Item->mask, HDI_STATE))
        item->State = Item->state;

    if (FlagOn(Item->mask, HDI_IMAGE))
    {
        item->ImageCallback = FALSE;

        if (Item->iImage == I_IMAGECALLBACK)
            item->ImageCallback = TRUE;
        else
            item->ImageIndex = Item->iImage;
    }

    if (FlagOn(Item->mask, HDI_ORDER))
    {
        LONG order = Item->iOrder;

        if (order < 0)
            order = 0;
        if (order >= (LONG)Context->Items->Count)
            order = (LONG)Context->Items->Count - 1;

        if (item->Order != order)
        {
            ULONG i;
            LONG oldOrder = item->Order;

            for (i = 0; i < Context->Items->Count; i++)
            {
                PPH_HEADERNEW_ITEM other = Context->Items->Items[i];

                if (other == item)
                    continue;

                if (oldOrder < order)
                {
                    if (other->Order > oldOrder && other->Order <= order)
                        other->Order--;
                }
                else
                {
                    if (other->Order >= order && other->Order < oldOrder)
                        other->Order++;
                }
            }

            item->Order = order;
            layout = TRUE;
        }
    }

    if (layout)
    {
        PhHeaderNewNormalizeOrder(Context);
        PhHeaderNewLayoutItems(Context);
        PhHeaderNewInvalidate(Context, NULL);
    }
    else
    {
        PhHeaderNewInvalidateItem(Context, Index);
    }

    return TRUE;
}

BOOLEAN PhHeaderNewGetItemInfo(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index,
    _Inout_ HDITEM *Item
    )
{
    PPH_HEADERNEW_ITEM item;

    if (!Item)
        return FALSE;

    if (!(item = PhHeaderNewGetItem(Context, Index)))
        return FALSE;

    if (FlagOn(Item->mask, HDI_WIDTH)) // also HDI_HEIGHT
        Item->cxy = item->Width;

    if (FlagOn(Item->mask, HDI_FORMAT))
        Item->fmt = item->Format;
    if (FlagOn(Item->mask, HDI_LPARAM))
        Item->lParam = item->Param;
    if (FlagOn(Item->mask, HDI_BITMAP))
        Item->hbm = item->Bitmap;
    if (FlagOn(Item->mask, HDI_IMAGE))
        Item->iImage = item->ImageCallback ? I_IMAGECALLBACK : item->ImageIndex;
    if (FlagOn(Item->mask, HDI_ORDER))
        Item->iOrder = item->Order;
    if (FlagOn(Item->mask, HDI_STATE))
        Item->state = item->State;

    // The caller supplies the buffer, exactly as with SysHeader32.

    if (FlagOn(Item->mask, HDI_TEXT) && Item->pszText && Item->cchTextMax > 0)
    {
        if (item->TextCallback)
        {
            Item->pszText = LPSTR_TEXTCALLBACK;
        }
        else if (item->Text)
        {
            PhCopyStringZ(
                item->Text->Buffer,
                item->Text->Length / sizeof(WCHAR),
                Item->pszText,
                Item->cchTextMax,
                NULL
                );
        }
        else
        {
            Item->pszText[0] = UNICODE_NULL;
        }
    }

    return TRUE;
}

BOOLEAN PhHeaderNewDeleteItem(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index
    )
{
    PPH_HEADERNEW_ITEM item;
    ULONG i;
    LONG order;

    if (!(item = PhHeaderNewGetItem(Context, Index)))
        return FALSE;

    order = item->Order;

    PhRemoveItemList(Context->Items, (ULONG)Index);
    PhHeaderNewFreeItem(item);

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_HEADERNEW_ITEM other = Context->Items->Items[i];

        if (other->Order > order)
            other->Order--;
    }

    // Indices shifted, so any cached index is stale.

    if (Context->HotIndex >= Index)
        Context->HotIndex = INT_ERROR;
    if (Context->PressedIndex >= Index)
        Context->PressedIndex = INT_ERROR;
    if (Context->FocusedIndex >= Index)
        Context->FocusedIndex = INT_ERROR;
    if (Context->TrackIndex >= Index)
        Context->TrackIndex = INT_ERROR;

    PhHeaderNewNormalizeOrder(Context);
    PhHeaderNewLayoutItems(Context);
    PhHeaderNewInvalidate(Context, NULL);

    return TRUE;
}

VOID PhHeaderNewDeleteAllItems(
    _In_ PPH_HEADERNEW_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->Items->Count; i++)
        PhHeaderNewFreeItem(Context->Items->Items[i]);

    PhClearList(Context->Items);

    Context->HotIndex = INT_ERROR;
    Context->PressedIndex = INT_ERROR;
    Context->FocusedIndex = INT_ERROR;
    Context->TrackIndex = INT_ERROR;
    Context->DragIndex = INT_ERROR;
    Context->HotDivider = INT_ERROR;
}

// Input

VOID PhHeaderNewSetHotIndex(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Index
    )
{
    LONG oldIndex;

    if (Context->HotIndex == Index)
        return;

    oldIndex = Context->HotIndex;
    Context->HotIndex = Index;

    // Repaint only the two items whose state changed. WM_PAINT refills each
    // item background, so no erase pass is needed.

    PhHeaderNewInvalidateItem(Context, oldIndex);
    PhHeaderNewInvalidateItem(Context, Index);
}

/**
 * Applies a candidate width to the tracked item, giving the parent the chance
 * to veto or clamp it through HDN_ITEMCHANGING.
 */
VOID PhHeaderNewApplyTrackWidth(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG Width,
    _In_ BOOLEAN Final
    )
{
    PPH_HEADERNEW_ITEM item;
    HDITEM changeItem;

    if (!(item = PhHeaderNewGetItem(Context, Context->TrackIndex)))
        return;

    if (Width < 0)
        Width = 0;

    memset(&changeItem, 0, sizeof(HDITEM));
    changeItem.mask = HDI_WIDTH;
    changeItem.cxy = Width;

    // HDN_TRACK is informational; HDN_ITEMCHANGING is the veto point. The
    // handler may rewrite pitem->cxy, and returning TRUE cancels the change.

    if (!Final)
        PhHeaderNewNotifyItem(Context, HDN_TRACK, Context->TrackIndex, &changeItem, 0);

    if (PhHeaderNewNotifyItem(Context, HDN_ITEMCHANGING, Context->TrackIndex, &changeItem, 0))
        return;

    if (item->Width == changeItem.cxy)
        return;

    item->Width = changeItem.cxy;

    PhHeaderNewLayoutItems(Context);
    PhHeaderNewInvalidate(Context, NULL);

    PhHeaderNewNotifyItem(Context, HDN_ITEMCHANGED, Context->TrackIndex, &changeItem, 0);
}

VOID PhHeaderNewEndTrack(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ BOOLEAN Commit
    )
{
    HDITEM changeItem;
    PPH_HEADERNEW_ITEM item;

    if (!Context->Tracking)
        return;

    Context->Tracking = FALSE;

    if (item = PhHeaderNewGetItem(Context, Context->TrackIndex))
    {
        if (!Commit && item->Width != Context->TrackStartWidth)
        {
            item->Width = Context->TrackStartWidth;
            PhHeaderNewLayoutItems(Context);
            PhHeaderNewInvalidate(Context, NULL);
        }

        memset(&changeItem, 0, sizeof(HDITEM));
        changeItem.mask = HDI_WIDTH;
        changeItem.cxy = item->Width;

        PhHeaderNewNotifyItem(Context, HDN_ENDTRACK, Context->TrackIndex, &changeItem, 0);
    }

    Context->TrackIndex = INT_ERROR;
}

VOID PhHeaderNewEndDrag(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ BOOLEAN Commit
    )
{
    LONG fromIndex;
    LONG toOrder;

    Context->DragPending = FALSE;

    if (!Context->Dragging)
        return;

    Context->Dragging = FALSE;

    fromIndex = Context->DragIndex;
    toOrder = Context->DragOrder;

    Context->DragIndex = INT_ERROR;
    Context->DragOrder = INT_ERROR;
    Context->HotDivider = INT_ERROR;

    if (Commit && fromIndex != INT_ERROR && toOrder != INT_ERROR)
    {
        HDITEM dropItem;

        memset(&dropItem, 0, sizeof(HDITEM));
        dropItem.mask = HDI_ORDER;
        dropItem.iOrder = toOrder;

        // Returning TRUE from HDN_ENDDRAG cancels the move. The item is moved
        // before the notification so handlers observe the final arrangement,
        // matching what the common control does.

        PhHeaderNewSetItem(Context, fromIndex, &dropItem);
        PhHeaderNewNotifyItem(Context, HDN_ENDDRAG, fromIndex, &dropItem, 0);
    }
    else
    {
        PhHeaderNewNotifyItem(Context, HDN_ENDDRAG, fromIndex, NULL, 0);
    }

    PhHeaderNewInvalidate(Context, NULL);
}

/**
 * Computes the order the dragged item would land on for a given x position.
 */
LONG PhHeaderNewDragTargetOrder(
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ LONG X
    )
{
    LONG order;

    for (order = 0; order < (LONG)Context->Items->Count; order++)
    {
        PPH_HEADERNEW_ITEM item;
        LONG index;

        if ((index = PhHeaderNewOrderToIndex(Context, order)) == INT_ERROR)
            continue;

        item = Context->Items->Items[index];

        if (X < (item->Rect.left + item->Rect.right) / 2)
            return order;
    }

    return (LONG)Context->Items->Count - 1;
}

LRESULT PhHeaderNewOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ PPH_HEADERNEW_CONTEXT Context,
    _In_ ULONG WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case HDM_GETITEMCOUNT:
        return (LRESULT)Context->Items->Count;
    case HDM_INSERTITEMW:
        return PhHeaderNewInsertItem(Context, (LONG)wParam, (HDITEM *)lParam);
    case HDM_DELETEITEM:
        return PhHeaderNewDeleteItem(Context, (LONG)wParam);
    case HDM_GETITEMW:
        return PhHeaderNewGetItemInfo(Context, (LONG)wParam, (HDITEM *)lParam);
    case HDM_SETITEMW:
        return PhHeaderNewSetItem(Context, (LONG)wParam, (HDITEM *)lParam);

    case HDM_LAYOUT:
        {
            HDLAYOUT *layout = (HDLAYOUT *)lParam;

            if (!layout || !layout->prc || !layout->pwpos)
                return FALSE;

            PhHeaderNewUpdateHeight(Context);

            layout->pwpos->hwnd = WindowHandle;
            layout->pwpos->hwndInsertAfter = NULL;
            layout->pwpos->x = layout->prc->left;
            layout->pwpos->y = layout->prc->top;
            layout->pwpos->cx = layout->prc->right - layout->prc->left;
            layout->pwpos->cy = Context->HeaderHeight;
            layout->pwpos->flags = SWP_NOZORDER |
                (FlagOn(Context->Style, HDS_HIDDEN) ? SWP_HIDEWINDOW : SWP_SHOWWINDOW);
        }
        return TRUE;
    case HDM_HITTEST:
        {
            HDHITTESTINFO *hitTestInfo = (HDHITTESTINFO *)lParam;
            ULONG flags;

            if (!hitTestInfo)
                return INT_ERROR;

            if (Context->NeedsLayout)
                PhHeaderNewLayoutItems(Context);

            hitTestInfo->iItem = PhHeaderNewHitTest(Context, &hitTestInfo->pt, &flags);
            hitTestInfo->flags = flags;

            return hitTestInfo->iItem;
        }
        break;
    case HDM_GETITEMRECT:
        {
            PPH_HEADERNEW_ITEM item;

            if (!lParam)
                return FALSE;

            if (Context->NeedsLayout)
                PhHeaderNewLayoutItems(Context);

            if (!(item = PhHeaderNewGetItem(Context, (LONG)wParam)))
                return FALSE;

            *(RECT *)lParam = item->Rect;
        }
        return TRUE;
    case HDM_SETIMAGELIST:
        {
            HIMAGELIST oldImageList = Context->ImageListHandle;

            Context->ImageListHandle = (HIMAGELIST)lParam;
            PhHeaderNewInvalidate(Context, NULL);

            return (LRESULT)oldImageList;
        }
        break;
    case HDM_GETIMAGELIST:
        return (LRESULT)Context->ImageListHandle;
    case HDM_ORDERTOINDEX:
        return PhHeaderNewOrderToIndex(Context, (LONG)wParam);
    case HDM_GETORDERARRAY:
        {
            PLONG orderArray = (PLONG)lParam;
            LONG count = (LONG)wParam;
            LONG order;

            if (!orderArray || count < (LONG)Context->Items->Count)
                return FALSE;

            for (order = 0; order < (LONG)Context->Items->Count; order++)
                orderArray[order] = PhHeaderNewOrderToIndex(Context, order);
        }
        return TRUE;
    case HDM_SETORDERARRAY:
        {
            PLONG orderArray = (PLONG)lParam;
            LONG count = (LONG)wParam;
            LONG order;

            if (!orderArray || count != (LONG)Context->Items->Count)
                return FALSE;

            for (order = 0; order < count; order++)
            {
                PPH_HEADERNEW_ITEM item;

                if (!(item = PhHeaderNewGetItem(Context, orderArray[order])))
                    return FALSE;

                item->Order = order;
            }

            PhHeaderNewNormalizeOrder(Context);
            PhHeaderNewLayoutItems(Context);
            PhHeaderNewInvalidate(Context, NULL);
        }
        return TRUE;
    case HDM_SETHOTDIVIDER:
        {
            PPH_HEADERNEW_ITEM item;
            LONG divider;

            if (wParam)
            {
                POINT point;
                ULONG flags;

                // wParam TRUE: lParam is a client-relative point.
                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                item = PhHeaderNewGetItem(Context, PhHeaderNewHitTest(Context, &point, &flags));
                divider = item ? item->Order : INT_ERROR;
            }
            else
            {
                // wParam FALSE: lParam is an item index.
                item = PhHeaderNewGetItem(Context, (LONG)lParam);
                divider = item ? item->Order : INT_ERROR;
            }

            if (Context->HotDivider != divider)
            {
                Context->HotDivider = divider;
                PhHeaderNewInvalidate(Context, NULL);
            }

            return divider;
        }
        break;
    case HDM_SETBITMAPMARGIN:
        {
            LONG oldMargin = Context->BitmapMargin;

            Context->BitmapMargin = (LONG)wParam;
            PhHeaderNewInvalidate(Context, NULL);

            return oldMargin;
        }
        break;
    case HDM_GETBITMAPMARGIN:
        return Context->BitmapMargin;
    case HDM_GETFOCUSEDITEM:
        return Context->FocusedIndex;
    case HDM_SETFOCUSEDITEM:
        {
            if (!PhHeaderNewGetItem(Context, (LONG)lParam))
                return FALSE;

            PhHeaderNewInvalidateItem(Context, Context->FocusedIndex);
            Context->FocusedIndex = (LONG)lParam;
            PhHeaderNewInvalidateItem(Context, Context->FocusedIndex);
        }
        return TRUE;
    case HDM_GETITEMDROPDOWNRECT:
        {
            PPH_HEADERNEW_ITEM item;
            RECT rect;

            if (!lParam)
                return FALSE;

            if (!(item = PhHeaderNewGetItem(Context, (LONG)wParam)))
                return FALSE;

            if (!FlagOn(item->Format, HDF_SPLITBUTTON))
                return FALSE;

            rect = item->Rect;
            rect.left = max(rect.left, rect.right - PhScaleToDisplay(13, Context->WindowDpi));
            *(RECT *)lParam = rect;
        }
        return TRUE;
    case HDM_GETOVERFLOWRECT:
        {
            RECT clientRect;
            RECT rect;

            if (!lParam || !FlagOn(Context->Style, HDS_OVERFLOW))
                return FALSE;

            if (!PhGetClientRect(WindowHandle, &clientRect))
                return FALSE;

            rect = clientRect;
            rect.left = max(rect.left, rect.right - PhScaleToDisplay(13, Context->WindowDpi));
            *(RECT *)lParam = rect;
        }
        return TRUE;
    case HDM_CREATEDRAGIMAGE:
        {
            PPH_HEADERNEW_ITEM item;
            HIMAGELIST imageList;
            HBITMAP bitmap;
            HDC hdc;
            HDC bufferDc;
            HBITMAP oldBitmap;
            PH_HEADERNEW_COLORS colors;
            LONG width;
            LONG height;

            if (!(item = PhHeaderNewGetItem(Context, (LONG)wParam)))
                return FALSE;

            width = item->Rect.right - item->Rect.left;
            height = item->Rect.bottom - item->Rect.top;

            if (width <= 0 || height <= 0)
                return FALSE;

            if (!(hdc = GetDC(WindowHandle)))
                return FALSE;

            imageList = NULL;

            if (bufferDc = CreateCompatibleDC(hdc))
            {
                if (bitmap = PhCreateDIBSection(hdc, PHBF_DIB, width, height, NULL))
                {
                    RECT itemRect = item->Rect;

                    oldBitmap = SelectBitmap(bufferDc, bitmap);

                    // Draw the item at the origin of the scratch surface.
                    PhOffsetRect(&item->Rect, -item->Rect.left, -item->Rect.top);
                    PhHeaderNewGetColors(Context, &colors);
                    PhHeaderNewDrawItem(Context, bufferDc, (LONG)wParam, item, &colors);
                    item->Rect = itemRect;

                    SelectBitmap(bufferDc, oldBitmap);

                    if (imageList = ImageList_Create(width, height, ILC_COLOR32, 1, 0))
                    {
                        ImageList_Add(imageList, bitmap, NULL);
                    }

                    DeleteBitmap(bitmap);
                }

                DeleteDC(bufferDc);
            }

            ReleaseDC(WindowHandle, hdc);

            return (LRESULT)imageList;
        }
        break;
    case HDM_SETUNICODEFORMAT:
        {
            BOOLEAN oldFormat = !!Context->UnicodeFormat;

            // The control is unicode-only; the flag is tracked but ANSI is
            // never actually used.
            Context->UnicodeFormat = TRUE;

            return oldFormat;
        }
        break;
    case HDM_GETUNICODEFORMAT:
        return TRUE;

    // Filter bar: not implemented. Return the documented failure value rather
    // than silently pretending the call succeeded.
    case HDM_SETFILTERCHANGETIMEOUT:
    case HDM_EDITFILTER:
    case HDM_CLEARFILTER:
        return 0;
    case PHHM_SETTHEME:
        {
            Context->Theme = (ULONG)wParam;
            PhHeaderNewInvalidate(Context, NULL);
        }
        return TRUE;
    case PHHM_GETTHEME:
        return Context->Theme;
    case PHHM_SETTHEMEDARK:
        {
            Context->ThemeDark = !!wParam;
            PhSetControlTheme(WindowHandle, Context->ThemeDark ? L"DarkMode_ItemsView" : L"Explorer");
            PhHeaderNewInvalidate(Context, NULL);
        }
        return 0;
    case PHHM_GETTHEMEDARK:
        return !!Context->ThemeDark;
    case PHHM_SETHEIGHT:
        {
            Context->CustomHeight = (LONG)wParam;
            PhHeaderNewUpdateHeight(Context);
            PhHeaderNewLayoutItems(Context);
            PhHeaderNewInvalidate(Context, NULL);
        }
        return 0;
    case PHHM_GETHEIGHT:
        return Context->HeaderHeight;
    case PHHM_SETPADDING:
        {
            Context->PaddingX = (LONG)wParam;
            Context->PaddingY = (LONG)lParam;
            PhHeaderNewUpdateHeight(Context);
            PhHeaderNewLayoutItems(Context);
            PhHeaderNewInvalidate(Context, NULL);
        }
        return 0;
    case PHHM_INVALIDATEITEM:
        PhHeaderNewInvalidateItem(Context, (LONG)wParam);
        return 0;
    case PHHM_SETCALLBACK:
        {
            Context->Callback = (PPH_HEADERNEW_MESSAGE_CALLBACK)wParam;
            Context->Context = (PVOID)lParam;
        }
        return 0;
    case PHHM_SETFLAGS:
        {
            ULONG oldFlags = Context->Flags;

            Context->Flags = (ULONG)wParam;
            PhHeaderNewInvalidate(Context, NULL);

            return oldFlags;
        }
        break;
    case PHHM_GETFLAGS:
        return Context->Flags;
    }

    return 0;
}

VOID PhHeaderNewDestroyContext(
    _In_ _Post_invalid_ PPH_HEADERNEW_CONTEXT Context
    )
{
    PhHeaderNewDeleteBuffer(Context);

    if (Context->Items)
    {
        PhHeaderNewDeleteAllItems(Context);
        PhDereferenceObject(Context->Items);
    }

    if (Context->ThemeHandle)
        PhCloseThemeData(Context->ThemeHandle);

    if (Context->FontHandle && Context->OwnFont)
        DeleteFont(Context->FontHandle);

    PhFree(Context);
}

LRESULT CALLBACK PhHeaderNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_HEADERNEW_CONTEXT context;

    context = PhGetWindowContextEx(WindowHandle);

    if (WindowMessage == WM_NCCREATE)
    {
        CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;
        PPH_HEADERNEW_CREATEPARAMS createParameters;

        context = PhAllocateZero(sizeof(PH_HEADERNEW_CONTEXT));
        context->WindowHandle = WindowHandle;
        context->ParentHandle = createStruct->hwndParent;
        context->Style = createStruct->style;
        context->Id = (LONG_PTR)createStruct->hMenu;
        context->Items = PhCreateList(8);
        context->WindowDpi = PhGetWindowDpi(WindowHandle);
        context->Theme = PhHeaderNewThemeUxTheme;
        context->HotIndex = INT_ERROR;
        context->PressedIndex = INT_ERROR;
        context->FocusedIndex = INT_ERROR;
        context->TrackIndex = INT_ERROR;
        context->DragIndex = INT_ERROR;
        context->DragOrder = INT_ERROR;
        context->HotDivider = INT_ERROR;
        context->Redraw = TRUE;
        context->UnicodeFormat = TRUE;

        if (createParameters = createStruct->lpCreateParams)
        {
            if (RTL_CONTAINS_FIELD(createParameters, createParameters->Size, Flags))
                context->Flags = createParameters->Flags;
            if (RTL_CONTAINS_FIELD(createParameters, createParameters->Size, Callback))
                context->Callback = createParameters->Callback;
            if (RTL_CONTAINS_FIELD(createParameters, createParameters->Size, Context))
                context->Context = createParameters->Context;
            if (RTL_CONTAINS_FIELD(createParameters, createParameters->Size, Theme))
                context->Theme = createParameters->Theme;
        }

        PhSetWindowContextEx(WindowHandle, context);
    }

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    if (PhHeaderNewIsUserMessage(WindowMessage))
    {
        return PhHeaderNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
    }

    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            PhHeaderNewUpdateDpiMetrics(context);
            PhHeaderNewUpdateTheme(context);
            PhHeaderNewUpdateFont(context);
            PhHeaderNewUpdateHeight(context);
        }
        return TRUE;
    case WM_CREATE:
        {
            PhSetControlTheme(WindowHandle, context->ThemeDark ? L"DarkMode_ItemsView" : L"Explorer");
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(WindowHandle);
            PhHeaderNewDestroyContext(context);
        }
        return 0;

    case WM_SIZE:
        {
            PhHeaderNewDeleteBuffer(context);
            PhHeaderNewLayoutItems(context);
            PhHeaderNewInvalidate(context, NULL);
        }
        return 0;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC hdc;
            RECT clientRect;
            RECT paintRect;

            PhGetClientRect(WindowHandle, &clientRect);

            if (hdc = BeginPaint(WindowHandle, &paintStruct))
            {
                if (PhIntersectRect(&paintRect, &paintStruct.rcPaint, &clientRect))
                    PhHeaderNewPaint(context, hdc, &paintRect);
                EndPaint(WindowHandle, &paintStruct);
            }
        }
        return 0;
    case WM_PRINTCLIENT:
        {
            RECT clientRect;

            if (PhGetClientRect(WindowHandle, &clientRect))
            {
                PhHeaderNewDraw(context, (HDC)wParam, &clientRect);
            }
        }
        return 0;
    case WM_SETREDRAW:
        {
            context->Redraw = !!wParam;

            if (context->Redraw)
                InvalidateRect(WindowHandle, NULL, FALSE);
        }
        return 0;
    case WM_SETFONT:
        {
            if (context->FontHandle && context->OwnFont)
                DeleteFont(context->FontHandle);

            context->FontHandle = (HFONT)wParam;
            context->OwnFont = FALSE;

            if (!context->FontHandle)
                PhHeaderNewUpdateFont(context);

            PhHeaderNewUpdateHeight(context);
            PhHeaderNewLayoutItems(context);

            if (LOWORD(lParam))
                PhHeaderNewInvalidate(context, NULL);
        }
        return 0;
    case WM_GETFONT:
        return (LRESULT)context->FontHandle;
    case WM_STYLECHANGED:
        {
            STYLESTRUCT *styleStruct = (STYLESTRUCT *)lParam;

            if (wParam == GWL_STYLE && styleStruct)
            {
                context->Style = styleStruct->styleNew;
                PhHeaderNewInvalidate(context, NULL);
            }
        }
        return 0;
    // WM_SETTINGCHANGE is deliberately not handled: it broadcasts for many
    // unrelated settings, and phlib's theme walker sends WM_THEMECHANGED to
    // this class explicitly on a palette switch.
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
        {
            PhHeaderNewUpdateTheme(context);
            PhSetControlTheme(WindowHandle, context->ThemeDark ? L"DarkMode_ItemsView" : L"Explorer");
            PhHeaderNewUpdateHeight(context);
            PhHeaderNewLayoutItems(context);
            PhHeaderNewInvalidate(context, NULL);
        }
        return 0;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(WindowHandle);

            PhHeaderNewUpdateDpiMetrics(context);
            PhHeaderNewUpdateTheme(context);
            PhHeaderNewUpdateFont(context);
            PhHeaderNewUpdateHeight(context);
            PhHeaderNewLayoutItems(context);
            PhHeaderNewInvalidate(context, NULL);
        }
        return 0;
    case WM_NOTIFYFORMAT:
        return NFR_UNICODE;
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;
    case WM_SETFOCUS:
        {
            PhHeaderNewNotify(context, NM_SETFOCUS);
            PhHeaderNewInvalidate(context, NULL);
        }
        return 0;
    case WM_KILLFOCUS:
        {
            PhHeaderNewNotify(context, NM_KILLFOCUS);
            PhHeaderNewInvalidate(context, NULL);
        }
        return 0;
    case WM_SETCURSOR:
        {
            POINT point;
            ULONG flags;

            if (context->Tracking)
            {
                PhSetCursor(PhLoadCursor(NULL, IDC_SIZEWE));
                return TRUE;
            }

            if (GetCursorPos(&point) && ScreenToClient(WindowHandle, &point))
            {
                PhHeaderNewHitTest(context, &point, &flags);

                if (FlagOn(flags, HHT_ONDIVIDER | HHT_ONDIVOPEN))
                {
                    PhSetCursor(PhLoadCursor(NULL, IDC_SIZEWE));
                    return TRUE;
                }
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            POINT point;
            ULONG flags;
            LONG index;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            if (context->Tracking)
            {
                LONG width = context->TrackStartWidth + (point.x - context->TrackStartX);

                PhHeaderNewApplyTrackWidth(context, width, FALSE);
                return 0;
            }

            if (context->DragPending)
            {
                LONG threshold = PhScaleToDisplay(PH_HEADERNEW_DRAG_THRESHOLD, context->WindowDpi);
                LONG delta = point.x - context->DragStartPoint.x;

                if (delta < 0)
                    delta = -delta;

                if (delta > threshold)
                {
                    if (!PhHeaderNewNotifyItem(context, HDN_BEGINDRAG, context->DragIndex, NULL, 0))
                    {
                        context->Dragging = TRUE;
                        context->PressedIndex = INT_ERROR;
                    }

                    context->DragPending = FALSE;
                }
            }

            if (context->Dragging)
            {
                LONG order = PhHeaderNewDragTargetOrder(context, point.x);

                if (context->DragOrder != order)
                {
                    context->DragOrder = order;
                    context->HotDivider = order;
                    PhHeaderNewInvalidate(context, NULL);
                }

                return 0;
            }

            index = PhHeaderNewHitTest(context, &point, &flags);

            if (!FlagOn(flags, HHT_ONHEADER))
                index = INT_ERROR;

            // Keep the hover state active for themed headers even when the
            // caller did not request the legacy HDS_HOTTRACK style.
            PhHeaderNewSetHotIndex(context, index);

            if (!context->MouseActive)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = WindowHandle;
                trackMouseEvent.dwHoverTime = 0;

                if (TrackMouseEvent(&trackMouseEvent))
                    context->MouseActive = TRUE;
            }
        }
        return 0;
    case WM_MOUSELEAVE:
        {
            context->MouseActive = FALSE;
            PhHeaderNewSetHotIndex(context, INT_ERROR);
        }
        return 0;
    case WM_LBUTTONDOWN:
        {
            POINT point;
            ULONG flags;
            LONG index;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            SetFocus(WindowHandle);

            index = PhHeaderNewHitTest(context, &point, &flags);

            if (index == INT_ERROR)
                return 0;

            if (FlagOn(flags, HHT_ONDIVIDER | HHT_ONDIVOPEN))
            {
                PPH_HEADERNEW_ITEM item = PhHeaderNewGetItem(context, index);

                if (!item)
                    return 0;

                if (PhHeaderNewNotifyItem(context, HDN_BEGINTRACK, index, NULL, 0))
                    return 0;

                context->Tracking = TRUE;
                context->TrackIndex = index;
                context->TrackStartX = point.x;
                context->TrackStartWidth = item->Width;
                SetCapture(WindowHandle);
            }
            else if (FlagOn(flags, HHT_ONHEADER))
            {
                context->FocusedIndex = index;

                if (FlagOn(context->Style, HDS_BUTTONS))
                {
                    context->PressedIndex = index;
                    PhHeaderNewInvalidateItem(context, index);
                }

                if (FlagOn(context->Style, HDS_DRAGDROP))
                {
                    context->DragPending = TRUE;
                    context->DragIndex = index;
                    context->DragOrder = INT_ERROR;
                    context->DragStartPoint = point;
                }

                SetCapture(WindowHandle);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        {
            POINT point;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            // The in-flight operation is committed before the capture is
            // released. ReleaseCapture dispatches WM_CAPTURECHANGED
            // synchronously, and that handler cancels whatever is still marked
            // in flight - so it has to find nothing left to cancel.

            if (context->Tracking)
            {
                PhHeaderNewApplyTrackWidth(context, context->TrackStartWidth + (point.x - context->TrackStartX), TRUE);
                PhHeaderNewEndTrack(context, TRUE);
            }
            else if (context->Dragging)
            {
                PhHeaderNewEndDrag(context, TRUE);
            }
            else
            {
                LONG pressedIndex = context->PressedIndex;

                context->DragPending = FALSE;
                context->DragIndex = INT_ERROR;

                if (pressedIndex != INT_ERROR)
                {
                    ULONG flags;
                    LONG index;

                    context->PressedIndex = INT_ERROR;
                    PhHeaderNewInvalidateItem(context, pressedIndex);

                    index = PhHeaderNewHitTest(context, &point, &flags);

                    if (index == pressedIndex && FlagOn(flags, HHT_ONHEADER))
                        PhHeaderNewNotifyItem(context, HDN_ITEMCLICK, index, NULL, 0);
                }
            }

            if (GetCapture() == WindowHandle)
                ReleaseCapture();
        }
        return 0;
    case WM_LBUTTONDBLCLK:
        {
            POINT point;
            ULONG flags;
            LONG index;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            index = PhHeaderNewHitTest(context, &point, &flags);

            if (index == INT_ERROR)
                return 0;

            if (FlagOn(flags, HHT_ONDIVIDER | HHT_ONDIVOPEN))
                PhHeaderNewNotifyItem(context, HDN_DIVIDERDBLCLICK, index, NULL, 0);
            else if (FlagOn(flags, HHT_ONHEADER))
                PhHeaderNewNotifyItem(context, HDN_ITEMDBLCLICK, index, NULL, 0);
        }
        return 0;
    case WM_RBUTTONUP:
        {
            if (!PhHeaderNewNotify(context, NM_RCLICK))
                return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
        }
        return 0;
    case WM_MBUTTONUP:
        {
            POINT point;
            ULONG flags;
            LONG index;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            index = PhHeaderNewHitTest(context, &point, &flags);

            if (index != INT_ERROR && FlagOn(flags, HHT_ONHEADER))
                PhHeaderNewNotifyItem(context, HDN_ITEMCLICK, index, NULL, 2);
        }
        return 0;
    case WM_CAPTURECHANGED:
        {
            // Capture was taken away while an operation was in flight, so
            // abandon it without committing. The normal mouse-up path finishes
            // and clears the operation before releasing the capture, so it
            // never reaches this point with anything pending.

            BOOLEAN cancelled = context->Tracking || context->Dragging;

            if (context->Tracking)
                PhHeaderNewEndTrack(context, FALSE);

            if (context->Dragging)
                PhHeaderNewEndDrag(context, FALSE);

            context->DragPending = FALSE;

            if (context->PressedIndex != INT_ERROR)
            {
                LONG pressedIndex = context->PressedIndex;

                context->PressedIndex = INT_ERROR;
                PhHeaderNewInvalidateItem(context, pressedIndex);
            }

            // Only reported for an abandoned operation. Sending it on every
            // capture release would make listeners that treat it as a
            // drag-completed signal - TreeNew does - fire on plain clicks.

            if (cancelled)
                PhHeaderNewNotify(context, NM_RELEASEDCAPTURE);
        }
        return 0;
    case WM_KEYDOWN:
        {
            switch (wParam)
            {
            case VK_ESCAPE:
                {
                    if (context->Tracking)
                    {
                        if (GetCapture() == WindowHandle)
                            ReleaseCapture();

                        PhHeaderNewEndTrack(context, FALSE);
                        return 0;
                    }

                    if (context->Dragging)
                    {
                        if (GetCapture() == WindowHandle)
                            ReleaseCapture();

                        PhHeaderNewEndDrag(context, FALSE);
                        return 0;
                    }
                }
                break;
            case VK_LEFT:
            case VK_RIGHT:
                {
                    PPH_HEADERNEW_ITEM focusedItem;
                    LONG order = 0;
                    LONG index;

                    if (!context->Items->Count)
                        break;

                    if (focusedItem = PhHeaderNewGetItem(context, context->FocusedIndex))
                        order = focusedItem->Order;

                    order += (wParam == VK_LEFT) ? -1 : 1;

                    if (order < 0)
                        order = 0;
                    if (order >= (LONG)context->Items->Count)
                        order = (LONG)context->Items->Count - 1;

                    if ((index = PhHeaderNewOrderToIndex(context, order)) != INT_ERROR)
                    {
                        PhHeaderNewInvalidateItem(context, context->FocusedIndex);
                        context->FocusedIndex = index;
                        PhHeaderNewInvalidateItem(context, index);
                    }
                }
                return 0;
            case VK_SPACE:
            case VK_RETURN:
                {
                    if (context->FocusedIndex != INT_ERROR)
                        PhHeaderNewNotifyItem(context, HDN_ITEMCLICK, context->FocusedIndex, NULL, 0);
                }
                return 0;
            }
        }
        break;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Sends a message to a PhHeaderNew control, bypassing the window message queue
 * for messages the control owns.
 */
LRESULT PhHeaderNewSendMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    )
{
    if (!WindowHandle)
        return 0;

    if (PhHeaderNewIsUserMessage(WindowMessage))
    {
        PPH_HEADERNEW_CONTEXT context;

        if (context = PhGetWindowContextEx(WindowHandle))
            return PhHeaderNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
    }

    return SendMessage(WindowHandle, WindowMessage, wParam, lParam);
}

/*
 * Process Hacker - 
 *   tree list
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phgui.h>
#include <treelist.h>
#include <treelistp.h>
#include <vsstyle.h>

BOOLEAN PhTreeListInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = 0;
    c.lpfnWndProc = PhpTreeListWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhInstanceHandle;
    c.hIcon = NULL;
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_TREELIST_CLASSNAME;
    c.hIconSm = NULL;

    return !!RegisterClassEx(&c);
}

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        PH_TREELIST_CLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        ParentHandle,
        (HMENU)Id,
        PhInstanceHandle,
        NULL
        );
}

VOID PhpInitializeTreeListContext(
    __out PPHP_TREELIST_CONTEXT Context
    )
{
    Context->Callback = PhTreeListNullCallback;
    Context->Context = NULL;

    Context->MaxId = 0;
    Context->Columns = NULL;
    Context->NumberOfColumns = 0;
    Context->AllocatedColumns = 0;
    Context->List = PhCreateList(64);
    Context->CanAnyExpand = FALSE;

    Context->TriState = FALSE;
    Context->SortColumn = 0;
    Context->SortOrder = AscendingSortOrder;

    Context->EnableStateHighlighting = 0;
    Context->HighlightingDuration = 1000;
    Context->NewColor = RGB(0x00, 0xff, 0x00);
    Context->RemovingColor = RGB(0xff, 0x00, 0x00);

    Context->OldLvWndProc = NULL;

    Context->EnableRedraw = 0;
    Context->Cursor = NULL;
    Context->BrushCache = PhCreateSimpleHashtable(16);
    Context->ThemeData = NULL;
    Context->PlusBitmap = NULL;
    Context->MinusBitmap = NULL;
    Context->IconDc = NULL;
}

VOID PhpDeleteTreeListContext(
    __inout PPHP_TREELIST_CONTEXT Context
    )
{
    PhpClearBrushCache(Context);

    if (Context->Columns)
        PhFree(Context->Columns);

    PhDereferenceObject(Context->List);
    PhDereferenceObject(Context->BrushCache);

    if (Context->ThemeData)
        CloseThemeData_I(Context->ThemeData);
    if (Context->IconDc)
        DeleteDC(Context->IconDc);
}

static BOOLEAN NTAPI PhpColumnHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return ((PPH_TREELIST_COLUMN)Entry1)->Id == ((PPH_TREELIST_COLUMN)Entry2)->Id;
}

static ULONG NTAPI PhpColumnHashtableHashFunction(
    __in PVOID Entry
    )
{
    return ((PPH_TREELIST_COLUMN)Entry)->Id;
}

LRESULT CALLBACK PhpTreeListWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPHP_TREELIST_CONTEXT context;

    context = (PPHP_TREELIST_CONTEXT)GetWindowLongPtr(hwnd, 0);

    if (uMsg == WM_CREATE)
    {
        context = PhAllocate(sizeof(PHP_TREELIST_CONTEXT));
        PhpInitializeTreeListContext(context);
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT createStruct = (LPCREATESTRUCT)lParam;

            context->Handle = hwnd;

            context->ListViewHandle = CreateWindow(
                WC_LISTVIEW,
                L"",
                WS_CHILD | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS |
                WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
                0,
                0,
                createStruct->cx,
                createStruct->cy,
                hwnd,
                (HMENU)PH_TREELIST_LISTVIEW_ID,
                PhInstanceHandle,
                NULL
                );

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");

            // Make sure we get to store item state.
            ListView_SetCallbackMask(
                context->ListViewHandle,
                LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED |
                LVIS_SELECTED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK
                );

            // Hook the list view window procedure.
            context->OldLvWndProc = (WNDPROC)GetWindowLongPtr(context->ListViewHandle, GWLP_WNDPROC);
            SetWindowLongPtr(context->ListViewHandle, GWLP_WNDPROC, (LONG_PTR)PhpTreeListLvHookWndProc);
            SetProp(context->ListViewHandle, L"TreeListContext", (HANDLE)context);

            PhpReloadThemeData(context);

            SendMessage(hwnd, WM_SETFONT, (WPARAM)PhIconTitleFont, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteTreeListContext(context);
            PhFree(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        break;
    case WM_SIZE:
        {
            RECT clientRect;

            GetClientRect(hwnd, &clientRect);
            SetWindowPos(context->ListViewHandle, NULL, 0, 0, clientRect.right, clientRect.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_SETFONT:
        {
            SendMessage(context->ListViewHandle, WM_SETFONT, wParam, lParam);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == context->ListViewHandle)
            {
                switch (hdr->code)
                {
                case LVN_GETDISPINFO:
                    {
                        NMLVDISPINFO *ldi = (NMLVDISPINFO *)hdr;
                        PPH_TREELIST_NODE node;

                        node = context->List->Items[ldi->item.iItem];

                        if (ldi->item.mask & LVIF_STATE)
                        {
                            ldi->item.state = node->s.ViewState;
                        }

                        if (ldi->item.mask & LVIF_TEXT)
                        {
                            PH_STRINGREF text;

                            if (PhpGetNodeText(context, node, ldi->item.iSubItem, &text))
                            {
                                PhCopyUnicodeStringZ(
                                    text.Buffer,
                                    min(text.Length / 2, ldi->item.cchTextMax - 1),
                                    ldi->item.pszText,
                                    ldi->item.cchTextMax,
                                    NULL
                                    );
                            }
                        }
                    }
                    break;
                case LVN_ITEMCHANGED:
                    {
                        NMLISTVIEW *lv = (NMLISTVIEW *)hdr;

                        if (lv->iItem != -1)
                        {
                            PPH_TREELIST_NODE node = context->List->Items[lv->iItem];

                            // The list view has stupid bug where the LVN_ITEMCHANGED 
                            // notification is sent *after* LVN_ODSTATECHANGED and 
                            // with uNewState = 0, deselecting what's meant to be 
                            // selected. Maybe it's by design, but verifying that 
                            // uOldState is correct fixes this problem.
                            if ((lv->uNewState & LVIS_SELECTED) || lv->uOldState == node->s.ViewState)
                                PhpApplyNodeState(node, lv->uNewState);
                        }
                        else
                        {
                            ULONG i;

                            for (i = 0; i < context->List->Count; i++)
                            {
                                PPH_TREELIST_NODE node = context->List->Items[i];

                                PhpApplyNodeState(node, lv->uNewState);
                            }
                        }
                    }
                    break;
                case LVN_ODSTATECHANGED:
                    {
                        NMLVODSTATECHANGE *losc = (NMLVODSTATECHANGE *)hdr;
                        ULONG indexLow;
                        ULONG indexHigh;
                        ULONG i;

                        indexLow = losc->iFrom;
                        indexHigh = losc->iTo;

                        for (i = indexLow; i <= indexHigh; i++)
                        {
                            PPH_TREELIST_NODE node = context->List->Items[i];

                            PhpApplyNodeState(node, losc->uNewState);
                        }
                    }
                    return 0;
                case NM_CUSTOMDRAW:
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)hdr;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;
                        case CDDS_ITEMPREPAINT:
                            {
                                PhpCustomDrawPrePaintItem(context, customDraw);
                            }
                            return CDRF_NOTIFYSUBITEMDRAW;
                        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                            {
                                // We sometimes get useless notifications where the 
                                // rectangle is 0 - just ignore them.
                                if (customDraw->nmcd.rc.right - customDraw->nmcd.rc.left == 0)
                                    return CDRF_SKIPDEFAULT;

                                PhpCustomDrawPrePaintSubItem(context, customDraw);
                            }
                            return CDRF_SKIPDEFAULT;
                        }
                    }
                    break;
                }
            }
        }
        break;
    case WM_THEMECHANGED:
        {
            PhpReloadThemeData(context);
        }
        break;
    case TLM_SETCALLBACK:
        {
            context->Callback = (PPH_TREELIST_CALLBACK)lParam;
        }
        return TRUE;
    case TLM_SETCONTEXT:
        {
            context->Context = (PVOID)lParam;
        }
        return TRUE;
    case TLM_NODESADDED:
        {
            // TODO: Specific optimization for added items
            SendMessage(hwnd, TLM_NODESSTRUCTURED, 0, 0);
        }
        return TRUE;
    case TLM_NODESREMOVED:
        {
            // TODO: Specific optimization for removed items
            SendMessage(hwnd, TLM_NODESSTRUCTURED, 0, 0);
        }
        return TRUE;
    case TLM_NODESSTRUCTURED:
        {
            PPH_TREELIST_NODE *children;
            ULONG numberOfChildren;
            ULONG i;

            if (!PhpGetNodeChildren(context, NULL, &children, &numberOfChildren))
                return FALSE;

            PhClearList(context->List);
            context->CanAnyExpand = FALSE;

            for (i = 0; i < numberOfChildren; i++)
            {
                PhpInsertNodeChildren(context, children[i], 0);
            }

            ListView_SetItemCount(context->ListViewHandle, context->List->Count);
        }
        return TRUE;
    case TLM_ADDCOLUMN:
        {
            PPH_TREELIST_COLUMN column = (PPH_TREELIST_COLUMN)lParam;
            PPH_TREELIST_COLUMN realColumn;

            // Check if a column with the same ID already exists.
            if (column->Id < context->AllocatedColumns && context->Columns[column->Id])
                return FALSE;

            if (context->MaxId < column->Id)
                context->MaxId = column->Id;

            realColumn = PhAllocateCopy(column, sizeof(PH_TREELIST_COLUMN));

            // Boring array management
            if (context->AllocatedColumns < context->MaxId + 1)
            {
                context->AllocatedColumns *= 2;

                if (context->AllocatedColumns < context->MaxId + 1)
                    context->AllocatedColumns = context->MaxId + 1;

                if (context->Columns)
                {
                    ULONG oldAllocatedColumns;
                    
                    oldAllocatedColumns = context->AllocatedColumns;
                    context->Columns = PhReAlloc(
                        context->Columns,
                        context->AllocatedColumns * sizeof(PPH_TREELIST_COLUMN)
                        );

                    // Zero the newly allocated portion.
                    memset(
                        &context->Columns[oldAllocatedColumns],
                        0,
                        (context->AllocatedColumns - oldAllocatedColumns) * sizeof(PPH_TREELIST_COLUMN)
                        );
                }
                else
                {
                    context->Columns = PhAllocate(
                        context->AllocatedColumns * sizeof(PPH_TREELIST_COLUMN)
                        );
                    memset(context->Columns, 0, context->AllocatedColumns * sizeof(PPH_TREELIST_COLUMN));
                }

                context->Columns[context->NumberOfColumns++] = realColumn;
            }

            if (realColumn->Visible)
            {
                realColumn->s.ViewIndex = PhpInsertColumn(context, column);
            }
            else
            {
                realColumn->s.ViewIndex = -1;
            }
        }
        return TRUE;
    case TLM_SETCOLUMN:
        {
            ULONG mask = (ULONG)wParam;
            PPH_TREELIST_COLUMN column = (PPH_TREELIST_COLUMN)lParam;
            PPH_TREELIST_COLUMN realColumn;

            if (
                column->Id >= context->AllocatedColumns ||
                !(realColumn = context->Columns[column->Id])
                )
                return FALSE;

            if (mask & TLCM_VISIBLE)
            {
                if (realColumn->Visible != column->Visible)
                {
                    if (column->Visible)
                    {
                        realColumn->s.ViewIndex = PhpInsertColumn(context, column);
                        // Other attributes already set by insertion.
                        return TRUE;
                    }
                    else
                    {
                        PhpDeleteColumn(context, realColumn);

                        return TRUE;
                    }
                }
            }

            if (mask & (TLCM_TEXT | TLCM_WIDTH | TLCM_ALIGNMENT | TLCM_DISPLAYINDEX))
            {
                LVCOLUMN lvColumn;

                lvColumn.mask = 0;

                if (mask & TLCM_TEXT)
                {
                    lvColumn.mask |= LVCF_TEXT;
                    lvColumn.pszText = realColumn->Text = column->Text;
                }

                if (mask & TLCM_WIDTH)
                {
                    lvColumn.mask |= LVCF_WIDTH;
                    lvColumn.cx = realColumn->Width = column->Width;
                }

                if (mask & TLCM_ALIGNMENT)
                {
                    lvColumn.mask |= LVCF_FMT;
                    lvColumn.fmt = PhToListViewColumnAlign(realColumn->Alignment = column->Alignment);
                }

                if (mask & TLCM_DISPLAYINDEX)
                {
                    lvColumn.mask |= LVCF_ORDER;
                    lvColumn.iOrder = realColumn->DisplayIndex = column->DisplayIndex;
                }

                ListView_SetColumn(context->ListViewHandle, realColumn->s.ViewIndex, &lvColumn);
            }
        }
        return TRUE;
    case TLM_REMOVECOLUMN:
        {
            PPH_TREELIST_COLUMN column = (PPH_TREELIST_COLUMN)lParam;
            PPH_TREELIST_COLUMN realColumn;

            if (
                column->Id >= context->AllocatedColumns ||
                !(realColumn = context->Columns[column->Id])
                )
                return FALSE;

            PhpDeleteColumn(context, realColumn);
            context->Columns[column->Id] = NULL;
        }
        return TRUE;
    case TLM_SETPLUSMINUS:
        {
            context->PlusBitmap = (HBITMAP)wParam;
            context->MinusBitmap = (HBITMAP)lParam;
        }
        return TRUE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpTreeListLvHookWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPHP_TREELIST_CONTEXT context;
    WNDPROC oldWndProc;

    context = (PPHP_TREELIST_CONTEXT)GetProp(hwnd, L"TreeListContext");
    oldWndProc = context->OldLvWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            RemoveProp(hwnd, L"TreeListContext");
        }
        break;
    case WM_SETFOCUS:
        {
            context->HasFocus = TRUE;
        }
        break;
    case WM_KILLFOCUS:
        {
            context->HasFocus = FALSE;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        {
            LONG x = LOWORD(lParam);
            LONG y = HIWORD(lParam);
            LVHITTESTINFO htInfo = { 0 };
            INT itemIndex;
            PPH_TREELIST_NODE node;
            LONG glyphX;

            htInfo.pt.x = x;
            htInfo.pt.y = y;

            if (
                (itemIndex = ListView_HitTest(hwnd, &htInfo)) != -1 &&
                (ULONG)itemIndex < context->List->Count
                )
            {
                // Determine whether the event took place on the 
                // plus/minus glyph.

                node = context->List->Items[itemIndex];
                glyphX = node->Level * 16;

                if (
                    !node->s.IsLeaf &&
                    x >= glyphX &&
                    x < glyphX + 16 + 5 // allow for some extra space
                    )
                {
                    switch (uMsg)
                    {
                    case WM_LBUTTONDOWN:
                        node->Expanded = !node->Expanded;

                        // Let the LV select the item.
                        CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);

                        SendMessage(context->Handle, TLM_NODESSTRUCTURED, 0, 0);

                        return 0;
                    case WM_LBUTTONDBLCLK:
                        return 0;
                    }
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case HDN_ITEMCHANGING:
            case HDN_ITEMCHANGED:
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        LPNMHEADER header2 = (LPNMHEADER)header;
                        LVCOLUMN lvColumn;
                        PPH_TREELIST_COLUMN column;

                        lvColumn.mask = LVCF_SUBITEM;

                        // Update the width.
                        if (ListView_GetColumn(hwnd, header2->iItem, &lvColumn))
                        {
                            column = context->Columns[lvColumn.iSubItem];
                            column->Width = header2->pitem->cxy;
                        }
                    }
                }
                break;
            case HDN_ITEMCLICK:
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        LPNMHEADER header2 = (LPNMHEADER)header;
                        LVCOLUMN lvColumn;

                        if (header2->iItem == context->SortColumn)
                        {
                            if (context->TriState)
                            {
                                if (context->SortOrder == AscendingSortOrder)
                                    context->SortOrder = DescendingSortOrder;
                                else if (context->SortOrder == DescendingSortOrder)
                                    context->SortOrder = NoSortOrder;
                                else
                                    context->SortOrder = AscendingSortOrder;
                            }
                            else
                            {
                                if (context->SortOrder == AscendingSortOrder)
                                    context->SortOrder = DescendingSortOrder;
                                else
                                    context->SortOrder = AscendingSortOrder;
                            }
                        }
                        else
                        {
                            lvColumn.mask = LVCF_SUBITEM;

                            if (ListView_GetColumn(hwnd, header2->iItem, &lvColumn))
                            {
                                context->SortColumn = lvColumn.iSubItem;
                                context->SortOrder = AscendingSortOrder;
                            }
                        }

                        PhSetHeaderSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);
                    }
                }
                break;
            case HDN_ENDDRAG:
            case NM_RELEASEDCAPTURE:
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        PhpRefreshColumns(context);
                    }
                }
                break;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

static VOID PhpCustomDrawPrePaintItem(
    __in PPHP_TREELIST_CONTEXT Context,
    __in LPNMLVCUSTOMDRAW CustomDraw
    )
{
    PPH_TREELIST_NODE node;
    ULONG itemIndex;
    HDC hdc;
    HBRUSH backBrush;
    PPVOID cacheItem;

    itemIndex = (ULONG)CustomDraw->nmcd.dwItemSpec;
    node = Context->List->Items[itemIndex];
    hdc = CustomDraw->nmcd.hdc;

    if (node->State == NormalItemState)
    {
        if (!node->s.CachedColorValid)
        {
            PH_TREELIST_GET_NODE_COLOR getNodeColor;

            getNodeColor.Flags = 0;
            getNodeColor.Node = node;
            getNodeColor.BackColor = RGB(0xff, 0xff, 0xff);
            getNodeColor.ForeColor = RGB(0x00, 0x00, 0x00);

            if (Context->Callback(
                Context->Handle,
                TreeListGetNodeColor,
                &getNodeColor,
                NULL,
                Context->Context
                ))
            {
                node->BackColor = getNodeColor.BackColor;
                node->ForeColor = getNodeColor.ForeColor;
                node->ColorFlags = getNodeColor.Flags & TLGNC_AUTO_FORECOLOR;

                if (getNodeColor.Flags & TLC_CACHE)
                    node->s.CachedColorValid = TRUE;

                node->s.DrawForeColor = node->ForeColor;
            }
            else
            {
                node->BackColor = getNodeColor.BackColor;
                node->ForeColor = getNodeColor.ForeColor;
            }
        }

        if (!node->s.CachedFontValid)
        {
            PH_TREELIST_GET_NODE_FONT getNodeFont;

            getNodeFont.Flags = 0;
            getNodeFont.Node = node;
            getNodeFont.Font = NULL;

            if (Context->Callback(
                Context->Handle,
                TreeListGetNodeFont,
                &getNodeFont,
                NULL,
                Context->Context
                ))
            {
                node->Font = getNodeFont.Font;

                if (getNodeFont.Flags & TLC_CACHE)
                    node->s.CachedFontValid = TRUE;
            }
            else
            {
                node->Font = NULL;
            }
        }

        if (!node->s.CachedIconValid)
        {
            PH_TREELIST_GET_NODE_ICON getNodeIcon;

            getNodeIcon.Flags = 0;
            getNodeIcon.Node = node;
            getNodeIcon.Icon = NULL;

            if (Context->Callback(
                Context->Handle,
                TreeListGetNodeIcon,
                &getNodeIcon,
                NULL,
                Context->Context
                ))
            {
                node->Icon = getNodeIcon.Icon;

                if (getNodeIcon.Flags & TLC_CACHE)
                    node->s.CachedIconValid = TRUE;
            }
            else
            {
                node->Icon = NULL;
            }
        }
    }
    else if (node->State == NewItemState)
    {
        node->s.DrawForeColor = Context->NewColor;
    }
    else if (node->State == RemovingItemState)
    {
        node->s.DrawForeColor = Context->RemovingColor;
    }

    if (node->State != NormalItemState ||
        (node->ColorFlags & TLGNC_AUTO_FORECOLOR))
    {
        if (PhGetColorBrightness(node->BackColor) > 100) // slightly less than half
            node->s.DrawForeColor = RGB(0x00, 0x00, 0x00);
        else
            node->s.DrawForeColor = RGB(0xff, 0xff, 0xff);
    }

    if (
        node->Selected &&
        // Don't draw if the explorer style is active.
        !(Context->ThemeActive && WindowsVersion >= WINDOWS_VISTA)
        )
    {
        if (Context->HasFocus)
        {
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            backBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
        }
        else
        {
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
            backBrush = GetSysColorBrush(COLOR_BTNFACE);
        }
    }
    else
    {
        SetTextColor(hdc, node->ForeColor);

        cacheItem = PhGetSimpleHashtableItem(Context->BrushCache, (PVOID)node->BackColor);

        if (cacheItem)
        {
            backBrush = (HBRUSH)*cacheItem;
        }
        else
        {
            backBrush = CreateSolidBrush(node->BackColor);

            if (backBrush)
            {
                PhAddSimpleHashtableItem(
                    Context->BrushCache,
                    (PVOID)node->BackColor,
                    (PVOID)backBrush
                    );
            }
        }
    }

    FillRect(
        hdc,
        &CustomDraw->nmcd.rc,
        backBrush
        );
}

static VOID PhpCustomDrawPrePaintSubItem(
    __in PPHP_TREELIST_CONTEXT Context,
    __in LPNMLVCUSTOMDRAW CustomDraw
    )
{
    PPH_TREELIST_NODE node;
    ULONG itemIndex;
    ULONG subItemIndex;
    HDC hdc;
    COLORREF backColor;
    COLORREF foreColor;
    HFONT font;
    PH_STRINGREF text;
    PPH_TREELIST_COLUMN column;
    RECT textRect;
    ULONG textFlags;

    itemIndex = (ULONG)CustomDraw->nmcd.dwItemSpec;
    node = Context->List->Items[itemIndex];
    subItemIndex = (ULONG)CustomDraw->iSubItem;
    hdc = CustomDraw->nmcd.hdc;

    backColor = node->BackColor;
    foreColor = node->s.DrawForeColor;
    font = node->Font;
    column = Context->Columns[subItemIndex];
    textRect = CustomDraw->nmcd.rc;
    textFlags = column->TextFlags;

    // Initial margins used by default list view
    textRect.left += 2;
    textRect.top += 2;
    textRect.right -= 2;
    textRect.bottom -= 2;

    if (subItemIndex == 0)
    {
        textRect.left += node->Level * 16;

        if (Context->CanAnyExpand)
        {
            BOOLEAN drewUsingTheme = FALSE;
            RECT themeRect;

            if (!node->s.IsLeaf)
            {
                // Draw the plus/minus glyph.

                themeRect.left = textRect.left;
                themeRect.right = themeRect.left + 16;
                themeRect.top = textRect.top;
                themeRect.bottom = themeRect.top + 16;

                if (Context->ThemeData)
                {
                    if (SUCCEEDED(DrawThemeBackground_I(
                        Context->ThemeData,
                        CustomDraw->nmcd.hdc,
                        TVP_GLYPH,
                        node->Expanded ? GLPS_OPENED : GLPS_CLOSED,
                        &themeRect,
                        NULL
                        )))
                        drewUsingTheme = TRUE;
                }

                if (!drewUsingTheme)
                {
                    if (!Context->IconDc)
                        Context->IconDc = CreateCompatibleDC(CustomDraw->nmcd.hdc);

                    if (node->Expanded)
                        SelectObject(Context->IconDc, Context->MinusBitmap);
                    else
                        SelectObject(Context->IconDc, Context->PlusBitmap);

                    BitBlt(
                        CustomDraw->nmcd.hdc,
                        textRect.left + (16 - 9) / 2, // TODO: un-hardcode the 9
                        textRect.top + (16 - 9) / 2,
                        9,
                        9,
                        Context->IconDc,
                        0,
                        0,
                        SRCCOPY
                        );
                }
            }

            textRect.left += 16;
        }

        // Draw the icon.
        if (node->Icon)
        {
            DrawIconEx(
                CustomDraw->nmcd.hdc,
                textRect.left,
                textRect.top,
                node->Icon,
                16,
                16,
                0,
                NULL,
                DI_NORMAL
                );

            textRect.left += 16 + 4; // 4px margin
        }

        if (textRect.left > textRect.right)
            textRect.left = textRect.right;
    }
    else
    {
        // Margins used by default list view
        textRect.left += 4;
        textRect.right -= 4;
    }

    if (!(textFlags & (DT_PATH_ELLIPSIS | DT_WORD_ELLIPSIS)))
        textFlags |= DT_END_ELLIPSIS;

    textFlags |= DT_VCENTER;

    if (PhpGetNodeText(Context, node, subItemIndex, &text))
    {
        DrawText(
            CustomDraw->nmcd.hdc,
            text.Buffer,
            text.Length / 2,
            &textRect,
            textFlags
            );
    }
}

static BOOLEAN PhpIsNodeLeaf(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node
    )
{
    PH_TREELIST_IS_LEAF isLeaf;

    isLeaf.Flags = 0;
    isLeaf.Node = Node;
    isLeaf.IsLeaf = TRUE;

    if (Context->Callback(
        Context->Handle,
        TreeListIsLeaf,
        &isLeaf,
        NULL,
        Context->Context
        ))
    {
        return isLeaf.IsLeaf;
    }

    // Doesn't matter, decide when we do the get-children callback.
    return FALSE;
}

static BOOLEAN PhpGetNodeChildren(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node,
    __out PPH_TREELIST_NODE **Children,
    __out PULONG NumberOfChildren
    )
{
    PH_TREELIST_GET_CHILDREN getChildren;

    getChildren.Flags = 0;
    getChildren.Node = Node;
    getChildren.Children = NULL;
    getChildren.NumberOfChildren = 0;

    if (Context->Callback(
        Context->Handle,
        TreeListGetChildren,
        &getChildren,
        NULL,
        Context->Context
        ))
    {
        *Children = getChildren.Children;
        *NumberOfChildren = getChildren.NumberOfChildren;

        return TRUE;
    }

    return FALSE;
}

static BOOLEAN PhpGetNodeText(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node,
    __in ULONG Id,
    __out PPH_STRINGREF Text
    )
{
    PH_TREELIST_GET_NODE_TEXT getNodeText;

    if (Id < Node->TextCacheSize && Node->TextCache[Id].Buffer)
    {
        *Text = Node->TextCache[Id];
        return TRUE;
    }

    getNodeText.Flags = 0;
    getNodeText.Node = Node;
    getNodeText.Id = Id;
    PhInitializeEmptyStringRef(&getNodeText.Text);

    if (Context->Callback(
        Context->Handle,
        TreeListGetNodeText,
        &getNodeText,
        NULL,
        Context->Context
        ) && getNodeText.Text.Buffer)
    {
        *Text = getNodeText.Text;

        if ((getNodeText.Flags & TLC_CACHE) && Id < Node->TextCacheSize)
            Node->TextCache[Id] = getNodeText.Text;

        return TRUE;
    }

    return FALSE;
}

static VOID PhpInsertNodeChildren(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node,
    __in ULONG Level
    )
{
    PPH_TREELIST_NODE *children;
    ULONG numberOfChildren;
    ULONG i;

    if (!Node->Visible)
        return;

    Node->Level = Level;

    Node->s.ViewIndex = Context->List->Count;
    PhAddListItem(Context->List, Node);

    if (!(Node->s.IsLeaf = PhpIsNodeLeaf(Context, Node)))
    {
        Context->CanAnyExpand = TRUE;

        if (Node->Expanded)
        {
            if (PhpGetNodeChildren(Context, Node, &children, &numberOfChildren))
            {
                for (i = 0; i < numberOfChildren; i++)
                {
                    PhpInsertNodeChildren(Context, children[i], Level + 1);
                }

                if (numberOfChildren == 0)
                    Node->s.IsLeaf = TRUE;
            }
        }
    }
}

static INT PhpInsertColumn(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_COLUMN Column
    )
{
    LVCOLUMN lvColumn;

    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    lvColumn.fmt = PhToListViewColumnAlign(Column->Alignment);
    lvColumn.cx = Column->Width;
    lvColumn.pszText = Column->Text;
    lvColumn.iSubItem = Column->Id;
    lvColumn.iOrder = Column->DisplayIndex;

    return ListView_InsertColumn(Context->ListViewHandle, MAXINT, &lvColumn);
}

static VOID PhpDeleteColumn(
    __in PPHP_TREELIST_CONTEXT Context,
    __inout PPH_TREELIST_COLUMN Column
    )
{
    ListView_DeleteColumn(Context->ListViewHandle, Column->s.ViewIndex);
    PhpRefreshColumns(Context);
}

static VOID PhpRefreshColumns(
    __in PPHP_TREELIST_CONTEXT Context
    )
{
    ULONG i;
    LVCOLUMN lvColumn;
    PPH_TREELIST_COLUMN column;

    i = 0;
    lvColumn.mask = LVCF_SUBITEM | LVCF_ORDER;

    while (ListView_GetColumn(Context->ListViewHandle, i, &lvColumn))
    {
        column = Context->Columns[lvColumn.iSubItem];
        column->s.ViewIndex = i;
        column->DisplayIndex = lvColumn.iOrder;

        i++;
    }
}

static VOID PhpApplyNodeState(
    __in PPH_TREELIST_NODE Node,
    __in ULONG State
    )
{
    Node->Selected = !!(State & LVIS_SELECTED);
    Node->Focused = !!(State & LVIS_FOCUSED);
    Node->s.ViewState = State;
}

static VOID PhpClearBrushCache(
    __in PPHP_TREELIST_CONTEXT Context
    )
{
    ULONG enumerationKey = 0;
    PPH_KEY_VALUE_PAIR pair;

    while (PhEnumHashtable(Context->BrushCache, &pair, &enumerationKey))
    {
        DeleteObject((HGDIOBJ)pair->Value);
    }

    PhClearHashtable(Context->BrushCache);
}

static VOID PhpReloadThemeData(
    __in PPHP_TREELIST_CONTEXT Context
    )
{
    if (
        IsThemeActive_I &&
        OpenThemeData_I &&
        CloseThemeData_I &&
        DrawThemeBackground_I
        )
    {
        Context->ThemeActive = !!IsThemeActive_I();

        if (Context->ThemeData)
            CloseThemeData_I(Context->ThemeData);

        Context->ThemeData = OpenThemeData_I(Context->Handle, L"TREEVIEW");
    }
    else
    {
        Context->ThemeData = NULL;
        Context->ThemeActive = FALSE;
    }
}

VOID PhInitializeTreeListNode(
    __in PPH_TREELIST_NODE Node
    )
{
    memset(Node, 0, sizeof(PH_TREELIST_NODE));

    Node->Visible = TRUE;
    Node->Expanded = TRUE;

    Node->State = NormalItemState;
}

VOID PhInvalidateTreeListNode(
    __inout PPH_TREELIST_NODE Node,
    __in ULONG Flags
    )
{
    if (Flags & TLIN_COLOR)
        Node->s.CachedColorValid = FALSE;
    if (Flags & TLIN_FONT)
        Node->s.CachedFontValid = FALSE;
    if (Flags & TLIN_ICON)
        Node->s.CachedIconValid = FALSE;
}

BOOLEAN PhAddTreeListColumn(
    __in HWND hwnd,
    __in ULONG Id,
    __in BOOLEAN Visible,
    __in PWSTR Text,
    __in ULONG Width,
    __in ULONG Alignment,
    __in ULONG DisplayIndex,
    __in ULONG TextFlags
    )
{
    PH_TREELIST_COLUMN column;

    column.Id = Id;
    column.Visible = Visible;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.DisplayIndex = DisplayIndex;
    column.TextFlags = TextFlags;

    return !!TreeList_AddColumn(hwnd, &column);
}

__callback BOOLEAN NTAPI PhTreeListNullCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Context
    )
{
    return FALSE;
}

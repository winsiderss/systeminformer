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

/*
 * The tree list is a wrapper around the list view control, in effect 
 * providing a tree view with columns. The sole reason I created this 
 * is because other tree list controls out there are MFC-based!
 *
 * A callback function must be specified, which recieves requests for 
 * data and certain notifications. The tree structure is built by 
 * sending the TreeListGetChildren message recursively, and flattening 
 * the tree into a list which is then displayed by a list view in 
 * virtual mode. The list view is custom drawn due to the indenting 
 * and icon drawing involved. The list view window procedure is also 
 * hooked in order to trap mouse down events on the plus/minus glyph.
 *
 * Currently the entire list is re-created when nodes are added, but 
 * this is obviously not very efficient and will change.
 *
 * PH_STRINGREFs are used in most places to speed up the code, avoiding 
 * unneeded wcslen calls.
 */

#include <phgui.h>
#include <treelist.h>
#include <treelistp.h>
#include <vsstyle.h>

static HIMAGELIST PhpTreeListDummyImageList;

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

    if (!RegisterClassEx(&c))
        return FALSE;

    PhpTreeListDummyImageList = ImageList_Create(16, 16, ILC_COLOR, 1, 1);

    return TRUE;
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

VOID PhpCreateTreeListContext(
    __out PPHP_TREELIST_CONTEXT *Context
    )
{
    PPHP_TREELIST_CONTEXT context;

    context = PhAllocate(sizeof(PHP_TREELIST_CONTEXT));

    context->RefCount = 1;

    context->Callback = PhTreeListNullCallback;
    context->Context = NULL;

    context->MaxId = 0;
    context->Columns = NULL;
    context->NumberOfColumns = 0;
    context->AllocatedColumns = 0;
    context->ColumnsForViewX = NULL;
    context->AllocatedColumnsForViewX = 0;
    context->List = PhCreateList(64);
    context->CanAnyExpand = FALSE;

    context->TriState = FALSE;
    context->SortColumn = 0;
    context->SortOrder = AscendingSortOrder;

    context->OldLvWndProc = NULL;

    context->EnableRedraw = 1;
    context->NeedsRestructure = FALSE;
    context->Cursor = NULL;
    context->ThemeData = NULL;
    context->PlusBitmap = NULL;
    context->MinusBitmap = NULL;
    context->IconDc = NULL;

    *Context = context;
}

VOID PhpReferenceTreeListContext(
    __inout PPHP_TREELIST_CONTEXT Context
    )
{
    Context->RefCount++;
}

VOID PhpDereferenceTreeListContext(
    __inout PPHP_TREELIST_CONTEXT Context
    )
{
    if (--Context->RefCount == 0)
    {
        if (Context->Columns)
            PhFree(Context->Columns);
        if (Context->ColumnsForViewX)
            PhFree(Context->ColumnsForViewX);

        PhDereferenceObject(Context->List);

        if (Context->ThemeData)
            CloseThemeData_I(Context->ThemeData);
        if (Context->IconDc)
            DeleteDC(Context->IconDc);

        PhFree(Context);
    }
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
        PhpCreateTreeListContext(&context);
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
#ifdef PH_TREELIST_ENABLE_EXPLORER_STYLE
            PhSetControlTheme(context->ListViewHandle, L"explorer");
#endif

            // Make sure we get to store item state.
            ListView_SetCallbackMask(
                context->ListViewHandle,
                LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED |
                LVIS_SELECTED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK
                );

            // Hook the list view window procedure.
            context->OldLvWndProc = (WNDPROC)GetWindowLongPtr(context->ListViewHandle, GWLP_WNDPROC);
            SetWindowLongPtr(context->ListViewHandle, GWLP_WNDPROC, (LONG_PTR)PhpTreeListLvHookWndProc);
            PhpReferenceTreeListContext(context);
            SetProp(context->ListViewHandle, L"TreeListContext", (HANDLE)context);

            // Open theme data if available.
            PhpReloadThemeData(context);

            SendMessage(hwnd, WM_SETFONT, (WPARAM)PhIconTitleFont, FALSE);

            // Make sure we have a minimum size of 16 pixels for each row using this hack.
            ListView_SetImageList(context->ListViewHandle, PhpTreeListDummyImageList, LVSIL_SMALL);
        }
        break;
    case WM_DESTROY:
        {
            PhpDereferenceTreeListContext(context);
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
    case WM_THEMECHANGED:
        {
            PhpReloadThemeData(context);
        }
        break;
    case WM_SETCURSOR:
        {
            if (context->Cursor)
            {
                SetCursor(context->Cursor);
                return TRUE;
            }
        }
        break;
    case WM_SETFOCUS:
        {
            SetFocus(context->ListViewHandle);
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
                            ULONG bytesToCopy;

                            if (PhpGetNodeText(context, node, ldi->item.iSubItem, &text))
                            {
                                // Never copy more than cchTextMax - 1 characters.
                                bytesToCopy = min(text.Length, (ldi->item.cchTextMax - 1) * 2);

                                // Copy and null terminate.
                                memcpy(ldi->item.pszText, text.Buffer, bytesToCopy);
                                ldi->item.pszText[bytesToCopy / 2] = 0;
                            }
                        }
                    }
                    break;
                case LVN_ODFINDITEM:
                    {
                        NMLVFINDITEM *fi = (NMLVFINDITEM *)hdr;
                        BOOLEAN stringSearch = FALSE;
                        BOOLEAN partialSearch;
                        BOOLEAN wrapSearch = FALSE;
                        SIZE_T searchLength;
                        INT startIndex;
                        INT currentIndex;
                        INT foundIndex;

                        if (context->List->Count == 0)
                            return -1;

                        // Only string searches are supported.

                        if (fi->lvfi.flags & (LVFI_PARTIAL | LVFI_SUBSTRING))
                        {
                            stringSearch = TRUE;
                            partialSearch = TRUE;
                        }
                        else if (fi->lvfi.flags & LVFI_STRING)
                        {
                            stringSearch = TRUE;
                            partialSearch = FALSE;
                        }

                        if (fi->lvfi.flags & LVFI_WRAP)
                        {
                            if (!stringSearch)
                            {
                                stringSearch = TRUE;
                                partialSearch = FALSE;
                            }

                            wrapSearch = TRUE;
                        }

                        if (stringSearch)
                        {
                            if (partialSearch)
                                searchLength = wcslen(fi->lvfi.psz);

                            startIndex = fi->iStart;
                            currentIndex = startIndex;
                            foundIndex = -1;

                            do
                            {
                                PH_STRINGREF text;

                                if ((ULONG)currentIndex >= context->List->Count)
                                {
                                    if (wrapSearch)
                                        currentIndex = 0;
                                    else
                                        break;
                                }

                                if (PhpGetNodeText(context, context->List->Items[currentIndex], 0, &text))
                                {
                                    if (partialSearch)
                                    {
                                        if (wcsnicmp(text.Buffer, fi->lvfi.psz, searchLength) == 0)
                                        {
                                            foundIndex = currentIndex;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        if (wcsicmp(text.Buffer, fi->lvfi.psz) == 0)
                                        {
                                            foundIndex = currentIndex;
                                            break;
                                        }
                                    }
                                }

                                currentIndex++;
                            } while (currentIndex != startIndex);

                            return foundIndex;
                        }
                    }
                    return -1;
                case LVN_GETINFOTIP:
                    {
                        LPNMLVGETINFOTIP getInfoTip = (LPNMLVGETINFOTIP)hdr;
                        PH_TREELIST_GET_NODE_TOOLTIP getNodeTooltip;
                        ULONG copyLength;

                        getNodeTooltip.Flags = 0;
                        getNodeTooltip.Node = context->List->Items[getInfoTip->iItem];
                        getNodeTooltip.ExistingText = getInfoTip->dwFlags == 0 ? getInfoTip->pszText : NULL;
                        PhInitializeEmptyStringRef(&getNodeTooltip.Text);

                        if (context->Callback(
                            hwnd,
                            TreeListGetNodeTooltip,
                            &getNodeTooltip,
                            NULL,
                            context->Context
                            ) && getNodeTooltip.Text.Buffer)
                        {
                            copyLength = min(
                                (ULONG)getNodeTooltip.Text.Length / 2,
                                (ULONG)getInfoTip->cchTextMax - 1
                                );
                            memcpy(
                                getInfoTip->pszText,
                                getNodeTooltip.Text.Buffer,
                                copyLength * 2
                                );
                            getInfoTip->pszText[copyLength] = 0;
                        }
                    }
                    break;
                case LVN_ITEMCHANGED:
                    {
                        NMLISTVIEW *lv = (NMLISTVIEW *)hdr;

                        if (lv->iItem != -1) // -1 means all items
                        {
                            PPH_TREELIST_NODE node = context->List->Items[lv->iItem];

                            // The list view has stupid bug where the LVN_ITEMCHANGED 
                            // notification is sent *after* LVN_ODSTATECHANGED and 
                            // with uNewState = 0, deselecting what's meant to be 
                            // selected. Maybe it's by design, but verifying that 
                            // uOldState is correct fixes this problem.
                            if (
                                // Bug when trying to select an item when the list view doesn't have focus
                                (lv->uNewState & LVIS_SELECTED) ||
                                // Primary hack
                                lv->uOldState == node->s.ViewState
                                )
                            {
                                // FIXME: There is still a bug. Shift select multiple items, and try to 
                                // deselect the top/bottom item using Ctrl.
                                PhpApplyNodeState(node, lv->uNewState);
                                ListView_Update(context->ListViewHandle, lv->iItem);
                            }
                        }
                        else
                        {
                            ULONG i;

                            for (i = 0; i < context->List->Count; i++)
                            {
                                PPH_TREELIST_NODE node = context->List->Items[i];

                                PhpApplyNodeState(node, lv->uNewState);
                            }

                            // bErase needs to be TRUE, otherwise the item background 
                            // colors don't get refreshed properly (the selection highlight 
                            // on some items may still be active).
                            InvalidateRect(context->ListViewHandle, NULL, TRUE);
                        }

                        context->Callback(hwnd, TreeListSelectionChanged, NULL, NULL, context->Context);
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

                        ListView_RedrawItems(context->ListViewHandle, indexLow, indexHigh);

                        context->Callback(hwnd, TreeListSelectionChanged, NULL, NULL, context->Context);
                    }
                    return 0;
                case LVN_KEYDOWN:
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)hdr;

                        context->Callback(hwnd, TreeListKeyDown, (PVOID)keyDown->wVKey, NULL, context->Context);
                    }
                    break;
                case NM_CUSTOMDRAW:
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)hdr;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;
                        case CDDS_ITEMPREPAINT:
                            {
                                if (PH_TREELIST_NEEDS_RECT_HACK)
                                {
                                    // On XP:
                                    // When the mouse is hovered over an item we get 
                                    // useless notifications where CDDS_ITEMPREPAINT 
                                    // is sent but CDDS_ITEMPREPAINT | CDDS_SUBITEM is 
                                    // only sent for sub item 0. This screws up the 
                                    // whole thing, but we can filter them out by checking 
                                    // the item state.
                                    if (customDraw->nmcd.uItemState == 0)
                                        return CDRF_SKIPDEFAULT;
                                }

                                PhpCustomDrawPrePaintItem(context, customDraw);
                            }
                            return CDRF_NOTIFYSUBITEMDRAW;
                        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                            {
                                if (!PH_TREELIST_NEEDS_RECT_HACK)
                                {
                                    // We sometimes get useless notifications where the 
                                    // rectangle is 0 - just ignore them.
                                    if ((customDraw->nmcd.rc.left | 
                                        customDraw->nmcd.rc.top |
                                        customDraw->nmcd.rc.right |
                                        customDraw->nmcd.rc.bottom) == 0)
                                        return CDRF_SKIPDEFAULT;
                                }

                                PhpCustomDrawPrePaintSubItem(context, customDraw);
                            }
                            return CDRF_SKIPDEFAULT;
                        }
                    }
                    break;
                case NM_CLICK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListNodeLeftClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                case NM_RCLICK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListNodeRightClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                case NM_DBLCLK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListNodeLeftDoubleClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                case NM_RDBLCLK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListNodeRightDoubleClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                }
            }
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
            if (context->EnableRedraw > 0)
            {
                PhpRestructureNodes(context);
            }
            else
            {
                context->NeedsRestructure = TRUE;
                return TRUE;
            }
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
                ULONG oldAllocatedColumns;

                oldAllocatedColumns = context->AllocatedColumns;
                context->AllocatedColumns *= 2;

                if (context->AllocatedColumns < context->MaxId + 1)
                    context->AllocatedColumns = context->MaxId + 1;

                if (context->Columns)
                {
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
            }

            context->Columns[context->NumberOfColumns++] = realColumn;

            if (realColumn->Visible)
            {
                realColumn->s.ViewIndex = PhpInsertColumn(context, column);
            }
            else
            {
                realColumn->s.ViewIndex = -1;
            }

            PhpRefreshColumnsViewX(context);
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
                        column->DisplayIndex = MAXINT;
                        realColumn->s.ViewIndex = PhpInsertColumn(context, column);
                        // Other attributes already set by insertion.
                        PhpRefreshColumnsViewX(context);
                        return TRUE;
                    }
                    else
                    {
                        PhpDeleteColumn(context, realColumn);
                        PhpRefreshColumnsViewX(context);

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
                PhpRefreshColumnsViewX(context);
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
            PhpRefreshColumnsViewX(context);
            context->Columns[column->Id] = NULL;

            context->NumberOfColumns--;
        }
        return TRUE;
    case TLM_SETPLUSMINUS:
        {
            HBITMAP plusBitmap = (HBITMAP)wParam;
            HBITMAP minusBitmap = (HBITMAP)lParam;
            BITMAP bitmap;

            if (GetObject(plusBitmap, sizeof(BITMAP), &bitmap))
            {
                context->PlusBitmapSize.X = bitmap.bmWidth;
                context->PlusBitmapSize.Y = bitmap.bmHeight;
            }
            else
            {
                return FALSE;
            }

            if (GetObject(minusBitmap, sizeof(BITMAP), &bitmap))
            {
                context->MinusBitmapSize.X = bitmap.bmWidth;
                context->MinusBitmapSize.Y = bitmap.bmHeight;
            }
            else
            {
                return FALSE;
            }

            context->PlusBitmap = plusBitmap;
            context->MinusBitmap = minusBitmap;
        }
        return TRUE;
    case TLM_UPDATENODE:
        {
            PPH_TREELIST_NODE node = (PPH_TREELIST_NODE)lParam;

            ListView_Update(context->ListViewHandle, node->s.ViewIndex);
        }
        return TRUE;
    case TLM_SETCURSOR:
        {
            context->Cursor = (HCURSOR)lParam;
        }
        return TRUE;
    case TLM_SETREDRAW:
        {
            if (wParam)
                context->EnableRedraw++;
            else
                context->EnableRedraw--;

            if (context->EnableRedraw == 1)
            {
                if (context->NeedsRestructure)
                {
                    PhpRestructureNodes(context);
                    context->NeedsRestructure = FALSE;
                }

                SendMessage(context->ListViewHandle, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(context->ListViewHandle, NULL, TRUE);
            }
            else if (context->EnableRedraw == 0)
            {
                SendMessage(context->ListViewHandle, WM_SETREDRAW, FALSE, 0);
            }
        }
        return TRUE;
    case TLM_GETSORT:
        {
            PULONG sortColumn = (PULONG)wParam;
            PPH_SORT_ORDER sortOrder = (PPH_SORT_ORDER)lParam;

            if (sortColumn)
                *sortColumn = context->SortColumn;
            if (sortOrder)
                *sortOrder = context->SortOrder;
        }
        break;
    case TLM_SETSORT:
        {
            context->SortColumn = (ULONG)wParam;
            context->SortOrder = (PH_SORT_ORDER)lParam;

            PhSetHeaderSortIcon(
                ListView_GetHeader(context->ListViewHandle),
                context->SortColumn,
                context->SortOrder
                );

            context->Callback(context->Handle, TreeListSortChanged, NULL, NULL, context->Context);
        }
        return TRUE;
    case TLM_SETTRISTATE:
        {
            context->TriState = !!wParam;
        }
        return TRUE;
    case TLM_ENSUREVISIBLE:
        {
            BOOLEAN partialOk = !!wParam;
            PPH_TREELIST_NODE node = (PPH_TREELIST_NODE)lParam;

            ListView_EnsureVisible(context->ListViewHandle, node->s.ViewIndex, partialOk);
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
            PhpDereferenceTreeListContext(context);
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

            // Process mouse events taking place on the plus/minus glyph.

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

                        // A column has been resized, so update our stored width.

                        lvColumn.mask = LVCF_SUBITEM | LVCF_WIDTH;

                        if (ListView_GetColumn(hwnd, header2->iItem, &lvColumn))
                        {
                            column = context->Columns[lvColumn.iSubItem];
                            column->Width = lvColumn.cx;
                        }

                        PhpRefreshColumnsViewX(context);
                    }
                }
                break;
            case HDN_ITEMCLICK:
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        LPNMHEADER header2 = (LPNMHEADER)header;
                        LVCOLUMN lvColumn;

                        // A column has been clicked, so update the sorting 
                        // information.

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

                        context->Callback(context->Handle, TreeListSortChanged, NULL, NULL, context->Context);
                    }
                }
                break;
            case HDN_ENDDRAG:
            case NM_RELEASEDCAPTURE: // necessary for some reason
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        // Columns have been reordered, so refresh our entire column list.

                        PhpRefreshColumns(context);
                        PhpRefreshColumnsViewX(context);
                    }
                }
                break;
            case NM_RCLICK:
                {
                    context->Callback(context->Handle, TreeListHeaderRightClick, NULL, NULL, context->Context);
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
    RECT rowRect;

    itemIndex = (ULONG)CustomDraw->nmcd.dwItemSpec;
    node = Context->List->Items[itemIndex];
    hdc = CustomDraw->nmcd.hdc;
    rowRect = CustomDraw->nmcd.rc;

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
        }
        else
        {
            node->BackColor = getNodeColor.BackColor;
            node->ForeColor = getNodeColor.ForeColor;
        }
    }

    node->s.DrawForeColor = node->ForeColor;

    if (node->UseTempBackColor)
        node->s.DrawBackColor = node->TempBackColor;
    else
        node->s.DrawBackColor = node->BackColor;

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

    if ((node->ColorFlags & TLGNC_AUTO_FORECOLOR) || node->UseTempBackColor)
    {
        if (PhGetColorBrightness(node->s.DrawBackColor) > 100) // slightly less than half
            node->s.DrawForeColor = RGB(0x00, 0x00, 0x00);
        else
            node->s.DrawForeColor = RGB(0xff, 0xff, 0xff);
    }

    if (
        node->Selected &&
#ifdef PH_TREELIST_ENABLE_EXPLORER_STYLE
        // Don't draw if the explorer style is active.
        !(Context->ThemeActive && WindowsVersion >= WINDOWS_VISTA)
#else
        TRUE
#endif
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
        SetTextColor(hdc, node->s.DrawForeColor);
        SetDCBrushColor(hdc, node->s.DrawBackColor);
        backBrush = GetStockObject(DC_BRUSH);
    }

    GetTextMetrics(hdc, &Context->TextMetrics);

    if (PH_TREELIST_NEEDS_RECT_HACK)
    {
        // XP doesn't fill in the nmcd.rc field properly, so we have to use this hack.
        ListView_GetSubItemRect(Context->ListViewHandle, itemIndex, 0, LVIR_BOUNDS, &Context->RowRect);
        rowRect = Context->RowRect;
    }

    FillRect(
        hdc,
        &rowRect,
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
    HFONT font; // font to use
    PH_STRINGREF text; // text to draw
    PPH_TREELIST_COLUMN column; // column of sub item
    RECT origTextRect; // original draw rectangle
    RECT textRect; // working rectangle, modified as needed
    ULONG textFlags; // DT_* flags
    ULONG textVertMargin; // top/bottom margin for text (determined using height of font)
    ULONG iconVertMargin; // top/bottom margin for icons (determined using height 16)

    itemIndex = (ULONG)CustomDraw->nmcd.dwItemSpec;
    node = Context->List->Items[itemIndex];
    subItemIndex = (ULONG)CustomDraw->iSubItem;
    hdc = CustomDraw->nmcd.hdc;

    font = node->Font;
    column = Context->Columns[subItemIndex];
    textFlags = column->TextFlags;

    origTextRect = CustomDraw->nmcd.rc;

    if (PH_TREELIST_NEEDS_RECT_HACK)
    {
        origTextRect.left = Context->RowRect.left + column->s.ViewX; // left may be negative if scrolled horizontally
        origTextRect.top = Context->RowRect.top;
        origTextRect.right = origTextRect.left + column->Width;
        origTextRect.bottom = Context->RowRect.bottom;
    }

    textRect = origTextRect;

    // Initial margins used by default list view
    textRect.left += 2;
    textRect.right -= 2;

    // text margin = (height of row - height of font) / 2
    // icon margin = (height of row - 16) / 2
    textVertMargin = ((textRect.bottom - textRect.top) - Context->TextMetrics.tmHeight) / 2;
    iconVertMargin = ((textRect.bottom - textRect.top) - 16) / 2;

    textRect.top += iconVertMargin;
    textRect.bottom -= iconVertMargin;

    if (column->DisplayIndex == 0)
    {
        textRect.left += node->Level * 16;

        if (Context->CanAnyExpand) // flag is used so we can avoid indenting when it's a flat list
        {
            BOOLEAN drewUsingTheme = FALSE;
            RECT themeRect;
            PH_INTEGER_PAIR glyphSize;

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
                    {
                        SelectObject(Context->IconDc, Context->MinusBitmap);
                        glyphSize = Context->MinusBitmapSize;
                    }
                    else
                    {
                        SelectObject(Context->IconDc, Context->PlusBitmap);
                        glyphSize = Context->PlusBitmapSize;
                    }

                    BitBlt(
                        CustomDraw->nmcd.hdc,
                        textRect.left + (16 - glyphSize.X) / 2,
                        textRect.top + (16 - glyphSize.Y) / 2,
                        glyphSize.X,
                        glyphSize.Y,
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
        textRect.top = origTextRect.top + textVertMargin;
        textRect.bottom = origTextRect.bottom - textVertMargin;

        DrawText(
            CustomDraw->nmcd.hdc,
            text.Buffer,
            text.Length / 2,
            &textRect,
            textFlags
            );
    }
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

static VOID PhpRestructureNodes(
    __in PPHP_TREELIST_CONTEXT Context
    )
{
    PPH_TREELIST_NODE *children;
    ULONG numberOfChildren;
    ULONG i;

    if (!PhpGetNodeChildren(Context, NULL, &children, &numberOfChildren))
        return;

    // At this point we rebuild the entire list.

    PhClearList(Context->List);
    Context->CanAnyExpand = FALSE;

    for (i = 0; i < numberOfChildren; i++)
    {
        PhpInsertNodeChildren(Context, children[i], 0);
    }

    ListView_SetItemCountEx(Context->ListViewHandle, Context->List->Count, LVSICF_NOSCROLL);
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

static VOID PhpRefreshColumnsViewX(
    __in PPHP_TREELIST_CONTEXT Context
    )
{
    ULONG i;
    ULONG x;

    if (Context->AllocatedColumnsForViewX < Context->NumberOfColumns)
    {
        if (Context->ColumnsForViewX)
            PhFree(Context->ColumnsForViewX);

        Context->ColumnsForViewX = PhAllocate(sizeof(PPH_TREELIST_COLUMN) * Context->NumberOfColumns);
        Context->AllocatedColumnsForViewX = Context->NumberOfColumns;
    }

    for (i = 0; i < Context->NumberOfColumns; i++)
    {
        if (Context->Columns[i]->DisplayIndex >= Context->NumberOfColumns)
            PhRaiseStatus(STATUS_INTERNAL_ERROR);

        Context->ColumnsForViewX[Context->Columns[i]->DisplayIndex] = Context->Columns[i];
    }

    x = 0;

    for (i = 0; i < Context->AllocatedColumnsForViewX; i++)
    {
        Context->ColumnsForViewX[i]->s.ViewX = x;
        x += Context->ColumnsForViewX[i]->Width;
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
}

VOID PhInvalidateTreeListNode(
    __inout PPH_TREELIST_NODE Node,
    __in ULONG Flags
    )
{
    if (Flags & TLIN_STATE)
    {
        Node->s.ViewState = 0;

        if (Node->Selected)
            Node->s.ViewState |= LVIS_SELECTED;
        if (Node->Focused)
            Node->s.ViewState |= LVIS_FOCUSED;
    }

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

    memset(&column, 0, sizeof(PH_TREELIST_COLUMN));
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

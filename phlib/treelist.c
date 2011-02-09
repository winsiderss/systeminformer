/*
 * Process Hacker - 
 *   tree list
 * 
 * Copyright (C) 2010-2011 wj32
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
static LONG SmallIconWidth;
static LONG SmallIconHeight;

BOOLEAN PhTreeListInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = CS_GLOBALCLASS;
    c.lpfnWndProc = PhpTreeListWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhLibImageBase;
    c.hIcon = NULL;
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_TREELIST_CLASSNAME;
    c.hIconSm = NULL;

    if (!RegisterClassEx(&c))
        return FALSE;

    SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
    SmallIconHeight = GetSystemMetrics(SM_CYSMICON);

    PhpTreeListDummyImageList = ImageList_Create(SmallIconWidth, SmallIconHeight, ILC_COLOR, 1, 1);

    return TRUE;
}

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return PhCreateTreeListControlEx(ParentHandle, Id, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TLSTYLE_BORDER);
}

HWND PhCreateTreeListControlEx(
    __in HWND ParentHandle,
    __in INT_PTR Id,
    __in ULONG Style
    )
{
    return CreateWindow(
        PH_TREELIST_CLASSNAME,
        L"",
        Style,
        0,
        0,
        3,
        3,
        ParentHandle,
        (HMENU)Id,
        PhLibImageBase,
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

    //context->LvRecursionGuard = 0;

    context->Callback = PhTreeListNullCallback;
    context->Context = NULL;

    context->Columns = NULL;
    context->NextId = 0;
    context->AllocatedColumns = 0;
    context->NumberOfColumns = 0;

    context->ColumnsForViewX = NULL;
    context->AllocatedColumnsForViewX = 0;
    context->ColumnsForDraw = NULL;
    context->AllocatedColumnsForDraw = 0;
    context->List = PhCreateList(64);
    context->CanAnyExpand = FALSE;

    context->TriState = FALSE;
    context->SortColumn = 0;
    context->SortOrder = AscendingSortOrder;

    context->OldLvWndProc = NULL;

    context->EnableRedraw = 1;
    context->NeedsRestructure = FALSE;
    context->Cursor = NULL;
    context->HasFocus = FALSE;

    context->TextMetricsValid = FALSE;
    context->ThemeData = NULL;
    context->ThemeInitialized = FALSE;
    context->EnableExplorerStyle = FALSE;
    context->EnableExplorerGlyphs = FALSE;
    context->LastMouseLocation.x = MINLONG;
    context->LastMouseLocation.y = MINLONG;

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
        ULONG i;

        for (i = 0; i < Context->NextId; i++)
        {
            if (Context->Columns[i])
                PhFree(Context->Columns[i]);
        }

        if (Context->Columns)
            PhFree(Context->Columns);
        if (Context->ColumnsForViewX)
            PhFree(Context->ColumnsForViewX);
        if (Context->ColumnsForDraw)
            PhFree(Context->ColumnsForDraw);

        PhDereferenceObject(Context->List);

        if (Context->ThemeData)
            CloseThemeData_I(Context->ThemeData);

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

static PWSTR PhpMakeTreeListContextAtom()
{
    PH_DEFINE_MAKE_ATOM(L"PhLib_TreeListContext");
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
            ULONG style;
            ULONG exStyle;

            context->Handle = hwnd;

            style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | 
                LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;
            exStyle = 0;

            if (createStruct->style & TLSTYLE_BORDER)
                style |= WS_BORDER;
            if (createStruct->style & TLSTYLE_CLIENTEDGE)
                exStyle |= WS_EX_CLIENTEDGE;

            context->ListViewHandle = CreateWindowEx(
                exStyle,
                WC_LISTVIEW,
                L"",
                style,
                0,
                0,
                createStruct->cx,
                createStruct->cy,
                hwnd,
                (HMENU)PH_TREELIST_LISTVIEW_ID,
                createStruct->hInstance,
                NULL
                );

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            // HACK to fix tooltips showing behind Always On Top windows.
            SetWindowPos(ListView_GetToolTips(context->ListViewHandle), HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

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
            SetProp(context->ListViewHandle, PhpMakeTreeListContextAtom(), (HANDLE)context);

            if (createStruct->style & TLSTYLE_ICONS)
            {
                // Make sure we have a minimum size of 16 pixels (or whatever the small icon height is) for each row using this hack.
                ListView_SetImageList(context->ListViewHandle, PhpTreeListDummyImageList, LVSIL_SMALL);
            }
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
    case WM_GETFONT:
        return SendMessage(context->ListViewHandle, WM_GETFONT, wParam, lParam);
    case WM_SETFONT:
        {
            SendMessage(context->ListViewHandle, WM_SETFONT, wParam, lParam);
            context->TextMetricsValid = FALSE;
        }
        break;
    case WM_THEMECHANGED:
        {
            PhpReloadThemeData(context);
        }
        break;
    case WM_SETTINGCHANGE:
        {
            context->TextMetricsValid = FALSE;
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
                            ULONG id;
                            ULONG bytesToCopy;

                            // iSubItem is actually the column index, not the ID as it should be.
                            id = context->ColumnsForDraw[ldi->item.iSubItem]->Id;

                            if (PhpGetNodeText(context, node, id, &text))
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
                        PH_STRINGREF searchText;
                        INT startIndex;
                        INT currentIndex;
                        INT foundIndex;
                        BOOLEAN firstTime = TRUE;

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
                            PhInitializeStringRef(&searchText, (PWSTR)fi->lvfi.psz);

                            // We do actually need to check the start index.
                            startIndex = (ULONG)fi->iStart < context->List->Count ? fi->iStart : 0;
                            currentIndex = startIndex;
                            foundIndex = -1;

                            while (TRUE)
                            {
                                PH_STRINGREF text;

                                if ((ULONG)currentIndex >= context->List->Count)
                                {
                                    if (wrapSearch)
                                        currentIndex = 0;
                                    else
                                        break;
                                }

                                // We use the firstTime variable instead of a simpler check because 
                                // we want to include the current item in the search. E.g. the 
                                // current item is the only item beginning with "Z". If the user 
                                // searches for "Z", we want to return the current item as being 
                                // found.
                                if (!firstTime && currentIndex == startIndex)
                                    break;

                                if (PhpGetNodeText(context, context->List->Items[currentIndex], 0, &text))
                                {
                                    if (partialSearch)
                                    {
                                        if (PhStartsWithStringRef(&text, &searchText, TRUE))
                                        {
                                            foundIndex = currentIndex;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        if (PhEqualStringRef(&text, &searchText, TRUE))
                                        {
                                            foundIndex = currentIndex;
                                            break;
                                        }
                                    }
                                }

                                currentIndex++;
                                firstTime = FALSE;
                            }

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

                        // The documentation is vague about the meaning of uOldState and 
                        // uNewState. uOldState is actually a mask and uNewState is the value.

                        if (lv->iItem != -1) // -1 means all items
                        {
                            PPH_TREELIST_NODE node = context->List->Items[lv->iItem];

                            node->s.ViewState &= ~lv->uOldState;
                            node->s.ViewState |= lv->uNewState;
                            PhpApplyNodeState(node, node->s.ViewState);
                            ListView_Update(context->ListViewHandle, lv->iItem);
                        }
                        else
                        {
                            ULONG i;

                            for (i = 0; i < context->List->Count; i++)
                            {
                                PPH_TREELIST_NODE node = context->List->Items[i];

                                node->s.ViewState &= ~lv->uOldState;
                                node->s.ViewState |= lv->uNewState;
                                PhpApplyNodeState(node, node->s.ViewState);
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

                            node->s.ViewState &= ~losc->uOldState;
                            node->s.ViewState |= losc->uNewState;
                            PhpApplyNodeState(node, node->s.ViewState);
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

                        // Open the theme data if available. This is delay-loaded.
                        if (!context->ThemeInitialized)
                        {
                            PhpReloadThemeData(context);
                            context->ThemeInitialized = TRUE;
                        }

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;
                        case CDDS_ITEMPREPAINT:
                            {
                                if (PH_TREELIST_USE_HACKAROUNDS)
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
                                if (!PH_TREELIST_USE_HACKAROUNDS)
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
                        context->Callback(hwnd, TreeListLeftClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                case NM_RCLICK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListRightClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                case NM_DBLCLK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListLeftDoubleClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                case NM_RDBLCLK:
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)hdr;
                        PH_TREELIST_MOUSE_EVENT mouseEvent;

                        PhpFillTreeListMouseEvent(&mouseEvent, itemActivate);
                        context->Callback(hwnd, TreeListRightDoubleClick,
                            context->List->Items[itemActivate->iItem], &mouseEvent, context->Context);
                    }
                    break;
                }
            }
        }
        break;
    case TLM_SETCALLBACK:
        {
            if (!lParam)
                lParam = (LPARAM)PhTreeListNullCallback;

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

            if (context->NextId < column->Id + 1)
                context->NextId = column->Id + 1;

            realColumn = PhAllocateCopy(column, sizeof(PH_TREELIST_COLUMN));

            // Boring array management
            if (context->AllocatedColumns < context->NextId)
            {
                PhpExpandAllocatedColumns(context);
            }

            context->Columns[column->Id] = realColumn;
            context->NumberOfColumns++;

            if (realColumn->Visible)
            {
                realColumn->s.ViewIndex = PhpInsertColumn(context, column);
            }
            else
            {
                realColumn->s.ViewIndex = -1;
            }

            PhpRefreshColumnsLookup(context);
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
            context->Columns[realColumn->Id] = NULL;
            PhFree(realColumn);
            PhpRefreshColumnsLookup(context);

            context->NumberOfColumns--;
        }
        return TRUE;
    case TLM_GETCOLUMN:
        {
            PPH_TREELIST_COLUMN column = (PPH_TREELIST_COLUMN)lParam;
            PPH_TREELIST_COLUMN realColumn;

            if (
                column->Id >= context->AllocatedColumns ||
                !(realColumn = context->Columns[column->Id])
                )
                return FALSE;

            memcpy(column, realColumn, sizeof(PH_TREELIST_COLUMN));
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

            if (mask & TLCM_FLAGS)
            {
                realColumn->CustomDraw = column->CustomDraw;

                if (realColumn->Visible != column->Visible)
                {
                    if (column->Visible)
                    {
                        realColumn->DisplayIndex = Header_GetItemCount(ListView_GetHeader(context->ListViewHandle));
                        realColumn->s.ViewIndex = PhpInsertColumn(context, realColumn);
                        // Set other attributes below.
                    }
                    else
                    {
                        PhpDeleteColumn(context, realColumn);
                        PhpRefreshColumnsLookup(context);

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
                PhpRefreshColumnsLookup(context); // display indicies have changed
            }
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
    case TLM_SETSELECTIONMARK:
        {
            ListView_SetSelectionMark(context->ListViewHandle, (INT)wParam);
        }
        return TRUE;
    case TLM_SETSTATEALL:
        {
            ListView_SetItemState(context->ListViewHandle, -1, (UINT)wParam, (UINT)lParam);
        }
        return TRUE;
    case TLM_GETCOLUMNCOUNT:
        return context->NumberOfColumns;
    case TLM_SCROLL:
        {
            ListView_Scroll(context->ListViewHandle, wParam, lParam);
        }
        return TRUE;
    case TLM_SETCOLUMNORDERARRAY:
        {
            ListView_SetColumnOrderArray(context->ListViewHandle, wParam, lParam);
            PhpRefreshColumns(context);
            PhpRefreshColumnsLookup(context);
        }
        return TRUE;
    case TLM_GETNODETEXT:
        {
            PPH_TL_GETNODETEXT getNodeText = (PPH_TL_GETNODETEXT)lParam;

            return PhpGetNodeText(
                context,
                getNodeText->Node,
                getNodeText->Id,
                &getNodeText->Text
                );
        }
        break;
    case TLM_GETVISIBLENODECOUNT:
        return context->List->Count;
    case TLM_GETVISIBLENODE:
        {
            ULONG index = (ULONG)wParam;

            if (index >= context->List->Count)
                return (LRESULT)NULL;

            return (LRESULT)context->List->Items[index];
        }
        break;
    case TLM_GETLISTVIEW:
        return (LRESULT)context->ListViewHandle;
    case TLM_ENABLEEXPLORERSTYLE:
        PhSetControlTheme(context->ListViewHandle, L"explorer");
        context->EnableExplorerStyle = TRUE;
        return TRUE;
    case TLM_GETMAXID:
        return (LRESULT)((LONG_PTR)context->NextId - 1);
    case TLM_SETNODESTATE:
        {
            PPH_TREELIST_NODE node = (PPH_TREELIST_NODE)lParam;
            ULONG state = (ULONG)wParam;
            LVITEM lvItem;

            lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
            lvItem.state = state;

            return CallWindowProc(context->OldLvWndProc, context->ListViewHandle, LVM_SETITEMSTATE, (WPARAM)node->s.ViewIndex, (LPARAM)&lvItem);
        }
    case TLM_SETNODEEXPANDED:
        {
            PPH_TREELIST_NODE node = (PPH_TREELIST_NODE)lParam;
            BOOLEAN expanded = (BOOLEAN)wParam;
            PH_TREELIST_NODE_EVENT nodeEvent;

            if (node->Expanded != expanded)
            {
                memset(&nodeEvent, 0, sizeof(PH_TREELIST_NODE_EVENT));
                context->Callback(context->Handle, TreeListNodePlusMinusChanged,
                    node, &nodeEvent, context->Context);

                if (!nodeEvent.Handled)
                {
                    node->Expanded = expanded;
                    TreeList_NodesStructured(hwnd);
                }
            }
        }
        break;
    case TLM_SETMAXID:
        {
            ULONG maxId = (ULONG)wParam;

            if (context->NextId < maxId + 1)
            {
                context->NextId = maxId + 1;

                if (context->AllocatedColumns < context->NextId)
                {
                    PhpExpandAllocatedColumns(context);
                }
            }
        }
        break;
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

    context = (PPHP_TREELIST_CONTEXT)GetProp(hwnd, PhpMakeTreeListContextAtom());
    oldWndProc = context->OldLvWndProc;

    //if (context->LvRecursionGuard > 0)
    //    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhpDereferenceTreeListContext(context);
            RemoveProp(hwnd, PhpMakeTreeListContextAtom());
        }
        break;
    case WM_SETFOCUS:
        {
            context->HasFocus = TRUE;
            InvalidateRect(context->Handle, NULL, TRUE); // refresh the backgrounds
        }
        break;
    case WM_KILLFOCUS:
        {
            context->HasFocus = FALSE;
            InvalidateRect(context->Handle, NULL, TRUE); // refresh the backgrounds
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (context->EnableExplorerGlyphs)
            {
                LONG oldX;
                LONG newX;
                LONG newY;

                oldX = context->LastMouseLocation.x;
                newX = LOWORD(lParam);
                newY = HIWORD(lParam);

                if (context->NumberOfColumns != 0 && newX < (LONG)context->ColumnsForViewX[0]->Width)
                {
                    LVHITTESTINFO htInfo = { 0 };
                    INT itemIndex;
                    PPH_TREELIST_NODE node;
                    BOOLEAN oldInX;
                    BOOLEAN newInX;

                    // The mouse is in the first column, and may have entered or left the glyph area. 
                    // Redraw the node if necessary.
                    // Note that we don't handle the case where the mouse moves from one item to 
                    // another - this is already handled by the "explorer" style.

                    htInfo.pt.x = newX;
                    htInfo.pt.y = newY;

                    if (
                        (itemIndex = ListView_HitTest(hwnd, &htInfo)) != -1 &&
                        (ULONG)itemIndex < context->List->Count
                        )
                    {
                        node = context->List->Items[itemIndex];

                        oldInX = IS_X_IN_GLYPH(oldX, node->Level);
                        newInX = IS_X_IN_GLYPH(newX, node->Level);

                        if (oldInX != newInX)
                        {
                            ListView_RedrawItems(hwnd, itemIndex, itemIndex);
                        }
                    }
                }

                context->LastMouseLocation.x = newX;
            }
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

                if (
                    !node->s.IsLeaf &&
                    IS_X_IN_GLYPH(x, node->Level) &&
                    // make sure the click resides in the first column; important when it's getting clipped
                    context->NumberOfColumns != 0 &&
                    x < (LONG)context->ColumnsForViewX[0]->Width
                    )
                {
                    switch (uMsg)
                    {
                    case WM_LBUTTONDOWN:
                        {
                            PH_TREELIST_NODE_EVENT nodeEvent;

                            memset(&nodeEvent, 0, sizeof(PH_TREELIST_NODE_EVENT));
                            context->Callback(context->Handle, TreeListNodePlusMinusChanged,
                                node, &nodeEvent, context->Context);

                            if (!nodeEvent.Handled)
                                node->Expanded = !node->Expanded;

                            // Let the LV select the item.
                            CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);

                            if (!nodeEvent.Handled)
                                SendMessage(context->Handle, TLM_NODESSTRUCTURED, 0, 0);
                        }
                        return 0;
                    case WM_LBUTTONDBLCLK:
                        return 0;
                    }
                }
            }
        }
        break;
    case WM_KEYDOWN:
        {
            switch (wParam)
            {
            case VK_LEFT:
            case VK_RIGHT:
                {
                    ULONG selectedIndex;
                    PPH_TREELIST_NODE node;
                    PPH_TREELIST_NODE newNode;

                    if (ListView_GetSelectedCount(hwnd) == 1)
                        selectedIndex = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
                    else
                        selectedIndex = -1;

                    if (selectedIndex != -1 && selectedIndex < context->List->Count)
                    {
                        node = context->List->Items[selectedIndex];

                        switch (wParam)
                        {
                            case VK_LEFT:
                                {
                                    ULONG i;
                                    ULONG targetLevel;

                                    // If the node is expanded, collapse it. Otherwise, select the node's 
                                    // parent.
                                    if (!node->s.IsLeaf && node->Expanded)
                                    {
                                        SendMessage(context->Handle, TLM_SETNODEEXPANDED, (WPARAM)FALSE, (LPARAM)node);
                                    }
                                    else if (node->Level != 0)
                                    {
                                        i = selectedIndex;
                                        targetLevel = node->Level - 1;

                                        while (i != 0)
                                        {
                                            i--;
                                            newNode = context->List->Items[i];

                                            if (newNode->Level == targetLevel)
                                            {
                                                ListView_SetItemState(hwnd, selectedIndex, 0, LVIS_FOCUSED | LVIS_SELECTED);
                                                ListView_SetItemState(hwnd, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                                                break;
                                            }
                                        }
                                    }
                                }
                                break;
                            case VK_RIGHT:
                                {
                                    if (!node->s.IsLeaf)
                                    {
                                        // If the node is collapsed, expand it. Otherwise, select the node's 
                                        // first child.
                                        if (!node->Expanded)
                                        {
                                            SendMessage(context->Handle, TLM_SETNODEEXPANDED, (WPARAM)TRUE, (LPARAM)node);
                                        }
                                        else
                                        {
                                            if (selectedIndex + 1 < context->List->Count)
                                            {
                                                newNode = context->List->Items[selectedIndex + 1];

                                                if (newNode->Level == node->Level + 1)
                                                {
                                                    ListView_SetItemState(hwnd, selectedIndex, 0, LVIS_FOCUSED | LVIS_SELECTED);
                                                    ListView_SetItemState(hwnd, selectedIndex + 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                                                }
                                            }
                                        }
                                    }
                                }
                                break;
                        }
                    }
                }
                break;
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

                        PhpRefreshColumnsLookup(context);
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

                        lvColumn.mask = LVCF_SUBITEM;

                        if (ListView_GetColumn(hwnd, header2->iItem, &lvColumn))
                        {
                            if (lvColumn.iSubItem == context->SortColumn)
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
                                context->SortColumn = lvColumn.iSubItem;
                                context->SortOrder = AscendingSortOrder;
                            }

                            PhSetHeaderSortIcon(ListView_GetHeader(hwnd), header2->iItem, context->SortOrder);

                            context->Callback(context->Handle, TreeListSortChanged, NULL, NULL, context->Context);
                        }
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
                        PhpRefreshColumnsLookup(context);
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
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == hwnd)
            {
                PH_TREELIST_MOUSE_EVENT mouseEvent;
                POINT point;
                LVHITTESTINFO htInfo = { 0 };
                PPH_TREELIST_NODE node = NULL;

                point.x = (SHORT)LOWORD(lParam); // force sign extension
                point.y = (SHORT)HIWORD(lParam);

                if (point.x != -1 || point.y != -1)
                {
                    ScreenToClient(hwnd, &point);

                    htInfo.pt = point;

                    if (ListView_HitTest(hwnd, &htInfo))
                    {
                        mouseEvent.Index = htInfo.iItem;
                        mouseEvent.Id = htInfo.iSubItem;

                        if (mouseEvent.Index < context->List->Count)
                            node = context->List->Items[mouseEvent.Index];
                    }
                    else
                    {
                        mouseEvent.Index = -1;
                        mouseEvent.Id = -1;
                    }

                    mouseEvent.KeyFlags = 0;
                }
                else
                {
                    INT selectedIndex;
                    RECT bounds;

                    // The user pressed a key to display the context menu. 
                    // Suggest where the context menu should display.

                    if (selectedIndex = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED))
                    {
                        mouseEvent.Index = selectedIndex;

                        if (mouseEvent.Index < context->List->Count)
                            node = context->List->Items[mouseEvent.Index];

                        if (ListView_GetItemRect(hwnd, selectedIndex, &bounds, LVIR_BOUNDS))
                        {
                            point.x = bounds.left + SmallIconWidth / 2;
                            point.y = bounds.top + SmallIconHeight / 2;
                        }
                    }

                    mouseEvent.KeyFlags = -1;
                }

                mouseEvent.Location = point;

                if (context->Callback(context->Handle, TreeListContextMenu, node, &mouseEvent, context->Context))
                {
                    return 0;
                }
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
        // Don't draw if the explorer style is active.
        (!Context->EnableExplorerStyle || !(Context->ThemeActive && WindowsVersion >= WINDOWS_VISTA))
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

    if (!Context->TextMetricsValid)
    {
        GetTextMetrics(hdc, &Context->TextMetrics);
        Context->TextMetricsValid = TRUE;
    }

    if (PH_TREELIST_USE_HACKAROUNDS)
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

static VOID PhpDrawPlusMinusGlyph(
    __in HDC hdc,
    __in PRECT Rect,
    __in BOOLEAN Plus
    )
{
    INT savedDc;
    ULONG width;
    ULONG height;
    POINT points[2];

    savedDc = SaveDC(hdc);

    SelectObject(hdc, GetStockObject(DC_PEN));
    SetDCPenColor(hdc, RGB(0x55, 0x55, 0x55));
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));

    width = Rect->right - Rect->left;
    height = Rect->bottom - Rect->top;

    // Draw the rectangle.
    Rectangle(hdc, Rect->left, Rect->top, Rect->right + 1, Rect->bottom + 1);

    SetDCPenColor(hdc, RGB(0x00, 0x00, 0x00));

    // Draw the horizontal line.
    points[0].x = Rect->left + 2;
    points[0].y = Rect->top + height / 2;
    points[1].x = Rect->right - 2 + 1;
    points[1].y = points[0].y;
    Polyline(hdc, points, 2);

    if (Plus)
    {
        // Draw the vertical line.
        points[0].x = Rect->left + width / 2;
        points[0].y = Rect->top + 2;
        points[1].x = points[0].x;
        points[1].y = Rect->bottom - 2 + 1;
        Polyline(hdc, points, 2);
    }

    RestoreDC(hdc, savedDc);
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
    HFONT oldFont;
    PH_STRINGREF text; // text to draw
    PPH_TREELIST_COLUMN column; // column of sub item
    RECT cellRect; // original draw rectangle
    RECT textRect; // working rectangle, modified as needed
    ULONG textFlags; // DT_* flags
    ULONG textVertMargin; // top/bottom margin for text (determined using height of font)
    ULONG iconVertMargin; // top/bottom margin for icons (determined using height of small icon)

    itemIndex = (ULONG)CustomDraw->nmcd.dwItemSpec;
    node = Context->List->Items[itemIndex];
    hdc = CustomDraw->nmcd.hdc;
    //wprintf(L"0x%Ix draw subitem %u %u\n", Context, itemIndex, subItemIndex);

    font = node->Font;
    column = Context->ColumnsForDraw[(ULONG)CustomDraw->iSubItem];
    subItemIndex = column->Id;
    textFlags = column->TextFlags;

    cellRect = CustomDraw->nmcd.rc;

    if (PH_TREELIST_USE_HACKAROUNDS)
    {
        cellRect.left = Context->RowRect.left + column->s.ViewX; // left may be negative if scrolled horizontally
        cellRect.top = Context->RowRect.top;
        cellRect.right = cellRect.left + column->Width;
        cellRect.bottom = Context->RowRect.bottom;
    }

    textRect = cellRect;

    // Initial margins used by default list view
    textRect.left += 2;
    textRect.right -= 2;

    // text margin = (height of row - height of font) / 2
    // icon margin = (height of row - height of small icon) / 2
    textVertMargin = ((textRect.bottom - textRect.top) - Context->TextMetrics.tmHeight) / 2;
    iconVertMargin = ((textRect.bottom - textRect.top) - SmallIconHeight) / 2;

    textRect.top += iconVertMargin;
    textRect.bottom -= iconVertMargin;

    if (column->DisplayIndex == 0)
    {
        BOOLEAN needsClip;
        HRGN oldClipRegion;
        HRGN newClipRegion;

        textRect.left += node->Level * SmallIconWidth;

        needsClip = column->Width <
            (ULONG)(textRect.left + (Context->CanAnyExpand ? SmallIconWidth : 0) + (node->Icon ? SmallIconWidth : 0));

        if (needsClip)
        {
            // HACK
            if (!PH_TREELIST_USE_HACKAROUNDS)
            {
                oldClipRegion = CreateRectRgn(0, 0, 0, 0);

                if (GetClipRgn(hdc, oldClipRegion) != 1)
                {
                    DeleteObject(oldClipRegion);
                    oldClipRegion = NULL;
                }

                // Clip contents to the first column.
                IntersectClipRect(hdc, 0, textRect.top, column->Width, textRect.bottom);
            }
            else
            {
                RECT clipRect;

                clipRect.left = 0;
                clipRect.top = textRect.top;
                clipRect.right = column->Width;
                clipRect.bottom = textRect.bottom;

                // GetClipRgn/SelectClipRgn doesn't restore the old clipping region properly. 
                // Just create a huge clipping region to "restore".
                oldClipRegion = CreateRectRgn(0, 0, 1 << 26, 1 << 26);

                // CreateRectRgn accepts logical coordinates, yet LPtoDP seems to be the only way to 
                // get this working. I don't know :(.
                LPtoDP(hdc, (POINT *)&clipRect, 2);
                newClipRegion = CreateRectRgn(clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);
                SelectClipRgn(hdc, newClipRegion);
                DeleteObject(newClipRegion);
            }
        }

        if (Context->CanAnyExpand) // flag is used so we can avoid indenting when it's a flat list
        {
            BOOLEAN drewUsingTheme = FALSE;
            RECT themeRect;

            if (!node->s.IsLeaf)
            {
                // Draw the plus/minus glyph.

                themeRect.left = textRect.left;
                themeRect.right = themeRect.left + SmallIconWidth;
                themeRect.top = textRect.top;
                themeRect.bottom = themeRect.top + SmallIconHeight;

                if (Context->ThemeData)
                {
                    INT partId;
                    INT stateId;

                    if (Context->EnableExplorerGlyphs && (CustomDraw->nmcd.uItemState & CDIS_HOT))
                    {
                        // Determine if the mouse is actually over the glyph, not just some part of the item.
                        if (IS_X_IN_GLYPH(Context->LastMouseLocation.x, node->Level) &&
                            Context->LastMouseLocation.x < (LONG)column->Width)
                        {
                            partId = TVP_HOTGLYPH;
                        }
                        else
                        {
                            partId = TVP_GLYPH;
                        }
                    }
                    else
                    {
                        partId = TVP_GLYPH;
                    }

                    stateId = node->Expanded ? GLPS_OPENED : GLPS_CLOSED;

                    if (SUCCEEDED(DrawThemeBackground_I(
                        Context->ThemeData,
                        hdc,
                        partId,
                        stateId,
                        &themeRect,
                        NULL
                        )))
                        drewUsingTheme = TRUE;
                }

                if (!drewUsingTheme)
                {
                    ULONG glyphWidth;
                    ULONG glyphHeight;
                    RECT glyphRect;

                    glyphWidth = SmallIconWidth / 2;
                    glyphHeight = SmallIconHeight / 2;

                    glyphRect.left = textRect.left + (SmallIconWidth - glyphWidth) / 2;
                    glyphRect.right = glyphRect.left + glyphWidth;
                    glyphRect.top = textRect.top + (SmallIconHeight - glyphHeight) / 2;
                    glyphRect.bottom = glyphRect.top + glyphHeight;

                    PhpDrawPlusMinusGlyph(hdc, &glyphRect, !node->Expanded);
                }
            }

            textRect.left += SmallIconWidth;
        }

        // Draw the icon.
        if (node->Icon)
        {
            DrawIconEx(
                hdc,
                textRect.left,
                textRect.top,
                node->Icon,
                SmallIconWidth,
                SmallIconHeight,
                0,
                NULL,
                DI_NORMAL
                );

            textRect.left += SmallIconWidth + 4; // 4px margin
        }
        else if (PH_TREELIST_USE_HACKAROUNDS)
        {
            // XP is weird.
            textRect.left += 4;
        }

        if (needsClip && oldClipRegion)
        {
            SelectClipRgn(hdc, oldClipRegion);
            DeleteObject(oldClipRegion);
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

    if (column->CustomDraw)
    {
        BOOLEAN result;
        PH_TREELIST_CUSTOM_DRAW tlCustomDraw;
        INT savedDc;

        tlCustomDraw.Node = node;
        tlCustomDraw.Column = column;
        tlCustomDraw.Dc = hdc;
        tlCustomDraw.CellRect = cellRect;
        tlCustomDraw.TextRect = textRect;

        // Fix up the rectangles so the caller doesn't get confused.
        // Some of the x values may be larger than the y values, for example.
        if (tlCustomDraw.CellRect.right < tlCustomDraw.CellRect.left)
            tlCustomDraw.CellRect.right = tlCustomDraw.CellRect.left;
        if (tlCustomDraw.TextRect.right < tlCustomDraw.TextRect.left)
            tlCustomDraw.TextRect.right = tlCustomDraw.TextRect.left;

        savedDc = SaveDC(hdc);
        result = Context->Callback(Context->Handle, TreeListCustomDraw, &tlCustomDraw, NULL, Context->Context);
        RestoreDC(hdc, savedDc);

        if (result)
            return;
    }

    if (PhpGetNodeText(Context, node, subItemIndex, &text))
    {
        if (!(textFlags & (DT_PATH_ELLIPSIS | DT_WORD_ELLIPSIS)))
            textFlags |= DT_END_ELLIPSIS;

        textFlags |= DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE;

        textRect.top = cellRect.top + textVertMargin;
        textRect.bottom = cellRect.bottom - textVertMargin;

        if (font)
        {
            // Remove the margins we calculated, because they don't actually apply here 
            // since we're using a custom font...
            textRect.top = cellRect.top;
            textRect.bottom = cellRect.bottom;
            oldFont = SelectObject(hdc, font);
        }

        DrawText(
            hdc,
            text.Buffer,
            text.Length / 2,
            &textRect,
            textFlags
            );

        if (font)
            SelectObject(hdc, oldFont);
    }
}

static BOOLEAN PhpGetNodeChildren(
    __in PPHP_TREELIST_CONTEXT Context,
    __in_opt PPH_TREELIST_NODE Node,
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
    ULONG nextLevel;

    if (Node->Visible)
    {
        Node->Level = Level;

        Node->s.ViewIndex = Context->List->Count;
        PhAddItemList(Context->List, Node);

        nextLevel = Level + 1;
    }
    else
    {
        nextLevel = 0; // children of this node should be level 0
    }

    if (!(Node->s.IsLeaf = PhpIsNodeLeaf(Context, Node)))
    {
        Context->CanAnyExpand = TRUE;

        if (Node->Expanded)
        {
            if (PhpGetNodeChildren(Context, Node, &children, &numberOfChildren))
            {
                for (i = 0; i < numberOfChildren; i++)
                {
                    PhpInsertNodeChildren(Context, children[i], nextLevel);
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
    ULONG oldCount;

    if (!PhpGetNodeChildren(Context, NULL, &children, &numberOfChildren))
        return;

    // At this point we rebuild the entire list.

    oldCount = Context->List->Count;

    PhClearList(Context->List);
    Context->CanAnyExpand = FALSE;

    for (i = 0; i < numberOfChildren; i++)
    {
        PhpInsertNodeChildren(Context, children[i], 0);
    }

    // Avoid using ListView_SetItemCountEx if we can.
    if (Context->List->Count != oldCount)
        ListView_SetItemCountEx(Context->ListViewHandle, Context->List->Count, LVSICF_NOSCROLL);
    else
        InvalidateRect(Context->ListViewHandle, NULL, TRUE);
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

    Column->Visible = TRUE;

    return ListView_InsertColumn(Context->ListViewHandle, MAXINT, &lvColumn);
}

static VOID PhpDeleteColumn(
    __in PPHP_TREELIST_CONTEXT Context,
    __inout PPH_TREELIST_COLUMN Column
    )
{
    ListView_DeleteColumn(Context->ListViewHandle, Column->s.ViewIndex);
    Column->Visible = FALSE;
    Column->s.ViewIndex = -1;
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

static VOID PhpExpandAllocatedColumns(
    __in PPHP_TREELIST_CONTEXT Context
    )
{
    if (Context->Columns)
    {
        ULONG oldAllocatedColumns;

        oldAllocatedColumns = Context->AllocatedColumns;
        Context->AllocatedColumns *= 2;

        if (Context->AllocatedColumns < Context->NextId)
            Context->AllocatedColumns = Context->NextId;

        Context->Columns = PhReAllocate(
            Context->Columns,
            Context->AllocatedColumns * sizeof(PPH_TREELIST_COLUMN)
            );

        // Zero the newly allocated portion.
        memset(
            &Context->Columns[oldAllocatedColumns],
            0,
            (Context->AllocatedColumns - oldAllocatedColumns) * sizeof(PPH_TREELIST_COLUMN)
            );
    }
    else
    {
        Context->AllocatedColumns = 16;

        if (Context->AllocatedColumns < Context->NextId)
            Context->AllocatedColumns = Context->NextId;

        Context->Columns = PhAllocate(
            Context->AllocatedColumns * sizeof(PPH_TREELIST_COLUMN)
            );
        memset(Context->Columns, 0, Context->AllocatedColumns * sizeof(PPH_TREELIST_COLUMN));
    }
}

static VOID PhpRefreshColumnsLookup(
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

    if (Context->AllocatedColumnsForDraw < Context->NumberOfColumns)
    {
        if (Context->ColumnsForDraw)
            PhFree(Context->ColumnsForDraw);

        Context->ColumnsForDraw = PhAllocate(sizeof(PPH_TREELIST_COLUMN) * Context->NumberOfColumns);
        Context->AllocatedColumnsForDraw = Context->NumberOfColumns;
    }

    memset(Context->ColumnsForViewX, 0, sizeof(PPH_TREELIST_COLUMN) * Context->AllocatedColumnsForViewX);
    memset(Context->ColumnsForDraw, 0, sizeof(PPH_TREELIST_COLUMN) * Context->AllocatedColumnsForDraw);

    for (i = 0; i < Context->NextId; i++)
    {
        if (!Context->Columns[i])
            continue;

        if (Context->Columns[i]->Visible && Context->Columns[i]->DisplayIndex != -1)
        {
            if (Context->Columns[i]->DisplayIndex >= Context->NumberOfColumns)
                PhRaiseStatus(STATUS_INTERNAL_ERROR);

            Context->ColumnsForViewX[Context->Columns[i]->DisplayIndex] = Context->Columns[i];
        }

        if (Context->Columns[i]->s.ViewIndex != -1)
        {
            if (Context->Columns[i]->s.ViewIndex >= Context->NumberOfColumns)
                PhRaiseStatus(STATUS_INTERNAL_ERROR);

            Context->ColumnsForDraw[Context->Columns[i]->s.ViewIndex] = Context->Columns[i];
        }
    }

    x = 0;

    for (i = 0; i < Context->AllocatedColumnsForViewX; i++)
    {
        if (!Context->ColumnsForViewX[i])
            break;

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
        {
            CloseThemeData_I(Context->ThemeData);
            Context->ThemeData = NULL;
        }

        Context->EnableExplorerGlyphs = FALSE;

        if (Context->EnableExplorerStyle)
        {
            Context->ThemeData = OpenThemeData_I(Context->Handle, L"EXPLORER::TREEVIEW");

            if (Context->ThemeData)
                Context->EnableExplorerGlyphs = TRUE;
        }

        if (!Context->ThemeData)
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
    if (Flags & TLIN_COLOR)
        Node->s.CachedColorValid = FALSE;
    if (Flags & TLIN_FONT)
        Node->s.CachedFontValid = FALSE;
    if (Flags & TLIN_ICON)
        Node->s.CachedIconValid = FALSE;
}

VOID PhInvalidateStateTreeListNode(
    __in HWND hwnd,
    __inout PPH_TREELIST_NODE Node
    )
{
    // This function is required because the LV control is a piece of shit 
    // and has bugs left over from NT4 which still haven't been fixed.

    Node->s.ViewState = 0;

    if (Node->Selected)
        Node->s.ViewState |= LVIS_SELECTED;
    if (Node->Focused)
        Node->s.ViewState |= LVIS_FOCUSED;

    // Update the LV with the real state of the item so it doesn't get 
    // confused. Even though it should be asking us for the state of each 
    // item when it is required, it doesn't because it's retarded.
    TreeList_SetNodeState(hwnd, Node, Node->s.ViewState);
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

BOOLEAN PhLoadTreeListColumnSettings(
    __in HWND TreeListHandle,
    __in PPH_STRING Settings
    )
{
#define ORDER_LIMIT 100
    BOOLEAN result = FALSE;
    ULONG i;
    ULONG count;
    ULONG total;
    ULONG length;
    ULONG indexOfComma;
    ULONG indexOfComma2;
    ULONG indexOfPipe;
    PH_STRINGREF stringRef;
    ULONG64 integer;
    PPH_HASHTABLE columnHashtable;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_KEY_VALUE_PAIR pair;
    INT orderArray[ORDER_LIMIT];
    INT maxOrder;

    // This is one of those functions where you wish you could use C#.

    if (Settings->Length == 0)
        return FALSE;

    columnHashtable = PhCreateSimpleHashtable(20);

    i = 0;
    length = (ULONG)Settings->Length / 2;

    memset(orderArray, 0, sizeof(orderArray));
    maxOrder = 0;

    while (i < length)
    {
        PPH_TREELIST_COLUMN column;
        ULONG id;
        ULONG displayIndex;
        ULONG width;

        indexOfComma = PhFindCharInString(Settings, i, ',');

        if (indexOfComma == -1)
            goto CleanupExit;

        indexOfPipe = PhFindCharInString(Settings, i, '|');

        if (indexOfPipe == -1) // last pair in string
            indexOfPipe = Settings->Length / 2;

        indexOfComma2 = PhFindCharInString(Settings, indexOfComma + 1, ',');

        if (indexOfComma2 == -1 || indexOfComma2 > indexOfPipe)
            goto CleanupExit;

        // Id

        stringRef.Buffer = &Settings->Buffer[i];
        stringRef.Length = (USHORT)((indexOfComma - i) * 2);

        if (!PhStringToInteger64(&stringRef, 10, &integer))
            goto CleanupExit;

        id = (ULONG)integer;

        // Display Index

        stringRef.Buffer = &Settings->Buffer[indexOfComma + 1];
        stringRef.Length = (USHORT)((indexOfComma2 - indexOfComma - 1) * 2);

        if (!PhStringToInteger64(&stringRef, 10, &integer))
            goto CleanupExit;

        displayIndex = (ULONG)integer;

        // Width

        stringRef.Buffer = &Settings->Buffer[indexOfComma2 + 1];
        stringRef.Length = (USHORT)((indexOfPipe - indexOfComma2 - 1) * 2);

        if (!PhStringToInteger64(&stringRef, 10, &integer))
            goto CleanupExit;

        width = (ULONG)integer;

        column = PhAllocate(sizeof(PH_TREELIST_COLUMN));
        column->Id = id;
        column->DisplayIndex = displayIndex;
        column->Width = width;
        PhAddItemSimpleHashtable(columnHashtable, (PVOID)column->Id, column);

        i = indexOfPipe + 1;
    }

    // Set visibility and width.

    i = 0;
    count = 0;
    total = TreeList_GetColumnCount(TreeListHandle);

    while (count < total)
    {
        PH_TREELIST_COLUMN setColumn;
        PPH_TREELIST_COLUMN *columnPtr;

        setColumn.Id = i;

        if (TreeList_GetColumn(TreeListHandle, &setColumn))
        {
            columnPtr = (PPH_TREELIST_COLUMN *)PhFindItemSimpleHashtable(columnHashtable, (PVOID)i);

            if (columnPtr)
            {
                setColumn.Width = (*columnPtr)->Width;
                setColumn.Visible = TRUE;
                TreeList_SetColumn(TreeListHandle, &setColumn, TLCM_FLAGS | TLCM_WIDTH);

                TreeList_GetColumn(TreeListHandle, &setColumn); // get the ViewIndex for use in the second pass
                (*columnPtr)->s.ViewIndex = setColumn.s.ViewIndex;
            }
            else
            {
                setColumn.Visible = FALSE;
                TreeList_SetColumn(TreeListHandle, &setColumn, TLCM_FLAGS);
            }

            count++;
        }

        i++;
    }

    PhBeginEnumHashtable(columnHashtable, &enumContext);

    // Do a second pass to create the order array. This is because the ViewIndex of each column 
    // were unstable in the previous pass since we were both adding and removing columns.
    while (pair = PhNextEnumHashtable(&enumContext))
    {
        PPH_TREELIST_COLUMN column;

        column = pair->Value;

        if (column->DisplayIndex < ORDER_LIMIT)
        {
            orderArray[column->DisplayIndex] = column->s.ViewIndex;

            if ((ULONG)maxOrder < column->DisplayIndex + 1)
                maxOrder = column->DisplayIndex + 1;
        }
    }

    // Set the order array.

    TreeList_SetColumnOrderArray(TreeListHandle, maxOrder, orderArray);

    result = TRUE;

CleanupExit:

    PhBeginEnumHashtable(columnHashtable, &enumContext);

    while (pair = PhNextEnumHashtable(&enumContext))
        PhFree(pair->Value);

    PhDereferenceObject(columnHashtable);

    return result;
}

PPH_STRING PhSaveTreeListColumnSettings(
    __in HWND TreeListHandle
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i = 0;
    ULONG count = 0;
    ULONG total;
    PH_TREELIST_COLUMN column;

    total = TreeList_GetColumnCount(TreeListHandle);

    PhInitializeStringBuilder(&stringBuilder, 20);

    while (count < total)
    {
        column.Id = i;

        if (TreeList_GetColumn(TreeListHandle, &column))
        {
            if (column.Visible)
            {
                PhAppendFormatStringBuilder(
                    &stringBuilder,
                    L"%u,%u,%u|",
                    i,
                    column.DisplayIndex,
                    column.Width
                    );
            }

            count++;
        }

        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

__callback BOOLEAN NTAPI PhTreeListNullCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    return FALSE;
}

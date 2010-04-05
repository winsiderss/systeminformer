#include <phgui.h>
#include <treelist.h>
#include <treelistp.h>

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
    Context->Columns = PhCreateHashtable(
        sizeof(PH_TREELIST_COLUMN),
        PhpColumnHashtableCompareFunction,
        PhpColumnHashtableHashFunction,
        8
        );
    Context->List = PhCreateList(64);

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
}

VOID PhpDeleteTreeListContext(
    __inout PPHP_TREELIST_CONTEXT Context
    )
{
    PhDereferenceObject(Context->Columns);
    PhDereferenceObject(Context->List);
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
                WS_CHILD | LVS_REPORT | LVS_OWNERDATA | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
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
                            PH_TREELIST_GET_NODE_TEXT getNodeText;

                            getNodeText.Flags = 0;
                            getNodeText.Node = node;
                            getNodeText.Id = ldi->item.iSubItem;
                            getNodeText.Text = NULL;

                            if (context->Callback(
                                hwnd,
                                TreeListGetNodeText,
                                &getNodeText,
                                NULL,
                                context->Context
                                ) && getNodeText.Text)
                            {
                                PhCopyUnicodeStringZ(
                                    getNodeText.Text,
                                    ldi->item.cchTextMax - 1,
                                    ldi->item.pszText,
                                    ldi->item.cchTextMax,
                                    NULL
                                    );
                            }
                        }

                        if (ldi->item.mask & LVIF_INDENT)
                        {
                            ldi->item.iIndent = node->Level;
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
                        ULONG itemIndex = (ULONG)customDraw->nmcd.dwItemSpec;
                        ULONG subItemIndex = (ULONG)customDraw->iSubItem;
                        PPH_TREELIST_NODE node;
                        HFONT newFont = NULL;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            if (itemIndex < context->List->Count)
                                return CDRF_NOTIFYITEMDRAW;
                            break;
                        case CDDS_ITEMPREPAINT:
                            {
                                BOOLEAN colorChanged = FALSE;
                                BOOLEAN autoForeColor = FALSE;

                                if (itemIndex >= context->List->Count)
                                    return CDRF_DODEFAULT;

                                node = context->List->Items[itemIndex];

                                if (node->State == NormalItemState)
                                {
                                    if (node->s.CachedColorValid)
                                    {
                                        customDraw->clrTextBk = node->BackColor;
                                        customDraw->clrText = node->ForeColor;
                                        colorChanged = TRUE;
                                        autoForeColor = !!(node->ColorFlags & TLGNC_AUTO_FORECOLOR);
                                    }
                                    else
                                    {
                                        PH_TREELIST_GET_NODE_COLOR getNodeColor;

                                        getNodeColor.Flags = 0;
                                        getNodeColor.Node = node;
                                        getNodeColor.BackColor = RGB(0xff, 0xff, 0xff);
                                        getNodeColor.ForeColor = RGB(0x00, 0x00, 0x00);

                                        if (context->Callback(
                                            hwnd,
                                            TreeListGetNodeColor,
                                            &getNodeColor,
                                            NULL,
                                            context->Context
                                            ))
                                        {
                                            customDraw->clrTextBk = getNodeColor.BackColor;
                                            customDraw->clrText = getNodeColor.ForeColor;

                                            if (getNodeColor.Flags & TLC_CACHE)
                                            {
                                                node->BackColor = getNodeColor.BackColor;
                                                node->ForeColor = getNodeColor.ForeColor;
                                                node->ColorFlags = getNodeColor.Flags & TLGNC_AUTO_FORECOLOR;
                                                node->s.CachedColorValid = TRUE;
                                            }

                                            colorChanged = TRUE;
                                            autoForeColor = !!(getNodeColor.Flags & TLGNC_AUTO_FORECOLOR);
                                        }
                                    }

                                    if (node->s.CachedFontValid)
                                    {
                                        if (node->Font)
                                            SelectObject(customDraw->nmcd.hdc, node->Font);
                                    }
                                    else
                                    {
                                        PH_TREELIST_GET_NODE_FONT getNodeFont;

                                        getNodeFont.Flags = 0;
                                        getNodeFont.Node = node;
                                        getNodeFont.Font = NULL;

                                        if (context->Callback(
                                            hwnd,
                                            TreeListGetNodeFont,
                                            &getNodeFont,
                                            NULL,
                                            context->Context
                                            ))
                                        {
                                            if (getNodeFont.Flags & TLC_CACHE)
                                            {
                                                node->Font = getNodeFont.Font;
                                                node->s.CachedFontValid = TRUE;
                                            }

                                            newFont = getNodeFont.Font;

                                            if (getNodeFont.Font)
                                                SelectObject(customDraw->nmcd.hdc, getNodeFont.Font);
                                        }
                                    }
                                }
                                else if (node->State == NewItemState)
                                {
                                    customDraw->clrTextBk = context->NewColor;
                                    colorChanged = TRUE;
                                    autoForeColor = TRUE;
                                }
                                else if (node->State == RemovingItemState)
                                {
                                    customDraw->clrTextBk = context->RemovingColor;
                                    colorChanged = TRUE;
                                    autoForeColor = TRUE;
                                }

                                if (colorChanged)
                                {
                                    if (PhGetColorBrightness(customDraw->clrTextBk) > 100) // slightly less than half
                                        customDraw->clrText = RGB(0x00, 0x00, 0x00);
                                    else
                                        customDraw->clrText = RGB(0xff, 0xff, 0xff);
                                }

                                if (!newFont)
                                    return CDRF_DODEFAULT;
                                else
                                    return CDRF_NEWFONT;
                            }
                            break;
                        }
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
            PPH_TREELIST_NODE *children;
            ULONG numberOfChildren;
            ULONG i;

            if (!PhpGetNodeChildren(context, NULL, &children, &numberOfChildren))
                return FALSE;

            PhClearList(context->List);

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

            if (context->MaxId < column->Id)
                context->MaxId = column->Id;

            realColumn = PhAddHashtableEntry(context->Columns, column);

            if (!realColumn)
                return FALSE;

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

            realColumn = PhGetHashtableEntry(context->Columns, column);

            if (!realColumn)
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

            realColumn = PhGetHashtableEntry(context->Columns, column);

            if (!realColumn)
                return FALSE;

            PhpDeleteColumn(context, realColumn);
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case HDN_ITEMCLICK:
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        LPNMHEADER header2 = (LPNMHEADER)header;

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
                            context->SortColumn = header2->iItem;
                            context->SortOrder = AscendingSortOrder;
                        }

                        PhSetHeaderSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);
                    }
                }
                break;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
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

static VOID PhpInsertNodeChildren(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node,
    __in ULONG Level
    )
{
    PPH_TREELIST_NODE *children;
    ULONG numberOfChildren;
    ULONG i;

    Node->Level = Level;

    PhAddListItem(Context->List, Node);

    if (Node->Expanded)
    {
        if (PhpGetNodeChildren(Context, Node, &children, &numberOfChildren))
        {
            for (i = 0; i < numberOfChildren; i++)
            {
                PhpInsertNodeChildren(Context, children[i], Level + 1);
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
    ULONG i;
    LVCOLUMN lvColumn;
    PH_TREELIST_COLUMN lookupColumn;
    PPH_TREELIST_COLUMN column;

    lvColumn.mask = LVCF_WIDTH | LVCF_ORDER;

    // Get the column's attributes and save them in case we need to re-add the column 
    // later.
    if (ListView_GetColumn(Context->ListViewHandle, Column->s.ViewIndex, &lvColumn))
    {
        Column->Width = lvColumn.cx;
        Column->DisplayIndex = lvColumn.iOrder;
    }

    ListView_DeleteColumn(Context->ListViewHandle, Column->s.ViewIndex);

    // Refresh indicies, since they have changed.

    i = Column->s.ViewIndex;
    lvColumn.mask = LVCF_SUBITEM;

    while (ListView_GetColumn(Context->ListViewHandle, i, &lvColumn))
    {
        lookupColumn.Id = lvColumn.iSubItem;

        if (column = PhGetHashtableEntry(Context->Columns, &lookupColumn))
        {
            column->s.ViewIndex = i;
        }

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

VOID PhInitializeTreeListNode(
    __in PPH_TREELIST_NODE Node
    )
{
    memset(Node, 0, sizeof(PH_TREELIST_NODE));

    Node->Visible = TRUE;
    Node->Expanded = TRUE;

    Node->State = NormalItemState;
}

BOOLEAN PhAddTreeListColumn(
    __in HWND hwnd,
    __in ULONG Id,
    __in BOOLEAN Visible,
    __in PWSTR Text,
    __in ULONG Width,
    __in ULONG Alignment,
    __in ULONG DisplayIndex
    )
{
    PH_TREELIST_COLUMN column;

    column.Id = Id;
    column.Visible = Visible;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.DisplayIndex = DisplayIndex;

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

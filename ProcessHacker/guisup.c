#include <phgui.h>

HWND PhCreateListViewControl(
    HWND ParentHandle,
    INT_PTR Id
    )
{
    return CreateWindow(
        WC_LISTVIEW,
        L"",
        WS_CHILD | LVS_REPORT | WS_VISIBLE | WS_BORDER,
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

INT PhAddListViewColumn(
    HWND ListViewHandle,
    INT Index,
    INT DisplayIndex,
    INT SubItemIndex,
    INT Format,
    INT Width,
    PWSTR Text
    )
{
    LVCOLUMN column;

    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    column.fmt = Format;
    column.cx = Width;
    column.pszText = Text;
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    return ListView_InsertColumn(ListViewHandle, Index, &column);
}

INT PhAddListViewItem(
    HWND ListViewHandle,
    INT Index,
    PWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = Text;

    return ListView_InsertItem(ListViewHandle, &item);
}

HWND PhCreateTabControl(
    HWND ParentHandle
    )
{
    HWND tabControlHandle;

    tabControlHandle = CreateWindow(
        WC_TABCONTROL,
        L"",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        0,
        0,
        3,
        3,
        ParentHandle,
        NULL,
        PhInstanceHandle,
        NULL
        );

    if (tabControlHandle)
    {
        SendMessage(tabControlHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);
    }

    return tabControlHandle;
}

INT PhAddTabControlTab(
    HWND TabControlHandle,
    INT Index,
    PWSTR Text
    )
{
    TCITEM item;

    item.mask = TCIF_TEXT;
    item.pszText = Text;

    return TabCtrl_InsertItem(TabControlHandle, Index, &item);
}

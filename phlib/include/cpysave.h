#ifndef _PH_CPYSAVE_H
#define _PH_CPYSAVE_H

#define PH_EXPORT_MODE_TABS 0
#define PH_EXPORT_MODE_SPACES 1
#define PH_EXPORT_MODE_CSV 2

VOID PhaCreateTextTable(
    _Out_ PPH_STRING ***Table,
    _In_ ULONG Rows,
    _In_ ULONG Columns
    );

PPH_LIST PhaFormatTextTable(
    _In_ PPH_STRING **Table,
    _In_ ULONG Rows,
    _In_ ULONG Columns,
    _In_ ULONG Mode
    );

VOID PhMapDisplayIndexTreeNew(
    _In_ HWND TreeNewHandle,
    _Out_opt_ PULONG *DisplayToId,
    _Out_opt_ PWSTR **DisplayToText,
    _Out_ PULONG NumberOfColumns
    );

PHLIBAPI
PPH_STRING PhGetTreeNewText(
    _In_ HWND TreeNewHandle,
    _Reserved_ ULONG Reserved
    );

PHLIBAPI
PPH_LIST PhGetGenericTreeNewLines(
    _In_ HWND TreeNewHandle,
    _In_ ULONG Mode
    );

VOID PhaMapDisplayIndexListView(
    _In_ HWND ListViewHandle,
    _Out_writes_(Count) PULONG DisplayToId,
    _Out_writes_opt_(Count) PPH_STRING *DisplayToText,
    _In_ ULONG Count,
    _Out_ PULONG NumberOfColumns
    );

PPH_STRING PhaGetListViewItemText(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT SubItemIndex
    );

PPH_STRING PhGetListViewText(
    _In_ HWND ListViewHandle
    );

PPH_LIST PhGetListViewLines(
    _In_ HWND ListViewHandle,
    _In_ ULONG Mode
    );

#endif

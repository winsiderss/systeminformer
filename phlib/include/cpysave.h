#ifndef _PH_CPYSAVE_H
#define _PH_CPYSAVE_H

#define PH_EXPORT_MODE_TABS 0
#define PH_EXPORT_MODE_SPACES 1
#define PH_EXPORT_MODE_CSV 2

VOID PhaCreateTextTable(
    __out PPH_STRING ***Table,
    __in ULONG Rows,
    __in ULONG Columns
    );

PPH_LIST PhaFormatTextTable(
    __in PPH_STRING **Table,
    __in ULONG Rows,
    __in ULONG Columns,
    __in ULONG Mode
    );

VOID PhMapDisplayIndexTreeList(
    __in HWND TreeListHandle,
    __in ULONG MaximumNumberOfColumns,
    __out_ecount(MaximumNumberOfColumns) PULONG DisplayToId,
    __out_ecount_opt(MaximumNumberOfColumns) PWSTR *DisplayToText,
    __out PULONG NumberOfColumns
    );

VOID PhMapDisplayIndexTreeNew(
    __in HWND TreeNewHandle,
    __in ULONG MaximumNumberOfColumns,
    __out_ecount(MaximumNumberOfColumns) PULONG DisplayToId,
    __out_ecount_opt(MaximumNumberOfColumns) PWSTR *DisplayToText,
    __out PULONG NumberOfColumns
    );

PHLIBAPI
PPH_FULL_STRING PhGetTreeListText(
    __in HWND TreeListHandle,
    __in ULONG MaximumNumberOfColumns
    );

PHLIBAPI
PPH_FULL_STRING PhGetTreeNewText(
    __in HWND TreeNewHandle,
    __in ULONG MaximumNumberOfColumns
    );

PPH_LIST PhGetGenericTreeListLines(
    __in HWND TreeListHandle,
    __in ULONG Mode
    );

PPH_LIST PhGetGenericTreeNewLines(
    __in HWND TreeNewHandle,
    __in ULONG Mode
    );

VOID PhaMapDisplayIndexListView(
    __in HWND ListViewHandle,
    __out_ecount(Count) PULONG DisplayToId,
    __out_ecount_opt(Count) PPH_STRING *DisplayToText,
    __in ULONG Count,
    __out PULONG NumberOfColumns
    );

PPH_FULL_STRING PhGetListViewText(
    __in HWND ListViewHandle
    );

PPH_LIST PhGetListViewLines(
    __in HWND ListViewHandle,
    __in ULONG Mode
    );

#endif

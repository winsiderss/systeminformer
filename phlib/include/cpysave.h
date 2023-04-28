/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2019
 *
 */

#ifndef _PH_CPYSAVE_H
#define _PH_CPYSAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_EXPORT_MODE_TABS 0
#define PH_EXPORT_MODE_SPACES 1
#define PH_EXPORT_MODE_CSV 2

PHLIBAPI
VOID PhaCreateTextTable(
    _Out_ PPH_STRING ***Table,
    _In_ ULONG Rows,
    _In_ ULONG Columns
    );

PHLIBAPI
PPH_LIST PhaFormatTextTable(
    _In_ PPH_STRING **Table,
    _In_ ULONG Rows,
    _In_ ULONG Columns,
    _In_ ULONG Mode
    );

PHLIBAPI
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

PHLIBAPI
VOID PhaMapDisplayIndexListView(
    _In_ HWND ListViewHandle,
    _Out_writes_(Count) PULONG DisplayToId,
    _Out_writes_opt_(Count) PPH_STRING *DisplayToText,
    _In_ ULONG Count,
    _Out_ PULONG NumberOfColumns
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetListViewItemText(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT SubItemIndex
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaGetListViewItemText(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT SubItemIndex
    );

PHLIBAPI
PPH_STRING PhGetListViewText(
    _In_ HWND ListViewHandle
    );

PHLIBAPI
PPH_LIST PhGetListViewLines(
    _In_ HWND ListViewHandle,
    _In_ ULONG Mode
    );

#ifdef __cplusplus
}
#endif

#endif

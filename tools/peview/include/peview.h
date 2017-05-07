#ifndef PEVIEW_H
#define PEVIEW_H

#include <ph.h>
#include <guisup.h>
#include <prsht.h>
#include <prpsh.h>
#include <settings.h>

#include "resource.h"

extern PPH_STRING PvFileName;

// peprp

VOID PvPeProperties(
    VOID
    );

// libprp

VOID PvLibProperties(
    VOID
    );

// misc

PPH_STRING PvResolveShortcutTarget(
    _In_ PPH_STRING ShortcutFileName
    );

VOID PvCopyListView(
    _In_ HWND ListViewHandle
    );

VOID PvHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    );

// settings

VOID PeInitializeSettings(
    VOID
    );

VOID PeSaveSettings(
    VOID
    );

// symbols

INT_PTR CALLBACK PvpSymbolsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PeDumpFileSymbols(
    _In_ HWND ListViewHandle,
    _In_ PWSTR FileName
    );

VOID PvPdbProperties(
    VOID
    );

#endif

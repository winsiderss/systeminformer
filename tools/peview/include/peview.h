#ifndef PEVIEW_H
#define PEVIEW_H

#include <ph.h>
#include <guisup.h>
#include <mapimg.h>
#include <prsht.h>
#include <prpsh.h>
#include <settings.h>
#include <symprv.h>

#include <uxtheme.h>

#include "resource.h"

extern PPH_STRING PvFileName;
extern PH_MAPPED_IMAGE PvMappedImage;
extern PIMAGE_COR20_HEADER PvImageCor20Header;
extern PPH_SYMBOL_PROVIDER PvSymbolProvider;

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

// 

INT_PTR CALLBACK PvpPeImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeClrDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeCgfDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif

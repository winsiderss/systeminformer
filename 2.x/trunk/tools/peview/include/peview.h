#ifndef PEVIEW_H
#define PEVIEW_H

#include <phgui.h>
#include <prsht.h>
#include "resource.h"

#ifndef MAIN_PRIVATE
extern PPH_STRING PvFileName;
#endif

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

#endif

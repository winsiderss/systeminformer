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
    __in PPH_STRING ShortcutFileName
    );

VOID PvCopyListView(
    __in HWND ListViewHandle
    );

VOID PvHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    );

#endif

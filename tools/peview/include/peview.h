#ifndef PEVIEW_H
#define PEVIEW_H

#include <phgui.h>
#include <prsht.h>
#include "resource.h"

#ifndef MAIN_PRIVATE
extern PPH_STRING PvFileName;
#endif

// peprp

VOID PvPeProperties();

// libprp

VOID PvLibProperties();

// misc

VOID PvCopyListView(
    __in HWND ListViewHandle
    );

VOID PvHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    );

#endif

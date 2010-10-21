#ifndef EXTENDEDTOOLS_H
#define EXTENDEDTOOLS_H

#define PHNT_VERSION PHNT_VISTA
#include <phdk.h>

extern PPH_PLUGIN PluginInstance;

// objprp

VOID EtHandlePropertiesInitializing(
    __in PVOID Parameter
    );

// unldll

VOID EtShowUnloadedDllsDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

#endif

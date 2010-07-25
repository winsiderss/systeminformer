#ifndef _PH_PHPLUG_H
#define _PH_PHPLUG_H

// Callbacks

typedef enum _PH_GENERAL_CALLBACK
{
    GeneralCallbackMainWindowShowing = 0, // INT ShowCommand [main thread]
    GeneralCallbackProcessesUpdated = 1, // [main thread]
    GeneralCallbackGetProcessHighlightingColor = 2, // PPH_PLUGIN_GET_HIGHLIGHTING_COLOR Data [main thread]
    GeneralCallbackGetProcessTooltipText = 3, // PPH_PLUGIN_GET_TOOLTIP_TEXT Data [main thread]

    GeneralCallbackMaximum
} PH_GENERAL_CALLBACK, *PPH_GENERAL_CALLBACK;

typedef struct _PH_PLUGIN_GET_HIGHLIGHTING_COLOR
{
    // Parameter is:
    // PPH_PROCESS_ITEM for GeneralCallbackGetProcessHighlightingColor

    PVOID Parameter;
    COLORREF BackColor;
    BOOLEAN Handled;
    BOOLEAN Cache;
} PH_PLUGIN_GET_HIGHLIGHTING_COLOR, *PPH_PLUGIN_GET_HIGHLIGHTING_COLOR;

typedef struct _PH_PLUGIN_GET_TOOLTIP_TEXT
{
    // Parameter is:
    // PPH_PROCESS_ITEM for GeneralCallbackGetProcessTooltipText

    PVOID Parameter;
    PPH_STRING_BUILDER StringBuilder;
} PH_PLUGIN_GET_TOOLTIP_TEXT, *PPH_PLUGIN_GET_TOOLTIP_TEXT;

typedef enum _PH_PLUGIN_CALLBACK
{
    PluginCallbackLoad = 0, // [main thread]
    PluginCallbackUnload = 1, // [main thread]
    PluginCallbackShowOptions = 2, // HWND ParentWindowHandle [main thread]
    PluginCallbackMenuItem = 3, // PPH_PLUGIN_MENU_ITEM MenuItem [main thread]

    PluginCallbackMaximum
} PH_PLUGIN_CALLBACK, *PPH_PLUGIN_CALLBACK;

typedef struct _PH_PLUGIN
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;

    PWSTR Name;
    PVOID DllBase;

    PWSTR DisplayName;
    PWSTR Author;
    PWSTR Description;
    BOOLEAN HasOptions;

    PH_CALLBACK Callbacks[PluginCallbackMaximum];
} PH_PLUGIN, *PPH_PLUGIN;

typedef struct _PH_PLUGIN_INFORMATION
{
    PWSTR DisplayName;
    PWSTR Author;
    PWSTR Description;
    BOOLEAN HasOptions;
} PH_PLUGIN_INFORMATION, *PPH_PLUGIN_INFORMATION;

PHAPPAPI
PPH_PLUGIN PhRegisterPlugin(
    __in PWSTR Name,
    __in PVOID DllBase,
    __in_opt PPH_PLUGIN_INFORMATION Information
    );

PHAPPAPI
PPH_CALLBACK PhGetPluginCallback(
    __in PPH_PLUGIN Plugin,
    __in PH_PLUGIN_CALLBACK Callback
    );

PHAPPAPI
PPH_CALLBACK PhGetGeneralCallback(
    __in PH_GENERAL_CALLBACK Callback
    );

typedef struct _PH_PLUGIN_MENU_ITEM
{
    PPH_PLUGIN Plugin;
    ULONG Id;
    PVOID Context;
} PH_PLUGIN_MENU_ITEM, *PPH_PLUGIN_MENU_ITEM;

#define PH_MENU_ITEM_LOCATION_TOOLS 2

PHAPPAPI
BOOLEAN PhPluginAddMenuItem(
    __in PPH_PLUGIN Plugin,
    __in ULONG Location,
    __in_opt PWSTR InsertAfter,
    __in ULONG Id,
    __in PWSTR Text,
    __in PVOID Context
    );

#endif

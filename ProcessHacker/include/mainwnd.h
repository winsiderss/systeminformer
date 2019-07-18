#ifndef PH_MAINWND_H
#define PH_MAINWND_H

PHAPPAPI extern HWND PhMainWndHandle; // phapppub
extern BOOLEAN PhMainWndExiting;

#define WM_PH_FIRST (WM_APP + 99)
#define WM_PH_ACTIVATE (WM_APP + 99)
#define PH_ACTIVATE_REPLY 0x1119

// begin_phapppub
#define WM_PH_SHOW_PROCESS_PROPERTIES (WM_APP + 120)
#define WM_PH_DESTROY (WM_APP + 121)
#define WM_PH_SAVE_ALL_SETTINGS (WM_APP + 122)
#define WM_PH_PREPARE_FOR_EARLY_SHUTDOWN (WM_APP + 123)
#define WM_PH_CANCEL_EARLY_SHUTDOWN (WM_APP + 124)
// end_phapppub
#define WM_PH_DELAYED_LOAD_COMPLETED (WM_APP + 125)
#define WM_PH_NOTIFY_ICON_MESSAGE (WM_APP + 126)
// begin_phapppub
#define WM_PH_TOGGLE_VISIBLE (WM_APP + 127)
#define WM_PH_SHOW_MEMORY_EDITOR (WM_APP + 128)
#define WM_PH_SHOW_MEMORY_RESULTS (WM_APP + 129)
#define WM_PH_SELECT_TAB_PAGE (WM_APP + 130)
#define WM_PH_GET_CALLBACK_LAYOUT_PADDING (WM_APP + 131)
#define WM_PH_INVALIDATE_LAYOUT_PADDING (WM_APP + 132)
#define WM_PH_SELECT_PROCESS_NODE (WM_APP + 133)
#define WM_PH_SELECT_SERVICE_ITEM (WM_APP + 134)
#define WM_PH_SELECT_NETWORK_ITEM (WM_APP + 135)
#define WM_PH_UPDATE_FONT (WM_APP + 136)
#define WM_PH_GET_FONT (WM_APP + 137)
// end_phapppub
// begin_phapppub
#define WM_PH_INVOKE (WM_APP + 138)
// WM_PH_DEPRECATED (WM_APP + 139)
#define WM_PH_CREATE_TAB_PAGE (WM_APP + 140)
#define WM_PH_REFRESH (WM_APP + 141)
#define WM_PH_GET_UPDATE_AUTOMATICALLY (WM_APP + 142)
#define WM_PH_SET_UPDATE_AUTOMATICALLY (WM_APP + 143)
// end_phapppub
#define WM_PH_ICON_CLICK (WM_APP + 144)
#define WM_PH_LAST (WM_APP + 144)

// begin_phapppub
#define ProcessHacker_ShowProcessProperties(hWnd, ProcessItem) \
    SendMessage(hWnd, WM_PH_SHOW_PROCESS_PROPERTIES, 0, (LPARAM)(ProcessItem))
#define ProcessHacker_Destroy(hWnd) \
    SendMessage(hWnd, WM_PH_DESTROY, 0, 0)
#define ProcessHacker_SaveAllSettings(hWnd) \
    SendMessage(hWnd, WM_PH_SAVE_ALL_SETTINGS, 0, 0)
#define ProcessHacker_PrepareForEarlyShutdown(hWnd) \
    SendMessage(hWnd, WM_PH_PREPARE_FOR_EARLY_SHUTDOWN, 0, 0)
#define ProcessHacker_CancelEarlyShutdown(hWnd) \
    SendMessage(hWnd, WM_PH_CANCEL_EARLY_SHUTDOWN, 0, 0)
#define ProcessHacker_ToggleVisible(hWnd, AlwaysShow) \
    SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, (WPARAM)(AlwaysShow), 0)
#define ProcessHacker_ShowMemoryEditor(hWnd, ShowMemoryEditor) \
    PostMessage(hWnd, WM_PH_SHOW_MEMORY_EDITOR, 0, (LPARAM)(ShowMemoryEditor))
#define ProcessHacker_ShowMemoryResults(hWnd, ShowMemoryResults) \
    PostMessage(hWnd, WM_PH_SHOW_MEMORY_RESULTS, 0, (LPARAM)(ShowMemoryResults))
#define ProcessHacker_SelectTabPage(hWnd, Index) \
    SendMessage(hWnd, WM_PH_SELECT_TAB_PAGE, (WPARAM)(Index), 0)
#define ProcessHacker_GetCallbackLayoutPadding(hWnd) \
    ((PPH_CALLBACK)SendMessage(hWnd, WM_PH_GET_CALLBACK_LAYOUT_PADDING, 0, 0))
#define ProcessHacker_InvalidateLayoutPadding(hWnd) \
    SendMessage(hWnd, WM_PH_INVALIDATE_LAYOUT_PADDING, 0, 0)
#define ProcessHacker_SelectProcessNode(hWnd, ProcessNode) \
    SendMessage(hWnd, WM_PH_SELECT_PROCESS_NODE, 0, (LPARAM)(ProcessNode))
#define ProcessHacker_SelectServiceItem(hWnd, ServiceItem) \
    SendMessage(hWnd, WM_PH_SELECT_SERVICE_ITEM, 0, (LPARAM)(ServiceItem))
#define ProcessHacker_SelectNetworkItem(hWnd, NetworkItem) \
    SendMessage(hWnd, WM_PH_SELECT_NETWORK_ITEM, 0, (LPARAM)(NetworkItem))
#define ProcessHacker_Invoke(hWnd, Function, Parameter) \
    PostMessage(hWnd, WM_PH_INVOKE, (WPARAM)(Parameter), (LPARAM)(Function))
#define ProcessHacker_CreateTabPage(hWnd, Template) \
    ((struct _PH_MAIN_TAB_PAGE *)SendMessage(hWnd, WM_PH_CREATE_TAB_PAGE, 0, (LPARAM)(Template)))
#define ProcessHacker_Refresh(hWnd) \
    SendMessage(hWnd, WM_PH_REFRESH, 0, 0)
#define ProcessHacker_GetUpdateAutomatically(hWnd) \
    ((BOOLEAN)SendMessage(hWnd, WM_PH_GET_UPDATE_AUTOMATICALLY, 0, 0))
#define ProcessHacker_SetUpdateAutomatically(hWnd, Value) \
    SendMessage(hWnd, WM_PH_SET_UPDATE_AUTOMATICALLY, (WPARAM)(Value), 0)
// end_phapppub
#define ProcessHacker_IconClick(hWnd) \
    SendMessage(hWnd, WM_PH_ICON_CLICK, 0, 0)

typedef struct _PH_SHOW_MEMORY_EDITOR
{
    HWND OwnerWindow;
    HANDLE ProcessId;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG SelectOffset;
    ULONG SelectLength;
    PPH_STRING Title;
    ULONG Flags;
} PH_SHOW_MEMORY_EDITOR, *PPH_SHOW_MEMORY_EDITOR;

typedef struct _PH_SHOW_MEMORY_RESULTS
{
    HANDLE ProcessId;
    PPH_LIST Results;
} PH_SHOW_MEMORY_RESULTS, *PPH_SHOW_MEMORY_RESULTS;

// begin_phapppub
typedef struct _PH_LAYOUT_PADDING_DATA
{
    RECT Padding;
} PH_LAYOUT_PADDING_DATA, *PPH_LAYOUT_PADDING_DATA;
// end_phapppub

// begin_phapppub
typedef enum _PH_MAIN_TAB_PAGE_MESSAGE
{
    MainTabPageCreate,
    MainTabPageDestroy,
    MainTabPageCreateWindow, // HWND *Parameter1 (WindowHandle)
    MainTabPageSelected, // BOOLEAN Parameter1 (Selected)
    MainTabPageInitializeSectionMenuItems, // PPH_MAIN_TAB_PAGE_MENU_INFORMATION Parameter1

    MainTabPageLoadSettings,
    MainTabPageSaveSettings,
    MainTabPageExportContent, // PPH_MAIN_TAB_PAGE_EXPORT_CONTENT Parameter1
    MainTabPageFontChanged, // HFONT Parameter1 (Font)
    MainTabPageUpdateAutomaticallyChanged, // BOOLEAN Parameter1 (UpdateAutomatically)

    MaxMainTabPageMessage
} PH_MAIN_TAB_PAGE_MESSAGE;

typedef BOOLEAN (NTAPI *PPH_MAIN_TAB_PAGE_CALLBACK)(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

typedef struct _PH_MAIN_TAB_PAGE_EXPORT_CONTENT
{
    PPH_FILE_STREAM FileStream;
    ULONG Mode;
} PH_MAIN_TAB_PAGE_EXPORT_CONTENT, *PPH_MAIN_TAB_PAGE_EXPORT_CONTENT;

typedef struct _PH_MAIN_TAB_PAGE_MENU_INFORMATION
{
    struct _PH_EMENU_ITEM *Menu;
    ULONG StartIndex;
} PH_MAIN_TAB_PAGE_MENU_INFORMATION, *PPH_MAIN_TAB_PAGE_MENU_INFORMATION;

typedef struct _PH_MAIN_TAB_PAGE
{
    // Public

    PH_STRINGREF Name;
    ULONG Flags;
    PPH_MAIN_TAB_PAGE_CALLBACK Callback;
    PVOID Context;

    INT Index;
    union
    {
        ULONG StateFlags;
        struct
        {
            ULONG Selected : 1;
            ULONG CreateWindowCalled : 1;
            ULONG SpareStateFlags : 30;
        };
    };
    PVOID Reserved[2];
// end_phapppub

    // Private

    HWND WindowHandle;
// begin_phapppub
} PH_MAIN_TAB_PAGE, *PPH_MAIN_TAB_PAGE;
// end_phapppub

// begin_phapppub
#define PH_NOTIFY_MINIMUM 0x1
#define PH_NOTIFY_PROCESS_CREATE 0x1
#define PH_NOTIFY_PROCESS_DELETE 0x2
#define PH_NOTIFY_SERVICE_CREATE 0x4
#define PH_NOTIFY_SERVICE_DELETE 0x8
#define PH_NOTIFY_SERVICE_START 0x10
#define PH_NOTIFY_SERVICE_STOP 0x20
#define PH_NOTIFY_MAXIMUM 0x40
#define PH_NOTIFY_VALID_MASK 0x3f
// end_phapppub

BOOLEAN PhMainWndInitialization(
    _In_ INT ShowCommand
    );

VOID PhAddMiniProcessMenuItems(
    _Inout_ struct _PH_EMENU_ITEM *Menu,
    _In_ HANDLE ProcessId
    );

BOOLEAN PhHandleMiniProcessMenuItem(
    _Inout_ struct _PH_EMENU_ITEM *MenuItem
    );

VOID PhShowIconContextMenu(
    _In_ POINT Location
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhShowIconNotification(
    _In_ PWSTR Title,
    _In_ PWSTR Text,
    _In_ ULONG Flags
    );
// end_phapppub

VOID PhShowDetailsForIconNotification(
    VOID
    );

VOID PhShowProcessContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

VOID PhShowServiceContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

VOID PhShowNetworkContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

#endif

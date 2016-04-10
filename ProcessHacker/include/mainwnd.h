#ifndef PH_MAINWND_H
#define PH_MAINWND_H

#define PH_MAINWND_CLASSNAME L"ProcessHacker" // phapppub

PHAPPAPI extern HWND PhMainWndHandle; // phapppub
extern BOOLEAN PhMainWndExiting;

#define WM_PH_FIRST (WM_APP + 99)
#define WM_PH_ACTIVATE (WM_APP + 99)
#define PH_ACTIVATE_REPLY 0x1119

#define WM_PH_PROCESS_ADDED (WM_APP + 101)
#define WM_PH_PROCESS_MODIFIED (WM_APP + 102)
#define WM_PH_PROCESS_REMOVED (WM_APP + 103)
#define WM_PH_PROCESSES_UPDATED (WM_APP + 104)

#define WM_PH_SERVICE_ADDED (WM_APP + 105)
#define WM_PH_SERVICE_MODIFIED (WM_APP + 106)
#define WM_PH_SERVICE_REMOVED (WM_APP + 107)
#define WM_PH_SERVICES_UPDATED (WM_APP + 108)

#define WM_PH_NETWORK_ITEM_ADDED (WM_APP + 109)
#define WM_PH_NETWORK_ITEM_MODIFIED (WM_APP + 110)
#define WM_PH_NETWORK_ITEM_REMOVED (WM_APP + 111)
#define WM_PH_NETWORK_ITEMS_UPDATED (WM_APP + 112)

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
// end_phapppub
#define WM_PH_UPDATE_FONT (WM_APP + 136)
#define WM_PH_GET_FONT (WM_APP + 137)
// begin_phapppub
#define WM_PH_INVOKE (WM_APP + 138)
#define WM_PH_ADD_MENU_ITEM (WM_APP + 139)
#define WM_PH_ADD_TAB_PAGE (WM_APP + 140)
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
#define ProcessHacker_AddMenuItem(hWnd, AddMenuItem) \
    ((ULONG_PTR)SendMessage(hWnd, WM_PH_ADD_MENU_ITEM, 0, (LPARAM)(AddMenuItem)))
#define ProcessHacker_AddTabPage(hWnd, TabPage) \
    ((PPH_ADDITIONAL_TAB_PAGE)SendMessage(hWnd, WM_PH_ADD_TAB_PAGE, 0, (LPARAM)(TabPage)))
#define ProcessHacker_Refresh(hWnd) \
    SendMessage(hWnd, WM_PH_REFRESH, 0, 0)
#define ProcessHacker_GetUpdateAutomatically(hWnd) \
    ((BOOLEAN)SendMessage(hWnd, WM_PH_GET_UPDATE_AUTOMATICALLY, 0, 0))
#define ProcessHacker_SetUpdateAutomatically(hWnd, Value) \
    SendMessage(hWnd, WM_PH_SET_UPDATE_AUTOMATICALLY, (WPARAM)(Value), 0)
// end_phapppub
#define ProcessHacker_IconClick(hWnd) \
    SendMessage(hWnd, WM_PH_ICON_CLICK, 0, 0)

typedef struct _PH_SHOWMEMORYEDITOR
{
    HANDLE ProcessId;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG SelectOffset;
    ULONG SelectLength;
    PPH_STRING Title;
    ULONG Flags;
} PH_SHOWMEMORYEDITOR, *PPH_SHOWMEMORYEDITOR;

typedef struct _PH_SHOWMEMORYRESULTS
{
    HANDLE ProcessId;
    PPH_LIST Results;
} PH_SHOWMEMORYRESULTS, *PPH_SHOWMEMORYRESULTS;

// begin_phapppub
typedef struct _PH_LAYOUT_PADDING_DATA
{
    RECT Padding;
} PH_LAYOUT_PADDING_DATA, *PPH_LAYOUT_PADDING_DATA;

typedef struct _PH_ADDMENUITEM
{
    _In_ PVOID Plugin;
    _In_ ULONG Location;
    _In_opt_ PWSTR InsertAfter;
    _In_ ULONG Flags;
    _In_ ULONG Id;
    _In_ PWSTR Text;
    _In_opt_ PVOID Context;
} PH_ADDMENUITEM, *PPH_ADDMENUITEM;

typedef HWND (NTAPI *PPH_TAB_PAGE_CREATE_FUNCTION)(
    _In_ PVOID Context
    );

typedef VOID (NTAPI *PPH_TAB_PAGE_CALLBACK_FUNCTION)(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    );

typedef struct _PH_ADDITIONAL_TAB_PAGE
{
    PWSTR Text;
    PVOID Context;
    PPH_TAB_PAGE_CREATE_FUNCTION CreateFunction;
    HWND WindowHandle;
    INT Index;
    PPH_TAB_PAGE_CALLBACK_FUNCTION SelectionChangedCallback;
    PPH_TAB_PAGE_CALLBACK_FUNCTION SaveContentCallback;
    PPH_TAB_PAGE_CALLBACK_FUNCTION FontChangedCallback;
    PPH_TAB_PAGE_CALLBACK_FUNCTION InitializeSectionMenuItemsCallback;
    PVOID Reserved[3];
} PH_ADDITIONAL_TAB_PAGE, *PPH_ADDITIONAL_TAB_PAGE;
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

VOID PhLoadDbgHelpFromPath(
    _In_ PWSTR DbgHelpPath
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

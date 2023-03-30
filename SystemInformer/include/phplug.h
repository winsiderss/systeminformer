#ifndef PH_PHPLUG_H
#define PH_PHPLUG_H

#include <extmgr.h>
#include <sysinfo.h>
#include <miniinfo.h>

// begin_phapppub
// Callbacks

typedef enum _PH_GENERAL_CALLBACK
{
    GeneralCallbackMainWindowShowing = 0, // INT ShowCommand [main thread]
    GeneralCallbackProcessesUpdated = 1, // [main thread]
    GeneralCallbackGetProcessHighlightingColor = 2, // PPH_PLUGIN_GET_HIGHLIGHTING_COLOR Data [main thread]
    GeneralCallbackGetProcessTooltipText = 3, // PPH_PLUGIN_GET_TOOLTIP_TEXT Data [main thread]
    GeneralCallbackProcessPropertiesInitializing = 4, // PPH_PLUGIN_PROCESS_PROPCONTEXT Data [properties thread]
    GeneralCallbackMainMenuInitializing = 5, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
    GeneralCallbackNotifyEvent = 6, // PPH_PLUGIN_NOTIFY_EVENT Data [main thread]
    GeneralCallbackServicePropertiesInitializing = 7, // PPH_PLUGIN_OBJECT_PROPERTIES Data [properties thread]
    GeneralCallbackHandlePropertiesInitializing = 8, // PPH_PLUGIN_OBJECT_PROPERTIES Data [properties thread]
    GeneralCallbackProcessMenuInitializing = 9, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
    GeneralCallbackServiceMenuInitializing = 10, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
    GeneralCallbackNetworkMenuInitializing = 11, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
    GeneralCallbackIconMenuInitializing = 12, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
    GeneralCallbackThreadMenuInitializing = 13, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
    GeneralCallbackModuleMenuInitializing = 14, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
    GeneralCallbackMemoryMenuInitializing = 15, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
    GeneralCallbackHandleMenuInitializing = 16, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
    GeneralCallbackProcessTreeNewInitializing = 17, // PPH_PLUGIN_TREENEW_INFORMATION Data [main thread]
    GeneralCallbackServiceTreeNewInitializing = 18, // PPH_PLUGIN_TREENEW_INFORMATION Data [main thread]
    GeneralCallbackNetworkTreeNewInitializing = 19, // PPH_PLUGIN_TREENEW_INFORMATION Data [main thread]
    GeneralCallbackModuleTreeNewInitializing = 20, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackModuleTreeNewUninitializing = 21, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackThreadTreeNewInitializing = 22, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackThreadTreeNewUninitializing = 23, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackHandleTreeNewInitializing = 24, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackHandleTreeNewUninitializing = 25, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackThreadStackControl = 26, // PPH_PLUGIN_THREAD_STACK_CONTROL Data [properties thread]
    GeneralCallbackSystemInformationInitializing = 27, // PPH_PLUGIN_SYSINFO_POINTERS Data [system information thread]
    GeneralCallbackMainWindowTabChanged = 28, // INT NewIndex [main thread]
    GeneralCallbackMemoryTreeNewInitializing = 29, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackMemoryTreeNewUninitializing = 30, // PPH_PLUGIN_TREENEW_INFORMATION Data [properties thread]
    GeneralCallbackMemoryItemListControl = 31, // PPH_PLUGIN_MEMORY_ITEM_LIST_CONTROL Data [properties thread]
    GeneralCallbackMiniInformationInitializing = 32, // PPH_PLUGIN_MINIINFO_POINTERS Data [main thread]
    GeneralCallbackMiListSectionMenuInitializing = 33, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
    GeneralCallbackOptionsWindowInitializing = 34, // PPH_PLUGIN_OBJECT_PROPERTIES Data [main thread]

    GeneralCallbackProcessProviderAddedEvent, // [process provider thread]
    GeneralCallbackProcessProviderModifiedEvent, // [process provider thread]
    GeneralCallbackProcessProviderRemovedEvent, // [process provider thread]
    GeneralCallbackProcessProviderUpdatedEvent, // [process provider thread]
    GeneralCallbackServiceProviderAddedEvent, // [service provider thread]
    GeneralCallbackServiceProviderModifiedEvent, // [service provider thread]
    GeneralCallbackServiceProviderRemovedEvent, // [service provider thread]
    GeneralCallbackServiceProviderUpdatedEvent, // [service provider thread]
    GeneralCallbackNetworkProviderAddedEvent, // [network provider thread]
    GeneralCallbackNetworkProviderModifiedEvent, // [network provider thread]
    GeneralCallbackNetworkProviderRemovedEvent, // [network provider thread]
    GeneralCallbackNetworkProviderUpdatedEvent, // [network provider thread]

    GeneralCallbackLoggedEvent,
    GeneralCallbackTrayIconsInitializing,
    GeneralCallbackWindowNotifyEvent,
    GeneralCallbackProcessStatsNotifyEvent,
    GeneralCallbackSettingsUpdated,

    GeneralCallbackMaximum
} PH_GENERAL_CALLBACK, *PPH_GENERAL_CALLBACK;

typedef enum _PH_PLUGIN_CALLBACK
{
    PluginCallbackLoad = 0, // PPH_LIST Parameters [main thread] // list of strings, might be NULL
    PluginCallbackUnload = 1, // BOOLEAN SessionEnding [main thread]
    PluginCallbackShowOptions = 2, // HWND ParentWindowHandle [main thread]
    PluginCallbackMenuItem = 3, // PPH_PLUGIN_MENU_ITEM MenuItem [main/properties thread]
    PluginCallbackTreeNewMessage = 4, // PPH_PLUGIN_TREENEW_MESSAGE Message [main/properties thread]
    PluginCallbackPhSvcRequest = 5, // PPH_PLUGIN_PHSVC_REQUEST Message [phsvc thread]
    PluginCallbackMenuHook = 6, // PH_PLUGIN_MENU_HOOK_INFORMATION MenuHookInfo [menu thread]
    PluginCallbackMaximum
} PH_PLUGIN_CALLBACK, *PPH_PLUGIN_CALLBACK;

typedef struct _PH_PLUGIN_GET_HIGHLIGHTING_COLOR
{
    // Parameter is:
    // PPH_PROCESS_ITEM for GeneralCallbackGetProcessHighlightingColor

    PVOID Parameter;
    COLORREF BackColor;
    COLORREF ForeColor;
    BOOLEAN Handled;
    BOOLEAN Cache;
} PH_PLUGIN_GET_HIGHLIGHTING_COLOR, *PPH_PLUGIN_GET_HIGHLIGHTING_COLOR;

typedef struct _PH_PLUGIN_GET_TOOLTIP_TEXT
{
    // Parameter is:
    // PPH_PROCESS_ITEM for GeneralCallbackGetProcessTooltipText

    PVOID Parameter;
    PPH_STRING_BUILDER StringBuilder;
    ULONG ValidForMs;
} PH_PLUGIN_GET_TOOLTIP_TEXT, *PPH_PLUGIN_GET_TOOLTIP_TEXT;

typedef struct _PH_PLUGIN_PROCESS_PROPCONTEXT
{
    PPH_PROCESS_PROPCONTEXT PropContext;
    PPH_PROCESS_ITEM ProcessItem;
} PH_PLUGIN_PROCESS_PROPCONTEXT, *PPH_PLUGIN_PROCESS_PROPCONTEXT;

typedef struct _PH_PLUGIN_NOTIFY_EVENT
{
    // Parameter is:
    // PPH_PROCESS_ITEM for Type = PH_NOTIFY_PROCESS_*
    // PPH_SERVICE_ITEM for Type = PH_NOTIFY_SERVICE_*

    ULONG Type;
    BOOLEAN Handled;
    PVOID Parameter;
} PH_PLUGIN_NOTIFY_EVENT, *PPH_PLUGIN_NOTIFY_EVENT;

typedef struct _PH_PLUGIN_OBJECT_PROPERTIES
{
    // Parameter is:
    // PPH_SERVICE_ITEM for GeneralCallbackServicePropertiesInitializing
    // PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT for GeneralCallbackHandlePropertiesInitializing

    PVOID Parameter;
    ULONG NumberOfPages;
    ULONG MaximumNumberOfPages;
    HPROPSHEETPAGE *Pages;
} PH_PLUGIN_OBJECT_PROPERTIES, *PPH_PLUGIN_OBJECT_PROPERTIES;

typedef struct _PH_PLUGIN_PROCESS_STATS_EVENT
{
    ULONG Version;
    ULONG Type;
    PPH_PROCESS_ITEM ProcessItem;
    PVOID Parameter;
} PH_PLUGIN_PROCESS_STATS_EVENT, *PPH_PLUGIN_PROCESS_STATS_EVENT;

typedef struct _PH_PLUGIN_HANDLE_PROPERTIES_CONTEXT
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
} PH_PLUGIN_HANDLE_PROPERTIES_CONTEXT, *PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT;

typedef struct _PH_EMENU_ITEM *PPH_EMENU_ITEM, *PPH_EMENU;

#define PH_PLUGIN_MENU_DISALLOW_HOOKS 0x1

typedef struct _PH_PLUGIN_MENU_INFORMATION
{
    PPH_EMENU Menu;
    HWND OwnerWindow;

    union
    {
        struct
        {
            PVOID Reserved[8]; // Reserve space for future expansion of this union
        } DoNotUse;
        struct
        {
            ULONG SubMenuIndex;
        } MainMenu;
        struct
        {
            PPH_PROCESS_ITEM *Processes;
            ULONG NumberOfProcesses;
        } Process;
        struct
        {
            PPH_SERVICE_ITEM *Services;
            ULONG NumberOfServices;
        } Service;
        struct
        {
            PPH_NETWORK_ITEM *NetworkItems;
            ULONG NumberOfNetworkItems;
        } Network;
        struct
        {
            HANDLE ProcessId;
            PPH_THREAD_ITEM *Threads;
            ULONG NumberOfThreads;
        } Thread;
        struct
        {
            HANDLE ProcessId;
            PPH_MODULE_ITEM *Modules;
            ULONG NumberOfModules;
        } Module;
        struct
        {
            HANDLE ProcessId;
            PPH_MEMORY_NODE *MemoryNodes;
            ULONG NumberOfMemoryNodes;
        } Memory;
        struct
        {
            HANDLE ProcessId;
            PPH_HANDLE_ITEM *Handles;
            ULONG NumberOfHandles;
        } Handle;
        struct
        {
            PPH_STRINGREF SectionName;
            PPH_PROCESS_GROUP ProcessGroup;
        } MiListSection;
    } u;

    ULONG Flags;
    PPH_LIST PluginHookList;
} PH_PLUGIN_MENU_INFORMATION, *PPH_PLUGIN_MENU_INFORMATION;

C_ASSERT(RTL_FIELD_SIZE(PH_PLUGIN_MENU_INFORMATION, u) == RTL_FIELD_SIZE(PH_PLUGIN_MENU_INFORMATION, u.DoNotUse));

typedef struct _PH_PLUGIN_MENU_HOOK_INFORMATION
{
    PPH_PLUGIN_MENU_INFORMATION MenuInfo;
    PPH_EMENU SelectedItem;
    PVOID Context;
    BOOLEAN Handled;
} PH_PLUGIN_MENU_HOOK_INFORMATION, *PPH_PLUGIN_MENU_HOOK_INFORMATION;

typedef struct _PH_PLUGIN_TREENEW_INFORMATION
{
    HWND TreeNewHandle;
    PVOID CmData;
    PVOID SystemContext; // e.g. PPH_THREADS_CONTEXT
} PH_PLUGIN_TREENEW_INFORMATION, *PPH_PLUGIN_TREENEW_INFORMATION;

typedef enum _PH_PLUGIN_THREAD_STACK_CONTROL_TYPE
{
    PluginThreadStackInitializing,
    PluginThreadStackUninitializing,
    PluginThreadStackResolveSymbol,
    PluginThreadStackGetTooltip,
    PluginThreadStackWalkStack,
    PluginThreadStackBeginDefaultWalkStack,
    PluginThreadStackEndDefaultWalkStack,
    PluginThreadStackMaximum
} PH_PLUGIN_THREAD_STACK_CONTROL_TYPE;

typedef struct _PH_SYMBOL_PROVIDER *PPH_SYMBOL_PROVIDER;
typedef struct _PH_THREAD_STACK_FRAME *PPH_THREAD_STACK_FRAME;

typedef BOOLEAN (NTAPI *PPH_PLUGIN_WALK_THREAD_STACK_CALLBACK)(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    );

typedef struct _PH_PLUGIN_THREAD_STACK_CONTROL
{
    PH_PLUGIN_THREAD_STACK_CONTROL_TYPE Type;
    PVOID UniqueKey;

    union
    {
        struct
        {
            HANDLE ProcessId;
            HANDLE ThreadId;
            HANDLE ThreadHandle;
            HANDLE ProcessHandle;
            PPH_SYMBOL_PROVIDER SymbolProvider;
            BOOLEAN CustomWalk;
        } Initializing;
        struct
        {
            PPH_THREAD_STACK_FRAME StackFrame;
            PPH_STRING Symbol;
            PPH_STRING FileName;
        } ResolveSymbol;
        struct
        {
            PPH_THREAD_STACK_FRAME StackFrame;
            PPH_STRING_BUILDER StringBuilder;
        } GetTooltip;
        struct
        {
            NTSTATUS Status;
            HANDLE ThreadHandle;
            HANDLE ProcessHandle;
            PCLIENT_ID ClientId;
            ULONG Flags;
            PPH_PLUGIN_WALK_THREAD_STACK_CALLBACK Callback;
            PVOID CallbackContext;
        } WalkStack;
    } u;
} PH_PLUGIN_THREAD_STACK_CONTROL, *PPH_PLUGIN_THREAD_STACK_CONTROL;

typedef enum _PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL_TYPE
{
    PluginMemoryItemListInitialized,
    PluginMemoryItemListMaximum
} PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL_TYPE;

typedef struct _PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL
{
    PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL_TYPE Type;

    union
    {
        struct
        {
            PPH_MEMORY_ITEM_LIST List;
        } Initialized;
    } u;
} PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL, *PPH_PLUGIN_MEMORY_ITEM_LIST_CONTROL;

typedef PPH_SYSINFO_SECTION (NTAPI *PPH_SYSINFO_CREATE_SECTION)(
    _In_ PPH_SYSINFO_SECTION Template
    );

typedef PPH_SYSINFO_SECTION (NTAPI *PPH_SYSINFO_FIND_SECTION)(
    _In_ PPH_STRINGREF Name
    );

typedef VOID (NTAPI *PPH_SYSINFO_ENTER_SECTION_VIEW)(
    _In_ PPH_SYSINFO_SECTION NewSection
    );

typedef VOID (NTAPI *PPH_SYSINFO_RESTORE_SUMMARY_VIEW)(
    VOID
    );

typedef struct _PH_PLUGIN_SYSINFO_POINTERS
{
    HWND WindowHandle;
    PPH_SYSINFO_CREATE_SECTION CreateSection;
    PPH_SYSINFO_FIND_SECTION FindSection;
    PPH_SYSINFO_ENTER_SECTION_VIEW EnterSectionView;
    PPH_SYSINFO_RESTORE_SUMMARY_VIEW RestoreSummaryView;
} PH_PLUGIN_SYSINFO_POINTERS, *PPH_PLUGIN_SYSINFO_POINTERS;

typedef PPH_MINIINFO_SECTION (NTAPI *PPH_MINIINFO_CREATE_SECTION)(
    _In_ PPH_MINIINFO_SECTION Template
    );

typedef PPH_MINIINFO_SECTION (NTAPI *PPH_MINIINFO_FIND_SECTION)(
    _In_ PPH_STRINGREF Name
    );

typedef PPH_MINIINFO_LIST_SECTION (NTAPI *PPH_MINIINFO_CREATE_LIST_SECTION)(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION Template
    );

typedef struct _PH_PLUGIN_MINIINFO_POINTERS
{
    HWND WindowHandle;
    PPH_MINIINFO_CREATE_SECTION CreateSection;
    PPH_MINIINFO_FIND_SECTION FindSection;
    PPH_MINIINFO_CREATE_LIST_SECTION CreateListSection;
} PH_PLUGIN_MINIINFO_POINTERS, *PPH_PLUGIN_MINIINFO_POINTERS;
// end_phapppub

// begin_phapppub
/**
 * Creates a notification icon.
 *
 * \param Plugin A plugin instance structure.
 * \param SubId An identifier for the column. This should be unique within the
 * plugin.
 * \param Guid A unique guid for this icon.
 * \param Context A user-defined value.
 * \param Text A string describing the notification icon.
 * \param Flags A combination of flags.
 * \li \c PH_NF_ICON_UNAVAILABLE The notification icon is currently unavailable.
 * \param RegistrationData A \ref PH_NF_ICON_REGISTRATION_DATA structure that
 * contains registration information.
 */
typedef struct _PH_NF_ICON * (NTAPI *PPH_REGISTER_TRAY_ICON)(
    _In_ struct _PH_PLUGIN * Plugin,
    _In_ ULONG SubId,
    _In_ GUID Guid,
    _In_opt_ PVOID Context,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_ struct _PH_NF_ICON_REGISTRATION_DATA *RegistrationData
    );

typedef struct _PH_TRAY_ICON_POINTERS
{
    PPH_REGISTER_TRAY_ICON RegisterTrayIcon;
} PH_TRAY_ICON_POINTERS, *PPH_TRAY_ICON_POINTERS;
// end_phapppub

// begin_phapppub
typedef struct _PH_OPTIONS_SECTION
{
    PH_STRINGREF Name;
    // end_phapppub

    PVOID Instance;
    PWSTR Template;
    DLGPROC DialogProc;
    PVOID Parameter;

    HWND DialogHandle;
    HTREEITEM TreeItemHandle;
    // begin_phapppub
} PH_OPTIONS_SECTION, *PPH_OPTIONS_SECTION;
// end_phapppub

// begin_phapppub
typedef PPH_OPTIONS_SECTION (NTAPI *PPH_OPTIONS_CREATE_SECTION)(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

typedef PPH_OPTIONS_SECTION (NTAPI *PPH_OPTIONS_FIND_SECTION)(
    _In_ PPH_STRINGREF Name
    );

typedef VOID (NTAPI *PPH_OPTIONS_ENTER_SECTION_VIEW)(
    _In_ PPH_OPTIONS_SECTION NewSection
    );

typedef struct _PH_PLUGIN_OPTIONS_POINTERS
{
    HWND WindowHandle;
    PPH_OPTIONS_CREATE_SECTION CreateSection;
    PPH_OPTIONS_FIND_SECTION FindSection;
    PPH_OPTIONS_ENTER_SECTION_VIEW EnterSectionView;
} PH_PLUGIN_OPTIONS_POINTERS, *PPH_PLUGIN_OPTIONS_POINTERS;
// end_phapppub

// begin_phapppub
typedef struct _PH_PLUGIN_TREENEW_MESSAGE
{
    HWND TreeNewHandle;
    PH_TREENEW_MESSAGE Message;
    PVOID Parameter1;
    PVOID Parameter2;
    ULONG SubId;
    PVOID Context;
} PH_PLUGIN_TREENEW_MESSAGE, *PPH_PLUGIN_TREENEW_MESSAGE;

typedef LONG (NTAPI *PPH_PLUGIN_TREENEW_SORT_FUNCTION)(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    );

typedef NTSTATUS (NTAPI *PPHSVC_SERVER_PROBE_BUFFER)(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ ULONG Alignment,
    _In_ BOOLEAN AllowNull,
    _Out_ PVOID *Pointer
    );

typedef NTSTATUS (NTAPI *PPHSVC_SERVER_CAPTURE_BUFFER)(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PVOID *CapturedBuffer
    );

typedef NTSTATUS (NTAPI *PPHSVC_SERVER_CAPTURE_STRING)(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PPH_STRING *CapturedString
    );

typedef struct _PH_PLUGIN_PHSVC_REQUEST
{
    ULONG SubId;
    NTSTATUS ReturnStatus;
    PVOID InBuffer;
    ULONG InLength;
    PVOID OutBuffer;
    ULONG OutLength;

    PPHSVC_SERVER_PROBE_BUFFER ProbeBuffer;
    PPHSVC_SERVER_CAPTURE_BUFFER CaptureBuffer;
    PPHSVC_SERVER_CAPTURE_STRING CaptureString;
} PH_PLUGIN_PHSVC_REQUEST, *PPH_PLUGIN_PHSVC_REQUEST;

typedef VOID (NTAPI *PPHSVC_CLIENT_FREE_HEAP)(
    _In_ PVOID Memory
    );

typedef PVOID (NTAPI *PPHSVC_CLIENT_CREATE_STRING)(
    _In_opt_ PVOID String,
    _In_ SIZE_T Length,
    _Out_ PPH_RELATIVE_STRINGREF StringRef
    );

typedef struct _PH_PLUGIN_PHSVC_CLIENT
{
    HANDLE ServerProcessId;
    PPHSVC_CLIENT_FREE_HEAP FreeHeap;
    PPHSVC_CLIENT_CREATE_STRING CreateString;
} PH_PLUGIN_PHSVC_CLIENT, *PPH_PLUGIN_PHSVC_CLIENT;

// Plugin structures

typedef struct _PH_PLUGIN_INFORMATION
{
    PWSTR DisplayName;
    PWSTR Author;
    PWSTR Description;
    PWSTR Url;
    BOOLEAN HasOptions;
    BOOLEAN Reserved1[3];
    PVOID Interface;
} PH_PLUGIN_INFORMATION, *PPH_PLUGIN_INFORMATION;

#define PH_PLUGIN_FLAG_RESERVED 0x1
// end_phapppub

// begin_phapppub
typedef struct _PH_PLUGIN
{
    // Public

    PH_AVL_LINKS Links;

    PVOID Reserved;
    PVOID DllBase;
// end_phapppub

    // Private

    //PPH_STRING FileName;
    ULONG Flags;
    PH_STRINGREF Name;
    PH_PLUGIN_INFORMATION Information;

    PH_CALLBACK Callbacks[PluginCallbackMaximum];
    PH_EM_APP_CONTEXT AppContext;
// begin_phapppub
} PH_PLUGIN, *PPH_PLUGIN;
// end_phapppub

// begin_phapppub
// Plugin API

PHAPPAPI
PPH_PLUGIN
NTAPI
PhRegisterPlugin(
    _In_ PWSTR Name,
    _In_ PVOID DllBase,
    _Out_opt_ PPH_PLUGIN_INFORMATION *Information
    );

PHAPPAPI
PPH_PLUGIN
NTAPI
PhFindPlugin(
    _In_ PWSTR Name
    );

PHAPPAPI
PPH_PLUGIN_INFORMATION
NTAPI
PhGetPluginInformation(
    _In_ PPH_PLUGIN Plugin
    );

PHAPPAPI
PPH_CALLBACK
NTAPI
PhGetPluginCallback(
    _In_ PPH_PLUGIN Plugin,
    _In_ PH_PLUGIN_CALLBACK Callback
    );

PHAPPAPI
PPH_CALLBACK
NTAPI
PhGetGeneralCallback(
    _In_ PH_GENERAL_CALLBACK Callback
    );

PHAPPAPI
ULONG
NTAPI
PhPluginReserveIds(
    _In_ ULONG Count
    );

typedef VOID (NTAPI *PPH_PLUGIN_MENU_ITEM_DELETE_FUNCTION)(
    _In_ struct _PH_PLUGIN_MENU_ITEM *MenuItem
    );

typedef struct _PH_PLUGIN_MENU_ITEM
{
    PPH_PLUGIN Plugin;
    ULONG Id;
    ULONG Reserved1;
    PVOID Context;

    HWND OwnerWindow; // valid only when the menu item is chosen
    PVOID Reserved2;
    PVOID Reserved3;
    PPH_PLUGIN_MENU_ITEM_DELETE_FUNCTION DeleteFunction; // valid only for EMENU-based menu items
} PH_PLUGIN_MENU_ITEM, *PPH_PLUGIN_MENU_ITEM;

// Location
#define PH_MENU_ITEM_LOCATION_HACKER 0
#define PH_MENU_ITEM_LOCATION_VIEW 1
#define PH_MENU_ITEM_LOCATION_TOOLS 2
#define PH_MENU_ITEM_LOCATION_USERS 3
#define PH_MENU_ITEM_LOCATION_HELP 4

typedef struct _PH_PLUGIN_SYSTEM_STATISTICS
{
    PSYSTEM_PERFORMANCE_INFORMATION Performance;

    ULONG NumberOfProcesses;
    ULONG NumberOfThreads;
    ULONG NumberOfHandles;

    FLOAT CpuKernelUsage;
    FLOAT CpuUserUsage;

    PH_UINT64_DELTA IoReadDelta;
    PH_UINT64_DELTA IoWriteDelta;
    PH_UINT64_DELTA IoOtherDelta;

    ULONG CommitPages;
    ULONG PhysicalPages;

    HANDLE MaxCpuProcessId;
    HANDLE MaxIoProcessId;

    PPH_CIRCULAR_BUFFER_FLOAT CpuKernelHistory;
    PPH_CIRCULAR_BUFFER_FLOAT CpuUserHistory;
    PPH_CIRCULAR_BUFFER_FLOAT *CpusKernelHistory;
    PPH_CIRCULAR_BUFFER_FLOAT *CpusUserHistory;
    PPH_CIRCULAR_BUFFER_ULONG64 IoReadHistory;
    PPH_CIRCULAR_BUFFER_ULONG64 IoWriteHistory;
    PPH_CIRCULAR_BUFFER_ULONG64 IoOtherHistory;
    PPH_CIRCULAR_BUFFER_ULONG CommitHistory;
    PPH_CIRCULAR_BUFFER_ULONG PhysicalHistory;
    PPH_CIRCULAR_BUFFER_ULONG MaxCpuHistory; // ID of max. CPU process
    PPH_CIRCULAR_BUFFER_ULONG MaxIoHistory; // ID of max. I/O process
    PPH_CIRCULAR_BUFFER_FLOAT MaxCpuUsageHistory;
    PPH_CIRCULAR_BUFFER_ULONG64 MaxIoReadOtherHistory;
    PPH_CIRCULAR_BUFFER_ULONG64 MaxIoWriteHistory;
} PH_PLUGIN_SYSTEM_STATISTICS, *PPH_PLUGIN_SYSTEM_STATISTICS;

PHAPPAPI
VOID
NTAPI
PhPluginGetSystemStatistics(
    _Out_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

PHAPPAPI
PPH_EMENU_ITEM
NTAPI
PhPluginCreateEMenuItem(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG Flags,
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_opt_ PVOID Context
    );

PHAPPAPI
BOOLEAN
NTAPI
PhPluginAddMenuHook(
    _Inout_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_ PPH_PLUGIN Plugin,
    _In_opt_ PVOID Context
    );
// end_phapppub

VOID
NTAPI
PhPluginInitializeMenuInfo(
    _Out_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_opt_ PPH_EMENU Menu,
    _In_ HWND OwnerWindow,
    _In_ ULONG Flags
    );

BOOLEAN
NTAPI
PhPluginTriggerEMenuItem(
    _In_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_ PPH_EMENU_ITEM Item
    );

// begin_phapppub
PHAPPAPI
BOOLEAN
NTAPI
PhPluginAddTreeNewColumn(
    _In_ PPH_PLUGIN Plugin,
    _In_ PVOID CmData,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_PLUGIN_TREENEW_SORT_FUNCTION SortFunction
    );

PHAPPAPI
VOID
NTAPI
PhPluginSetObjectExtension(
    _In_ PPH_PLUGIN Plugin,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ ULONG ExtensionSize,
    _In_opt_ PPH_EM_OBJECT_CALLBACK CreateCallback,
    _In_opt_ PPH_EM_OBJECT_CALLBACK DeleteCallback
    );

PHAPPAPI
PVOID
NTAPI
PhPluginGetObjectExtension(
    _In_ PPH_PLUGIN Plugin,
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType
    );

PHAPPAPI
VOID
NTAPI
PhPluginEnableTreeNewNotify(
    _In_ PPH_PLUGIN Plugin,
    _In_ PVOID CmData
    );

PHAPPAPI
_Success_(return)
BOOLEAN
NTAPI
PhPluginQueryPhSvc(
    _Out_ PPH_PLUGIN_PHSVC_CLIENT Client
    );

PHAPPAPI
NTSTATUS
NTAPI
PhPluginCallPhSvc(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG SubId,
    _In_reads_bytes_opt_(InLength) PVOID InBuffer,
    _In_ ULONG InLength,
    _Out_writes_bytes_opt_(OutLength) PVOID OutBuffer,
    _In_ ULONG OutLength
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetPluginName(
    _In_ PPH_PLUGIN Plugin
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetPluginFileName(
    _In_ PPH_PLUGIN Plugin
    );

typedef BOOLEAN (NTAPI* PPH_PLUGIN_ENUMERATE)(
    _In_ PPH_PLUGIN Information,
    _In_opt_ PVOID Context
    );

PHAPPAPI
VOID
NTAPI
PhEnumeratePlugins(
    _In_ PPH_PLUGIN_ENUMERATE Callback,
    _In_opt_ PVOID Context
    );

// end_phapppub

#endif

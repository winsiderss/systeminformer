/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016-2017
 *     dmex    2017-2023
 *
 */

#ifndef PH_APPSUP_H
#define PH_APPSUP_H

extern GUID XP_CONTEXT_GUID;
extern GUID VISTA_CONTEXT_GUID;
extern GUID WIN7_CONTEXT_GUID;
extern GUID WIN8_CONTEXT_GUID;
extern GUID WINBLUE_CONTEXT_GUID;
extern GUID WIN10_CONTEXT_GUID;

// begin_phapppub
PHAPPAPI
BOOLEAN
NTAPI
PhGetProcessIsSuspended(
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhIsProcessSuspended(
    _In_ HANDLE ProcessId
    );

BOOLEAN
NTAPI
PhIsProcessBackground(
    _In_ ULONG PriorityClass
    );

PHAPPAPI
PPH_STRINGREF
NTAPI
PhGetProcessPriorityClassString(
    _In_ ULONG PriorityClass
    );
// end_phapppub

NTSTATUS PhGetProcessSwitchContext(
    _In_ HANDLE ProcessHandle,
    _Out_ PGUID Guid
    );

// begin_phapppub
typedef enum _PH_KNOWN_PROCESS_TYPE
{
    UnknownProcessType,
    SystemProcessType, // ntoskrnl/ntkrnlpa/...
    SessionManagerProcessType, // smss
    WindowsSubsystemProcessType, // csrss
    WindowsStartupProcessType, // wininit
    ServiceControlManagerProcessType, // services
    LocalSecurityAuthorityProcessType, // lsass
    LocalSessionManagerProcessType, // lsm
    WindowsLogonProcessType, // winlogon
    ServiceHostProcessType, // svchost
    RunDllAsAppProcessType, // rundll32
    ComSurrogateProcessType, // dllhost
    TaskHostProcessType, // taskeng, taskhost, taskhostex
    ExplorerProcessType, // explorer
    UmdfHostProcessType, // wudfhost
    NtVdmHostProcessType, // ntvdm
    //EdgeProcessType, // Microsoft Edge
    WmiProviderHostType,
    MaximumProcessType,
    KnownProcessTypeMask = 0xffff,

    KnownProcessWow64 = 0x20000
} PH_KNOWN_PROCESS_TYPE;

PHAPPAPI
NTSTATUS
NTAPI
PhGetProcessKnownType(
    _In_ HANDLE ProcessHandle,
    _Out_ PH_KNOWN_PROCESS_TYPE *KnownProcessType
    );

PHAPPAPI
PH_KNOWN_PROCESS_TYPE
NTAPI
PhGetProcessKnownTypeEx(
    _In_opt_ HANDLE ProcessId,
    _In_ PPH_STRING FileName
    );

typedef union _PH_KNOWN_PROCESS_COMMAND_LINE
{
    struct
    {
        PPH_STRING GroupName;
    } ServiceHost;
    struct
    {
        PPH_STRING FileName;
        PPH_STRING ProcedureName;
    } RunDllAsApp;
    struct
    {
        GUID Guid;
        PPH_STRING Name; // optional
        PPH_STRING FileName; // optional
    } ComSurrogate;
} PH_KNOWN_PROCESS_COMMAND_LINE, *PPH_KNOWN_PROCESS_COMMAND_LINE;

PHAPPAPI
_Success_(return)
BOOLEAN
NTAPI
PhaGetProcessKnownCommandLine(
    _In_ PPH_STRING CommandLine,
    _In_ PH_KNOWN_PROCESS_TYPE KnownProcessType,
    _Out_ PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    );
// end_phapppub

PPH_STRING PhEscapeStringForDelimiter(
    _In_ PPH_STRING String,
    _In_ WCHAR Delimiter
    );

PPH_STRING PhUnescapeStringForDelimiter(
    _In_ PPH_STRING String,
    _In_ WCHAR Delimiter
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhSearchOnlineString(
    _In_ HWND WindowHandle,
    _In_ PWSTR String
    );

PHAPPAPI
VOID
NTAPI
PhShellExecuteUserString(
    _In_ HWND WindowHandle,
    _In_ PWSTR Setting,
    _In_ PWSTR String,
    _In_ BOOLEAN UseShellExecute,
    _In_opt_ PWSTR ErrorMessage
    );

PHAPPAPI
VOID
NTAPI
PhLoadSymbolProviderOptions(
    _Inout_ PPH_SYMBOL_PROVIDER SymbolProvider
    );

PHAPPAPI
VOID
NTAPI
PhCopyListViewInfoTip(
    _Inout_ LPNMLVGETINFOTIP GetInfoTip,
    _In_ PPH_STRINGREF Tip
    );

PHAPPAPI
VOID
NTAPI
PhCopyListView(
    _In_ HWND ListViewHandle
    );

PHAPPAPI
VOID
NTAPI
PhHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    );

#define PH_LIST_VIEW_CTRL_C_BEHAVIOR 0x1
#define PH_LIST_VIEW_CTRL_A_BEHAVIOR 0x2
#define PH_LIST_VIEW_DEFAULT_1_BEHAVIORS (PH_LIST_VIEW_CTRL_C_BEHAVIOR | PH_LIST_VIEW_CTRL_A_BEHAVIOR)

PHAPPAPI
VOID
NTAPI
PhHandleListViewNotifyBehaviors(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle,
    _In_ ULONG Behaviors
    );

PHAPPAPI
BOOLEAN
NTAPI
PhGetListViewContextMenuPoint(
    _In_ HWND ListViewHandle,
    _Out_ PPOINT Point
    );
// end_phapppub

VOID PhSetWindowOpacity(
    _In_ HWND WindowHandle,
    _In_ ULONG OpacityPercent
    );

#define PH_OPACITY_TO_ID(Opacity) (ID_OPACITY_10 + (10 - (Opacity) / 10) - 1)
#define PH_ID_TO_OPACITY(Id) (100 - (((Id) - ID_OPACITY_10) + 1) * 10)

// begin_phapppub
PHAPPAPI
PPH_STRING
NTAPI
PhGetPhVersion(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetPhVersionNumbers(
    _Out_opt_ PULONG MajorVersion,
    _Out_opt_ PULONG MinorVersion,
    _Out_opt_ PULONG BuildNumber,
    _Out_opt_ PULONG RevisionNumber
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetPhVersionHash(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhWritePhTextHeader(
    _Inout_ PPH_FILE_STREAM FileStream
    );

#define PH_SHELL_APP_PROPAGATE_PARAMETERS 0x1
#define PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY 0x2

PHAPPAPI
BOOLEAN
NTAPI
PhShellProcessHacker(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_ ULONG AppFlags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    );
// end_phapppub

BOOLEAN PhShellProcessHackerEx(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_ ULONG AppFlags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    );

BOOLEAN PhCreateProcessIgnoreIfeoDebugger(
    _In_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine
    );

// begin_phapppub
typedef struct _PH_EMENU_ITEM* PPH_EMENU_ITEM;

typedef struct _PH_TN_COLUMN_MENU_DATA
{
    HWND TreeNewHandle;
    PPH_TREENEW_HEADER_MOUSE_EVENT MouseEvent;
    ULONG DefaultSortColumn;
    PH_SORT_ORDER DefaultSortOrder;

    PPH_EMENU_ITEM Menu;
    PPH_EMENU_ITEM Selection;
    ULONG ProcessedId;
} PH_TN_COLUMN_MENU_DATA, *PPH_TN_COLUMN_MENU_DATA;

#define PH_TN_COLUMN_MENU_HIDE_COLUMN_ID ((ULONG)-1)
#define PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID ((ULONG)-2)
#define PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID ((ULONG)-3)
#define PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID ((ULONG)-4)
#define PH_TN_COLUMN_MENU_RESET_SORT_ID ((ULONG)-5)

PHAPPAPI
VOID
NTAPI
PhInitializeTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    );

#define PH_TN_COLUMN_MENU_NO_VISIBILITY 0x1
#define PH_TN_COLUMN_MENU_SHOW_RESET_SORT 0x2

PHAPPAPI
VOID
NTAPI
PhInitializeTreeNewColumnMenuEx(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data,
    _In_ ULONG Flags
    );

PHAPPAPI
BOOLEAN
NTAPI
PhHandleTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    );

PHAPPAPI
VOID
NTAPI
PhDeleteTreeNewColumnMenu(
    _In_ PPH_TN_COLUMN_MENU_DATA Data
    );

typedef struct _PH_TN_FILTER_SUPPORT
{
    PPH_LIST FilterList;
    HWND TreeNewHandle;
    PPH_LIST NodeList;
} PH_TN_FILTER_SUPPORT, *PPH_TN_FILTER_SUPPORT;

_Function_class_(PH_TN_FILTER_FUNCTION)
typedef BOOLEAN (NTAPI PH_TN_FILTER_FUNCTION)(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );
typedef PH_TN_FILTER_FUNCTION *PPH_TN_FILTER_FUNCTION;

typedef struct _PH_TN_FILTER_ENTRY
{
    PPH_TN_FILTER_FUNCTION Filter;
    PVOID Context;
} PH_TN_FILTER_ENTRY, *PPH_TN_FILTER_ENTRY;

PHAPPAPI
VOID
NTAPI
PhInitializeTreeNewFilterSupport(
    _Out_ PPH_TN_FILTER_SUPPORT Support,
    _In_ HWND TreeNewHandle,
    _In_ PPH_LIST NodeList
    );

PHAPPAPI
VOID
NTAPI
PhDeleteTreeNewFilterSupport(
    _In_ PPH_TN_FILTER_SUPPORT Support
    );

PHAPPAPI
PPH_TN_FILTER_ENTRY
NTAPI
PhAddTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_FUNCTION Filter,
    _In_opt_ PVOID Context
    );

PHAPPAPI
VOID
NTAPI
PhRemoveTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_ENTRY Entry
    );

PHAPPAPI
BOOLEAN
NTAPI
PhApplyTreeNewFiltersToNode(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TREENEW_NODE Node
    );

PHAPPAPI
VOID
NTAPI
PhApplyTreeNewFilters(
    _In_ PPH_TN_FILTER_SUPPORT Support
    );

typedef struct _PH_COPY_CELL_CONTEXT
{
    HWND TreeNewHandle;
    ULONG Id; // column ID
    PPH_STRING MenuItemText;
} PH_COPY_CELL_CONTEXT, *PPH_COPY_CELL_CONTEXT;

PHAPPAPI
BOOLEAN
NTAPI
PhInsertCopyCellEMenuItem(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND TreeNewHandle,
    _In_ PPH_TREENEW_COLUMN Column
    );

PHAPPAPI
BOOLEAN
NTAPI
PhHandleCopyCellEMenuItem(
    _In_ PPH_EMENU_ITEM SelectedItem
    );

typedef struct _PH_COPY_ITEM_CONTEXT
{
    HWND ListViewHandle;
    ULONG Id;
    ULONG SubId;
    PPH_STRING MenuItemText;
} PH_COPY_ITEM_CONTEXT, *PPH_COPY_ITEM_CONTEXT;

PHAPPAPI
BOOLEAN
NTAPI
PhInsertCopyListViewEMenuItem(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND ListViewHandle
    );

PHAPPAPI
BOOLEAN
NTAPI
PhHandleCopyListViewEMenuItem(
    _In_ PPH_EMENU_ITEM SelectedItem
    );

PHAPPAPI
VOID
NTAPI
PhShellOpenKey(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING KeyName
    );

PHAPPAPI
BOOLEAN
NTAPI
PhShellOpenKey2(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING KeyName
    );
// end_phapppub

PPH_STRING PhPcre2GetErrorMessage(
    _In_ INT ErrorCode
    );

// begin_phapppub
PHAPPAPI
HBITMAP
NTAPI
PhGetShieldBitmap(
    _In_ LONG WindowDpi,
    _In_opt_ LONG Width,
    _In_opt_ LONG Height
    );

PHAPPAPI
HICON
NTAPI
PhGetApplicationIcon(
    _In_ BOOLEAN SmallIcon
    );

PHAPPAPI
HICON
NTAPI
PhGetApplicationIconEx(
    _In_ BOOLEAN SmallIcon,
    _In_opt_ LONG WindowDpi
    );

PHAPPAPI
VOID
NTAPI
PhSetApplicationWindowIcon(
    _In_ HWND WindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhSetApplicationWindowIconEx(
    _In_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    );

PHAPPAPI
VOID
NTAPI
PhDeleteApplicationWindowIcon(
    _In_ HWND WindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhSetStaticWindowIcon(
    _In_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    );

PHAPPAPI
VOID
NTAPI
PhDeleteStaticWindowIcon(
    _In_ HWND WindowHandle
    );
// end_phapppub

#define SI_RUNAS_ADMIN_TASK_NAME ((PH_STRINGREF)PH_STRINGREF_INIT(L"SystemInformerTaskAdmin"))

HRESULT PhRunAsAdminTask(
    _In_ PPH_STRINGREF TaskName
    );

HRESULT PhDeleteAdminTask(
    _In_ PPH_STRINGREF TaskName
    );

HRESULT PhCreateAdminTask(
    _In_ PPH_STRINGREF TaskName,
    _In_ PPH_STRINGREF FileName
    );

// begin_phapppub
PHAPPAPI
BOOLEAN
NTAPI
PhWordMatchStringRef(
    _In_ PPH_STRINGREF SearchText,
    _In_ PPH_STRINGREF Text
    );

FORCEINLINE
BOOLEAN
NTAPI
PhWordMatchStringZ(
    _In_ PPH_STRING SearchText,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PhWordMatchStringRef(&SearchText->sr, &text);
}

FORCEINLINE
BOOLEAN
NTAPI
PhWordMatchStringLongHintZ(
    _In_ PPH_STRING SearchText,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRefLongHint(&text, Text);

    return PhWordMatchStringRef(&SearchText->sr, &text);
}

PHAPPAPI
PVOID
NTAPI
PhCreateKsiSettingsBlob( // ksisup.c
    VOID
    );
// end_phapppub

#define PH_LOAD_SHARED_ICON_SMALL(BaseAddress, Name, dpiValue) PhLoadIcon(BaseAddress, (Name), PH_LOAD_ICON_SHARED | PH_LOAD_ICON_SIZE_SMALL, 0, 0, dpiValue) // phapppub
#define PH_LOAD_SHARED_ICON_LARGE(BaseAddress, Name, dpiValue) PhLoadIcon(BaseAddress, (Name), PH_LOAD_ICON_SHARED | PH_LOAD_ICON_SIZE_LARGE, 0, 0, dpiValue) // phapppub

FORCEINLINE PVOID PhpGenericPropertyPageHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ ULONG ContextHash
    )
{
    PVOID context;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

            context = (PVOID)propSheetPage->lParam;
            PhSetWindowContext(hwndDlg, ContextHash, context);
        }
        break;
    case WM_NCDESTROY:
        {
            context = PhGetWindowContext(hwndDlg, ContextHash);
            PhRemoveWindowContext(hwndDlg, ContextHash);
        }
        break;
    default:
        {
            context = PhGetWindowContext(hwndDlg, ContextHash);
        }
        break;
    }

    return context;
}

#define SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER (SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)
#define SWP_SHOWWINDOW_ONLY (SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER | SWP_SHOWWINDOW)
#define SWP_HIDEWINDOW_ONLY (SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER | SWP_HIDEWINDOW)

#endif

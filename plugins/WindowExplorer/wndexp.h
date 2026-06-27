/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2013
 *     dmex    2015-2023
 *
 */

#ifndef WNDEXP_H
#define WNDEXP_H

#define PH_PROPSHEET_NEW 1

#include <phdk.h>
#include <phapppub.h>
#include <phappresource.h>
#include <settings.h>
#include <searchbox.h>
#include <mapldr.h>

#include "resource.h"
#include "prpsh.h"
#include "wndtree.h"

extern PPH_PLUGIN PluginInstance;

#define PLUGIN_NAME L"WindowExplorer"
#define SETTING_NAME_SHOW_DESKTOP_WINDOWS (PLUGIN_NAME L".ShowDesktopWindows")
#define SETTING_NAME_WINDOW_ENUM_ALTERNATE (PLUGIN_NAME L".EnableAltEnumWindow")
#define SETTING_NAME_WINDOW_ENUM_MESSAGEONLY (PLUGIN_NAME L".EnumMessageOnly")
#define SETTING_NAME_WINDOW_ENUM_NONVISIBLE (PLUGIN_NAME L".EnumNonVisible")
#define SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY (PLUGIN_NAME L".HighlightMessageOnly")
#define SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY_COLOR (PLUGIN_NAME L".HighlightMessageOnlyColor")
#define SETTING_NAME_WINDOW_ENABLE_ICONS (PLUGIN_NAME L".EnableIcons")
#define SETTING_NAME_WINDOW_ENABLE_ICONS_INTERNAL (PLUGIN_NAME L".EnableIconsInternal")
#define SETTING_NAME_WINDOW_FIND_SNAPSHOT (PLUGIN_NAME L".FindWindowSnapshot")
#define SETTING_NAME_WINDOW_TREE_LIST_COLUMNS (PLUGIN_NAME L".WindowTreeListColumns")
#define SETTING_NAME_WINDOWS_WINDOW_POSITION (PLUGIN_NAME L".WindowsWindowPosition")
#define SETTING_NAME_WINDOWS_WINDOW_SIZE (PLUGIN_NAME L".WindowsWindowSize")
#define SETTING_NAME_WINDOWS_PROPERTY_COLUMNS (PLUGIN_NAME L".WindowsPropertyColumns")
#define SETTING_NAME_WINDOWS_PROPERTY_POSITION (PLUGIN_NAME L".WindowsPropertyPosition")
#define SETTING_NAME_WINDOWS_PROPERTY_SIZE (PLUGIN_NAME L".WindowsPropertySize")
#define SETTING_NAME_WINDOWS_PROPERTY_PAGE (PLUGIN_NAME L".WindowsPropertyPage")
#define SETTING_NAME_WINDOWS_PROPLIST_COLUMNS (PLUGIN_NAME L".WindowsPropListColumns")
#define SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS (PLUGIN_NAME L".WindowsPropStorageColumns")
#define SETTING_NAME_WINDOWS_DWMATTRIBUTES_COLUMNS (PLUGIN_NAME L".WindowsDwmAttributesColumns")

#define WE_WM_WINDOWS_UPDATED (WM_APP + 301)

typedef struct _WE_WINDOW_PROVIDER *PWE_WINDOW_PROVIDER;

typedef struct _WINDOWS_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchBoxHandle;
    HFONT TreeWindowFont;

    HWND FindWindowButtonHandle;
    HWND PauseResumeButtonHandle;
    WNDPROC FindWindowButtonWindowProc;
    PH_CALLBACK_REGISTRATION WindowNotifyCallbackRegistration;

    PH_CALLBACK_REGISTRATION WindowProviderAddedRegistration;
    PH_CALLBACK_REGISTRATION WindowProviderModifiedRegistration;
    PH_CALLBACK_REGISTRATION WindowProviderRemovedRegistration;
    PH_CALLBACK_REGISTRATION WindowProviderUpdatedRegistration;

    WE_WINDOW_TREE_CONTEXT TreeContext;
    WE_WINDOW_SELECTOR Selector;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;

    BOOLEAN TargetingWindow;
    BOOLEAN TargetingCurrentWindowDraw;
    BOOLEAN TargetingCompleted;
    HWND TargetingCurrentWindow;

    PVOID UIAutomationPtr;

    COLORREF ColorNew;
    COLORREF ColorRemoved;
    ULONG HighlightingDuration;

    BOOLEAN NodesNeedRestructure;
    ULONG WindowProviderRunCount;
    PWE_WINDOW_PROVIDER WindowProvider;
    PH_PROVIDER_THREAD ProviderThread;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_PROVIDER_EVENT_QUEUE EventQueue;

    BOOLEAN ProviderPaused;

} WINDOWS_CONTEXT, *PWINDOWS_CONTEXT;

// wnddlg

VOID WepFillWindowInfo(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE Node
    );

PWE_WINDOW_NODE WepAddChildWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND WindowHandle
    );

VOID WepAddEnumWindowsByViewMode(
    _In_ PWINDOWS_CONTEXT Context
    );

VOID WepInsertWindowNodeByZOrder(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ PWE_WINDOW_NODE WindowNode
    );

VOID WepEnumerateParentChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    );

VOID WepEnumerateZOrderWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    );

VOID WepEnumerateOwnerChainWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    );

VOID WeShowWindowsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWE_WINDOW_SELECTOR Selector
    );

VOID WeShowWindowsPropPage(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    );

// wndprp

typedef struct _WINDOW_PROPERTIES_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND PropertySheetHandle;
    HWND OptionsButtonHandle;
    WNDPROC PropertySheetWindowProcedure;
    HFONT TreeWindowFont;

    HICON WindowIcon;
    ULONG PropsListCount;
    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;

    CLIENT_ID ClientId;
    PH_INITONCE SymbolProviderInitOnce;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    LIST_ENTRY ResolveListHead;
    PH_QUEUED_LOCK ResolveListLock;

    PPH_STRING WndProcSymbol;
    ULONG WndProcResolving;
    PPH_STRING DlgProcSymbol;
    ULONG DlgProcResolving;
    PPH_STRING ClassWndProcSymbol;
    ULONG ClassWndProcResolving;

    ULONG_PTR WndProc;
    ULONG_PTR DlgProc;
    RTL_ATOM ClassAtom;
    WNDCLASSEX ClassInfo;

    LONG PreviewScrollX;
    LONG PreviewScrollY;
    LONG PreviewImageWidth;
    LONG PreviewImageHeight;

    HANDLE DxSharedSurface;
    LUID DxAdapterLuid;
    ULONG DxgiFormat;
    UINT64 DxUpdateId;
    BOOL DxSurfaceValid;

} WINDOW_PROPERTIES_CONTEXT, *PWINDOW_PROPERTIES_CONTEXT;

BOOLEAN WeShowWindowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle
    );

PPH_EMENU WepCreateWindowMenu(
    _In_ PPH_EMENU WindowMenu,
    _In_ BOOLEAN IncludeProperties,
    _In_ BOOLEAN IncludeCopy
    );

VOID WepSetWindowMenuState(
    _In_ PPH_EMENU WindowMenu,
    _In_ HWND WindowHandle
    );

BOOLEAN WepExecuteWindowCommand(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG CommandId
    );

// utils

typedef struct _WINDOWCOMPOSITIONATTRIBUTEDATA WINDOWCOMPOSITIONATTRIBDATA;

typedef DWORD (WINAPI *PFN_GetWindowBand)(
    _In_ HWND WindowHandle
    );

typedef BOOL (WINAPI *PFN_IsShellManagedWindow)(
    _In_ HWND WindowHandle
    );

typedef BOOL (WINAPI *PFN_GetWindowCompositionAttribute)(
    _In_ HWND WindowHandle,
    _Inout_ WINDOWCOMPOSITIONATTRIBDATA *Data
    );

typedef BOOL (WINAPI *PFN_IsShellFrameWindow)(
    _In_ HWND WindowHandle
    );

typedef BOOL (WINAPI *PFN_GetModernAppWindow)(
    _In_ HANDLE ProcessHandle,
    _Out_ HWND *WindowHandle
    );

BOOLEAN WeWindowEndTask(
    _In_ HWND WindowHandle,
    _In_ BOOL Force
    );

VOID WeSetWindowToDpiForTesting(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDPI
    );

NTSTATUS WeDestroyRemoteWindow(
    _In_ HWND TargetWindow,
    _In_ HANDLE ProcessId
    );

BOOL WeIsTopLevelWindow(
    _In_ HWND WindowHandle
    );

BOOL WeIsInDesktopWindowBand(
    _In_ HWND WindowHandle
    );

HICON WeGetInternalWindowIcon(
    _In_ HWND WindowHandle,
    _In_ UINT IconType
    );

HICON WeGetWindowIcon(
    _In_ HWND WindowHandle
    );

PPH_STRING WeHashWindowExtraBytes(
    _In_ HWND WindowHandle
    );

BOOLEAN WeWindowHasAutomationProvider(
    _In_ HWND WindowHandle
    );

BOOLEAN WeIsWindowCloaked(
    _In_ HWND WindowHandle
    );

ULONG WeGetWindowBand(
    _In_ HWND WindowHandle
    );

BOOLEAN WeIsWindowEffectivelyVisible(
    _In_ HWND WindowHandle
    );

VOID WeDrawWindowBorderForTargeting(
    _In_ HWND WindowHandle
    );

VOID WeInvertWindowBorder(
    _In_ HWND hWnd
    );

NTSTATUS CallerIdentityIsHostedWindow(
    _In_ HWND WindowHandle,
    _Out_ PULONG IsHosted
    );

typedef enum _WE_WINDOW_SUBGROUP
{
    WeWindowSubgroupNone = 0,
    WeWindowSubgroupStandard = 4,
    WeWindowSubgroupShell = 8,
    WeWindowSubgroupSystem = 0x400000,
    WeWindowSubgroupUnknown = 0x8000000
} WE_WINDOW_SUBGROUP;

typedef struct _WE_APPLICATION_INFO
{
    PPH_STRING ExistingDisplayName;
    PPH_STRING RelaunchDisplayName;
} WE_APPLICATION_INFO, *PWE_APPLICATION_INFO;

NTSTATUS WeEnumerateWindowTabs(
    _In_ HWND WindowHandle,
    _Out_ PPH_LIST *WindowList
    );

WE_WINDOW_SUBGROUP WeDetermineWindowSubgroup(
    _In_ HWND WindowHandle,
    _Out_ HWND *ResolvedWindow
    );

NTSTATUS WeDetermineDllhostProcessNameFromWindow(
    _In_ HWND WindowHandle,
    _Inout_ PWE_APPLICATION_INFO AppInfo
    );

// wndprv.c

typedef struct _WE_WINDOW_ITEM
{
    HWND WindowHandle;
    HWND ParentHandle;

    PPH_STRING WindowText;

    CLIENT_ID ClientId;

    BOOLEAN WindowVisible;
    BOOLEAN WindowMessageOnly;

    USHORT WindowIndex;
    USHORT WindowGeneration;

    PPH_STRING WindowClassText;
} WE_WINDOW_ITEM, *PWE_WINDOW_ITEM;

typedef struct _WE_WINDOW_PROVIDER
{
    PPH_HASHTABLE WindowHashtable;
    PH_QUEUED_LOCK WindowHashtableLock;

    PH_CALLBACK WindowAddedEvent;
    PH_CALLBACK WindowModifiedEvent;
    PH_CALLBACK WindowRemovedEvent;
    PH_CALLBACK WindowUpdatedEvent;
} WE_WINDOW_PROVIDER, *PWE_WINDOW_PROVIDER;

PWE_WINDOW_PROVIDER WeCreateWindowProvider(
    VOID
    );

VOID WeDeleteWindowProvider(
    _In_ PWE_WINDOW_PROVIDER Provider
    );

PWE_WINDOW_ITEM WeCreateWindowItem(
    _In_ HWND WindowHandle
    );

VOID WeDeleteWindowItem(
    _In_ PWE_WINDOW_ITEM WindowItem
    );

PWE_WINDOW_ITEM WeReferenceWindowItem(
    _In_ PWE_WINDOW_PROVIDER Provider,
    _In_ HWND WindowHandle
    );

VOID WeEnumWindowItems(
    _In_ PWE_WINDOW_PROVIDER Provider,
    _Out_opt_ PWE_WINDOW_ITEM **WindowItems,
    _Out_ PULONG NumberOfWindowItems
    );

_Function_class_(PH_PROVIDER_FUNCTION)
VOID WeWindowProviderUpdate(
    _In_ PVOID Object
    );

#endif

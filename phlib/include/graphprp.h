/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#ifndef _PH_PROPSHEETWINDOWNEW_H
#define _PH_PROPSHEETWINDOWNEW_H

#include <tabnew.h>
#include <guisup.h>

EXTERN_C_START

// Host class name (registered lazily on first show)
#define PH_PROPSHEETNEW_CLASSNAME L"PhPropSheetNew"

// Host flags
#define PH_PROPSHEETNEW_RESIZABLE        0x00000001  // WS_THICKFRAME + min-size enforcement
#define PH_PROPSHEETNEW_MODELESS         0x00000002  // PhPropSheetNewShow returns the host hwnd immediately
#define PH_PROPSHEETNEW_CENTER           0x00000004  // center on parent on first show
#define PH_PROPSHEETNEW_NOICON           0x00000008  // suppress WM_SETICON
#define PH_PROPSHEETNEW_SAVE_PLACEMENT   0x00000010  // honour SettingNamePosition / SettingNameSize
#define PH_PROPSHEETNEW_SAVE_ACTIVE_PAGE 0x00000020  // honour SettingNameActivePage
#define PH_PROPSHEETNEW_CLOSE_BUTTON     0x00000040  // render a "Close" button anchored bottom-right

// Per-page flags
#define PH_PROPSHEETNEW_PAGE_EAGER       0x00000001  // create dialog at startup, not lazily

typedef enum _PH_PROPSHEETNEW_LAYOUT
{
    PhPropSheetNewLayoutTop = 0,   // tab strip on top (default)
    PhPropSheetNewLayoutLeft = 1   // tab strip on left (sidebar)
} PH_PROPSHEETNEW_LAYOUT;

typedef struct _PH_PROPSHEETNEW_PAGE
{
    PCWSTR Id;                     // stable persistence key; Name is used when NULL
    PCWSTR Name;                   // tab label; also used as the persistence key for SAVE_ACTIVE_PAGE
    PVOID Instance;                // HINSTANCE for the dialog template
    PCWSTR Template;               // MAKEINTRESOURCE(IDD_*) or pointer
    DLGPROC DialogProc;
    PVOID Parameter;               // lParam handed to the dialog on WM_INITDIALOG
    ULONG Flags;                   // PH_PROPSHEETNEW_PAGE_*
    HWND DialogHandle;             // out — filled after dialog is created (NULL until then)
} PH_PROPSHEETNEW_PAGE, *PPH_PROPSHEETNEW_PAGE;

typedef VOID (NTAPI *PPH_PROPSHEETNEW_PAGE_DELETE_CALLBACK)(
    _In_opt_ PVOID Context
    );

typedef struct _PH_PROPSHEETNEW_PAGE_DESCRIPTOR
{
    PCWSTR Id;
    PCWSTR Name;
    PVOID Instance;
    PCWSTR Template;
    DLGPROC DialogProc;
    PVOID Context;
    PPH_PROPSHEETNEW_PAGE_DELETE_CALLBACK DeleteContext;
    ULONG Flags;
} PH_PROPSHEETNEW_PAGE_DESCRIPTOR, *PPH_PROPSHEETNEW_PAGE_DESCRIPTOR;

typedef struct _PH_PROPSHEETNEW_BUILDER PH_PROPSHEETNEW_BUILDER, *PPH_PROPSHEETNEW_BUILDER;

// Fired after the host window is created and the initial page has been
// activated, but before the host is shown. Lets callers subclass the host,
// attach extra child controls, register window callbacks, etc.
typedef
VOID
(NTAPI *PPH_PROPSHEETNEW_INITIALIZED_CALLBACK)(
    _In_ HWND HostHandle,
    _In_opt_ PVOID Context
    );

// Fired once per message inside PhPropSheetNewShowModal's loop, before
// IsDialogMessage. Returning TRUE consumes the message. Use this to wire
// global hotkeys (F5 refresh, etc.) that would otherwise be swallowed by
// the focused page child.
typedef
BOOLEAN
(NTAPI *PPH_PROPSHEETNEW_PRETRANSLATE_CALLBACK)(
    _In_ HWND HostHandle,
    _In_ MSG *Message,
    _In_opt_ PVOID Context
    );

typedef struct _PH_PROPSHEETNEW
{
    PCWSTR Caption;
    HWND ParentWindow;
    HICON Icon;
    PH_PROPSHEETNEW_LAYOUT Layout;
    PH_TABNEW_SKIN Skin;
    ULONG Flags;                   // PH_PROPSHEETNEW_*
    SIZE InitialSize;              // {0,0} = auto
    SIZE MinimumSize;              // {0,0} = MapDialogRect({0,0,290,320})
    PCWSTR SettingNamePosition;    // optional, requires PH_PROPSHEETNEW_SAVE_PLACEMENT
    PCWSTR SettingNameSize;        // optional, requires PH_PROPSHEETNEW_SAVE_PLACEMENT
    PCWSTR SettingNameActivePage;  // optional, requires PH_PROPSHEETNEW_SAVE_ACTIVE_PAGE
    ULONG PageCount;
    PPH_PROPSHEETNEW_PAGE Pages;
    PVOID Context;                 // opaque caller context, returned by PhPropSheetNewGetContext
    PPH_PROPSHEETNEW_INITIALIZED_CALLBACK Initialized;        // optional
    PPH_PROPSHEETNEW_PRETRANSLATE_CALLBACK PreTranslateMessage; // optional (modal only)
} PH_PROPSHEETNEW, *PPH_PROPSHEETNEW;

PHLIBAPI
PPH_PROPSHEETNEW_BUILDER
NTAPI
PhPropSheetNewBuilderCreate(
    _In_ PPH_PROPSHEETNEW Sheet
    );

PHLIBAPI
BOOLEAN
NTAPI
PhPropSheetNewBuilderAddPage(
    _Inout_ PPH_PROPSHEETNEW_BUILDER Builder,
    _In_ PPH_PROPSHEETNEW_PAGE_DESCRIPTOR Descriptor
    );

PHLIBAPI
HWND
NTAPI
PhPropSheetNewBuilderShow(
    _Inout_ PPH_PROPSHEETNEW_BUILDER Builder
    );

PHLIBAPI
INT_PTR
NTAPI
PhPropSheetNewBuilderShowModal(
    _Inout_ PPH_PROPSHEETNEW_BUILDER Builder
    );

PHLIBAPI
VOID
NTAPI
PhPropSheetNewBuilderDestroy(
    _In_opt_ PPH_PROPSHEETNEW_BUILDER Builder
    );

#define PH_PROPSHEETNEW_PAGE_LAYOUT_PARENT ((PPH_LAYOUT_ITEM)0x1)

PHLIBAPI
PPH_LAYOUT_ITEM
NTAPI
PhPropSheetNewPageAddLayoutItem(
    _In_ HWND PageWindow,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

PHLIBAPI
PPH_LAYOUT_ITEM
NTAPI
PhPropSheetNewPageAddLayoutItemEx(
    _In_ HWND PageWindow,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_opt_ PVOID Instance,
    _In_opt_ PCWSTR Template
    );

PHLIBAPI
VOID
NTAPI
PhPropSheetNewPageLayout(
    _In_ HWND PageWindow
    );

// Modeless: returns the host hwnd; caller is responsible for the message loop.
PHLIBAPI
HWND
NTAPI
PhPropSheetNewShow(
    _In_ PPH_PROPSHEETNEW Sheet
    );

// Modal: runs its own message loop until the window is destroyed; returns IDOK / IDCANCEL.
PHLIBAPI
INT_PTR
NTAPI
PhPropSheetNewShowModal(
    _In_ PPH_PROPSHEETNEW Sheet
    );

PHLIBAPI
HWND
NTAPI
PhPropSheetNewGetCurrentPage(
    _In_ HWND HostHandle
    );

PHLIBAPI
HWND
NTAPI
PhPropSheetNewGetTabControl(
    _In_ HWND HostHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhPropSheetNewSetCurrentPageByName(
    _In_ HWND HostHandle,
    _In_ PCWSTR Name
    );

PHLIBAPI
PVOID
NTAPI
PhPropSheetNewGetContext(
    _In_ HWND HostHandle
    );

EXTERN_C_END

#endif

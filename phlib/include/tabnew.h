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

#ifndef _PH_TABNEW_H
#define _PH_TABNEW_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_TABNEW_CLASSNAME L"PhTabNew"

PHLIBAPI
RTL_ATOM
NTAPI
PhTabNewInitialization(
    VOID
    );

// Style flags (passed in dwStyle alongside WS_*)
#define TNS_TOP           0x00000000  // tab strip on top (default)
#define TNS_BOTTOM        0x00000001  // tab strip on bottom
#define TNS_LEFT          0x00000002  // tab strip on left
#define TNS_RIGHT         0x00000003  // tab strip on right
#define TNS_SIDE_MASK     0x00000003
#define TNS_MULTILINE     0x00000004  // wrap tabs when strip overflows
#define TNS_FIXEDWIDTH    0x00000010  // all tabs share equal width
#define TNS_REORDER       0x00000020  // allow Ctrl+drag tab reordering
#define TNS_RIGHTALIGN    0x00000040  // right-align row 0 (default = left)
#define TNS_DRAW_PANEL    0x00000080  // send PHTNN_DRAWPANEL during paint

// Visual skin
typedef enum _PH_TABNEW_SKIN
{
    PhTabNewSkinWin7 = 0,    // Aero-style replica (custom-painted)
    PhTabNewSkinWin10 = 1,   // flat with accent underline
    PhTabNewSkinUxTheme = 2  // native OpenThemeData(L"Tab") (default)
} PH_TABNEW_SKIN;

// Hit-test flags
#define TNHT_NOWHERE   0x0001
#define TNHT_ONITEM    0x0002
#define TNHT_ONLABEL   0x0004
#define TNHT_ONICON    0x0008

typedef struct _PH_TABNEW_HITTESTINFO
{
    POINT Point;        // in
    ULONG Flags;        // out
    INT ItemIndex;      // out, -1 if not over an item
} PH_TABNEW_HITTESTINFO, *PPH_TABNEW_HITTESTINFO;

// Item insertion
typedef struct _PH_TABNEW_INSERTITEM
{
    PWSTR Text;
    INT ImageIndex;
    LPARAM Param;
} PH_TABNEW_INSERTITEM, *PPH_TABNEW_INSERTITEM;

typedef _Function_class_(PH_TABNEW_LAYOUT_CALLBACK)
BOOLEAN NTAPI PH_TABNEW_LAYOUT_CALLBACK(
    _In_ HWND WindowHandle,
    _In_ LPARAM ItemParam,
    _Out_ PPH_STRINGREF Identifier,
    _In_opt_ PVOID Context
    );
typedef PH_TABNEW_LAYOUT_CALLBACK *PPH_TABNEW_LAYOUT_CALLBACK;

// Messages
#define PHTNM_FIRST              (WM_USER + 0x600)
#define PHTNM_INSERTITEM         (PHTNM_FIRST + 1)   // wParam=index, lParam=PPH_TABNEW_INSERTITEM
#define PHTNM_DELETEITEM         (PHTNM_FIRST + 2)   // wParam=index
#define PHTNM_DELETEALLITEMS     (PHTNM_FIRST + 3)
#define PHTNM_GETITEMCOUNT       (PHTNM_FIRST + 4)
#define PHTNM_SETCURSEL          (PHTNM_FIRST + 5)   // wParam=index
#define PHTNM_GETCURSEL          (PHTNM_FIRST + 6)
#define PHTNM_GETITEMTEXT        (PHTNM_FIRST + 7)   // wParam=index, lParam=PPH_STRINGREF (out)
#define PHTNM_SETITEMTEXT        (PHTNM_FIRST + 8)   // wParam=index, lParam=PWSTR
#define PHTNM_GETITEMPARAM       (PHTNM_FIRST + 9)   // wParam=index, returns LPARAM
#define PHTNM_SETITEMPARAM       (PHTNM_FIRST + 10)  // wParam=index, lParam=LPARAM
#define PHTNM_GETPAGERECT        (PHTNM_FIRST + 11)  // lParam=PRECT (in parent client coords)
#define PHTNM_SETSKIN            (PHTNM_FIRST + 12)  // wParam=PH_TABNEW_SKIN
#define PHTNM_GETSKIN            (PHTNM_FIRST + 13)
#define PHTNM_SETSIDE            (PHTNM_FIRST + 14)  // wParam=TNS_TOP/BOTTOM/LEFT/RIGHT
#define PHTNM_GETSIDE            (PHTNM_FIRST + 15)
#define PHTNM_HITTEST            (PHTNM_FIRST + 16)  // lParam=PPH_TABNEW_HITTESTINFO
#define PHTNM_MOVEITEM           (PHTNM_FIRST + 17)  // wParam=from, lParam=to
#define PHTNM_SETIMAGELIST       (PHTNM_FIRST + 18)  // wParam=HIMAGELIST
#define PHTNM_GETIMAGELIST       (PHTNM_FIRST + 19)
#define PHTNM_SETPADDING         (PHTNM_FIRST + 20)  // wParam=cx, lParam=cy
#define PHTNM_SETMINTABWIDTH     (PHTNM_FIRST + 21)  // wParam=px
#define PHTNM_GETROWCOUNT        (PHTNM_FIRST + 22)
#define PHTNM_INVALIDATEITEM     (PHTNM_FIRST + 23)  // wParam=index
#define PHTNM_SETTHEMEDARK       (PHTNM_FIRST + 24)  // wParam=BOOLEAN
#define PHTNM_SETCALLBACK        (PHTNM_FIRST + 25)  // wParam=PPH_TABNEW_MESSAGE_CALLBACK, lParam=context
#define PHTNM_SAVELAYOUT         (PHTNM_FIRST + 26)  // lParam=PPH_TABNEW_LAYOUT_MESSAGE
#define PHTNM_RESTORELAYOUT      (PHTNM_FIRST + 27)  // lParam=PPH_TABNEW_LAYOUT_MESSAGE
#define PHTNM_ADDPAGE            (PHTNM_FIRST + 28)  // lParam=PPH_TABNEW_ADD_PAGE_MESSAGE
#define PHTNM_FINDPAGE           (PHTNM_FIRST + 29)  // lParam=PPH_STRINGREF
#define PHTNM_GETPAGEBYINDEX     (PHTNM_FIRST + 30)  // wParam=index
#define PHTNM_NOTIFYPAGES        (PHTNM_FIRST + 31)  // lParam=PPH_TABNEW_NOTIFY_PAGES_MESSAGE
#define PHTNM_GETCURRENTPAGE     (PHTNM_FIRST + 32)
#define PHTNM_LAST               PHTNM_GETCURRENTPAGE

#if defined(_PHLIB_)

EXTERN_C LRESULT PhTabNewSendMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    );

#else
#define PhTabNewSendMessage SendMessage
#endif

// Convenience wrappers
#define PhTabNew_InsertItem(hwnd, idx, item) \
    ((INT)PhTabNewSendMessage((hwnd), PHTNM_INSERTITEM, (WPARAM)(idx), (LPARAM)(item)))
#define PhTabNew_DeleteItem(hwnd, idx) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_DELETEITEM, (WPARAM)(idx), 0))
#define PhTabNew_DeleteAllItems(hwnd) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_DELETEALLITEMS, 0, 0))
#define PhTabNew_GetItemCount(hwnd) \
    ((INT)PhTabNewSendMessage((hwnd), PHTNM_GETITEMCOUNT, 0, 0))
#define PhTabNew_SetCurSel(hwnd, idx) \
    ((INT)PhTabNewSendMessage((hwnd), PHTNM_SETCURSEL, (WPARAM)(idx), 0))
#define PhTabNew_GetCurSel(hwnd) \
    ((INT)PhTabNewSendMessage((hwnd), PHTNM_GETCURSEL, 0, 0))
#define PhTabNew_SetItemText(hwnd, idx, text) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_SETITEMTEXT, (WPARAM)(idx), (LPARAM)(text)))
#define PhTabNew_GetItemParam(hwnd, idx) \
    ((LPARAM)PhTabNewSendMessage((hwnd), PHTNM_GETITEMPARAM, (WPARAM)(idx), 0))
#define PhTabNew_SetItemParam(hwnd, idx, param) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_SETITEMPARAM, (WPARAM)(idx), (LPARAM)(param)))
#define PhTabNew_GetPageRect(hwnd, prect) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_GETPAGERECT, 0, (LPARAM)(prect)))
#define PhTabNew_SetSkin(hwnd, skin) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_SETSKIN, (WPARAM)(skin), 0))
#define PhTabNew_GetSkin(hwnd) \
    ((PH_TABNEW_SKIN)PhTabNewSendMessage((hwnd), PHTNM_GETSKIN, 0, 0))
#define PhTabNew_SetSide(hwnd, side) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_SETSIDE, (WPARAM)(side), 0))
#define PhTabNew_GetSide(hwnd) \
    ((ULONG)PhTabNewSendMessage((hwnd), PHTNM_GETSIDE, 0, 0))
#define PhTabNew_HitTest(hwnd, info) \
    ((INT)PhTabNewSendMessage((hwnd), PHTNM_HITTEST, 0, (LPARAM)(info)))
#define PhTabNew_MoveItem(hwnd, from, to) \
    ((BOOL)PhTabNewSendMessage((hwnd), PHTNM_MOVEITEM, (WPARAM)(from), (LPARAM)(to)))
#define PhTabNew_SetImageList(hwnd, himl) \
    ((HIMAGELIST)PhTabNewSendMessage((hwnd), PHTNM_SETIMAGELIST, (WPARAM)(himl), 0))
#define PhTabNew_GetRowCount(hwnd) \
    ((INT)PhTabNewSendMessage((hwnd), PHTNM_GETROWCOUNT, 0, 0))
#define PhTabNew_InvalidateItem(hwnd, idx) \
    ((VOID)PhTabNewSendMessage((hwnd), PHTNM_INVALIDATEITEM, (WPARAM)(idx), 0))
#define PhTabNew_SetThemeDark(hwnd, dark) \
    ((VOID)PhTabNewSendMessage((hwnd), PHTNM_SETTHEMEDARK, (WPARAM)(dark), 0))
#define PhTabNew_SetCallback(hwnd, callback, context) \
    ((VOID)PhTabNewSendMessage((hwnd), PHTNM_SETCALLBACK, (WPARAM)(callback), (LPARAM)(context)))

PHLIBAPI
PPH_STRING
NTAPI
PhTabNewSaveLayout(
    _In_ HWND TabControl,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
BOOLEAN
NTAPI
PhTabNewRestoreLayout(
    _In_ HWND TabControl,
    _In_ PPH_STRINGREF Layout,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

// Notifications via WM_NOTIFY -> NMHDR.code
#define PHTNN_FIRST          (0U - 1100U)
#define PHTNN_SELCHANGING    (PHTNN_FIRST - 1)
#define PHTNN_SELCHANGED     (PHTNN_FIRST - 2)
#define PHTNN_LAYOUT         (PHTNN_FIRST - 4)
#define PHTNN_REORDERED      (PHTNN_FIRST - 5)
#define PHTNN_RCLICK         (PHTNN_FIRST - 6)
#define PHTNN_DRAWPANEL      (PHTNN_FIRST - 7)

typedef _Function_class_(PH_TABNEW_MESSAGE_CALLBACK)
BOOLEAN NTAPI PH_TABNEW_MESSAGE_CALLBACK(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );
typedef PH_TABNEW_MESSAGE_CALLBACK *PPH_TABNEW_MESSAGE_CALLBACK;

typedef struct _PH_TABNEW_CREATEPARAMS
{
    ULONG Size;
    ULONG Flags;
    PPH_TABNEW_MESSAGE_CALLBACK Callback;
    PVOID Context;
    // Add new fields here.
} PH_TABNEW_CREATEPARAMS, *PPH_TABNEW_CREATEPARAMS;

typedef struct _NMTABNEW
{
    NMHDR Header;
    INT ItemIndex;
    LPARAM ItemParam;
} NMTABNEW, *PNMTABNEW;

typedef struct _NMTABNEWREORDER
{
    NMHDR Header;
    INT FromIndex;
    INT ToIndex;
} NMTABNEWREORDER, *PNMTABNEWREORDER;

typedef struct _NMTABNEWLAYOUT
{
    NMHDR Header;
    RECT PageRect;       // in parent client coords
} NMTABNEWLAYOUT, *PNMTABNEWLAYOUT;

typedef struct _NMTABNEWDRAWPANEL
{
    NMHDR Header;
    HDC hdc;
    RECT Rect;
} NMTABNEWDRAWPANEL, *PNMTABNEWDRAWPANEL;

// -------------------------------------------------------------------------
// Page helper layer (optional, mirrors the PhMwp* main-window pattern)
// -------------------------------------------------------------------------

typedef enum _PH_TABNEW_PAGE_MESSAGE
{
    PhTabNewPageCreate,
    PhTabNewPageDestroy,
    PhTabNewPageCreateWindow,    // Parameter1=PHWND (out)
    PhTabNewPageSelected,
    PhTabNewPageLoadSettings,
    PhTabNewPageSaveSettings,
    PhTabNewPageFontChanged,
    PhTabNewPageDpiChanged,
    PhTabNewPageMaximum
} PH_TABNEW_PAGE_MESSAGE;

typedef struct _PH_TABNEW_PAGE PH_TABNEW_PAGE, *PPH_TABNEW_PAGE;

typedef _Function_class_(PH_TABNEW_PAGE_CALLBACK)
BOOLEAN NTAPI PH_TABNEW_PAGE_CALLBACK(
    _In_ PPH_TABNEW_PAGE Page,
    _In_ PH_TABNEW_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );
typedef PH_TABNEW_PAGE_CALLBACK *PPH_TABNEW_PAGE_CALLBACK;

typedef struct _PH_TABNEW_PAGE
{
    PH_STRINGREF Name;
    ULONG Flags;
    PPH_TABNEW_PAGE_CALLBACK Callback;
    PVOID Context;

    INT Index;
    HWND TabControl;
    HWND WindowHandle;     // lazily created
    union
    {
        ULONG StateFlags;
        struct
        {
            ULONG Selected : 1;
            ULONG CreateWindowCalled : 1;
            ULONG Spare : 30;
        };
    };
} PH_TABNEW_PAGE, *PPH_TABNEW_PAGE;

PHLIBAPI
PPH_TABNEW_PAGE
NTAPI
PhTabNewAddPage(
    _In_ HWND TabControl,
    _In_ PPH_STRINGREF Name,
    _In_ ULONG Flags,
    _In_ PPH_TABNEW_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
PPH_TABNEW_PAGE
NTAPI
PhTabNewFindPage(
    _In_ HWND TabControl,
    _In_ PPH_STRINGREF Name
    );

PHLIBAPI
PPH_TABNEW_PAGE
NTAPI
PhTabNewGetPageByIndex(
    _In_ HWND TabControl,
    _In_ LONG Index
    );

PHLIBAPI
VOID
NTAPI
PhTabNewSelectPage(
    _In_ HWND TabControl,
    _In_ PPH_TABNEW_PAGE Page
    );

PHLIBAPI
VOID
NTAPI
PhTabNewNotifyAllPages(
    _In_ HWND TabControl,
    _In_ PH_TABNEW_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

PHLIBAPI
PPH_TABNEW_PAGE
NTAPI
PhTabNewGetCurrentPage(
    _In_ HWND TabControl
    );

// -------------------------------------------------------------------------
// PropertySheet replacement
// -------------------------------------------------------------------------

typedef enum _PH_TABNEW_PROPSHEET_LAYOUT
{
    PhTabNewPropSheetClassic = 0,   // top tabs + bottom button row
    PhTabNewPropSheetSideBar = 1    // left tabs, no buttons (peview style)
} PH_TABNEW_PROPSHEET_LAYOUT;

// Page flags
#define PHTNPSF_PREMATURECREATE   0x0001   // create dialog up-front (not lazy)
#define PHTNPSF_DISABLED          0x0002

typedef struct _PH_TABNEW_PROPSHEET_PAGE
{
    PWSTR Name;
    PVOID Instance;
    PWSTR Template;          // MAKEINTRESOURCE or pointer
    DLGPROC DialogProc;
    PVOID Parameter;
    ULONG Flags;
    HWND DialogHandle;       // out — filled by the propsheet after creation
} PH_TABNEW_PROPSHEET_PAGE, *PPH_TABNEW_PROPSHEET_PAGE;

// Sheet flags
#define PHTNPS_RESIZABLE       0x00000001
#define PHTNPS_MODELESS        0x00000002
#define PHTNPS_NOAPPLY         0x00000004
#define PHTNPS_CENTER          0x00000008
#define PHTNPS_NOICON          0x00000010

typedef struct _PH_TABNEW_PROPSHEET
{
    PWSTR Caption;
    HWND ParentWindow;
    HICON Icon;
    PH_TABNEW_PROPSHEET_LAYOUT Layout;
    PH_TABNEW_SKIN Skin;
    ULONG Flags;
    SIZE InitialSize;        // 0,0 = auto (sum of largest template + chrome)
    SIZE MinimumSize;        // 0,0 = auto
    PWSTR SettingNameSize;
    PWSTR SettingNamePosition;
    ULONG PageCount;
    PPH_TABNEW_PROPSHEET_PAGE Pages;
    PVOID Context;
} PH_TABNEW_PROPSHEET, *PPH_TABNEW_PROPSHEET;

PHLIBAPI
INT_PTR
NTAPI
PhTabNewPropertySheet(
    _In_ PPH_TABNEW_PROPSHEET Sheet
    );

PHLIBAPI
HWND
NTAPI
PhTabNewPropertySheetModeless(
    _In_ PPH_TABNEW_PROPSHEET Sheet
    );

PHLIBAPI
HWND
NTAPI
PhTabNewPropertySheetGetCurrentPage(
    _In_ HWND SheetHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhTabNewPropertySheetSetCurrentPageByName(
    _In_ HWND SheetHandle,
    _In_ PWSTR Name
    );

#ifdef __cplusplus
}
#endif

#endif

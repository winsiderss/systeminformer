#ifndef _PH_PHGUI_H
#define _PH_PHGUI_H

#pragma once

#include <ph.h>
#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

// guisup

typedef BOOL (WINAPI *_ChangeWindowMessageFilter)(
    _In_ UINT message,
    _In_ DWORD dwFlag
    );

typedef BOOL (WINAPI *_IsImmersiveProcess)(
    _In_ HANDLE hProcess
    );

#define RFF_NOBROWSE 0x0001
#define RFF_NODEFAULT 0x0002
#define RFF_CALCDIRECTORY 0x0004
#define RFF_NOLABEL 0x0008
#define RFF_NOSEPARATEMEM 0x0020

#define RFN_VALIDATE (-510)

typedef struct _NMRUNFILEDLGW
{
    NMHDR hdr;
    LPCWSTR lpszFile;
    LPCWSTR lpszDirectory;
    UINT nShow;
} NMRUNFILEDLGW, *LPNMRUNFILEDLGW, *PNMRUNFILEDLGW;

typedef NMRUNFILEDLGW NMRUNFILEDLG;
typedef PNMRUNFILEDLGW PNMRUNFILEDLG;
typedef LPNMRUNFILEDLGW LPNMRUNFILEDLG;

#define RF_OK 0x0000
#define RF_CANCEL 0x0001
#define RF_RETRY 0x0002

typedef HANDLE HTHEME;

typedef BOOL (WINAPI *_RunFileDlg)(
    _In_ HWND hwndOwner,
    _In_opt_ HICON hIcon,
    _In_opt_ LPCWSTR lpszDirectory,
    _In_opt_ LPCWSTR lpszTitle,
    _In_opt_ LPCWSTR lpszDescription,
    _In_ ULONG uFlags
    );

typedef BOOL (WINAPI *_IsThemeActive)(
    VOID
    );

typedef HTHEME (WINAPI *_OpenThemeData)(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszClassList
    );

typedef HRESULT (WINAPI *_CloseThemeData)(
    _In_ HTHEME hTheme
    );

typedef BOOL (WINAPI *_IsThemePartDefined)(
    _In_ HTHEME hTheme,
    _In_ int iPartId,
    _In_ int iStateId
    );

typedef HRESULT (WINAPI *_DrawThemeBackground)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ int iPartId,
    _In_ int iStateId,
    _In_ const RECT *pRect,
    _In_ const RECT *pClipRect
    );

typedef HRESULT (WINAPI *_DrawThemeText)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ int iPartId,
    _In_ int iStateId,
    _In_reads_(cchText) LPCWSTR pszText,
    _In_ int cchText,
    _In_ DWORD dwTextFlags,
    _Reserved_ DWORD dwTextFlags2,
    _In_ LPCRECT pRect
    );

typedef HRESULT (WINAPI *_GetThemeInt)(
    _In_ HTHEME hTheme,
    _In_ int iPartId,
    _In_ int iStateId,
    _In_ int iPropId,
    _Out_ int *piVal
    );

typedef HRESULT (WINAPI *_SHAutoComplete)(
    _In_ HWND hwndEdit,
    _In_ DWORD dwFlags
    );

typedef HRESULT (WINAPI *_TaskDialogIndirect)(
    _In_ const TASKDIALOGCONFIG *pTaskConfig,
    _In_ int *pnButton,
    _In_ int *pnRadioButton,
    _In_ BOOL *pfVerificationFlagChecked
    );

extern _ChangeWindowMessageFilter ChangeWindowMessageFilter_I;
extern _IsImmersiveProcess IsImmersiveProcess_I;
extern _RunFileDlg RunFileDlg;
extern _IsThemeActive IsThemeActive_I;
extern _OpenThemeData OpenThemeData_I;
extern _CloseThemeData CloseThemeData_I;
extern _IsThemePartDefined IsThemePartDefined_I;
extern _DrawThemeBackground DrawThemeBackground_I;
extern _DrawThemeText DrawThemeText_I;
extern _GetThemeInt GetThemeInt_I;
extern _SHAutoComplete SHAutoComplete_I;
extern _TaskDialogIndirect TaskDialogIndirect_I;

VOID PhGuiSupportInitialization(
    VOID
    );

PHLIBAPI
VOID PhSetControlTheme(
    _In_ HWND Handle,
    _In_ PWSTR Theme
    );

FORCEINLINE VOID PhSetWindowStyle(
    _In_ HWND Handle,
    _In_ LONG_PTR Mask,
    _In_ LONG_PTR Value
    )
{
    LONG_PTR style;

    style = GetWindowLongPtr(Handle, GWL_STYLE);
    style = (style & ~Mask) | (Value & Mask);
    SetWindowLongPtr(Handle, GWL_STYLE, style);
}

FORCEINLINE VOID PhSetWindowExStyle(
    _In_ HWND Handle,
    _In_ LONG_PTR Mask,
    _In_ LONG_PTR Value
    )
{
    LONG_PTR style;

    style = GetWindowLongPtr(Handle, GWL_EXSTYLE);
    style = (style & ~Mask) | (Value & Mask);
    SetWindowLongPtr(Handle, GWL_EXSTYLE, style);
}

#ifndef WM_REFLECT
#define WM_REFLECT 0x2000
#endif

#define REFLECT_MESSAGE(hwnd, msg, wParam, lParam) \
    { \
        LRESULT result_ = PhReflectMessage(hwnd, msg, wParam, lParam); \
        \
        if (result_) \
            return result_; \
    }

#define REFLECT_MESSAGE_DLG(hwndDlg, hwnd, msg, wParam, lParam) \
    { \
        LRESULT result_ = PhReflectMessage(hwnd, msg, wParam, lParam); \
        \
        if (result_) \
        { \
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, result_); \
            return TRUE; \
        } \
    }

FORCEINLINE LRESULT PhReflectMessage(
    _In_ HWND Handle,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    if (Message == WM_NOTIFY)
    {
        LPNMHDR header = (LPNMHDR)lParam;

        if (header->hwndFrom == Handle)
            return SendMessage(Handle, WM_REFLECT + Message, wParam, lParam);
    }

    return 0;
}

#define PH_DEFINE_MAKE_ATOM(AtomName) \
do { \
    static UNICODE_STRING atomName = RTL_CONSTANT_STRING(AtomName); \
    static PH_INITONCE initOnce = PH_INITONCE_INIT; \
    static RTL_ATOM atom = 0; \
\
    if (PhBeginInitOnce(&initOnce)) \
    { \
        NtAddAtom(atomName.Buffer, atomName.Length, &atom); \
        PhEndInitOnce(&initOnce); \
    } \
\
    if (atom) \
        return (PWSTR)(ULONG_PTR)atom; \
    else \
        return atomName.Buffer; \
} while (0)

FORCEINLINE VOID PhSetListViewStyle(
    _In_ HWND Handle,
    _In_ BOOLEAN AllowDragDrop,
    _In_ BOOLEAN ShowLabelTips
    )
{
    ULONG style;

    style = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;

    if (AllowDragDrop)
        style |= LVS_EX_HEADERDRAGDROP;
    if (ShowLabelTips)
        style |= LVS_EX_LABELTIP;

    ListView_SetExtendedListViewStyleEx(
        Handle,
        style,
        -1
        );
}

PHLIBAPI
INT PhAddListViewColumn(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT DisplayIndex,
    _In_ INT SubItemIndex,
    _In_ INT Format,
    _In_ INT Width,
    _In_ PWSTR Text
    );

PHLIBAPI
INT PhAddListViewItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
INT PhFindListViewItemByFlags(
    _In_ HWND ListViewHandle,
    _In_ INT StartIndex,
    _In_ ULONG Flags
    );

PHLIBAPI
INT PhFindListViewItemByParam(
    _In_ HWND ListViewHandle,
    _In_ INT StartIndex,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LOGICAL PhGetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _Out_ PINT ImageIndex
    );

PHLIBAPI
LOGICAL PhGetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _Out_ PVOID *Param
    );

PHLIBAPI
VOID PhRemoveListViewItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index
    );

PHLIBAPI
VOID PhSetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT ImageIndex
    );

PHLIBAPI
VOID PhSetListViewSubItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT SubItemIndex,
    _In_ PWSTR Text
    );

PHLIBAPI
BOOLEAN PhLoadListViewColumnSettings(
    _In_ HWND ListViewHandle,
    _In_ PPH_STRING Settings
    );

PHLIBAPI
PPH_STRING PhSaveListViewColumnSettings(
    _In_ HWND ListViewHandle
    );

PHLIBAPI
INT PhAddTabControlTab(
    _In_ HWND TabControlHandle,
    _In_ INT Index,
    _In_ PWSTR Text
    );

#define PhaGetDlgItemText(hwndDlg, id) \
    ((PPH_STRING)PhAutoDereferenceObject(PhGetWindowText(GetDlgItem(hwndDlg, id))))

PHLIBAPI
PPH_STRING PhGetWindowText(
    _In_ HWND hwnd
    );

#define PH_GET_WINDOW_TEXT_INTERNAL 0x1
#define PH_GET_WINDOW_TEXT_LENGTH_ONLY 0x2

PHLIBAPI
ULONG PhGetWindowTextEx(
    _In_ HWND hwnd,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *Text
    );

PHLIBAPI
VOID PhAddComboBoxStrings(
    _In_ HWND hWnd,
    _In_ PWSTR *Strings,
    _In_ ULONG NumberOfStrings
    );

PHLIBAPI
PPH_STRING PhGetComboBoxString(
    _In_ HWND hwnd,
    _In_ INT Index
    );

PHLIBAPI
INT PhSelectComboBoxString(
    _In_ HWND hwnd,
    _In_ PWSTR String,
    _In_ BOOLEAN Partial
    );

PHLIBAPI
PPH_STRING PhGetListBoxString(
    _In_ HWND hwnd,
    _In_ INT Index
    );

PHLIBAPI
VOID PhSetStateAllListViewItems(
    _In_ HWND hWnd,
    _In_ ULONG State,
    _In_ ULONG Mask
    );

PHLIBAPI
PVOID PhGetSelectedListViewItemParam(
    _In_ HWND hWnd
    );

PHLIBAPI
VOID PhGetSelectedListViewItemParams(
    _In_ HWND hWnd,
    _Out_ PVOID **Items,
    _Out_ PULONG NumberOfItems
    );

PHLIBAPI
VOID PhSetImageListBitmap(
    _In_ HIMAGELIST ImageList,
    _In_ INT Index,
    _In_ HINSTANCE InstanceHandle,
    _In_ LPCWSTR BitmapName
    );

PHLIBAPI
VOID PhGetStockApplicationIcon(
    _Out_opt_ HICON *SmallIcon,
    _Out_opt_ HICON *LargeIcon
    );

PHLIBAPI
HICON PhGetFileShellIcon(
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR DefaultExtension,
    _In_ BOOLEAN LargeIcon
    );

PHLIBAPI
VOID PhSetClipboardString(
    _In_ HWND hWnd,
    _In_ PPH_STRINGREF String
    );

typedef struct _DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *PDLGTEMPLATEEX;

HWND PhCreateDialogFromTemplate(
    _In_ HWND Parent,
    _In_ ULONG Style,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

BOOLEAN PhModalPropertySheet(
    _Inout_ PROPSHEETHEADER *Header
    );

#define PH_ANCHOR_LEFT 0x1
#define PH_ANCHOR_TOP 0x2
#define PH_ANCHOR_RIGHT 0x4
#define PH_ANCHOR_BOTTOM 0x8
#define PH_ANCHOR_ALL 0xf

// This interface is horrible and should be rewritten, but it works for now.

#define PH_LAYOUT_FORCE_INVALIDATE 0x1000 // invalidate the control when it is resized
#define PH_LAYOUT_TAB_CONTROL 0x2000 // this is a dummy item, a hack for the tab control
#define PH_LAYOUT_IMMEDIATE_RESIZE 0x4000 // needed for the tab control hack

#define PH_LAYOUT_DUMMY_MASK (PH_LAYOUT_TAB_CONTROL) // items that don't have a window handle, or don't actually get their window resized

typedef struct _PH_LAYOUT_ITEM
{
    HWND Handle;
    struct _PH_LAYOUT_ITEM *ParentItem; // for rectangle calculation
    struct _PH_LAYOUT_ITEM *LayoutParentItem; // for actual resizing
    ULONG LayoutNumber;
    ULONG NumberOfChildren;
    HDWP DeferHandle;

    RECT Rect;
    RECT OrigRect;
    RECT Margin;
    ULONG Anchor;
} PH_LAYOUT_ITEM, *PPH_LAYOUT_ITEM;

typedef struct _PH_LAYOUT_MANAGER
{
    PPH_LIST List;
    PH_LAYOUT_ITEM RootItem;

    ULONG LayoutNumber;
} PH_LAYOUT_MANAGER, *PPH_LAYOUT_MANAGER;

PHLIBAPI
VOID PhInitializeLayoutManager(
    _Out_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND RootWindowHandle
    );

PHLIBAPI
VOID PhDeleteLayoutManager(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    );

PHLIBAPI
PPH_LAYOUT_ITEM PhAddLayoutItem(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

PHLIBAPI
PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_ RECT Margin
    );

PHLIBAPI
VOID PhLayoutManagerLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    );

FORCEINLINE VOID PhResizingMinimumSize(
    _Inout_ PRECT Rect,
    _In_ WPARAM Edge,
    _In_ LONG MinimumWidth,
    _In_ LONG MinimumHeight
    )
{
    if (Edge == WMSZ_BOTTOMRIGHT || Edge == WMSZ_RIGHT || Edge == WMSZ_TOPRIGHT)
    {
        if (Rect->right - Rect->left < MinimumWidth)
            Rect->right = Rect->left + MinimumWidth;
    }
    else if (Edge == WMSZ_BOTTOMLEFT || Edge == WMSZ_LEFT || Edge == WMSZ_TOPLEFT)
    {
        if (Rect->right - Rect->left < MinimumWidth)
            Rect->left = Rect->right - MinimumWidth;
    }

    if (Edge == WMSZ_BOTTOMRIGHT || Edge == WMSZ_BOTTOM || Edge == WMSZ_BOTTOMLEFT)
    {
        if (Rect->bottom - Rect->top < MinimumHeight)
            Rect->bottom = Rect->top + MinimumHeight;
    }
    else if (Edge == WMSZ_TOPRIGHT || Edge == WMSZ_TOP || Edge == WMSZ_TOPLEFT)
    {
        if (Rect->bottom - Rect->top < MinimumHeight)
            Rect->top = Rect->bottom - MinimumHeight;
    }
}

FORCEINLINE VOID PhCopyControlRectangle(
    _In_ HWND ParentWindowHandle,
    _In_ HWND FromControlHandle,
    _In_ HWND ToControlHandle
    )
{
    RECT windowRect;

    GetWindowRect(FromControlHandle, &windowRect);
    MapWindowPoints(NULL, ParentWindowHandle, (POINT *)&windowRect, 2);
    MoveWindow(ToControlHandle, windowRect.left, windowRect.top,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);
}

// icotobmp

PHLIBAPI
HBITMAP
NTAPI
PhIconToBitmap(
    _In_ HICON Icon,
    _In_ ULONG Width,
    _In_ ULONG Height
    );

// extlv

#define PH_ALIGN_CENTER 0x0
#define PH_ALIGN_LEFT 0x1
#define PH_ALIGN_RIGHT 0x2
#define PH_ALIGN_TOP 0x4
#define PH_ALIGN_BOTTOM 0x8

typedef enum _PH_ITEM_STATE
{
    // The item is normal. Use the ItemColorFunction
    // to determine the color of the item.
    NormalItemState = 0,
    // The item is new. On the next tick,
    // change the state to NormalItemState. When an
    // item is in this state, highlight it in NewColor.
    NewItemState,
    // The item is being removed. On the next tick,
    // delete the item. When an item is in this state,
    // highlight it in RemovingColor.
    RemovingItemState
} PH_ITEM_STATE;

typedef COLORREF (NTAPI *PPH_EXTLV_GET_ITEM_COLOR)(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );

typedef HFONT (NTAPI *PPH_EXTLV_GET_ITEM_FONT)(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhSetExtendedListView(
    _In_ HWND hWnd
    );

PHLIBAPI
VOID
NTAPI
PhSetHeaderSortIcon(
    _In_ HWND hwnd,
    _In_ INT Index,
    _In_ PH_SORT_ORDER Order
    );

// next 1122

#define ELVM_ADDFALLBACKCOLUMN (WM_APP + 1106)
#define ELVM_ADDFALLBACKCOLUMNS (WM_APP + 1109)
#define ELVM_RESERVED5 (WM_APP + 1120)
#define ELVM_INIT (WM_APP + 1102)
#define ELVM_SETCOLUMNWIDTH (WM_APP + 1121)
#define ELVM_SETCOMPAREFUNCTION (WM_APP + 1104)
#define ELVM_SETCONTEXT (WM_APP + 1103)
#define ELVM_SETCURSOR (WM_APP + 1114)
#define ELVM_RESERVED4 (WM_APP + 1118)
#define ELVM_SETITEMCOLORFUNCTION (WM_APP + 1111)
#define ELVM_SETITEMFONTFUNCTION (WM_APP + 1117)
#define ELVM_RESERVED1 (WM_APP + 1112)
#define ELVM_SETREDRAW (WM_APP + 1116)
#define ELVM_RESERVED2 (WM_APP + 1113)
#define ELVM_SETSORT (WM_APP + 1108)
#define ELVM_SETSORTFAST (WM_APP + 1119)
#define ELVM_RESERVED0 (WM_APP + 1110)
#define ELVM_SETTRISTATE (WM_APP + 1107)
#define ELVM_SETTRISTATECOMPAREFUNCTION (WM_APP + 1105)
#define ELVM_SORTITEMS (WM_APP + 1101)
#define ELVM_RESERVED3 (WM_APP + 1115)

#define ExtendedListView_AddFallbackColumn(hWnd, Column) \
    SendMessage((hWnd), ELVM_ADDFALLBACKCOLUMN, (WPARAM)(Column), 0)
#define ExtendedListView_AddFallbackColumns(hWnd, NumberOfColumns, Columns) \
    SendMessage((hWnd), ELVM_ADDFALLBACKCOLUMNS, (WPARAM)(NumberOfColumns), (LPARAM)(Columns))
#define ExtendedListView_Init(hWnd) \
    SendMessage((hWnd), ELVM_INIT, 0, 0)
#define ExtendedListView_SetColumnWidth(hWnd, Column, Width) \
    SendMessage((hWnd), ELVM_SETCOLUMNWIDTH, (WPARAM)(Column), (LPARAM)(Width))
#define ExtendedListView_SetCompareFunction(hWnd, Column, CompareFunction) \
    SendMessage((hWnd), ELVM_SETCOMPAREFUNCTION, (WPARAM)(Column), (LPARAM)(CompareFunction))
#define ExtendedListView_SetContext(hWnd, Context) \
    SendMessage((hWnd), ELVM_SETCONTEXT, 0, (LPARAM)(Context))
#define ExtendedListView_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), ELVM_SETCURSOR, 0, (LPARAM)(Cursor))
#define ExtendedListView_SetItemColorFunction(hWnd, ItemColorFunction) \
    SendMessage((hWnd), ELVM_SETITEMCOLORFUNCTION, 0, (LPARAM)(ItemColorFunction))
#define ExtendedListView_SetItemFontFunction(hWnd, ItemFontFunction) \
    SendMessage((hWnd), ELVM_SETITEMFONTFUNCTION, 0, (LPARAM)(ItemFontFunction))
#define ExtendedListView_SetRedraw(hWnd, Redraw) \
    SendMessage((hWnd), ELVM_SETREDRAW, (WPARAM)(Redraw), 0)
#define ExtendedListView_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), ELVM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))
#define ExtendedListView_SetSortFast(hWnd, Fast) \
    SendMessage((hWnd), ELVM_SETSORTFAST, (WPARAM)(Fast), 0)
#define ExtendedListView_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), ELVM_SETTRISTATE, (WPARAM)(TriState), 0)
#define ExtendedListView_SetTriStateCompareFunction(hWnd, CompareFunction) \
    SendMessage((hWnd), ELVM_SETTRISTATECOMPAREFUNCTION, 0, (LPARAM)(CompareFunction))
#define ExtendedListView_SortItems(hWnd) \
    SendMessage((hWnd), ELVM_SORTITEMS, 0, 0)

#define ELVSCW_AUTOSIZE (-1)
#define ELVSCW_AUTOSIZE_USEHEADER (-2)
#define ELVSCW_AUTOSIZE_REMAININGSPACE (-3)

/**
 * Gets the brightness of a color.
 *
 * \param Color The color.
 *
 * \return A value ranging from 0 to 255,
 * indicating the brightness of the color.
 */
FORCEINLINE
ULONG
PhGetColorBrightness(
    _In_ COLORREF Color
    )
{
    ULONG r = Color & 0xff;
    ULONG g = (Color >> 8) & 0xff;
    ULONG b = (Color >> 16) & 0xff;
    ULONG min;
    ULONG max;

    min = r;
    if (g < min) min = g;
    if (b < min) min = b;

    max = r;
    if (g > max) max = g;
    if (b > max) max = b;

    return (min + max) / 2;
}

FORCEINLINE
COLORREF
PhHalveColorBrightness(
    _In_ COLORREF Color
    )
{
    /*return RGB(
        (UCHAR)Color / 2,
        (UCHAR)(Color >> 8) / 2,
        (UCHAR)(Color >> 16) / 2
        );*/
    // Since all targets are little-endian, we can use the following method.
    *((PUCHAR)&Color) /= 2;
    *((PUCHAR)&Color + 1) /= 2;
    *((PUCHAR)&Color + 2) /= 2;

    return Color;
}

FORCEINLINE
COLORREF
PhMakeColorBrighter(
    _In_ COLORREF Color,
    _In_ UCHAR Increment
    )
{
    UCHAR r;
    UCHAR g;
    UCHAR b;

    r = (UCHAR)Color;
    g = (UCHAR)(Color >> 8);
    b = (UCHAR)(Color >> 16);

    if (r <= 255 - Increment)
        r += Increment;
    else
        r = 255;

    if (g <= 255 - Increment)
        g += Increment;
    else
        g = 255;

    if (b <= 255 - Increment)
        b += Increment;
    else
        b = 255;

    return RGB(r, g, b);
}

#ifdef __cplusplus
}
#endif

#endif
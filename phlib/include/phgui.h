#ifndef _PH_PHGUI_H
#define _PH_PHGUI_H

#pragma once

#include <ph.h>
#include <commctrl.h>
#define CINTERFACE
#define COBJMACROS
#include <shlobj.h>
#undef CINTERFACE
#undef COBJMACROS
#include <prsht.h>
#include <aclapi.h>

// guisup

typedef BOOL (WINAPI *_ChangeWindowMessageFilter)(
    __in UINT message,
    __in DWORD dwFlag
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

typedef BOOL (WINAPI *_RunFileDlg)(
    __in HWND hwndOwner,
    __in_opt HICON hIcon,
    __in_opt LPCWSTR lpszDirectory,
    __in_opt LPCWSTR lpszTitle,
    __in_opt LPCWSTR lpszDescription,
    __in ULONG uFlags
    );

typedef HRESULT (WINAPI *_SetWindowTheme)(
    __in HWND hwnd,
    __in LPCWSTR pszSubAppName,
    __in LPCWSTR pszSubIdList
    );

typedef BOOL (WINAPI *_IsThemeActive)(); 

typedef HTHEME (WINAPI *_OpenThemeData)(
    __in HWND hwnd,
    __in LPCWSTR pszClassList
    );

typedef HRESULT (WINAPI *_CloseThemeData)(
    __in HTHEME hTheme
    );

typedef HRESULT (WINAPI *_DrawThemeBackground)(
    __in HTHEME hTheme,
    __in HDC hdc,
    __in int iPartId,
    __in int iStateId,
    __in const RECT *pRect,
    __in const RECT *pClipRect
    );

typedef HRESULT (WINAPI *_SHAutoComplete)(
    __in HWND hwndEdit,
    __in DWORD dwFlags
    );

typedef HRESULT (WINAPI *_SHCreateShellItem)(
    __in_opt PCIDLIST_ABSOLUTE pidlParent,
    __in_opt IShellFolder *psfParent,
    __in PCUITEMID_CHILD pidl,
    __out IShellItem **ppsi
    );

typedef HRESULT (WINAPI *_SHOpenFolderAndSelectItems)(
    __in PCIDLIST_ABSOLUTE pidlFolder,
    __in UINT cidl,
    __in_ecount_opt(cidl) PCUITEMID_CHILD_ARRAY *apidl,
    __in DWORD dwFlags
    );

typedef HRESULT (WINAPI *_SHParseDisplayName)(
    __in LPCWSTR pszName,
    __in_opt IBindCtx *pbc,
    __out PIDLIST_ABSOLUTE *ppidl,
    __in SFGAOF sfgaoIn,
    __out SFGAOF *psfgaoOut
    );

typedef INT (WINAPI *_StrCmpLogicalW)(
    __in LPCWSTR psz1,
    __in LPCWSTR psz2
    );

typedef HRESULT (WINAPI *_TaskDialogIndirect)(      
    __in const TASKDIALOGCONFIG *pTaskConfig,
    __in int *pnButton,
    __in int *pnRadioButton,
    __in BOOL *pfVerificationFlagChecked
    );

#ifndef GUISUP_PRIVATE
extern _ChangeWindowMessageFilter ChangeWindowMessageFilter_I;
extern _RunFileDlg RunFileDlg;
extern _IsThemeActive IsThemeActive_I;
extern _OpenThemeData OpenThemeData_I;
extern _CloseThemeData CloseThemeData_I;
extern _DrawThemeBackground DrawThemeBackground_I;
extern _SHAutoComplete SHAutoComplete_I;
extern _SHCreateShellItem SHCreateShellItem_I;
extern _SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems_I;
extern _SHParseDisplayName SHParseDisplayName_I;
extern _StrCmpLogicalW StrCmpLogicalW_I;
extern _TaskDialogIndirect TaskDialogIndirect_I;
#endif

VOID PhGuiSupportInitialization();

PHLIBAPI
VOID PhSetControlTheme(
    __in HWND Handle,
    __in PWSTR Theme
    );

FORCEINLINE VOID PhSetWindowStyle(
    __in HWND Handle,
    __in LONG_PTR Mask,
    __in LONG_PTR Value
    )
{
    LONG_PTR style;

    style = GetWindowLongPtr(Handle, GWL_STYLE);
    style &= ~Mask;
    style |= Value;
    SetWindowLongPtr(Handle, GWL_STYLE, style);
}

FORCEINLINE VOID PhSetWindowExStyle(
    __in HWND Handle,
    __in LONG_PTR Mask,
    __in LONG_PTR Value
    )
{
    LONG_PTR style;

    style = GetWindowLongPtr(Handle, GWL_EXSTYLE);
    style &= ~Mask;
    style |= Value;
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
    __in HWND Handle,
    __in UINT Message,
    __in WPARAM wParam,
    __in LPARAM lParam
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

HWND PhCreateListViewControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

FORCEINLINE VOID PhSetListViewStyle(
    __in HWND Handle,
    __in BOOLEAN AllowDragDrop,
    __in BOOLEAN ShowLabelTips
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
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT DisplayIndex,
    __in INT SubItemIndex,
    __in INT Format,
    __in INT Width,
    __in PWSTR Text
    );

PHLIBAPI
INT PhAddListViewItem(
    __in HWND ListViewHandle,
    __in INT Index,
    __in PWSTR Text,
    __in PVOID Param
    );

PHLIBAPI
INT PhFindListViewItemByFlags(
    __in HWND ListViewHandle,
    __in INT StartIndex,
    __in ULONG Flags
    );

PHLIBAPI
INT PhFindListViewItemByParam(
    __in HWND ListViewHandle,
    __in INT StartIndex,
    __in PVOID Param
    );

PHLIBAPI
LOGICAL PhGetListViewItemImageIndex(
    __in HWND ListViewHandle,
    __in INT Index,
    __out PINT ImageIndex
    );

PHLIBAPI
LOGICAL PhGetListViewItemParam(
    __in HWND ListViewHandle,
    __in INT Index,
    __out PPVOID Param
    );

PHLIBAPI
VOID PhRemoveListViewItem(
    __in HWND ListViewHandle,
    __in INT Index
    );

PHLIBAPI
VOID PhSetListViewItemImageIndex(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT ImageIndex
    );

PHLIBAPI
VOID PhSetListViewItemStateImage(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT StateImage
    );

PHLIBAPI
VOID PhSetListViewSubItem(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT SubItemIndex,
    __in PWSTR Text
    );

PHLIBAPI
BOOLEAN PhLoadListViewColumnSettings(
    __in HWND ListViewHandle,
    __in PPH_STRING Settings
    );

PHLIBAPI
PPH_STRING PhSaveListViewColumnSettings(
    __in HWND ListViewHandle
    );

HWND PhCreateTabControl(
    __in HWND ParentHandle
    );

PHLIBAPI
INT PhAddTabControlTab(
    __in HWND TabControlHandle,
    __in INT Index,
    __in PWSTR Text
    );

#define PHA_GET_DLGITEM_TEXT(hwndDlg, id) \
    ((PPH_STRING)PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, id))))

PHLIBAPI
PPH_STRING PhGetWindowText(
    __in HWND hwnd
    );

PHLIBAPI
VOID PhAddComboBoxStrings(
    __in HWND hWnd,
    __in PWSTR *Strings,
    __in ULONG NumberOfStrings
    );

PHLIBAPI
PPH_STRING PhGetComboBoxString(
    __in HWND hwnd,
    __in INT Index
    );

PHLIBAPI
PPH_STRING PhGetListBoxString(
    __in HWND hwnd,
    __in INT Index
    );

PHLIBAPI
VOID PhShowContextMenu(
    __in HWND hwnd,
    __in HWND subHwnd,
    __in HMENU menu,
    __in POINT point
    );

PHLIBAPI
UINT PhShowContextMenu2(
    __in HWND hwnd,
    __in HWND subHwnd,
    __in HMENU menu,
    __in POINT point
    );

PHLIBAPI
VOID PhSetMenuItemBitmap(
    __in HMENU Menu,
    __in ULONG Item,
    __in BOOLEAN ByPosition,
    __in HBITMAP Bitmap
    );

PHLIBAPI
VOID PhSetRadioCheckMenuItem(
    __in HMENU Menu,
    __in ULONG Id,
    __in BOOLEAN RadioCheck
    );

PHLIBAPI
VOID PhEnableMenuItem(
    __in HMENU Menu,
    __in ULONG Id,
    __in BOOLEAN Enable
    );

PHLIBAPI
VOID PhEnableAllMenuItems(
    __in HMENU Menu,
    __in BOOLEAN Enable
    );

PHLIBAPI
VOID PhSetStateAllListViewItems(
    __in HWND hWnd,
    __in ULONG State,
    __in ULONG Mask
    );

PHLIBAPI
PVOID PhGetSelectedListViewItemParam(
    __in HWND hWnd
    );

PHLIBAPI
VOID PhGetSelectedListViewItemParams(
    __in HWND hWnd,
    __out PVOID **Items,
    __out PULONG NumberOfItems
    );

PHLIBAPI
VOID PhSetImageListBitmap(
    __in HIMAGELIST ImageList,
    __in INT Index,
    __in HINSTANCE InstanceHandle,
    __in LPCWSTR BitmapName
    );

typedef struct _PH_IMAGE_LIST_WRAPPER
{
    HIMAGELIST Handle;
    PPH_LIST FreeList;
} PH_IMAGE_LIST_WRAPPER, *PPH_IMAGE_LIST_WRAPPER;

PHLIBAPI
VOID PhInitializeImageListWrapper(
    __out PPH_IMAGE_LIST_WRAPPER Wrapper,
    __in ULONG Width,
    __in ULONG Height,
    __in ULONG Flags
    );

PHLIBAPI
VOID PhDeleteImageListWrapper(
    __inout PPH_IMAGE_LIST_WRAPPER Wrapper
    );

PHLIBAPI
INT PhImageListWrapperAddIcon(
    __in PPH_IMAGE_LIST_WRAPPER Wrapper,
    __in HICON Icon
    );

PHLIBAPI
VOID PhImageListWrapperRemove(
    __in PPH_IMAGE_LIST_WRAPPER Wrapper,
    __in INT Index
    );

PHLIBAPI
HICON PhGetFileShellIcon(
    __in_opt PWSTR FileName,
    __in_opt PWSTR DefaultExtension,
    __in BOOLEAN LargeIcon
    );

PHLIBAPI
VOID PhSetClipboardString(
    __in HWND hWnd,
    __in PPH_STRINGREF String
    );

PHLIBAPI
VOID PhSetClipboardStringEx(
    __in HWND hWnd,
    __in PWSTR Buffer,
    __in SIZE_T Length
    );

#define PH_ANCHOR_LEFT 0x1
#define PH_ANCHOR_TOP 0x2
#define PH_ANCHOR_RIGHT 0x4
#define PH_ANCHOR_BOTTOM 0x8
#define PH_ANCHOR_ALL 0xf

#define PH_LAYOUT_FORCE_INVALIDATE 0x1000 // invalidate the control when it is resized
#define PH_LAYOUT_TAB_CONTROL 0x2000 // this is a dummy item, a hack for the tab control
#define PH_LAYOUT_IMMEDIATE_RESIZE 0x4000 // needed for the tab control hack

#define PH_LAYOUT_DUMMY_MASK (PH_LAYOUT_TAB_CONTROL) // determines which items don't get resized at all; just dummy

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
    __out PPH_LAYOUT_MANAGER Manager,
    __in HWND RootWindowHandle
    );

PHLIBAPI
VOID PhDeleteLayoutManager(
    __inout PPH_LAYOUT_MANAGER Manager
    );

PHLIBAPI
PPH_LAYOUT_ITEM PhAddLayoutItem(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in_opt PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor
    );

PHLIBAPI
PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in_opt PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor,
    __in RECT Margin
    );

PHLIBAPI
VOID PhLayoutManagerLayout(
    __inout PPH_LAYOUT_MANAGER Manager
    );

FORCEINLINE VOID PhResizingMinimumSize(
    __inout PRECT Rect,
    __in WPARAM Edge,
    __in LONG MinimumWidth,
    __in LONG MinimumHeight
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
    __in HWND ParentWindowHandle,
    __in HWND FromControlHandle,
    __in HWND ToControlHandle
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
HBITMAP PhIconToBitmap(
    __in HICON Icon,
    __in ULONG Width,
    __in ULONG Height
    );

// extlv

#define PH_ALIGN_CENTER 0x0
#define PH_ALIGN_LEFT 0x1
#define PH_ALIGN_RIGHT 0x2
#define PH_ALIGN_TOP 0x4
#define PH_ALIGN_BOTTOM 0x8

FORCEINLINE ULONG PhToListViewColumnAlign(
    __in ULONG Align
    )
{
    switch (Align)
    {
    case PH_ALIGN_LEFT:
        return LVCFMT_LEFT;
    case PH_ALIGN_RIGHT:
        return LVCFMT_RIGHT;
    default:
        return LVCFMT_CENTER;
    }
}

FORCEINLINE ULONG PhFromListViewColumnAlign(
    __in ULONG Format
    )
{
    if (Format & LVCFMT_LEFT)
        return PH_ALIGN_LEFT;
    else if (Format & LVCFMT_RIGHT)
        return PH_ALIGN_RIGHT;
    else
        return PH_ALIGN_CENTER;
}

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
    __in INT Index,
    __in PVOID Param,
    __in_opt PVOID Context
    );

typedef HFONT (NTAPI *PPH_EXTLV_GET_ITEM_FONT)(
    __in INT Index,
    __in PVOID Param,
    __in_opt PVOID Context
    );

PHLIBAPI
VOID PhSetExtendedListView(
    __in HWND hWnd
    );

PHLIBAPI
VOID PhSetHeaderSortIcon(
    __in HWND hwnd,
    __in INT Index,
    __in PH_SORT_ORDER Order
    );

// next 1119

#define ELVM_ADDFALLBACKCOLUMN (WM_APP + 1106)
#define ELVM_ADDFALLBACKCOLUMNS (WM_APP + 1109)
#define ELVM_INIT (WM_APP + 1102)
#define ELVM_SETCOMPAREFUNCTION (WM_APP + 1104)
#define ELVM_SETCONTEXT (WM_APP + 1103)
#define ELVM_SETCURSOR (WM_APP + 1114)
#define ELVM_SETHIGHLIGHTINGDURATION (WM_APP + 1118)
#define ELVM_SETITEMCOLORFUNCTION (WM_APP + 1111)
#define ELVM_SETITEMFONTFUNCTION (WM_APP + 1117)
#define ELVM_SETNEWCOLOR (WM_APP + 1112)
#define ELVM_SETREDRAW (WM_APP + 1116)
#define ELVM_SETREMOVINGCOLOR (WM_APP + 1113)
#define ELVM_SETSORT (WM_APP + 1108)
#define ELVM_SETSTATEHIGHLIGHTING (WM_APP + 1110)
#define ELVM_SETTRISTATE (WM_APP + 1107)
#define ELVM_SETTRISTATECOMPAREFUNCTION (WM_APP + 1105)
#define ELVM_SORTITEMS (WM_APP + 1101)
#define ELVM_TICK (WM_APP + 1115)

#define ExtendedListView_AddFallbackColumn(hWnd, Column) \
    SendMessage((hWnd), ELVM_ADDFALLBACKCOLUMN, (WPARAM)(Column), 0)
#define ExtendedListView_AddFallbackColumns(hWnd, NumberOfColumns, Columns) \
    SendMessage((hWnd), ELVM_ADDFALLBACKCOLUMNS, (WPARAM)(NumberOfColumns), (LPARAM)(Columns))
#define ExtendedListView_Init(hWnd) \
    SendMessage((hWnd), ELVM_INIT, 0, 0)
#define ExtendedListView_SetCompareFunction(hWnd, Column, CompareFunction) \
    SendMessage((hWnd), ELVM_SETCOMPAREFUNCTION, (WPARAM)(Column), (LPARAM)(CompareFunction))
#define ExtendedListView_SetContext(hWnd, Context) \
    SendMessage((hWnd), ELVM_SETCONTEXT, 0, (LPARAM)(Context))
#define ExtendedListView_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), ELVM_SETCURSOR, 0, (LPARAM)(Cursor))
#define ExtendedListView_SetHighlightingDuration(hWnd, Duration) \
    SendMessage((hWnd), ELVM_SETHIGHLIGHTINGDURATION, (WPARAM)(Duration), 0)
#define ExtendedListView_SetItemColorFunction(hWnd, ItemColorFunction) \
    SendMessage((hWnd), ELVM_SETITEMCOLORFUNCTION, 0, (LPARAM)(ItemColorFunction))
#define ExtendedListView_SetItemFontFunction(hWnd, ItemFontFunction) \
    SendMessage((hWnd), ELVM_SETITEMFONTFUNCTION, 0, (LPARAM)(ItemFontFunction))
#define ExtendedListView_SetNewColor(hWnd, NewColor) \
    SendMessage((hWnd), ELVM_SETNEWCOLOR, (WPARAM)(NewColor), 0)
#define ExtendedListView_SetRedraw(hWnd, Redraw) \
    SendMessage((hWnd), ELVM_SETREDRAW, (WPARAM)(Redraw), 0)
#define ExtendedListView_SetRemovingColor(hWnd, RemovingColor) \
    SendMessage((hWnd), ELVM_SETREMOVINGCOLOR, (WPARAM)(RemovingColor), 0)
#define ExtendedListView_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), ELVM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))
#define ExtendedListView_SetStateHighlighting(hWnd, StateHighlighting) \
    SendMessage((hWnd), ELVM_SETSTATEHIGHLIGHTING, (WPARAM)(StateHighlighting), 0)
#define ExtendedListView_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), ELVM_SETTRISTATE, (WPARAM)(TriState), 0)
#define ExtendedListView_SetTriStateCompareFunction(hWnd, CompareFunction) \
    SendMessage((hWnd), ELVM_SETTRISTATECOMPAREFUNCTION, 0, (LPARAM)(CompareFunction))
#define ExtendedListView_SortItems(hWnd) \
    SendMessage((hWnd), ELVM_SORTITEMS, 0, 0)
#define ExtendedListView_Tick(hWnd) \
    SendMessage((hWnd), ELVM_TICK, 0, 0)

/**
 * Gets the brightness of a color.
 *
 * \param Color The color.
 *
 * \return A value ranging from 0 to 255, 
 * indicating the brightness of the color.
 */
FORCEINLINE ULONG PhGetColorBrightness(
    __in COLORREF Color
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

FORCEINLINE COLORREF PhHalveColorBrightness(
    __in COLORREF Color
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

FORCEINLINE COLORREF PhMakeColorBrighter(
    __in COLORREF Color,
    __in UCHAR Increment
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

// secedit

typedef NTSTATUS (NTAPI *PPH_GET_OBJECT_SECURITY)(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in_opt PVOID Context
    );

typedef NTSTATUS (NTAPI *PPH_SET_OBJECT_SECURITY)(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in_opt PVOID Context
    );

typedef struct _PH_ACCESS_ENTRY
{
    PWSTR Name;
    ACCESS_MASK Access;
    BOOLEAN General;
    BOOLEAN Specific;
    PWSTR ShortName;
} PH_ACCESS_ENTRY, *PPH_ACCESS_ENTRY;

PHLIBAPI
HPROPSHEETPAGE PhCreateSecurityPage(
    __in PWSTR ObjectName,
    __in PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    __in PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    __in_opt PVOID Context,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
    );

PHLIBAPI
VOID PhEditSecurity(
    __in HWND hWnd,
    __in PWSTR ObjectName,
    __in PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    __in PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    __in_opt PVOID Context,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
    );

typedef NTSTATUS (NTAPI *PPH_OPEN_OBJECT)(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PVOID Context
    );

typedef struct _PH_STD_OBJECT_SECURITY
{
    PPH_OPEN_OBJECT OpenObject;
    PWSTR ObjectType;
    PVOID Context;
} PH_STD_OBJECT_SECURITY, *PPH_STD_OBJECT_SECURITY;

FORCEINLINE ACCESS_MASK PhGetAccessForGetSecurity(
    __in SECURITY_INFORMATION SecurityInformation
    )
{
    ACCESS_MASK access = 0;

    if (
        (SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION) ||
        (SecurityInformation & DACL_SECURITY_INFORMATION)
        )
    {
        access |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        access |= ACCESS_SYSTEM_SECURITY;
    }

    return access;
}

FORCEINLINE ACCESS_MASK PhGetAccessForSetSecurity(
    __in SECURITY_INFORMATION SecurityInformation
    )
{
    ACCESS_MASK access = 0;

    if (
        (SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION)
        )
    {
        access |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        access |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        access |= ACCESS_SYSTEM_SECURITY;
    }

    return access;
}

PHLIBAPI
__callback NTSTATUS PhStdGetObjectSecurity(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in_opt PVOID Context
    );

PHLIBAPI
__callback NTSTATUS PhStdSetObjectSecurity(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhGetSeObjectSecurity(
    __in HANDLE Handle,
    __in SE_OBJECT_TYPE ObjectType,
    __in SECURITY_INFORMATION SecurityInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

PHLIBAPI
NTSTATUS PhSetSeObjectSecurity(
    __in HANDLE Handle,
    __in SE_OBJECT_TYPE ObjectType,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// secdata

PHLIBAPI
BOOLEAN PhGetAccessEntries(
    __in PWSTR Type,
    __out PPH_ACCESS_ENTRY *AccessEntries,
    __out PULONG NumberOfAccessEntries
    );

PHLIBAPI
PPH_STRING PhGetAccessString(
    __in ACCESS_MASK Access,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
    );

#endif
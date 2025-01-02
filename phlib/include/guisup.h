/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_PHGUI_H
#define _PH_PHGUI_H

#include <commctrl.h>
#include <guisupview.h>

EXTERN_C_START

// guisup

#define RFF_NOBROWSE 0x0001
#define RFF_NODEFAULT 0x0002
#define RFF_CALCDIRECTORY 0x0004
#define RFF_NOLABEL 0x0008
#define RFF_NOSEPARATEMEM 0x0020
#define RFF_OPTRUNAS 0x0040

#define RFN_VALIDATE (-510)
#define RFN_LIMITEDRUNAS (-511)

typedef struct _NMRUNFILEDLGW
{
    NMHDR hdr;
    PWSTR lpszFile;
    PWSTR lpszDirectory;
    UINT ShowCmd;
} NMRUNFILEDLGW, *LPNMRUNFILEDLGW, *PNMRUNFILEDLGW;

typedef NMRUNFILEDLGW NMRUNFILEDLG;
typedef PNMRUNFILEDLGW PNMRUNFILEDLG;
typedef LPNMRUNFILEDLGW LPNMRUNFILEDLG;

#define RF_OK 0x0000
#define RF_CANCEL 0x0001
#define RF_RETRY 0x0002

typedef HANDLE HTHEME;

#define DCX_USESTYLE 0x00010000
#define DCX_NODELETERGN 0x00040000

#define HRGN_FULL ((HRGN)1) // passed by WM_NCPAINT even though it's completely undocumented (wj32)

extern LONG PhSystemDpi;
extern PH_INTEGER_PAIR PhSmallIconSize;
extern PH_INTEGER_PAIR PhLargeIconSize;

PHLIBAPI
VOID
NTAPI
PhGuiSupportInitialization(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhGuiSupportUpdateSystemMetrics(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    );

PHLIBAPI
VOID
NTAPI
PhInitializeFont(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    );

PHLIBAPI
VOID
NTAPI
PhInitializeMonospaceFont(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    );

PHLIBAPI
HDC
NTAPI
PhGetScreenDC(
    VOID
    );

PHLIBAPI
HGDIOBJ
NTAPI
PhGetStockObject(
    _In_ LONG Index
    );

#define PhGetStockBrush(i) ((HBRUSH)PhGetStockObject(i))
#define PhGetStockPen(i) ((HPEN)PhGetStockObject(i))

PHLIBAPI
HTHEME
NTAPI
PhOpenThemeData(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR ClassList,
    _In_ LONG WindowDpi
    );

PHLIBAPI
VOID
NTAPI
PhCloseThemeData(
    _In_ HTHEME ThemeHandle
    );

PHLIBAPI
VOID
NTAPI
PhSetControlTheme(
    _In_ HWND Handle,
    _In_opt_ PCWSTR Theme
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsThemeActive(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsThemePartDefined(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeClass(
    _In_ HTHEME ThemeHandle,
    _Out_writes_z_(ClassLength) PWSTR Class,
    _In_ ULONG ClassLength
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeColor(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _Out_ COLORREF* Color
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeInt(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _Out_ PLONG Value
    );

typedef enum _THEMEPARTSIZE
{
    THEMEPARTSIZE_MIN, // minimum size
    THEMEPARTSIZE_TRUE, // size without stretching
    THEMEPARTSIZE_DRAW // size that theme mgr will use to draw part
} THEMEPARTSIZE;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemePartSize(
    _In_ HTHEME ThemeHandle,
    _In_opt_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_opt_ LPCRECT Rect,
    _In_ THEMEPARTSIZE Flags,
    _Out_ PSIZE Size
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDrawThemeBackground(
    _In_ HTHEME ThemeHandle,
    _In_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LPCRECT Rect,
    _In_opt_ LPCRECT ClipRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhAllowDarkModeForWindow(
    _In_ HWND WindowHandle,
    _In_ BOOL Enabled
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsDarkModeAllowedForWindow(
    _In_ HWND WindowHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetWindowRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT WindowRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetClientRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT ClientRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsHungAppWindow(
    _In_ HWND WindowHandle
    );

PHLIBAPI
HWND
NTAPI
PhGetShellWindow(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetDpi(
    _In_ LONG Number,
    _In_ LONG DpiValue
    );

PHLIBAPI
LONG
NTAPI
PhGetMonitorDpi(
    _In_ LPCRECT rect
    );

PHLIBAPI
LONG
NTAPI
PhGetSystemDpi(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetTaskbarDpi(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetWindowDpi(
    _In_ HWND WindowHandle
    );

PHLIBAPI
LONG
NTAPI
PhGetDpiValue(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LPCRECT Rect
    );

PHLIBAPI
LONG
NTAPI
PhGetSystemMetrics(
    _In_ LONG Index,
    _In_opt_ LONG DpiValue
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetSystemSafeBootMode(
    VOID
    );

PHLIBAPI
BOOL
NTAPI
PhGetSystemParametersInfo(
    _In_ LONG Action,
    _In_ ULONG Param1,
    _Pre_maybenull_ _Post_valid_ PVOID Param2,
    _In_opt_ LONG DpiValue
    );

PHLIBAPI
VOID
NTAPI
PhGetSizeDpiValue(
    _Inout_ PRECT rect,
    _In_ LONG DpiValue,
    _In_ BOOLEAN isUnpack
    );

FORCEINLINE LONG_PTR PhGetClassStyle(
    _In_ HWND WindowHandle
    )
{
    return GetClassLongPtr(WindowHandle, GCL_STYLE);
}

FORCEINLINE VOID PhSetClassStyle(
    _In_ HWND Handle,
    _In_ LONG_PTR Mask,
    _In_ LONG_PTR Value
    )
{
    LONG_PTR style;

    style = GetClassLongPtr(Handle, GCL_STYLE);
    style = (style & ~Mask) | (Value & Mask);
    SetClassLongPtr(Handle, GCL_STYLE, style);
}

FORCEINLINE LONG_PTR PhGetWindowStyle(
    _In_ HWND WindowHandle
    )
{
    return GetWindowLongPtr(WindowHandle, GWL_STYLE);
}

FORCEINLINE LONG_PTR PhGetWindowStyleEx(
    _In_ HWND WindowHandle
    )
{
    return GetWindowLongPtr(WindowHandle, GWL_EXSTYLE);
}

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

FORCEINLINE WNDPROC PhGetWindowProcedure(
    _In_ HWND WindowHandle
    )
{
    return (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
}

FORCEINLINE WNDPROC PhSetWindowProcedure(
    _In_ HWND WindowHandle,
    _In_ WNDPROC SubclassProcedure
    )
{
    return (WNDPROC)SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)SubclassProcedure);
}

#define PH_WINDOW_TIMER_DEFAULT 0xF

FORCEINLINE ULONG_PTR PhSetTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR TimerID,
    _In_ ULONG Elapse,
    _In_opt_ TIMERPROC TimerProcedure
    )
{
    assert(WindowHandle);
    return SetTimer(WindowHandle, TimerID, Elapse, TimerProcedure);
}

FORCEINLINE BOOL PhKillTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR TimerID
    )
{
    assert(WindowHandle);
    return KillTimer(WindowHandle, TimerID);
}

#define IDC_DIVIDER MAKEINTRESOURCE(106) // comctl32.dll

FORCEINLINE
HCURSOR
NTAPI
PhLoadCursor(
    _In_opt_ PVOID BaseAddress,
    _In_ PCWSTR CursorName
    )
{
    return (HCURSOR)LoadImage((HINSTANCE)BaseAddress, CursorName, IMAGE_CURSOR, 0, 0, LR_SHARED);
    //return LoadCursor((HINSTANCE)BaseAddress, CursorName);
}

FORCEINLINE
HCURSOR
NTAPI
PhGetCursor(
    VOID
    )
{
    return GetCursor();
}

FORCEINLINE
HCURSOR
NTAPI
PhSetCursor(
    _In_opt_ HCURSOR CursorHandle
    )
{
    return SetCursor(CursorHandle);
}

FORCEINLINE
BOOLEAN
NTAPI
PhGetKeyState(
    _In_ LONG VirtualKey
    )
{
    return GetKeyState(VirtualKey) < 0;
}

#ifndef WM_REFLECT
#define WM_REFLECT 0x2000
#endif

FORCEINLINE LRESULT PhReflectMessage(
    _In_ HWND Handle,
    _In_ ULONG Message,
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
        INT_ERROR
        );
}

PHLIBAPI
LONG
NTAPI
PhAddListViewColumnDpi(
    _In_ HWND ListViewHandle,
    _In_ LONG ListViewDpi,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddIListViewColumnDpi(
    _In_ IListView* ListView,
    _In_ LONG ListViewDpi,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewColumn(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddIListViewColumn(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhAddIListViewItem(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhFindListViewItemByFlags(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    );

PHLIBAPI
LONG
NTAPI
PhFindIListViewItemByFlags(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    );

PHLIBAPI
LONG
NTAPI
PhFindListViewItemByParam(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhFindIListViewItemByParam(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _Out_ PLONG ImageIndex
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _Outptr_ PVOID *Param
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetIListViewItemParam(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _Outptr_ PVOID * Param
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ PVOID Param
    );

PHLIBAPI
VOID
NTAPI
PhRemoveListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index
    );

PHLIBAPI
VOID
NTAPI
PhSetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG ImageIndex
    );

PHLIBAPI
VOID
NTAPI
PhSetListViewSubItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    );

PHLIBAPI
VOID
NTAPI
PhSetIListViewSubItem(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    );

PHLIBAPI
VOID
NTAPI
PhSetIListViewGroupSubItem(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    );

PHLIBAPI
VOID
NTAPI
PhRedrawListViewItems(
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhRedrawIListViewItems(
    _In_ IListView* ListView,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddIListViewGroup(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewGroupItem(
    _In_ HWND ListViewHandle,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhAddIListViewGroupItem(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhAddTabControlTab(
    _In_ HWND TabControlHandle,
    _In_ LONG Index,
    _In_ PCWSTR Text
    );

#define PhaGetDlgItemText(hwndDlg, id) \
    PH_AUTO_T(PH_STRING, PhGetWindowText(GetDlgItem(hwndDlg, id)))

PHLIBAPI
PPH_STRING
NTAPI
PhGetWindowText(
    _In_ HWND WindowHandle
    );

#define PhaGetWindowText(WindowHandle) \
    PH_AUTO_T(PH_STRING, PhGetWindowText(WindowHandle))

#define PH_GET_WINDOW_TEXT_INTERNAL 0x1
#define PH_GET_WINDOW_TEXT_LENGTH_ONLY 0x2

PHLIBAPI
ULONG
NTAPI
PhGetWindowTextEx(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *Text
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetWindowTextToBuffer(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

FORCEINLINE
VOID
NTAPI
PhAddComboBoxStrings(
    _In_ HWND WindowHandle,
    _In_ PCWSTR* Strings,
    _In_ ULONG NumberOfStrings
    )
{
    for (ULONG i = 0; i < NumberOfStrings; i++)
        ComboBox_AddString(WindowHandle, Strings[i]);
}

FORCEINLINE
VOID
NTAPI
PhAddComboBoxStringRefs(
    _In_ HWND WindowHandle,
    _In_ CONST PPH_STRINGREF* Strings,
    _In_ ULONG NumberOfStrings
    )
{
    for (ULONG i = 0; i < NumberOfStrings; i++)
        ComboBox_AddString(WindowHandle, Strings[i]->Buffer);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetComboBoxString(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    );

PHLIBAPI
LONG
NTAPI
PhSelectComboBoxString(
    _In_ HWND WindowHandle,
    _In_ PCWSTR String,
    _In_ BOOLEAN Partial
    );

PHLIBAPI
VOID
NTAPI
PhDeleteComboBoxStrings(
    _In_ HWND ComboBoxHandle,
    _In_ BOOLEAN ResetContent
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetListBoxString(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    );

PHLIBAPI
VOID
NTAPI
PhSetStateAllListViewItems(
    _In_ HWND WindowHandle,
    _In_ ULONG State,
    _In_ ULONG Mask
    );

PHLIBAPI
VOID
NTAPI
PhSetStateAllIListViewItems(
    _In_ IListView* ListView,
    _In_ ULONG State,
    _In_ ULONG Mask
    );

PHLIBAPI
PVOID
NTAPI
PhGetSelectedListViewItemParam(
    _In_ HWND WindowHandle
    );

PHLIBAPI
PVOID
NTAPI
PhGetSelectedIListViewItemParam(
    _In_ IListView* ListView
    );

PHLIBAPI
VOID
NTAPI
PhGetSelectedListViewItemParams(
    _In_ HWND WindowHandle,
    _Out_ PVOID **Items,
    _Out_ PULONG NumberOfItems
    );

PHLIBAPI
VOID
NTAPI
PhGetSelectedIListViewItemParams(
    _In_ IListView* ListView,
    _Out_ PVOID** Items,
    _Out_ PULONG NumberOfItems
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetIListViewClientRect(
    _In_ IListView* ListView,
    _Inout_ PRECT ClientRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetIListViewItemRect(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_ ULONG Flags,
    _Inout_ PRECT ItemRect
    );

FORCEINLINE
IListView*
NTAPI
PhGetListViewInterface(
    _In_ HWND ListViewHandle
    )
{
    IListView* listviewInterface = nullptr;

    SendMessage(
        ListViewHandle,
        LVM_QUERYINTERFACE,
        (WPARAM)&IID_IListView,
        (LPARAM)&listviewInterface
        );

    return listviewInterface;
}

#define PhDestroyListViewInterface(ListViewClass) \
    if (ListViewClass) IListView_Release((struct IListView*)(ListViewClass));

PHLIBAPI
VOID
NTAPI
PhSetImageListBitmap(
    _In_ HIMAGELIST ImageList,
    _In_ LONG Index,
    _In_ HINSTANCE InstanceHandle,
    _In_ PCWSTR BitmapName
    );

#define PH_LOAD_ICON_SHARED 0x1
#define PH_LOAD_ICON_SIZE_SMALL 0x2
#define PH_LOAD_ICON_SIZE_LARGE 0x4
#define PH_LOAD_ICON_STRICT 0x8

PHLIBAPI
HICON
NTAPI
PhLoadIcon(
    _In_opt_ PVOID ImageBaseAddress,
    _In_ PCWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ LONG Width,
    _In_opt_ LONG Height,
    _In_opt_ LONG SystemDpi
    );

PHLIBAPI
VOID
NTAPI
PhGetStockApplicationIcon(
    _Out_opt_ HICON *SmallIcon,
    _Out_opt_ HICON *LargeIcon
    );

//PHLIBAPI
//HICON PhGetFileShellIcon(
//    _In_opt_ PWSTR FileName,
//    _In_opt_ PWSTR DefaultExtension,
//    _In_ BOOLEAN LargeIcon
//    );

PHLIBAPI
VOID
NTAPI
PhSetClipboardString(
    _In_ HWND WindowHandle,
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetClipboardString(
    _In_ HWND WindowHandle
    );

#include <pshpack1.h>
typedef struct _DLGTEMPLATEEX
{
    USHORT dlgVer;
    USHORT signature;
    ULONG helpID;
    ULONG exStyle;
    ULONG style;
    USHORT cDlgItems;
    SHORT x;
    SHORT y;
    SHORT cx;
    SHORT cy;
    //sz_Or_Ord menu;
    //sz_Or_Ord windowClass;
    //WCHAR title[titleLen];
    //USHORT pointsize;
    //USHORT weight;
    //BYTE italic;
    //BYTE charset;
    //WCHAR typeface[stringLen];
} DLGTEMPLATEEX, *PDLGTEMPLATEEX;
#include <poppack.h>

PHLIBAPI
HWND
NTAPI
PhCreateDialogFromTemplate(
    _In_ HWND Parent,
    _In_ ULONG Style,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

PHLIBAPI
HWND
NTAPI
PhCreateDialog(
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
HWND
NTAPI
PhCreateWindowEx(
    _In_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ ULONG ExStyle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU MenuHandle,
    _In_opt_ PVOID InstanceHandle,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
INT_PTR
NTAPI
PhDialogBox(
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
HMENU
NTAPI
PhLoadMenu(
    _In_ PVOID DllBase,
    _In_ PCWSTR MenuName
    );

PHLIBAPI
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
    RECT Margin;
    ULONG Anchor;
} PH_LAYOUT_ITEM, *PPH_LAYOUT_ITEM;

typedef struct _PH_LAYOUT_MANAGER
{
    PPH_LIST List;
    PH_LAYOUT_ITEM RootItem;

    ULONG LayoutNumber;

    LONG dpiValue;
} PH_LAYOUT_MANAGER, *PPH_LAYOUT_MANAGER;

PHLIBAPI
VOID
NTAPI
PhInitializeLayoutManager(
    _Out_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND RootWindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhDeleteLayoutManager(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    );

PHLIBAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddLayoutItem(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

PHLIBAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddLayoutItemEx(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_ RECT Margin
    );

PHLIBAPI
VOID
NTAPI
PhLayoutManagerLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    );

#define PH_WINDOW_CONTEXT_DEFAULT 0xFFFF

PHLIBAPI
PVOID
NTAPI
PhGetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    );

PHLIBAPI
VOID
NTAPI
PhSetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash,
    _In_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhRemoveWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    );

FORCEINLINE
PVOID
NTAPI
PhGetWindowContextEx(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    return PhGetWindowContext(WindowHandle, MAXCHAR);
#else
    //assert(GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA) == sizeof(PVOID));
    return (PVOID)GetWindowLongPtr(WindowHandle, 0);
#endif
}

FORCEINLINE
VOID
NTAPI
PhSetWindowContextEx(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhSetWindowContext(WindowHandle, MAXCHAR, Context);
#else
    //assert(GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA) == sizeof(PVOID));
    SetWindowLongPtr(WindowHandle, 0, (LONG_PTR)Context);
#endif
}

FORCEINLINE
VOID
NTAPI
PhRemoveWindowContextEx(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhRemoveWindowContext(WindowHandle, MAXCHAR);
#else
    //assert(GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA) == sizeof(PVOID));
    SetWindowLongPtr(WindowHandle, 0, (LONG_PTR)NULL);
#endif
}

FORCEINLINE
PVOID
NTAPI
PhGetDialogContext(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    return PhGetWindowContext(WindowHandle, MAXCHAR);
#else
    return (PVOID)GetWindowLongPtr(WindowHandle, DWLP_USER);
#endif
}

FORCEINLINE
VOID
NTAPI
PhSetDialogContext(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhSetWindowContext(WindowHandle, MAXCHAR, Context);
#else
    SetWindowLongPtr(WindowHandle, DWLP_USER, (LONG_PTR)Context);
#endif
}

FORCEINLINE
VOID
NTAPI
PhRemoveDialogContext(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhRemoveWindowContext(WindowHandle, MAXCHAR);
#else
    SetWindowLongPtr(WindowHandle, DWLP_USER, (LONG_PTR)NULL);
#endif
}

typedef BOOL (CALLBACK* PH_ENUM_CALLBACK)(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

VOID PhEnumWindows(
    _In_ PH_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (CALLBACK *PH_CHILD_ENUM_CALLBACK)(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

VOID PhEnumChildWindows(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Limit,
    _In_ PH_CHILD_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

HWND PhGetProcessMainWindow(
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    );

HWND PhGetProcessMainWindowEx(
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ BOOLEAN SkipInvisible
    );

PHLIBAPI
ULONG
NTAPI
PhGetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID
    );

PHLIBAPI
VOID
NTAPI
PhSetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID,
    _In_ ULONG Value,
    _In_ BOOLEAN Signed
    );

PHLIBAPI
VOID
NTAPI
PhSetDialogItemText(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID,
    _In_ PCWSTR WindowText
    );

PHLIBAPI
VOID
NTAPI
PhSetWindowText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    );

PHLIBAPI
VOID
NTAPI
PhSetGroupBoxText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    );

PHLIBAPI
VOID
NTAPI
PhSetWindowAlwaysOnTop(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN AlwaysOnTop
    );

FORCEINLINE ULONG PhGetWindowTextLength(
    _In_ HWND WindowHandle
    )
{
    return (ULONG)SendMessage(WindowHandle, WM_GETTEXTLENGTH, 0, 0); // DefWindowProc
}

FORCEINLINE VOID PhSetDialogFocus(
    _In_ HWND WindowHandle,
    _In_ HWND FocusHandle
    )
{
    // Do not use the SendMessage function to send a WM_NEXTDLGCTL message if your application will
    // concurrently process other messages that set the focus. Use the PostMessage function instead.
    SendMessage(WindowHandle, WM_NEXTDLGCTL, (WPARAM)FocusHandle, MAKELPARAM(TRUE, 0));
}

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
    MapWindowRect(NULL, ParentWindowHandle, &windowRect);
    MoveWindow(ToControlHandle, windowRect.left, windowRect.top,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);
}

// icotobmp

PHLIBAPI
HBITMAP
NTAPI
PhIconToBitmap(
    _In_ HICON Icon,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
VOID
NTAPI
PhBitmapSetAlpha(
    _In_ PVOID Bits,
    _In_ LONG Width,
    _In_ LONG Height
    );

// extlv

#define PH_ALIGN_CENTER 0x0
#define PH_ALIGN_LEFT 0x1
#define PH_ALIGN_RIGHT 0x2
#define PH_ALIGN_TOP 0x4
#define PH_ALIGN_BOTTOM 0x8

#define PH_ALIGN_MONOSPACE_FONT 0x80000000

typedef enum _PH_ITEM_STATE
{
    // The item is normal. Use the ItemColorFunction to determine the color of the item.
    NormalItemState = 0,
    // The item is new. On the next tick, change the state to NormalItemState. When an item is in
    // this state, highlight it in NewColor.
    NewItemState,
    // The item is being removed. On the next tick, delete the item. When an item is in this state,
    // highlight it in RemovingColor.
    RemovingItemState
} PH_ITEM_STATE;

typedef COLORREF (NTAPI *PPH_EXTLV_GET_ITEM_COLOR)(
    _In_ LONG Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );

typedef HFONT (NTAPI *PPH_EXTLV_GET_ITEM_FONT)(
    _In_ LONG Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhSetExtendedListView(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhSetExtendedListViewEx(
    _In_ HWND WindowHandle,
    _In_ ULONG SortColumn,
    _In_ ULONG SortOrder
    );

PHLIBAPI
VOID
NTAPI
PhSetHeaderSortIcon(
    _In_ HWND hwnd,
    _In_ LONG Index,
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
#define ELVM_GETSORT (WM_APP + 1113)
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
#define ExtendedListView_GetSort(hWnd, Column, Order) \
    SendMessage((hWnd), ELVM_GETSORT, (WPARAM)(Column), (LPARAM)(Order))
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
 * \return A value ranging from 0 to 255, indicating the brightness of the color.
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

// Window support

typedef enum _PH_PLUGIN_WINDOW_EVENT_TYPE
{
    PH_PLUGIN_WINDOW_EVENT_TYPE_NONE,
    PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST,
    PH_PLUGIN_WINDOW_EVENT_TYPE_MAX
} PH_PLUGIN_WINDOW_EVENT_TYPE;

typedef struct _PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION
{
    HWND WindowHandle;
    PH_PLUGIN_WINDOW_EVENT_TYPE Type;
} PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION, *PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION;

typedef struct _PH_PLUGIN_WINDOW_NOTIFY_EVENT
{
    PH_PLUGIN_WINDOW_EVENT_TYPE Type;
    union
    {
        BOOLEAN TopMost;
        //HFONT FontHandle;
    };
} PH_PLUGIN_WINDOW_NOTIFY_EVENT, *PPH_PLUGIN_WINDOW_NOTIFY_EVENT;

typedef struct _PH_PLUGIN_MAINWINDOW_NOTIFY_EVENT
{
    PPH_PLUGIN_WINDOW_NOTIFY_EVENT Event;
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION Callback;
} PH_PLUGIN_MAINWINDOW_NOTIFY_EVENT, *PPH_PLUGIN_MAINWINDOW_NOTIFY_EVENT;

PHLIBAPI
VOID
NTAPI
PhRegisterWindowCallback(
    _In_ HWND WindowHandle,
    _In_ PH_PLUGIN_WINDOW_EVENT_TYPE Type,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhUnregisterWindowCallback(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhWindowNotifyTopMostEvent(
    _In_ BOOLEAN TopMost
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateEnvironmentBlock(
    _Out_ PVOID* Environment,
    _In_opt_ HANDLE TokenHandle,
    _In_ BOOLEAN Inherit
    );

PHLIBAPI
VOID
NTAPI
PhDestroyEnvironmentBlock(
    _In_ _Post_invalid_ PVOID Environment
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhRegenerateUserEnvironment(
    _Out_opt_ PVOID* NewEnvironment,
    _In_ BOOLEAN UpdateCurrentEnvironment
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsImmersiveProcess(
    _In_ HANDLE ProcessHandle
    );

typedef enum _PROCESS_UICONTEXT
{
    PROCESS_UICONTEXT_DESKTOP,
    PROCESS_UICONTEXT_IMMERSIVE,
    PROCESS_UICONTEXT_IMMERSIVE_BROKER,
    PROCESS_UICONTEXT_IMMERSIVE_BROWSER
} PROCESS_UICONTEXT;

typedef enum _PROCESS_UI_FLAGS
{
    PROCESS_UIF_NONE,
    PROCESS_UIF_AUTHORING_MODE,
    PROCESS_UIF_RESTRICTIONS_DISABLED
} PROCESS_UI_FLAGS;

typedef struct _PROCESS_UICONTEXT_INFORMATION
{
    PROCESS_UICONTEXT ProcessUIContext;
    PROCESS_UI_FLAGS Flags;
} PROCESS_UICONTEXT_INFORMATION, *PPROCESS_UICONTEXT_INFORMATION;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
    );

typedef enum _PH_PROCESS_DPI_AWARENESS
{
    PH_PROCESS_DPI_AWARENESS_UNAWARE = 0,
    PH_PROCESS_DPI_AWARENESS_SYSTEM_DPI_AWARE = 1,
    PH_PROCESS_DPI_AWARENESS_PER_MONITOR_DPI_AWARE = 2,
    PH_PROCESS_DPI_AWARENESS_PER_MONITOR_AWARE_V2 = 3,
    PH_PROCESS_DPI_AWARENESS_UNAWARE_GDISCALED = 4,
} PH_PROCESS_DPI_AWARENESS, *PPH_PROCESS_DPI_AWARENESS;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetProcessDpiAwareness(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_DPI_AWARENESS ProcessDpiAwareness
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetPhysicallyInstalledSystemMemory(
    _Out_ PULONGLONG TotalMemory,
    _Out_ PULONGLONG ReservedMemory
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSessionGuiResources(
    _In_ ULONG Flags,
    _Out_ PULONG Total
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessGuiResources(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _Out_ PULONG Total
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThreadWin32Thread(
    _In_ HANDLE ThreadId
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetSendMessageReceiver(
    _In_ HANDLE ThreadId,
    _Out_ HWND *WindowHandle
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhExtractIcon(
    _In_ PCWSTR FileName,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhExtractIconEx(
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName,
    _In_ LONG IconIndex,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall,
    _In_ LONG WindowDpi
    );

// Imagelist support

PHLIBAPI
HIMAGELIST
NTAPI
PhImageListCreate(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Flags,
    _In_ LONG InitialCount,
    _In_ LONG GrowCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListDestroy(
    _In_opt_ HIMAGELIST ImageListHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListSetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _In_ ULONG Count
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListGetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PLONG Count
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListSetBkColor(
    _In_ HIMAGELIST ImageListHandle,
    _In_ COLORREF BackgroundColor
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListRemoveIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index
    );

#define PhImageListRemoveAll(ImageListHandle) \
    PhImageListRemoveIcon((ImageListHandle), INT_ERROR)

PHLIBAPI
LONG
NTAPI
PhImageListAddIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HICON IconHandle
    );

PHLIBAPI
LONG
NTAPI
PhImageListAddBitmap(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    );

PHLIBAPI
HICON
NTAPI
PhImageListGetIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ ULONG Flags
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListGetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PLONG cx,
    _Out_ PLONG cy
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListReplace(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListDrawIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HDC Hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ UINT32 Style,
    _In_ BOOLEAN Disabled
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListDrawEx(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HDC Hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ LONG dx,
    _In_ LONG dy,
    _In_ COLORREF BackColor,
    _In_ COLORREF ForeColor,
    _In_ UINT32 Style,
    _In_ ULONG State
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListSetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG cx,
    _In_ LONG cy
    );

#define PH_SHUTDOWN_RESTART 0x1
#define PH_SHUTDOWN_POWEROFF 0x2
#define PH_SHUTDOWN_INSTALL_UPDATES 0x4
#define PH_SHUTDOWN_HYBRID 0x8
#define PH_SHUTDOWN_RESTART_BOOTOPTIONS 0x10

PHLIBAPI
ULONG
NTAPI
PhInitiateShutdown(
    _In_ ULONG Flags
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetProcessShutdownParameters(
    _In_ ULONG Level,
    _In_ ULONG Flags
    );

#define PH_DRAW_TIMELINE_OVERFLOW 0x1
#define PH_DRAW_TIMELINE_DARKTHEME 0x2

PHLIBAPI
VOID
NTAPI
PhCustomDrawTreeTimeLine(
    _In_ HDC Hdc,
    _In_ RECT CellRect,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER StartTime,
    _In_ PLARGE_INTEGER CreateTime
    );

// Windows Imaging Component (WIC) bitmap support

typedef enum _PH_BUFFERFORMAT
{
    PHBF_COMPATIBLEBITMAP,    // Compatible bitmap
    PHBF_DIB,                 // Device-independent bitmap
    PHBF_TOPDOWNDIB,          // Top-down device-independent bitmap
    PHBF_TOPDOWNMONODIB       // Top-down monochrome device-independent bitmap
} PH_BUFFERFORMAT;

HBITMAP PhCreateDIBSection(
    _In_ HDC Hdc,
    _In_ PH_BUFFERFORMAT Format,
    _In_ LONG Width,
    _In_ LONG Height,
    _Outptr_opt_ _When_(return != NULL, _Notnull_) PVOID* Bits
    );

typedef enum _PH_IMAGE_FORMAT_TYPE
{
    PH_IMAGE_FORMAT_TYPE_NONE,
    PH_IMAGE_FORMAT_TYPE_ICO,
    PH_IMAGE_FORMAT_TYPE_BMP,
    PH_IMAGE_FORMAT_TYPE_JPG,
    PH_IMAGE_FORMAT_TYPE_PNG,
} PH_IMAGE_FORMAT_TYPE, *PPH_IMAGE_FORMAT_TYPE;

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFormatFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ PH_IMAGE_FORMAT_TYPE Format,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFromFile(
    _In_ PCWSTR FileName,
    _In_ LONG Width,
    _In_ LONG Height
    );

// Acrylic support

typedef enum _WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_CORNER_STYLE = 27,
    WCA_PART_COLOR = 28,
    WCA_DISABLE_MOVESIZE_FEEDBACK = 29,
    WCA_SYSTEMBACKDROP_TYPE = 30,
    WCA_SET_TAGGED_WINDOW_RECT = 31,
    WCA_CLEAR_TAGGED_WINDOW_RECT = 32,
    WCA_LAST
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBUTEDATA
{
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID Data;
    SIZE_T Length;
} WINDOWCOMPOSITIONATTRIBUTEDATA, *PWINDOWCOMPOSITIONATTRIBUTEDATA;

PHLIBAPI
BOOLEAN
NTAPI
PhSetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _In_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
    );

// TODO: https://stackoverflow.com/questions/12304848/fast-algorithm-to-invert-an-argb-color-value-to-abgr/42133405#42133405
FORCEINLINE ULONG MakeARGB(
    _In_ BYTE a,
    _In_ BYTE r,
    _In_ BYTE g,
    _In_ BYTE b)
{
    return (((ULONG)(b) << 0) | ((ULONG)(g) << 8) | ((ULONG)(r) << 16) | ((ULONG)(a) << 24));
}

FORCEINLINE ULONG MakeABGR(
    _In_ BYTE a,
    _In_ BYTE b,
    _In_ BYTE g,
    _In_ BYTE r)
{
    return (((ULONG)(a) << 24) | ((ULONG)(b) << 16) | ((ULONG)(g) << 8) | ((ULONG)(r) << 0));
}

FORCEINLINE ULONG MakeARGBFromCOLORREF(_In_ BYTE Alpha, _In_ COLORREF rgb)
{
    return MakeARGB(Alpha, GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
}

FORCEINLINE ULONG MakeABGRFromCOLORREF(_In_ BYTE Alpha, _In_ COLORREF rgb)
{
    return MakeABGR(Alpha, GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}

typedef enum _ACCENT_STATE
{
    ACCENT_DISABLED,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5,
    ACCENT_INVALID_STATE
} ACCENT_STATE;

typedef enum _ACCENT_FLAG
{
    ACCENT_NONE,
    ACCENT_WINDOWS11_LUMINOSITY = 0x2,
    ACCENT_BORDER_LEFT = 0x20,
    ACCENT_BORDER_TOP = 0x40,
    ACCENT_BORDER_RIGHT = 0x80,
    ACCENT_BORDER_BOTTOM = 0x100,
    ACCENT_BORDER_ALL = (ACCENT_BORDER_LEFT | ACCENT_BORDER_TOP | ACCENT_BORDER_RIGHT | ACCENT_BORDER_BOTTOM)
} ACCENT_FLAG;

typedef struct _ACCENT_POLICY
{
    ACCENT_STATE AccentState;
    ULONG AccentFlags;
    ULONG GradientColor;
    ULONG AnimationId;
} ACCENT_POLICY;

PHLIBAPI
BOOLEAN
NTAPI
PhSetWindowAcrylicCompositionColor(
    _In_ HWND WindowHandle,
    _In_ ULONG GradientColor
    );

PHLIBAPI
HCURSOR
NTAPI
PhLoadArrowCursor(
    VOID
    );

PHLIBAPI
HCURSOR
NTAPI
PhLoadDividerCursor(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsInteractiveUserSession(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetCurrentWindowStationName(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetCurrentThreadDesktopName(
    VOID
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhRecentListCreate(
    _Out_ PHANDLE RecentHandle
    );

PHLIBAPI
VOID
NTAPI
PhRecentListDestroy(
    _In_ HANDLE RecentHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhRecentListAddCommand(
    _In_ PPH_STRINGREF Command
    );

typedef BOOLEAN (NTAPI* PPH_ENUM_MRULIST_CALLBACK)(
    _In_ PPH_STRINGREF Command,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhEnumerateRecentList(
    _In_ PPH_ENUM_MRULIST_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

#ifndef DBT_DEVICEARRIVAL
#define DBT_DEVICEARRIVAL        0x8000  // system detected a new device
#define DBT_DEVICEREMOVECOMPLETE 0x8004  // device is gone

#define DBT_DEVTYP_VOLUME        0x00000002  // logical volume

typedef struct _DEV_BROADCAST_HDR
{
    ULONG dbch_size;
    ULONG dbch_devicetype;
    ULONG dbch_reserved;
} DEV_BROADCAST_HDR, *PDEV_BROADCAST_HDR;
#endif

// theme support (theme.c)

extern HFONT PhApplicationFont;
extern HFONT PhTreeWindowFont;
extern HFONT PhMonospaceFont;
extern HBRUSH PhThemeWindowBackgroundBrush;
extern BOOLEAN PhEnableThemeSupport;
extern BOOLEAN PhEnableThemeAcrylicSupport;
extern BOOLEAN PhEnableThemeAcrylicWindowSupport;
extern BOOLEAN PhEnableThemeNativeButtons;
extern BOOLEAN PhEnableThemeListviewBorder;
extern COLORREF PhThemeWindowForegroundColor;
extern COLORREF PhThemeWindowBackgroundColor;
extern COLORREF PhThemeWindowBackground2Color;
extern COLORREF PhThemeWindowHighlightColor;
extern COLORREF PhThemeWindowHighlight2Color;
extern COLORREF PhThemeWindowTextColor;

PHLIBAPI
VOID
NTAPI
PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    );

PHLIBAPI
VOID
NTAPI
PhInitializeWindowThemeEx(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhReInitializeWindowTheme(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhInitializeThemeWindowFrame(
    _In_ HWND WindowHandle
    );

PHLIBAPI
HBRUSH
NTAPI
PhWindowThemeControlColor(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ HWND ChildWindowHandle,
    _In_ LONG Type
    );

PHLIBAPI
BOOLEAN
NTAPI
PhThemeWindowDrawItem(
    _In_ HWND WindowHandle,
    _In_ PDRAWITEMSTRUCT DrawInfo
    );

PHLIBAPI
LRESULT
NTAPI
PhThemeWindowDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    );

PHLIBAPI
BOOLEAN
NTAPI
PhThemeWindowMeasureItem(
    _In_ HWND WindowHandle,
    _In_ PMEASUREITEMSTRUCT DrawInfo
    );

PHLIBAPI
VOID
NTAPI
PhInitializeWindowThemeMainMenu(
    _In_ HMENU MenuHandle
    );

PHLIBAPI
LRESULT
CALLBACK
PhThemeWindowDrawRebar(
    _In_ LPNMCUSTOMDRAW DrawInfo
    );

PHLIBAPI
LRESULT
CALLBACK
PhThemeWindowDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    );

// Font support

FORCEINLINE
HFONT
NTAPI
PhCreateFont(
    _In_opt_ PCWSTR Name,
    _In_ ULONG Size,
    _In_ ULONG Weight,
    _In_ ULONG PitchAndFamily,
    _In_ LONG dpiValue
    )
{
    return CreateFont(
        -(LONG)PhMultiplyDivide(Size, dpiValue, 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        PitchAndFamily,
        Name
        );
}

FORCEINLINE
HFONT
NTAPI
PhCreateCommonFont(
    _In_ LONG Size,
    _In_ INT Weight,
    _In_opt_ HWND hwnd,
    _In_ LONG dpiValue
    )
{
    HFONT fontHandle;
    LOGFONT logFont;

    if (!PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, dpiValue))
        return NULL;

    fontHandle = CreateFont(
        -PhMultiplyDivideSigned(Size, dpiValue, 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        logFont.lfFaceName
        );

    if (!fontHandle)
        return NULL;

    if (hwnd)
        SetWindowFont(hwnd, fontHandle, TRUE);

    return fontHandle;
}

FORCEINLINE
HFONT
NTAPI
PhCreateIconTitleFont(
    _In_opt_ LONG WindowDpi
    )
{
    LOGFONT logFont;

    if (PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, WindowDpi))
        return CreateFontIndirect(&logFont);

    return NULL;
}

FORCEINLINE
HFONT
NTAPI
PhCreateMessageFont(
    _In_opt_ LONG WindowDpi
    )
{
    NONCLIENTMETRICS metrics = { sizeof(metrics) };

    if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, WindowDpi))
        return CreateFontIndirect(&metrics.lfMessageFont);

    return NULL;
}

FORCEINLINE
HFONT
NTAPI
PhDuplicateFont(
    _In_ HFONT Font
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
        return CreateFontIndirect(&logFont);

    return NULL;
}

FORCEINLINE
HFONT
NTAPI
PhDuplicateFontWithNewWeight(
    _In_ HFONT Font,
    _In_ LONG NewWeight
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
    {
        logFont.lfWeight = NewWeight;
        return CreateFontIndirect(&logFont);
    }

    return NULL;
}

FORCEINLINE
HFONT
NTAPI
PhDuplicateFontWithNewHeight(
    _In_ HFONT Font,
    _In_ LONG NewHeight,
    _In_ LONG dpiValue
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
    {
        logFont.lfHeight = PhGetDpi(NewHeight, dpiValue);
        return CreateFontIndirect(&logFont);
    }

    return NULL;
}

FORCEINLINE
BOOLEAN
NTAPI
PhRectEmpty(
    _In_ PRECT Rect
    )
{
#if defined(PHNT_NATIVE_RECT)
    return !!IsRectEmpty(Rect);
#else
    if (!Rect)
        return TRUE;

    if (Rect->left >= Rect->right || Rect->top >= Rect->bottom)
        return TRUE;

    return FALSE;
#endif
}

FORCEINLINE
VOID
NTAPI
PhInflateRect(
    _In_ PRECT Rect,
    _In_ LONG dx,
    _In_ LONG dy
    )
{
#if defined(PHNT_NATIVE_RECT)
    InflateRect(Rect, dx, dy);
#else
    Rect->left -= dx;
    Rect->top -= dy;
    Rect->right += dx;
    Rect->bottom += dy;
#endif
}

FORCEINLINE
VOID
NTAPI
PhOffsetRect(
    _In_ PRECT Rect,
    _In_ LONG dx,
    _In_ LONG dy
    )
{
#if defined(PHNT_NATIVE_RECT)
    OffsetRect(Rect, dx, dy);
#else
    Rect->left += dx;
    Rect->top += dy;
    Rect->right += dx;
    Rect->bottom += dy;
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
PhPtInRect(
    _In_ PRECT Rect,
    _In_ POINT Point
    )
{
#if defined(PHNT_NATIVE_RECT)
    return !!PtInRect(Rect, Point);
#else
    return Point.x >= Rect->left && Point.x < Rect->right &&
        Point.y >= Rect->top && Point.y < Rect->bottom;
#endif
}

VOID PhWindowThemeMainMenuBorder(
    _In_ HWND WindowHandle
    );

// directdraw.cpp

HICON PhGdiplusConvertBitmapToIcon(
    _In_ HBITMAP Bitmap,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ COLORREF Background
    );

HWND PhCreateBackgroundWindow(
    _In_ HWND ParentWindowHandle
    );

HICON PhGdiplusConvertHBitmapToHIcon(
    _In_ HBITMAP NitmapHandle
    );

EXTERN_C_END

#endif

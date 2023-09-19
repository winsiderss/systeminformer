/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2023
 *
 */

#ifndef _PH_GUISUPP_H
#define _PH_GUISUPP_H

typedef HTHEME (WINAPI* _OpenThemeDataForDpi)(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR ClassList,
    _In_ UINT DpiValue
    );

typedef HTHEME (WINAPI* _OpenThemeData)(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR ClassList
    );

typedef HRESULT (WINAPI* _CloseThemeData)(
    _In_ HTHEME ThemeHandle
    );

typedef HRESULT (WINAPI *_SetWindowTheme)(
    _In_ HWND WindowHandle,
    _In_opt_ PCWSTR SubAppName,
    _In_opt_ PCWSTR SubIdList
    );

typedef BOOL (WINAPI *_IsThemeActive)(
    VOID
    );

typedef BOOL (WINAPI *_IsThemePartDefined)(
    _In_ HTHEME hTheme,
    _In_ INT iPartId,
    _In_ INT iStateId
    );

typedef HRESULT (WINAPI *_DrawThemeBackground)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_ const RECT *pRect,
    _In_opt_ const RECT *pClipRect
    );

typedef HRESULT (WINAPI *_DrawThemeText)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_reads_(cchText) LPCWSTR pszText,
    _In_ INT cchText,
    _In_ ULONG dwTextFlags,
    _Reserved_ ULONG dwTextFlags2,
    _In_ LPCRECT pRect
    );

typedef HRESULT(WINAPI* _DrawThemeTextEx)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_reads_(cchText) LPCWSTR pszText,
    _In_ INT cchText,
    _In_ ULONG dwTextFlags,
    _Inout_ LPRECT pRect,
    _In_opt_ const PVOID pOptions // DTTOPTS*
    );

typedef HRESULT (WINAPI* _GetThemeClass)(
    _In_ HTHEME ThemeHandle,
    _Out_writes_z_(BufferLength) PWSTR Buffer,
    _In_ ULONG BufferLength
    );

typedef HRESULT (WINAPI *_GetThemeInt)(
    _In_ HTHEME hTheme,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_ INT iPropId,
    _Out_ INT *piVal
    );

typedef HRESULT (WINAPI* _GetThemePartSize)(
    _In_ HTHEME hTheme,
    _In_opt_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_opt_ LPCRECT prc,
    _In_ enum THEMESIZE eSize,
    _Out_ SIZE* psz
    );

typedef HRESULT (WINAPI* _GetDpiForMonitor)(
    _In_ HMONITOR hmonitor,
    _In_ enum MONITOR_DPI_TYPE dpiType,
    _Out_ PUINT dpiX,
    _Out_ PUINT dpiY
    );

typedef UINT (WINAPI* _GetDpiForWindow)(
    _In_ HWND hwnd
    );

typedef UINT (WINAPI* _GetDpiForSystem)(
    VOID
    );

typedef UINT (WINAPI* _GetDpiForSession)(
    VOID
    );

typedef INT (WINAPI* _GetSystemMetricsForDpi)(
    _In_ INT Index,
    _In_ UINT dpi
    );

typedef BOOL (WINAPI* _SystemParametersInfoForDpi)(
    _In_ UINT uiAction,
    _In_ UINT uiParam,
    _Pre_maybenull_ _Post_valid_ PVOID pvParam,
    _In_ UINT fWinIni,
    _In_ UINT dpi
    );

// Comctl32

typedef INT32 (CALLBACK *MRUSTRINGCMPPROC)(PCWSTR String1, PCWSTR String2);
typedef INT32 (CALLBACK *MRUINARYCMPPROC)(PVOID String1, PVOID String2, ULONG Length);

#define MRU_STRING 0x0000
#define MRU_BINARY 0x0001
#define MRU_CACHEWRITE 0x0002

typedef struct _MRUINFO
{
    ULONG cbSize;
    UINT uMaxItems;
    UINT uFlags;
    HANDLE hKey;
    PCWSTR lpszSubKey;
    MRUSTRINGCMPPROC lpfnCompare;
} MRUINFO, *PMRUINFO;

typedef HANDLE (WINAPI* _CreateMRUList)(
    _In_ PMRUINFO lpmi
    );
typedef INT32 (WINAPI* _AddMRUString)(
    _In_ HANDLE hMRU,
    _In_ PCWSTR szString
    );
typedef INT32 (WINAPI* _EnumMRUList)(
    _In_ HANDLE hMRU,
    _In_ INT nItem,
    _Out_ PVOID lpData,
    _In_ UINT uLen
    );
typedef INT32 (WINAPI* _FreeMRUList)(
    _In_ HANDLE hMRU
    );

// Icons

typedef struct _PHP_ICON_ENTRY
{
    PVOID InstanceHandle;
    PWSTR Name;
    ULONG Width;
    ULONG Height;
    HICON Icon;
    LONG DpiValue;
} PHP_ICON_ENTRY, *PPHP_ICON_ENTRY;

#define PHP_ICON_ENTRY_SIZE_SMALL (-1)
#define PHP_ICON_ENTRY_SIZE_LARGE (-2)

FORCEINLINE ULONG PhpGetIconEntrySize(
    _In_ ULONG InputSize,
    _In_ ULONG Flags
    )
{
    if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        return PHP_ICON_ENTRY_SIZE_SMALL;
    if (Flags & PH_LOAD_ICON_SIZE_LARGE)
        return PHP_ICON_ENTRY_SIZE_LARGE;
    return InputSize;
}

#endif

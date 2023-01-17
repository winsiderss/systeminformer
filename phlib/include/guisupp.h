/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2022
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
    _In_ PCWSTR SubAppName,
    _In_ PCWSTR SubIdList
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
    _In_ const RECT *pClipRect
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

typedef HRESULT (WINAPI* _GetThemeClass)(
    _In_ HTHEME ThemeHandle,
    _Out_writes_(BufferLength) PWSTR Buffer,
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

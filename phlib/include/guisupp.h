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

typedef HRESULT (WINAPI* _GetThemeClass)(
    _In_ HTHEME ThemeHandle,
    _Out_writes_z_(BufferLength) PWSTR Buffer,
    _In_ ULONG BufferLength
    );

typedef BOOL(WINAPI* _AllowDarkModeForWindow)(
    _In_ HWND WindowHandle,
    _In_ BOOL Enabled
    );

typedef BOOL(WINAPI* _IsDarkModeAllowedForWindow)(
    _In_ HWND WindowHandle
    );

typedef LONG (WINAPI* _GetDpiForSession)(
    VOID
    );

// Comctl32

typedef LONG (CALLBACK *MRUSTRINGCMPPROC)(PCWSTR String1, PCWSTR String2);
typedef LONG (CALLBACK *MRUINARYCMPPROC)(PVOID String1, PVOID String2, ULONG Length);

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
typedef LONG (WINAPI* _AddMRUString)(
    _In_ HANDLE hMRU,
    _In_ PCWSTR szString
    );
typedef LONG (WINAPI* _EnumMRUList)(
    _In_ HANDLE hMRU,
    _In_ INT nItem,
    _Out_ PVOID lpData,
    _In_ UINT uLen
    );
typedef LONG (WINAPI* _FreeMRUList)(
    _In_ HANDLE hMRU
    );

// Icons

typedef struct _PHP_ICON_ENTRY
{
    PVOID InstanceHandle;
    PCWSTR Name;
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

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <mapldr.h>

typedef HRESULT (WINAPI *PGET_THEME_DEFAULTS)(
    _In_ PCWSTR ThemeFileName,
    _Out_writes_(ColorNameLength) PWSTR ColorName,
    _In_ ULONG ColorNameLength,
    _Out_writes_(SizeNameLength) PWSTR SizeName,
    _In_ ULONG SizeNameLength
    );

struct _PH_THEME_FILE
{
    CHAR Header[7];
    PVOID SharedSectionView;
    HANDLE SharedSectionHandle;
    PVOID NonSharedSectionView;
    HANDLE NonSharedSectionHandle;
    CHAR End[3];
};

static_assert(FIELD_OFFSET(PH_THEME_FILE, SharedSectionView) == 8);
static_assert(FIELD_OFFSET(PH_THEME_FILE, NonSharedSectionView) == (sizeof(PVOID) == 8 ? 24 : 16));
static_assert(FIELD_OFFSET(PH_THEME_FILE, End) == (sizeof(PVOID) == 8 ? 40 : 24));
static_assert(sizeof(PH_THEME_FILE) == (sizeof(PVOID) == 8 ? 48 : 28));

static PGET_THEME_DEFAULTS PhpGetThemeDefaults = nullptr;
static decltype(&LoaderLoadTheme) PhpLoaderLoadTheme = nullptr;
static decltype(&OpenThemeDataFromFile) PhpOpenThemeDataFromFile = nullptr;

static BOOLEAN PhpThemeFileApiInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"uxtheme.dll"))
        {
            PhpGetThemeDefaults = reinterpret_cast<PGET_THEME_DEFAULTS>(
                PhGetDllBaseProcedureAddress(baseAddress, nullptr, 7));
            PhpLoaderLoadTheme = reinterpret_cast<decltype(&LoaderLoadTheme)>(
                PhGetDllBaseProcedureAddress(baseAddress, nullptr, UXTHEME_ORDINAL_LOADER_LOAD_THEME));
            PhpOpenThemeDataFromFile = reinterpret_cast<decltype(&OpenThemeDataFromFile)>(
                PhGetDllBaseProcedureAddress(baseAddress, nullptr, UXTHEME_ORDINAL_OPEN_THEME_DATA_FROM_FILE));
        }

        if (PhpGetThemeDefaults && PhpLoaderLoadTheme && PhpOpenThemeDataFromFile)
            initialized = TRUE;

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

VOID NTAPI PhUnloadThemeFile(
    _In_opt_ PPH_THEME_FILE ThemeFile
    )
{
    if (!ThemeFile)
        return;

    if (ThemeFile->NonSharedSectionView)
        UnmapViewOfFile(ThemeFile->NonSharedSectionView);

    if (ThemeFile->SharedSectionView)
        UnmapViewOfFile(ThemeFile->SharedSectionView);

    if (ThemeFile->NonSharedSectionHandle)
        NtClose(ThemeFile->NonSharedSectionHandle);

    if (ThemeFile->SharedSectionHandle)
        NtClose(ThemeFile->SharedSectionHandle);

    PhFree(ThemeFile);
}

HRESULT NTAPI PhLoadThemeFile(
    _In_ PCWSTR ThemeFileName,
    _Out_ PPH_THEME_FILE *ThemeFile
    )
{
    HRESULT result;
    WCHAR colorName[MAX_PATH];
    WCHAR sizeName[MAX_PATH];
    HANDLE sharedSectionHandle = nullptr;
    HANDLE nonSharedSectionHandle = nullptr;
    PPH_THEME_FILE themeFile;

    if (!PhpThemeFileApiInitialized())
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    result = PhpGetThemeDefaults(
        ThemeFileName,
        colorName,
        RTL_NUMBER_OF(colorName),
        sizeName,
        RTL_NUMBER_OF(sizeName)
        );

    if (!HR_SUCCESS(result))
        return result;

    result = PhpLoaderLoadTheme(
        nullptr,
        nullptr,
        ThemeFileName,
        colorName,
        sizeName,
        &sharedSectionHandle,
        nullptr,
        0,
        &nonSharedSectionHandle,
        nullptr,
        0,
        nullptr,
        nullptr,
        0,
        0
        );

    if (!HR_SUCCESS(result))
    {
        if (nonSharedSectionHandle)
            NtClose(nonSharedSectionHandle);

        if (sharedSectionHandle)
            NtClose(sharedSectionHandle);

        return result;
    }

    themeFile = static_cast<PPH_THEME_FILE>(PhAllocateZero(sizeof(PH_THEME_FILE)));
    memcpy(themeFile->Header, "thmfile", sizeof(themeFile->Header));
    memcpy(themeFile->End, "end", sizeof(themeFile->End));
    themeFile->SharedSectionHandle = sharedSectionHandle;
    themeFile->NonSharedSectionHandle = nonSharedSectionHandle;
    themeFile->SharedSectionView = MapViewOfFile(
        sharedSectionHandle,
        FILE_MAP_READ,
        0,
        0,
        0
        );

    if (!themeFile->SharedSectionView)
    {
        result = HRESULT_FROM_WIN32(GetLastError());
        PhUnloadThemeFile(themeFile);
        return result;
    }

    themeFile->NonSharedSectionView = MapViewOfFile(
        nonSharedSectionHandle,
        FILE_MAP_READ,
        0,
        0,
        0
        );

    if (!themeFile->NonSharedSectionView)
    {
        result = HRESULT_FROM_WIN32(GetLastError());
        PhUnloadThemeFile(themeFile);
        return result;
    }

    *ThemeFile = themeFile;

    return S_OK;
}

HTHEME NTAPI PhOpenThemeDataFromFile(
    _In_ PPH_THEME_FILE ThemeFile,
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR ClassList,
    _In_ ULONG Flags
    )
{
    if (!PhpThemeFileApiInitialized())
    {
        RtlSetLastWin32Error(ERROR_PROC_NOT_FOUND);
        return nullptr;
    }

    return PhpOpenThemeDataFromFile(
        reinterpret_cast<HTHEMEFILE>(ThemeFile),
        WindowHandle,
        ClassList,
        Flags
        );
}

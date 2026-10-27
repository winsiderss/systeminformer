/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2023
 *     jxy-s   2023
 *
 */

#include <peview.h>

/**
 * \brief Creates a search control for the PE viewer.
 *
 * \param ParentWindowHandle A handle to the parent window.
 * \param WindowHandle A handle to the search edit control.
 * \param BannerText An optional string for the cue banner text.
 * \param Callback A callback function that is invoked when the search text changes.
 * \param Context An optional user-defined value passed to the callback function.
 */
VOID PvCreateSearchControl(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PPH_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PhCreateSearchControlEx(
        ParentWindowHandle,
        WindowHandle,
        BannerText,
        NtCurrentImageBase(),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_DARK),
        NULL,
        L"SearchControlRegex",
        L"SearchControlCaseSensitive",
        L"SearchControlFuzzy",
        Callback,
        Context
        );
}

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2023
 *     jxy-s   2023-2024
 *
 */

#include <phapp.h>

VOID PhCreateSearchControl(
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
        PhInstanceHandle,
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_DARK),
        L"SearchControlRegex",
        L"SearchControlCaseSensitive",
        Callback,
        Context
        );
}

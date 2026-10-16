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
#include <phsettings.h>

/**
 * \brief Creates a search control for System Informer.
 *
 * \param ParentWindowHandle A handle to the parent window.
 * \param SearchWindowHandle A handle to the search edit control.
 * \param BannerText An optional string for the cue banner text.
 * \param Callback A callback function that is invoked when the search text changes.
 * \param Context An optional user-defined value passed to the callback function.
 */
VOID PhCreateSearchControl(
    _In_ HWND ParentWindowHandle,
    _In_ HWND SearchWindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PPH_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PhCreateSearchControl2(
        ParentWindowHandle,
        SearchWindowHandle,
        BannerText,
        SETTING_SEARCH_CONTROL_REGEX,
        SETTING_SEARCH_CONTROL_CASE_SENSITIVE,
        Callback,
        Context
        );
}

/**
 * \brief Creates a search control for System Informer.
 *
 * \param ParentWindowHandle A handle to the parent window.
 * \param SearchWindowHandle A handle to the search edit control.
 * \param BannerText An optional string for the cue banner text.
 * \param RegexSetting The registered integer setting used to persist regex mode.
 * \param CaseSetting The registered integer setting used to persist case-sensitive mode.
 * Both setting strings must remain valid for the lifetime of the control.
 * \param Callback A callback function that is invoked when the search text changes.
 * \param Context An optional user-defined value passed to the callback function.
 */
VOID PhCreateSearchControl2(
    _In_ HWND ParentWindowHandle,
    _In_ HWND SearchWindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PCWSTR RegexSetting,
    _In_ PCWSTR CaseSetting,
    _In_ PPH_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PhCreateSearchControlEx(
        ParentWindowHandle,
        SearchWindowHandle,
        BannerText,
        NtCurrentImageBase(),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_REGEX_MODERN_DARK),
        PhEnableThemeSupport ? MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_LIGHT) : MAKEINTRESOURCE(IDB_SEARCH_CASE_MODERN_DARK),
        NULL,
        RegexSetting,
        CaseSetting,
        SETTING_SEARCH_CONTROL_FUZZY,
        Callback,
        Context
        );
}

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     tianxing    2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <settings.h>
#include <phi18n.h>

// Default language initialization
static WCHAR PhCurrentLanguageCode[16] = L"en-US";

/**
 * Initializes the multi-language engine and detects system UI language
 */
PHLIBAPI
VOID
NTAPI
PhInitializeI18n(
    VOID
    )
{
    const PPH_STRING languageSetting = PhGetStringSetting(L"Language");
    if (!PhIsNullOrEmptyString(languageSetting))
    {
        wcsncpy_s(PhCurrentLanguageCode, sizeof(PhCurrentLanguageCode) / sizeof(WCHAR), languageSetting->Buffer, _TRUNCATE);
        PhDereferenceObject(languageSetting);
    }
    else
    {
        LANGID langId = GetUserDefaultUILanguage();
        if (PRIMARYLANGID(langId) == LANG_CHINESE)
        {
            wcsncpy_s(PhCurrentLanguageCode, sizeof(PhCurrentLanguageCode) / sizeof(WCHAR), L"zh-CN", _TRUNCATE);
        }
        else
        {
            wcsncpy_s(PhCurrentLanguageCode, sizeof(PhCurrentLanguageCode) / sizeof(WCHAR), L"en-US", _TRUNCATE);
        }
    }
}

// Built-in initial localization dictionary
static PH_I18N_ENTRY PhChineseTranslations[] =
{
    { L"&System", L"系统" },
    { L"&View", L"视图" },
    { L"&Tools", L"工具" },
    { L"&Users", L"用户" },
    { L"&Help", L"帮助" },
    { L"&Language", L"语言" },
    { L"English", L"English" },
    { L"简体中文", L"简体中文" }
};

PHLIBAPI
PCWSTR
NTAPI
PhGetLocalizedString(
    _In_ const PCWSTR String
    )
{
    if (!String)
        return String;

    // Fallback directly to original English string if active language is en-US
    if (PhEqualStringZ(PhCurrentLanguageCode, L"en-US", TRUE))
    {
        return String;
    }

    // Lookup in active translation table
    for (ULONG i = 0; i < sizeof(PhChineseTranslations) / sizeof(PhChineseTranslations[0]); i++)
    {
        if (PhEqualStringZ(String, PhChineseTranslations[i].Key, FALSE))
        {
            return PhChineseTranslations[i].Value;
        }
    }

    // Fallback to original string if no translation match is found
    return String;
}

/**
 * Sets current active language code
 */
PHLIBAPI
VOID
NTAPI
PhSetCurrentLanguage(
    _In_ const PCWSTR LanguageCode
    )
{
    if (LanguageCode)
    {
        wcsncpy_s(PhCurrentLanguageCode, sizeof(PhCurrentLanguageCode) / sizeof(WCHAR), LanguageCode, _TRUNCATE);
    }
}

/**
 * Retrieves current active language code
 */
PHLIBAPI
PCWSTR
NTAPI
PhGetCurrentLanguage(
    VOID
    )
{
    return PhCurrentLanguageCode;
}

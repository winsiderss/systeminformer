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

#ifndef PH_PHI18N_H
#define PH_PHI18N_H

EXTERN_C_START

typedef struct PH_I18N_ENTRY
{
    PCWSTR Key;
    PCWSTR Value;
} PH_I18N_ENTRY, *PPH_I18N_ENTRY;

/**
 * Initializes the multi-language engine and automatically detects the Windows system UI language
 */
PHLIBAPI
VOID
NTAPI
PhInitializeI18n(
    VOID
    );

/**
 * Retrieves the localized version of the specified string
 *
 * \param String Input string to look up
 * \return Localized string pointer or original string
 */
PHLIBAPI
PCWSTR
NTAPI
PhGetLocalizedString(
    _In_ PCWSTR String
    );

/**
 * Dynamically sets the current active language code
 *
 * \param LanguageCode Target language code string
 */
PHLIBAPI
VOID
NTAPI
PhSetCurrentLanguage(
    _In_ PCWSTR LanguageCode
    );

/**
 * Retrieves the current active language code
 *
 * \return Pointer to the current language code string
 */
PHLIBAPI
PCWSTR
NTAPI
PhGetCurrentLanguage(
    VOID
    );

#define PH_I18N(String) PhGetLocalizedString(String)

EXTERN_C_END

#endif

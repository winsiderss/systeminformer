/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

#include <phapp.h>
#include <phsettings.h>
#include <settings.h>
#include <verify.h>

static HMODULE PhLocalizationResourceModule = NULL;
static PPH_STRING PhLocalizationStringCache[PH_LOCALIZATION_STRING_LIMIT] = { 0 };

static BOOLEAN PhpIsValidLocaleName(
    _In_ PCPH_STRINGREF LocaleName
    )
{
    SIZE_T count;

    count = LocaleName->Length / sizeof(WCHAR);

    if (count == 0 || count >= LOCALE_NAME_MAX_LENGTH)
        return FALSE;

    for (SIZE_T i = 0; i < count; i++)
    {
        WCHAR character = LocaleName->Buffer[i];

        if (!(
            (character >= L'a' && character <= L'z') ||
            (character >= L'A' && character <= L'Z') ||
            (character >= L'0' && character <= L'9') ||
            character == L'-'
            ))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOLEAN PhpIsEnglishLocale(
    _In_ PCPH_STRINGREF LocaleName
    )
{
    SIZE_T count = LocaleName->Length / sizeof(WCHAR);

    return count >= 2 &&
        (LocaleName->Buffer[0] == L'e' || LocaleName->Buffer[0] == L'E') &&
        (LocaleName->Buffer[1] == L'n' || LocaleName->Buffer[1] == L'N') &&
        (count == 2 || LocaleName->Buffer[2] == L'-');
}

static PPH_STRING PhpGetUserDefaultLocaleName(
    VOID
    )
{
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
    INT localeNameLength;

    localeNameLength = GetUserDefaultLocaleName(
        localeName,
        RTL_NUMBER_OF(localeName)
        );

    if (localeNameLength <= 1)
        return NULL;

    return PhCreateStringEx(
        localeName,
        (localeNameLength - 1) * sizeof(WCHAR)
        );
}

static BOOLEAN PhpIsLocalizationResourceTrusted(
    _In_ PPH_STRING FileName
    )
{
#ifdef DEBUG
    return TRUE;
#else
    BOOLEAN trusted = FALSE;
    VERIFY_RESULT applicationVerifyResult;
    VERIFY_RESULT translationVerifyResult;
    PPH_STRING applicationFileName = NULL;
    PPH_STRING applicationSignerName = NULL;
    PPH_STRING translationSignerName = NULL;

    // Community builds without KSI can load unsigned translation resources for local development.
    // 未启用 KSI 的社区构建可加载未签名翻译资源，便于本地开发。
    if (!PhEnableKsiSupport)
        return TRUE;

    applicationFileName = PhGetApplicationFileNameWin32();

    if (!applicationFileName)
        goto CleanupExit;

    applicationVerifyResult = PhVerifyFile(
        applicationFileName->Buffer,
        &applicationSignerName
        );
    translationVerifyResult = PhVerifyFile(
        FileName->Buffer,
        &translationSignerName
        );

    trusted = applicationVerifyResult == VrTrusted &&
        translationVerifyResult == VrTrusted &&
        !PhIsNullOrEmptyString(applicationSignerName) &&
        !PhIsNullOrEmptyString(translationSignerName) &&
        PhEqualString(applicationSignerName, translationSignerName, TRUE);

CleanupExit:
    if (translationSignerName)
        PhDereferenceObject(translationSignerName);
    if (applicationSignerName)
        PhDereferenceObject(applicationSignerName);
    if (applicationFileName)
        PhDereferenceObject(applicationFileName);

    return trusted;
#endif
}

static PPH_STRING PhpLoadLocalizedString(
    _In_ ULONG ResourceId
    )
{
    HRSRC resourceHandle;
    HGLOBAL resourceData;
    PWSTR resourceBlock;
    PBYTE resourceEnd;
    ULONG resourceSize;
    USHORT stringLength;
    ULONG stringIndex;

    resourceHandle = FindResource(
        PhLocalizationResourceModule,
        MAKEINTRESOURCE((ResourceId >> 4) + 1),
        RT_STRING
        );

    if (!resourceHandle)
        return NULL;

    resourceSize = SizeofResource(
        PhLocalizationResourceModule,
        resourceHandle
        );

    if (resourceSize < sizeof(USHORT))
        return NULL;

    resourceData = LoadResource(PhLocalizationResourceModule, resourceHandle);

    if (!resourceData)
        return NULL;

    resourceBlock = LockResource(resourceData);

    if (!resourceBlock)
        return NULL;

    resourceEnd = PTR_ADD_OFFSET(resourceBlock, resourceSize);
    stringIndex = ResourceId & 0xf;

    for (ULONG i = 0; i <= stringIndex; i++)
    {
        if ((SIZE_T)(resourceEnd - (PBYTE)resourceBlock) < sizeof(USHORT))
            return NULL;

        stringLength = resourceBlock[0];
        resourceBlock++;

        if ((SIZE_T)(resourceEnd - (PBYTE)resourceBlock) < stringLength * sizeof(WCHAR))
            return NULL;

        if (i == stringIndex)
            break;

        resourceBlock += stringLength;
    }

    if (stringLength == 0)
        return NULL;

    return PhCreateStringEx(
        resourceBlock,
        stringLength * sizeof(WCHAR)
        );
}

VOID PhInitializeLocalization(
    VOID
    )
{
    static CONST PH_STRINGREF automaticLocale = PH_STRINGREF_INIT(L"auto");
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_STRING configuredLocale;
    PPH_STRING localeName = NULL;
    PPH_STRING relativeFileName = NULL;
    PPH_STRING translationFileName = NULL;

    if (!PhBeginInitOnce(&initOnce))
        return;

    configuredLocale = PhGetStringSetting(SETTING_LANGUAGE);

    if (PhEqualStringRef(&configuredLocale->sr, &automaticLocale, TRUE))
        localeName = PhpGetUserDefaultLocaleName();
    else
        localeName = PhReferenceObject(configuredLocale);

    PhDereferenceObject(configuredLocale);

    if (!localeName ||
        !PhpIsValidLocaleName(&localeName->sr) ||
        PhpIsEnglishLocale(&localeName->sr))
    {
        goto CleanupExit;
    }

    relativeFileName = PhFormatString(
        L"translations\\SystemInformer.%s.dll",
        localeName->Buffer
        );
    translationFileName = PhGetApplicationDirectoryFileName(
        &relativeFileName->sr,
        FALSE
        );

    if (!translationFileName ||
        !PhpIsLocalizationResourceTrusted(translationFileName))
    {
        goto CleanupExit;
    }

    PhLocalizationResourceModule = LoadLibraryEx(
        translationFileName->Buffer,
        NULL,
        LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE
        );

CleanupExit:
    if (translationFileName)
        PhDereferenceObject(translationFileName);
    if (relativeFileName)
        PhDereferenceObject(relativeFileName);
    if (localeName)
        PhDereferenceObject(localeName);

    PhEndInitOnce(&initOnce);
}

PCWSTR PhGetLocalizedString(
    _In_ ULONG ResourceId,
    _In_ PCWSTR Fallback
    )
{
    ULONG cacheIndex;
    PPH_STRING localizedString;
    PPH_STRING cachedString;

    if (!PhLocalizationResourceModule ||
        ResourceId < PH_LOCALIZATION_STRING_BASE ||
        ResourceId >= PH_LOCALIZATION_STRING_BASE + PH_LOCALIZATION_STRING_LIMIT)
    {
        return Fallback;
    }

    cacheIndex = ResourceId - PH_LOCALIZATION_STRING_BASE;
    cachedString = InterlockedCompareExchangePointer(
        &PhLocalizationStringCache[cacheIndex],
        NULL,
        NULL
        );

    if (cachedString)
        return cachedString->Buffer;

    localizedString = PhpLoadLocalizedString(ResourceId);

    if (!localizedString)
        return Fallback;

    cachedString = InterlockedCompareExchangePointer(
        &PhLocalizationStringCache[cacheIndex],
        localizedString,
        NULL
        );

    if (cachedString)
    {
        PhDereferenceObject(localizedString);
        return cachedString->Buffer;
    }

    return localizedString->Buffer;
}

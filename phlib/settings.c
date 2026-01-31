/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *
 */

/*
 * This file contains a program-specific settings system. All possible
 * settings are defined at program startup and added to a hashtable.
 * The values of these settings can then be read in from a XML file or
 * saved to a XML file at any time. Settings which are not recognized
 * are added to a list of "ignored settings"; this is necessary to
 * support plugin settings, as we don't want their settings to get
 * deleted whenever the plugins are disabled.
 *
 * The get/set functions are very strict. If the wrong function is used
 * (the get-integer-setting function is used on a string setting) or
 * the setting does not exist, an exception will be raised.
 */

#include <ph.h>
#include <apiimport.h>
#include <guisup.h>
#include <guisupview.h>
#include <mapimg.h>
#include <mapldr.h>
#include <thirdparty.h>
#include <settings.h>
#include <json.h>
#include <filestream.h>

PPH_HASHTABLE PhSettingsHashtable;
PH_QUEUED_LOCK PhSettingsLock = PH_QUEUED_LOCK_INIT;
PPH_LIST PhIgnoredSettings;

// Settings store descriptors (priority order: lower = higher priority)
static const PH_SETTINGS_STORE_DESCRIPTOR PhSettingsStores[] =
{
    { SettingsFormatJson, L".json", TRUE,   TRUE,   FALSE, 1 },
    { SettingsFormatXml, L".xml",   TRUE,   FALSE,  TRUE,  2 },
    { SettingsFormatKey, L".dat",   TRUE,   FALSE,  FALSE, 3 },
    { SettingsFormatReg, NULL,      FALSE,  FALSE,  FALSE, 4 },
    { SettingsFormatBin, L".bin",   TRUE,   FALSE,  FALSE, 5 },
};

#define PH_SETTINGS_STORE_COUNT RTL_NUMBER_OF(PhSettingsStores)

// Track which format was loaded
PH_SETTINGS_FORMAT PhSettingsLoadedFormat = SettingsFormatJson;

#define PH_SETTINGS_BIN_SIGNATURE 0x4E485042 // 'BPHN'
#define PH_SETTINGS_BIN_VERSION 1

#include <pshpack1.h>
typedef struct _PH_SETTINGS_BIN_HEADER
{
    ULONG Signature;
    ULONG Version;
    ULONG NumberOfSettings;
} PH_SETTINGS_BIN_HEADER, *PPH_SETTINGS_BIN_HEADER;

typedef struct _PH_SETTINGS_BIN_SETTING
{
    ULONG NameLength; // in bytes
    ULONG Type;
    ULONG ValueLength; // in bytes
} PH_SETTINGS_BIN_SETTING, *PPH_SETTINGS_BIN_SETTING;
#include <poppack.h>

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN NTAPI PhSettingsHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_SETTING setting1 = (PPH_SETTING)Entry1;
    PPH_SETTING setting2 = (PPH_SETTING)Entry2;

    return PhEqualStringRef(&setting1->Name, &setting2->Name, TRUE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG NTAPI PhSettingsHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_SETTING setting = (PPH_SETTING)Entry;

    return PhHashStringRefEx(&setting->Name, TRUE, PH_STRING_HASH_XXH32);
}

VOID PhSettingsInitialization(
    VOID
    )
{
    PhSettingsHashtable = PhCreateHashtable(
        sizeof(PH_SETTING),
        PhSettingsHashtableEqualFunction,
        PhSettingsHashtableHashFunction,
        512
        );
    PhIgnoredSettings = PhCreateList(1);
}

PPH_STRING PhSettingToString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    )
{
    switch (Type)
    {
    case StringSettingType:
        {
            if (!Setting->u.Pointer)
                return PhReferenceEmptyString();

            PhReferenceObject(Setting->u.Pointer);

            return (PPH_STRING)Setting->u.Pointer;
        }
        break;
    case IntegerSettingType:
        {
            PH_FORMAT format[1];

            // %x
            PhInitFormatX(&format[0], Setting->u.Integer);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        break;
    case IntegerPairSettingType:
        {
            PPH_INTEGER_PAIR integerPair = &Setting->u.IntegerPair;
            PH_FORMAT format[3];

            // %ld,%ld
            PhInitFormatD(&format[0], integerPair->X);
            PhInitFormatC(&format[1], L',');
            PhInitFormatD(&format[2], integerPair->Y);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        break;
    case ScalableIntegerPairSettingType:
        {
            PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = Setting->u.Pointer;
            PH_FORMAT format[6];

            if (!scalableIntegerPair)
                return PhReferenceEmptyString();

            // @%lu|%ld,%ld
            PhInitFormatC(&format[0], L'@');
            PhInitFormatU(&format[1], scalableIntegerPair->Scale);
            PhInitFormatC(&format[2], L'|');
            PhInitFormatD(&format[3], scalableIntegerPair->X);
            PhInitFormatC(&format[4], L',');
            PhInitFormatD(&format[5], scalableIntegerPair->Y);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        break;
    }

    return PhReferenceEmptyString();
}

BOOLEAN PhSettingFromString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PCPH_STRINGREF StringRef,
    _In_opt_ PPH_STRING String,
    _Inout_ PPH_SETTING Setting
    )
{
    switch (Type)
    {
    case StringSettingType:
        {
            if (String)
            {
                PhSetReference(&Setting->u.Pointer, String);
            }
            else
            {
                Setting->u.Pointer = PhCreateString2(StringRef);
            }

            return TRUE;
        }
        break;
    case IntegerSettingType:
        {
            ULONG64 integer;

            if (PhStringToInteger64(StringRef, 16, &integer))
            {
                Setting->u.Integer = (ULONG)integer;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        break;
    case IntegerPairSettingType:
        {
            LONG64 x;
            LONG64 y;
            PH_STRINGREF xString;
            PH_STRINGREF yString;

            if (!PhSplitStringRefAtChar(StringRef, L',', &xString, &yString))
                return FALSE;

            if (PhStringToInteger64(&xString, 10, &x) && PhStringToInteger64(&yString, 10, &y))
            {
                Setting->u.IntegerPair.X = (LONG)x;
                Setting->u.IntegerPair.Y = (LONG)y;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        break;
    case ScalableIntegerPairSettingType:
        {
            LONG64 scale;
            LONG64 x;
            LONG64 y;
            PH_STRINGREF stringRef;
            PH_STRINGREF firstPart;
            PH_STRINGREF secondPart;
            PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair;

            stringRef = *StringRef;

            if (stringRef.Length != 0 && stringRef.Buffer[0] == L'@')
            {
                PhSkipStringRef(&stringRef, sizeof(WCHAR));

                if (!PhSplitStringRefAtChar(&stringRef, L'|', &firstPart, &stringRef))
                    return FALSE;
                if (!PhStringToInteger64(&firstPart, 10, &scale))
                    return FALSE;
            }
            else
            {
                scale = USER_DEFAULT_SCREEN_DPI;
            }

            if (!PhSplitStringRefAtChar(&stringRef, L',', &firstPart, &secondPart))
                return FALSE;

            if (PhStringToInteger64(&firstPart, 10, &x) && PhStringToInteger64(&secondPart, 10, &y))
            {
                scalableIntegerPair = PhAllocateZero(sizeof(PH_SCALABLE_INTEGER_PAIR));
                scalableIntegerPair->X = (LONG)x;
                scalableIntegerPair->Y = (LONG)y;
                scalableIntegerPair->Scale = (ULONG)scale;
                Setting->u.Pointer = scalableIntegerPair;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        break;
    }

    return FALSE;
}

static VOID PhpFreeSettingValue(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    )
{
    switch (Type)
    {
    case StringSettingType:
        PhClearReference(&Setting->u.Pointer);
        break;
    case ScalableIntegerPairSettingType:
        PhFree(Setting->u.Pointer);
        Setting->u.Pointer = NULL;
        break;
    }
}

static PVOID PhpLookupSetting(
    _In_ PCPH_STRINGREF Name
    )
{
    PH_SETTING lookupSetting;
    PPH_SETTING setting;

    lookupSetting.Name = *Name;
    setting = (PPH_SETTING)PhFindEntryHashtable(
        PhSettingsHashtable,
        &lookupSetting
        );

    return setting;
}

VOID PhEnumSettings(
    _In_ PPH_SETTINGS_ENUM_CALLBACK Callback,
    _In_ PVOID Context
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        if (!Callback(setting, Context))
            break;
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

ULONG PhGetIntegerStringRefSetting(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_SETTING setting;
    ULONG value;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting && setting->Type == IntegerSettingType);

    if (setting && setting->Type == IntegerSettingType)
    {
        value = setting->u.Integer;
    }
    else
    {
        value = 0;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    return value;
}

BOOLEAN PhGetIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _Out_ PPH_INTEGER_PAIR IntegerPair
    )
{
    BOOLEAN result;
    PPH_SETTING setting;
    PH_INTEGER_PAIR value;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting && setting->Type == IntegerPairSettingType);

    if (setting && setting->Type == IntegerPairSettingType)
    {
        value = setting->u.IntegerPair;
        result = TRUE;
    }
    else
    {
        RtlZeroMemory(&value, sizeof(PH_INTEGER_PAIR));
        result = FALSE;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    RtlZeroMemory(IntegerPair, sizeof(PH_INTEGER_PAIR));
    RtlCopyMemory(IntegerPair, &value, sizeof(PH_INTEGER_PAIR));

    return result;
}

BOOLEAN PhGetScalableIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ BOOLEAN ScaleToDpi,
    _In_ LONG Dpi,
    _Out_ PPH_SCALABLE_INTEGER_PAIR* ScalableIntegerPair
    )
{
    BOOLEAN result;
    PPH_SETTING setting;
    PPH_SCALABLE_INTEGER_PAIR value;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting && setting->Type == ScalableIntegerPairSettingType);

    if (setting && setting->Type == ScalableIntegerPairSettingType)
    {
        value = setting->u.Pointer;
        result = TRUE;
    }
    else
    {
        value = NULL;
        result = FALSE;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    if (ScaleToDpi)
    {
        if (value->Scale != Dpi && value->Scale != 0)
        {
            value->X = PhMultiplyDivideSigned(value->X, Dpi, value->Scale);
            value->Y = PhMultiplyDivideSigned(value->Y, Dpi, value->Scale);
            value->Scale = Dpi;
        }
    }

    *ScalableIntegerPair = value;

    return result;
}

PPH_STRING PhGetStringRefSetting(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_SETTING setting;
    PPH_STRING value = NULL;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting && setting->Type == StringSettingType);

    if (setting && setting->Type == StringSettingType)
    {
        if (setting->u.Pointer)
        {
            PhSetReference(&value, setting->u.Pointer);
        }
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    if (PhIsNullOrEmptyString(value))
    {
        value = PhReferenceEmptyString();
    }

    return value;
}

BOOLEAN PhGetBinarySetting(
    _In_ PCPH_STRINGREF Name,
    _Out_ PVOID Buffer
    )
{
    PPH_STRING setting;
    BOOLEAN result;

    setting = PhGetStringRefSetting(Name);
    result = PhHexStringToBuffer(&setting->sr, (PUCHAR)Buffer);
    PhDereferenceObject(setting);

    return result;
}

VOID PhSetIntegerStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ ULONG Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting);

    if (setting && setting->Type == IntegerSettingType)
    {
        setting->u.Integer = Value;
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

VOID PhSetIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ PPH_INTEGER_PAIR Value
    )
{
    PPH_SETTING setting;
    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting);

    if (setting && setting->Type == IntegerPairSettingType)
    {
        memcpy(&setting->u.IntegerPair, Value, sizeof(PH_INTEGER_PAIR));
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

VOID PhSetScalableIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ PPH_SCALABLE_INTEGER_PAIR Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting);

    if (setting && setting->Type == ScalableIntegerPairSettingType)
    {
        PhpFreeSettingValue(ScalableIntegerPairSettingType, setting);
        setting->u.Pointer = PhAllocateCopy(Value, sizeof(PH_SCALABLE_INTEGER_PAIR));
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

VOID PhSetScalableIntegerPairStringRefSetting2(
    _In_ PCPH_STRINGREF Name,
    _In_ PPH_INTEGER_PAIR Value,
    _In_ LONG dpiValue
    )
{
    PH_SCALABLE_INTEGER_PAIR scalableIntegerPair;

    ZeroMemory(&scalableIntegerPair, sizeof(PH_SCALABLE_INTEGER_PAIR));
    memcpy(&scalableIntegerPair.Pair, Value, sizeof(PH_INTEGER_PAIR));
    scalableIntegerPair.Scale = dpiValue;

    PhSetScalableIntegerPairStringRefSetting(Name, &scalableIntegerPair);
}

VOID PhSetStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);
    assert(setting);

    if (setting && setting->Type == StringSettingType)
    {
        PhpFreeSettingValue(StringSettingType, setting);
        setting->u.Pointer = PhCreateString2(Value);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

VOID PhpFreeIgnoredSetting(
    _In_ PPH_SETTING Setting
    )
{
    PhFree(Setting->Name.Buffer);
    PhDereferenceObject(Setting->u.Pointer);

    PhFree(Setting);
}

VOID PhpClearIgnoredSettings(
    VOID
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    for (i = 0; i < PhIgnoredSettings->Count; i++)
    {
        PhpFreeIgnoredSetting(PhIgnoredSettings->Items[i]);
    }

    PhClearList(PhIgnoredSettings);

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

VOID PhClearIgnoredSettings(
    VOID
    )
{
    PhpClearIgnoredSettings();
}

ULONG PhCountIgnoredSettings(
    VOID
    )
{
    ULONG count;

    PhAcquireQueuedLockShared(&PhSettingsLock);
    count = PhIgnoredSettings->Count;
    PhReleaseQueuedLockShared(&PhSettingsLock);

    return count;
}

VOID PhConvertIgnoredSettings(
    VOID
    )
{
    PPH_SETTING ignoredSetting;
    PPH_SETTING setting;
    ULONG i;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    for (i = 0; i < PhIgnoredSettings->Count; i++)
    {
        ignoredSetting = PhIgnoredSettings->Items[i];

        setting = PhpLookupSetting(&ignoredSetting->Name);

        if (setting)
        {
            PhpFreeSettingValue(setting->Type, setting);

            if (!PhSettingFromString(
                setting->Type,
                &((PPH_STRING)ignoredSetting->u.Pointer)->sr,
                ignoredSetting->u.Pointer,
                setting
                ))
            {
                PhSettingFromString(
                    setting->Type,
                    &setting->DefaultValue,
                    NULL,
                    setting
                    );
            }

            PhpFreeIgnoredSetting(ignoredSetting);

            PhRemoveItemList(PhIgnoredSettings, i);
            i--;
        }
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

NTSTATUS PhLoadSettingsBin(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PVOID viewBase;
    SIZE_T viewSize;
    PPH_SETTINGS_BIN_HEADER header;
    PBYTE pointer;
    PBYTE limit;

    status = PhMapViewOfEntireFileEx(FileName, NULL, &viewBase, &viewSize);

    if (!NT_SUCCESS(status))
        return status;

    if (viewSize < sizeof(PH_SETTINGS_BIN_HEADER))
    {
        PhUnmapViewOfSection(NtCurrentProcess(), viewBase);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    header = (PPH_SETTINGS_BIN_HEADER)viewBase;

    if (header->Signature != PH_SETTINGS_BIN_SIGNATURE || header->Version != PH_SETTINGS_BIN_VERSION)
    {
        PhUnmapViewOfSection(NtCurrentProcess(), viewBase);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    PhpClearIgnoredSettings();

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    pointer = (PBYTE)viewBase + sizeof(PH_SETTINGS_BIN_HEADER);
    limit = (PBYTE)viewBase + viewSize;

    for (ULONG i = 0; i < header->NumberOfSettings; i++)
    {
        PPH_SETTINGS_BIN_SETTING entry;
        PPH_SETTING setting;
        PH_STRINGREF settingName;
        PH_STRINGREF settingValue;

        if (pointer + sizeof(PH_SETTINGS_BIN_SETTING) > limit)
            break;

        entry = (PPH_SETTINGS_BIN_SETTING)pointer;
        pointer += sizeof(PH_SETTINGS_BIN_SETTING);

        if (pointer + entry->NameLength + entry->ValueLength > limit)
            break;

        settingName.Buffer = (PWCH)pointer;
        settingName.Length = entry->NameLength;
        pointer += entry->NameLength;

        settingValue.Buffer = (PWCH)pointer;
        settingValue.Length = entry->ValueLength;
        pointer += entry->ValueLength;

        if (setting = PhpLookupSetting(&settingName))
        {
            PhpFreeSettingValue(setting->Type, setting);

            if (!PhSettingFromString(
                setting->Type,
                &settingValue,
                NULL,
                setting
                ))
            {
                PhSettingFromString(
                    setting->Type,
                    &setting->DefaultValue,
                    NULL,
                    setting
                    );
            }
        }
        else
        {
            setting = PhAllocateZero(sizeof(PH_SETTING));
            setting->Type = StringSettingType;
            setting->Name.Buffer = PhAllocateCopy(settingName.Buffer, settingName.Length + sizeof(WCHAR));
            setting->Name.Length = settingName.Length;
            setting->Name.Buffer[settingName.Length / sizeof(WCHAR)] = 0;
            setting->u.Pointer = PhCreateString2(&settingValue);

            PhAddItemList(PhIgnoredSettings, setting);
        }
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    PhUnmapViewOfSection(NtCurrentProcess(), viewBase);

    return STATUS_SUCCESS;
}

NTSTATUS PhSaveSettingsBin(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;
    PH_SETTINGS_BIN_HEADER header;
    SIZE_T totalSize = 0;
    PBYTE buffer;
    PBYTE pointer;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    // Calculate total size required
    totalSize = sizeof(PH_SETTINGS_BIN_HEADER);

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        PPH_STRING settingValue = PhSettingToString(setting->Type, setting);
        totalSize += sizeof(PH_SETTINGS_BIN_SETTING) + setting->Name.Length + settingValue->Length;
        PhDereferenceObject(settingValue);
    }

    for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
    {
        setting = PhIgnoredSettings->Items[i];
        PPH_STRING settingValue = setting->u.Pointer;
        totalSize += sizeof(PH_SETTINGS_BIN_SETTING) + setting->Name.Length + settingValue->Length;
    }

    buffer = PhAllocate(totalSize);
    pointer = buffer;

    __analysis_assume(pointer <= (PBYTE)buffer + totalSize);
    header.Signature = PH_SETTINGS_BIN_SIGNATURE;
    header.Version = PH_SETTINGS_BIN_VERSION;
    header.NumberOfSettings = PhSettingsHashtable->Count + PhIgnoredSettings->Count;

    __analysis_assume(pointer + sizeof(PH_SETTINGS_BIN_HEADER) <= (PBYTE)buffer + totalSize);
    memcpy(pointer, &header, sizeof(PH_SETTINGS_BIN_HEADER));
    pointer += sizeof(PH_SETTINGS_BIN_HEADER);

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        PPH_STRING settingValue = PhSettingToString(setting->Type, setting);
        PH_SETTINGS_BIN_SETTING entry;

        entry.NameLength = (ULONG)setting->Name.Length;
        entry.Type = (ULONG)setting->Type;
        entry.ValueLength = (ULONG)settingValue->Length;

        memcpy(pointer, &entry, sizeof(PH_SETTINGS_BIN_SETTING));
        pointer += sizeof(PH_SETTINGS_BIN_SETTING);
        memcpy(pointer, setting->Name.Buffer, setting->Name.Length);
        pointer += setting->Name.Length;
        memcpy(pointer, settingValue->Buffer, settingValue->Length);
        pointer += settingValue->Length;

        PhDereferenceObject(settingValue);
    }

    for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
    {
        setting = PhIgnoredSettings->Items[i];
        PPH_STRING settingValue = setting->u.Pointer;
        PH_SETTINGS_BIN_SETTING entry;

        entry.NameLength = (ULONG)setting->Name.Length;
        entry.Type = (ULONG)setting->Type;
        entry.ValueLength = (ULONG)settingValue->Length;

        memcpy(pointer, &entry, sizeof(PH_SETTINGS_BIN_SETTING));
        pointer += sizeof(PH_SETTINGS_BIN_SETTING);
        memcpy(pointer, setting->Name.Buffer, setting->Name.Length);
        pointer += setting->Name.Length;
        memcpy(pointer, settingValue->Buffer, settingValue->Length);
        pointer += settingValue->Length;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = NtWriteFile(fileHandle, NULL, NULL, NULL, &isb, buffer, (ULONG)totalSize, NULL, NULL);
        NtClose(fileHandle);
    }

    PhFree(buffer);

    return status;
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
static BOOLEAN NTAPI PhSettingsKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PH_STRINGREF settingName;
    PH_STRINGREF settingValue;
    PPH_SETTING setting;

    settingName.Buffer = Information->Name;
    settingName.Length = Information->NameLength;
    settingValue.Buffer = PTR_ADD_OFFSET(Information, Information->DataOffset);
    settingValue.Length = Information->DataLength - (Information->Type == REG_SZ ? sizeof(UNICODE_NULL) : 0);

    if (setting = PhpLookupSetting(&settingName))
    {
        PhpFreeSettingValue(setting->Type, setting);

        switch (setting->Type)
        {
        case StringSettingType:
            {
                if (PhSettingFromString(
                    setting->Type,
                    &settingValue,
                    NULL,
                    setting
                    ))
                {
                    return TRUE;
                }

                if (PhSettingFromString(
                    setting->Type,
                    &setting->DefaultValue,
                    NULL,
                    setting
                    ))
                {
                    return TRUE;
                }
            }
            break;
        case IntegerSettingType:
            {
                if (Information->Type == REG_DWORD)
                {
                    PLARGE_INTEGER value = PTR_ADD_OFFSET(Information, Information->DataOffset);

                    setting->u.Integer = value->LowPart;
                    return TRUE;
                }
                else if (Information->Type == REG_SZ)
                {
                    if (PhSettingFromString(setting->Type, &settingValue, NULL, setting))
                        return TRUE;
                }
            }
            break;
        case IntegerPairSettingType:
            {
                if (Information->Type == REG_QWORD)
                {
                    PLARGE_INTEGER value = PTR_ADD_OFFSET(Information, Information->DataOffset);

                    setting->u.IntegerPair.X = (LONG)value->LowPart;
                    setting->u.IntegerPair.Y = (LONG)value->HighPart;
                    return TRUE;
                }
                else if (Information->Type == REG_SZ)
                {
                    if (PhSettingFromString(setting->Type, &settingValue, NULL, setting))
                        return TRUE;
                }
            }
            break;
        case ScalableIntegerPairSettingType:
            {
                if (Information->Type == REG_BINARY)
                {
                    PPH_SCALABLE_INTEGER_PAIR value = PTR_ADD_OFFSET(Information, Information->DataOffset);

                    setting->u.Pointer = PhAllocateCopy(value, sizeof(PH_SCALABLE_INTEGER_PAIR));
                    return TRUE;
                }
                else if (Information->Type == REG_SZ)
                {
                    if (PhSettingFromString(setting->Type, &settingValue, NULL, setting))
                        return TRUE;
                }
            }
            break;
        }

        assert(FALSE);
    }
    else
    {
        setting = PhAllocateZero(sizeof(PH_SETTING));
        setting->Type = StringSettingType;
        setting->Name.Buffer = PhAllocateCopy(settingName.Buffer, settingName.Length + sizeof(WCHAR));
        setting->Name.Length = settingName.Length;
        setting->u.Pointer = PhCreateString2(&settingValue);

        PhAddItemList(PhIgnoredSettings, setting);
    }

    return TRUE;
}

static VOID PhpSaveSettingsToKey(
    _In_ HANDLE KeyHandle
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        switch (setting->Type)
        {
        case StringSettingType:
            {
                PPH_STRING value = (PPH_STRING)setting->u.Pointer;

                if (PhIsNullOrEmptyString(value))
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
                else
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_SZ,
                        value->Buffer,
                        (ULONG)value->Length + sizeof(UNICODE_NULL)
                        );
                }
            }
            break;
        case IntegerSettingType:
            {
                if (setting->u.Integer)
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_DWORD,
                        &setting->u.Integer,
                        sizeof(setting->u.Integer)
                        );
                }
                else
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
            }
            break;
        case IntegerPairSettingType:
            {
                PPH_INTEGER_PAIR integerPair = &setting->u.IntegerPair;
                LARGE_INTEGER value;

                value.LowPart = integerPair->X;
                value.HighPart = integerPair->Y;

                if (value.QuadPart)
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_QWORD,
                        &value.QuadPart,
                        sizeof(value.QuadPart)
                        );
                }
                else
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
            }
            break;
        case ScalableIntegerPairSettingType:
            {
                PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = setting->u.Pointer;

                if (scalableIntegerPair && scalableIntegerPair->X && scalableIntegerPair->Y && scalableIntegerPair->Scale)
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_BINARY,
                        scalableIntegerPair,
                        sizeof(PH_SCALABLE_INTEGER_PAIR)
                        );
                }
                else
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
            }
            break;
        default:
            {
                PPH_STRING settingValue = PhSettingToString(setting->Type, setting);

                PhSetValueKey(
                    KeyHandle,
                    &setting->Name,
                    REG_SZ,
                    settingValue->Buffer,
                    (ULONG)settingValue->Length + sizeof(UNICODE_NULL)
                    );

                PhDereferenceObject(settingValue);
            }
            break;
        }
    }

    for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
    {
        setting = PhIgnoredSettings->Items[i];

        switch (setting->Type)
        {
        case StringSettingType:
            {
                PPH_STRING value = (PPH_STRING)setting->u.Pointer;

                if (PhIsNullOrEmptyString(value))
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
                else
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_SZ,
                        value->Buffer,
                        (ULONG)value->Length + sizeof(UNICODE_NULL)
                        );
                }
            }
            break;
        case IntegerSettingType:
            {
                if (setting->u.Integer)
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_DWORD,
                        &setting->u.Integer,
                        sizeof(setting->u.Integer)
                        );
                }
                else
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
            }
            break;
        case IntegerPairSettingType:
            {
                PPH_INTEGER_PAIR integerPair = &setting->u.IntegerPair;
                LARGE_INTEGER value;

                value.LowPart = integerPair->X;
                value.HighPart = integerPair->Y;

                if (value.QuadPart)
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_QWORD,
                        &value.QuadPart,
                        sizeof(value.QuadPart)
                        );
                }
                else
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
            }
            break;
        case ScalableIntegerPairSettingType:
            {
                PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = setting->u.Pointer;

                if (scalableIntegerPair && scalableIntegerPair->X && scalableIntegerPair->Y && scalableIntegerPair->Scale)
                {
                    PhSetValueKey(
                        KeyHandle,
                        &setting->Name,
                        REG_BINARY,
                        scalableIntegerPair,
                        sizeof(PH_SCALABLE_INTEGER_PAIR)
                        );
                }
                else
                {
                    PhDeleteValueKey(KeyHandle, &setting->Name);
                }
            }
            break;
        default:
            {
                PPH_STRING settingValue = PhSettingToString(setting->Type, setting);

                PhSetValueKey(
                    KeyHandle,
                    &setting->Name,
                    REG_SZ,
                    settingValue->Buffer,
                    (ULONG)settingValue->Length + sizeof(UNICODE_NULL)
                    );

                PhDereferenceObject(settingValue);
            }
            break;
        }
    }
}

NTSTATUS PhLoadSettingsAppKey(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhLoadAppKey(
        &keyHandle,
        FileName,
        KEY_ALL_ACCESS,
        REG_APP_HIVE | REG_PROCESS_PRIVATE | REG_HIVE_NO_RM
        );

    if (!NT_SUCCESS(status))
        return status;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    status = PhEnumerateValueKey(
        keyHandle,
        KeyValueFullInformation,
        PhSettingsKeyCallback,
        NULL
        );

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    NtClose(keyHandle);

    return status;
}

NTSTATUS PhSaveSettingsAppKey(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhLoadAppKey(
        &keyHandle,
        FileName,
        KEY_ALL_ACCESS,
        REG_APP_HIVE | REG_PROCESS_PRIVATE | REG_HIVE_NO_RM
        );

    if (!NT_SUCCESS(status))
        return status;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    PhpSaveSettingsToKey(keyHandle);

    PhReleaseQueuedLockShared(&PhSettingsLock);

    NtClose(keyHandle);

    return status;
}

NTSTATUS PhLoadSettingsKey(
    VOID
    )
{
    static CONST PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\SystemInformer");
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyName,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    status = PhEnumerateValueKey(
        keyHandle,
        KeyValueFullInformation,
        PhSettingsKeyCallback,
        NULL
        );

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    NtClose(keyHandle);

    return status;
}

NTSTATUS PhSaveSettingsKey(
    VOID
    )
{
    static CONST PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\SystemInformer");
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhCreateKey(
        &keyHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &keyName,
        0,
        0,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    PhpSaveSettingsToKey(keyHandle);

    PhReleaseQueuedLockShared(&PhSettingsLock);

    NtClose(keyHandle);

    return status;
}

static BOOLEAN PhLoadSettingsEnumJsonCallback(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PVOID Value,
    _In_opt_ PVOID Context
    )
{
    PPH_SETTING setting;
    PPH_STRING settingName;

    settingName = PhZeroExtendToUtf16(Key);

    if (setting = PhpLookupSetting(&settingName->sr))
    {
        PhpFreeSettingValue(setting->Type, setting);

        switch (setting->Type)
        {
        case IntegerSettingType:
            {
                if (PhGetJsonObjectType(Value) == PH_JSON_OBJECT_TYPE_INT)
                {
                    setting->u.Integer = PhGetJsonUInt32Object(Value);
                    break;
                }
            }
            goto DefaultCase;
        case IntegerPairSettingType:
            {
                if (PhGetJsonObjectType(Value) == PH_JSON_OBJECT_TYPE_INT)
                {
                    ULARGE_INTEGER value;
                    value.QuadPart = PhGetJsonUInt64Object(Value);
                    setting->u.IntegerPair.X = (LONG)value.LowPart;
                    setting->u.IntegerPair.Y = (LONG)value.HighPart;
                    break;
                }
            }
            goto DefaultCase;
        case StringSettingType:
        case ScalableIntegerPairSettingType:
DefaultCase:
            {
                PPH_STRING settingValue;

                settingValue = PhGetJsonObjectString(Value);

                if (settingValue)
                {
                    PhSetReference(&setting->u.Pointer, settingValue);
                }
                else
                {
                    setting->u.Pointer = PhCreateString(L"");
                }

                if (!PhSettingFromString(
                    setting->Type,
                    &settingValue->sr,
                    settingValue,
                    setting
                    ))
                {
                    PhSettingFromString(
                        setting->Type,
                        &setting->DefaultValue,
                        NULL,
                        setting
                        );
                }
            }
            break;
        }
    }
    else
    {
        PPH_STRING settingValue;

        settingValue = PhGetJsonObjectString(Value);

        setting = PhAllocateZero(sizeof(PH_SETTING));
        setting->Type = StringSettingType;
        setting->Name.Buffer = PhAllocateCopy(settingName->Buffer, settingName->Length + sizeof(WCHAR));
        setting->Name.Length = settingName->Length;
        PhReferenceObject(settingValue);
        setting->u.Pointer = settingValue;

        PhAddItemList(PhIgnoredSettings, setting);
    }

    PhDereferenceObject(settingName);
    return TRUE;
}

NTSTATUS PhLoadSettingsJson(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PVOID object;

    status = PhLoadJsonObjectFromFile(&object, FileName);

    if (NT_SUCCESS(status))
    {
        if (PhGetJsonObjectType(object) == PH_JSON_OBJECT_TYPE_OBJECT)
        {
            PhAcquireQueuedLockExclusive(&PhSettingsLock);
            PhEnumJsonArrayObject(object, PhLoadSettingsEnumJsonCallback, NULL);
            PhReleaseQueuedLockExclusive(&PhSettingsLock);
        }

        PhFreeJsonObject(object);
    }

    return status;
}

NTSTATUS PhSaveSettingsJson(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PVOID object;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;
    PPH_LIST strings = NULL;

    object = PhCreateJsonObject();

    if (!object)
        return STATUS_FILE_CORRUPT_ERROR;

    strings = PhCreateList(1);

    PhAcquireQueuedLockShared(&PhSettingsLock);

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        switch (setting->Type)
        {
        //case IntegerSettingType:
        //    {
        //        PPH_BYTES stringName = PhConvertStringRefToUtf8(&setting->Name);
        //        PhAddJsonObjectInt64(object, stringName->Buffer, setting->u.Integer);
        //        PhAddItemList(strings, stringName);
        //    }
        //    break;
        //case IntegerPairSettingType:
        //    {
        //        PPH_BYTES stringName = PhConvertStringRefToUtf8(&setting->Name);
        //        ULARGE_INTEGER value;
        //
        //        value.LowPart = setting->u.IntegerPair.X;
        //        value.HighPart = setting->u.IntegerPair.Y;
        //        PhAddJsonObjectUInt64(object, stringName->Buffer, value.QuadPart);
        //        PhAddItemList(strings, stringName);
        //    }
        //    break;
        case IntegerSettingType:
        case IntegerPairSettingType:
        case StringSettingType:
        case ScalableIntegerPairSettingType:
            {
                PPH_STRING stringSetting;
                PPH_BYTES stringName;
                PPH_BYTES stringValue;

                stringName = PhConvertStringRefToUtf8(&setting->Name);
                stringSetting = PhSettingToString(setting->Type, setting);
                stringValue = PhConvertStringToUtf8(stringSetting);

                PhAddJsonObject2(object, stringName->Buffer, stringValue->Buffer, stringValue->Length);

                PhAddItemList(strings, stringValue);
                PhAddItemList(strings, stringSetting);
                PhAddItemList(strings, stringName);
            }
            break;
        }
    }

    for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
    {
        PPH_STRING stringSetting;
        PPH_BYTES stringName;
        PPH_BYTES stringValue;

        setting = PhIgnoredSettings->Items[i];

        stringSetting = setting->u.Pointer;
        stringName = PhConvertStringRefToUtf8(&setting->Name);
        stringValue = PhConvertStringToUtf8(stringSetting);

        PhAddJsonObject2(object, stringName->Buffer, stringValue->Buffer, stringValue->Length);

        PhAddItemList(strings, stringValue);
        PhAddItemList(strings, stringName);
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    status = PhSaveJsonObjectToFile(
        FileName,
        object,
        PH_JSON_TO_STRING_PRETTY
        );

    PhFreeJsonObject(object);

    PhDereferenceObjects(strings->Items, strings->Count);
    PhDereferenceObject(strings);

    return status;
}

NTSTATUS PhLoadSettingsXml(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PVOID topNode;
    PVOID currentNode;
    PPH_SETTING setting;
    PPH_STRING settingName;
    PPH_STRING settingValue;

    PhpClearIgnoredSettings();

    if (!NT_SUCCESS(status = PhLoadXmlObjectFromFile(FileName, &topNode)))
        return status;
    if (!topNode) // Return corrupt status and reset the settings.
        return STATUS_FILE_CORRUPT_ERROR;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    currentNode = PhGetXmlNodeFirstChild(topNode);

    while (currentNode)
    {
        if (settingName = PhGetXmlNodeAttributeText(currentNode, "name"))
        {
            settingValue = PhGetXmlNodeOpaqueText(currentNode);

            {
                if (setting = PhpLookupSetting(&settingName->sr))
                {
                    PhpFreeSettingValue(setting->Type, setting);

                    if (!PhSettingFromString(
                        setting->Type,
                        &settingValue->sr,
                        settingValue,
                        setting
                        ))
                    {
                        PhSettingFromString(
                            setting->Type,
                            &setting->DefaultValue,
                            NULL,
                            setting
                            );
                    }
                }
                else
                {
                    setting = PhAllocateZero(sizeof(PH_SETTING));
                    setting->Type = StringSettingType;
                    setting->Name.Buffer = PhAllocateCopy(settingName->Buffer, settingName->Length + sizeof(WCHAR));
                    setting->Name.Length = settingName->Length;
                    PhReferenceObject(settingValue);
                    setting->u.Pointer = settingValue;

                    PhAddItemList(PhIgnoredSettings, setting);
                }
            }

            PhDereferenceObject(settingValue);
            PhDereferenceObject(settingName);
        }

        currentNode = PhGetXmlNodeNextChild(currentNode);
    }

    PhFreeXmlObject(topNode);

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    return STATUS_SUCCESS;
}

//static BOOLEAN PhXmlLiteInitialized(
//    VOID
//    )
//{
//    static PH_INITONCE initOnce = PH_INITONCE_INIT;
//    static BOOLEAN XmlLiteInitialized = FALSE;
//
//    if (PhBeginInitOnce(&initOnce))
//    {
//        if (CreateXmlReader_Import() && CreateXmlWriter_Import() && SHCreateStreamOnFileEx_Import())
//        {
//            XmlLiteInitialized = TRUE;
//        }
//
//        PhEndInitOnce(&initOnce);
//    }
//
//    return XmlLiteInitialized;
//}
//
//HRESULT PhLoadSettingsXmlRead(
//    _In_ PCWSTR FileName
//    )
//{
//    HRESULT status;
//    IXmlReader* xmlReader = NULL;
//    IStream* fileStream = NULL;
//    PPH_SETTING setting;
//    SIZE_T settingBufferLength;
//    WCHAR settingBuffer[0x1000];
//    PH_STRINGREF settingName;
//    PH_STRINGREF settingValue;
//    XmlNodeType nodeType;
//    PCWSTR nodeName;
//    PCWSTR attrName;
//
//    status = SHCreateStreamOnFileEx_Import()(
//        FileName,
//        STGM_READ | STGM_SIMPLE,
//        FILE_ATTRIBUTE_NORMAL,
//        FALSE,
//        NULL,
//        &fileStream
//        );
//
//    if (HR_FAILED(status))
//        goto CleanupExit;
//
//    status = CreateXmlReader_Import()(&IID_IXmlReader, &xmlReader, NULL);
//
//    if (HR_FAILED(status))
//        goto CleanupExit;
//
//    IXmlReader_SetProperty(xmlReader, XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);
//
//    status = IXmlReader_SetInput(xmlReader, (IUnknown*)fileStream);
//
//    if (HR_FAILED(status))
//        goto CleanupExit;
//
//    while (HR_SUCCESS(IXmlReader_Read(xmlReader, &nodeType)))
//    {
//        if (nodeType == XmlNodeType_Element)
//        {
//            if (HR_SUCCESS(IXmlReader_GetLocalName(xmlReader, &nodeName, NULL)))
//            {
//                if (PhEqualStringZ(nodeName, L"setting", TRUE))
//                {
//                    if (HR_SUCCESS(IXmlReader_MoveToFirstAttribute(xmlReader)))
//                    {
//                        if (HR_SUCCESS(IXmlReader_GetLocalName(xmlReader, &attrName, NULL)))
//                        {
//                            if (PhEqualStringZ(attrName, L"name", TRUE))
//                            {
//                                ULONG nameStringLength = 0;
//                                PCWSTR nameStringBuffer;
//
//                                if (HR_SUCCESS(IXmlReader_GetValue(xmlReader, &nameStringBuffer, &nameStringLength)))
//                                {
//                                    settingBufferLength = nameStringLength * sizeof(WCHAR);
//
//                                    if (settingBufferLength % 2 != 0)
//                                        continue;
//                                    if (settingBufferLength > sizeof(settingBuffer))
//                                        continue;
//
//                                    // Note: IXmlReader_GetValue returns a pointer to the string offset in the IStream buffer.
//                                    // Copy the string since since the offset might be invalid after the next buffer read. (dmex)
//                                    memcpy(settingBuffer, nameStringBuffer, settingBufferLength);
//                                    settingName.Buffer = settingBuffer;
//                                    settingName.Length = settingBufferLength;
//
//                                    if (HR_SUCCESS(IXmlReader_Read(xmlReader, &nodeType)) && nodeType == XmlNodeType_Text)
//                                    {
//                                        ULONG valueStringLength = 0;
//                                        PCWSTR valueStringBuffer;
//
//                                        if (HR_SUCCESS(IXmlReader_GetValue(xmlReader, &valueStringBuffer, &valueStringLength)))
//                                        {
//                                            settingBufferLength = valueStringLength * sizeof(WCHAR);
//
//                                            if (settingBufferLength % 2 != 0)
//                                                continue;
//
//                                            settingValue.Buffer = (PWCH)valueStringBuffer;
//                                            settingValue.Length = settingBufferLength;
//
//                                            {
//                                                setting = PhpLookupSetting(&settingName);
//
//                                                if (setting)
//                                                {
//                                                    PhpFreeSettingValue(setting->Type, setting);
//
//                                                    if (!PhSettingFromString(
//                                                        setting->Type,
//                                                        &settingValue,
//                                                        NULL,
//                                                        setting
//                                                        ))
//                                                    {
//                                                        PhSettingFromString(
//                                                            setting->Type,
//                                                            &setting->DefaultValue,
//                                                            NULL,
//                                                            setting
//                                                            );
//                                                    }
//                                                }
//                                                else
//                                                {
//                                                    setting = PhAllocate(sizeof(PH_SETTING));
//                                                    setting->Name.Buffer = PhAllocateCopy(settingName.Buffer, settingName.Length + sizeof(UNICODE_NULL));
//                                                    setting->Name.Length = settingName.Length;
//                                                    setting->u.Pointer = PhCreateString2(&settingValue);
//
//                                                    PhAddItemList(PhIgnoredSettings, setting);
//                                                }
//                                            }
//                                        }
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//CleanupExit:
//    if (xmlReader) IXmlReader_Release(xmlReader);
//    if (fileStream) IStream_Release(fileStream);
//
//    return status;
//}
//
//NTSTATUS PhLoadSettingsXmlLite(
//    _In_ PCPH_STRINGREF FileName
//    )
//{
//    HRESULT status;
//    PPH_STRING fileNameWin32;
//
//    fileNameWin32 = PhResolveDevicePrefix(FileName);
//
//    if (PhIsNullOrEmptyString(fileNameWin32))
//        return STATUS_UNSUCCESSFUL;
//
//    PhMoveReference(&fileNameWin32, PhConcatStringRef2(&PhWin32ExtendedPathPrefix, &fileNameWin32->sr));
//
//    PhpClearIgnoredSettings();
//
//    PhAcquireQueuedLockExclusive(&PhSettingsLock);
//    status = PhLoadSettingsXmlRead(PhGetString(fileNameWin32));
//    PhReleaseQueuedLockExclusive(&PhSettingsLock);
//
//    PhDereferenceObject(fileNameWin32);
//
//    if (HR_FAILED(status))
//        return STATUS_UNSUCCESSFUL; // HRESULT_CODE(status);
//    return STATUS_SUCCESS;
//}
//
//DEFINE_GUID(IID_IXmlWriterLite, 0x862494C6, 0x1310, 0x4AAD, 0xB3, 0xCD, 0x2D, 0xBE, 0xEB, 0xF6, 0x70, 0xD3);
//
//HRESULT PhSaveSettingsXmlWrite(
//    _In_ PCWSTR FileName
//    )
//{
//    HRESULT status;
//    PH_AUTO_POOL autoPool;
//    IStream* fileStream = NULL;
//    PH_HASHTABLE_ENUM_CONTEXT enumContext;
//    PPH_SETTING setting;
//
//    status = SHCreateStreamOnFileEx_Import()(
//        FileName,
//        STGM_WRITE | STGM_CREATE,
//        FILE_ATTRIBUTE_NORMAL,
//        TRUE,
//        NULL,
//        &fileStream
//        );
//
//    if (HR_FAILED(status))
//        return status;
//
//    PhInitializeAutoPool(&autoPool);
//
//    {
//        IXmlWriter* xmlWriter = NULL;
//
//        status = CreateXmlWriter_Import()(&IID_IXmlWriter, &xmlWriter, NULL);
//
//        if (HR_FAILED(status))
//            goto CleanupExit;
//
//        IXmlWriter_SetProperty(xmlWriter, XmlWriterProperty_Indent, TRUE);
//        IXmlWriter_SetProperty(xmlWriter, XmlWriterProperty_OmitXmlDeclaration, TRUE);
//
//        status = IXmlWriter_SetOutput(xmlWriter, (IUnknown*)fileStream);
//
//        if (HR_FAILED(status))
//            goto CleanupExit;
//
//        IXmlWriter_WriteStartElement(xmlWriter, NULL, L"settings", NULL);
//
//        PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);
//
//        while (setting = PhNextEnumHashtable(&enumContext))
//        {
//            PPH_STRING settingValue;
//
//            settingValue = PH_AUTO_T(PH_STRING, PhSettingToString(setting->Type, setting));
//            IXmlWriter_WriteStartElement(xmlWriter, NULL, L"setting", NULL);
//            IXmlWriter_WriteAttributeString(xmlWriter, NULL, L"name", NULL, PhGetStringRefZ(&setting->Name));
//            IXmlWriter_WriteString(xmlWriter, PhGetStringRefZ(&settingValue->sr));
//            IXmlWriter_WriteEndElement(xmlWriter);
//        }
//
//        for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
//        {
//            PPH_STRING settingValue;
//
//            setting = PhIgnoredSettings->Items[i];
//            settingValue = setting->u.Pointer;
//
//            IXmlWriter_WriteStartElement(xmlWriter, NULL, L"setting", NULL);
//            IXmlWriter_WriteAttributeString(xmlWriter, NULL, L"name", NULL, PhGetStringRefZ(&setting->Name));
//            IXmlWriter_WriteString(xmlWriter, PhGetStringRefZ(&settingValue->sr));
//            IXmlWriter_WriteEndElement(xmlWriter);
//        }
//
//        IXmlWriter_WriteEndElement(xmlWriter);
//
//        status = IXmlWriter_Flush(xmlWriter);
//
//        IXmlWriter_Release(xmlWriter);
//    }
//
//    //{
//    //    IXmlWriterLite* xmlWriterLite = NULL;
//    //
//    //    status = CreateXmlWriter_Import()(&IID_IXmlWriterLite, &xmlWriterLite, NULL);
//    //
//    //    if (HR_FAILED(status))
//    //        goto CleanupExit;
//    //
//    //    status = IXmlWriterLite_SetOutput(xmlWriterLite, (IUnknown*)fileStream);
//    //
//    //    if (HR_FAILED(status))
//    //        goto CleanupExit;
//
//    //    IXmlWriterLite_WriteStartElement(xmlWriterLite, L"settings", 8);
//    //    IXmlWriterLite_WriteWhitespace(xmlWriterLite, L"\n");
//    //
//    //    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);
//    //
//    //    while (setting = PhNextEnumHashtable(&enumContext))
//    //    {
//    //        PPH_STRING settingValue;
//    //
//    //        settingValue = PhSettingToString(setting->Type, setting);
//    //        IXmlWriterLite_WriteStartElement(xmlWriterLite, L"setting", 7);
//    //        IXmlWriterLite_WriteAttributeString(xmlWriterLite, L"name", 4, setting->Name.Buffer, (ULONG)setting->Name.Length / sizeof(WCHAR));
//    //        IXmlWriterLite_WriteString(xmlWriterLite, PhGetStringRefZ(&settingValue->sr));
//    //        //IXmlWriterLite_WriteChars(xmlWriterLite, settingValue->Buffer, (ULONG)settingValue->Length / sizeof(WCHAR));
//    //        IXmlWriterLite_WriteEndElement(xmlWriterLite, L"setting", 7);
//    //        IXmlWriterLite_WriteWhitespace(xmlWriterLite, L"\n");
//    //        PhDereferenceObject(settingValue);
//    //    }
//    //
//    //    // Write the ignored settings.
//    //
//    //    for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
//    //    {
//    //        PPH_STRING settingValue;
//    //
//    //        setting = PhIgnoredSettings->Items[i];
//    //        settingValue = setting->u.Pointer;
//    //        IXmlWriterLite_WriteStartElement(xmlWriterLite, L"setting", 7);
//    //        IXmlWriterLite_WriteAttributeString(xmlWriterLite, L"name", 4, setting->Name.Buffer, (ULONG)setting->Name.Length / sizeof(WCHAR));
//    //        IXmlWriterLite_WriteString(xmlWriterLite, PhGetStringRefZ(&settingValue->sr));
//    //        IXmlWriterLite_WriteEndElement(xmlWriterLite, L"setting", 7);
//    //        IXmlWriterLite_WriteWhitespace(xmlWriterLite, L"\n");
//    //    }
//    //
//    //    IXmlWriterLite_WriteEndElement(xmlWriterLite, L"settings", 8);
//
//    //    status = IXmlWriterLite_Flush(xmlWriterLite);
//    //
//    //    IXmlWriterLite_Release(xmlWriterLite);
//    //}
//
//CleanupExit:
//    //IStream_Commit(fileStream, STGC_DEFAULT);
//    IStream_Release(fileStream);
//
//    PhDeleteAutoPool(&autoPool);
//
//    return status;
//}
//
//NTSTATUS PhSaveSettingsXmlLite(
//    _In_ PCPH_STRINGREF FileName
//    )
//{
//    static CONST PH_STRINGREF extension = PH_STRINGREF_INIT(L".tmp");
//    HRESULT status;
//    PPH_STRING fileNameWin32;
//    PPH_STRING fileNameTempWin32;
//
//    fileNameWin32 = PhResolveDevicePrefix(FileName);
//
//    if (PhIsNullOrEmptyString(fileNameWin32))
//        return STATUS_UNSUCCESSFUL;
//
//    // TODO: Write XmlLite to buffer and atomic rename (same as PhSaveXmlObjectToFile) (dmex)
//    PhMoveReference(&fileNameWin32, PhConcatStringRef2(&PhWin32ExtendedPathPrefix, &fileNameWin32->sr));
//    fileNameTempWin32 = PhConcatStringRef2(&fileNameWin32->sr, &extension);
//
//    PhpClearIgnoredSettings();
//
//    PhAcquireQueuedLockShared(&PhSettingsLock);
//    status = PhSaveSettingsXmlWrite(PhGetString(fileNameTempWin32));
//    PhReleaseQueuedLockShared(&PhSettingsLock);
//
//    if (HR_FAILED(status))
//    {
//        PhDereferenceObject(fileNameTempWin32);
//        PhDereferenceObject(fileNameWin32);
//        return STATUS_UNSUCCESSFUL; // HRESULT_CODE(status);
//    }
//
//    status = PhMoveFileWin32(
//        PhGetString(fileNameTempWin32),
//        PhGetString(fileNameWin32),
//        FALSE
//        );
//
//    PhDereferenceObject(fileNameTempWin32);
//    PhDereferenceObject(fileNameWin32);
//    return status;
//}

PCSTR PhpSettingsSaveCallback(
    _In_ PVOID cbdata,
    _In_ PVOID node,
    _In_ LONG when
    )
{
    PCSTR elementName;

    if (!(elementName = PhGetXmlNodeElementText(node)))
        return NULL;

    if (PhEqualBytesZ(elementName, "setting", TRUE))
    {
        if (when == MXML_WS_BEFORE_OPEN)
            return "  ";
        else if (when == MXML_WS_AFTER_CLOSE)
            return "\r\n";
    }
    else if (PhEqualBytesZ(elementName, "settings", TRUE))
    {
        if (when == MXML_WS_AFTER_OPEN)
            return "\r\n";
    }

    return NULL;
}

PVOID PhpCreateSettingElement(
    _Inout_ PVOID ParentNode,
    _In_ PCPH_STRINGREF SettingName,
    _In_ PCPH_STRINGREF SettingValue
    )
{
    PVOID settingNode;
    PPH_BYTES settingNameUtf8;
    PPH_BYTES settingValueUtf8;

    // Create the setting element.

    settingNode = PhCreateXmlNode(ParentNode, "setting");

    settingNameUtf8 = PhConvertUtf16ToUtf8Ex(SettingName->Buffer, SettingName->Length);
    PhSetXmlNodeAttributeText(settingNode, "name", settingNameUtf8->Buffer);
    PhDereferenceObject(settingNameUtf8);

    // Set the value.

    settingValueUtf8 = PhConvertUtf16ToUtf8Ex(SettingValue->Buffer, SettingValue->Length);
    PhCreateXmlOpaqueNode(settingNode, settingValueUtf8->Buffer);
    PhDereferenceObject(settingValueUtf8);

    return settingNode;
}

NTSTATUS PhSaveSettingsXml(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PVOID topNode;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;

    topNode = PhCreateXmlNode(NULL, "settings");

    PhAcquireQueuedLockShared(&PhSettingsLock);

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        PPH_STRING settingValue;

        settingValue = PhSettingToString(setting->Type, setting);
        PhpCreateSettingElement(topNode, &setting->Name, &settingValue->sr);
        PhDereferenceObject(settingValue);
    }

    // Write the ignored settings.

    for (ULONG i = 0; i < PhIgnoredSettings->Count; i++)
    {
        PPH_STRING settingValue;

        setting = PhIgnoredSettings->Items[i];
        settingValue = setting->u.Pointer;
        PhpCreateSettingElement(topNode, &setting->Name, &settingValue->sr);
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    status = PhSaveXmlObjectToFile(
        FileName,
        topNode,
        PhpSettingsSaveCallback
        );
    PhFreeXmlObject(topNode);

    if (status == STATUS_SHARING_VIOLATION) // Skip multiple instances (dmex)
        status = STATUS_SUCCESS;

    return status;
}

typedef struct _PH_SETTINGS_DISCOVERY_RESULT
{
    PH_SETTINGS_FORMAT Format;
    PPH_STRING FilePath;
    LARGE_INTEGER LastWriteTime;
    BOOLEAN Found;
} PH_SETTINGS_DISCOVERY_RESULT, *PPH_SETTINGS_DISCOVERY_RESULT;

static ULONG PhpDiscoverSettingsStores(
    _In_opt_ PPH_STRING BasePath,
    _In_opt_ PCWSTR DefaultName,
    _Out_writes_(PH_SETTINGS_STORE_COUNT) PPH_SETTINGS_DISCOVERY_RESULT Results,
    _In_ ULONG ResultCount,
    _Out_opt_ PBOOLEAN IsPortable
    )
{
    ULONG foundCount = 0;

    RtlZeroMemory(Results, sizeof(PH_SETTINGS_DISCOVERY_RESULT) * ResultCount);

    if (IsPortable) *IsPortable = FALSE;

    if (!BasePath)
    {
        PPH_STRING searchPath;

        // 1. Portable
        if (searchPath = PhGetApplicationFileNameZ(L".settings"))
        {
            foundCount = PhpDiscoverSettingsStores(searchPath, DefaultName, Results, ResultCount, NULL);
            PhDereferenceObject(searchPath);
            if (foundCount > 0)
            {
                if (IsPortable) *IsPortable = TRUE;
                return foundCount;
            }
        }

        // 2. AppData
        if (DefaultName && (searchPath = PhGetRoamingAppDataDirectoryZ(DefaultName, TRUE)))
        {
            foundCount = PhpDiscoverSettingsStores(searchPath, DefaultName, Results, ResultCount, NULL);
            PhDereferenceObject(searchPath);
            if (foundCount > 0) return foundCount;
        }
    }

    for (ULONG i = 0; i < PH_SETTINGS_STORE_COUNT; i++)
// ... (omitting lines for brevity, but I will include them in the real tool call)
    {
        const PH_SETTINGS_STORE_DESCRIPTOR* store = &PhSettingsStores[i];

        Results[i].Format = store->Format;
        Results[i].Found = FALSE;

        if (store->IsFileBased)
        {
            if (!BasePath)
                continue;

            PPH_STRING filePath = PhConcatStringRefZ(&BasePath->sr, store->Extension);

            if (PhDoesFileExist(&filePath->sr))
            {
                FILE_NETWORK_OPEN_INFORMATION networkOpenInfo;

                if (NT_SUCCESS(PhQueryFullAttributesFile(&filePath->sr, &networkOpenInfo)))
                {
                    Results[i].Found = TRUE;
                    Results[i].FilePath = filePath;
                    Results[i].LastWriteTime = networkOpenInfo.LastWriteTime;
                    foundCount++;
                }
                else
                {
                    PhDereferenceObject(filePath);
                }
            }
            else
            {
                PhDereferenceObject(filePath);
            }
        }
        else if (store->Format == SettingsFormatReg)
        {
            if (BasePath)
                continue;

            static CONST PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\SystemInformer");
            HANDLE keyHandle;

            if (NT_SUCCESS(PhOpenKey(
                &keyHandle,
                KEY_READ,
                PH_KEY_CURRENT_USER,
                &keyName,
                0
                )))
            {
                //LARGE_INTEGER lastwriteTime = { 0 };
                //PhQueryKeyLastWriteTime(keyHandle, &lastwriteTime);

                Results[i].Found = TRUE;
                Results[i].FilePath = NULL;
                //Results[i].LastWriteTime = lastwriteTime;
                foundCount++;
                NtClose(keyHandle);
            }
        }
    }

    return foundCount;
}

static LONG PhpSelectBestSettingsStore(
    _In_reads_(PH_SETTINGS_STORE_COUNT) PPH_SETTINGS_DISCOVERY_RESULT Results
    )
{
    LONG preferredIndex = -1;
    LONG newestIndex = -1;
    LONG highestPriorityIndex = -1;
    LARGE_INTEGER newestTime = { 0 };
    LONG highestPriority = INT_MAX;

    for (ULONG i = 0; i < PH_SETTINGS_STORE_COUNT; i++)
    {
        if (!Results[i].Found)
            continue;

        if (PhSettingsStores[i].IsPreferred)
        {
            preferredIndex = i;
        }

        if (PhSettingsStores[i].IsFileBased)
        {
            if (Results[i].LastWriteTime.QuadPart > newestTime.QuadPart)
            {
                newestTime = Results[i].LastWriteTime;
                newestIndex = i;
            }
        }

        if (PhSettingsStores[i].Priority < highestPriority)
        {
            highestPriority = PhSettingsStores[i].Priority;
            highestPriorityIndex = i;
        }
    }

    if (preferredIndex >= 0)
        return preferredIndex;

    if (newestIndex >= 0)
        return newestIndex;

    return highestPriorityIndex;
}

static VOID PhpFreeDiscoveryResults(
    _In_reads_(PH_SETTINGS_STORE_COUNT) PPH_SETTINGS_DISCOVERY_RESULT Results
    )
{
    for (ULONG i = 0; i < PH_SETTINGS_STORE_COUNT; i++)
    {
        if (Results[i].FilePath)
        {
            PhDereferenceObject(Results[i].FilePath);
            Results[i].FilePath = NULL;
        }
    }
}

NTSTATUS PhLoadSettingsAutoDetect(
    _In_opt_ PPH_STRING BasePath,
    _In_opt_ PCWSTR DefaultName,
    _Out_opt_ PPH_STRING* ActualPath,
    _Out_opt_ PH_SETTINGS_FORMAT* ActualFormat,
    _Out_opt_ PBOOLEAN IsPortable
    )
{
    PH_SETTINGS_DISCOVERY_RESULT results[PH_SETTINGS_STORE_COUNT];
    NTSTATUS status = STATUS_NOT_FOUND;
    LONG selectedIndex;
    ULONG foundCount;

    foundCount = PhpDiscoverSettingsStores(BasePath, DefaultName, results, PH_SETTINGS_STORE_COUNT, IsPortable);

    if (foundCount == 0)
    {
        if (ActualPath && BasePath)
        {
            for (ULONG i = 0; i < PH_SETTINGS_STORE_COUNT; i++)
            {
                if (PhSettingsStores[i].IsPreferred && PhSettingsStores[i].IsFileBased)
                {
                    *ActualPath = PhConcatStringRefZ(&BasePath->sr, PhSettingsStores[i].Extension);
                    if (ActualFormat)
                        *ActualFormat = PhSettingsStores[i].Format;
                    break;
                }
            }
        }
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    selectedIndex = PhpSelectBestSettingsStore(results);

    if (selectedIndex < 0)
    {
        status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    const PH_SETTINGS_STORE_DESCRIPTOR* selectedStore = &PhSettingsStores[selectedIndex];

    switch (selectedStore->Format)
    {
        case SettingsFormatJson:
            status = PhLoadSettingsJson(&results[selectedIndex].FilePath->sr);
            break;
        case SettingsFormatXml:
            status = PhLoadSettingsXml(&results[selectedIndex].FilePath->sr);
            break;
        case SettingsFormatKey:
            status = PhLoadSettingsAppKey(&results[selectedIndex].FilePath->sr);
            break;
        case SettingsFormatReg:
            status = PhLoadSettingsKey();
            break;
        case SettingsFormatBin:
            status = PhLoadSettingsBin(&results[selectedIndex].FilePath->sr);
            break;
        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    if (NT_SUCCESS(status))
    {
        if (ActualPath)
        {
            if (results[selectedIndex].FilePath)
                *ActualPath = PhReferenceObject(results[selectedIndex].FilePath);
            else
                *ActualPath = NULL;
        }

        if (ActualFormat)
            *ActualFormat = selectedStore->Format;

        PhSettingsLoadedFormat = selectedStore->Format;
    }

Cleanup:
    PhpFreeDiscoveryResults(results);
    return status;
}

NTSTATUS PhLoadSettings(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    // Detect format from extension
    for (ULONG i = 0; i < PH_SETTINGS_STORE_COUNT; i++)
    {
        if (PhSettingsStores[i].IsFileBased &&
            PhEndsWithStringRef2(FileName, PhSettingsStores[i].Extension, TRUE))
        {
            PhSettingsLoadedFormat = PhSettingsStores[i].Format;

            switch (PhSettingsStores[i].Format)
            {
                case SettingsFormatBin:
                    status = PhLoadSettingsBin(FileName);
                    break;
                case SettingsFormatJson:
                    status = PhLoadSettingsJson(FileName);
                    break;
                case SettingsFormatXml:
                    status = PhLoadSettingsXml(FileName);
                    break;
                case SettingsFormatKey:
                    status = PhLoadSettingsAppKey(FileName);
                    break;
            }
            break;
        }
    }

    return status;
}

NTSTATUS PhSaveSettings(
    _In_opt_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    switch (PhSettingsLoadedFormat)
    {
    case SettingsFormatJson:
        {
            if (FileName)
            {
                status = PhSaveSettingsJson(FileName);
            }
        }
        break;
    case SettingsFormatXml:
        {
            if (FileName)
            {
                status = PhSaveSettingsXml(FileName);
            }
        }
        break;
    case SettingsFormatKey:
        {
            if (FileName)
            {
                status = PhSaveSettingsAppKey(FileName);
            }
        }
        break;
    case SettingsFormatReg:
        {
            status = PhSaveSettingsKey();
        }
        break;
    case SettingsFormatBin:
        {
            if (FileName)
            {
                status = PhSaveSettingsBin(FileName);
            }
        }
        break;
    }

    return status;
}

VOID PhResetSettings(
    _In_ HWND hwnd
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    PhBeginEnumHashtable(PhSettingsHashtable, &enumContext);

    while (setting = PhNextEnumHashtable(&enumContext))
    {
        PhpFreeSettingValue(setting->Type, setting);
        PhSettingFromString(setting->Type, &setting->DefaultValue, NULL, setting);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

NTSTATUS PhResetSettingsFile(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PVOID data = NULL;
    SIZE_T dataLength = 0;
    CHAR jsonData[] = "{}";
    CHAR xmlData[] = "<settings></settings>";
    PH_SETTINGS_FORMAT detectedFormat = SettingsFormatJson;
    PH_SETTINGS_BIN_HEADER binData = { PH_SETTINGS_BIN_SIGNATURE, PH_SETTINGS_BIN_VERSION, 0 };

    // Detect format from file extension
    for (ULONG i = 0; i < PH_SETTINGS_STORE_COUNT; i++)
    {
        if (PhSettingsStores[i].IsFileBased &&
            PhEndsWithStringRef2(FileName, PhSettingsStores[i].Extension, TRUE))
        {
            detectedFormat = PhSettingsStores[i].Format;
            break;
        }
    }

    // Select appropriate reset data based on format
    switch (detectedFormat)
    {
        case SettingsFormatJson:
            data = jsonData;
            dataLength = sizeof(jsonData) - 1;
            break;
        case SettingsFormatXml:
            data = xmlData;
            dataLength = sizeof(xmlData) - 1;
            break;
        case SettingsFormatKey:
            // For AppKey/DAT files, delete and recreate is better
            PhDeleteFile(FileName);
            return STATUS_SUCCESS;
        case SettingsFormatReg:
            // Registry format doesn't use file - should not be called
            return STATUS_NOT_SUPPORTED;
        case SettingsFormatBin:
            data = &binData;
            dataLength = sizeof(PH_SETTINGS_BIN_HEADER);
            break;
        default:
            // Unknown format - default to JSON
            data = jsonData;
            dataLength = sizeof(jsonData) - 1;
            break;
    }

    // Overwrite the file with empty valid content
    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        PhWriteFile(fileHandle, data, (ULONG)dataLength, NULL, NULL);
        NtClose(fileHandle);
    }

    return status;
}

VOID PhAddSetting(
    _In_ PH_SETTING_TYPE Type,
    _In_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF DefaultValue
    )
{
    PH_SETTING setting;

    memset(&setting, 0, sizeof(PH_SETTING));
    setting.Type = Type;
    setting.Name = *Name;
    setting.DefaultValue = *DefaultValue;
    memset(&setting.u, 0, sizeof(setting.u));

    PhSettingFromString(Type, &setting.DefaultValue, NULL, &setting);

    PhAddEntryHashtable(PhSettingsHashtable, &setting);
}

VOID PhAddSettings(
    _In_ PPH_SETTING_CREATE Settings,
    _In_ ULONG NumberOfSettings
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    for (i = 0; i < NumberOfSettings; i++)
    {
        PH_STRINGREF name;
        PH_STRINGREF defaultValue;

        PhInitializeStringRefLongHint(&name, Settings[i].Name);
        PhInitializeStringRefLongHint(&defaultValue, Settings[i].DefaultValue);
        PhAddSetting(Settings[i].Type, &name, &defaultValue);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

PPH_SETTING PhGetSetting(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockShared(&PhSettingsLock);
    setting = PhpLookupSetting(Name);
    PhReleaseQueuedLockShared(&PhSettingsLock);

    return setting;
}

VOID PhLoadWindowPlacementFromRectangle(
    _In_ PCWSTR PositionSettingName,
    _In_ PCWSTR SizeSettingName,
    _Inout_ PPH_RECTANGLE WindowRectangle
    )
{
    PH_INTEGER_PAIR windowIntegerPair = { 0 };
    PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = NULL;
    LONG windowDpi;
    RECT windowRect;

    windowIntegerPair = PhGetIntegerPairSetting(PositionSettingName);
    scalableIntegerPair = PhGetScalableIntegerPairSetting(SizeSettingName, FALSE, 0);

    memset(WindowRectangle, 0, sizeof(PH_RECTANGLE));
    WindowRectangle->Position = windowIntegerPair;
    WindowRectangle->Size = scalableIntegerPair->Pair;

    PhRectangleToRect(&windowRect, WindowRectangle);
    windowDpi = PhGetMonitorDpi(NULL, &windowRect);

    PhScalableIntegerPairToScale(scalableIntegerPair, windowDpi);
    PhAdjustRectangleToWorkingArea(NULL, WindowRectangle);
}

BOOLEAN PhLoadWindowPlacementFromSetting(
    _In_opt_ PCWSTR PositionSettingName,
    _In_opt_ PCWSTR SizeSettingName,
    _In_ HWND WindowHandle
    )
{
    if (PositionSettingName && SizeSettingName)
    {
        PH_INTEGER_PAIR windowIntegerPair = { 0 };
        PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = NULL;
        PH_RECTANGLE windowRectangle = { 0 };
        LONG dpi;
        RECT rectForAdjust;

        windowIntegerPair = PhGetIntegerPairSetting(PositionSettingName);
        scalableIntegerPair = PhGetScalableIntegerPairSetting(SizeSettingName, FALSE, 0);
        windowRectangle.Position = windowIntegerPair;
        windowRectangle.Size = scalableIntegerPair->Pair;

        if (windowRectangle.Position.X == 0)
            return FALSE;

        PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

        // Update the window position before querying the DPI or changing the size. (dmex)
        SetWindowPos(
            WindowHandle,
            NULL,
            windowRectangle.Left,
            windowRectangle.Top,
            0,
            0,
            SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
            );

        //dpi = PhGetMonitorDpiFromRect(&windowRectangle);
        dpi = PhGetWindowDpi(WindowHandle);
        PhScalableIntegerPairToScale(scalableIntegerPair, dpi);

        RtlZeroMemory(&windowRectangle, sizeof(PH_RECTANGLE));
        windowRectangle.Position = windowIntegerPair;
        windowRectangle.Size = scalableIntegerPair->Pair;

        // Let the window adjust for the minimum size if needed.
        PhRectangleToRect(&rectForAdjust, &windowRectangle);
        SendMessage(WindowHandle, WM_SIZING, WMSZ_BOTTOMRIGHT, (LPARAM)&rectForAdjust);
        PhRectToRectangle(&windowRectangle, &rectForAdjust);

        MoveWindow(WindowHandle, windowRectangle.Left, windowRectangle.Top,
            windowRectangle.Width, windowRectangle.Height, FALSE);
    }
    else
    {
        PH_RECTANGLE windowRectangle = { 0 };
        PH_INTEGER_PAIR position = { 0 };
        PH_INTEGER_PAIR size;
        ULONG flags;
        LONG dpi;

        flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER;

        if (PositionSettingName)
        {
            position = PhGetIntegerPairSetting(PositionSettingName);
            ClearFlag(flags, SWP_NOMOVE);
        }
        else
        {
            position.X = 0;
            position.Y = 0;
        }

        if (SizeSettingName)
        {
            //RECT rect;
            //
            //windowRectangle.Position = position;
            //rect = PhRectangleToRect(windowRectangle);
            //dpi = PhGetMonitorDpi(&rect);
            dpi = PhGetWindowDpi(WindowHandle);
            size = PhGetScalableIntegerPairSetting(SizeSettingName, TRUE, dpi)->Pair;
            ClearFlag(flags, SWP_NOSIZE);
        }
        else
        {
            RECT windowRect;

            //size.X = 16;
            //size.Y = 16;

            if (!PhGetWindowRect(WindowHandle, &windowRect))
                return FALSE;

            size.X = windowRect.right - windowRect.left;
            size.Y = windowRect.bottom - windowRect.top;
        }

        // Make sure the window doesn't get positioned on disconnected monitors. (dmex)
        windowRectangle.Position = position;
        windowRectangle.Size = size;
        PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

        SetWindowPos(WindowHandle, NULL, windowRectangle.Left, windowRectangle.Top, size.X, size.Y, flags);
    }

    return TRUE;
}

VOID PhSaveWindowPlacementToSetting(
    _In_opt_ PCWSTR PositionSettingName,
    _In_opt_ PCWSTR SizeSettingName,
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };
    PH_RECTANGLE windowRectangle;
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
    //RECT rect;
    LONG dpi;

    GetWindowPlacement(WindowHandle, &placement);
    PhRectToRectangle(&windowRectangle, &placement.rcNormalPosition);

    // The rectangle is in workspace coordinates. Convert the values back to screen coordinates.
    if (GetMonitorInfo(MonitorFromRect(&placement.rcNormalPosition, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
    {
        windowRectangle.Left += monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
        windowRectangle.Top += monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;
    }

    //PhRectangleToRect(&rect, &windowRectangle);
    //dpi = PhGetMonitorDpi(&rect);
    dpi = PhGetWindowDpi(WindowHandle);

    if (PositionSettingName)
        PhSetIntegerPairSetting(PositionSettingName, windowRectangle.Position);
    if (SizeSettingName)
        PhSetScalableIntegerPairSetting2(SizeSettingName, windowRectangle.Size, dpi);
}

BOOLEAN PhLoadListViewColumnSettings(
    _In_ HWND ListViewHandle,
    _In_ PPH_STRING Settings
    )
{
#define ORDER_LIMIT 50
    PH_STRINGREF remainingPart;
    ULONG columnIndex;
    ULONG orderArray[ORDER_LIMIT]; // HACK, but reasonable limit
    ULONG maxOrder;
    LONG dpi = 0;

#ifdef DEBUG
    HWND headerHandle = ListView_GetHeader(ListViewHandle);
    assert(Header_GetItemCount(headerHandle) < ORDER_LIMIT);
#endif

    if (PhIsNullOrEmptyString(Settings))
        return FALSE;

    remainingPart = Settings->sr;
    columnIndex = 0;
    memset(orderArray, 0, sizeof(orderArray));
    maxOrder = 0;

    if (WindowsVersion >= WINDOWS_10)
    {
        dpi = PhGetWindowDpi(ListViewHandle);
    }

    while (remainingPart.Length != 0)
    {
        PH_STRINGREF columnPart;
        PH_STRINGREF orderPart;
        PH_STRINGREF widthPart;
        ULONG64 integer;
        ULONG order;
        LONG width;
        LVCOLUMN lvColumn;

        PhSplitStringRefAtChar(&remainingPart, L'|', &columnPart, &remainingPart);

        if (columnPart.Length == 0)
            return FALSE;

        PhSplitStringRefAtChar(&columnPart, L',', &orderPart, &widthPart);

        if (orderPart.Length == 0 || widthPart.Length == 0)
            return FALSE;

        // Order

        if (!PhStringToInteger64(&orderPart, 10, &integer))
            return FALSE;

        order = (ULONG)integer;

        if (order < ORDER_LIMIT)
        {
            orderArray[order] = columnIndex;

            if (maxOrder < order + 1)
                maxOrder = order + 1;
        }

        // Width

        if (!PhStringToInteger64(&widthPart, 10, &integer))
            return FALSE;

        width = (LONG)integer;

        lvColumn.mask = LVCF_WIDTH;
        lvColumn.cx = WindowsVersion >= WINDOWS_10 ? PhScaleToDisplay(width, dpi) : width;
        ListView_SetColumn(ListViewHandle, columnIndex, &lvColumn);

        columnIndex++;
    }

    ListView_SetColumnOrderArray(ListViewHandle, maxOrder, orderArray);

    return TRUE;
}

PPH_STRING PhSaveListViewColumnSettings(
    _In_ HWND ListViewHandle
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i = 0;
    LVCOLUMN lvColumn;
    LONG dpiValue;

    PhInitializeStringBuilder(&stringBuilder, 20);

    dpiValue = PhGetWindowDpi(ListViewHandle);

    //{
    //    PH_FORMAT format[3];
    //    SIZE_T returnLength;
    //    WCHAR buffer[PH_INT64_STR_LEN_1];
    //
    //    // @%lu|
    //    PhInitFormatC(&format[0], L'@');
    //    PhInitFormatU(&format[1], dpiValue);
    //    PhInitFormatC(&format[2], L'|');
    //
    //    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
    //    {
    //        PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
    //    }
    //    else
    //    {
    //        PhAppendFormatStringBuilder(&stringBuilder, L"@%lu|", dpiValue);
    //    }
    //}

    lvColumn.mask = LVCF_WIDTH | LVCF_ORDER;

    while (ListView_GetColumn(ListViewHandle, i, &lvColumn))
    {
        PH_FORMAT format[4];
        SIZE_T returnLength;
        WCHAR buffer[PH_INT64_STR_LEN_1];

        // %u,%u|
        PhInitFormatU(&format[0], lvColumn.iOrder);
        PhInitFormatC(&format[1], L',');
        PhInitFormatU(&format[2], WindowsVersion >= WINDOWS_10 ? PhScaleToDefault(lvColumn.cx, dpiValue) : lvColumn.cx);
        PhInitFormatC(&format[3], L'|');

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
        {
            PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
        }
        else
        {
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%u,%u|",
                lvColumn.iOrder,
                WindowsVersion >= WINDOWS_10 ? PhScaleToDefault(lvColumn.cx, dpiValue) : lvColumn.cx
                );
        }
        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

BOOLEAN PhLoadIListViewColumnSettings(
    _In_ IListView* ListView,
    _In_ PPH_STRING Settings
    )
{
#define ORDER_LIMIT 50
    HWND headerHandle = NULL;
    PH_STRINGREF remainingPart;
    ULONG columnIndex;
    ULONG orderArray[ORDER_LIMIT]; // HACK, but reasonable limit
    ULONG maxOrder;
    LONG dpi = 0;

    if (!SUCCEEDED(IListView_GetHeaderControl(ListView, &headerHandle)))
        return FALSE;
#ifdef DEBUG
    assert(Header_GetItemCount(headerHandle) < ORDER_LIMIT);
#endif
    if (PhIsNullOrEmptyString(Settings))
        return FALSE;

    remainingPart = Settings->sr;
    columnIndex = 0;
    memset(orderArray, 0, sizeof(orderArray));
    maxOrder = 0;

    //if (remainingPart.Length != 0 && remainingPart.Buffer[0] == L'@')
    //{
    //    PH_STRINGREF scalePart;
    //    LONG64 integer;
    //
    //    PhSkipStringRef(&remainingPart, sizeof(WCHAR));
    //    PhSplitStringRefAtChar(&remainingPart, L'|', &scalePart, &remainingPart);
    //
    //    if (scalePart.Length == 0 || !PhStringToInteger64(&scalePart, 10, &integer))
    //        return FALSE;
    //
    //    scale = (LONG)integer;
    //}

    if (WindowsVersion >= WINDOWS_10)
    {
        dpi = PhGetWindowDpi(headerHandle);
    }

    while (remainingPart.Length != 0)
    {
        PH_STRINGREF columnPart;
        PH_STRINGREF orderPart;
        PH_STRINGREF widthPart;
        ULONG64 integer;
        ULONG order;
        LONG width;
        LVCOLUMN lvColumn;

        PhSplitStringRefAtChar(&remainingPart, L'|', &columnPart, &remainingPart);

        if (columnPart.Length == 0)
            return FALSE;

        PhSplitStringRefAtChar(&columnPart, L',', &orderPart, &widthPart);

        if (orderPart.Length == 0 || widthPart.Length == 0)
            return FALSE;

        // Order

        if (!PhStringToInteger64(&orderPart, 10, &integer))
            return FALSE;

        order = (ULONG)integer;

        if (order < ORDER_LIMIT)
        {
            orderArray[order] = columnIndex;

            if (maxOrder < order + 1)
                maxOrder = order + 1;
        }

        // Width

        if (!PhStringToInteger64(&widthPart, 10, &integer))
            return FALSE;

        width = (LONG)integer;

        lvColumn.mask = LVCF_WIDTH;
        lvColumn.cx = WindowsVersion >= WINDOWS_10 ? PhScaleToDisplay(width, dpi) : width;
        IListView_SetColumn(ListView, columnIndex, &lvColumn);

        columnIndex++;
    }

    IListView_SetColumnOrderArray(ListView, maxOrder, orderArray);

    return TRUE;
}

PPH_STRING PhSaveIListViewColumnSettings(
    _In_ IListView* ListView
    )
{
    HWND headerHandle = NULL;
    PH_STRING_BUILDER stringBuilder;
    ULONG i = 0;
    LVCOLUMN lvColumn;
    LONG dpiValue = 0;

    if (!SUCCEEDED(IListView_GetHeaderControl(ListView, &headerHandle)))
        return NULL;

    PhInitializeStringBuilder(&stringBuilder, 20);

    if (WindowsVersion >= WINDOWS_10)
    {
        dpiValue = PhGetWindowDpi(headerHandle);
    }

    //{
    //    PH_FORMAT format[3];
    //    SIZE_T returnLength;
    //    WCHAR buffer[PH_INT64_STR_LEN_1];
    //
    //    // @%lu|
    //    PhInitFormatC(&format[0], L'@');
    //    PhInitFormatU(&format[1], dpiValue);
    //    PhInitFormatC(&format[2], L'|');
    //
    //    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
    //    {
    //        PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
    //    }
    //    else
    //    {
    //        PhAppendFormatStringBuilder(&stringBuilder, L"@%lu|", dpiValue);
    //    }
    //}

    memset(&lvColumn, 0, sizeof(LVCOLUMN));
    lvColumn.mask = LVCF_WIDTH | LVCF_ORDER;

    while (SUCCEEDED(IListView_GetColumn(ListView, i, &lvColumn)))
    {
        PH_FORMAT format[4];
        SIZE_T returnLength;
        WCHAR buffer[PH_INT64_STR_LEN_1];

        // %u,%u|
        PhInitFormatU(&format[0], lvColumn.iOrder);
        PhInitFormatC(&format[1], L',');
        PhInitFormatU(&format[2], WindowsVersion >= WINDOWS_10 ? PhScaleToDefault(lvColumn.cx, dpiValue) : lvColumn.cx);
        PhInitFormatC(&format[3], L'|');

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
        {
            PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
        }
        else
        {
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%u,%u|",
                lvColumn.iOrder,
                WindowsVersion >= WINDOWS_10 ? PhScaleToDefault(lvColumn.cx, dpiValue) : lvColumn.cx
                );
        }
        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PhLoadListViewColumnsFromSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(Name);
    PhLoadListViewColumnSettings(ListViewHandle, string);
    PhDereferenceObject(string);
}

VOID PhSaveListViewColumnsToSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhSaveListViewColumnSettings(ListViewHandle);
    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}

VOID PhLoadIListViewColumnsFromSetting(
    _In_ PCWSTR Name,
    _In_ IListView* ListViewClass
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(Name);
    PhLoadIListViewColumnSettings(ListViewClass, string);
    PhDereferenceObject(string);
}

VOID PhSaveIListViewColumnsToSetting(
    _In_ PCWSTR Name,
    _In_ IListView* ListViewClass
    )
{
    PPH_STRING string;

    string = PhSaveIListViewColumnSettings(ListViewClass);
    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}

VOID PhLoadListViewSortColumnsFromSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;
    ULONG sortColumn = 0;
    PH_SORT_ORDER sortOrder = AscendingSortOrder;
    PH_STRINGREF remainingPart;

    string = PhGetStringSetting(Name);

    if (PhIsNullOrEmptyString(string))
        return;

    remainingPart = string->sr;

    if (remainingPart.Length != 0)
    {
        PH_STRINGREF columnPart;
        PH_STRINGREF orderPart;
        ULONG64 integer;

        if (!PhSplitStringRefAtChar(&remainingPart, L',', &columnPart, &orderPart))
            return;

        if (!PhStringToInteger64(&columnPart, 10, &integer))
            return;

        sortColumn = (ULONG)integer;

        if (!PhStringToInteger64(&orderPart, 10, &integer))
            return;

        sortOrder = (ULONG)integer;
    }

    ExtendedListView_SetSort(ListViewHandle, sortColumn, sortOrder);

    PhDereferenceObject(string);
}

VOID PhSaveListViewSortColumnsToSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;
    ULONG sortColumn = 0;
    PH_SORT_ORDER sortOrder = AscendingSortOrder;

    if (ExtendedListView_GetSort(ListViewHandle, &sortColumn, &sortOrder))
    {
        PH_FORMAT format[3];

        // %lu,%lu
        PhInitFormatU(&format[0], sortColumn);
        PhInitFormatC(&format[1], L',');
        PhInitFormatU(&format[2], sortOrder);

        string = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        string = PhCreateString(L"0,0");
    }

    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}

VOID PhLoadListViewGroupStatesFromSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    ULONG64 countInteger;
    PPH_STRING settingsString;
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    settingsString = PhaGetStringSetting(Name);
    remaining = settingsString->sr;

    if (remaining.Length == 0)
        return;

    if (!PhSplitStringRefAtChar(&remaining, L'|', &part, &remaining))
        return;

    if (!PhStringToInteger64(&part, 10, &countInteger))
        return;

    for (LONG index = 0; index < (LONG)countInteger; index++)
    {
        ULONG64 groupId;
        ULONG64 stateMask;
        PH_STRINGREF groupIdPart;
        PH_STRINGREF stateMaskPart;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L'|', &groupIdPart, &remaining);

        if (groupIdPart.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L'|', &stateMaskPart, &remaining);

        if (stateMaskPart.Length == 0)
            break;

        if (!PhStringToInteger64(&groupIdPart, 10, &groupId))
            break;
        if (!PhStringToInteger64(&stateMaskPart, 10, &stateMask))
            break;

        ListView_SetGroupState(
            ListViewHandle,
            (LONG)groupId,
            LVGS_NORMAL | LVGS_COLLAPSED,
            (UINT)stateMask
            );
    }
}

VOID PhSaveListViewGroupStatesToSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    LONG index;
    LONG count;
    PPH_STRING settingsString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    count = (LONG)ListView_GetGroupCount(ListViewHandle);

    PhAppendFormatStringBuilder(
        &stringBuilder,
        L"%d|",
        count
        );

    for (index = 0; index < count; index++)
    {
        LVGROUP group;
        PH_FORMAT format[4];
        SIZE_T returnLength;
        WCHAR buffer[PH_INT64_STR_LEN_1];

        memset(&group, 0, sizeof(LVGROUP));
        group.cbSize = sizeof(LVGROUP);
        group.mask = LVGF_GROUPID | LVGF_STATE;
        group.stateMask = LVGS_NORMAL | LVGS_COLLAPSED;

        if (ListView_GetGroupInfoByIndex(ListViewHandle, index, &group) == -1)
            continue;

        PhInitFormatD(&format[0], group.iGroupId);
        PhInitFormatC(&format[1], L'|');
        PhInitFormatU(&format[2], group.state);
        PhInitFormatC(&format[3], L'|');

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
        {
            PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
        }
        else
        {
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%d|%u|",
                group.iGroupId,
                group.state
                );
        }
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(Name, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

VOID PhLoadCustomColorList(
    _In_ PCWSTR Name,
    _In_ PULONG CustomColorList,
    _In_ ULONG CustomColorCount
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    if (CustomColorCount != 16)
        return;

    settingsString = PhGetStringSetting(Name);

    if (PhIsNullOrEmptyString(settingsString))
        goto CleanupExit;

    remaining = PhGetStringRef(settingsString);

    for (ULONG i = 0; i < CustomColorCount; i++)
    {
        ULONG64 integer = 0;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L',', &part, &remaining);

        if (PhStringToInteger64(&part, 10, &integer))
        {
            CustomColorList[i] = (COLORREF)integer;
        }
    }

CleanupExit:
    PhClearReference(&settingsString);
}

VOID PhSaveCustomColorList(
    _In_ PCWSTR Name,
    _In_ PULONG CustomColorList,
    _In_ ULONG CustomColorCount
    )
{
    PH_STRING_BUILDER stringBuilder;

    if (CustomColorCount != 16)
        return;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (ULONG i = 0; i < CustomColorCount; i++)
    {
        PH_FORMAT format[2];
        SIZE_T returnLength;
        WCHAR formatBuffer[0x100];

        PhInitFormatU(&format[0], CustomColorList[i]);
        PhInitFormatC(&format[1], L',');

        if (PhFormatToBuffer(
            format,
            RTL_NUMBER_OF(format),
            formatBuffer,
            sizeof(formatBuffer),
            &returnLength
            ))
        {
            PhAppendStringBuilderEx(&stringBuilder, formatBuffer, returnLength - sizeof(UNICODE_NULL));
        }
        else
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%lu,", CustomColorList[i]);
        }
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    PhSetStringSetting2(Name, &stringBuilder.String->sr);

    PhDeleteStringBuilder(&stringBuilder);
}

static VOID PhBytesStripSubstringZ(
    _In_ PSTR String,
    _In_ PCSTR SubString
    )
{
    SIZE_T length = PhCountBytesZ(SubString);

    if (length == 0)
        return;

    PSTR offset = strstr(String, SubString);

    while (offset)
    {
        // Calculate the size of the remaining string (including null terminator) in bytes
        // and shift it over the substring to be removed.
        memmove(offset, offset + length, (PhCountBytesZ(offset + length) + 1) * sizeof(CHAR));

        // Rescan from the beginning of the string to handle nested occurrences
        offset = strstr(String, SubString);
    }
}

static VOID PhStringStripSubstringZ(
    _In_ PWSTR String,
    _In_ PCWSTR SubString
    )
{
    SIZE_T length = PhCountStringZ(SubString);
    
    if (length == 0)
        return;

    PWSTR offset = wcsstr(String, SubString);

    while (offset)
    {
        // Calculate the size of the remaining string (including null terminator) in bytes
        // and shift it over the substring to be removed.
        memmove(offset, offset + length, (PhCountStringZ(offset + length) + 1) * sizeof(WCHAR));

        // Rescan from the beginning of the string to handle nested occurrences
        offset = wcsstr(String, SubString);
    }
}

static PPH_STRING PhRemoveSubstringFromStringSafe(
    _In_ PPH_STRING String,
    _In_ PCPH_STRINGREF Separator,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF firstPart;
    PH_STRINGREF secondPart;
    PH_STRINGREF remainingPart;
    PH_STRING_BUILDER stringBuilder;

    if (!String || !String->Buffer || !Separator || Separator->Length == 0)
        return NULL;

    remainingPart = PhGetStringRef(String);

    PhInitializeStringBuilder(&stringBuilder, 0x1000);

    while (remainingPart.Length != 0)
    {
        if (!PhSplitStringRefAtString(&remainingPart, Separator, IgnoreCase, &firstPart, &secondPart))
            break;

        PhAppendStringBuilder(&stringBuilder, &firstPart);
    }

    if (remainingPart.Length)
    {
        PhAppendStringBuilder(&stringBuilder, &remainingPart);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

static PPH_STRING PhRemoveSubstringFromString(
    _In_ PPH_STRING String,
    _In_ PCPH_STRINGREF Substring,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRING_BUILDER stringBuilder;
    PH_STRINGREF remainingPart;
    PH_STRINGREF stringPart;

    remainingPart = PhGetStringRef(String);

    PhInitializeStringBuilder(&stringBuilder, String->Length);

    while (PhSplitStringRefAtString(&remainingPart, Substring, IgnoreCase, &stringPart, &remainingPart))
    {
        PhAppendStringBuilder(&stringBuilder, &stringPart);
    }

    // If 'remainingPart' still has the same length as the original string,
    // it means the Substring was never found. Return the original to save memory.
    //if (remainingPart.Length == String->Length)
    //{
    //    PhDeleteStringBuilder(&stringBuilder);
    //    return PhReferenceObject(String);
    //}

    // Append the final chunk (or the whole string if no Substring was found)
    if (remainingPart.Length)
    {
        PhAppendStringBuilder(&stringBuilder, &remainingPart);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

static VOID PhRemoveSubstringFromStringUnsafe(
    _In_ PPH_STRING String,
    _In_ PCPH_STRINGREF Separator,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF firstPart;
    PH_STRINGREF secondPart;
    PH_STRINGREF remainingPart;
    PH_STRING_BUILDER stringBuilder;

    remainingPart = PhGetStringRef(String);

    PhInitializeStringBuilder(&stringBuilder, 0x1000);

    while (remainingPart.Length != 0)
    {
        if (!PhSplitStringRefAtString(&remainingPart, Separator, TRUE, &firstPart, &secondPart))
            break;

        // Calculate the destination pointer (end of the first part)
        PWSTR destination = (PWSTR)PTR_ADD_OFFSET(firstPart.Buffer, firstPart.Length);
        // Calculate the source pointer (start of the second part)
        PWSTR source = secondPart.Buffer;

        // Move the remaining part of the string (including the null terminator)
        // over the substring we want to remove.
        memmove(destination, source, secondPart.Length + sizeof(UNICODE_NULL));

        // Update the Length field.
        String->Length -= Separator->Length;

        // Re-initialize the stringRef to the modified string to continue searching.
        // This is necessary because the string length has changed.
        // Since we modified the buffer in-place, the 'STRINGREF' view is now stale.
        // Re-initialize it to the new string state to find subsequent occurrences.
        remainingPart.Buffer = String->Buffer;
        remainingPart.Length = String->Length;
    }
}

NTSTATUS PhConvertSettingsXmlToJson(
    _In_ PCPH_STRINGREF XmlFileName,
    _In_ PCPH_STRINGREF JsonFileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PPH_BYTES fileContent;
    PVOID topNode = NULL;
    PVOID currentNode;
    PVOID object = NULL;
    PPH_STRING settingName;
    PPH_STRING settingValue;
    PPH_LIST strings = NULL;

    status = PhCreateFile(
        &fileHandle,
        XmlFileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileText(
        &fileContent,
        fileHandle,
        FALSE
        );

    NtClose(fileHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    {
        PPH_STRING string = PhConcatStrings(
            14,
            L"P", L"r", L"o", L"c" L"e"
            L"s", L"s", L"H", L"a", L"c",
            L"k", L"e", L"r", L".");
        PPH_BYTES bytes = PhConvertStringRefToUtf8(&string->sr);
        PhBytesStripSubstringZ(fileContent->Buffer, bytes->Buffer);
        PhDereferenceObject(bytes);
        PhDereferenceObject(string);
    }

    topNode = PhLoadXmlObjectFromString(fileContent->Buffer);
    PhDereferenceObject(fileContent);

    if (!topNode)
    {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto CleanupExit;
    }

    if (!(object = PhCreateJsonObject()))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (!(currentNode = PhGetXmlNodeFirstChild(topNode)))
    {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto CleanupExit;
    }

    strings = PhCreateList(1);

    while (currentNode)
    {
        if (settingName = PhGetXmlNodeAttributeText(currentNode, "name"))
        {
            if (settingValue = PhGetXmlNodeOpaqueText(currentNode))
            {
                PPH_BYTES stringName;
                PPH_BYTES stringValue;

                stringName = PhConvertStringToUtf8(settingName);
                stringValue = PhConvertStringToUtf8(settingValue);

                PhAddJsonObject2(
                    object,
                    stringName->Buffer,
                    stringValue->Buffer,
                    stringValue->Length
                    );

                PhAddItemList(strings, stringName);
                PhAddItemList(strings, stringValue);

                PhDereferenceObject(settingValue);
            }

            PhDereferenceObject(settingName);
        }

        currentNode = PhGetXmlNodeNextChild(currentNode);
    }

    status = PhSaveJsonObjectToFile(
        JsonFileName,
        object,
        PH_JSON_TO_STRING_PLAIN | PH_JSON_TO_STRING_PRETTY
        );

    //if (!NT_SUCCESS(status))
    //    goto CleanupExit;
    //
    //status = PhMoveFile(
    //    XmlFileName,
    //    &convertFilePath->sr,
    //    NULL
    //    );

CleanupExit:
    if (object)
    {
        PhFreeJsonObject(object);
    }

    if (topNode)
    {
        PhFreeXmlObject(topNode);
    }

    if (strings)
    {
        PhDereferenceObjects(strings->Items, strings->Count);
        PhDereferenceObject(strings);
    }

    return status;
}

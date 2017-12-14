/*
 * Process Hacker -
 *   settings
 *
 * Copyright (C) 2010-2016 wj32
 * Copyright (C) 2017 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
#include <phbasesup.h>
#include <phutil.h>
#include <settings.h>

#include <commctrl.h>

#include "mxml/mxml.h"

BOOLEAN NTAPI PhpSettingsHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpSettingsHashtableHashFunction(
    _In_ PVOID Entry
    );

ULONG PhpGetCurrentScale(
    VOID
    );

VOID PhpFreeSettingValue(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    );

PVOID PhpLookupSetting(
    _In_ PPH_STRINGREF Name
    );

PPH_HASHTABLE PhSettingsHashtable;
PH_QUEUED_LOCK PhSettingsLock = PH_QUEUED_LOCK_INIT;
PPH_LIST PhIgnoredSettings;

VOID PhSettingsInitialization(
    VOID
    )
{
    PhSettingsHashtable = PhCreateHashtable(
        sizeof(PH_SETTING),
        PhpSettingsHashtableEqualFunction,
        PhpSettingsHashtableHashFunction,
        256
        );
    PhIgnoredSettings = PhCreateList(4);

    PhAddDefaultSettings();
    PhUpdateCachedSettings();
}

BOOLEAN NTAPI PhpSettingsHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_SETTING setting1 = (PPH_SETTING)Entry1;
    PPH_SETTING setting2 = (PPH_SETTING)Entry2;

    return PhEqualStringRef(&setting1->Name, &setting2->Name, FALSE);
}

ULONG NTAPI PhpSettingsHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_SETTING setting = (PPH_SETTING)Entry;

    return PhHashBytes((PUCHAR)setting->Name.Buffer, setting->Name.Length);
}

static ULONG PhpGetCurrentScale(
    VOID
    )
{
    static PH_INITONCE initOnce;
    static ULONG dpi = 96;

    if (PhBeginInitOnce(&initOnce))
    {
        HDC hdc;

        if (hdc = GetDC(NULL))
        {
            dpi = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        }

        PhEndInitOnce(&initOnce);
    }

    return dpi;
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
    case IntegerSettingType:
        {
            return PhFormatString(L"%x", Setting->u.Integer);
        }
    case IntegerPairSettingType:
        {
            PPH_INTEGER_PAIR integerPair = &Setting->u.IntegerPair;

            return PhFormatString(L"%ld,%ld", integerPair->X, integerPair->Y);
        }
    case ScalableIntegerPairSettingType:
        {
            PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = Setting->u.Pointer;

            if (!scalableIntegerPair)
                return PhReferenceEmptyString();

            return PhFormatString(L"@%lu|%ld,%ld", scalableIntegerPair->Scale, scalableIntegerPair->X, scalableIntegerPair->Y);
        }
    }

    return PhReferenceEmptyString();
}

BOOLEAN PhSettingFromString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_STRINGREF StringRef,
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
    case IntegerPairSettingType:
        {
            LONG64 x;
            LONG64 y;
            PH_STRINGREF xString;
            PH_STRINGREF yString;

            if (!PhSplitStringRefAtChar(StringRef, ',', &xString, &yString))
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
    case ScalableIntegerPairSettingType:
        {
            ULONG64 scale;
            LONG64 x;
            LONG64 y;
            PH_STRINGREF stringRef;
            PH_STRINGREF firstPart;
            PH_STRINGREF secondPart;
            PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair;

            stringRef = *StringRef;

            if (stringRef.Length != 0 && stringRef.Buffer[0] == '@')
            {
                PhSkipStringRef(&stringRef, sizeof(WCHAR));

                if (!PhSplitStringRefAtChar(&stringRef, '|', &firstPart, &stringRef))
                    return FALSE;
                if (!PhStringToInteger64(&firstPart, 10, &scale))
                    return FALSE;
            }
            else
            {
                scale = PhpGetCurrentScale();
            }

            if (!PhSplitStringRefAtChar(&stringRef, ',', &firstPart, &secondPart))
                return FALSE;

            if (PhStringToInteger64(&firstPart, 10, &x) && PhStringToInteger64(&secondPart, 10, &y))
            {
                scalableIntegerPair = PhAllocate(sizeof(PH_SCALABLE_INTEGER_PAIR));
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
    _In_ PPH_STRINGREF Name
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

_May_raise_ ULONG PhGetIntegerSetting(
    _In_ PWSTR Name
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;
    ULONG value;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == IntegerSettingType)
    {
        value = setting->u.Integer;
    }
    else
    {
        setting = NULL;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);

    return value;
}

_May_raise_ PH_INTEGER_PAIR PhGetIntegerPairSetting(
    _In_ PWSTR Name
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;
    PH_INTEGER_PAIR value;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == IntegerPairSettingType)
    {
        value = setting->u.IntegerPair;
    }
    else
    {
        setting = NULL;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);

    return value;
}

_May_raise_ PH_SCALABLE_INTEGER_PAIR PhGetScalableIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ BOOLEAN ScaleToCurrent
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;
    PH_SCALABLE_INTEGER_PAIR value;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == ScalableIntegerPairSettingType)
    {
        value = *(PPH_SCALABLE_INTEGER_PAIR)setting->u.Pointer;
    }
    else
    {
        setting = NULL;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);

    if (ScaleToCurrent)
    {
        ULONG currentScale;

        currentScale = PhpGetCurrentScale();

        if (value.Scale != currentScale && value.Scale != 0)
        {
            value.X = PhMultiplyDivideSigned(value.X, currentScale, value.Scale);
            value.Y = PhMultiplyDivideSigned(value.Y, currentScale, value.Scale);
            value.Scale = currentScale;
        }
    }

    return value;
}

_May_raise_ PPH_STRING PhGetStringSetting(
    _In_ PWSTR Name
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;
    PPH_STRING value;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == StringSettingType)
    {
        if (setting->u.Pointer)
        {
            PhSetReference(&value, setting->u.Pointer);
        }
        else
        {
            // Set to NULL, create an empty string
            // outside of the lock.
            value = NULL;
        }
    }
    else
    {
        setting = NULL;
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);

    if (!value)
        value = PhReferenceEmptyString();

    return value;
}

_May_raise_ BOOLEAN PhGetBinarySetting(
    _In_ PWSTR Name,
    _Out_ PVOID Buffer
    )
{
    PPH_STRING setting;
    BOOLEAN result;

    setting = PhGetStringSetting(Name);
    result = PhHexStringToBuffer(&setting->sr, (PUCHAR)Buffer);
    PhDereferenceObject(setting);

    return result;
}

_May_raise_ VOID PhSetIntegerSetting(
    _In_ PWSTR Name,
    _In_ ULONG Value
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == IntegerSettingType)
    {
        setting->u.Integer = Value;
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

_May_raise_ VOID PhSetIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == IntegerPairSettingType)
    {
        setting->u.IntegerPair = Value;
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

_May_raise_ VOID PhSetScalableIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_SCALABLE_INTEGER_PAIR Value
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == ScalableIntegerPairSettingType)
    {
        PhpFreeSettingValue(ScalableIntegerPairSettingType, setting);
        setting->u.Pointer = PhAllocateCopy(&Value, sizeof(PH_SCALABLE_INTEGER_PAIR));
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

_May_raise_ VOID PhSetScalableIntegerPairSetting2(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    )
{
    PH_SCALABLE_INTEGER_PAIR scalableIntegerPair;

    scalableIntegerPair.Pair = Value;
    scalableIntegerPair.Scale = PhpGetCurrentScale();
    PhSetScalableIntegerPairSetting(Name, scalableIntegerPair);
}

_May_raise_ VOID PhSetStringSetting(
    _In_ PWSTR Name,
    _In_ PWSTR Value
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == StringSettingType)
    {
        PhpFreeSettingValue(StringSettingType, setting);
        setting->u.Pointer = PhCreateString(Value);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

_May_raise_ VOID PhSetStringSetting2(
    _In_ PWSTR Name,
    _In_ PPH_STRINGREF Value
    )
{
    PPH_SETTING setting;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(&name);

    if (setting && setting->Type == StringSettingType)
    {
        PhpFreeSettingValue(StringSettingType, setting);
        setting->u.Pointer = PhCreateString2(Value);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

_May_raise_ VOID PhSetBinarySetting(
    _In_ PWSTR Name,
    _In_ PVOID Buffer,
    _In_ ULONG Length
    )
{
    PPH_STRING binaryString;
    
    binaryString = PhBufferToHexString((PUCHAR)Buffer, Length);
    PhSetStringSetting(Name, binaryString->Buffer);
    PhDereferenceObject(binaryString);
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

VOID PhConvertIgnoredSettings(
    VOID
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    for (i = 0; i < PhIgnoredSettings->Count; i++)
    {
        PPH_SETTING ignoredSetting = PhIgnoredSettings->Items[i];
        PPH_SETTING setting;

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

PPH_STRING PhpGetOpaqueXmlNodeText(
    _In_ mxml_node_t *node
    )
{
    if (node->child && node->child->type == MXML_OPAQUE && node->child->value.opaque)
    {
        return PhConvertUtf8ToUtf16(node->child->value.opaque);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

NTSTATUS PhLoadSettings(
    _In_ PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    mxml_node_t *topNode;
    mxml_node_t *currentNode;

    PhpClearIgnoredSettings();

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    if (NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) && fileSize.QuadPart == 0)
    {
        // A blank file is OK. There are no settings to load.
        NtClose(fileHandle);
        return status;
    }

    topNode = mxmlLoadFd(NULL, fileHandle, MXML_OPAQUE_CALLBACK);
    NtClose(fileHandle);

    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    if (topNode->type != MXML_ELEMENT)
    {
        mxmlDelete(topNode);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    currentNode = topNode->child;

    while (currentNode)
    {
        PPH_STRING settingName = NULL;

        if (
            currentNode->type == MXML_ELEMENT &&
            currentNode->value.element.num_attrs >= 1 &&
            _stricmp(currentNode->value.element.attrs[0].name, "name") == 0
            )
        {
            settingName = PhConvertUtf8ToUtf16(currentNode->value.element.attrs[0].value);
        }

        if (settingName)
        {
            PPH_STRING settingValue = 0;

            settingValue = PhpGetOpaqueXmlNodeText(currentNode);

            PhAcquireQueuedLockExclusive(&PhSettingsLock);

            {
                PPH_SETTING setting;

                setting = PhpLookupSetting(&settingName->sr);

                if (setting)
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
                    setting = PhAllocate(sizeof(PH_SETTING));
                    setting->Name.Buffer = PhAllocateCopy(settingName->Buffer, settingName->Length + sizeof(WCHAR));
                    setting->Name.Length = settingName->Length;
                    PhReferenceObject(settingValue);
                    setting->u.Pointer = settingValue;

                    PhAddItemList(PhIgnoredSettings, setting);
                }
            }

            PhReleaseQueuedLockExclusive(&PhSettingsLock);

            PhDereferenceObject(settingValue);
            PhDereferenceObject(settingName);
        }

        currentNode = currentNode->next;
    }

    mxmlDelete(topNode);

    PhUpdateCachedSettings();

    return STATUS_SUCCESS;
}

char *PhpSettingsSaveCallback(
    _In_ mxml_node_t *node,
    _In_ int position
    )
{
    if (PhEqualBytesZ(node->value.element.name, "setting", TRUE))
    {
        if (position == MXML_WS_BEFORE_OPEN)
            return "  ";
        else if (position == MXML_WS_AFTER_CLOSE)
            return "\r\n";
    }
    else if (PhEqualBytesZ(node->value.element.name, "settings", TRUE))
    {
        if (position == MXML_WS_AFTER_OPEN)
            return "\r\n";
    }

    return NULL;
}

mxml_node_t *PhpCreateSettingElement(
    _Inout_ mxml_node_t *ParentNode,
    _In_ PPH_STRINGREF SettingName,
    _In_ PPH_STRINGREF SettingValue
    )
{
    mxml_node_t *settingNode;
    mxml_node_t *textNode;
    PPH_BYTES settingNameUtf8;
    PPH_BYTES settingValueUtf8;

    // Create the setting element.

    settingNode = mxmlNewElement(ParentNode, "setting");

    settingNameUtf8 = PhConvertUtf16ToUtf8Ex(SettingName->Buffer, SettingName->Length);
    mxmlElementSetAttr(settingNode, "name", settingNameUtf8->Buffer);
    PhDereferenceObject(settingNameUtf8);

    // Set the value.

    settingValueUtf8 = PhConvertUtf16ToUtf8Ex(SettingValue->Buffer, SettingValue->Length);
    textNode = mxmlNewOpaque(settingNode, settingValueUtf8->Buffer);
    PhDereferenceObject(settingValueUtf8);

    return settingNode;
}

NTSTATUS PhSaveSettings(
    _In_ PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SETTING setting;

    topNode = mxmlNewElement(MXML_NO_PARENT, "settings");

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
    {
        ULONG i;

        for (i = 0; i < PhIgnoredSettings->Count; i++)
        {
            PPH_STRING settingValue;

            setting = PhIgnoredSettings->Items[i];
            settingValue = setting->u.Pointer;
            PhpCreateSettingElement(topNode, &setting->Name, &settingValue->sr);
        }
    }

    PhReleaseQueuedLockShared(&PhSettingsLock);

    // Create the directory if it does not exist.
    {
        PPH_STRING fullPath;
        ULONG indexOfFileName;
        PPH_STRING directoryName;

        fullPath = PhGetFullPath(FileName, &indexOfFileName);

        if (fullPath)
        {
            if (indexOfFileName != -1)
            {
                directoryName = PhSubstring(fullPath, 0, indexOfFileName);
                //PhCreateDirectory(directoryName);
                PhDereferenceObject(directoryName);
            }

            PhDereferenceObject(fullPath);
        }
    }

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        0,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        mxmlDelete(topNode);
        return status;
    }

    mxmlSaveFd(topNode, fileHandle, PhpSettingsSaveCallback);
    mxmlDelete(topNode);
    NtClose(fileHandle);

    return STATUS_SUCCESS;
}

VOID PhResetSettings(
    VOID
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

VOID PhAddSetting(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF DefaultValue
    )
{
    PH_SETTING setting;

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

VOID PhLoadWindowPlacementFromSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    )
{
    PH_RECTANGLE windowRectangle;

    if (PositionSettingName && SizeSettingName)
    {
        RECT rectForAdjust;

        windowRectangle.Position = PhGetIntegerPairSetting(PositionSettingName);
        windowRectangle.Size = PhGetScalableIntegerPairSetting(SizeSettingName, TRUE).Pair;
        PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

        // Let the window adjust for the minimum size if needed.
        rectForAdjust = PhRectangleToRect(windowRectangle);
        SendMessage(WindowHandle, WM_SIZING, WMSZ_BOTTOMRIGHT, (LPARAM)&rectForAdjust);
        windowRectangle = PhRectToRectangle(rectForAdjust);

        MoveWindow(WindowHandle, windowRectangle.Left, windowRectangle.Top,
            windowRectangle.Width, windowRectangle.Height, FALSE);
    }
    else
    {
        PH_INTEGER_PAIR position;
        PH_INTEGER_PAIR size;
        ULONG flags;

        flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER;

        if (PositionSettingName)
        {
            position = PhGetIntegerPairSetting(PositionSettingName);
            flags &= ~SWP_NOMOVE;
        }
        else
        {
            position.X = 0;
            position.Y = 0;
        }

        if (SizeSettingName)
        {
            size = PhGetScalableIntegerPairSetting(SizeSettingName, TRUE).Pair;
            flags &= ~SWP_NOSIZE;
        }
        else
        {
            size.X = 16;
            size.Y = 16;
        }

        SetWindowPos(WindowHandle, NULL, position.X, position.Y, size.X, size.Y, flags);
    }
}

VOID PhSaveWindowPlacementToSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };
    PH_RECTANGLE windowRectangle;
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

    GetWindowPlacement(WindowHandle, &placement);
    windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

    // The rectangle is in workspace coordinates. Convert the values back to screen coordinates.
    if (GetMonitorInfo(MonitorFromRect(&placement.rcNormalPosition, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
    {
        windowRectangle.Left += monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
        windowRectangle.Top += monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;
    }

    if (PositionSettingName)
        PhSetIntegerPairSetting(PositionSettingName, windowRectangle.Position);
    if (SizeSettingName)
        PhSetScalableIntegerPairSetting2(SizeSettingName, windowRectangle.Size);
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
    ULONG scale;

    if (Settings->Length == 0)
        return FALSE;

    remainingPart = Settings->sr;
    columnIndex = 0;
    memset(orderArray, 0, sizeof(orderArray));
    maxOrder = 0;

    if (remainingPart.Length != 0 && remainingPart.Buffer[0] == '@')
    {
        PH_STRINGREF scalePart;
        ULONG64 integer;

        PhSkipStringRef(&remainingPart, sizeof(WCHAR));
        PhSplitStringRefAtChar(&remainingPart, '|', &scalePart, &remainingPart);

        if (scalePart.Length == 0 || !PhStringToInteger64(&scalePart, 10, &integer))
            return FALSE;

        scale = (ULONG)integer;
    }
    else
    {
        scale = PhGlobalDpi;
    }

    while (remainingPart.Length != 0)
    {
        PH_STRINGREF columnPart;
        PH_STRINGREF orderPart;
        PH_STRINGREF widthPart;
        ULONG64 integer;
        ULONG order;
        ULONG width;
        LVCOLUMN lvColumn;

        PhSplitStringRefAtChar(&remainingPart, '|', &columnPart, &remainingPart);

        if (columnPart.Length == 0)
            return FALSE;

        PhSplitStringRefAtChar(&columnPart, ',', &orderPart, &widthPart);

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

        width = (ULONG)integer;

        if (scale != PhGlobalDpi && scale != 0)
            width = PhMultiplyDivide(width, PhGlobalDpi, scale);

        lvColumn.mask = LVCF_WIDTH;
        lvColumn.cx = width;
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

    PhInitializeStringBuilder(&stringBuilder, 20);

    PhAppendFormatStringBuilder(&stringBuilder, L"@%u|", PhGlobalDpi);

    lvColumn.mask = LVCF_WIDTH | LVCF_ORDER;

    while (ListView_GetColumn(ListViewHandle, i, &lvColumn))
    {
        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%u,%u|",
            lvColumn.iOrder,
            lvColumn.cx
            );
        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PhLoadListViewColumnsFromSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(Name);
    PhLoadListViewColumnSettings(ListViewHandle, string);
    PhDereferenceObject(string);
}

VOID PhSaveListViewColumnsToSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhSaveListViewColumnSettings(ListViewHandle);
    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}
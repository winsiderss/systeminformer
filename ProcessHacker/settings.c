/*
 * Process Hacker - 
 *   program settings
 * 
 * Copyright (C) 2010 wj32
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

#define SETTINGS_PRIVATE
#include <phapp.h>
#include <settings.h>
#include <settingsp.h>

PPH_HASHTABLE PhSettingsHashtable;
PH_QUEUED_LOCK PhSettingsLock = PH_QUEUED_LOCK_INIT;

PPH_LIST PhIgnoredSettings;

VOID PhSettingsInitialization()
{
    PhSettingsHashtable = PhCreateHashtable(
        sizeof(PH_SETTING),
        PhpSettingsHashtableCompareFunction,
        PhpSettingsHashtableHashFunction,
        128
        );
    PhIgnoredSettings = PhCreateList(4);

    PhpAddIntegerSetting(L"AllowOnlyOneInstance", L"0");
    PhpAddIntegerSetting(L"CollapseServicesOnStart", L"0");
    PhpAddStringSetting(L"DbgHelpPath", L"dbghelp.dll");
    PhpAddStringSetting(L"DbgHelpSearchPath", L"");
    PhpAddIntegerSetting(L"DbgHelpUndecorate", L"1");
    PhpAddIntegerSetting(L"ElevationLevel", L"1"); // PromptElevateAction
    PhpAddIntegerSetting(L"EnableKph", L"1");
    PhpAddIntegerSetting(L"EnablePlugins", L"0");
    PhpAddIntegerSetting(L"EnableStage2", L"0");
    PhpAddIntegerSetting(L"EnableWarnings", L"1");
    PhpAddStringSetting(L"EnvironmentListViewColumns", L"");
    PhpAddStringSetting(L"FindObjListViewColumns", L"");
    PhpAddIntegerPairSetting(L"FindObjWindowPosition", L"350,350");
    PhpAddIntegerPairSetting(L"FindObjWindowSize", L"550,420");
    PhpAddIntegerSetting(L"FirstRun", L"1");
    PhpAddStringSetting(L"HandleListViewColumns", L"");
    PhpAddStringSetting(L"HiddenProcessesListViewColumns", L"");
    PhpAddIntegerPairSetting(L"HiddenProcessesWindowPosition", L"400,400");
    PhpAddIntegerPairSetting(L"HiddenProcessesWindowSize", L"520,400");
    PhpAddIntegerSetting(L"HideOnClose", L"0");
    PhpAddIntegerSetting(L"HideOnMinimize", L"0");
    PhpAddIntegerSetting(L"HideUnnamedHandles", L"1");
    PhpAddIntegerSetting(L"HighlightingDuration", L"3e8"); // 1000ms
    PhpAddIntegerSetting(L"IconMask", L"1"); // PH_ICON_CPU_HISTORY
    PhpAddIntegerSetting(L"IconNotifyMask", L"c"); // PH_NOTIFY_SERVICE_CREATE | PH_NOTIFY_SERVICE_DELETE
    PhpAddIntegerSetting(L"IconProcesses", L"a"); // 10
    PhpAddIntegerSetting(L"IconSingleClick", L"0");
    PhpAddIntegerSetting(L"LogEntries", L"200"); // 512
    PhpAddStringSetting(L"LogListViewColumns", L"");
    PhpAddIntegerPairSetting(L"LogWindowPosition", L"300,300");
    PhpAddIntegerPairSetting(L"LogWindowSize", L"450,500");
    PhpAddIntegerSetting(L"MainWindowAlwaysOnTop", L"0");
    PhpAddIntegerSetting(L"MainWindowOpacity", L"0"); // means 100%
    PhpAddIntegerPairSetting(L"MainWindowPosition", L"100,100");
    PhpAddIntegerPairSetting(L"MainWindowSize", L"800,600");
    PhpAddIntegerSetting(L"MainWindowState", L"1");
    PhpAddIntegerSetting(L"MaxSizeUnit", L"6");
    PhpAddStringSetting(L"MemEditGotoChoices", L"");
    PhpAddIntegerPairSetting(L"MemEditPosition", L"450,450");
    PhpAddIntegerPairSetting(L"MemEditSize", L"600,500");
    PhpAddStringSetting(L"MemFilterChoices", L"");
    PhpAddStringSetting(L"MemResultsListViewColumns", L"");
    PhpAddIntegerPairSetting(L"MemResultsPosition", L"300,300");
    PhpAddIntegerPairSetting(L"MemResultsSize", L"500,520");
    PhpAddStringSetting(L"MemoryListViewColumns", L"");
    PhpAddStringSetting(L"MemoryReadWriteAddressChoices", L"");
    PhpAddStringSetting(L"ModuleListViewColumns", L"");
    PhpAddStringSetting(L"NetworkListViewColumns", L"");
    PhpAddIntegerSetting(L"NoPurgeProcessRecords", L"0");
    PhpAddStringSetting(L"PluginsDirectory", L"plugins");
    PhpAddStringSetting(L"ProcessServiceListViewColumns", L"");
    PhpAddStringSetting(L"ProcessTreeListColumns", L"");
    PhpAddStringSetting(L"ProcPropPage", L"General");
    PhpAddIntegerPairSetting(L"ProcPropPosition", L"200,200");
    PhpAddIntegerPairSetting(L"ProcPropSize", L"460,580");
    PhpAddStringSetting(L"ProgramInspectExecutables", L"peview.exe \"%s\"");
    PhpAddStringSetting(L"RunAsProgram", L"");
    PhpAddStringSetting(L"RunAsUserName", L"");
    PhpAddIntegerSetting(L"SampleCount", L"200"); // 512
    PhpAddStringSetting(L"SearchEngine", L"http://www.google.com/search?q=\"%s\"");
    PhpAddStringSetting(L"ServiceListViewColumns", L"");
    PhpAddIntegerSetting(L"StartHidden", L"0");
    PhpAddIntegerSetting(L"SysInfoWindowAlwaysOnTop", L"0");
    PhpAddIntegerSetting(L"SysInfoWindowOneGraphPerCpu", L"0");
    PhpAddIntegerPairSetting(L"SysInfoWindowPosition", L"200,200");
    PhpAddIntegerPairSetting(L"SysInfoWindowSize", L"620,590");
    PhpAddStringSetting(L"ThreadListViewColumns", L"");
    PhpAddStringSetting(L"ThreadStackListViewColumns", L"");
    PhpAddIntegerPairSetting(L"ThreadStackWindowSize", L"420,380");
    PhpAddIntegerSetting(L"UpdateInterval", L"3e8"); // 1000ms

    // Colors are specified with R in the lowest byte, then G, then B.
    // So: bbggrr.
    PhpAddIntegerSetting(L"ColorNew", L"00ff7f"); // Chartreuse
    PhpAddIntegerSetting(L"ColorRemoved", L"283cff");
    PhpAddIntegerSetting(L"UseColorOwnProcesses", L"1");
    PhpAddIntegerSetting(L"ColorOwnProcesses", L"aaffff");
    PhpAddIntegerSetting(L"UseColorSystemProcesses", L"1");
    PhpAddIntegerSetting(L"ColorSystemProcesses", L"ffccaa");
    PhpAddIntegerSetting(L"UseColorServiceProcesses", L"1");
    PhpAddIntegerSetting(L"ColorServiceProcesses", L"ffffcc");
    PhpAddIntegerSetting(L"UseColorJobProcesses", L"1");
    PhpAddIntegerSetting(L"ColorJobProcesses", L"3f85cd"); // Peru
    PhpAddIntegerSetting(L"UseColorWow64Processes", L"1");
    PhpAddIntegerSetting(L"ColorWow64Processes", L"8f8fbc"); // Rosy Brown
    PhpAddIntegerSetting(L"UseColorPosixProcesses", L"1");
    PhpAddIntegerSetting(L"ColorPosixProcesses", L"8b3d48"); // Dark Slate Blue
    PhpAddIntegerSetting(L"UseColorDebuggedProcesses", L"1");
    PhpAddIntegerSetting(L"ColorDebuggedProcesses", L"ffbbcc");
    PhpAddIntegerSetting(L"UseColorElevatedProcesses", L"1");
    PhpAddIntegerSetting(L"ColorElevatedProcesses", L"00aaff");
    PhpAddIntegerSetting(L"UseColorSuspended", L"1");
    PhpAddIntegerSetting(L"ColorSuspended", L"777777");
    PhpAddIntegerSetting(L"UseColorDotNet", L"1");
    PhpAddIntegerSetting(L"ColorDotNet", L"00ffde");
    PhpAddIntegerSetting(L"UseColorPacked", L"1");
    PhpAddIntegerSetting(L"ColorPacked", L"9314ff"); // Deep Pink
    PhpAddIntegerSetting(L"UseColorGuiThreads", L"1");
    PhpAddIntegerSetting(L"ColorGuiThreads", L"77ffff");
    PhpAddIntegerSetting(L"UseColorRelocatedModules", L"1");
    PhpAddIntegerSetting(L"ColorRelocatedModules", L"80c0ff");
    PhpAddIntegerSetting(L"UseColorProtectedHandles", L"1");
    PhpAddIntegerSetting(L"ColorProtectedHandles", L"777777");
    PhpAddIntegerSetting(L"UseColorInheritHandles", L"1");
    PhpAddIntegerSetting(L"ColorInheritHandles", L"ffff77");

    PhpAddIntegerSetting(L"GraphShowText", L"1");
    PhpAddIntegerSetting(L"ColorCpuKernel", L"00ff00");
    PhpAddIntegerSetting(L"ColorCpuUser", L"0000ff");
    PhpAddIntegerSetting(L"ColorIoReadOther", L"00ffff");
    PhpAddIntegerSetting(L"ColorIoWrite", L"ff0077");
    PhpAddIntegerSetting(L"ColorPrivate", L"0077ff");
    PhpAddIntegerSetting(L"ColorPhysical", L"ffff00");

    PhUpdateCachedSettings();
}

VOID PhUpdateCachedSettings()
{
#define UPDATE_INTEGER_CS(Name) (PhCs##Name = PhGetIntegerSetting(L#Name)) 

    UPDATE_INTEGER_CS(CollapseServicesOnStart);
    UPDATE_INTEGER_CS(HighlightingDuration);
    UPDATE_INTEGER_CS(UpdateInterval);

    UPDATE_INTEGER_CS(ColorNew);
    UPDATE_INTEGER_CS(ColorRemoved);
    UPDATE_INTEGER_CS(UseColorOwnProcesses);
    UPDATE_INTEGER_CS(ColorOwnProcesses);
    UPDATE_INTEGER_CS(UseColorSystemProcesses);
    UPDATE_INTEGER_CS(ColorSystemProcesses);
    UPDATE_INTEGER_CS(UseColorServiceProcesses);
    UPDATE_INTEGER_CS(ColorServiceProcesses);
    UPDATE_INTEGER_CS(UseColorJobProcesses);
    UPDATE_INTEGER_CS(ColorJobProcesses);
    UPDATE_INTEGER_CS(UseColorWow64Processes);
    UPDATE_INTEGER_CS(ColorWow64Processes);
    UPDATE_INTEGER_CS(UseColorPosixProcesses);
    UPDATE_INTEGER_CS(ColorPosixProcesses);
    UPDATE_INTEGER_CS(UseColorDebuggedProcesses);
    UPDATE_INTEGER_CS(ColorDebuggedProcesses);
    UPDATE_INTEGER_CS(UseColorElevatedProcesses);
    UPDATE_INTEGER_CS(ColorElevatedProcesses);
    UPDATE_INTEGER_CS(UseColorSuspended);
    UPDATE_INTEGER_CS(ColorSuspended);
    UPDATE_INTEGER_CS(UseColorDotNet);
    UPDATE_INTEGER_CS(ColorDotNet);
    UPDATE_INTEGER_CS(UseColorPacked);
    UPDATE_INTEGER_CS(ColorPacked);
    UPDATE_INTEGER_CS(UseColorGuiThreads);
    UPDATE_INTEGER_CS(ColorGuiThreads);
    UPDATE_INTEGER_CS(UseColorRelocatedModules);
    UPDATE_INTEGER_CS(ColorRelocatedModules);
    UPDATE_INTEGER_CS(UseColorProtectedHandles);
    UPDATE_INTEGER_CS(ColorProtectedHandles);
    UPDATE_INTEGER_CS(UseColorInheritHandles);
    UPDATE_INTEGER_CS(ColorInheritHandles);
    UPDATE_INTEGER_CS(GraphShowText);
    UPDATE_INTEGER_CS(ColorCpuKernel);
    UPDATE_INTEGER_CS(ColorCpuUser);
    UPDATE_INTEGER_CS(ColorIoReadOther);
    UPDATE_INTEGER_CS(ColorIoWrite);
    UPDATE_INTEGER_CS(ColorPrivate);
    UPDATE_INTEGER_CS(ColorPhysical);
}

BOOLEAN NTAPI PhpSettingsHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_SETTING setting1 = (PPH_SETTING)Entry1;
    PPH_SETTING setting2 = (PPH_SETTING)Entry2;

    return WSTR_EQUAL(setting1->Name, setting2->Name);
}

ULONG NTAPI PhpSettingsHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPH_SETTING setting = (PPH_SETTING)Entry;

    return PhHashBytes((PBYTE)setting->Name, (ULONG)wcslen(setting->Name) * 2);
}

static VOID PhpAddStringSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    )
{
    PhpAddSetting(StringSettingType, Name, DefaultValue);
}

static VOID PhpAddIntegerSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    )
{
    PhpAddSetting(IntegerSettingType, Name, DefaultValue);
}

static VOID PhpAddIntegerPairSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    )
{
    PhpAddSetting(IntegerPairSettingType, Name, DefaultValue);
}

__assumeLocked static VOID PhpAddSetting(
    __in PH_SETTING_TYPE Type,
    __in PWSTR Name,
    __in PWSTR DefaultValue
    )
{
    PH_SETTING setting;

    setting.Type = Type;
    setting.Name = Name;
    setting.DefaultValue = PhCreateString(DefaultValue);
    setting.Value = NULL;

    PhpSettingFromString(Type, setting.DefaultValue, &setting.Value);

    PhAddEntryHashtable(PhSettingsHashtable, &setting);
}

static PPH_STRING PhpSettingToString(
    __in PH_SETTING_TYPE Type,
    __in PVOID Value
    )
{
    switch (Type)
    {
    case StringSettingType:
        {
            if (!Value)
                return PhCreateString(L"");

            PhReferenceObject(Value);

            return (PPH_STRING)Value;
        }
    case IntegerSettingType:
        {
            return PhFormatString(L"%x", (ULONG)Value);
        }
    case IntegerPairSettingType:
        {
            PPH_INTEGER_PAIR integerPair = (PPH_INTEGER_PAIR)Value;

            return PhFormatString(L"%u,%u", integerPair->X, integerPair->Y);
        }
    }

    return PhCreateString(L"");
}

static BOOLEAN PhpSettingFromString(
    __in PH_SETTING_TYPE Type,
    __in PPH_STRING String,
    __out PPVOID Value
    )
{
    switch (Type)
    {
    case StringSettingType:
        {
            PhReferenceObject(String);
            *Value = String;

            return TRUE;
        }
    case IntegerSettingType:
        {
            ULONG64 integer;

            if (PhStringToInteger64(&String->sr, 16, &integer))
            {
                *Value = (PVOID)(ULONG)integer;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    case IntegerPairSettingType:
        {
            PH_INTEGER_PAIR integerPair;
            LONG64 x;
            LONG64 y;
            ULONG indexOfComma;
            PH_STRINGREF xString;
            PH_STRINGREF yString;

            indexOfComma = PhFindCharInString(String, 0, ',');

            if (indexOfComma == -1)
                return FALSE;

            xString.Buffer = String->Buffer; // start
            xString.Length = (USHORT)(indexOfComma * 2);
            yString.Buffer = &String->Buffer[indexOfComma + 1]; // after the comma
            yString.Length = (USHORT)(String->Length - xString.Length - 2);

            if (PhStringToInteger64(&xString, 10, &x) && PhStringToInteger64(&yString, 10, &y))
            {
                integerPair.X = (LONG)x;
                integerPair.Y = (LONG)y;
                *Value = PhAllocateCopy(&integerPair, sizeof(PH_INTEGER_PAIR));
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
    __in PH_SETTING_TYPE Type,
    __in PVOID Value
    )
{
    switch (Type)
    {
    case StringSettingType:
        if (Value)
            PhDereferenceObject(Value);
        break;
    case IntegerPairSettingType:
        if (Value)
            PhFree(Value);
        break;
    }
}

static PVOID PhpLookupSetting(
    __in PWSTR Name
    )
{
    PH_SETTING lookupSetting;
    PPH_SETTING setting;

    lookupSetting.Name = Name;
    setting = (PPH_SETTING)PhFindEntryHashtable(
        PhSettingsHashtable,
        &lookupSetting
        );

    return setting;
}

__mayRaise ULONG PhGetIntegerSetting(
    __in PWSTR Name
    )
{
    PPH_SETTING setting;
    ULONG value;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == IntegerSettingType)
    {
        value = (ULONG)setting->Value;
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

__mayRaise PH_INTEGER_PAIR PhGetIntegerPairSetting(
    __in PWSTR Name
    )
{
    PPH_SETTING setting;
    PH_INTEGER_PAIR value;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == IntegerPairSettingType)
    {
        if (setting->Value)
            value = *(PPH_INTEGER_PAIR)setting->Value;
        else
            memset(&value, 0, sizeof(PH_INTEGER_PAIR));
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

__mayRaise PPH_STRING PhGetStringSetting(
    __in PWSTR Name
    )
{
    PPH_SETTING setting;
    PPH_STRING value;

    PhAcquireQueuedLockShared(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == StringSettingType)
    {
        if (setting->Value)
        {
            value = (PPH_STRING)setting->Value;
            PhReferenceObject(value);
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
        value = PhCreateString(L"");

    return value;
}

__mayRaise VOID PhSetIntegerSetting(
    __in PWSTR Name,
    __in ULONG Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == IntegerSettingType)
    {
        setting->Value = (PVOID)Value;
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

__mayRaise VOID PhSetIntegerPairSetting(
    __in PWSTR Name,
    __in PH_INTEGER_PAIR Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == IntegerPairSettingType)
    {
        PhpFreeSettingValue(IntegerPairSettingType, setting->Value);
        setting->Value = PhAllocateCopy(&Value, sizeof(PH_INTEGER_PAIR));
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

__mayRaise VOID PhSetStringSetting(
    __in PWSTR Name,
    __in PWSTR Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == StringSettingType)
    {
        PhpFreeSettingValue(StringSettingType, setting->Value);
        setting->Value = PhCreateString(Value);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

__mayRaise VOID PhSetStringSetting2(
    __in PWSTR Name,
    __in PPH_STRINGREF Value
    )
{
    PPH_SETTING setting;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    setting = PhpLookupSetting(Name);

    if (setting && setting->Type == StringSettingType)
    {
        PhpFreeSettingValue(StringSettingType, setting->Value);
        setting->Value = PhCreateStringEx(Value->Buffer, Value->Length);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);

    if (!setting)
        PhRaiseStatus(STATUS_NOT_FOUND);
}

VOID PhpClearIgnoredSettings()
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    for (i = 0; i < PhIgnoredSettings->Count; i++)
    {
        PPH_SETTING setting = PhIgnoredSettings->Items[i];

        PhFree(setting->Name);

        if (setting->Value)
            PhDereferenceObject(setting->Value);

        PhFree(setting);
    }

    PhClearList(PhIgnoredSettings);

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

VOID PhClearIgnoredSettings()
{
    PhpClearIgnoredSettings();
}

mxml_type_t PhpSettingsLoadCallback(
    __in mxml_node_t *node
    )
{
    return MXML_OPAQUE;
}

NTSTATUS PhLoadSettings(
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
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

    topNode = mxmlLoadFd(NULL, fileHandle, PhpSettingsLoadCallback);
    NtClose(fileHandle);

    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    if (!topNode->child)
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
            stricmp(currentNode->value.element.attrs[0].name, "name") == 0
            )
        {
            settingName = PhCreateStringFromAnsi(currentNode->value.element.attrs[0].value);
        }

        if (settingName)
        {
            PPH_STRING settingValue;

            settingValue = PhGetOpaqueXmlNodeText(currentNode);

            PhAcquireQueuedLockExclusive(&PhSettingsLock);

            {
                PPH_SETTING setting;

                setting = PhpLookupSetting(settingName->Buffer);

                if (setting)
                {
                    PhpFreeSettingValue(setting->Type, setting->Value);
                    setting->Value = NULL;

                    if (!PhpSettingFromString(
                        setting->Type,
                        settingValue,
                        &setting->Value
                        ))
                    {
                        PhpSettingFromString(
                            setting->Type,
                            setting->DefaultValue,
                            &setting->Value
                            );
                    }
                }
                else
                {
                    setting = PhAllocate(sizeof(PH_SETTING));
                    setting->Name = PhAllocateCopy(settingName->Buffer, settingName->Length + sizeof(WCHAR));
                    PhReferenceObject(settingValue);
                    setting->Value = settingValue;

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
    __in mxml_node_t *node,
    __in int position
    )
{
    if (STR_IEQUAL(node->value.element.name, "setting"))
    {
        if (position == MXML_WS_BEFORE_OPEN)
            return "  ";
        else if (position == MXML_WS_AFTER_CLOSE)
            return "\n";
    }
    else if (STR_IEQUAL(node->value.element.name, "settings"))
    {
        if (position == MXML_WS_AFTER_OPEN)
            return "\n";
    }

    return NULL;
}

NTSTATUS PhSaveSettings(
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    ULONG enumerationKey = 0;
    PPH_SETTING setting;

    topNode = mxmlNewElement(MXML_NO_PARENT, "settings");

    PhAcquireQueuedLockShared(&PhSettingsLock);

    while (PhEnumHashtable(PhSettingsHashtable, &setting, &enumerationKey))
    {
        mxml_node_t *settingNode;
        mxml_node_t *textNode;
        PPH_ANSI_STRING settingName;
        PPH_STRING settingValue;
        PPH_ANSI_STRING settingValueAnsi;

        // Create the setting element.

        settingNode = mxmlNewElement(topNode, "setting");

        settingName = PhCreateAnsiStringFromUnicode(setting->Name);
        mxmlElementSetAttr(settingNode, "name", settingName->Buffer);
        PhDereferenceObject(settingName);

        // Set the value.

        settingValue = PhpSettingToString(setting->Type, setting->Value);
        settingValueAnsi = PhCreateAnsiStringFromUnicodeEx(settingValue->Buffer, settingValue->Length);
        PhDereferenceObject(settingValue);

        textNode = mxmlNewOpaque(settingNode, settingValueAnsi->Buffer);
        PhDereferenceObject(settingValueAnsi);
    }

    // Write the ignored settings.
    {
        ULONG i;

        for (i = 0; i < PhIgnoredSettings->Count; i++)
        {
            PPH_SETTING setting = PhIgnoredSettings->Items[i];
            mxml_node_t *settingNode;
            mxml_node_t *textNode;
            PPH_ANSI_STRING settingName;
            PPH_STRING settingValue;
            PPH_ANSI_STRING settingValueAnsi;

            // Create the setting element.

            settingNode = mxmlNewElement(topNode, "setting");

            settingName = PhCreateAnsiStringFromUnicode(setting->Name);
            mxmlElementSetAttr(settingNode, "name", settingName->Buffer);
            PhDereferenceObject(settingName);

            // Set the value.

            settingValue = setting->Value;
            settingValueAnsi = PhCreateAnsiStringFromUnicodeEx(settingValue->Buffer, settingValue->Length);
            textNode = mxmlNewOpaque(settingNode, settingValueAnsi->Buffer);
            PhDereferenceObject(settingValueAnsi);
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
                SHCreateDirectoryEx(NULL, directoryName->Buffer, NULL);
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

VOID PhAddSettings(
    __in PPH_SETTING_CREATE Settings,
    __in ULONG NumberOfSettings
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&PhSettingsLock);

    for (i = 0; i < NumberOfSettings; i++)
    {
        PhpAddSetting(Settings[i].Type, Settings[i].Name, Settings[i].DefaultValue);
    }

    PhReleaseQueuedLockExclusive(&PhSettingsLock);
}

#include <settings.h>
#include "mxml/mxml.h"

BOOLEAN NTAPI PhpSettingsHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpSettingsHashtableHashFunction(
    __in PVOID Entry
    );

static VOID PhpAddStringSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

static VOID PhpAddIntegerSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

static VOID PhpAddIntegerPairSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

static VOID PhpAddSetting(
    __in PH_SETTING_TYPE Type,
    __in PWSTR Name,
    __in PVOID DefaultValue
    );

static PPH_STRING PhpSettingToString(
    __in PH_SETTING_TYPE Type,
    __in PVOID Value
    );

static BOOLEAN PhpSettingFromString(
    __in PH_SETTING_TYPE Type,
    __in PPH_STRING String,
    __out PPVOID Value
    );

static VOID PhpFreeSettingValue(
    __in PH_SETTING_TYPE Type,
    __in PVOID Value
    );

static PVOID PhpLookupSetting(
    __in PPH_STRING Name
    );

static PPH_STRING PhpJoinXmlTextNodes(
    __in mxml_node_t *node
    );

PPH_HASHTABLE PhSettingsHashtable;
PH_FAST_LOCK PhSettingsLock;

VOID PhSettingsInitialization()
{
    PhSettingsHashtable = PhCreateHashtable(
        sizeof(PH_SETTING),
        PhpSettingsHashtableCompareFunction,
        PhpSettingsHashtableHashFunction,
        20
        );
    PhInitializeFastLock(&PhSettingsLock);

    PhpAddIntegerPairSetting(L"MainWindowPosition", L"100,100");
    PhpAddIntegerPairSetting(L"MainWindowSize", L"800,600");
}

BOOLEAN NTAPI PhpSettingsHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_SETTING setting1 = (PPH_SETTING)Entry1;
    PPH_SETTING setting2 = (PPH_SETTING)Entry2;

    return PhStringEquals(setting1->Name, setting2->Name, FALSE);
}

ULONG NTAPI PhpSettingsHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPH_SETTING setting = (PPH_SETTING)Entry;

    return PhHashBytesSdbm((PBYTE)setting->Name->Buffer, setting->Name->Length);
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

static VOID PhpAddSetting(
    __in PH_SETTING_TYPE Type,
    __in PWSTR Name,
    __in PWSTR DefaultValue
    )
{
    // No need to lock because we only add settings 
    // during initialization.

    PH_SETTING setting;

    setting.Type = Type;
    setting.Name = PhCreateString(Name);
    setting.DefaultValue = PhCreateString(DefaultValue);
    setting.Value = NULL;

    PhpSettingFromString(Type, setting.DefaultValue, &setting.Value);

    PhAddHashtableEntry(PhSettingsHashtable, &setting);
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
            return PhFormatString(L"%u", (ULONG)Value);
        }
    case IntegerPairSettingType:
        {
            PPH_INTEGER_PAIR integerPair = (PPH_INTEGER_PAIR)Value;

            return PhFormatString(L"%u,%u", integerPair->X, integerPair->Y);
        }
    }
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
            ULONG integer;

            if (swscanf(String->Buffer, L"%u", &integer) != EOF)
            {
                *Value = (PVOID)integer;
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

            if (swscanf(String->Buffer, L"%u,%u", &integerPair.X, &integerPair.Y) != EOF)
            {
                *Value = PhAllocateCopy(&integerPair, sizeof(PH_INTEGER_PAIR));
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }
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
    __in PPH_STRING Name
    )
{
    PH_SETTING lookupSetting;
    PPH_SETTING setting;

    lookupSetting.Name = Name;
    setting = (PPH_SETTING)PhGetHashtableEntry(
        PhSettingsHashtable,
        &lookupSetting
        );

    return setting;
}

static PPH_STRING PhpJoinXmlTextNodes(
    __in mxml_node_t *node
    )
{
    PPH_STRING_BUILDER stringBuilder;
    PPH_STRING string;

    stringBuilder = PhCreateStringBuilder(10);

    while (node)
    {
        if (node->type == MXML_TEXT)
        {
            PPH_STRING textString;

            if (node->value.text.whitespace)
                PhStringBuilderAppend2(stringBuilder, L" ");

            textString = PhCreateStringFromAnsi(node->value.text.string);
            PhStringBuilderAppend(stringBuilder, textString);
            PhDereferenceObject(textString);
        }

        node = node->next;
    }

    string = PhReferenceStringBuilderString(stringBuilder);
    PhDereferenceObject(stringBuilder);

    return string;
}

BOOLEAN PhLoadSettings(
    __in PWSTR FileName
    )
{
    FILE *settingsFile;
    mxml_node_t *topNode;
    mxml_node_t *currentNode;

    settingsFile = _wfopen(FileName, L"r");

    if (!settingsFile)
        return FALSE;

    topNode = mxmlLoadFile(NULL, settingsFile, MXML_NO_CALLBACK);
    fclose(settingsFile);

    if (!topNode)
        return FALSE;

    if (!topNode->child)
    {
        mxmlDelete(topNode);
        return FALSE;
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

            settingValue = PhpJoinXmlTextNodes(currentNode->child);

            PhAcquireFastLockExclusive(&PhSettingsLock);

            {
                PPH_SETTING setting;

                setting = PhpLookupSetting(settingName);

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
            }

            PhReleaseFastLockExclusive(&PhSettingsLock);

            PhDereferenceObject(settingValue);
            PhDereferenceObject(settingName);
        }

        currentNode = currentNode->next;
    }

    mxmlDelete(topNode);

    return TRUE;
}

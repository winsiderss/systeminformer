#ifndef SETTINGS_H
#define SETTINGS_H

#include <ph.h>

typedef enum _PH_SETTING_TYPE
{
    StringSettingType,
    IntegerSettingType,
    IntegerPairSettingType
} PH_SETTING_TYPE, PPH_SETTING_TYPE;

typedef struct _PH_SETTING
{
    PH_SETTING_TYPE Type;
    PWSTR Name;
    PPH_STRING DefaultValue;

    PVOID Value;
} PH_SETTING, *PPH_SETTING;

VOID PhSettingsInitialization();

ULONG PhGetIntegerSetting(
    __in PWSTR Name
    );

PH_INTEGER_PAIR PhGetIntegerPairSetting(
    __in PWSTR Name
    );

PPH_STRING PhGetStringSetting(
    __in PWSTR Name
    );

VOID PhSetIntegerSetting(
    __in PWSTR Name,
    __in ULONG Value
    );

VOID PhSetIntegerPairSetting(
    __in PWSTR Name,
    __in PH_INTEGER_PAIR Value
    );

VOID PhSetStringSetting(
    __in PWSTR Name,
    __in PWSTR Value
    );

BOOLEAN PhLoadSettings(
    __in PWSTR FileName
    );

BOOLEAN PhSaveSettings(
    __in PWSTR FileName
    );

#endif

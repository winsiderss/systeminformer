#ifndef PH_SETTINGSP_H
#define PH_SETTINGSP_H

#include <shlobj.h>

BOOLEAN NTAPI PhpSettingsHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpSettingsHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpAddSetting(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF DefaultValue
    );

PPH_STRING PhpSettingToString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    );

BOOLEAN PhpSettingFromString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_STRINGREF StringRef,
    _In_opt_ PPH_STRING String,
    _Inout_ PPH_SETTING Setting
    );

VOID PhpFreeSettingValue(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    );

PVOID PhpLookupSetting(
    _In_ PPH_STRINGREF Name
    );

#endif

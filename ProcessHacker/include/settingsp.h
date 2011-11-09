#ifndef SETTINGSP_H
#define SETTINGSP_H

#include <shlobj.h>

BOOLEAN NTAPI PhpSettingsHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpSettingsHashtableHashFunction(
    __in PVOID Entry
    );

__assumeLocked VOID PhpAddSetting(
    __in PH_SETTING_TYPE Type,
    __in PPH_STRINGREF Name,
    __in PPH_STRINGREF DefaultValue
    );

PPH_STRING PhpSettingToString(
    __in PH_SETTING_TYPE Type,
    __in PPH_SETTING Setting
    );

BOOLEAN PhpSettingFromString(
    __in PH_SETTING_TYPE Type,
    __in PPH_STRINGREF StringRef,
    __in_opt PPH_STRING String,
    __inout PPH_SETTING Setting
    );

VOID PhpFreeSettingValue(
    __in PH_SETTING_TYPE Type,
    __in PPH_SETTING Setting
    );

PVOID PhpLookupSetting(
    __in PPH_STRINGREF Name
    );

#endif

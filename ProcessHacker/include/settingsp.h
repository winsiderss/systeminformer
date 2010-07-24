#ifndef SETTINGSP_H
#define SETTINGSP_H

#include <shlobj.h>
#include "mxml/mxml.h"

BOOLEAN NTAPI PhpSettingsHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpSettingsHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpAddStringSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

VOID PhpAddIntegerSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

VOID PhpAddIntegerPairSetting(
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

__assumeLocked VOID PhpAddSetting(
    __in PH_SETTING_TYPE Type,
    __in PWSTR Name,
    __in PWSTR DefaultValue
    );

PPH_STRING PhpSettingToString(
    __in PH_SETTING_TYPE Type,
    __in PVOID Value
    );

BOOLEAN PhpSettingFromString(
    __in PH_SETTING_TYPE Type,
    __in PPH_STRING String,
    __out PPVOID Value
    );

VOID PhpFreeSettingValue(
    __in PH_SETTING_TYPE Type,
    __in PVOID Value
    );

PVOID PhpLookupSetting(
    __in PWSTR Name
    );

#endif

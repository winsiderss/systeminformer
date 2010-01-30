#ifndef SETTINGSP_H
#define SETTINGSP_H

#include <shlobj.h>
#include "mxml/mxml.h"

VOID PhpUpdateCachedSettings();

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
    __in PWSTR Name
    );

static PPH_STRING PhpJoinXmlTextNodes(
    __in mxml_node_t *node
    );

#endif

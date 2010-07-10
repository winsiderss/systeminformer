#ifndef SETTINGS_H
#define SETTINGS_H

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

VOID PhUpdateCachedSettings();

__mayRaise ULONG PhGetIntegerSetting(
    __in PWSTR Name
    );

__mayRaise PH_INTEGER_PAIR PhGetIntegerPairSetting(
    __in PWSTR Name
    );

__mayRaise PPH_STRING PhGetStringSetting(
    __in PWSTR Name
    );

__mayRaise VOID PhSetIntegerSetting(
    __in PWSTR Name,
    __in ULONG Value
    );

__mayRaise VOID PhSetIntegerPairSetting(
    __in PWSTR Name,
    __in PH_INTEGER_PAIR Value
    );

__mayRaise VOID PhSetStringSetting(
    __in PWSTR Name,
    __in PWSTR Value
    );

__mayRaise VOID PhSetStringSetting2(
    __in PWSTR Name,
    __in PPH_STRINGREF Value
    );

NTSTATUS PhLoadSettings(
    __in PWSTR FileName
    );

NTSTATUS PhSaveSettings(
    __in PWSTR FileName
    );

#define PHA_GET_STRING_SETTING(Name) ((PPH_STRING)PHA_DEREFERENCE(PhGetStringSetting(Name)))

// Cached settings

#undef EXT

#ifdef SETTINGS_PRIVATE
#define EXT
#else
#define EXT extern
#endif

EXT ULONG PhCsCollapseServicesOnStart;
EXT ULONG PhCsUpdateInterval;

EXT ULONG PhCsColorNew;
EXT ULONG PhCsColorRemoved;
EXT ULONG PhCsUseColorOwnProcesses;
EXT ULONG PhCsColorOwnProcesses;
EXT ULONG PhCsUseColorSystemProcesses;
EXT ULONG PhCsColorSystemProcesses;
EXT ULONG PhCsUseColorServiceProcesses;
EXT ULONG PhCsColorServiceProcesses;
EXT ULONG PhCsUseColorJobProcesses;
EXT ULONG PhCsColorJobProcesses;
EXT ULONG PhCsUseColorWow64Processes;
EXT ULONG PhCsColorWow64Processes;
EXT ULONG PhCsUseColorPosixProcesses;
EXT ULONG PhCsColorPosixProcesses;
EXT ULONG PhCsUseColorDebuggedProcesses;
EXT ULONG PhCsColorDebuggedProcesses;
EXT ULONG PhCsUseColorElevatedProcesses;
EXT ULONG PhCsColorElevatedProcesses;
EXT ULONG PhCsUseColorSuspended;
EXT ULONG PhCsColorSuspended;
EXT ULONG PhCsUseColorDotNet;
EXT ULONG PhCsColorDotNet;
EXT ULONG PhCsUseColorPacked;
EXT ULONG PhCsColorPacked;
EXT ULONG PhCsUseColorGuiThreads;
EXT ULONG PhCsColorGuiThreads;
EXT ULONG PhCsUseColorRelocatedModules;
EXT ULONG PhCsColorRelocatedModules;
EXT ULONG PhCsUseColorProtectedHandles;
EXT ULONG PhCsColorProtectedHandles;
EXT ULONG PhCsUseColorInheritHandles;
EXT ULONG PhCsColorInheritHandles;
EXT ULONG PhCsGraphShowText;

#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(L#Name, PhCs##Name = (Value)))

#endif

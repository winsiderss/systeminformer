#ifndef PH_SETTINGS_H
#define PH_SETTINGS_H

// begin_phapppub
typedef enum _PH_SETTING_TYPE
{
    StringSettingType,
    IntegerSettingType,
    IntegerPairSettingType
} PH_SETTING_TYPE, PPH_SETTING_TYPE;
// end_phapppub

typedef struct _PH_SETTING
{
    PH_SETTING_TYPE Type;
    PH_STRINGREF Name;
    PH_STRINGREF DefaultValue;

    union
    {
        PVOID Pointer;
        ULONG Integer;
        PH_INTEGER_PAIR IntegerPair;
    } u;
} PH_SETTING, *PPH_SETTING;

VOID PhSettingsInitialization(
    VOID
    );

VOID PhUpdateCachedSettings(
    VOID
    );

// begin_phapppub
PHAPPAPI
_May_raise_ ULONG
NTAPI
PhGetIntegerSetting(
    _In_ PWSTR Name
    );

PHAPPAPI
_May_raise_ PH_INTEGER_PAIR
NTAPI
PhGetIntegerPairSetting(
    _In_ PWSTR Name
    );

PHAPPAPI
_May_raise_ PPH_STRING
NTAPI
PhGetStringSetting(
    _In_ PWSTR Name
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetIntegerSetting(
    _In_ PWSTR Name,
    _In_ ULONG Value
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetStringSetting(
    _In_ PWSTR Name,
    _In_ PWSTR Value
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetStringSetting2(
    _In_ PWSTR Name,
    _In_ PPH_STRINGREF Value
    );
// end_phapppub

VOID PhClearIgnoredSettings(
    VOID
    );

VOID PhConvertIgnoredSettings(
    VOID
    );

NTSTATUS PhLoadSettings(
    _In_ PWSTR FileName
    );

NTSTATUS PhSaveSettings(
    _In_ PWSTR FileName
    );

VOID PhResetSettings(
    VOID
    );

#define PhaGetStringSetting(Name) ((PPH_STRING)PhAutoDereferenceObject(PhGetStringSetting(Name))) // phapppub

// begin_phapppub
// High-level settings creation

typedef struct _PH_SETTING_CREATE
{
    PH_SETTING_TYPE Type;
    PWSTR Name;
    PWSTR DefaultValue;
} PH_SETTING_CREATE, *PPH_SETTING_CREATE;

PHAPPAPI
VOID
NTAPI
PhAddSettings(
    _In_ PPH_SETTING_CREATE Settings,
    _In_ ULONG NumberOfSettings
    );
// end_phapppub

// Cached settings

#undef EXT

#ifdef PH_SETTINGS_PRIVATE
#define EXT
#else
#define EXT extern
#endif

EXT ULONG PhCsCollapseServicesOnStart;
EXT ULONG PhCsForceNoParent;
EXT ULONG PhCsHighlightingDuration;
EXT ULONG PhCsPropagateCpuUsage;
EXT ULONG PhCsScrollToNewProcesses;
EXT ULONG PhCsShowCpuBelow001;
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
EXT ULONG PhCsUseColorImmersiveProcesses;
EXT ULONG PhCsColorImmersiveProcesses;
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
EXT ULONG PhCsGraphColorMode;
EXT ULONG PhCsColorCpuKernel;
EXT ULONG PhCsColorCpuUser;
EXT ULONG PhCsColorIoReadOther;
EXT ULONG PhCsColorIoWrite;
EXT ULONG PhCsColorPrivate;
EXT ULONG PhCsColorPhysical;

#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(L#Name, PhCs##Name = (Value)))

#endif

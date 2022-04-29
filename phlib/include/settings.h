#ifndef PHLIB_SETTINGS_H
#define PHLIB_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

// begin_phapppub

// These macros make sure the C strings can be seamlessly converted into
// PH_STRINGREFs at compile time, for a small speed boost.

#define ADD_SETTING_WRAPPER(Type, Name, DefaultValue) \
{ \
    static PH_STRINGREF name = PH_STRINGREF_INIT(Name); \
    static PH_STRINGREF defaultValue = PH_STRINGREF_INIT(DefaultValue); \
    PhAddSetting(Type, &name, &defaultValue); \
}

#define PhpAddStringSetting(A, B) ADD_SETTING_WRAPPER(StringSettingType, A, B)
#define PhpAddIntegerSetting(A, B) ADD_SETTING_WRAPPER(IntegerSettingType, A, B)
#define PhpAddIntegerPairSetting(A, B) ADD_SETTING_WRAPPER(IntegerPairSettingType, A, B)
#define PhpAddScalableIntegerPairSetting(A, B) ADD_SETTING_WRAPPER(ScalableIntegerPairSettingType, A, B)

typedef enum _PH_SETTING_TYPE
{
    StringSettingType,
    IntegerSettingType,
    IntegerPairSettingType,
    ScalableIntegerPairSettingType
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

PHLIBAPI
VOID 
PhSettingsInitialization(
    VOID
    );

// Note: Program specific function.
VOID PhAddDefaultSettings(
    VOID
    );

// Note: Program specific function.
VOID PhUpdateCachedSettings(
    VOID
    );

// private

PPH_STRING PhSettingToString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    );

BOOLEAN PhSettingFromString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_STRINGREF StringRef,
    _In_opt_ PPH_STRING String,
    _Inout_ PPH_SETTING Setting
    );

typedef BOOLEAN (NTAPI *PPH_SETTINGS_ENUM_CALLBACK)(
    _In_ PPH_SETTING Setting,
    _In_ PVOID Context
    );

VOID PhEnumSettings(
    _In_ PPH_SETTINGS_ENUM_CALLBACK Callback,
    _In_ PVOID Context
    );

// begin_phapppub
_May_raise_
PHLIBAPI
ULONG
NTAPI
PhGetIntegerSetting(
    _In_ PWSTR Name
    );

_May_raise_
PHLIBAPI
PH_INTEGER_PAIR
NTAPI
PhGetIntegerPairSetting(
    _In_ PWSTR Name
    );

_May_raise_
PHLIBAPI
PH_SCALABLE_INTEGER_PAIR
NTAPI
PhGetScalableIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ BOOLEAN ScaleToCurrent
    );

_May_raise_
PHLIBAPI
PPH_STRING
NTAPI
PhGetStringSetting(
    _In_ PWSTR Name
    );

FORCEINLINE 
PPH_STRING 
PhGetExpandStringSetting(
    _In_ PWSTR Name
    )
{
    PPH_STRING setting;

    setting = PhGetStringSetting(Name);
#ifdef __cplusplus
    PhMoveReference(reinterpret_cast<PVOID*>(&setting), PhExpandEnvironmentStrings(&setting->sr));
#else
    PhMoveReference(&setting, PhExpandEnvironmentStrings(&setting->sr));
#endif
    return setting;
}

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetIntegerSetting(
    _In_ PWSTR Name,
    _In_ ULONG Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetScalableIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_SCALABLE_INTEGER_PAIR Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetScalableIntegerPairSetting2(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetStringSetting(
    _In_ PWSTR Name,
    _In_ PWSTR Value
    );

_May_raise_
PHLIBAPI
VOID
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

#define PhaGetStringSetting(Name) PH_AUTO_T(PH_STRING, PhGetStringSetting(Name)) // phapppub

// begin_phapppub
// High-level settings creation

VOID PhAddSetting(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF DefaultValue
    );

typedef struct _PH_SETTING_CREATE
{
    PH_SETTING_TYPE Type;
    PWSTR Name;
    PWSTR DefaultValue;
} PH_SETTING_CREATE, *PPH_SETTING_CREATE;

PHLIBAPI
VOID
NTAPI
PhAddSettings(
    _In_ PPH_SETTING_CREATE Settings,
    _In_ ULONG NumberOfSettings
    );

VOID
NTAPI
PhLoadWindowPlacementFromSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    );

VOID
NTAPI
PhSaveWindowPlacementToSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    );

VOID
NTAPI
PhLoadListViewColumnsFromSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

VOID
NTAPI
PhSaveListViewColumnsToSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

VOID
NTAPI
PhLoadListViewSortColumnsFromSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

VOID
NTAPI
PhSaveListViewSortColumnsToSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

VOID
NTAPI
PhLoadListViewGroupStatesFromSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

VOID
NTAPI
PhSaveListViewGroupStatesToSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

VOID
NTAPI
PhLoadCustomColorList(
    _In_ PWSTR Name,
    _In_ PULONG CustomColorList,
    _In_ ULONG CustomColorCount
    );

VOID
NTAPI
PhSaveCustomColorList(
    _In_ PWSTR Name,
    _In_ PULONG CustomColorList,
    _In_ ULONG CustomColorCount
    );
// end_phapppub

#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(TEXT(#Name), PhCs##Name = (Value)))

#ifdef __cplusplus
}
#endif

#endif

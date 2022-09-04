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
    _In_ LONG dpiValue,
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
PhGetIntegerStringRefSetting(
    _In_ PPH_STRINGREF Name
    );

_May_raise_
PHLIBAPI
PH_INTEGER_PAIR
NTAPI
PhGetIntegerPairStringRefSetting(
    _In_ PPH_STRINGREF Name
    );

_May_raise_
PHLIBAPI
PH_SCALABLE_INTEGER_PAIR
NTAPI
PhGetScalableIntegerPairStringRefSetting(
    _In_ PPH_STRINGREF Name,
    _In_ BOOLEAN ScaleToCurrent,
    _In_ LONG dpiValue
    );

_May_raise_
PHLIBAPI
PPH_STRING
NTAPI
PhGetStringRefSetting(
    _In_ PPH_STRINGREF Name
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetIntegerStringRefSetting(
    _In_ PPH_STRINGREF Name,
    _In_ ULONG Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetIntegerPairStringRefSetting(
    _In_ PPH_STRINGREF Name,
    _In_ PH_INTEGER_PAIR Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetScalableIntegerPairStringRefSetting(
    _In_ PPH_STRINGREF Name,
    _In_ PH_SCALABLE_INTEGER_PAIR Value
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetScalableIntegerPairStringRefSetting2(
    _In_ PPH_STRINGREF Name,
    _In_ PH_INTEGER_PAIR Value,
    _In_ LONG dpiValue
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhSetStringRefSetting(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF Value
    );

FORCEINLINE
ULONG
NTAPI
PhGetIntegerSetting(
    _In_ PWSTR Name
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    return PhGetIntegerStringRefSetting(&name);
}

FORCEINLINE
PH_INTEGER_PAIR
NTAPI
PhGetIntegerPairSetting(
    _In_ PWSTR Name
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    return PhGetIntegerPairStringRefSetting(&name);
}

FORCEINLINE
PH_SCALABLE_INTEGER_PAIR
NTAPI
PhGetScalableIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ BOOLEAN ScaleToCurrent,
    _In_ LONG dpiValue
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    return PhGetScalableIntegerPairStringRefSetting(&name, ScaleToCurrent, dpiValue);
}

FORCEINLINE
PPH_STRING
NTAPI
PhGetStringSetting(
    _In_ PWSTR Name
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    return PhGetStringRefSetting(&name);
}

#define PhaGetStringSetting(Name) PH_AUTO_T(PH_STRING, PhGetStringSetting(Name)) // phapppub

FORCEINLINE 
PPH_STRING
NTAPI
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

FORCEINLINE
VOID
NTAPI
PhSetIntegerSetting(
    _In_ PWSTR Name,
    _In_ ULONG Value
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetIntegerStringRefSetting(&name, Value);
}

FORCEINLINE
VOID
NTAPI
PhSetStringSetting(
    _In_ PWSTR Name,
    _In_ PWSTR Value
    )
{
    PH_STRINGREF name;
    PH_STRINGREF value;

    PhInitializeStringRef(&name, Name);
    PhInitializeStringRef(&value, Value);

    PhSetStringRefSetting(&name, &value);
}

FORCEINLINE
VOID
NTAPI
PhSetStringSetting2(
    _In_ PWSTR Name,
    _In_ PPH_STRINGREF Value
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetStringRefSetting(&name, Value);
}

FORCEINLINE
VOID
NTAPI
PhSetIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetIntegerPairStringRefSetting(&name, Value);
}

FORCEINLINE
VOID
NTAPI
PhSetScalableIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_SCALABLE_INTEGER_PAIR Value
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetScalableIntegerPairStringRefSetting(&name, Value);
}

FORCEINLINE
VOID
NTAPI
PhSetScalableIntegerPairSetting2(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value,
    _In_ LONG dpiValue
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetScalableIntegerPairStringRefSetting2(&name, Value, dpiValue);
}

// end_phapppub

VOID PhClearIgnoredSettings(
    VOID
    );

VOID PhConvertIgnoredSettings(
    VOID
    );

NTSTATUS PhLoadSettings(
    _In_ PPH_STRINGREF FileName
    );

NTSTATUS PhSaveSettings(
    _In_ PPH_STRINGREF FileName
    );

VOID PhResetSettings(
    _In_ HWND hwnd
    );

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

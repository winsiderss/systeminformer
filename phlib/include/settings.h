/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2023
 *
 */

#ifndef PHLIB_SETTINGS_H
#define PHLIB_SETTINGS_H

EXTERN_C_START

// begin_phapppub

// These macros make sure the C strings can be seamlessly converted into
// PH_STRINGREFs at compile time, for a small speed boost.

#define ADD_SETTING_WRAPPER(Type, Name, DefaultValue) \
{ \
    static CONST PH_STRINGREF name = PH_STRINGREF_INIT(Name); \
    static CONST PH_STRINGREF defaultValue = PH_STRINGREF_INIT(DefaultValue); \
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
NTAPI
PhSettingsInitialization(
    VOID
    );

// private

PPH_STRING PhSettingToString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PPH_SETTING Setting
    );

BOOLEAN PhSettingFromString(
    _In_ PH_SETTING_TYPE Type,
    _In_ PCPH_STRINGREF StringRef,
    _In_opt_ PPH_STRING String,
    _Inout_ PPH_SETTING Setting
    );

typedef _Function_class_(PH_SETTINGS_ENUM_CALLBACK)
BOOLEAN NTAPI PH_SETTINGS_ENUM_CALLBACK(
    _In_ PPH_SETTING Setting,
    _In_ PVOID Context
    );
typedef PH_SETTINGS_ENUM_CALLBACK* PPH_SETTINGS_ENUM_CALLBACK;

VOID PhEnumSettings(
    _In_ PPH_SETTINGS_ENUM_CALLBACK Callback,
    _In_ PVOID Context
    );

// begin_phapppub

PHLIBAPI
ULONG
NTAPI
PhGetIntegerStringRefSetting(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _Out_ PPH_INTEGER_PAIR IntegerPair
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetScalableIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ BOOLEAN ScaleToDpi,
    _In_ LONG Dpi,
    _Out_ PPH_SCALABLE_INTEGER_PAIR* ScalableIntegerPair
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetStringRefSetting(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
VOID
NTAPI
PhSetIntegerStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ ULONG Value
    );

PHLIBAPI
VOID
NTAPI
PhSetIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ PPH_INTEGER_PAIR Value
    );

PHLIBAPI
VOID
NTAPI
PhSetScalableIntegerPairStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ PPH_SCALABLE_INTEGER_PAIR Value
    );

PHLIBAPI
VOID
NTAPI
PhSetScalableIntegerPairStringRefSetting2(
    _In_ PCPH_STRINGREF Name,
    _In_ PPH_INTEGER_PAIR Value,
    _In_ LONG dpiValue
    );

PHLIBAPI
VOID
NTAPI
PhSetStringRefSetting(
    _In_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF Value
    );

FORCEINLINE
VOID
PhScalableIntegerPairToScale(
    _In_ PPH_SCALABLE_INTEGER_PAIR ScalableIntegerPair,
    _In_ LONG Scale
    )
{
    if (ScalableIntegerPair->Scale != Scale && ScalableIntegerPair->Scale != 0)
    {
        ScalableIntegerPair->X = PhMultiplyDivideSigned(ScalableIntegerPair->X, Scale, ScalableIntegerPair->Scale);
        ScalableIntegerPair->Y = PhMultiplyDivideSigned(ScalableIntegerPair->Y, Scale, ScalableIntegerPair->Scale);
        ScalableIntegerPair->Scale = Scale;
    }
}

FORCEINLINE
VOID
PhScalableIntegerPairScaleToDefault(
    _In_ PPH_SCALABLE_INTEGER_PAIR ScalableIntegerPair
    )
{
    PhScalableIntegerPairToScale(ScalableIntegerPair, USER_DEFAULT_SCREEN_DPI);
}

FORCEINLINE
ULONG
NTAPI
PhGetIntegerSetting(
    _In_ PCWSTR Name
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
    _In_ PCWSTR Name
    )
{
    PH_INTEGER_PAIR scalableIntegerPair = { 0 };
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhGetIntegerPairStringRefSetting(&name, &scalableIntegerPair);

    return scalableIntegerPair;
}

FORCEINLINE
PPH_SCALABLE_INTEGER_PAIR
NTAPI
PhGetScalableIntegerPairSetting(
    _In_ PCWSTR Name,
    _In_ BOOLEAN ScaleToDpi,
    _In_ LONG Dpi
    )
{
    PPH_SCALABLE_INTEGER_PAIR scalableIntegerPair = NULL;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhGetScalableIntegerPairStringRefSetting(&name, ScaleToDpi, Dpi, &scalableIntegerPair);

    return scalableIntegerPair;
}

FORCEINLINE
PPH_STRING
NTAPI
PhGetStringSetting(
    _In_ PCWSTR Name
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
    _In_ PCWSTR Name
    )
{
    PPH_STRING setting;

    setting = PhGetStringSetting(Name);
    PhMoveReference(&setting, PhExpandEnvironmentStrings(&setting->sr));

    return setting;
}

FORCEINLINE
VOID
NTAPI
PhSetIntegerSetting(
    _In_ PCWSTR Name,
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
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
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
    _In_ PCWSTR Name,
    _In_ PCPH_STRINGREF Value
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
    _In_ PCWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetIntegerPairStringRefSetting(&name, &Value);
}

FORCEINLINE
VOID
NTAPI
PhSetScalableIntegerPairSetting(
    _In_ PCWSTR Name,
    _In_ PPH_SCALABLE_INTEGER_PAIR Value
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
    _In_ PCWSTR Name,
    _In_ PH_INTEGER_PAIR Value,
    _In_ LONG dpiValue
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    PhSetScalableIntegerPairStringRefSetting2(&name, &Value, dpiValue);
}

// end_phapppub

VOID PhClearIgnoredSettings(
    VOID
    );

ULONG PhCountIgnoredSettings(
    VOID
    );

VOID PhConvertIgnoredSettings(
    VOID
    );

NTSTATUS PhLoadSettings(
    _In_ PCPH_STRINGREF FileName
    );

NTSTATUS PhSaveSettings(
    _In_ PCPH_STRINGREF FileName
    );

FORCEINLINE
NTSTATUS
PhLoadSettings2(
    _In_ PPH_STRING FileName
    )
{
    if (PhIsNullOrEmptyString(FileName))
        return STATUS_UNSUCCESSFUL;

    return PhLoadSettings(&FileName->sr);
}

FORCEINLINE
NTSTATUS
PhSaveSettings2(
    _In_ PPH_STRING FileName
    )
{
    if (PhIsNullOrEmptyString(FileName))
        return STATUS_UNSUCCESSFUL;

    return PhSaveSettings(&FileName->sr);
}

VOID PhResetSettings(
    _In_ HWND hwnd
    );

NTSTATUS PhResetSettingsFile(
    _In_ PCPH_STRINGREF FileName
    );

// begin_phapppub
// High-level settings creation

VOID PhAddSetting(
    _In_ PH_SETTING_TYPE Type,
    _In_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF DefaultValue
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

PHLIBAPI
PPH_SETTING
NTAPI
PhGetSetting(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
VOID
NTAPI
PhLoadWindowPlacementFromRectangle(
    _In_ PCWSTR PositionSettingName,
    _In_ PCWSTR SizeSettingName,
    _Inout_ PPH_RECTANGLE WindowRectangle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLoadWindowPlacementFromSetting(
    _In_opt_ PCWSTR PositionSettingName,
    _In_opt_ PCWSTR SizeSettingName,
    _In_ HWND WindowHandle
    );

FORCEINLINE
BOOLEAN
NTAPI
PhValidWindowPlacementFromSetting(
    _In_ PCWSTR Name
    )
{
    PH_STRINGREF name;
    PH_INTEGER_PAIR integerPair;

    PhInitializeStringRef(&name, Name);

    if (PhGetIntegerPairStringRefSetting(&name, &integerPair))
    {
        if (integerPair.X != 0)
            return TRUE;
    }

    return FALSE;
}

PHLIBAPI
VOID
NTAPI
PhSaveWindowPlacementToSetting(
    _In_opt_ PCWSTR PositionSettingName,
    _In_opt_ PCWSTR SizeSettingName,
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhLoadListViewColumnsFromSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhSaveListViewColumnsToSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhLoadListViewSortColumnsFromSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhSaveListViewSortColumnsToSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhLoadListViewGroupStatesFromSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhSaveListViewGroupStatesToSetting(
    _In_ PCWSTR Name,
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhLoadCustomColorList(
    _In_ PCWSTR Name,
    _In_ PULONG CustomColorList,
    _In_ ULONG CustomColorCount
    );

PHLIBAPI
VOID
NTAPI
PhSaveCustomColorList(
    _In_ PCWSTR Name,
    _In_ PULONG CustomColorList,
    _In_ ULONG CustomColorCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhConvertSettingsXmlToJson(
    _In_ PCPH_STRINGREF XmlFileName,
    _In_ PCPH_STRINGREF JsonFileName
    );

// end_phapppub

#define PH_GET_INTEGER_CACHED_SETTING(Name) ((PhCs##Name) = PhGetIntegerSetting(TEXT(#Name)))
#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(TEXT(#Name), (PhCs##Name) = (Value)))

EXTERN_C_END

#endif

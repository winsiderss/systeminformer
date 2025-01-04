/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_PHUTIL_H
#define _PH_PHUTIL_H

EXTERN_C_START

extern CONST WCHAR *PhSizeUnitNames[7];
extern ULONG PhMaxSizeUnit;
extern USHORT PhMaxPrecisionUnit;
extern FLOAT PhMaxPrecisionLimit;

typedef struct _PH_INTEGER_PAIR
{
    LONG X;
    LONG Y;
} PH_INTEGER_PAIR, *PPH_INTEGER_PAIR;

typedef struct _PH_SCALABLE_INTEGER_PAIR
{
    union
    {
        PH_INTEGER_PAIR Pair;
        struct
        {
            LONG X;
            LONG Y;
        };
    };
    union
    {
        PH_INTEGER_PAIR Padding;
        struct
        {
            LONG Scale;
            LONG Spare;
        };
    };
} PH_SCALABLE_INTEGER_PAIR, *PPH_SCALABLE_INTEGER_PAIR;

typedef struct _PH_RECTANGLE
{
    union
    {
        PH_INTEGER_PAIR Position;
        struct
        {
            LONG Left;
            LONG Top;
        };
    };
    union
    {
        PH_INTEGER_PAIR Size;
        struct
        {
            LONG Width;
            LONG Height;
        };
    };
} PH_RECTANGLE, *PPH_RECTANGLE;

FORCEINLINE
PH_RECTANGLE
PhRectToRectangle(
    _In_ RECT Rect
    )
{
    PH_RECTANGLE rectangle;

    rectangle.Left = Rect.left;
    rectangle.Top = Rect.top;
    rectangle.Width = Rect.right - Rect.left;
    rectangle.Height = Rect.bottom - Rect.top;

    return rectangle;
}

FORCEINLINE
RECT
PhRectangleToRect(
    _In_ PH_RECTANGLE Rectangle
    )
{
    RECT rect;

    rect.left = Rectangle.Left;
    rect.top = Rectangle.Top;
    rect.right = Rectangle.Left + Rectangle.Width;
    rect.bottom = Rectangle.Top + Rectangle.Height;

    return rect;
}

FORCEINLINE
VOID
PhConvertRect(
    _Inout_ PRECT Rect,
    _In_ PRECT ParentRect
    )
{
    Rect->right = ParentRect->right - ParentRect->left - Rect->right;
    Rect->bottom = ParentRect->bottom - ParentRect->top - Rect->bottom;
}

FORCEINLINE
RECT
PhMapRect(
    _In_ RECT InnerRect,
    _In_ RECT OuterRect
    )
{
    RECT rect;

    rect.left = InnerRect.left - OuterRect.left;
    rect.top = InnerRect.top - OuterRect.top;
    rect.right = InnerRect.right - OuterRect.left;
    rect.bottom = InnerRect.bottom - OuterRect.top;

    return rect;
}

PHLIBAPI
VOID
NTAPI
PhAdjustRectangleToBounds(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID
NTAPI
PhCenterRectangle(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID
NTAPI
PhAdjustRectangleToWorkingArea(
    _In_opt_ HWND WindowHandle,
    _Inout_ PPH_RECTANGLE Rectangle
    );

PHLIBAPI
VOID
NTAPI
PhCenterWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentWindowHandle
    );

// NLS

PHLIBAPI
LANGID
NTAPI
PhGetUserDefaultLangID(
    VOID
    );

PHLIBAPI
LCID
NTAPI
PhGetSystemDefaultLCID(
    VOID
    );

PHLIBAPI
LCID
NTAPI
PhGetUserDefaultLCID(
    VOID
    );

// Time

PHLIBAPI
VOID
NTAPI
PhLargeIntegerToSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSystemTimeToLargeInteger(
    _Out_ PLARGE_INTEGER LargeInteger,
    _In_ PSYSTEMTIME SystemTime
    );

PHLIBAPI
VOID
NTAPI
PhLargeIntegerToLocalSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLocalSystemTimeToLargeInteger(
    _Out_ PLARGE_INTEGER LargeInteger,
    _In_ PSYSTEMTIME SystemTime
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSystemTimeToTzSpecificLocalTime(
    _In_ CONST SYSTEMTIME* UniversalTime,
    _Out_ PSYSTEMTIME LocalTime
    );

// Error messages

PHLIBAPI
PPH_STRING
NTAPI
PhGetMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG MessageTableId,
    _In_ ULONG MessageLanguageId,
    _In_ ULONG MessageId
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetNtMessage(
    _In_ NTSTATUS Status
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetWin32Message(
    _In_ ULONG Result
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetWin32FormatMessage(
    _In_ ULONG Result
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetNtFormatMessage(
    _In_ NTSTATUS Status
    );

PHLIBAPI
LONG
NTAPI
PhShowMessage(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ PCWSTR Format,
    ...
    );

#define PhShowError(WindowHandle, Format, ...) PhShowMessage(WindowHandle, MB_OK | MB_ICONERROR, Format __VA_OPT__(,) __VA_ARGS__)
#define PhShowWarning(WindowHandle, Format, ...) PhShowMessage(WindowHandle, MB_OK | MB_ICONWARNING, Format __VA_OPT__(,) __VA_ARGS__)
#define PhShowInformation(WindowHandle, Format, ...) PhShowMessage(WindowHandle, MB_OK | MB_ICONINFORMATION, Format __VA_OPT__(,) __VA_ARGS__)

PHLIBAPI
LONG
NTAPI
PhShowMessage2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    );

#define TD_OK_BUTTON      0x0001
#define TD_YES_BUTTON     0x0002
#define TD_NO_BUTTON      0x0004
#define TD_CANCEL_BUTTON  0x0008
#define TD_RETRY_BUTTON   0x0010
#define TD_CLOSE_BUTTON   0x0020

#ifndef TD_WARNING_ICON
#define TD_WARNING_ICON         MAKEINTRESOURCEW(-1)
#endif
#ifndef TD_ERROR_ICON
#define TD_ERROR_ICON           MAKEINTRESOURCEW(-2)
#endif
#ifndef TD_INFORMATION_ICON
#define TD_INFORMATION_ICON     MAKEINTRESOURCEW(-3)
#endif
#ifndef TD_SHIELD_ICON
#define TD_SHIELD_ICON          MAKEINTRESOURCEW(-4)
#endif

#define PhShowError2(WindowHandle, Title, Format, ...) PhShowMessage2(WindowHandle, TD_CLOSE_BUTTON, TD_ERROR_ICON, Title, Format __VA_OPT__(,) __VA_ARGS__)
#define PhShowWarning2(WindowHandle, Title, Format, ...) PhShowMessage2(WindowHandle, TD_CLOSE_BUTTON, TD_WARNING_ICON, Title, Format __VA_OPT__(,) __VA_ARGS__)
#define PhShowInformation2(WindowHandle, Title, Format, ...) PhShowMessage2(WindowHandle, TD_CLOSE_BUTTON, TD_INFORMATION_ICON, Title, Format __VA_OPT__(,) __VA_ARGS__)

PHLIBAPI
BOOLEAN
NTAPI
PhShowMessageOneTime(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    );

PHLIBAPI
LONG
NTAPI
PhShowMessageOneTime2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _Out_opt_ PBOOLEAN Checked,
    _In_ PCWSTR Format,
    ...
    );

typedef struct _TASKDIALOGCONFIG TASKDIALOGCONFIG, *PTASKDIALOGCONFIG;

// TDM_NAVIGATE_PAGE is not thread safe and accelerator keys crash the process
// after navigating to the page and pressing ctrl, alt, home or insert keys. (dmex)
FORCEINLINE
VOID
PhTaskDialogNavigatePage(
    _In_ HWND WindowHandle,
    _In_ PTASKDIALOGCONFIG Config
    )
{
#ifdef DEBUG
    assert(HandleToUlong(NtCurrentThreadId()) == GetWindowThreadProcessId(WindowHandle, NULL));
#endif
    #define WM_TDM_NAVIGATE_PAGE (WM_USER + 101)

    SendMessage(WindowHandle, WM_TDM_NAVIGATE_PAGE, 0, (LPARAM)(Config));
}

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhShowTaskDialog(
    _In_ PTASKDIALOGCONFIG Config,
    _Out_opt_ PULONG Button,
    _Out_opt_ PULONG RadioButton,
    _Out_opt_ PBOOLEAN FlagChecked
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetStatusMessage(
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
VOID
NTAPI
PhShowStatus(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowContinueStatus(
    _In_ HWND WindowHandle,
    _In_opt_ PCWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowConfirmMessage(
    _In_ HWND WindowHandle,
    _In_ PCWSTR Verb,
    _In_ PCWSTR Object,
    _In_opt_ PCWSTR Message,
    _In_ BOOLEAN Warning
    );

/**
 * Finds an integer in an array of string-integer pairs.
 *
 * \param KeyValuePairs The array.
 * \param SizeOfKeyValuePairs The size of the array, in bytes.
 * \param String The string to search for.
 * \param Integer A variable which receives the found integer.
 *
 * \return TRUE if the string was found, otherwise FALSE.
 *
 * \remarks The search is case-sensitive.
 */
_Success_(return)
FORCEINLINE
BOOLEAN
PhFindIntegerSiKeyValuePairs(
    _In_ PPCH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PCWSTR String,
    _Out_ PULONG Integer
    )
{
    for (ULONG i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (PhEqualStringZ((PCWSTR)KeyValuePairs[i].Key, String, TRUE))
        {
            *Integer = PtrToUlong(KeyValuePairs[i].Value);
            return TRUE;
        }
    }

    return FALSE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
PhFindIntegerSiKeyValuePairsStringRef(
    _In_ PPCH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PPH_STRINGREF String,
    _Out_ PULONG Integer
    )
{
    for (ULONG i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (PhEqualStringRef((PPH_STRINGREF)KeyValuePairs[i].Key, String, TRUE))
        {
            *Integer = PtrToUlong(KeyValuePairs[i].Value);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Finds a string in an array of string-integer pairs.
 *
 * \param KeyValuePairs The array.
 * \param SizeOfKeyValuePairs The size of the array, in bytes.
 * \param Integer The integer to search for.
 * \param String A variable which receives the found string.
 *
 * \return TRUE if the integer was found, otherwise FALSE.
 */
_Success_(return)
FORCEINLINE
BOOLEAN
PhFindStringSiKeyValuePairs(
    _In_ PPCH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ ULONG Integer,
    _Out_ PWSTR *String
    )
{
    for (ULONG i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (PtrToUlong(KeyValuePairs[i].Value) == Integer)
        {
            *String = (PWSTR)KeyValuePairs[i].Key;
            return TRUE;
        }
    }

    return FALSE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
PhFindStringRefSiKeyValuePairs(
    _In_ PPCH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ ULONG Integer,
    _Out_ PPH_STRINGREF* String
    )
{
    for (ULONG i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (PtrToUlong(KeyValuePairs[i].Value) == Integer)
        {
            *String = (PPH_STRINGREF)KeyValuePairs[i].Key;
            return TRUE;
        }
    }

    return FALSE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
PhIndexStringSiKeyValuePairs(
    _In_ PPCH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ ULONG Integer,
    _Out_ PWSTR *String
    )
{
    if (Integer < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR))
    {
        *String = (PWSTR)KeyValuePairs[Integer].Key;
        return TRUE;
    }

    return PhFindStringSiKeyValuePairs(KeyValuePairs, SizeOfKeyValuePairs, Integer, String);
}

_Success_(return)
FORCEINLINE
BOOLEAN
PhIndexStringRefSiKeyValuePairs(
    _In_ PPCH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ ULONG Integer,
    _Out_ PPH_STRINGREF* String
    )
{
    if (Integer < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR))
    {
        *String = (PPH_STRINGREF)KeyValuePairs[Integer].Key;
        return TRUE;
    }

    return PhFindStringRefSiKeyValuePairs(KeyValuePairs, SizeOfKeyValuePairs, Integer, String);
}

#define GUID_VERSION_MAC 1
#define GUID_VERSION_DCE 2
#define GUID_VERSION_MD5 3
#define GUID_VERSION_RANDOM 4
#define GUID_VERSION_SHA1 5
#define GUID_VERSION_TIME 6
#define GUID_VERSION_EPOCH 7
#define GUID_VERSION_VENDOR 8

#define GUID_VARIANT_NCS_MASK 0x80
#define GUID_VARIANT_NCS 0x00
#define GUID_VARIANT_STANDARD_MASK 0xc0
#define GUID_VARIANT_STANDARD 0x80
#define GUID_VARIANT_MICROSOFT_MASK 0xe0
#define GUID_VARIANT_MICROSOFT 0xc0
#define GUID_VARIANT_RESERVED_MASK 0xe0
#define GUID_VARIANT_RESERVED 0xe0

typedef union _GUID_EX
{
    GUID Guid;
    UCHAR Data[16];
    struct
    {
        ULONG TimeLowPart;
        USHORT TimeMidPart;
        USHORT TimeHighPart;
        UCHAR ClockSequenceHigh;
        UCHAR ClockSequenceLow;
        UCHAR Node[6];
    } s;
    struct
    {
        ULONG Part0;
        USHORT Part32;
        UCHAR Part48;
        UCHAR Part56 : 4;
        UCHAR Version : 4;
        UCHAR Variant;
        UCHAR Part72;
        USHORT Part80;
        ULONG Part96;
    } s2;
} GUID_EX, *PGUID_EX;

PHLIBAPI
VOID
NTAPI
PhGenerateGuid(
    _Out_ PGUID Guid
    );

FORCEINLINE
VOID
NTAPI
PhReverseGuid(
    _Inout_ PGUID Guid
    )
{
    Guid->Data1 = _byteswap_ulong(Guid->Data1);
    Guid->Data2 = _byteswap_ushort(Guid->Data2);
    Guid->Data3 = _byteswap_ushort(Guid->Data3);
}

PHLIBAPI
VOID
NTAPI
PhGenerateGuidFromName(
    _Out_ PGUID Guid,
    _In_ PGUID Namespace,
    _In_ PCHAR Name,
    _In_ ULONG NameLength,
    _In_ UCHAR Version
    );

PHLIBAPI
VOID
NTAPI
PhGenerateClass5Guid(
    _In_ REFGUID NamespaceGuid,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PGUID Guid
    );

DEFINE_GUID(GUID_NAMESPACE_MICROSOFT, 0x70ffd812, 0x4c7f, 0x4c7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

PHLIBAPI
VOID
NTAPI
PhGenerateHardwareIDGuid(
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PGUID Guid
    );

PHLIBAPI
VOID
NTAPI
PhGenerateRandomAlphaString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ SIZE_T Count
    );

FORCEINLINE
VOID
PhGenerateRandomAlphaStringRef(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ SIZE_T Count,
    _Out_ PPH_STRINGREF String
    )
{
    PhGenerateRandomAlphaString(Buffer, Count);
    String->Buffer = Buffer;
    String->Length = (Count * sizeof(WCHAR)) - sizeof(UNICODE_NULL);
}

PHLIBAPI
ULONG64
NTAPI
PhGenerateRandomNumber64(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGenerateRandomNumber(
    _Out_ PLARGE_INTEGER Number
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGenerateRandomSeed(
    _Out_ PLARGE_INTEGER Seed
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEllipsisString(
    _In_ PPH_STRING String,
    _In_ SIZE_T DesiredCount
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEllipsisStringPath(
    _In_ PPH_STRING String,
    _In_ SIZE_T DesiredCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhMatchWildcards(
    _In_ PCWSTR Pattern,
    _In_ PCWSTR String,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEscapeStringForMenuPrefix(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
LONG
NTAPI
PhCompareUnicodeStringZIgnoreMenuPrefix(
    _In_ PCWSTR A,
    _In_ PCWSTR B,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN MatchIfPrefix
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDate(
    _In_opt_ PSYSTEMTIME Date,
    _In_opt_ PCWSTR Format
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatTime(
    _In_opt_ PSYSTEMTIME Time,
    _In_opt_ PCWSTR Format
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDateTime(
    _In_opt_ PSYSTEMTIME DateTime
    );

#define PhaFormatDateTime(DateTime) PH_AUTO_T(PH_STRING, PhFormatDateTime(DateTime))

#define PH_DATETIME_STR_LEN 256
#define PH_DATETIME_STR_LEN_1 (PH_DATETIME_STR_LEN + 1)

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhFormatDateTimeToBuffer(
    _In_opt_ PSYSTEMTIME DateTime,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatTimeSpan(
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatTimeSpanRelative(
    _In_ ULONG64 TimeSpan
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatUInt64(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    );

#define PhaFormatUInt64(Value, GroupDigits) PH_AUTO_T(PH_STRING, PhFormatUInt64((Value), (GroupDigits)))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDecimal(
    _In_ PCWSTR Value,
    _In_ ULONG FractionalDigits,
    _In_ BOOLEAN GroupDigits
    );

#define PhaFormatDecimal(Value, FractionalDigits, GroupDigits) \
    PH_AUTO_T(PH_STRING, PhFormatDecimal((Value), (FractionalDigits), (GroupDigits)))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatSize(
    _In_ ULONG64 Size,
    _In_ ULONG MaxSizeUnit
    );

#define PhaFormatSize(Size, MaxSizeUnit) PH_AUTO_T(PH_STRING, PhFormatSize((Size), (MaxSizeUnit)))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatGuid(
    _In_ PGUID Guid
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFormatGuidToBuffer(
    _In_ PGUID Guid,
    _Writable_bytes_(BufferLength) _When_(BufferLength != 0, _Notnull_) PWCHAR Buffer,
    _In_opt_ USHORT BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhStringToGuid(
    _In_ PPH_STRINGREF GuidString,
    _Out_ PGUID Guid
    );

typedef struct _VS_VERSION_INFO_STRUCT16
{
    USHORT Length;
    USHORT ValueLength;
    CHAR Key[1];
} VS_VERSION_INFO_STRUCT16, *PVS_VERSION_INFO_STRUCT16;

typedef struct _VS_VERSION_INFO_STRUCT32
{
    USHORT Length;
    USHORT ValueLength;
    USHORT Type;
    WCHAR Key[1];
} VS_VERSION_INFO_STRUCT32, *PVS_VERSION_INFO_STRUCT32;

FORCEINLINE
BOOLEAN
PhIsFileVersionInfo32(
    _In_ PVOID VersionInfo
    )
{
    return ((PVS_VERSION_INFO_STRUCT16)VersionInfo)->Key[0] < 32;
}

PHLIBAPI
PVOID
NTAPI
PhGetFileVersionInfo(
    _In_ PCWSTR FileName
    );

PHLIBAPI
PVOID
NTAPI
PhGetFileVersionInfoEx(
    _In_ PPH_STRINGREF FileName
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetFileVersionInfoKey(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo,
    _In_ SIZE_T KeyLength,
    _In_ PCWSTR Key,
    _Out_opt_ PVOID* Buffer
    );

PHLIBAPI
VS_FIXEDFILEINFO*
NTAPI
PhGetFileVersionFixedInfo(
    _In_ PVOID VersionInfo
    );

typedef struct _LANGANDCODEPAGE
{
    USHORT Language;
    USHORT CodePage;
} LANGANDCODEPAGE, *PLANGANDCODEPAGE;

PHLIBAPI
ULONG
NTAPI
PhGetFileVersionInfoLangCodePage(
    _In_ PVOID VersionInfo
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileVersionInfoString(
    _In_ PVOID VersionInfo,
    _In_ PCWSTR SubBlock
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileVersionInfoString2(
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage,
    _In_ PPH_STRINGREF KeyName
    );

typedef struct _PH_IMAGE_VERSION_INFO
{
    PPH_STRING CompanyName;
    PPH_STRING FileDescription;
    PPH_STRING FileVersion;
    PPH_STRING ProductName;
} PH_IMAGE_VERSION_INFO, *PPH_IMAGE_VERSION_INFO;

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhInitializeImageVersionInfo(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PCWSTR FileName
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhInitializeImageVersionInfoEx(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN ExtendedVersionInfo
    );

PHLIBAPI
VOID
NTAPI
PhDeleteImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatImageVersionInfo(
    _In_opt_ PPH_STRING FileName,
    _In_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_opt_ PPH_STRINGREF Indent,
    _In_opt_ ULONG LineLimit
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhInitializeImageVersionInfoCached(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN IsSubsystemProcess,
    _In_ BOOLEAN ExtendedVersion
    );

PHLIBAPI
VOID
NTAPI
PhFlushImageVersionInfoCache(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFullPath(
    _In_ PCWSTR FileName,
    _Out_ PPH_STRING *FullPath,
    _Out_opt_ PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhExpandEnvironmentStrings(
    _In_ PPH_STRINGREF String
    );

FORCEINLINE
PPH_STRING
PhExpandEnvironmentStringsZ(
    _In_ PCWSTR String
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);

    return PhExpandEnvironmentStrings(&string);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetBaseName(
    _In_ PPH_STRING FileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetBaseNameChangeExtension(
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_STRINGREF FileExtension
    );

FORCEINLINE
PPH_STRING
NTAPI
PhGetBaseNameChangeExtensionZ(
    _In_ PPH_STRINGREF FileName,
    _In_ PCWSTR FileExtension
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, FileExtension);

    return PhGetBaseNameChangeExtension(FileName, &string);
}

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetBasePath(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PPH_STRINGREF BasePathName,
    _Out_opt_ PPH_STRINGREF BaseFileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetBaseDirectory(
    _In_ PPH_STRING FileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSystemDirectory(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSystemDirectoryWin32(
    _In_opt_ PPH_STRINGREF AppendPath
    );

FORCEINLINE
PPH_STRING
PhGetSystemDirectoryWin32Z(
    _In_ PCWSTR AppendPath
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, AppendPath);

    return PhGetSystemDirectoryWin32(&string);
}

PHLIBAPI
VOID
NTAPI
PhGetSystemRoot(
    _Out_ PPH_STRINGREF SystemRoot
    );

PHLIBAPI
VOID
NTAPI
PhGetNtSystemRoot(
    _Out_ PPH_STRINGREF NtSystemRoot
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationFileName(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationFileNameWin32(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationDirectory(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationDirectoryWin32(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationDirectoryFileName(
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    );

FORCEINLINE
PPH_STRING
PhGetApplicationDirectoryFileNameZ(
    _In_ PCWSTR FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, FileName);

    return PhGetApplicationDirectoryFileName(&string, NativeFileName);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetTemporaryDirectoryRandomAlphaFileName(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetLocalAppDataDirectory(
    _In_opt_ PPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    );

FORCEINLINE
PPH_STRING
PhGetLocalAppDataDirectoryZ(
    _In_ PCWSTR String,
    _In_ BOOLEAN NativeFileName
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);

    return PhGetLocalAppDataDirectory(&string, NativeFileName);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetRoamingAppDataDirectory(
    _In_opt_ PPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    );

FORCEINLINE
PPH_STRING
PhGetRoamingAppDataDirectoryZ(
    _In_ PCWSTR String,
    _In_ BOOLEAN NativeFileName
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);

    return PhGetRoamingAppDataDirectory(&string, NativeFileName);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationDataFileName(
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    );

#define PH_FOLDERID_LocalAppData 1
#define PH_FOLDERID_RoamingAppData 2
#define PH_FOLDERID_ProgramFiles 3
#define PH_FOLDERID_ProgramData 4

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownLocation(
    _In_ ULONG Folder,
    _In_opt_ PPH_STRINGREF AppendPath,
    _In_ BOOLEAN NativeFileName
    );

FORCEINLINE
PPH_STRING
PhGetKnownLocationZ(
    _In_ ULONG Folder,
    _In_ PCWSTR AppendPath,
    _In_ BOOLEAN NativeFileName
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, AppendPath);

    return PhGetKnownLocation(Folder, &string, NativeFileName);
}

DEFINE_GUID(FOLDERID_LocalAppData, 0xF1B32785, 0x6FBA, 0x4FCF, 0x9D, 0x55, 0x7B, 0x8E, 0x7F, 0x15, 0x70, 0x91);
DEFINE_GUID(FOLDERID_RoamingAppData, 0x3EB685DB, 0x65F9, 0x4CF6, 0xA0, 0x3A, 0xE3, 0xEF, 0x65, 0x72, 0x9F, 0x3D);
DEFINE_GUID(FOLDERID_ProgramFiles, 0x905e63b6, 0xc1bf, 0x494e, 0xb2, 0x9c, 0x65, 0xb7, 0x32, 0xd3, 0xd2, 0x1a);
DEFINE_GUID(FOLDERID_ProgramData, 0x62AB5D82, 0xFDC1, 0x4DC3, 0xA9, 0xDD, 0x07, 0x0D, 0x1D, 0x49, 0x5D, 0x97);

#define PH_KF_FLAG_FORCE_PACKAGE_REDIRECTION 0x1
#define PH_KF_FLAG_FORCE_APPCONTAINER_REDIRECTION 0x2

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownFolderPath(
    _In_ PCGUID Folder,
    _In_opt_ PPH_STRINGREF AppendPath
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownFolderPathEx(
    _In_ PCGUID Folder,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PPH_STRINGREF AppendPath
    );

FORCEINLINE
PPH_STRING
PhGetKnownFolderPathZ(
    _In_ PCGUID Folder,
    _In_ PCWSTR AppendPath
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, AppendPath);

    return PhGetKnownFolderPath(Folder, &string);
}

FORCEINLINE
PPH_STRING
PhGetKnownFolderPathExZ(
    _In_ PCGUID Folder,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_ PCWSTR AppendPath
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, AppendPath);

    return PhGetKnownFolderPathEx(Folder, Flags, TokenHandle, &string);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetTemporaryDirectory(
    _In_opt_ PPH_STRINGREF AppendPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWaitForMultipleObjectsAndPump(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG NumberOfHandles,
    _In_ PHANDLE Handles,
    _In_ ULONG Timeout,
    _In_ ULONG WakeMask
    );

typedef struct _PH_CREATE_PROCESS_INFO
{
    PUNICODE_STRING DllPath;
    PUNICODE_STRING WindowTitle;
    PUNICODE_STRING DesktopInfo;
    PUNICODE_STRING ShellInfo;
    PUNICODE_STRING RuntimeData;
} PH_CREATE_PROCESS_INFO, *PPH_CREATE_PROCESS_INFO;

#define PH_CREATE_PROCESS_INHERIT_HANDLES 0x1
#define PH_CREATE_PROCESS_UNICODE_ENVIRONMENT 0x2
#define PH_CREATE_PROCESS_SUSPENDED 0x4
#define PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB 0x8
#define PH_CREATE_PROCESS_NEW_CONSOLE 0x10
#define PH_CREATE_PROCESS_DEBUG 0x20
#define PH_CREATE_PROCESS_DEBUG_ONLY_THIS_PROCESS 0x40
#define PH_CREATE_PROCESS_EXTENDED_STARTUPINFO 0x80
#define PH_CREATE_PROCESS_DEFAULT_ERROR_MODE 0x100

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcess(
    _In_ PCWSTR FileName,
    _In_opt_ PPH_STRINGREF CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PPH_STRINGREF CurrentDirectory,
    _In_opt_ PPH_CREATE_PROCESS_INFO Information,
    _In_ ULONG Flags,
    _In_opt_ HANDLE ParentProcessHandle,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessWin32(
    _In_opt_ PCWSTR FileName,
    _In_opt_ PCWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCWSTR CurrentDirectory,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessWin32Ex(
    _In_opt_ PCWSTR FileName,
    _In_opt_ PCWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCWSTR CurrentDirectory,
    _In_opt_ PVOID StartupInfo,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

typedef struct _PH_CREATE_PROCESS_AS_USER_INFO
{
    _In_opt_ PCWSTR ApplicationName;
    _In_opt_ PCWSTR CommandLine;
    _In_opt_ PCWSTR CurrentDirectory;
    _In_opt_ PVOID Environment;
    _In_opt_ PCWSTR DesktopName;
    _In_opt_ ULONG SessionId; // use PH_CREATE_PROCESS_SET_SESSION_ID
    union
    {
        struct
        {
            _In_ PCWSTR DomainName;
            _In_ PCWSTR UserName;
            _In_ PCWSTR Password;
            _In_opt_ ULONG LogonType;
            _In_opt_ ULONG LogonFlags;
        };
        _In_ HANDLE ProcessIdWithToken; // use PH_CREATE_PROCESS_USE_PROCESS_TOKEN
        _In_ ULONG SessionIdWithToken; // use PH_CREATE_PROCESS_USE_SESSION_TOKEN
    };
} PH_CREATE_PROCESS_AS_USER_INFO, *PPH_CREATE_PROCESS_AS_USER_INFO;

#define PH_CREATE_PROCESS_USE_PROCESS_TOKEN 0x1000
#define PH_CREATE_PROCESS_USE_SESSION_TOKEN 0x2000
#define PH_CREATE_PROCESS_USE_LINKED_TOKEN 0x10000
#define PH_CREATE_PROCESS_SET_SESSION_ID 0x20000
#define PH_CREATE_PROCESS_WITH_PROFILE 0x40000
#define PH_CREATE_PROCESS_SET_UIACCESS 0x80000

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessAsUser(
    _In_ PPH_CREATE_PROCESS_AS_USER_INFO Information,
    _In_ ULONG Flags,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFilterTokenForLimitedUser(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE NewTokenHandle
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSecurityDescriptorAsString(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
PSECURITY_DESCRIPTOR
NTAPI
PhGetSecurityDescriptorFromString(
    _In_ PCWSTR SecurityDescriptorString
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetObjectSecurityDescriptorAsString(
    _In_ HANDLE Handle,
    _Out_ PPH_STRING* SecurityDescriptorString
    );

typedef struct _SHELLEXECUTEINFOW SHELLEXECUTEINFOW, *PSHELLEXECUTEINFOW;

PHLIBAPI
BOOLEAN
NTAPI
PhShellExecuteWin32(
    _Inout_ PSHELLEXECUTEINFOW ExecInfo
    );

PHLIBAPI
VOID
NTAPI
PhShellExecute(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Parameters
    );

#define PH_SHELL_EXECUTE_DEFAULT 0x0
#define PH_SHELL_EXECUTE_ADMIN 0x1
#define PH_SHELL_EXECUTE_PUMP_MESSAGES 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhShellExecuteEx(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Parameters,
    _In_opt_ PCWSTR Directory,
    _In_ LONG ShowWindowType,
    _In_ ULONG Flags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    );

PHLIBAPI
VOID
NTAPI
PhShellExploreFile(
    _In_ HWND WindowHandle,
    _In_ PCWSTR FileName
    );

PHLIBAPI
VOID
NTAPI
PhShellProperties(
    _In_ HWND WindowHandle,
    _In_ PCWSTR FileName
    );

typedef struct _NOTIFYICONDATAW NOTIFYICONDATAW, *PNOTIFYICONDATAW;

PHLIBAPI
BOOLEAN
NTAPI
PhShellNotifyIcon(
    _In_ ULONG Message,
    _In_ PNOTIFYICONDATAW NotifyIconData
    );

PHLIBAPI
HRESULT
NTAPI
PhShellGetKnownFolderPath(
    _In_ PCGUID rfid, // REFKNOWNFOLDERID
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Outptr_ PWSTR* FolderPath
    );

PHLIBAPI
HRESULT
NTAPI
PhShellGetKnownFolderItem(
    _In_ PCGUID rfid, // REFKNOWNFOLDERID
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_ REFIID riid,
    _Outptr_ PVOID* ppv
    );

PHLIBAPI
PPH_STRING
NTAPI
PhExpandKeyName(
    _In_ PPH_STRING KeyName,
    _In_ BOOLEAN Computer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
    );

FORCEINLINE
PPH_STRING
NTAPI
PhQueryRegistryStringZ(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhQueryRegistryString(KeyHandle, &valueName);
}

PHLIBAPI
ULONG
NTAPI
PhQueryRegistryUlong(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
    );

FORCEINLINE
ULONG
NTAPI
PhQueryRegistryUlongZ(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhQueryRegistryUlong(KeyHandle, &valueName);
}

PHLIBAPI
ULONG64
NTAPI
PhQueryRegistryUlong64(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
    );

FORCEINLINE
ULONG64
NTAPI
PhQueryRegistryUlong64Z(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhQueryRegistryUlong64(KeyHandle, &valueName);
}

typedef struct _PH_FLAG_MAPPING
{
    ULONG Flag1;
    ULONG Flag2;
} PH_FLAG_MAPPING, *PPH_FLAG_MAPPING;

PHLIBAPI
VOID
NTAPI
PhMapFlags1(
    _Inout_ PULONG Value2,
    _In_ ULONG Value1,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    );

PHLIBAPI
VOID
NTAPI
PhMapFlags2(
    _Inout_ PULONG Value1,
    _In_ ULONG Value2,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    );

PHLIBAPI
PVOID
NTAPI
PhCreateOpenFileDialog(
    VOID
    );

PHLIBAPI
PVOID
NTAPI
PhCreateSaveFileDialog(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhFreeFileDialog(
    _In_ PVOID FileDialog
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowFileDialog(
    _In_opt_ HWND WindowHandle,
    _In_ PVOID FileDialog
    );

#define PH_FILEDIALOG_CREATEPROMPT 0x1
#define PH_FILEDIALOG_PATHMUSTEXIST 0x2 // default both
#define PH_FILEDIALOG_FILEMUSTEXIST 0x4 // default open
#define PH_FILEDIALOG_SHOWHIDDEN 0x8
#define PH_FILEDIALOG_NODEREFERENCELINKS 0x10
#define PH_FILEDIALOG_OVERWRITEPROMPT 0x20 // default save
#define PH_FILEDIALOG_DEFAULTEXPANDED 0x40
#define PH_FILEDIALOG_STRICTFILETYPES 0x80
#define PH_FILEDIALOG_PICKFOLDERS 0x100
#define PH_FILEDIALOG_NOPATHVALIDATE 0x200
#define PH_FILEDIALOG_DONTADDTORECENT 0x400

PHLIBAPI
ULONG
NTAPI
PhGetFileDialogOptions(
    _In_ PVOID FileDialog
    );

PHLIBAPI
VOID
NTAPI
PhSetFileDialogOptions(
    _In_ PVOID FileDialog,
    _In_ ULONG Options
    );

PHLIBAPI
ULONG
NTAPI
PhGetFileDialogFilterIndex(
    _In_ PVOID FileDialog
    );

typedef struct _PH_FILETYPE_FILTER
{
    PWSTR Name;
    PWSTR Filter;
} PH_FILETYPE_FILTER, *PPH_FILETYPE_FILTER;

PHLIBAPI
VOID
NTAPI
PhSetFileDialogFilter(
    _In_ PVOID FileDialog,
    _In_ PPH_FILETYPE_FILTER Filters,
    _In_ ULONG NumberOfFilters
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileDialogFileName(
    _In_ PVOID FileDialog
    );

PHLIBAPI
VOID
NTAPI
PhSetFileDialogFileName(
    _In_ PVOID FileDialog,
    _In_ PCWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIsExecutablePacked(
    _In_ PPH_STRING FileName,
    _Out_ PBOOLEAN IsPacked,
    _Out_opt_ PULONG NumberOfModules,
    _Out_opt_ PULONG NumberOfFunctions
    );

/**
* Image Coherency Scan Type
*/
typedef enum _PH_IMGCOHERENCY_SCAN_TYPE
{
    /**
    * Quick scan of the image coherency
    * - Image header information
    * - A few pages of each executable section
    * - Scans a few pages at entry point if it exists and was missed due to previous note
    * - .NET manifests if appropriate
    */
    PhImageCoherencyQuick,

    /**
    * Normal scan of the image coherency
    * - Image header information
    * - Up to 40Mib of each executable section
    * - Scans a few pages at entry point if it exists and was missed due to previous note
    * - .NET manifests if appropriate
    */
    PhImageCoherencyNormal,

    /**
    * Full scan of the image coherency
    * - Image header information
    * - Complete scan of all executable sections, this will include the entry point
    * - .NET manifests if appropriate
    * - Scans for code caves in tail of mapped sections (virtual mapping > size on disk)
    */
    PhImageCoherencyFull,

    /**
     * Fast scan of the image coherency
     * - only checks for shared original pages using NtQueryVirtualMemory
     */
    PhImageCoherencySharedOriginal,

} PH_IMAGE_COHERENCY_SCAN_TYPE;

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageCoherency(
    _In_ PPH_STRING FileName,
    _In_ HANDLE ProcessId,
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _Out_ PFLOAT ImageCoherency
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessModuleImageCoherency(
    _In_ PPH_STRING FileName,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ImageBaseAddress,
    _In_ SIZE_T ImageSize,
    _In_ BOOLEAN IsKernelModule,
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _Out_ PFLOAT ImageCoherency
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCheckImagePagesForTampering(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T SizeOfImage,
    _Out_ PSIZE_T NumberOfPages,
    _Out_ PSIZE_T NumberOfTamperedPages
    );

PHLIBAPI
ULONG
NTAPI
PhCrc32(
    _In_ ULONG Crc,
    _In_reads_(Length) PCHAR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
ULONG
NTAPI
PhCrc32C(
    _In_ ULONG Crc,
    _In_reads_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    );

typedef enum _PH_HASH_ALGORITHM
{
    Md5HashAlgorithm,
    Sha1HashAlgorithm,
    Crc32HashAlgorithm,
    Sha256HashAlgorithm
} PH_HASH_ALGORITHM;

typedef struct _PH_HASH_CONTEXT
{
    PH_HASH_ALGORITHM Algorithm;
    ULONG Context[64];
} PH_HASH_CONTEXT, *PPH_HASH_CONTEXT;

PHLIBAPI
VOID
NTAPI
PhInitializeHash(
    _Out_ PPH_HASH_CONTEXT Context,
    _In_ PH_HASH_ALGORITHM Algorithm
    );

PHLIBAPI
VOID
NTAPI
PhUpdateHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
BOOLEAN
NTAPI
PhFinalHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _Out_writes_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_opt_ PULONG ReturnLength
    );

typedef enum _PH_COMMAND_LINE_OPTION_TYPE
{
    NoArgumentType,
    MandatoryArgumentType,
    OptionalArgumentType
} PH_COMMAND_LINE_OPTION_TYPE, *PPH_COMMAND_LINE_OPTION_TYPE;

typedef struct _PH_COMMAND_LINE_OPTION
{
    ULONG Id;
    PCWSTR Name;
    PH_COMMAND_LINE_OPTION_TYPE Type;
} PH_COMMAND_LINE_OPTION, *PPH_COMMAND_LINE_OPTION;

typedef BOOLEAN (NTAPI *PPH_COMMAND_LINE_CALLBACK)(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    );

#define PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS 0x1
#define PH_COMMAND_LINE_IGNORE_FIRST_PART 0x2

PHLIBAPI
PPH_STRING
NTAPI
PhParseCommandLinePart(
    _In_ PPH_STRINGREF CommandLine,
    _Inout_ PULONG_PTR Index
    );

PHLIBAPI
BOOLEAN
NTAPI
PhParseCommandLine(
    _In_ PPH_STRINGREF CommandLine,
    _In_opt_ PPH_COMMAND_LINE_OPTION Options,
    _In_ ULONG NumberOfOptions,
    _In_ ULONG Flags,
    _In_ PPH_COMMAND_LINE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEscapeCommandLinePart(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
BOOLEAN
NTAPI
PhParseCommandLineFuzzy(
    _In_ PPH_STRINGREF CommandLine,
    _Out_ PPH_STRINGREF FileName,
    _Out_ PPH_STRINGREF Arguments,
    _Out_opt_ PPH_STRING *FullFileName
    );

PHLIBAPI
PPH_LIST
NTAPI
PhCommandLineToList(
    _In_ PCWSTR CommandLine
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCommandLineQuoteSpaces(
    _In_ PPH_STRINGREF CommandLine
    );

PHLIBAPI
PPH_STRING
NTAPI
PhSearchFilePath(
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Extension
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCreateCacheFile(
    _In_ BOOLEAN PortableDirectory,
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName
    );

PHLIBAPI
VOID
NTAPI
PhDeleteCacheFile(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName
    );

PHLIBAPI
VOID
NTAPI
PhClearCacheDirectory(
    _In_ BOOLEAN PortableDirectory
    );

PHLIBAPI
HANDLE
NTAPI
PhGetNamespaceHandle(
    VOID
    );

PHLIBAPI
HWND
NTAPI
PhHungWindowFromGhostWindow(
    _In_ HWND WindowHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileData(
    _In_ HANDLE FileHandle,
    _Out_ PVOID* Buffer,
    _Out_ PULONG BufferLength
    );

PHLIBAPI
PVOID
NTAPI
PhGetFileText(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Unicode
    );

PHLIBAPI
PVOID
NTAPI
PhFileReadAllText(
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN Unicode
    );

PHLIBAPI
PVOID
NTAPI
PhFileReadAllTextWin32(
    _In_ PCWSTR FileName,
    _In_ BOOLEAN Unicode
    );

PHLIBAPI
HRESULT
NTAPI
PhGetClassObjectDllBase(
    _In_ PVOID DllBase,
    _In_ REFCLSID Rclsid,
    _In_ REFIID Riid,
    _Out_ PVOID * Ppv
    );

PHLIBAPI
HRESULT
NTAPI
PhGetClassObject(
    _In_ PCWSTR DllName,
    _In_ REFCLSID Rclsid,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    );

PHLIBAPI
HRESULT
NTAPI
PhGetActivationFactory(
    _In_ PCWSTR DllName,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    );

PHLIBAPI
HRESULT
NTAPI
PhActivateInstance(
    _In_ PCWSTR DllName,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDelayExecution(
    _In_ ULONG Milliseconds
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDelayExecutionEx(
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER DelayInterval
    );

// Stopwatch

typedef struct _PH_STOPWATCH
{
    LARGE_INTEGER StartCounter;
    LARGE_INTEGER EndCounter;
    LARGE_INTEGER Frequency;
} PH_STOPWATCH, *PPH_STOPWATCH;

FORCEINLINE
VOID
PhInitializeStopwatch(
    _Out_ PPH_STOPWATCH Stopwatch
    )
{
    Stopwatch->StartCounter.QuadPart = 0;
    Stopwatch->EndCounter.QuadPart = 0;
}

FORCEINLINE
VOID
PhStartStopwatch(
    _Inout_ PPH_STOPWATCH Stopwatch
    )
{
    PhQueryPerformanceCounter(&Stopwatch->StartCounter);
    PhQueryPerformanceFrequency(&Stopwatch->Frequency);
}

FORCEINLINE
VOID
PhStopStopwatch(
    _Inout_ PPH_STOPWATCH Stopwatch
    )
{
    PhQueryPerformanceCounter(&Stopwatch->EndCounter);
}

FORCEINLINE
ULONG
PhGetMillisecondsStopwatch(
    _In_ PPH_STOPWATCH Stopwatch
    )
{
    LARGE_INTEGER elapsedMilliseconds;

    elapsedMilliseconds.QuadPart = Stopwatch->EndCounter.QuadPart - Stopwatch->StartCounter.QuadPart;
    elapsedMilliseconds.QuadPart *= 1000;
    elapsedMilliseconds.QuadPart /= Stopwatch->Frequency.QuadPart;

    return (ULONG)elapsedMilliseconds.QuadPart;
}

FORCEINLINE
ULONGLONG
PhGetMicrosecondsStopwatch(
    _In_ PPH_STOPWATCH Stopwatch
    )
{
    LARGE_INTEGER elapsedMicroseconds;

    // Convert to microseconds before dividing by ticks-per-second.
    elapsedMicroseconds.QuadPart = Stopwatch->EndCounter.QuadPart - Stopwatch->StartCounter.QuadPart;
    elapsedMicroseconds.QuadPart *= 1000000;
    elapsedMicroseconds.QuadPart /= Stopwatch->Frequency.QuadPart;

    return elapsedMicroseconds.QuadPart;
}

PHLIBAPI
PPH_STRING
NTAPI
PhApiSetResolveToHost(
    _In_ PPH_STRINGREF ApiSetName
    );

PHLIBAPI
HRESULT
NTAPI
PhCreateProcessAsInteractiveUser(
    _In_ PCWSTR CommandLine,
    _In_ PCWSTR CurrentDirectory
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessClone(
    _Out_ PHANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessReflection(
    _Out_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
VOID
NTAPI
PhFreeProcessReflection(
    _In_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessSnapshot(
    _Out_ PHANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
VOID
NTAPI
PhFreeProcessSnapshot(
    _In_ HANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessRedirection(
    _In_ PPH_STRING CommandLine,
    _In_opt_ PPH_STRINGREF CommandInput,
    _Out_opt_ PPH_STRING* CommandOutput
    );

PHLIBAPI
NTSTATUS
NTAPI
PhInitializeProcThreadAttributeList(
    _Out_ PPROC_THREAD_ATTRIBUTE_LIST* AttributeList,
    _In_ ULONG AttributeCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteProcThreadAttributeList(
    _In_ PPROC_THREAD_ATTRIBUTE_LIST AttributeList
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUpdateProcThreadAttribute(
    _In_ PPROC_THREAD_ATTRIBUTE_LIST AttributeList,
    _In_ ULONG_PTR Attribute,
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetActiveComputerName(
    VOID
    );

#include <devpropdef.h>
#include <devquery.h>

typedef enum _DEV_OBJECT_TYPE DEV_OBJECT_TYPE, *PDEV_OBJECT_TYPE;
typedef enum _DEV_QUERY_FLAGS DEV_QUERY_FLAGS, *PDEV_QUERY_FLAGS;
typedef struct _DEVPROPCOMPKEY DEVPROPCOMPKEY, *PDEVPROPCOMPKEY;
typedef struct _DEVPROP_FILTER_EXPRESSION DEVPROP_FILTER_EXPRESSION, *PDEVPROP_FILTER_EXPRESSION;
typedef struct _DEV_OBJECT DEV_OBJECT, *PDEV_OBJECT;

PHLIBAPI
_Check_return_
HRESULT
NTAPI
PhDevGetObjects(
    _In_ DEV_OBJECT_TYPE ObjectType,
    _In_ DEV_QUERY_FLAGS QueryFlags,
    _In_ ULONG RequestedPropertiesCount,
    _In_reads_opt_(RequestedPropertiesCount) const DEVPROPCOMPKEY* RequestedProperties,
    _In_ ULONG FilterExpressionCount,
    _In_reads_opt_(FilterExpressionCount) const DEVPROP_FILTER_EXPRESSION* FilterExpressions,
    _Out_ PULONG ObjectCount,
    _Outptr_result_buffer_maybenull_(*ObjectCount) const DEV_OBJECT** Objects
    );

PHLIBAPI
VOID
NTAPI
PhDevFreeObjects(
    _In_ ULONG ObjectCount,
    _In_reads_(ObjectCount) const DEV_OBJECT* Objects
    );

typedef GUID  DEVPROPGUID, *PDEVPROPGUID;
typedef ULONG DEVPROPID,   *PDEVPROPID;
typedef struct _DEVPROPKEY DEVPROPKEY, *PDEVPROPKEY;
typedef enum _DEVPROPSTORE DEVPROPSTORE, *PDEVPROPSTORE;
typedef ULONG DEVPROPTYPE, *PDEVPROPTYPE;
typedef struct _DEVPROPCOMPKEY DEVPROPCOMPKEY, *PDEVPROPCOMPKEY;

_Check_return_
PHLIBAPI
HRESULT
NTAPI
PhDevGetObjectProperties(
    _In_ DEV_OBJECT_TYPE ObjectType,
    _In_ PCWSTR ObjectId,
    _In_ DEV_QUERY_FLAGS QueryFlags,
    _In_ ULONG RequestedPropertiesCount,
    _In_reads_(RequestedPropertiesCount) const DEVPROPCOMPKEY* RequestedProperties,
    _Out_ PULONG PropertiesCount,
    _Outptr_result_buffer_(*PropertiesCount) const DEVPROPERTY** Properties
    );

PHLIBAPI
VOID
NTAPI
PhDevFreeObjectProperties(
    _In_ ULONG PropertiesCount,
    _In_reads_(PropertiesCount) const DEVPROPERTY* Properties
    );

_Check_return_
PHLIBAPI
HRESULT
NTAPI
PhDevCreateObjectQuery(
    _In_ DEV_OBJECT_TYPE ObjectType,
    _In_ DEV_QUERY_FLAGS QueryFlags,
    _In_ ULONG RequestedPropertiesCount,
    _In_reads_opt_(RequestedPropertiesCount) const DEVPROPCOMPKEY* RequestedProperties,
    _In_ ULONG FilterExpressionCount,
    _In_reads_opt_(FilterExpressionCount) const DEVPROP_FILTER_EXPRESSION* Filter,
    _In_ PDEV_QUERY_RESULT_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_ PHDEVQUERY DevQuery
    );

PHLIBAPI
VOID
NTAPI
PhDevCloseObjectQuery(
    _In_ HDEVQUERY QueryHandle
    );

PHLIBAPI
HRESULT
NTAPI
PhTaskbarListCreate(
    _Out_ PHANDLE TaskbarHandle
    );

PHLIBAPI
VOID
NTAPI
PhTaskbarListDestroy(
    _In_ HANDLE TaskbarHandle
    );

PHLIBAPI
VOID
NTAPI
PhTaskbarListSetProgressValue(
    _In_ HANDLE TaskbarHandle,
    _In_ HWND WindowHandle,
    _In_ ULONGLONG Completed,
    _In_ ULONGLONG Total
    );

#define PH_TBLF_NOPROGRESS 0x1
#define PH_TBLF_INDETERMINATE 0x2
#define PH_TBLF_NORMAL 0x4
#define PH_TBLF_ERROR 0x8
#define PH_TBLF_PAUSED 0x10

PHLIBAPI
VOID
NTAPI
PhTaskbarListSetProgressState(
    _In_ HANDLE TaskbarHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags
    );

PHLIBAPI
VOID
NTAPI
PhTaskbarListSetOverlayIcon(
    _In_ HANDLE TaskbarHandle,
    _In_ HWND WindowHandle,
    _In_opt_ HICON IconHandle,
    _In_opt_ PCWSTR IconDescription
    );

FORCEINLINE
BOOLEAN
PhPtrAddOffset(
    _Inout_ PVOID* Pointer,
    _In_ SIZE_T Offset
    )
{
    PVOID pointer;

    pointer = PTR_ADD_OFFSET(*Pointer, Offset);
    if (pointer < *Pointer)
        return FALSE;

    *Pointer = pointer;
    return TRUE;
}

FORCEINLINE
BOOLEAN
PhPtrAdvance(
    _Inout_ PVOID* Pointer,
    _In_ PVOID EndPointer,
    _In_ SIZE_T Offset
    )
{
    PVOID pointer;

    pointer = *Pointer;
    if (!PhPtrAddOffset(&pointer, Offset))
        return FALSE;

    if (pointer >= EndPointer)
        return FALSE;

    *Pointer = pointer;
    return TRUE;
}

typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;
typedef enum _D3DKMT_VIDPNSOURCEOWNER_TYPE D3DKMT_VIDPNSOURCEOWNER_TYPE;
typedef struct _D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP, *PD3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP;

BOOLEAN PhIsDirectXRunningFullScreen(
    VOID
    );

NTSTATUS PhRestoreFromDirectXRunningFullScreen(
    _In_ HANDLE ProcessHandle
    );

NTSTATUS PhQueryDirectXExclusiveOwnership(
    _Inout_ PD3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP QueryExclusiveOwnership
    );

EXTERN_C_END

#endif

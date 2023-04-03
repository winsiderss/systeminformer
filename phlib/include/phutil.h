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

extern WCHAR *PhSizeUnitNames[7];
extern ULONG PhMaxSizeUnit;
extern USHORT PhMaxPrecisionUnit;

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
    ULONG Scale;
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
    _In_opt_ HWND hWnd,
    _Inout_ PPH_RECTANGLE Rectangle
    );

PHLIBAPI
VOID
NTAPI
PhCenterWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentWindowHandle
    );

PHLIBAPI
LONG
NTAPI
PhGetDpi(
    _In_ LONG Number,
    _In_ LONG DpiValue
    );

PHLIBAPI
LONG
NTAPI
PhGetMonitorDpi(
    _In_ LPCRECT rect
    );

PHLIBAPI
LONG
NTAPI
PhGetSystemDpi(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetTaskbarDpi(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetWindowDpi(
    _In_ HWND WindowHandle
    );

PHLIBAPI
LONG
NTAPI
PhGetDpiValue(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LPCRECT Rect
    );

PHLIBAPI
LONG
NTAPI
PhGetSystemMetrics(
    _In_ INT Index,
    _In_opt_ LONG DpiValue
    );

PHLIBAPI
BOOL
NTAPI
PhGetSystemParametersInfo(
    _In_ INT Action,
    _In_ UINT Param1,
    _Pre_maybenull_ _Post_valid_ PVOID Param2,
    _In_opt_ LONG DpiValue
    );

PHLIBAPI
VOID
NTAPI
PhGetSizeDpiValue(
    _Inout_ PRECT rect,
    _In_ LONG DpiValue,
    _In_ BOOLEAN isUnpack
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
INT
NTAPI
PhShowMessage(
    _In_opt_ HWND hWnd,
    _In_ ULONG Type,
    _In_ PWSTR Format,
    ...
    );

#define PhShowError(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONERROR, Format, __VA_ARGS__)
#define PhShowWarning(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONWARNING, Format, __VA_ARGS__)
#define PhShowInformation(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONINFORMATION, Format, __VA_ARGS__)

PHLIBAPI
INT
NTAPI
PhShowMessage2(
    _In_opt_ HWND hWnd,
    _In_ ULONG Buttons,
    _In_opt_ PWSTR Icon,
    _In_opt_ PWSTR Title,
    _In_ PWSTR Format,
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

#define PhShowError2(hWnd, Title, Format, ...) PhShowMessage2(hWnd, TD_CLOSE_BUTTON, TD_ERROR_ICON, Title, Format, __VA_ARGS__)
#define PhShowWarning2(hWnd, Title, Format, ...) PhShowMessage2(hWnd, TD_CLOSE_BUTTON, TD_WARNING_ICON, Title, Format, __VA_ARGS__)
#define PhShowInformation2(hWnd, Title, Format, ...) PhShowMessage2(hWnd, TD_CLOSE_BUTTON, TD_INFORMATION_ICON, Title, Format, __VA_ARGS__)

PHLIBAPI
BOOLEAN
NTAPI
PhShowMessageOneTime(
    _In_opt_ HWND hWnd,
    _In_ ULONG Buttons,
    _In_opt_ PWSTR Icon,
    _In_opt_ PWSTR Title,
    _In_ PWSTR Format,
    ...
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
    _In_opt_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowContinueStatus(
    _In_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowConfirmMessage(
    _In_ HWND hWnd,
    _In_ PWSTR Verb,
    _In_ PWSTR Object,
    _In_opt_ PWSTR Message,
    _In_ BOOLEAN Warning
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhFindIntegerSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PWSTR String,
    _Out_ PULONG Integer
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhFindStringSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ ULONG Integer,
    _Out_ PWSTR *String
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhFindIntegerSiKeyValuePairsStringRef(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PPH_STRINGREF String,
    _Out_ PULONG Integer
    );

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
PhGenerateRandomAlphaString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ ULONG Count
    );

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
    _In_ PWSTR Pattern,
    _In_ PWSTR String,
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
    _In_ PWSTR A,
    _In_ PWSTR B,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN MatchIfPrefix
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDate(
    _In_opt_ PSYSTEMTIME Date,
    _In_opt_ PWSTR Format
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatTime(
    _In_opt_ PSYSTEMTIME Time,
    _In_opt_ PWSTR Format
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDateTime(
    _In_opt_ PSYSTEMTIME DateTime
    );

#define PhaFormatDateTime(DateTime) PH_AUTO_T(PH_STRING, PhFormatDateTime(DateTime))

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
    _In_ PWSTR Value,
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
    _In_ PWSTR FileName
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
    _In_ PWSTR Key,
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
    _In_ PWSTR SubBlock
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
    _In_ PWSTR FileName
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
_Success_(return != NULL)
PPH_STRING
NTAPI
PhGetFullPath(
    _In_ PWSTR FileName,
    _Out_opt_ PULONG IndexOfFileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFullPathEx(
    _In_ PWSTR FileName,
    _Out_opt_ PULONG IndexOfFileName,
    _Out_ PPH_STRING *FullPath
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
    _In_ PWSTR String
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
VOID
NTAPI
PhGetSystemRoot(
    _Out_ PPH_STRINGREF SystemRoot
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
    _In_ PWSTR FileName,
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
PhGetRoamingAppDataDirectory(
    _In_ PPH_STRINGREF FileName
    );

FORCEINLINE
PPH_STRING
PhGetRoamingAppDataDirectoryZ(
    _In_ PWSTR String
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);

    return PhGetRoamingAppDataDirectory(&string);
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

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownLocation(
    _In_ ULONG Folder,
    _In_opt_ PPH_STRINGREF AppendPath
    );

FORCEINLINE
PPH_STRING
PhGetKnownLocationZ(
    _In_ ULONG Folder,
    _In_ PWSTR AppendPath
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, AppendPath);

    return PhGetKnownLocation(Folder, &string);
}

DECLSPEC_SELECTANY GUID FOLDERID_LocalAppData = { 0xF1B32785, 0x6FBA, 0x4FCF, 0x9D, 0x55, 0x7B, 0x8E, 0x7F, 0x15, 0x70, 0x91 };
DECLSPEC_SELECTANY GUID FOLDERID_RoamingAppData = { 0x3EB685DB, 0x65F9, 0x4CF6, 0xA0, 0x3A, 0xE3, 0xEF, 0x65, 0x72, 0x9F, 0x3D };
DECLSPEC_SELECTANY GUID FOLDERID_ProgramFiles = { 0x905e63b6, 0xc1bf, 0x494e, 0xb2, 0x9c, 0x65, 0xb7, 0x32, 0xd3, 0xd2, 0x1a };

#define PH_KF_FLAG_FORCE_PACKAGE_REDIRECTION 0x1
#define PH_KF_FLAG_FORCE_APPCONTAINER_REDIRECTION 0x2

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownFolderPath(
    _In_ PGUID Folder,
    _In_opt_ PPH_STRINGREF AppendPath
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownFolderPathEx(
    _In_ PGUID Folder,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PPH_STRINGREF AppendPath
    );

FORCEINLINE
PPH_STRING
PhGetKnownFolderPathZ(
    _In_ PGUID Folder,
    _In_ PWSTR AppendPath
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, AppendPath);

    return PhGetKnownFolderPath(Folder, &string);
}

FORCEINLINE
PPH_STRING
PhGetKnownFolderPathExZ(
    _In_ PGUID Folder,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_ PWSTR AppendPath
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
    _In_opt_ HWND hWnd,
    _In_ ULONG NumberOfHandles,
    _In_ PHANDLE Handles,
    _In_ ULONG Timeout
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
    _In_ PWSTR FileName,
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
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PWSTR CurrentDirectory,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessWin32Ex(
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PWSTR CurrentDirectory,
    _In_opt_ STARTUPINFO *StartupInfo,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

typedef struct _PH_CREATE_PROCESS_AS_USER_INFO
{
    _In_opt_ PWSTR ApplicationName;
    _In_opt_ PWSTR CommandLine;
    _In_opt_ PWSTR CurrentDirectory;
    _In_opt_ PVOID Environment;
    _In_opt_ PWSTR DesktopName;
    _In_opt_ ULONG SessionId; // use PH_CREATE_PROCESS_SET_SESSION_ID
    union
    {
        struct
        {
            _In_ PWSTR DomainName;
            _In_ PWSTR UserName;
            _In_ PWSTR Password;
            _In_opt_ ULONG LogonType;
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
    _In_ PWSTR SecurityDescriptorString
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetObjectSecurityDescriptorAsString(
    _In_ HANDLE Handle,
    _Out_ PPH_STRING* SecurityDescriptorString
    );

PHLIBAPI
VOID
NTAPI
PhShellExecute(
    _In_opt_ HWND hWnd,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters
    );

#define PH_SHELL_EXECUTE_DEFAULT 0x0
#define PH_SHELL_EXECUTE_ADMIN 0x1
#define PH_SHELL_EXECUTE_PUMP_MESSAGES 0x2

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhShellExecuteEx(
    _In_opt_ HWND hWnd,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_opt_ PWSTR Directory,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    );

PHLIBAPI
VOID
NTAPI
PhShellExploreFile(
    _In_ HWND hWnd,
    _In_ PWSTR FileName
    );

PHLIBAPI
VOID
NTAPI
PhShellProperties(
    _In_ HWND hWnd,
    _In_ PWSTR FileName
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
    _In_ PWSTR ValueName
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
    _In_ PWSTR ValueName
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
    _In_ PWSTR ValueName
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
    _In_opt_ HWND hWnd,
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
    _In_ PWSTR FileName
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
    PhImageCoherencyFull

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
ULONG
NTAPI
PhCrc32(
    _In_ ULONG Crc,
    _In_reads_(Length) PCHAR Buffer,
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
    PWSTR Name;
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
PhSearchFilePath(
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Extension
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCreateCacheFile(
    _In_ PPH_STRING FileName
    );

PHLIBAPI
VOID
NTAPI
PhDeleteCacheFile(
    _In_ PPH_STRING FileName
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
    _In_ PWSTR FileName,
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
ULONGLONG
NTAPI
PhReadTimeStampCounter(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter
    );

PHLIBAPI
BOOLEAN
NTAPI
PhQueryPerformanceFrequency(
    _Out_ PLARGE_INTEGER PerformanceFrequency
    );

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
    _In_ PWSTR CommandLine,
    _In_ PWSTR CurrentDirectory
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessReflection(
    _Out_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ProcessId
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
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ProcessId
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

EXTERN_C_END

#endif

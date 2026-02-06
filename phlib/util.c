/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>

#include <commdlg.h>
#include <d3dkmthk.h>
#include <ntintsafe.h>
#include <processsnapshot.h>
#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <winsta.h>

#include <apiimport.h>
#include <appresolver.h>
#include <guisup.h>
#include <mapimg.h>
#include <mapldr.h>
#include <lsasup.h>
#include <phintrin.h>
#include <wslsup.h>
#include <thirdparty.h>

#if defined(PH_BUILD_MSIX)
#include <roapi.h>
#include <winstring.h>
#endif

DECLSPEC_SELECTANY CONST WCHAR *PhSizeUnitNames[7] = { L"B", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB" };
DECLSPEC_SELECTANY ULONG PhMaxSizeUnit = ULONG_MAX;
DECLSPEC_SELECTANY USHORT PhMaxPrecisionUnit = 2;
DECLSPEC_SELECTANY FLOAT PhMaxPrecisionLimit = 0.01f;

/**
 * Ensures a rectangle is positioned within the specified bounds.
 *
 * \param Rectangle The rectangle to be adjusted.
 * \param Bounds The bounds.
 *
 * \remarks If the rectangle is too large to fit inside the bounds, it is positioned at the top-left
 * of the bounds.
 */
VOID PhAdjustRectangleToBounds(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    )
{
    if (Rectangle->Left + Rectangle->Width > Bounds->Left + Bounds->Width)
        Rectangle->Left = Bounds->Left + Bounds->Width - Rectangle->Width;
    if (Rectangle->Top + Rectangle->Height > Bounds->Top + Bounds->Height)
        Rectangle->Top = Bounds->Top + Bounds->Height - Rectangle->Height;

    if (Rectangle->Left < Bounds->Left)
        Rectangle->Left = Bounds->Left;
    if (Rectangle->Top < Bounds->Top)
        Rectangle->Top = Bounds->Top;

    if (Rectangle->Width > Bounds->Width)
        Rectangle->Width = Bounds->Width;
    if (Rectangle->Height > Bounds->Height)
        Rectangle->Height = Bounds->Height;
}

/**
 * Positions a rectangle in the center of the specified bounds.
 *
 * \param Rectangle The rectangle to be adjusted.
 * \param Bounds The bounds.
 */
VOID PhCenterRectangle(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    )
{
    Rectangle->Left = Bounds->Left + (Bounds->Width - Rectangle->Width) / 2;
    Rectangle->Top = Bounds->Top + (Bounds->Height - Rectangle->Height) / 2;
}

/**
 * Ensures a rectangle is positioned within the working area of the specified window's monitor.
 *
 * \param WindowHandle A handle to a window. If NULL, the monitor closest to \a Rectangle is used.
 * \param Rectangle The rectangle to be adjusted.
 */
VOID PhAdjustRectangleToWorkingArea(
    _In_opt_ HWND WindowHandle,
    _Inout_ PPH_RECTANGLE Rectangle
    )
{
    HMONITOR monitor;
    MONITORINFO monitorInfo;

    if (WindowHandle)
    {
        monitor = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);
    }
    else
    {
        RECT rect = { 0 };

        PhRectangleToRect(&rect, Rectangle);
        monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
    }

    RtlZeroMemory(&monitorInfo, sizeof(MONITORINFO));
    monitorInfo.cbSize = sizeof(MONITORINFO);

    if (GetMonitorInfo(monitor, &monitorInfo))
    {
        PH_RECTANGLE bounds = { 0 };

        PhRectToRectangle(&bounds, &monitorInfo.rcWork);
        PhAdjustRectangleToBounds(Rectangle, &bounds);
    }
}

/**
 * Centers a window.
 *
 * \param WindowHandle The window to center.
 * \param ParentWindowHandle If specified, the window will be positioned at the center of this
 * window. Otherwise, the window will be positioned at the center of the monitor.
 */
VOID PhCenterWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentWindowHandle
    )
{
    if (ParentWindowHandle)
    {
        RECT rect, parentRect;
        PH_RECTANGLE rectangle = { 0 };
        PH_RECTANGLE parentRectangle = { 0 };

        if (!IsWindowVisible(ParentWindowHandle) || IsMinimized(ParentWindowHandle))
            return;
        if (!PhGetWindowRect(WindowHandle, &rect))
            return;
        if (!PhGetWindowRect(ParentWindowHandle, &parentRect))
            return;

        PhRectToRectangle(&rectangle, &rect);
        PhRectToRectangle(&parentRectangle, &parentRect);
        PhCenterRectangle(&rectangle, &parentRectangle);
        PhAdjustRectangleToWorkingArea(WindowHandle, &rectangle);

        MoveWindow(WindowHandle, rectangle.Left, rectangle.Top,
            rectangle.Width, rectangle.Height, FALSE);
    }
    else
    {
        MONITORINFO monitorInfo;

        RtlZeroMemory(&monitorInfo, sizeof(MONITORINFO));
        monitorInfo.cbSize = sizeof(MONITORINFO);

        if (GetMonitorInfo(
            MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST),
            &monitorInfo
            ))
        {
            RECT rect;
            PH_RECTANGLE rectangle = { 0 };;
            PH_RECTANGLE bounds = { 0 };;

            if (!PhGetWindowRect(WindowHandle, &rect))
                return;

            PhRectToRectangle(&rectangle, &rect);
            PhRectToRectangle(&bounds, &monitorInfo.rcWork);
            PhCenterRectangle(&rectangle, &bounds);

            MoveWindow(WindowHandle, rectangle.Left, rectangle.Top,
                rectangle.Width, rectangle.Height, FALSE);
        }
    }
}

NTSTATUS PhMoveWindowToMonitor(
    _In_ HWND WindowHandle,
    _In_ HMONITOR MonitorHandle
    )
{
    MONITORINFO currentMonitorInfo;
    MONITORINFO targetMonitorInfo;
    PH_RECTANGLE rectangle = { 0 };
    PH_RECTANGLE bounds = { 0 };
    HMONITOR monitor;
    RECT windowRect;
    RECT targetRect;

    // Get window position

    if (!PhGetWindowRect(WindowHandle, &windowRect))
        return PhGetLastWin32ErrorAsNtStatus();

    // Get current monitor

    memset(&currentMonitorInfo, 0, sizeof(MONITORINFO));
    currentMonitorInfo.cbSize = sizeof(currentMonitorInfo);

    monitor = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);

    if (!GetMonitorInfo(monitor, &currentMonitorInfo))
        return PhGetLastWin32ErrorAsNtStatus();

    // Get target monitor

    memset(&targetMonitorInfo, 0, sizeof(MONITORINFO));
    targetMonitorInfo.cbSize = sizeof(targetMonitorInfo);

    if (!GetMonitorInfo(MonitorHandle, &targetMonitorInfo))
        return PhGetLastWin32ErrorAsNtStatus();

    // Calculate relative position

    RECT currentMonitorRect = currentMonitorInfo.rcMonitor;
    LONG left = windowRect.left - currentMonitorRect.left;
    LONG top = windowRect.top - currentMonitorRect.top;
    LONG width = windowRect.right - windowRect.left;
    LONG height = windowRect.bottom - windowRect.top;

    targetRect.left = targetMonitorInfo.rcMonitor.left + left;
    targetRect.top = targetMonitorInfo.rcMonitor.top + top;
    targetRect.right = targetRect.left + width;
    targetRect.bottom = targetRect.top + height;

    // Adjust relative position

    PhRectToRectangle(&rectangle, &targetRect);
    PhRectToRectangle(&bounds, &targetMonitorInfo.rcWork);
    PhAdjustRectangleToBounds(&rectangle, &bounds);

    MoveWindow(WindowHandle, rectangle.Left, rectangle.Top,
        rectangle.Width, rectangle.Height, TRUE);

    return STATUS_SUCCESS;
}

// rev from GetSystemDefaultLCID
LCID PhGetSystemDefaultLCID(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    return GetSystemDefaultLCID();
#else
    LCID localeId = LOCALE_SYSTEM_DEFAULT;

    if (NT_SUCCESS(NtQueryDefaultLocale(FALSE, &localeId)))
        return localeId;

    return MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
#endif
}

// rev from GetUserDefaultLCID
LCID PhGetUserDefaultLCID(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    return GetUserDefaultLCID();
#else
    LCID localeId = LOCALE_USER_DEFAULT;

    if (NT_SUCCESS(NtQueryDefaultLocale(TRUE, &localeId)))
        return localeId;

    return MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
#endif
}

BOOLEAN PhGetUserLocaleInfoBool(
    _In_ LCTYPE LCType
    )
{
    WCHAR value[4];

    if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LCType, value, 4))
    {
        if (value[0] == L'1')
            return TRUE;
    }

    return FALSE;
}

// rev from GetThreadLocale
LCID PhGetCurrentThreadLCID(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    return GetThreadLocale();
#else
    PTEB currentTeb;

    currentTeb = NtCurrentTeb();

    if (!currentTeb->CurrentLocale)
        currentTeb->CurrentLocale = PhGetUserDefaultLCID();
    if (currentTeb->CurrentLocale == LOCALE_CUSTOM_DEFAULT)
        currentTeb->CurrentLocale = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    if (currentTeb->CurrentLocale == LOCALE_CUSTOM_UNSPECIFIED)
        currentTeb->CurrentLocale = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

    return currentTeb->CurrentLocale;
#endif
}

// rev from GetSystemDefaultLangID
LANGID PhGetSystemDefaultLangID(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    return GetSystemDefaultLangID();
#else
    return LANGIDFROMLCID(PhGetSystemDefaultLCID());
#endif
}

// rev from GetUserDefaultLangID
LANGID PhGetUserDefaultLangID(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    return GetUserDefaultLangID();
#else
    return LANGIDFROMLCID(PhGetUserDefaultLCID());
#endif
}

// rev from GetUserDefaultUILanguage
LANGID PhGetUserDefaultUILanguage(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    return GetUserDefaultUILanguage();
#else
    LANGID languageId;

    if (NT_SUCCESS(NtQueryDefaultUILanguage(&languageId)))
        return languageId;
    if (NT_SUCCESS(NtQueryInstallUILanguage(&languageId)))
        return languageId;

    return MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
#endif
}

// rev from GetUserDefaultLocaleName
PPH_STRING PhGetUserDefaultLocaleName(
    VOID
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    ULONG localNameLength;
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { UNICODE_NULL };

    localNameLength = GetUserDefaultLocaleName(localeName, RTL_NUMBER_OF(localeName));

    if (localNameLength > sizeof(UNICODE_NULL))
    {
        return PhCreateStringEx(localeName, localNameLength - sizeof(UNICODE_NULL));
    }
#else
    UNICODE_STRING localeNameUs;
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { UNICODE_NULL };

    RtlInitEmptyUnicodeString(&localeNameUs, localeName, sizeof(localeName));

    if (NT_SUCCESS(RtlLcidToLocaleName(PhGetUserDefaultLCID(), &localeNameUs, 0, FALSE)))
    {
        return PhCreateStringFromUnicodeString(&localeNameUs);
    }
#endif

    return NULL;
}

// rev from LCIDToLocaleName
PPH_STRING PhLCIDToLocaleName(
    _In_ LCID lcid
    )
{
#if defined(PHNT_NATIVE_LOCALE)
    ULONG localNameLength;
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { UNICODE_NULL };

    localNameLength = LCIDToLocaleName(lcid, localeName, RTL_NUMBER_OF(localeName), 0);

    if (localNameLength > sizeof(UNICODE_NULL))
    {
        return PhCreateStringEx(localeName, localNameLength - sizeof(UNICODE_NULL));
    }
#else
    UNICODE_STRING localeNameUs;
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { UNICODE_NULL };

    RtlInitEmptyUnicodeString(&localeNameUs, localeName, sizeof(localeName));

    if (lcid)
    {
        if (NT_SUCCESS(RtlLcidToLocaleName(lcid, &localeNameUs, 0, FALSE)))
        {
            return PhCreateStringFromUnicodeString(&localeNameUs);
        }
    }
    else // Return the current user locale when zero is specified.
    {
        if (NT_SUCCESS(RtlLcidToLocaleName(PhGetUserDefaultLCID(), &localeNameUs, 0, FALSE)))
        {
            return PhCreateStringFromUnicodeString(&localeNameUs);
        }
    }
#endif

    return NULL;
}

VOID PhLargeIntegerToSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    )
{
#if defined(PHNT_NATIVE_TIME)
    FILETIME fileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;
    FileTimeToSystemTime(&fileTime, SystemTime);
#else
    TIME_FIELDS timeFields;

    RtlZeroMemory(&timeFields, sizeof(TIME_FIELDS));

    RtlTimeToTimeFields(LargeInteger, &timeFields);
    SystemTime->wYear = timeFields.Year;
    SystemTime->wMonth = timeFields.Month;
    SystemTime->wDay = timeFields.Day;
    SystemTime->wHour = timeFields.Hour;
    SystemTime->wMinute = timeFields.Minute;
    SystemTime->wSecond = timeFields.Second;
    SystemTime->wMilliseconds = timeFields.Milliseconds;
    SystemTime->wDayOfWeek = timeFields.Weekday;
#endif
}

BOOLEAN PhSystemTimeToLargeInteger(
    _Out_ PLARGE_INTEGER LargeInteger,
    _In_ PSYSTEMTIME SystemTime
    )
{
#if defined(PHNT_NATIVE_TIME)
    FILETIME fileTime;

    if (!SystemTimeToFileTime(SystemTime, &fileTime))
        return FALSE;

    LargeInteger->LowPart = fileTime.dwLowDateTime;
    LargeInteger->HighPart = fileTime.dwHighDateTime;
    return TRUE;
#else
    TIME_FIELDS timeFields;

    RtlZeroMemory(&timeFields, sizeof(TIME_FIELDS));

    timeFields.Year = SystemTime->wYear;
    timeFields.Month = SystemTime->wMonth;
    timeFields.Day = SystemTime->wDay;
    timeFields.Hour = SystemTime->wHour;
    timeFields.Minute = SystemTime->wMinute;
    timeFields.Second = SystemTime->wSecond;
    timeFields.Milliseconds = SystemTime->wMilliseconds;
    timeFields.Weekday = SystemTime->wDayOfWeek;

    return RtlTimeFieldsToTime(&timeFields, LargeInteger);
#endif
}

VOID PhLargeIntegerToLocalSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    )
{
#if defined(PHNT_NATIVE_TIME)
    FILETIME fileTime;
    FILETIME newFileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;

    FileTimeToLocalFileTime(&fileTime, &newFileTime);
    FileTimeToSystemTime(&newFileTime, SystemTime);
#else
    LARGE_INTEGER timeZoneBias;
    LARGE_INTEGER fileTime;

    PhQueryTimeZoneBias(&timeZoneBias);

    fileTime.LowPart = LargeInteger->LowPart;
    fileTime.HighPart = LargeInteger->HighPart;
    fileTime.QuadPart -= timeZoneBias.QuadPart;
    PhLargeIntegerToSystemTime(SystemTime, &fileTime);
#endif
}

BOOLEAN PhLocalSystemTimeToLargeInteger(
    _Out_ PLARGE_INTEGER LargeInteger,
    _In_ PSYSTEMTIME SystemTime
    )
{
    LARGE_INTEGER timeZoneBias;

    if (!PhSystemTimeToLargeInteger(LargeInteger, SystemTime))
        return FALSE;

    PhQueryTimeZoneBias(&timeZoneBias);
    LargeInteger->QuadPart += timeZoneBias.QuadPart;

    return TRUE;
}

BOOLEAN PhSystemTimeToTzSpecificLocalTime(
    _In_ CONST SYSTEMTIME* UniversalTime,
    _Out_ PSYSTEMTIME LocalTime
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&SystemTimeToTzSpecificLocalTimeEx) SystemTimeToTzSpecificLocalTimeEx_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        SystemTimeToTzSpecificLocalTimeEx_I = PhGetDllProcedureAddress(L"kernel32.dll", "SystemTimeToTzSpecificLocalTimeEx", 0);
        PhEndInitOnce(&initOnce);
    }

    memset(LocalTime, 0, sizeof(SYSTEMTIME));

    if (SystemTimeToTzSpecificLocalTimeEx_I)
    {
        return !!SystemTimeToTzSpecificLocalTimeEx_I(NULL, UniversalTime, LocalTime);
    }

    return !!SystemTimeToTzSpecificLocalTime(NULL, UniversalTime, LocalTime);
}

/**
 * Gets a string stored in a DLL's message table.
 *
 * \param DllHandle The base address of the DLL.
 * \param MessageTableId The identifier of the message table.
 * \param MessageLanguageId The language ID of the message.
 * \param MessageId The identifier of the message.
 *
 * \return A pointer to a string containing the message. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhGetMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG MessageTableId,
    _In_ ULONG MessageLanguageId,
    _In_ ULONG MessageId
    )
{
    NTSTATUS status;
    PMESSAGE_RESOURCE_ENTRY messageEntry;

    status = RtlFindMessage(
        DllHandle,
        MessageTableId,
        MessageLanguageId,
        MessageId,
        &messageEntry
        );

    // Try using the system LANGID.
    if (!NT_SUCCESS(status))
    {
        status = RtlFindMessage(
            DllHandle,
            MessageTableId,
            PhGetSystemDefaultLangID(),
            MessageId,
            &messageEntry
            );
    }

    // Try using U.S. English.
    if (!NT_SUCCESS(status))
    {
        status = RtlFindMessage(
            DllHandle,
            MessageTableId,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            MessageId,
            &messageEntry
            );
    }

    if (!NT_SUCCESS(status))
        return NULL;

    // dmex: We don't support parsing insert sequences.
    if (messageEntry->Text[0] == L'%')
        return NULL;

    if (messageEntry->Flags & MESSAGE_RESOURCE_UNICODE)
        return PhCreateStringEx((PWCHAR)messageEntry->Text, messageEntry->Length);
    else if (messageEntry->Flags & MESSAGE_RESOURCE_UTF8)
        return PhConvertUtf8ToUtf16Ex((PCHAR)messageEntry->Text, messageEntry->Length);
    else
        return PhConvertMultiByteToUtf16Ex((PCHAR)messageEntry->Text, messageEntry->Length);
}

/**
 * Gets a message describing a NT status value.
 *
 * \param Status The NT status value.
 */
PPH_STRING PhGetNtMessage(
    _In_ NTSTATUS Status
    )
{
    PPH_STRING message;

    if (NT_CUSTOMER(Status))
        message = PhGetMessage(PhGetLoaderEntryDllBase(NULL, NULL), 0xb, PhGetUserDefaultLangID(), (ULONG)Status);
    else if (!NT_NTWIN32(Status))
        message = PhGetMessage(PhGetLoaderEntryDllBaseZ(L"ntdll.dll"), 0xb, PhGetUserDefaultLangID(), (ULONG)Status);
    else
        message = PhGetWin32Message(PhNtStatusToDosError(Status));

    if (PhIsNullOrEmptyString(message))
        return message;

    PhTrimToNullTerminatorString(message);

    // Remove any trailing newline.
    if (message->Length >= 2 * sizeof(WCHAR) &&
        message->Buffer[message->Length / sizeof(WCHAR) - 2] == L'\r' &&
        message->Buffer[message->Length / sizeof(WCHAR) - 1] == L'\n')
    {
        PhMoveReference(&message, PhCreateStringEx(message->Buffer, message->Length - 2 * sizeof(WCHAR)));
    }

    // Fix those messages which are formatted like:
    // {Asdf}\r\nAsdf asdf asdf...
    if (message->Buffer[0] == L'{')
    {
        PH_STRINGREF titlePart;
        PH_STRINGREF remainingPart;

        if (PhSplitStringRefAtChar(&message->sr, L'\n', &titlePart, &remainingPart))
            PhMoveReference(&message, PhCreateString2(&remainingPart));
    }

    return message;
}

/**
 * Gets a message describing a Win32 error code.
 *
 * \param Result The Win32 error code.
 */
PPH_STRING PhGetWin32Message(
    _In_ ULONG Result
    )
{
    PPH_STRING message;

    message = PhGetMessage(PhGetLoaderEntryDllBaseZ(L"kernel32.dll"), 0xb, PhGetUserDefaultLangID(), Result);

    if (message)
    {
        PhTrimToNullTerminatorString(message);

        // Remove any trailing newline.
        if (message && message->Length >= 2 * sizeof(WCHAR) &&
            message->Buffer[message->Length / sizeof(WCHAR) - 2] == L'\r' &&
            message->Buffer[message->Length / sizeof(WCHAR) - 1] == L'\n')
        {
            PhMoveReference(&message, PhCreateStringEx(message->Buffer, message->Length - 2 * sizeof(WCHAR)));
        }
    }
    else
    {
        message = PhGetWin32FormatMessage(Result);
    }

    return message;
}

PPH_STRING PhGetWin32FormatMessage(
    _In_ ULONG Result
    )
{
    PPH_STRING messageString = NULL;
    ULONG messageLength = 0;
    PWSTR messageBuffer = NULL;

    messageLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        Result,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (PWSTR)&messageBuffer,
        0,
        NULL
        );

    if (messageBuffer)
    {
        if (messageLength)
        {
            ULONG_PTR index;
            PH_STRINGREF string;

            string.Buffer = messageBuffer;
            string.Length = messageLength * sizeof(WCHAR);

            if ((index = PhFindStringInStringRefZ(&string, L"\r\n", FALSE)) != SIZE_MAX)
                messageString = PhCreateStringEx(messageBuffer, index * sizeof(WCHAR));
            else
                messageString = PhCreateStringEx(messageBuffer, messageLength * sizeof(WCHAR));
        }

        LocalFree(messageBuffer);
    }

    return messageString;
}

PPH_STRING PhGetNtFormatMessage(
    _In_ NTSTATUS Status
    )
{
    PPH_STRING messageString = NULL;
    ULONG messageLength = 0;
    PWSTR messageBuffer = NULL;

    messageLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
        PhGetLoaderEntryDllBaseZ(RtlNtdllName),
        Status,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (PWSTR)&messageBuffer,
        0,
        NULL
        );

    if (messageBuffer)
    {
        if (messageLength)
        {
            ULONG_PTR index;
            PH_STRINGREF string;

            string.Buffer = messageBuffer;
            string.Length = messageLength * sizeof(WCHAR);

            if ((index = PhFindStringInStringRefZ(&string, L"\r\n", FALSE)) != SIZE_MAX)
                messageString = PhCreateStringEx(messageBuffer, index * sizeof(WCHAR));
            else
                messageString = PhCreateStringEx(messageBuffer, messageLength * sizeof(WCHAR));
        }

        LocalFree(messageBuffer);
    }

    return messageString;
}

/**
 * Displays a message box.
 *
 * \param WindowHandle The owner window of the message box.
 * \param Type The type of message box to display.
 * \param Format A format string.
 * \return The user's response.
 */
LONG PhShowMessage(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ PCWSTR Format,
    ...
    )
{
    LONG result;
    va_list argptr;
    PPH_STRING message;

    va_start(argptr, Format);
    message = PhFormatString_V(Format, argptr);
    va_end(argptr);

    if (!message)
        return INT_ERROR;

    result = MessageBox(WindowHandle, message->Buffer, PhApplicationName, Type);
    PhDereferenceObject(message);

    return result;
}

static const PH_FLAG_MAPPING PhShowMessageTaskDialogButtonFlagMappings[] =
{
    { TD_OK_BUTTON, TDCBF_OK_BUTTON },
    { TD_YES_BUTTON, TDCBF_YES_BUTTON },
    { TD_NO_BUTTON, TDCBF_NO_BUTTON },
    { TD_CANCEL_BUTTON, TDCBF_CANCEL_BUTTON },
    { TD_RETRY_BUTTON, TDCBF_RETRY_BUTTON },
    { TD_CLOSE_BUTTON, TDCBF_CLOSE_BUTTON },
};

LONG PhShowMessage2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    )
{
    ULONG result;
    va_list argptr;
    PPH_STRING message;
    TASKDIALOGCONFIG config;
    ULONG buttonsFlags;

    va_start(argptr, Format);
    message = PhFormatString_V(Format, argptr);
    va_end(argptr);

    if (!message)
        return INT_ERROR;

    buttonsFlags = 0;
    PhMapFlags1(
        &buttonsFlags,
        Buttons,
        PhShowMessageTaskDialogButtonFlagMappings,
        ARRAYSIZE(PhShowMessageTaskDialogButtonFlagMappings)
        );

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | ((WindowHandle && IsWindowVisible(WindowHandle) && !IsMinimized(WindowHandle)) ? TDF_POSITION_RELATIVE_TO_WINDOW : 0);
    config.dwCommonButtons = buttonsFlags;
    config.hwndParent = WindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = Icon;
    config.pszMainInstruction = Title;
    config.pszContent = message->Buffer;

    if (PhShowTaskDialog(
        &config,
        &result,
        NULL,
        NULL
        ))
    {
        PhDereferenceObject(message);
        return result;
    }
    else
    {
        PhDereferenceObject(message);
        return INT_ERROR;
    }
}

BOOLEAN PhpShowMessageOneTime(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _Out_opt_ PLONG Result,
    _Out_opt_ PBOOLEAN Checked,
    _In_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    ULONG result;
    PPH_STRING message;
    TASKDIALOGCONFIG config;
    BOOLEAN checked = FALSE;
    ULONG buttonsFlags;

    if (Result)
        *Result = INT_ERROR;

    if (Checked)
        *Checked = FALSE;

    message = PhFormatString_V(Format, ArgPtr);

    if (!message)
        return FALSE;

    buttonsFlags = 0;
    PhMapFlags1(
        &buttonsFlags,
        Buttons,
        PhShowMessageTaskDialogButtonFlagMappings,
        ARRAYSIZE(PhShowMessageTaskDialogButtonFlagMappings)
        );

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | ((WindowHandle && IsWindowVisible(WindowHandle) && !IsMinimized(WindowHandle)) ? TDF_POSITION_RELATIVE_TO_WINDOW : 0);
    config.dwCommonButtons = buttonsFlags;
    config.hwndParent = WindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = Icon;
    config.pszMainInstruction = Title;
    config.pszContent = PhGetString(message);
    config.pszVerificationText = L"Don't show this message again";
    config.cxWidth = 200;

    if (PhShowTaskDialog(
        &config,
        &result,
        NULL,
        &checked
        ))
    {
        PhDereferenceObject(message);

        if (Result)
            *Result = result;

        if (Checked)
            *Checked = !!checked;

        return TRUE;
    }
    else
    {
        PhDereferenceObject(message);
        return FALSE;
    }
}

BOOLEAN PhShowMessageOneTime(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    )
{
    BOOLEAN checked;
    va_list argptr;

    va_start(argptr, Format);
    PhpShowMessageOneTime(WindowHandle, Buttons, Icon, Title, NULL, &checked, Format, argptr);
    va_end(argptr);

    return checked;
}

LONG PhShowMessageOneTime2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ PCWSTR Title,
    _Out_opt_ PBOOLEAN Checked,
    _In_ PCWSTR Format,
    ...
    )
{
    LONG result;
    va_list argptr;

    va_start(argptr, Format);
    PhpShowMessageOneTime(WindowHandle, Buttons, Icon, Title, &result, Checked, Format, argptr);
    va_end(argptr);

    return result;
}

_Success_(return)
BOOLEAN PhShowTaskDialog(
    _In_ PTASKDIALOGCONFIG Config,
    _Out_opt_ PULONG Button,
    _Out_opt_ PULONG RadioButton,
    _Out_opt_ PBOOLEAN FlagChecked
    )
{
    HRESULT status;
    LONG button;
    LONG radio;
    BOOL selected;

    status = TaskDialogIndirect(
        Config,
        &button,
        &radio,
        &selected
        );

    if (HR_SUCCESS(status))
    {
        if (Button) *Button = button;
        if (RadioButton) *RadioButton = radio;
        if (FlagChecked) *FlagChecked = !!selected;
        return TRUE;
    }

    return FALSE; // PhDosErrorToNtStatus(HRESULT_CODE(status));
}

PPH_STRING PhGetStatusMessage(
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    if (!Win32Result)
    {
        // In some cases we want the simple Win32 messages.
        if (
            Status == STATUS_ACCESS_DENIED ||
            Status == STATUS_ACCESS_VIOLATION ||
            Status == STATUS_NO_SUCH_FILE
            )
        {
            Win32Result = RtlNtStatusToDosErrorNoTeb(Status);
        }
        // Process NTSTATUS values with the NT-Win32 facility.
        else if (NT_NTWIN32(Status))
        {
            Win32Result = WIN32_FROM_NTSTATUS(Status);
        }
        else if (NT_FACILITY(Status) == FACILTIY_MUI_ERROR_CODE)
        {
            Win32Result = Status; // needed by PhGetLastWin32ErrorAsNtStatus(CERT_E_REVOKED) (dmex)
        }
    }

    if (Win32Result)
        return PhGetWin32Message(Win32Result);
    else
        return PhGetNtMessage(Status);
}

/**
 * Displays an error message for a NTSTATUS value or Win32 error code.
 *
 * \param WindowHandle The owner window of the message box.
 * \param Message A message describing the operation that failed.
 * \param Status A NTSTATUS value, or 0 if there is none.
 * \param Win32Result A Win32 error code, or 0 if there is none.
 */
VOID PhShowStatus(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    PPH_STRING statusMessage;

    if (statusMessage = PhGetStatusMessage(Status, Win32Result))
    {
        if (Message)
            PhShowError2(WindowHandle, Message, L"%s", PhGetString(statusMessage));
        else
            PhShowError2(WindowHandle, L"Unable to perform the operation.", L"%s", PhGetString(statusMessage));

        PhDereferenceObject(statusMessage);
    }
    else
    {
        if (Message)
            PhShowError2(WindowHandle, L"Unable to perform the operation.", L"%s", Message);
        else
            PhShowStatus(WindowHandle, L"Unable to perform the operation.", STATUS_UNSUCCESSFUL, 0);
    }
}

/**
 * Displays an error message for a NTSTATUS value or Win32 error code, and allows the user to cancel
 * the current operation.
 *
 * \param WindowHandle The owner window of the message box.
 * \param Message A message describing the operation that failed.
 * \param Status A NTSTATUS value, or 0 if there is none.
 * \param Win32Result A Win32 error code, or 0 if there is none.
 * \return TRUE if the user wishes to continue with the current operation, otherwise FALSE.
 */
BOOLEAN PhShowContinueStatus(
    _In_ HWND WindowHandle,
    _In_opt_ PCWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    PPH_STRING statusMessage;
    LONG result;

    statusMessage = PhGetStatusMessage(Status, Win32Result);

    if (Message && statusMessage)
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CLOSE_BUTTON, TD_ERROR_ICON, Message, L"%s", PhGetString(statusMessage));
    else if (Message)
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CANCEL_BUTTON, TD_ERROR_ICON, L"Unable to perform the operation.", L"%s", Message);
    else if (statusMessage)
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CANCEL_BUTTON, TD_ERROR_ICON, L"Unable to perform the operation.", L"%s", PhGetString(statusMessage));
    else
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CANCEL_BUTTON, TD_ERROR_ICON, L"Unable to perform the operation.", L"");

    if (statusMessage) PhDereferenceObject(statusMessage);

    return result == IDOK;
}

/**
 * Displays a confirmation message.
 *
 * \param WindowHandle The owner window of the message box.
 * \param Verb A verb describing the operation, e.g. "terminate".
 * \param Object The object of the operation, e.g. "the process".
 * \param Message A message describing the operation.
 * \param Warning TRUE to display the confirmation message as a warning, otherwise FALSE.
 * \return TRUE if the user wishes to continue, otherwise FALSE.
 */
BOOLEAN PhShowConfirmMessage(
    _In_ HWND WindowHandle,
    _In_ PCWSTR Verb,
    _In_ PCWSTR Object,
    _In_opt_ PCWSTR Message,
    _In_ BOOLEAN Warning
    )
{
    PPH_STRING verb;
    PPH_STRING verbCaps;
    PPH_STRING action;

    // Make sure the verb is all lowercase.
    verb = PhaLowerString(PhaCreateString(Verb));

    // "terminate" -> "Terminate"
    verbCaps = PhaDuplicateString(verb);
    if (verbCaps->Length > 0) verbCaps->Buffer[0] = PhUpcaseUnicodeChar(verbCaps->Buffer[0]);

    // "terminate", "the process" -> "terminate the process"
    action = PhaConcatStrings(3, verb->Buffer, L" ", Object);

    {
        ULONG button;
        TASKDIALOGCONFIG config;
        TASKDIALOG_BUTTON buttons[2];

        memset(&config, 0, sizeof(TASKDIALOGCONFIG));
        config.cbSize = sizeof(TASKDIALOGCONFIG);
        config.hwndParent = WindowHandle;
        config.hInstance = NtCurrentImageBase();
        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | ((WindowHandle && IsWindowVisible(WindowHandle) && !IsMinimized(WindowHandle)) ? TDF_POSITION_RELATIVE_TO_WINDOW : 0);
        config.pszWindowTitle = PhApplicationName;
        config.pszMainIcon = Warning ? TD_WARNING_ICON : TD_INFORMATION_ICON;
        config.pszMainInstruction = PhaConcatStrings(3, L"Do you want to ", action->Buffer, L"?")->Buffer;
        if (Message) config.pszContent = PhaConcatStrings2(Message, L" Are you sure you want to continue?")->Buffer;

        buttons[0].nButtonID = IDYES;
        buttons[0].pszButtonText = verbCaps->Buffer;
        buttons[1].nButtonID = IDNO;
        buttons[1].pszButtonText = L"Cancel";

        config.cButtons = 2;
        config.pButtons = buttons;
        config.nDefaultButton = IDYES;
        config.cxWidth = 200;

        if (PhShowTaskDialog(
            &config,
            &button,
            NULL,
            NULL
            ))
        {
            return button == IDYES;
        }

        if (PhShowMessage(
            WindowHandle,
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
            L"Are you sure you want to %s?",
            action->Buffer
            ) == IDYES)
        {
            return TRUE;
        }

        return FALSE;
    }
}

/**
 * Creates a random (type 4) UUID.
 *
 * \param Guid The destination UUID.
 */
VOID PhGenerateGuid(
    _Out_ PGUID Guid
    )
{
    ULARGE_INTEGER seed;
    // The top/sign bit is always unusable for RtlRandomEx (the result is always unsigned), so we'll
    // take the bottom 24 bits. We need 128 bits in total, so we'll call the function 6 times.
    ULONG random[6];
    ULONG i;

    seed.QuadPart = PhReadPerformanceCounter();

    for (i = 0; i < 6; i++)
        random[i] = RtlRandomEx(&seed.LowPart);

    // random[0] is usable
    *(PUSHORT)&Guid->Data1 = (USHORT)random[0];
    // top byte from random[0] is usable
    *((PUSHORT)&Guid->Data1 + 1) = (USHORT)((random[0] >> 16) | (random[1] & 0xff));
    // top 2 bytes from random[1] are usable
    Guid->Data2 = (SHORT)(random[1] >> 8);
    // random[2] is usable
    Guid->Data3 = (SHORT)random[2];
    // top byte from random[2] is usable
    *(PUSHORT)&Guid->Data4[0] = (USHORT)((random[2] >> 16) | (random[3] & 0xff));
    // top 2 bytes from random[3] are usable
    *(PUSHORT)&Guid->Data4[2] = (USHORT)(random[3] >> 8);
    // random[4] is usable
    *(PUSHORT)&Guid->Data4[4] = (USHORT)random[4];
    // top byte from random[4] is usable
    *(PUSHORT)&Guid->Data4[6] = (USHORT)((random[4] >> 16) | (random[5] & 0xff));

    ((PGUID_EX)Guid)->s2.Version = GUID_VERSION_RANDOM;
    ((PGUID_EX)Guid)->s2.Variant &= ~GUID_VARIANT_STANDARD_MASK;
    ((PGUID_EX)Guid)->s2.Variant |= GUID_VARIANT_STANDARD;
}

// rev from kernelbase (dmex)
//VOID PhGenerateGuid2(
//    _Out_ PGUID Guid
//    )
//{
//    LARGE_INTEGER seed;
//    PUSHORT buffer;
//    ULONG count;
//
//    PhQuerySystemTime(&seed);
//    buffer = (PUSHORT)Guid;
//    count = 8;
//
//    do
//    {
//        *buffer = (USHORT)RtlRandomEx(&seed.LowPart);
//        buffer = PTR_ADD_OFFSET(buffer, sizeof(USHORT));
//    } while (--count);
//}

/**
 * Creates a name-based (type 3 or 5) UUID.
 *
 * \param Guid The destination UUID.
 * \param Namespace The UUID of the namespace.
 * \param Name The input name.
 * \param NameLength The length of the input name, not including the null terminator if present.
 * \param Version The type of UUID.
 * \li \c GUID_VERSION_MD5 Creates a type 3, MD5-based UUID.
 * \li \c GUID_VERSION_SHA1 Creates a type 5, SHA1-based UUID.
 */
VOID PhGenerateGuidFromName(
    _Out_ PGUID Guid,
    _In_ PGUID Namespace,
    _In_ PCHAR Name,
    _In_ ULONG NameLength,
    _In_ UCHAR Version
    )
{
    PGUID_EX guid;
    PUCHAR data;
    ULONG dataLength;
    GUID ns;
    UCHAR hash[20];

    // Convert the namespace to big endian.

    ns = *Namespace;
    PhReverseGuid(&ns);

    // Compute the hash of the namespace concatenated with the name.

    dataLength = 16 + NameLength;
    data = PhAllocate(dataLength);
    memcpy(data, &ns, 16);
    memcpy(&data[16], Name, NameLength);

    if (Version == GUID_VERSION_MD5)
    {
        MD5_CTX context;

        MD5Init(&context);
        MD5Update(&context, data, dataLength);
        MD5Final(&context);

        memcpy(hash, context.digest, 16);
    }
    else
    {
        A_SHA_CTX context;

        A_SHAInit(&context);
        A_SHAUpdate(&context, data, dataLength);
        A_SHAFinal(&context, hash);

        Version = GUID_VERSION_SHA1;
    }

    PhFree(data);

    guid = (PGUID_EX)Guid;
    memcpy(guid->Data, hash, 16);
    PhReverseGuid(&guid->Guid);
    guid->s2.Version = Version;
    guid->s2.Variant &= ~GUID_VARIANT_STANDARD_MASK;
    guid->s2.Variant |= GUID_VARIANT_STANDARD;
}

// rev from ntoskrnl.exe!RtlGenerateClass5Guid (dmex)
VOID PhGenerateClass5Guid(
    _In_ REFGUID NamespaceGuid,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PGUID Guid
    )
{
    A_SHA_CTX context;
    GUID data;
    PGUID_EX guid;
    UCHAR hash[20];

    data = *NamespaceGuid;
    data.Data1 = _byteswap_ulong(NamespaceGuid->Data1);
    data.Data2 = _rotr16(NamespaceGuid->Data2, 8);
    data.Data3 = _rotr16(NamespaceGuid->Data3, 8);

    A_SHAInit(&context);
    A_SHAUpdate(&context, (PUCHAR)&data, sizeof(GUID));
    A_SHAUpdate(&context, Buffer, BufferSize);
    A_SHAFinal(&context, hash);

    guid = (PGUID_EX)Guid;
    memcpy(guid->Data, hash, sizeof(GUID));
    guid->Guid.Data1 = _byteswap_ulong(guid->Guid.Data1);
    guid->Guid.Data2 = _rotr16(guid->Guid.Data2, 8);
    guid->Guid.Data3 = _rotr16(guid->Guid.Data3, 8);
    guid->s2.Version = GUID_VERSION_SHA1;
    guid->s2.Variant &= ~GUID_VARIANT_STANDARD_MASK;
    guid->s2.Variant |= GUID_VARIANT_STANDARD;
}

// rev from Windows SDK\\ComputerHardwareIds.exe (dmex)
// https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/computerhardwareids
// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/specifying-hardware-ids-for-a-computer
VOID PhGenerateHardwareIDGuid(
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PGUID Guid
    )
{
    PhGenerateClass5Guid(&GUID_NAMESPACE_MICROSOFT, Buffer, BufferSize, Guid);
}

/**
 * Fills a buffer with random uppercase alphabetical characters.
 *
 * \param Buffer The buffer to fill with random characters, plus a null terminator.
 * \param Count The number of characters available in the buffer, including space for the null
 * terminator.
 */
VOID PhGenerateRandomAlphaString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ SIZE_T Count
    )
{
    ULARGE_INTEGER seed;
    ULONG i;

    if (Count == 0)
        return;

    seed.QuadPart = PhReadPerformanceCounter();

    for (i = 0; i < Count - 1; i++)
    {
        Buffer[i] = L'A' + (RtlRandomEx(&seed.LowPart) % 26);
    }

    Buffer[Count - 1] = UNICODE_NULL;
}

/**
 * Fills a buffer with random numeric characters.
 *
 * \param Buffer The buffer to fill with random characters, plus a null terminator.
 * \param Count The number of characters available in the buffer, including space for the null
 * terminator.
 */
VOID PhGenerateRandomNumericString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ SIZE_T Count
    )
{
    ULARGE_INTEGER seed;
    ULONG i;

    if (Count == 0)
        return;

    seed.QuadPart = PhReadPerformanceCounter();

    for (i = 0; i < Count - 1; i++)
    {
        Buffer[i] = L'0' + (RtlRandomEx(&seed.LowPart) % 10);
    }

    Buffer[Count - 1] = UNICODE_NULL;
}

ULONG64 PhGenerateRandomNumber64(
    VOID
    )
{
    ULARGE_INTEGER seed;
    ULARGE_INTEGER value;

    seed.QuadPart = PhReadPerformanceCounter();
    value.LowPart = RtlRandomEx(&seed.LowPart);
    value.HighPart = RtlRandomEx(&seed.LowPart);

    return value.QuadPart;
}

BOOLEAN PhGenerateRandomNumber(
    _Out_ PLARGE_INTEGER Number
    )
{
    memset(Number, 0, sizeof(LARGE_INTEGER));

#ifndef _M_ARM64
    if (PhIsProcessorFeaturePresent(PF_RDRAND_INSTRUCTION_AVAILABLE))
    {
        ULONG count = 0;

#ifdef _M_X64
        while (TRUE)
        {
            if (_rdrand64_step(&Number->QuadPart))
                break;
            if (++count >= 10)
                return FALSE;
        }
#else
        ULONG low = 0;
        ULONG high = 0;

        while (TRUE)
        {
            if (_rdrand32_step(&low) && _rdrand32_step(&high))
            {
                Number->LowPart = low;
                Number->HighPart = high;
                break;
            }

            if (++count >= 10)
                return FALSE;
        }
#endif
        return TRUE;
    }
#endif

    Number->QuadPart = PhGenerateRandomNumber64();
    return TRUE;
}

BOOLEAN PhGenerateRandomSeed(
    _Out_ PLARGE_INTEGER Seed
    )
{
    memset(Seed, 0, sizeof(LARGE_INTEGER));

//#ifndef _M_ARM64
//    if (PhIsProcessorFeaturePresent(PF_RDRAND_INSTRUCTION_AVAILABLE))
//    {
//#ifdef _M_X64
//        if (_rdseed64_step(&Seed->QuadPart))
//            return TRUE;
//#else
//        if (_rdseed32_step(&Seed->LowPart))
//            return TRUE;
//#endif
//    }
//#endif

    return PhQueryPerformanceCounter(Seed);
}

/**
 * Modifies a string to ensure it is within the specified length.
 *
 * \param String The input string.
 * \param DesiredCount The desired number of characters in the new string. If necessary, parts of
 * the string are replaced with an ellipsis to indicate characters have been omitted.
 * \return The new string.
 */
PPH_STRING PhEllipsisString(
    _In_ PPH_STRING String,
    _In_ SIZE_T DesiredCount
    )
{
    if (
        String->Length / sizeof(WCHAR) <= DesiredCount ||
        DesiredCount < 3
        )
    {
        return PhReferenceObject(String);
    }
    else
    {
        PPH_STRING string;

        string = PhCreateStringEx(NULL, DesiredCount * sizeof(WCHAR));
        memcpy(string->Buffer, String->Buffer, (DesiredCount - 3) * sizeof(WCHAR));
        memcpy(&string->Buffer[DesiredCount - 3], L"...", 6);

        return string;
    }
}

/**
 * Modifies a string to ensure it is within the specified length, parsing the string as a path.
 *
 * \param String The input string.
 * \param DesiredCount The desired number of characters in the new string. If necessary, parts of
 * the string are replaced with an ellipsis to indicate characters have been omitted.
 * \return The new string.
 */
PPH_STRING PhEllipsisStringPath(
    _In_ PPH_STRING String,
    _In_ SIZE_T DesiredCount
    )
{
    ULONG_PTR secondPartIndex;

    secondPartIndex = PhFindLastCharInString(String, 0, OBJ_NAME_PATH_SEPARATOR);

    if (secondPartIndex == SIZE_MAX)
        secondPartIndex = PhFindLastCharInString(String, 0, OBJ_NAME_ALTPATH_SEPARATOR);
    if (secondPartIndex == SIZE_MAX)
        return PhEllipsisString(String, DesiredCount);

    if (
        String->Length / sizeof(WCHAR) <= DesiredCount ||
        DesiredCount < 3
        )
    {
        return PhReferenceObject(String);
    }
    else
    {
        PPH_STRING string;
        ULONG_PTR firstPartCopyLength;
        ULONG_PTR secondPartCopyLength;

        string = PhCreateStringEx(NULL, DesiredCount * sizeof(WCHAR));
        secondPartCopyLength = String->Length / sizeof(WCHAR) - secondPartIndex;

        // Check if we have enough space for the entire second part of the string.
        if (secondPartCopyLength + 3 <= DesiredCount)
        {
            // Yes, copy part of the first part and the entire second part.
            firstPartCopyLength = DesiredCount - secondPartCopyLength - 3;
        }
        else
        {
            // No, copy part of both, from the beginning of the first part and the end of the second
            // part.
            firstPartCopyLength = (DesiredCount - 3) / sizeof(WCHAR);
            secondPartCopyLength = DesiredCount - 3 - firstPartCopyLength;
            secondPartIndex = String->Length / sizeof(WCHAR) - secondPartCopyLength;
        }

        memcpy(
            string->Buffer,
            String->Buffer,
            firstPartCopyLength * sizeof(WCHAR)
            );
        memcpy(
            &string->Buffer[firstPartCopyLength],
            L"...",
            6
            );
        memcpy(
            &string->Buffer[firstPartCopyLength + 3],
            &String->Buffer[secondPartIndex],
            secondPartCopyLength * sizeof(WCHAR)
            );

        return string;
    }
}

FORCEINLINE BOOLEAN PhpMatchWildcards(
    _In_ PCWSTR Pattern,
    _In_ PCWSTR String,
    _In_ BOOLEAN IgnoreCase
    )
{
    PCWCHAR s, p;
    BOOLEAN star = FALSE;

    // Code is from http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html

LoopStart:
    for (s = String, p = Pattern; *s; s++, p++)
    {
        switch (*p)
        {
        case L'?':
            break;
        case L'*':
            star = TRUE;
            String = s;
            Pattern = p;

            do
            {
                Pattern++;
            } while (*Pattern == L'*');

            if (!*Pattern) return TRUE;

            goto LoopStart;
        default:
            if (!IgnoreCase)
            {
                if (*s != *p)
                    goto StarCheck;
            }
            else
            {
                if (PhUpcaseUnicodeChar(*s) != PhUpcaseUnicodeChar(*p))
                    goto StarCheck;
            }

            break;
        }
    }

    while (*p == L'*')
        p++;

    return (!*p);

StarCheck:
    if (!star)
        return FALSE;

    String++;
    goto LoopStart;
}

/**
 * Matches a pattern against a string.
 *
 * \param Pattern The pattern, which can contain asterisks and question marks.
 * \param String The string which the pattern is matched against.
 * \param IgnoreCase Whether to ignore character cases.
 */
BOOLEAN PhMatchWildcards(
    _In_ PCWSTR Pattern,
    _In_ PCWSTR String,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return PhpMatchWildcards(Pattern, String, TRUE);
    else
        return PhpMatchWildcards(Pattern, String, FALSE);
}

/**
 * Escapes a string for prefix characters (ampersands).
 *
 * \param String The string to process.
 *
 * \return The escaped string, with each ampersand replaced by 2 ampersands.
 */
PPH_STRING PhEscapeStringForMenuPrefix(
    _In_ PCPH_STRINGREF String
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T i;
    SIZE_T length;
    PWCHAR runStart;
    SIZE_T runCount = 0;

    length = String->Length / sizeof(WCHAR);
    runStart = NULL;

    PhInitializeStringBuilder(&stringBuilder, String->Length);

    for (i = 0; i < length; i++)
    {
        switch (String->Buffer[i])
        {
        case L'&':
            if (runStart)
            {
                PhAppendStringBuilderEx(&stringBuilder, runStart, runCount * sizeof(WCHAR));
                runStart = NULL;
            }

            PhAppendStringBuilder2(&stringBuilder, L"&&");

            break;
        default:
            if (runStart)
            {
                runCount++;
            }
            else
            {
                runStart = &String->Buffer[i];
                runCount = 1;
            }

            break;
        }
    }

    if (runStart)
        PhAppendStringBuilderEx(&stringBuilder, runStart, runCount * sizeof(WCHAR));

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Compares two strings, ignoring prefix characters (ampersands).
 *
 * \param A The first string.
 * \param B The second string.
 * \param IgnoreCase Whether to ignore character cases.
 * \param MatchIfPrefix Specify TRUE to return 0 when \a A is a prefix of \a B.
 */
LONG PhCompareUnicodeStringZIgnoreMenuPrefix(
    _In_ PCWSTR A,
    _In_ PCWSTR B,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN MatchIfPrefix
    )
{
    WCHAR t;

    if (!A || !B)
        return -1;

    if (!IgnoreCase)
    {
        while (TRUE)
        {
            // This takes care of double ampersands as well (they are treated as one literal
            // ampersand).
            if (*A == L'&')
                A++;
            if (*B == L'&')
                B++;

            t = *A;

            if (t == UNICODE_NULL)
            {
                if (MatchIfPrefix)
                    return 0;

                break;
            }

            if (t != *B)
                break;

            A++;
            B++;
        }

        return C_2uTo4(t) - C_2uTo4(*B);
    }
    else
    {
        while (TRUE)
        {
            if (*A == L'&')
                A++;
            if (*B == L'&')
                B++;

            t = *A;

            if (t == UNICODE_NULL)
            {
                if (MatchIfPrefix)
                    return 0;

                break;
            }

            if (PhUpcaseUnicodeChar(t) != PhUpcaseUnicodeChar(*B))
                break;

            A++;
            B++;
        }

        return C_2uTo4(t) - C_2uTo4(*B);
    }
}

/**
 * Formats a date using the user's default locale.
 *
 * \param Date The time structure. If NULL, the current time is used.
 * \param Format The format of the date. If NULL, the format appropriate to the user's locale is
 * used.
 */
PPH_STRING PhFormatDate(
    _In_opt_ PSYSTEMTIME Date,
    _In_opt_ PCWSTR Format
    )
{
    PPH_STRING string;
    LONG bufferSize;

    bufferSize = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, Date, Format, NULL, 0, NULL);
    string = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));

    if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, Date, Format, string->Buffer, bufferSize, NULL))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

/**
 * Formats a time using the user's default locale.
 *
 * \param Time The time structure. If NULL, the current time is used.
 * \param Format The format of the time. If NULL, the format appropriate to the user's locale is
 * used.
 */
PPH_STRING PhFormatTime(
    _In_opt_ PSYSTEMTIME Time,
    _In_opt_ PCWSTR Format
    )
{
    PPH_STRING string;
    LONG bufferSize;

    bufferSize = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, Time, Format, NULL, 0);
    string = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));

    if (!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, Time, Format, string->Buffer, bufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

/**
 * Formats a date and time using the user's default locale.
 *
 * \param DateTime The time structure. If NULL, the current time is used.
 *
 * \return A string containing the time, a space character, then the date.
 */
PPH_STRING PhFormatDateTime(
    _In_opt_ PSYSTEMTIME DateTime
    )
{
    PPH_STRING string;
    LONG timeBufferSize;
    LONG dateBufferSize;
    ULONG count;

    timeBufferSize = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, NULL, 0);
    dateBufferSize = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, NULL, 0, NULL);

    string = PhCreateStringEx(NULL, ((SIZE_T)timeBufferSize + 1 + dateBufferSize) * sizeof(WCHAR));

    if (!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, &string->Buffer[0], timeBufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    count = (ULONG)PhCountStringZ(string->Buffer);
    string->Buffer[count] = L' ';

    if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, &string->Buffer[count + 1], dateBufferSize, NULL))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

_Success_(return)
BOOLEAN PhFormatDateTimeToBuffer(
    _In_opt_ PSYSTEMTIME DateTime,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    SIZE_T returnLength;
    LONG timeBufferSize;
    LONG dateBufferSize;

    timeBufferSize = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, NULL, 0);
    dateBufferSize = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, NULL, 0, NULL);

    returnLength = (timeBufferSize + 1 + dateBufferSize) * sizeof(WCHAR);

    if (returnLength >= BufferLength)
        goto CleanupExit;

    if (!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, &Buffer[0], timeBufferSize))
        goto CleanupExit;

    Buffer[timeBufferSize - 1] = L' ';

    if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, DateTime, NULL, &Buffer[timeBufferSize], dateBufferSize, NULL))
        goto CleanupExit;

    if (ReturnLength)
        *ReturnLength = returnLength - sizeof(UNICODE_NULL); // HACK
    Buffer[returnLength] = UNICODE_NULL;
    return TRUE;

CleanupExit:
    if (ReturnLength)
        *ReturnLength = returnLength;
    return FALSE;
}

PPH_STRING PhFormatTimeSpan(
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode
    )
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, PH_TIMESPAN_STR_LEN * sizeof(WCHAR));
    PhPrintTimeSpan(string->Buffer, Ticks, Mode);
    PhTrimToNullTerminatorString(string);

    return string;
}

PPH_STRING PhFormatTimeSpanEx(
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode
    )
{
    SIZE_T returnLength;
    WCHAR buffer[PH_TIMESPAN_STR_LEN_1];

    if (PhPrintTimeSpanToBuffer(
        Ticks,
        Mode,
        buffer,
        PH_TIMESPAN_STR_LEN,
        &returnLength
        ))
    {
        PH_STRINGREF string;

        string.Length = returnLength - sizeof(UNICODE_NULL);
        string.Buffer = buffer;

        return PhCreateString2(&string);
    }

    return PhReferenceEmptyString();
}

/**
 * Formats a relative time span.
 *
 * \param TimeSpan The time span, in ticks.
 */
PPH_STRING PhFormatTimeSpanRelative(
    _In_ ULONG64 TimeSpan
    )
{
    PPH_STRING string;
    FLOAT days;
    FLOAT weeks;
    FLOAT fortnights;
    FLOAT months;
    FLOAT years;
    FLOAT centuries;

    days = (FLOAT)TimeSpan / (FLOAT)PH_TICKS_PER_DAY;
    weeks = days / 7;
    fortnights = weeks / 2;
    years = days / 365.2425f;
    months = years * 12;
    centuries = years / 100;

    if (centuries >= 1)
    {
        string = PhFormatString(L"%u %s", (ULONG)centuries, (ULONG)centuries == 1 ? L"century" : L"centuries");
    }
    else if (years >= 1)
    {
        string = PhFormatString(L"%u %s", (ULONG)years, (ULONG)years == 1 ? L"year" : L"years");
    }
    else if (months >= 1)
    {
        string = PhFormatString(L"%u %s", (ULONG)months, (ULONG)months == 1 ? L"month" : L"months");
    }
    else if (fortnights >= 1)
    {
        string = PhFormatString(L"%u %s", (ULONG)fortnights, (ULONG)fortnights == 1 ? L"fortnight" : L"fortnights");
    }
    else if (weeks >= 1)
    {
        string = PhFormatString(L"%u %s", (ULONG)weeks, (ULONG)weeks == 1 ? L"week" : L"weeks");
    }
    else
    {
        FLOAT milliseconds;
        FLOAT seconds;
        FLOAT minutes;
        FLOAT hours;
        ULONG secondsPartial;
        ULONG minutesPartial;
        ULONG hoursPartial;

        milliseconds = (FLOAT)TimeSpan / (FLOAT)PH_TICKS_PER_MS;
        seconds = (FLOAT)TimeSpan / (FLOAT)PH_TICKS_PER_SEC;
        minutes = (FLOAT)TimeSpan / (FLOAT)PH_TICKS_PER_MIN;
        hours = (FLOAT)TimeSpan / (FLOAT)PH_TICKS_PER_HOUR;

        if (days >= 1)
        {
            hoursPartial = (ULONG)PH_TICKS_PARTIAL_HOURS(TimeSpan);

            if (hoursPartial >= 1)
            {
                string = PhFormatString(
                    L"%u %s and %u %s",
                    (ULONG)days,
                    (ULONG)days == 1 ? L"day" : L"days",
                    hoursPartial,
                    hoursPartial == 1 ? L"hour" : L"hours"
                    );
            }
            else
            {
                string = PhFormatString(L"%u %s", (ULONG)days, (ULONG)days == 1 ? L"day" : L"days");
            }
        }
        else if (hours >= 1)
        {
            minutesPartial = (ULONG)PH_TICKS_PARTIAL_MIN(TimeSpan);

            if (minutesPartial >= 1)
            {
                string = PhFormatString(
                    L"%u %s and %u %s",
                    (ULONG)hours,
                    (ULONG)hours == 1 ? L"hour" : L"hours",
                    (ULONG)minutesPartial,
                    (ULONG)minutesPartial == 1 ? L"minute" : L"minutes"
                    );
            }
            else
            {
                string = PhFormatString(L"%u %s", (ULONG)hours, (ULONG)hours == 1 ? L"hour" : L"hours");
            }
        }
        else if (minutes >= 1)
        {
            secondsPartial = (ULONG)PH_TICKS_PARTIAL_SEC(TimeSpan);

            if (secondsPartial >= 1)
            {
                string = PhFormatString(
                    L"%u %s and %u %s",
                    (ULONG)minutes,
                    (ULONG)minutes == 1 ? L"minute" : L"minutes",
                    (ULONG)secondsPartial,
                    (ULONG)secondsPartial == 1 ? L"second" : L"seconds"
                    );
            }
            else
            {
                string = PhFormatString(L"%u %s", (ULONG)minutes, (ULONG)minutes == 1 ? L"minute" : L"minutes");
            }
        }
        else if (seconds >= 1)
        {
            string = PhFormatString(L"%u %s", (ULONG)seconds, (ULONG)seconds == 1 ? L"second" : L"seconds");
        }
        else if (milliseconds >= 1)
        {
            string = PhFormatString(L"%u %s", (ULONG)milliseconds, (ULONG)milliseconds == 1 ? L"millisecond" : L"milliseconds");
        }
        else
        {
            string = PhCreateString(L"a very short time");
        }
    }

    // Turn 1 into "a", e.g. 1 minute -> a minute
    if (PhStartsWithString2(string, L"1 ", FALSE))
    {
        // Special vowel case: a hour -> an hour
        if (string->Buffer[2] != L'h')
            PhMoveReference(&string, PhConcatStrings2(L"a ", &string->Buffer[2]));
        else
            PhMoveReference(&string, PhConcatStrings2(L"an ", &string->Buffer[2]));
    }

    return string;
}

/**
 * Formats a 64-bit unsigned integer.
 *
 * \param Value The integer.
 * \param GroupDigits TRUE to group digits, otherwise FALSE.
 */
PPH_STRING PhFormatUInt64(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    )
{
    PH_FORMAT format;

    format.Type = UInt64FormatType | (GroupDigits ? FormatGroupDigits : 0);
    format.u.UInt64 = Value;

    return PhFormat(&format, 1, 0);
}

/**
 * Formats a 64-bit unsigned integer using Metric (SI) Prefixes (1000=1k, 1000000=1M, 1000000000000=1B)
 * https://en.wikipedia.org/wiki/Metric_prefix
 *
 * \param Value The integer.
 * \param GroupDigits TRUE to group digits, otherwise FALSE.
 */
PPH_STRING PhFormatUInt64Prefix(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    )
{
    DOUBLE number = (DOUBLE)Value;
    ULONG i = 0;
    PH_FORMAT format[2];

    while (
        number >= 1000 &&
        i + 1 < RTL_NUMBER_OF(PhPrefixUnitNamesCounted) &&
        i < PhMaxSizeUnit
        )
    {
        number /= 1000;
        i++;
    }

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros | (GroupDigits ? FormatGroupDigits : 0);
    format[0].Precision = 2;
    format[0].u.Double = number;
    PhInitFormatSR(&format[1], PhPrefixUnitNamesCounted[i]);

    return PhFormat(format, 2, 0);
}

/**
 * Formats a 64-bit unsigned integer using Data-rate (SI) Prefixes (1000=1Bps, 1000000=1Kbps, 1000000000000=1Mbps)
 * https://en.wikipedia.org/wiki/Data-rate_units
 *
 * \param Value The integer.
 * \param GroupDigits TRUE to group digits, otherwise FALSE.
 */
PPH_STRING PhFormatUInt64BitratePrefix(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    )
{
    DOUBLE number = (DOUBLE)Value;
    ULONG i = 0;
    PH_FORMAT format[2];

    while (
        number >= 1000 &&
        i + 1 < RTL_NUMBER_OF(PhSiPrefixUnitNamesCounted) &&
        i < PhMaxSizeUnit
        )
    {
        number /= 1000;
        i++;
    }

    format[0].Type = DoubleFormatType | FormatUsePrecision | (GroupDigits ? FormatGroupDigits : 0);
    format[0].Precision = 1;
    format[0].u.Double = number;
    PhInitFormatSR(&format[1], PhSiPrefixUnitNamesCounted[i]);

    return PhFormat(format, 2, 0);
}

PPH_STRING PhFormatDecimal(
    _In_ PCWSTR Value,
    _In_ ULONG FractionalDigits,
    _In_ BOOLEAN GroupDigits
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static WCHAR decimalSeparator[4];
    static WCHAR thousandSeparator[4];

    PPH_STRING string;
    NUMBERFMT format;
    ULONG bufferSize;

    if (PhBeginInitOnce(&initOnce))
    {
        if (!GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SDECIMAL, decimalSeparator, 4))
        {
            decimalSeparator[0] = L'.';
            decimalSeparator[1] = UNICODE_NULL;
        }

        if (!GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STHOUSAND, thousandSeparator, 4))
        {
            thousandSeparator[0] = L',';
            thousandSeparator[1] = UNICODE_NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    format.NumDigits = FractionalDigits;
    format.LeadingZero = 0;
    format.Grouping = GroupDigits ? 3 : 0;
    format.lpDecimalSep = decimalSeparator;
    format.lpThousandSep = thousandSeparator;
    format.NegativeOrder = 1;

    bufferSize = GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, Value, &format, NULL, 0);
    string = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));

    if (!GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, Value, &format, string->Buffer, bufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

/**
 * Gets a string representing a size.
 *
 * \param Size The size value.
 * \param MaxSizeUnit The largest unit of size to use, -1 to use PhMaxSizeUnit, or -2 for no limit.
 * \li \c 0 Bytes.
 * \li \c 1 Kilobytes.
 * \li \c 2 Megabytes.
 * \li \c 3 Gigabytes.
 * \li \c 4 Terabytes.
 * \li \c 5 Petabytes.
 * \li \c 6 Exabytes.
 */
PPH_STRING PhFormatSize(
    _In_ ULONG64 Size,
    _In_ ULONG MaxSizeUnit
    )
{
    PH_FORMAT format;

    // PhFormat handles this better than the old method.

    format.Type = SizeFormatType | FormatUseRadix;
    format.Radix = (UCHAR)(MaxSizeUnit != ULONG_MAX ? MaxSizeUnit : PhMaxSizeUnit);
    format.u.Size = Size;

    return PhFormat(&format, 1, 0);
}

/**
 * Gets a string representing a size.
 *
 * \param Size The size value.
 * \param MaxSizeUnit The largest unit of size to use, -1 to use PhMaxSizeUnit, or -2 for no limit.
 * \param Buffer A buffer. If NULL, no data is written.
 * \param BufferLength The number of bytes available in \a Buffer, including space for the null terminator.
 * \param ReturnLength The number of bytes required to hold the string, including the null terminator.
 */
BOOLEAN PhFormatSizeToBuffer(
    _In_ ULONG64 Size,
    _In_ ULONG MaxSizeUnit,
    _Out_writes_bytes_opt_(BufferLength) PWSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    PH_FORMAT format;

    // PhFormat handles this better than the old method.

    format.Type = SizeFormatType | FormatUseRadix;
    format.Radix = (UCHAR)(MaxSizeUnit != ULONG_MAX ? MaxSizeUnit : PhMaxSizeUnit);
    format.u.Size = Size;

    return PhFormatToBuffer(&format, 1, Buffer, BufferLength, ReturnLength);
}

/**
 * Converts a UUID to its string representation.
 *
 * \param Guid A UUID.
 */
PPH_STRING PhFormatGuid(
    _In_ PGUID Guid
    )
{
    PPH_STRING string;
    UNICODE_STRING unicodeString;

    if (!NT_SUCCESS(RtlStringFromGUID(Guid, &unicodeString)))
        return NULL;

    string = PhCreateStringFromUnicodeString(&unicodeString);
    RtlFreeUnicodeString(&unicodeString);

    return string;
}

/**
 * Converts a UUID to its string representation.
 *
 * \param Guid A UUID.
 * \param Buffer A buffer. If NULL, no data is written.
 * \param BufferLength The number of bytes available in Buffer, including space for the null terminator.
 * \param ReturnLength The number of bytes required to hold the string, including the null terminator.
 */
NTSTATUS PhFormatGuidToBuffer(
    _In_ PGUID Guid,
    _Writable_bytes_(BufferLength) _When_(BufferLength != 0, _Notnull_) PWCHAR Buffer,
    _In_opt_ USHORT BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&RtlStringFromGUIDEx) RtlStringFromGUIDEx_I = NULL;
    NTSTATUS status;
    UNICODE_STRING unicodeString;

    if (WindowsVersion < WINDOWS_10)
    {
        PPH_STRING guid = PhFormatGuid(Guid);

        if (BufferLength < guid->Length)
        {
            if (ReturnLength)
                *ReturnLength = guid->Length + sizeof(UNICODE_NULL);
            PhDereferenceObject(guid);
            return STATUS_BUFFER_TOO_SMALL;
        }

        memcpy(Buffer, guid->Buffer, BufferLength);

        if (ReturnLength)
            *ReturnLength = guid->Length + sizeof(UNICODE_NULL);
        PhDereferenceObject(guid);
        return STATUS_SUCCESS;
    }

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_10)
        {
            RtlStringFromGUIDEx_I = PhGetDllProcedureAddress(RtlNtdllName, "RtlStringFromGUIDEx", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!RtlStringFromGUIDEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    RtlInitEmptyUnicodeString(&unicodeString, Buffer, BufferLength);

    status = RtlStringFromGUIDEx_I(
        Guid,
        &unicodeString,
        FALSE
        );

    if (NT_SUCCESS(status))
    {
        if (ReturnLength)
        {
            *ReturnLength = unicodeString.Length + sizeof(UNICODE_NULL);
        }
    }

    return status;
}

NTSTATUS PhStringToGuid(
    _In_ PCPH_STRINGREF GuidString,
    _Out_ PGUID Guid
    )
{
    UNICODE_STRING unicodeString;

    if (!PhStringRefToUnicodeString(GuidString, &unicodeString))
        return STATUS_BUFFER_OVERFLOW;

    return RtlGUIDFromString(&unicodeString, Guid);
}

/**
 * Retrieves image version information for a file.
 *
 * \param FileName The file name.
 * \param VersionInfo A version information block. You must free this using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetFileVersionInfo(
    _In_ PCWSTR FileName,
    _Out_ PVOID* VersionInfo
    )
{
    NTSTATUS status;
    PVOID libraryModule;
    PVOID versionInfo;

    libraryModule = LoadLibraryEx(
        FileName,
        NULL,
        LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE
        );

    if (!libraryModule)
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }

    status = PhLoadResourceCopy(
        libraryModule,
        MAKEINTRESOURCE(VS_VERSION_INFO),
        VS_FILE_INFO,
        NULL,
        &versionInfo
        );

    if (NT_SUCCESS(status))
    {
        if (PhIsFileVersionInfo32(versionInfo))
        {
            PhFreeLibrary(libraryModule);
            *VersionInfo = versionInfo;
            return STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }

        PhFree(versionInfo);
    }

    PhFreeLibrary(libraryModule);

    return status;
}

NTSTATUS PhGetFileVersionInfoEx(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PVOID* VersionInfo
    )
{
    NTSTATUS status;
    PVOID imageBaseAddress;
    PVOID versionInfo;

    status = PhLoadLibraryAsImageResource(FileName, TRUE, &imageBaseAddress);

    if (!NT_SUCCESS(status))
        return status;

    //if (LdrResFindResource_Import())
    //{
    //    PVOID resourceBuffer = NULL;
    //    SIZE_T resourceLength = 0;
    //
    //    if (NT_SUCCESS(LdrResFindResource_Import()(
    //        imageBaseAddress,
    //        VS_FILE_INFO,
    //        MAKEINTRESOURCE(VS_VERSION_INFO),
    //        MAKEINTRESOURCE(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)),
    //        &resourceBuffer,
    //        &resourceLength,
    //        NULL,
    //        NULL,
    //        0
    //        )))
    //    {
    //        *VersionInfo = PhAllocateCopy(resourceBuffer, resourceLength);
    //        PhFreeLibraryAsImageResource(imageBaseAddress);
    //        return STATUS_SUCCESS;
    //    }
    //}

    // MUI version information
    if (PRIMARYLANGID(LANGIDFROMLCID(PhGetCurrentThreadLCID())) != LANG_ENGLISH)
    {
        PVOID resourceDllBase;
        SIZE_T resourceDllSize;

        if (NT_SUCCESS(LdrLoadAlternateResourceModule(imageBaseAddress, &resourceDllBase, &resourceDllSize, 0)))
        {
            if (NT_SUCCESS(PhLoadResourceCopy(
                resourceDllBase,
                MAKEINTRESOURCE(VS_VERSION_INFO),
                VS_FILE_INFO,
                NULL,
                &versionInfo
                )))
            {
                if (PhIsFileVersionInfo32(versionInfo))
                {
                    LdrUnloadAlternateResourceModule(resourceDllBase);
                    PhFreeLibraryAsImageResource(imageBaseAddress);
                    *VersionInfo = versionInfo;
                    return STATUS_SUCCESS;
                }

                PhFree(versionInfo);
            }

            LdrUnloadAlternateResourceModule(resourceDllBase);
        }
    }

    // EXE version information
    {
        status = PhLoadResourceCopy(
            imageBaseAddress,
            MAKEINTRESOURCE(VS_VERSION_INFO),
            VS_FILE_INFO,
            NULL,
            &versionInfo
            );

        if (NT_SUCCESS(status))
        {
            if (PhIsFileVersionInfo32(versionInfo))
            {
                PhFreeLibraryAsImageResource(imageBaseAddress);
                *VersionInfo = versionInfo;
                return STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_NOT_FOUND;
            }

            PhFree(versionInfo);
        }
    }

    PhFreeLibraryAsImageResource(imageBaseAddress);
    return status;
}

PVOID PhGetFileVersionInfoValue(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo
    )
{
    const PCWSTR keyOffset = VersionInfo->Key + PhCountStringZ(VersionInfo->Key) + 1;

    return PTR_ADD_OFFSET(VersionInfo, ALIGN_UP(PTR_SUB_OFFSET(keyOffset, VersionInfo), ULONG));
}

NTSTATUS PhGetFileVersionInfoKey(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo,
    _In_ SIZE_T KeyLength,
    _In_ PCWSTR Key,
    _Out_opt_ PVOID* Buffer
    )
{
    PVOID value;
    ULONG valueOffset;
    PVS_VERSION_INFO_STRUCT32 child;

    if (!(value = PhGetFileVersionInfoValue(VersionInfo)))
        return STATUS_NOT_FOUND;

    valueOffset = VersionInfo->ValueLength * (VersionInfo->Type ? sizeof(WCHAR) : sizeof(CHAR));
    child = PTR_ADD_OFFSET(value, ALIGN_UP(valueOffset, ULONG));

    while ((ULONG_PTR)child < (ULONG_PTR)PTR_ADD_OFFSET(VersionInfo, VersionInfo->Length))
    {
        if (_wcsnicmp(child->Key, Key, KeyLength) == 0 && child->Key[KeyLength] == UNICODE_NULL)
        {
            if (Buffer)
                *Buffer = child;

            return STATUS_SUCCESS;
        }

        if (child->Length == 0)
            break;

        child = PTR_ADD_OFFSET(child, ALIGN_UP(child->Length, ULONG));
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS PhGetFileVersionVarFileInfoValue(
    _In_ PVOID VersionInfo,
    _In_ PCPH_STRINGREF KeyName,
    _Out_opt_ PVOID* Buffer,
    _Out_opt_ PULONG BufferLength
    )
{
    static CONST PH_STRINGREF varfileBlockName = PH_STRINGREF_INIT(L"VarFileInfo");
    PVS_VERSION_INFO_STRUCT32 varfileBlockInfo;
    PVS_VERSION_INFO_STRUCT32 varfileBlockValue;
    NTSTATUS status;

    status = PhGetFileVersionInfoKey(
        VersionInfo,
        varfileBlockName.Length / sizeof(WCHAR),
        varfileBlockName.Buffer,
        &varfileBlockInfo
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetFileVersionInfoKey(
            varfileBlockInfo,
            KeyName->Length / sizeof(WCHAR),
            KeyName->Buffer,
            &varfileBlockValue
            );

        if (NT_SUCCESS(status))
        {
            if (BufferLength)
                *BufferLength = varfileBlockValue->ValueLength;
            if (Buffer)
                *Buffer = PhGetFileVersionInfoValue(varfileBlockValue);
        }
    }

    return status;
}

VS_FIXEDFILEINFO* PhGetFileVersionFixedInfo(
    _In_ PVOID VersionInfo
    )
{
    VS_FIXEDFILEINFO* fileInfo;

    fileInfo = PhGetFileVersionInfoValue(VersionInfo);

    if (fileInfo && fileInfo->dwSignature == VS_FFI_SIGNATURE)
    {
        return fileInfo;
    }
    else
    {
        return NULL;
    }
}

/**
 * Retrieves the language ID and code page used by a version information block.
 *
 * \param VersionInfo The version information block.
 */
ULONG PhGetFileVersionInfoLangCodePage(
    _In_ PVOID VersionInfo
    )
{
    PLANGANDCODEPAGE codePage;
    ULONG codePageLength;

    if (NT_SUCCESS(PhGetFileVersionVarFileInfoValueZ(VersionInfo, L"Translation", &codePage, &codePageLength)))
    {
        //for (ULONG i = 0; i < (codePageLength / sizeof(LANGANDCODEPAGE)); i++)
        return ((ULONG)codePage[0].Language << 16) + codePage[0].CodePage; // Combine the language ID and code page.
    }

    return (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 1252;
}

/**
 * Retrieves a string in a version information block.
 *
 * \param VersionInfo The version information block.
 * \param SubBlock The path to the sub-block.
 */
PPH_STRING PhGetFileVersionInfoString(
    _In_ PVOID VersionInfo,
    _In_ PCWSTR SubBlock
    )
{
    NTSTATUS status;
    PH_STRINGREF name;
    PWSTR buffer;
    ULONG length;

    PhInitializeStringRefLongHint(&name, SubBlock);

    status = PhGetFileVersionVarFileInfoValue(
        VersionInfo,
        &name,
        &buffer,
        &length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    return PhCreateStringZ2(buffer, length * sizeof(WCHAR));
}

/**
 * Retrieves a string in a version information block.
 *
 * \param VersionInfo The version information block.
 * \param LangCodePage The language ID and code page of the string.
 * \param KeyName The name of the string.
 */
PPH_STRING PhGetFileVersionInfoString2(
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage,
    _In_ PCPH_STRINGREF KeyName
    )
{
    static CONST PH_STRINGREF blockInfoName = PH_STRINGREF_INIT(L"StringFileInfo");
    PVS_VERSION_INFO_STRUCT32 blockStringInfo;
    PVS_VERSION_INFO_STRUCT32 blockLangInfo;
    PVS_VERSION_INFO_STRUCT32 stringNameBlockInfo;
    NTSTATUS status;
    PVOID stringNameBlockValue;
    SIZE_T returnLength;
    PH_FORMAT format[3];
    WCHAR langNameString[65];

    status = PhGetFileVersionInfoKey(
        VersionInfo,
        blockInfoName.Length / sizeof(WCHAR),
        blockInfoName.Buffer,
        &blockStringInfo
        );

    if (!NT_SUCCESS(status))
        return NULL;

    PhInitFormatX(&format[0], LangCodePage);
    format[0].Type |= FormatPadZeros | FormatUpperCase;
    format[0].Width = 8;

    if (!PhFormatToBuffer(format, 1, langNameString, sizeof(langNameString), &returnLength))
        return NULL;

    status = PhGetFileVersionInfoKey(
        blockStringInfo,
        returnLength / sizeof(WCHAR) - sizeof(ANSI_NULL),
        langNameString,
        &blockLangInfo
        );

    if (!NT_SUCCESS(status))
        return NULL;

    status = PhGetFileVersionInfoKey(
        blockLangInfo,
        KeyName->Length / sizeof(WCHAR),
        KeyName->Buffer,
        &stringNameBlockInfo
        );

    if (!NT_SUCCESS(status))
        return NULL;

    if (!(stringNameBlockValue = PhGetFileVersionInfoValue(stringNameBlockInfo)))
        return NULL;

    return PhCreateStringZ2(
        stringNameBlockValue,
        stringNameBlockInfo->ValueLength * sizeof(WCHAR)
        );
}

PPH_STRING PhGetFileVersionInfoStringEx(
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage,
    _In_ PCPH_STRINGREF KeyName
    )
{
    PPH_STRING string;

    // Use the default language code page.
    if (string = PhGetFileVersionInfoString2(VersionInfo, LangCodePage, KeyName))
        return string;

    // Use the windows-1252 code page.
    if (string = PhGetFileVersionInfoString2(VersionInfo, (LangCodePage & 0xffff0000) + 1252, KeyName))
        return string;

    // Use the UTF-16 code page.
    if (string = PhGetFileVersionInfoString2(VersionInfo, (LangCodePage & 0xffff0000) + 1200, KeyName))
        return string;

    // Use the default language (US English).
    if (string = PhGetFileVersionInfoString2(VersionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 1252, KeyName))
        return string;

    // Use the default language (US English).
    if (string = PhGetFileVersionInfoString2(VersionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 1200, KeyName))
        return string;

    // Use the default language (US English).
    if (string = PhGetFileVersionInfoString2(VersionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 0, KeyName))
        return string;

    return NULL;
}

VOID PhpGetImageVersionInfoFields(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage
    )
{
    static CONST PH_STRINGREF companyName = PH_STRINGREF_INIT(L"CompanyName");
    static CONST PH_STRINGREF fileDescription = PH_STRINGREF_INIT(L"FileDescription");
    static CONST PH_STRINGREF productName = PH_STRINGREF_INIT(L"ProductName");

    ImageVersionInfo->CompanyName = PhGetFileVersionInfoStringEx(VersionInfo, LangCodePage, &companyName);
    ImageVersionInfo->FileDescription = PhGetFileVersionInfoStringEx(VersionInfo, LangCodePage, &fileDescription);
    ImageVersionInfo->ProductName = PhGetFileVersionInfoStringEx(VersionInfo, LangCodePage, &productName);
}

VOID PhpGetImageVersionVersionString(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PVOID VersionInfo
    )
{
    VS_FIXEDFILEINFO* rootBlock;

    // The version information is language-independent and must be read from the root block.
    if (rootBlock = PhGetFileVersionFixedInfo(VersionInfo))
    {
        PH_FORMAT fileVersionFormat[7];

        PhInitFormatU(&fileVersionFormat[0], HIWORD(rootBlock->dwFileVersionMS));
        PhInitFormatC(&fileVersionFormat[1], L'.');
        PhInitFormatU(&fileVersionFormat[2], LOWORD(rootBlock->dwFileVersionMS));
        PhInitFormatC(&fileVersionFormat[3], L'.');
        PhInitFormatU(&fileVersionFormat[4], HIWORD(rootBlock->dwFileVersionLS));
        PhInitFormatC(&fileVersionFormat[5], L'.');
        PhInitFormatU(&fileVersionFormat[6], LOWORD(rootBlock->dwFileVersionLS));

        ImageVersionInfo->FileVersion = PhFormat(fileVersionFormat, RTL_NUMBER_OF(fileVersionFormat), 64);
    }
    else
    {
        ImageVersionInfo->FileVersion = NULL;
    }
}

VOID PhpGetImageVersionVersionStringEx(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage
    )
{
    static CONST PH_STRINGREF fileVersion = PH_STRINGREF_INIT(L"FileVersion");
    VS_FIXEDFILEINFO* rootBlock;
    PPH_STRING versionString;

    if (versionString = PhGetFileVersionInfoStringEx(VersionInfo, LangCodePage, &fileVersion))
    {
        ImageVersionInfo->FileVersion = versionString;
        return;
    }

    if (rootBlock = PhGetFileVersionFixedInfo(VersionInfo))
    {
        PH_FORMAT fileVersionFormat[7];

        PhInitFormatU(&fileVersionFormat[0], HIWORD(rootBlock->dwFileVersionMS));
        PhInitFormatC(&fileVersionFormat[1], L'.');
        PhInitFormatU(&fileVersionFormat[2], LOWORD(rootBlock->dwFileVersionMS));
        PhInitFormatC(&fileVersionFormat[3], L'.');
        PhInitFormatU(&fileVersionFormat[4], HIWORD(rootBlock->dwFileVersionLS));
        PhInitFormatC(&fileVersionFormat[5], L'.');
        PhInitFormatU(&fileVersionFormat[6], LOWORD(rootBlock->dwFileVersionLS));

        ImageVersionInfo->FileVersion = PhFormat(fileVersionFormat, RTL_NUMBER_OF(fileVersionFormat), 64);
    }
    else
    {
        ImageVersionInfo->FileVersion = NULL;
    }
}

/**
 * Initializes a structure with version information.
 *
 * \param ImageVersionInfo The version information structure.
 * \param FileName The file name of an image.
 */
NTSTATUS PhInitializeImageVersionInfo(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    PVOID versionInfo;
    ULONG langCodePage;

    status = PhGetFileVersionInfo(FileName, &versionInfo);

    if (!NT_SUCCESS(status))
        return status;

    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);
    PhpGetImageVersionVersionString(ImageVersionInfo, versionInfo);
    PhpGetImageVersionInfoFields(ImageVersionInfo, versionInfo, langCodePage);
    PhFree(versionInfo);

    return status;
}

NTSTATUS PhInitializeImageVersionInfoEx(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN ExtendedVersionInfo
    )
{
    NTSTATUS status;
    PVOID versionInfo;
    ULONG langCodePage;

    status = PhGetFileVersionInfoEx(FileName, &versionInfo);

    if (!NT_SUCCESS(status))
        return status;

    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);
    if (ExtendedVersionInfo)
        PhpGetImageVersionVersionStringEx(ImageVersionInfo, versionInfo, langCodePage);
    else
        PhpGetImageVersionVersionString(ImageVersionInfo, versionInfo);
    PhpGetImageVersionInfoFields(ImageVersionInfo, versionInfo, langCodePage);
    PhFree(versionInfo);

    return status;
}

/**
 * Frees a version information structure initialized by PhInitializeImageVersionInfo().
 *
 * \param ImageVersionInfo The version information structure.
 */
VOID PhDeleteImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo
    )
{
    if (ImageVersionInfo->CompanyName) PhDereferenceObject(ImageVersionInfo->CompanyName);
    if (ImageVersionInfo->FileDescription) PhDereferenceObject(ImageVersionInfo->FileDescription);
    if (ImageVersionInfo->FileVersion) PhDereferenceObject(ImageVersionInfo->FileVersion);
    if (ImageVersionInfo->ProductName) PhDereferenceObject(ImageVersionInfo->ProductName);
}

PPH_STRING PhFormatImageVersionInfo(
    _In_opt_ PPH_STRING FileName,
    _In_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_opt_ PCPH_STRINGREF Indent,
    _In_opt_ ULONG LineLimit
    )
{
    PH_STRING_BUILDER stringBuilder;

    if (LineLimit == 0)
        LineLimit = ULONG_MAX;

    PhInitializeStringBuilder(&stringBuilder, 40);

    // File name

    if (!PhIsNullOrEmptyString(FileName))
    {
        PPH_STRING temp;

        if (Indent) PhAppendStringBuilder(&stringBuilder, Indent);

        temp = PhEllipsisStringPath(FileName, LineLimit);
        PhAppendStringBuilder(&stringBuilder, &temp->sr);
        PhDereferenceObject(temp);
        PhAppendCharStringBuilder(&stringBuilder, L'\n');
    }

    // File description & version

    if (!(
        PhIsNullOrEmptyString(ImageVersionInfo->FileDescription) &&
        PhIsNullOrEmptyString(ImageVersionInfo->FileVersion)
        ))
    {
        PPH_STRING tempDescription = NULL;
        PPH_STRING tempVersion = NULL;
        ULONG limitForDescription;
        ULONG limitForVersion;

        if (LineLimit != ULONG_MAX)
        {
            limitForVersion = (LineLimit - 1) / 4; // 1/4 space for version (and space character)
            limitForDescription = LineLimit - limitForVersion;
        }
        else
        {
            limitForDescription = ULONG_MAX;
            limitForVersion = ULONG_MAX;
        }

        if (!PhIsNullOrEmptyString(ImageVersionInfo->FileDescription))
        {
            tempDescription = PhEllipsisString(
                ImageVersionInfo->FileDescription,
                limitForDescription
                );
        }

        if (!PhIsNullOrEmptyString(ImageVersionInfo->FileVersion))
        {
            tempVersion = PhEllipsisString(
                ImageVersionInfo->FileVersion,
                limitForVersion
                );
        }

        if (Indent) PhAppendStringBuilder(&stringBuilder, Indent);

        if (tempDescription)
        {
            PhAppendStringBuilder(&stringBuilder, &tempDescription->sr);

            if (tempVersion)
                PhAppendCharStringBuilder(&stringBuilder, L' ');
        }

        if (tempVersion)
            PhAppendStringBuilder(&stringBuilder, &tempVersion->sr);

        if (tempDescription)
            PhDereferenceObject(tempDescription);
        if (tempVersion)
            PhDereferenceObject(tempVersion);

        PhAppendCharStringBuilder(&stringBuilder, L'\n');
    }

    // File company

    if (!PhIsNullOrEmptyString(ImageVersionInfo->CompanyName))
    {
        PPH_STRING temp;

        if (Indent) PhAppendStringBuilder(&stringBuilder, Indent);

        temp = PhEllipsisString(ImageVersionInfo->CompanyName, LineLimit);
        PhAppendStringBuilder(&stringBuilder, &temp->sr);
        PhDereferenceObject(temp);
        PhAppendCharStringBuilder(&stringBuilder, L'\n');
    }

    // Remove the extra newline.
    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

typedef struct _PH_FILE_VERSIONINFO_CACHE_ENTRY
{
    PPH_STRING FileName;
    PPH_STRING CompanyName;
    PPH_STRING FileDescription;
    PPH_STRING FileVersion;
    PPH_STRING ProductName;
} PH_FILE_VERSIONINFO_CACHE_ENTRY, *PPH_FILE_VERSIONINFO_CACHE_ENTRY;

static PPH_HASHTABLE PhpImageVersionInfoCacheHashtable = NULL;
static PH_QUEUED_LOCK PhpImageVersionInfoCacheLock = PH_QUEUED_LOCK_INIT;

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN PhpImageVersionInfoCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry1 = Entry1;
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry2 = Entry2;

    return PhEqualString(entry1->FileName, entry2->FileName, FALSE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG PhpImageVersionInfoCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry = Entry;

    return PhHashStringRefEx(&entry->FileName->sr, FALSE, PH_STRING_HASH_XXH32);
}

NTSTATUS PhInitializeImageVersionInfoCached(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN IsSubsystemProcess,
    _In_ BOOLEAN ExtendedVersion
    )
{
    PH_IMAGE_VERSION_INFO versionInfo = { 0 };
    PH_FILE_VERSIONINFO_CACHE_ENTRY newEntry;

    if (PhpImageVersionInfoCacheHashtable)
    {
        PPH_FILE_VERSIONINFO_CACHE_ENTRY entry;
        PH_FILE_VERSIONINFO_CACHE_ENTRY lookupEntry;

        lookupEntry.FileName = FileName;

        PhAcquireQueuedLockShared(&PhpImageVersionInfoCacheLock);
        entry = PhFindEntryHashtable(PhpImageVersionInfoCacheHashtable, &lookupEntry);
        PhReleaseQueuedLockShared(&PhpImageVersionInfoCacheLock);

        if (entry)
        {
            PhSetReference(&ImageVersionInfo->CompanyName, entry->CompanyName);
            PhSetReference(&ImageVersionInfo->FileDescription, entry->FileDescription);
            PhSetReference(&ImageVersionInfo->FileVersion, entry->FileVersion);
            PhSetReference(&ImageVersionInfo->ProductName, entry->ProductName);
            return STATUS_SUCCESS;
        }
    }

    if (IsSubsystemProcess)
    {
        if (!NT_SUCCESS(PhInitializeLxssImageVersionInfo(&versionInfo, &FileName->sr)))
        {
            // Note: If the function fails the version info is empty. For extra performance
            // we still update the cache with a reference to the filename (without version info)
            // and just return the empty result from the cache instead of loading the same file again.
            // We flush every few hours so the cache doesn't waste memory or become state. (dmex)
        }
    }
    else
    {
        if (!NT_SUCCESS(PhInitializeImageVersionInfoEx(&versionInfo, &FileName->sr, ExtendedVersion)))
        {
            // Note: If the function fails the version info is empty. For extra performance
            // we still update the cache with a reference to the filename (without version info)
            // and just return the empty result from the cache instead of loading the same file again.
            // We flush every few hours so the cache doesn't waste memory or become state. (dmex)
        }
    }

    if (!PhpImageVersionInfoCacheHashtable)
    {
        PhpImageVersionInfoCacheHashtable = PhCreateHashtable(
            sizeof(PH_FILE_VERSIONINFO_CACHE_ENTRY),
            PhpImageVersionInfoCacheHashtableEqualFunction,
            PhpImageVersionInfoCacheHashtableHashFunction,
            100
            );
    }

    PhSetReference(&newEntry.FileName, FileName);
    PhSetReference(&newEntry.CompanyName, versionInfo.CompanyName);
    PhSetReference(&newEntry.FileDescription, versionInfo.FileDescription);
    PhSetReference(&newEntry.FileVersion, versionInfo.FileVersion);
    PhSetReference(&newEntry.ProductName, versionInfo.ProductName);

    PhAcquireQueuedLockExclusive(&PhpImageVersionInfoCacheLock);
    PhAddEntryHashtable(PhpImageVersionInfoCacheHashtable, &newEntry);
    PhReleaseQueuedLockExclusive(&PhpImageVersionInfoCacheLock);

    ImageVersionInfo->CompanyName = versionInfo.CompanyName;
    ImageVersionInfo->FileDescription = versionInfo.FileDescription;
    ImageVersionInfo->FileVersion = versionInfo.FileVersion;
    ImageVersionInfo->ProductName = versionInfo.ProductName;
    return STATUS_SUCCESS;
}

VOID PhFlushImageVersionInfoCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry;

    if (!PhpImageVersionInfoCacheHashtable)
        return;

    PhAcquireQueuedLockExclusive(&PhpImageVersionInfoCacheLock);

    PhBeginEnumHashtable(PhpImageVersionInfoCacheHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if (entry->FileName) PhDereferenceObject(entry->FileName);
        if (entry->CompanyName) PhDereferenceObject(entry->CompanyName);
        if (entry->FileDescription) PhDereferenceObject(entry->FileDescription);
        if (entry->FileVersion) PhDereferenceObject(entry->FileVersion);
        if (entry->ProductName) PhDereferenceObject(entry->ProductName);
    }

    PhClearReference(&PhpImageVersionInfoCacheHashtable);

    PhpImageVersionInfoCacheHashtable = PhCreateHashtable(
        sizeof(PH_FILE_VERSIONINFO_CACHE_ENTRY),
        PhpImageVersionInfoCacheHashtableEqualFunction,
        PhpImageVersionInfoCacheHashtableHashFunction,
        100
        );

    PhReleaseQueuedLockExclusive(&PhpImageVersionInfoCacheLock);
}

/**
 * Retrieves the full path and file name of the specified file.
 *
 * \param FileName The name of the file.
 * \param BufferLength The size of the buffer to receive the null-terminated string for the drive and path.
 * \param Buffer A pointer to a buffer that receives the null-terminated string for the drive and path.
 * \param FilePart A variable which receives the index of the base name.
 * \param BytesRequired The length of the string including the terminating null character.
 * \return Successful or errant status.
 */
NTSTATUS PhGetFullPathName(
    _In_ PCWSTR FileName,
    _In_ SIZE_T BufferLength,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _Out_opt_ PWSTR* FilePart,
    _Out_opt_ PULONG BytesRequired
    )
{
    NTSTATUS status;
    ULONG bufferLength;
    ULONG bytesRequired;

    status = RtlSizeTToULong(
        BufferLength,
        &bufferLength
        );

    if (!NT_SUCCESS(status))
        return status;

    bytesRequired = 0;
    status = RtlGetFullPathName_UEx(
        FileName,
        bufferLength, // * sizeof(WCHAR)
        Buffer,
        FilePart,
        &bytesRequired
        );

    if (!NT_SUCCESS(status))
        return status;

    if (BytesRequired)
    {
        *BytesRequired = bytesRequired; /* / sizeof(WCHAR) */
    }

    if (BufferLength < bytesRequired)
        return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

/**
 * Gets an absolute file name.
 *
 * \param FileName A file name.
 * \param FullPath An absolute file name, or NULL if the function failed.
 * \param IndexOfFileName A variable which receives the index of the base name.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetFullPath(
    _In_ PCWSTR FileName,
    _Out_ PPH_STRING *FullPath,
    _Out_opt_ PULONG IndexOfFileName
    )
{
    NTSTATUS status;
    PPH_STRING fullPath;
    ULONG fullPathLength;
    ULONG returnLength = 0;
    PWSTR filePart = NULL;

    fullPathLength = DOS_MAX_PATH_LENGTH;
    fullPath = PhCreateStringEx(NULL, fullPathLength);

    status = PhGetFullPathName(
        FileName,
        fullPath->Length + sizeof(UNICODE_NULL),
        fullPath->Buffer,
        &filePart,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL && returnLength > sizeof(UNICODE_NULL))
    {
        PhDereferenceObject(fullPath);
        fullPathLength = returnLength - sizeof(UNICODE_NULL);
        fullPath = PhCreateStringEx(NULL, fullPathLength);

        status = PhGetFullPathName(
            FileName,
            fullPath->Length + sizeof(UNICODE_NULL),
            fullPath->Buffer,
            &filePart,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(fullPath);
        return status;
    }

    PhTrimToNullTerminatorString(fullPath);
    *FullPath = fullPath;

    if (IndexOfFileName)
    {
        if (filePart)
        {
            // The path points to a file.
            *IndexOfFileName = (ULONG)(filePart - fullPath->Buffer);
        }
        else
        {
            // The path points to a directory.
            *IndexOfFileName = ULONG_MAX;
        }
    }

    return status;
}

/**
 * Expands environment variables in a string.
 *
 * \param String The string.
 */
PPH_STRING PhExpandEnvironmentStrings(
    _In_ PCPH_STRINGREF String
    )
{
    NTSTATUS status;
    UNICODE_STRING inputString;
    UNICODE_STRING outputString;
    PPH_STRING string;
    ULONG bufferLength;

    if (!PhStringRefToUnicodeString(String, &inputString))
        return NULL;

    bufferLength = 0x100;
    string = PhCreateStringEx(NULL, bufferLength);
    outputString.MaximumLength = (USHORT)bufferLength;
    outputString.Length = 0;
    outputString.Buffer = string->Buffer;

    status = RtlExpandEnvironmentStrings_U(
        NULL,
        &inputString,
        &outputString,
        &bufferLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhDereferenceObject(string);
        string = PhCreateStringEx(NULL, bufferLength);
        outputString.MaximumLength = (USHORT)bufferLength;
        outputString.Length = 0;
        outputString.Buffer = string->Buffer;

        status = RtlExpandEnvironmentStrings_U(
            NULL,
            &inputString,
            &outputString,
            &bufferLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    string->Length = outputString.Length;
    string->Buffer[string->Length / sizeof(WCHAR)] = UNICODE_NULL; // make sure there is a null terminator

    return string;
}

/**
 * Gets the base name from a file name.
 *
 * \param FileName The file name.
 */
PPH_STRING PhGetBaseName(
    _In_ PPH_STRING FileName
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF baseNamePart;

    if (!PhSplitStringRefAtLastChar(&FileName->sr, OBJ_NAME_PATH_SEPARATOR, &pathPart, &baseNamePart))
    {
        return PhReferenceObject(FileName);
    }

    return PhCreateString2(&baseNamePart);
}

/**
 * Changes the extension of the base name in a file path.
 *
 * This function locates the last path separator and the last dot in the file name.
 * If both are found and the dot is after the last path separator, it replaces the extension
 * with the specified new extension. Otherwise, it appends the new extension to the file name.
 *
 * \param FileName The input file name as a string reference.
 * \param FileExtension The new file extension as a string reference.
 * \return A new string with the base name's extension changed, or the original file name with the extension appended if no dot is found.
 */
PPH_STRING PhGetBaseNameChangeExtension(
    _In_ PCPH_STRINGREF FileName,
    _In_ PCPH_STRINGREF FileExtension
    )
{
    ULONG_PTR indexOfBackslash;
    ULONG_PTR indexOfLastDot;
    PH_STRINGREF baseFileName;
    PH_STRINGREF baseFilePath;

    if ((indexOfBackslash = PhFindLastCharInStringRef(FileName, OBJ_NAME_PATH_SEPARATOR, FALSE)) == SIZE_MAX)
        return PhConcatStringRef2(FileName, FileExtension);
    if ((indexOfLastDot = PhFindLastCharInStringRef(FileName, L'.', FALSE)) == SIZE_MAX)
        return PhConcatStringRef2(FileName, FileExtension);
    if (indexOfLastDot < indexOfBackslash)
        return PhConcatStringRef2(FileName, FileExtension);

    baseFileName.Buffer = FileName->Buffer + indexOfBackslash + 1;
    baseFileName.Length = (indexOfLastDot - indexOfBackslash - 1) * sizeof(WCHAR);
    baseFilePath.Buffer = FileName->Buffer;
    baseFilePath.Length = (indexOfBackslash + 1) * sizeof(WCHAR);

    return PhConcatStringRef3(&baseFilePath, &baseFileName, FileExtension);
}

/**
 * Splits a file path into its base path and base file name components.
 *
 * This function separates the input file name into the directory path and the file name.
 * If both output pointers are provided, both components are set if available.
 * If only one output pointer is provided, only that component is set.
 *
 * \param FileName The input file name as a string reference.
 * \param BasePathName Optional pointer to receive the base path component.
 * \param BaseFileName Optional pointer to receive the base file name component.
 * \return TRUE if the split was successful and the requested components are available, otherwise FALSE.
 */
_Success_(return)
BOOLEAN PhGetBasePath(
    _In_ PCPH_STRINGREF FileName,
    _Out_opt_ PPH_STRINGREF BasePathName,
    _Out_opt_ PPH_STRINGREF BaseFileName
    )
{
    PH_STRINGREF basePathPart;
    PH_STRINGREF baseNamePart;

    if (!PhSplitStringRefAtLastChar(FileName, OBJ_NAME_PATH_SEPARATOR, &basePathPart, &baseNamePart))
        return FALSE;

    if (BasePathName && BaseFileName)
    {
        if (basePathPart.Length && baseNamePart.Length)
        {
            BasePathName->Length = basePathPart.Length;
            BasePathName->Buffer = basePathPart.Buffer;

            BaseFileName->Length = baseNamePart.Length;
            BaseFileName->Buffer = baseNamePart.Buffer;
            return TRUE;
        }

        return FALSE;
    }

    if (BasePathName && basePathPart.Length)
    {
        BasePathName->Length = basePathPart.Length;
        BasePathName->Buffer = basePathPart.Buffer;
        return TRUE;
    }

    if (BaseFileName && baseNamePart.Length)
    {
        BaseFileName->Length = baseNamePart.Length;
        BaseFileName->Buffer = baseNamePart.Buffer;
        return TRUE;
    }

    return FALSE;
}

/**
 * Gets the parent directory from a file name.
 *
 * \param FileName The file name.
 */
PPH_STRING PhGetBaseDirectory(
    _In_ PPH_STRING FileName
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF baseNamePart;

    if (!PhSplitStringRefAtLastChar(&FileName->sr, OBJ_NAME_PATH_SEPARATOR, &pathPart, &baseNamePart))
        return NULL;

    return PhCreateString2(&pathPart);
}

/**
 * Retrieves the system directory path.
 */
PPH_STRING PhGetSystemDirectory(
    VOID
    )
{
    static CONST PH_STRINGREF system32String = PH_STRINGREF_INIT(L"\\System32");
    static PPH_STRING cachedSystemDirectory = NULL;
    PH_STRINGREF systemRootString;
    PPH_STRING systemDirectory;
    PPH_STRING previousSystemDirectory;

    // Use the cached value if possible.

    if (systemDirectory = ReadPointerAcquire(&cachedSystemDirectory))
        return PhReferenceObject(systemDirectory);

    PhGetSystemRoot(&systemRootString);
    systemDirectory = PhConcatStringRef2(&systemRootString, &system32String);

    PhReferenceObject(systemDirectory);

    if (previousSystemDirectory = InterlockedExchangePointer(&cachedSystemDirectory, systemDirectory))
    {
        PhDereferenceObject(previousSystemDirectory);
    }

    return systemDirectory;
}

/**
 * Retrieves the path to the Windows System32 directory in Win32 format, optionally appending a subpath.
 *
 * \param AppendPath Optional string to append to the System32 directory path.
 * \return A pointer to a string containing the full path to the System32 directory (with optional appended path).
 */
PPH_STRING PhGetSystemDirectoryWin32(
    _In_opt_ PCPH_STRINGREF AppendPath
    )
{
    static CONST PH_STRINGREF system32String = PH_STRINGREF_INIT(L"\\System32");
    PH_STRINGREF systemRootString;

    PhGetSystemRoot(&systemRootString);

    if (AppendPath)
    {
        return PhConcatStringRef3(&systemRootString, &system32String, AppendPath);
    }

    return PhConcatStringRef2(&systemRootString, &system32String);
}

/**
 * Retrieves the Windows system root directory path.
 *
 * This function returns the system root directory path as a PH_STRINGREF.
 * The value is cached after the first call for subsequent use.
 * The returned string does not include a trailing backslash.
 */
VOID PhGetSystemRoot(
    _Out_ PPH_STRINGREF SystemRoot
    )
{
    static PH_STRINGREF systemRoot;
    PH_STRINGREF localSystemRoot;
    SIZE_T count;

    if (systemRoot.Buffer)
    {
        *SystemRoot = systemRoot;
        return;
    }

    localSystemRoot.Buffer = RtlGetNtSystemRoot();
    count = PhCountStringZ(localSystemRoot.Buffer);
    localSystemRoot.Length = count * sizeof(WCHAR);

    // Make sure the system root string doesn't have a trailing backslash.
    if (localSystemRoot.Buffer[count - 1] == OBJ_NAME_PATH_SEPARATOR)
        localSystemRoot.Length -= sizeof(WCHAR);

    *SystemRoot = localSystemRoot;

    systemRoot.Length = localSystemRoot.Length;
    MemoryBarrier();
    systemRoot.Buffer = localSystemRoot.Buffer;
}

/**
 * Retrieves the native NT path of the Windows system root directory.
 *
 * This function returns the system root directory path in native NT format as a PH_STRINGREF.
 * The value is cached after the first call for subsequent use.
 */
VOID PhGetNtSystemRoot(
    _Out_ PPH_STRINGREF NtSystemRoot
    )
{
    static PPH_STRING systemRootNative;

    if (PhIsNullOrEmptyString(systemRootNative))
    {
        PH_STRINGREF localSystemRoot;

        PhGetSystemRoot(&localSystemRoot);
        systemRootNative = PhDosPathNameToNtPathName(&localSystemRoot);
    }

    *NtSystemRoot = systemRootNative->sr;
}

/**
 * Retrieves the Windows directory path.
 */
PPH_STRING PhGetWindowsDirectory(
    _In_opt_ PCPH_STRINGREF AppendPath
    )
{
    PH_STRINGREF systemRoot;
    PPH_STRING systemRootNative;

    PhGetSystemRoot(&systemRoot);
    systemRootNative = PhDosPathNameToNtPathName(&systemRoot);

    if (systemRootNative && AppendPath)
    {
        PhMoveReference(&systemRootNative, PhConcatStringRef2(&systemRootNative->sr, AppendPath));
    }

    return systemRootNative;
}

/**
 * Retrieves the Windows directory path.
 */
PPH_STRING PhGetWindowsDirectoryWin32(
    _In_opt_ PCPH_STRINGREF AppendPath
    )
{
    PH_STRINGREF systemRoot;

    PhGetSystemRoot(&systemRoot);

    if (AppendPath)
    {
        return PhConcatStringRef2(&systemRoot, AppendPath);
    }

    return PhCreateString2(&systemRoot);
}

/**
 * Retrieves the native file name of the current process image.
 */
PPH_STRING PhGetApplicationFileName(
    VOID
    )
{
    static PPH_STRING cachedFileName = NULL;
    PPH_STRING fileName;

    if (fileName = ReadPointerAcquire(&cachedFileName))
    {
        return PhReferenceObject(fileName);
    }

    if (!NT_SUCCESS(PhGetProcessImageFileName(NtCurrentProcess(), &fileName)))
    {
        if (!NT_SUCCESS(PhGetProcessImageFileNameByProcessId(NtCurrentProcessId(), &fileName)))
        {
            if (!NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), NtCurrentImageBase(), &fileName)))
            {
                if (fileName = PhGetDllFileName(NtCurrentImageBase(), NULL))
                {
                    PPH_STRING fullPath;

                    if (NT_SUCCESS(PhGetFullPath(PhGetString(fileName), &fullPath, NULL)))
                    {
                        PhMoveReference(&fileName, fullPath);
                    }

                    PhMoveReference(&fileName, PhDosPathNameToNtPathName(&fileName->sr));
                }
            }
        }
    }

    // Update the cached file name with atomic semantics (dmex)
    if (!InterlockedCompareExchangePointer(
        &cachedFileName,
        fileName,
        NULL
        ))
    {
        PhReferenceObject(fileName);
    }

    return fileName;
}

/**
 * Retrieves the file name of the current process image.
 */
PPH_STRING PhGetApplicationFileNameWin32(
    VOID
    )
{
    static PPH_STRING cachedFileName = NULL;
    PPH_STRING fileName;
    PPH_STRING string;

    if (fileName = ReadPointerAcquire(&cachedFileName))
    {
        return PhReferenceObject(fileName);
    }

    if (fileName = PhGetDllFileName(NtCurrentImageBase(), NULL))
    {
        if (NT_SUCCESS(PhGetFullPath(PhGetString(fileName), &string, NULL)))
        {
            PhMoveReference(&fileName, string);
        }
    }

    //if (NT_SUCCESS(PhGetProcessImageFileNameWin32(NtCurrentProcess(), &fileName)))
    //    PhMoveReference(&fileName, PhGetFileName(fileName));
    //
    //if (!NT_SUCCESS(PhGetProcessImageFileNameWin32(NtCurrentProcess(), &fileName)))
    //{
    //    if (!NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), PhInstanceHandle, &fileName)))
    //    {
    //        if (!NT_SUCCESS(PhGetProcessImageFileNameByProcessId(NtCurrentProcessId(), &fileName)))
    //        {
    //            PhMoveReference(&fileName, PhGetFileName(fileName));
    //        }
    //    }

    if (!InterlockedCompareExchangePointer(
        &cachedFileName,
        fileName,
        NULL
        ))
    {
        PhReferenceObject(fileName);
    }

    return fileName;
}

/**
 * Retrieves the directory path of the current process image (native NT path).
 *
 * This function returns the directory containing the executable image of the current process.
 * The result is cached for subsequent calls to improve performance.
 * \return A pointer to a string containing the directory path.
 */
PPH_STRING PhGetApplicationDirectory(
    VOID
    )
{
    static PPH_STRING cachedDirectoryPath = NULL;
    PPH_STRING directoryPath;
    PPH_STRING fileName;

    // Read the cached directory path with acquire semantics
    if (directoryPath = ReadPointerAcquire(&cachedDirectoryPath))
    {
        return PhReferenceObject(directoryPath);
    }

    // Get the application file name
    if (fileName = PhGetApplicationFileName())
    {
        ULONG_PTR indexOfFileName;

        // Find the last path separator in the file name
        indexOfFileName = PhFindLastCharInString(fileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfFileName != SIZE_MAX)
            indexOfFileName++;
        else
            indexOfFileName = 0;

        // Extract the directory path from the file name
        if (indexOfFileName != 0)
        {
            directoryPath = PhSubstring(fileName, 0, indexOfFileName);
        }

        PhDereferenceObject(fileName);
    }

    // Atomically set the cached directory path if it is currently NULL
    if (!InterlockedCompareExchangePointer(
        &cachedDirectoryPath,
        directoryPath,
        NULL
        ))
    {
        PhReferenceObject(directoryPath);
    }

    return directoryPath;
}

/**
 * Retrieves the directory path of the current process image (Win32 path).
 *
 * This function returns the directory containing the executable image of the current process,
 * using the Win32 path format. The result is cached for subsequent calls to improve performance.
 * \return A pointer to a string containing the directory path.
 */
PPH_STRING PhGetApplicationDirectoryWin32(
    VOID
    )
{
    static PPH_STRING cachedDirectoryPath = NULL;
    PPH_STRING directoryPath;
    PPH_STRING fileName;

    if (directoryPath = ReadPointerAcquire(&cachedDirectoryPath))
    {
        return PhReferenceObject(directoryPath);
    }

    if (fileName = PhGetApplicationFileNameWin32())
    {
        ULONG_PTR indexOfFileName;

        indexOfFileName = PhFindLastCharInString(fileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfFileName != SIZE_MAX)
            indexOfFileName++;
        else
            indexOfFileName = 0;

        // Extract the directory path from the file name
        if (indexOfFileName != 0)
        {
            directoryPath = PhSubstring(fileName, 0, indexOfFileName);
        }

        PhDereferenceObject(fileName);
    }

    // Atomically set the cached directory path if it is currently NULL
    if (!InterlockedCompareExchangePointer(
        &cachedDirectoryPath,
        directoryPath,
        NULL
        ))
    {
        PhReferenceObject(directoryPath);
    }

    return directoryPath;
}

/**
 * Gets the full path to a file in the application's directory.
 *
 * \param FileName The file name to append to the application directory.
 * \param NativeFileName TRUE to use the native NT path, FALSE for Win32 path.
 * \return A pointer to a string containing the full path, or NULL if the directory could not be determined.
 */
PPH_STRING PhGetApplicationDirectoryFileName(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    PPH_STRING applicationFileName = NULL;
    PPH_STRING applicationDirectory;

    if (NativeFileName)
        applicationDirectory = PhGetApplicationDirectory();
    else
        applicationDirectory = PhGetApplicationDirectoryWin32();

    if (applicationDirectory)
    {
        applicationFileName = PhConcatStringRef2(&applicationDirectory->sr, FileName);
        PhDereferenceObject(applicationDirectory);
    }

    return applicationFileName;
}

/**
 * Gets a temporary file name in the system temporary directory, using a random alpha string.
 *
 * \return A pointer to a string containing the full path to the temporary file.
 */
PPH_STRING PhGetTemporaryDirectoryRandomAlphaFileName(
    VOID
    )
{
    PH_STRINGREF randomAlphaString;
    WCHAR randomAlphaStringBuffer[33] = L"";

    PhGenerateRandomAlphaString(randomAlphaStringBuffer, RTL_NUMBER_OF(randomAlphaStringBuffer));
    randomAlphaStringBuffer[0] = OBJ_NAME_PATH_SEPARATOR;
    randomAlphaString.Buffer = randomAlphaStringBuffer;
    randomAlphaString.Length = sizeof(randomAlphaStringBuffer) - sizeof(UNICODE_NULL);

    return PhGetTemporaryDirectory(&randomAlphaString);
}

/**
 * Gets the local application data directory for System Informer, optionally appending a file name.
 *
 * \param FileName The file name to append, or NULL to return the directory only.
 * \param NativeFileName TRUE to use the native NT path, FALSE for Win32 path.
 * \return A pointer to a string containing the full path, or NULL if the directory could not be determined.
 */
PPH_STRING PhGetLocalAppDataDirectory(
    _In_opt_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    PPH_STRING localAppDataFileName = NULL;
    PPH_STRING localAppDataDirectory;

    if (localAppDataDirectory = PhGetKnownLocationZ(PH_FOLDERID_LocalAppData, L"\\SystemInformer\\", NativeFileName))
    {
        if (!FileName) return localAppDataDirectory;
        localAppDataFileName = PhConcatStringRef2(&localAppDataDirectory->sr, FileName);
        PhReferenceObject(localAppDataDirectory);
    }

    return localAppDataFileName;
}

/**
 * Gets the roaming application data directory for System Informer, optionally appending a file name.
 *
 * \param FileName The file name to append, or NULL to return the directory only.
 * \param NativeFileName TRUE to use the native NT path, FALSE for Win32 path.
 * \return A pointer to a string containing the full path, or NULL if the directory could not be determined.
 */
PPH_STRING PhGetRoamingAppDataDirectory(
    _In_opt_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    PPH_STRING roamingAppDataFileName = NULL;
    PPH_STRING roamingAppDataDirectory;

    if (roamingAppDataDirectory = PhGetKnownLocationZ(PH_FOLDERID_RoamingAppData, L"\\SystemInformer\\", NativeFileName))
    {
        if (!FileName) return roamingAppDataDirectory;
        roamingAppDataFileName = PhConcatStringRef2(&roamingAppDataDirectory->sr, FileName);
        PhReferenceObject(roamingAppDataDirectory);
    }

    return roamingAppDataFileName;
}

/**
 * Gets the full path to a file in the application's directory.
 *
 * \param FileName The file name to append to the application directory.
 * \param NativeFileName TRUE to use the native NT path, FALSE for Win32 path.
 * \return A pointer to a string containing the full path, or NULL if the directory could not be determined.
 */
PPH_STRING PhGetApplicationDataFileName(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    PPH_STRING applicationDataFileName;

    // Check the current directory. (dmex)

    if (applicationDataFileName = PhGetApplicationDirectoryFileName(FileName, NativeFileName))
    {
        if (NativeFileName)
        {
            if (PhDoesFileExist(&applicationDataFileName->sr))
                return applicationDataFileName;
        }
        else
        {
            if (PhDoesFileExistWin32(PhGetString(applicationDataFileName)))
                return applicationDataFileName;
        }

        PhClearReference(&applicationDataFileName);
    }

    // Return the appdata directory. (dmex)

    applicationDataFileName = PhGetRoamingAppDataDirectory(FileName, NativeFileName);

    return applicationDataFileName;
}

#if defined(PH_SHGETFOLDERPATH)
/**
 * Gets a known location as a file name.
 *
 * \param Folder A CSIDL value representing the known location.
 * \param AppendPath A string to append to the folder path.
 */
//PPH_STRING PhGetKnownLocation(
//    _In_ ULONG Folder,
//    _In_opt_ PWSTR AppendPath
//    )
//{
//    PPH_STRING path;
//    SIZE_T appendPathLength;
//
//    if (!SHGetFolderPathW_Import())
//        return NULL;
//
//    if (AppendPath)
//        appendPathLength = PhCountStringZ(AppendPath) * sizeof(WCHAR);
//    else
//        appendPathLength = 0;
//
//    path = PhCreateStringEx(NULL, MAX_PATH * sizeof(WCHAR) + appendPathLength);
//
//    if (SUCCEEDED(SHGetFolderPathW_Import()(
//        NULL,
//        Folder,
//        NULL,
//        SHGFP_TYPE_CURRENT,
//        path->Buffer
//        )))
//    {
//        PhTrimToNullTerminatorString(path);
//
//        if (AppendPath)
//        {
//            memcpy(&path->Buffer[path->Length / sizeof(WCHAR)], AppendPath, appendPathLength + sizeof(UNICODE_NULL)); // +2 for null terminator
//            path->Length += appendPathLength;
//        }
//
//        return path;
//    }
//
//    PhDereferenceObject(path);
//
//    return NULL;
//}
#endif

/**
 * Retrieves the full path of a known folder.
 *
 * \param Folder A reference that identifies the folder.
 * \param AppendPath The string to append to the path.
 * \param NativeFileName A boolean to return the native path or Win32 path.
 */
PPH_STRING PhGetKnownLocation(
    _In_ ULONG Folder,
    _In_opt_ PCPH_STRINGREF AppendPath,
    _In_ BOOLEAN NativeFileName
    )
{
#if !defined(PH_BUILD_MSIX)
    switch (Folder)
    {
    case PH_FOLDERID_LocalAppData:
        {
            static CONST PH_STRINGREF variableName = PH_STRINGREF_INIT(L"LOCALAPPDATA");
            static CONST PH_STRINGREF variableNameProfile = PH_STRINGREF_INIT(L"USERPROFILE");
            NTSTATUS status;
            PH_STRINGREF variableValue;
            WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

            PhInitializeBufferStringRef(&variableValue, variableBuffer, sizeof(variableBuffer));

            status = PhQueryEnvironmentVariableStringRef(
                NULL,
                &variableName,
                &variableValue
                );

            if (!NT_SUCCESS(status))
            {
                status = PhQueryEnvironmentVariableStringRef(
                    NULL,
                    &variableNameProfile,
                    &variableValue
                    );
            }

            if (NT_SUCCESS(status))
            {
                PPH_STRING fileName;

                if (AppendPath)
                {
                    if (NativeFileName)
                    {
                        if (fileName = PhDosPathNameToNtPathName(&variableValue))
                        {
                            PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, AppendPath));
                        }
                        else
                        {
                            fileName = NULL;
                        }
                    }
                    else
                    {
                        fileName = PhConcatStringRef2(&variableValue, AppendPath);
                    }
                }
                else
                {
                    if (NativeFileName)
                        fileName = PhDosPathNameToNtPathName(&variableValue);
                    else
                        fileName = PhCreateString2(&variableValue);
                }

                return fileName;
            }
        }
        break;
    case PH_FOLDERID_RoamingAppData:
        {
            static CONST PH_STRINGREF variableName = PH_STRINGREF_INIT(L"APPDATA");
            static CONST PH_STRINGREF variableNameProfile = PH_STRINGREF_INIT(L"USERPROFILE");
            NTSTATUS status;
            PH_STRINGREF variableValue;
            WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

            PhInitializeBufferStringRef(&variableValue, variableBuffer, sizeof(variableBuffer));

            status = PhQueryEnvironmentVariableStringRef(
                NULL,
                &variableName,
                &variableValue
                );

            if (!NT_SUCCESS(status))
            {
                status = PhQueryEnvironmentVariableStringRef(
                    NULL,
                    &variableNameProfile,
                    &variableValue
                    );
            }

            if (NT_SUCCESS(status))
            {
                PPH_STRING fileName;

                if (AppendPath)
                {
                    if (NativeFileName)
                    {
                        if (fileName = PhDosPathNameToNtPathName(&variableValue))
                        {
                            PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, AppendPath));
                        }
                        else
                        {
                            fileName = NULL;
                        }
                    }
                    else
                    {
                        fileName = PhConcatStringRef2(&variableValue, AppendPath);
                    }
                }
                else
                {
                    if (NativeFileName)
                    {
                        fileName = PhDosPathNameToNtPathName(&variableValue);
                    }
                    else
                    {
                        fileName = PhCreateString2(&variableValue);
                    }
                }

                return fileName;
            }
        }
        break;
    //case PH_FOLDERID_ProgramFiles:
    //    {
    //        PPH_STRING value;
    //
    //        if (value = PhExpandEnvironmentStringsZ(L"%ProgramFiles%"))
    //        {
    //            if (AppendPath)
    //            {
    //                PhMoveReference(&value, PhConcatStringRef2(&value->sr, AppendPath));
    //            }
    //
    //            return value;
    //        }
    //    }
    //    break;
    case PH_FOLDERID_ProgramData:
        {
            static CONST PH_STRINGREF variableName = PH_STRINGREF_INIT(L"PROGRAMDATA");
            NTSTATUS status;
            PH_STRINGREF variableValue;
            WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

            PhInitializeBufferStringRef(&variableValue, variableBuffer, sizeof(variableBuffer));

            status = PhQueryEnvironmentVariableStringRef(
                NULL,
                &variableName,
                &variableValue
                );

            if (NT_SUCCESS(status))
            {
                PPH_STRING fileName;

                if (AppendPath)
                {
                    if (NativeFileName)
                    {
                        if (fileName = PhDosPathNameToNtPathName(&variableValue))
                        {
                            PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, AppendPath));
                        }
                        else
                        {
                            fileName = NULL;
                        }
                    }
                    else
                    {
                        fileName = PhConcatStringRef2(&variableValue, AppendPath);
                    }
                }
                else
                {
                    if (NativeFileName)
                    {
                        fileName = PhDosPathNameToNtPathName(&variableValue);
                    }
                    else
                    {
                        fileName = PhCreateString2(&variableValue);
                    }
                }

                return fileName;
            }
        }
        break;
    }
#else
    switch (Folder)
    {
    case PH_FOLDERID_LocalAppData:
        {
            PPH_STRING value;

            if (value = PhGetKnownFolderPath(&FOLDERID_LocalAppData, AppendPath))
            {
                if (NativeFileName)
                {
                    PhMoveReference(&value, PhDosPathNameToNtPathName(&value->sr));
                }

                return value;
            }
        }
        break;
    case PH_FOLDERID_RoamingAppData:
        {
            PPH_STRING value;

            if (value = PhGetKnownFolderPath(&FOLDERID_RoamingAppData, AppendPath))
            {
                if (NativeFileName)
                {
                    PhMoveReference(&value, PhDosPathNameToNtPathName(&value->sr));
                }

                return value;
            }
        }
        break;
    //case PH_FOLDERID_ProgramFiles:
    //    return PhGetKnownFolderPath(&FOLDERID_ProgramFiles, AppendPath);
    case PH_FOLDERID_ProgramData:
        {
            PPH_STRING value;

            if (value = PhGetKnownFolderPath(&FOLDERID_ProgramData, AppendPath))
            {
                if (NativeFileName)
                {
                    PhMoveReference(&value, PhDosPathNameToNtPathName(&value->sr));
                }

                return value;
            }
        }
        break;
    }
#endif
    return NULL;
}

/**
 * Retrieves the full path of a known folder.
 *
 * \param Folder A pointer to the GUID that identifies the known folder.
 * \param AppendPath An optional string to append to the folder path.
 * \return A pointer to a string containing the full path of the known folder, or NULL if the operation fails.
 */
PPH_STRING PhGetKnownFolderPath(
    _In_ PCGUID Folder,
    _In_opt_ PCPH_STRINGREF AppendPath
    )
{
    return PhGetKnownFolderPathEx(Folder, 0, NULL, AppendPath);
}

/**
 * Flag mapping table for known folder path retrieval.
 *
 * Maps PH_KF_FLAG_* values to corresponding KF_FLAG_* values for use with SHGetKnownFolderPath.
 */
static const PH_FLAG_MAPPING PhpKnownFolderFlagMappings[] =
{
    { PH_KF_FLAG_FORCE_PACKAGE_REDIRECTION, KF_FLAG_FORCE_APP_DATA_REDIRECTION },
    { PH_KF_FLAG_FORCE_APPCONTAINER_REDIRECTION, KF_FLAG_FORCE_APPCONTAINER_REDIRECTION },
};

/**
 * Retrieves the full path of a known folder optionally using a token handle and additional flags.
 *
 * \param Folder A pointer to the GUID that identifies the known folder.
 * \param Flags Flags controlling folder path retrieval (see PH_KF_FLAG_*).
 * \param TokenHandle An optional token handle for user-specific folder resolution.
 * \param AppendPath An optional string to append to the folder path.
 * \return A pointer to a string containing the full path of the known folder, or NULL if the operation fails.
 */
PPH_STRING PhGetKnownFolderPathEx(
    _In_ PCGUID Folder,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PCPH_STRINGREF AppendPath
    )
{
    PPH_STRING path;
    ULONG flags;
    PWSTR knownFolderBuffer;
    SIZE_T knownFolderLength;
    SIZE_T knownAppendLength;

    flags = 0;
    PhMapFlags1(&flags, Flags, PhpKnownFolderFlagMappings, ARRAYSIZE(PhpKnownFolderFlagMappings));

    if (FAILED(PhShellGetKnownFolderPath(
        Folder,
        flags | KF_FLAG_DONT_VERIFY,
        TokenHandle,
        &knownFolderBuffer
        )))
    {
        return NULL;
    }

    knownFolderLength = PhCountStringZ(knownFolderBuffer) * sizeof(WCHAR);
    knownAppendLength = AppendPath ? AppendPath->Length : 0;

    path = PhCreateStringEx(NULL, knownFolderLength + knownAppendLength);
    memcpy(
        path->Buffer,
        knownFolderBuffer,
        knownFolderLength
        );

    if (AppendPath)
    {
        memcpy(
            PTR_ADD_OFFSET(path->Buffer, knownFolderLength),
            AppendPath->Buffer,
            knownAppendLength
            );
    }

    CoTaskMemFree(knownFolderBuffer);

    return path;
}

// rev from GetTempPath2W (dmex)
/**
 * Retrieves the path to the system temporary directory, optionally appending a subpath.
 *
 * This function determines the appropriate temporary directory for the current user or process context.
 * - If running as an elevated process with the LocalSystem SID, it returns the system root's "SystemTemp" directory if it exists.
 * - Otherwise, it checks the "TMP", "TEMP", and "USERPROFILE" environment variables in order, returning the first valid path found.
 * - If an AppendPath is provided, it is concatenated to the selected temporary directory.
 *
 * \param AppendPath Optional string to append to the temporary directory path.
 * \return A pointer to a string containing the full path to the temporary directory (with optional appended path),
 * \remarks
 * - If running as LocalSystem, the function prefers the "SystemTemp" directory under the system root.
 * - The function checks environment variables in the order: "TMP", "TEMP", "USERPROFILE".
 * - If no environment variable yields a valid path, the function returns NULL.
 * - The returned path is suitable for creating temporary files or directories.
 * - The function supports both native NT paths and Win32 paths, depending on context.
 */
PPH_STRING PhGetTemporaryDirectory(
    _In_opt_ PCPH_STRINGREF AppendPath
    )
{
    static CONST PH_STRINGREF variableNameTmp = PH_STRINGREF_INIT(L"TMP");
    static CONST PH_STRINGREF variableNameTemp = PH_STRINGREF_INIT(L"TEMP");
    static CONST PH_STRINGREF variableNameProfile = PH_STRINGREF_INIT(L"USERPROFILE");
    NTSTATUS status;
    PH_STRINGREF variableValue;
    WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

    if (
        PhGetOwnTokenAttributes().Elevated &&
        PhEqualSid(PhGetOwnTokenAttributes().TokenSid, &PhSeLocalSystemSid)
        )
    {
        static CONST PH_STRINGREF systemTemp = PH_STRINGREF_INIT(L"SystemTemp");
        PH_STRINGREF systemRoot;
        PPH_STRING systemPath;

        PhGetSystemRoot(&systemRoot);

        if (AppendPath)
        {
            systemPath = PhConcatStringRef4(&systemRoot, &PhNtPathSeparatorString, &systemTemp, AppendPath);
        }
        else
        {
            systemPath = PhConcatStringRef3(&systemRoot, &PhNtPathSeparatorString, &systemTemp);
        }

        if (PhDoesDirectoryExistWin32(PhGetString(systemPath))) // Required for Windows 7/8 (dmex)
        {
            return systemPath;
        }

        PhDereferenceObject(systemPath);
    }

    PhInitializeBufferStringRef(&variableValue, variableBuffer, sizeof(variableBuffer));

    status = PhQueryEnvironmentVariableStringRef(
        NULL,
        &variableNameTmp,
        &variableValue
        );

    if (!NT_SUCCESS(status))
    {
        PhInitializeBufferStringRef(&variableValue, variableBuffer, sizeof(variableBuffer));

        status = PhQueryEnvironmentVariableStringRef(
            NULL,
            &variableNameTemp,
            &variableValue
            );

        if (!NT_SUCCESS(status))
        {
            PhInitializeBufferStringRef(&variableValue, variableBuffer, sizeof(variableBuffer));

            status = PhQueryEnvironmentVariableStringRef(
                NULL,
                &variableNameProfile,
                &variableValue
                );
        }
    }

    if (NT_SUCCESS(status))
    {
        if (AppendPath)
        {
            return PhConcatStringRef2(&variableValue, AppendPath);
        }

        return PhCreateString2(&variableValue);
    }

    return NULL;
}

/**
 * Waits on multiple objects while processing window messages.
 *
 * \param WindowHandle The window to process messages for, or NULL to process all messages for the current thread.
 * \param NumberOfHandles The number of handles specified in \a Handles. This must not be greater than MAXIMUM_WAIT_OBJECTS - 1.
 * \param Handles An array of handles.
 * \param Timeout The number of milliseconds to wait on the objects, or INFINITE for no timeout.
 * \param WakeMask The input types for which an input event object handle will be added to the array of object handles.
 * \remarks The wait is always in WaitAny mode. Passing a specific HWND to PeekMessage filters out thread messages 
 * (e.g., WM_QUIT) and messages for other windows on the same thread. If you expect full pumping, the HWND must be NULL.
 */
NTSTATUS PhWaitForMultipleObjectsAndPump(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG NumberOfHandles,
    _In_ PHANDLE Handles,
    _In_ ULONG Timeout,
    _In_ ULONG WakeMask
    )
{
    NTSTATUS status;
    ULONG64 startTickCount;
    ULONG64 currentTickCount;
    ULONG64 currentTimeout;

    startTickCount = NtGetTickCount64();
    currentTimeout = Timeout;

    while (TRUE)
    {
        status = MsgWaitForMultipleObjects(
            NumberOfHandles,
            Handles,
            FALSE,
            (ULONG)currentTimeout,
            WakeMask
            );

        if (
            status >= STATUS_WAIT_0 && status < (NTSTATUS)(STATUS_WAIT_0 + NumberOfHandles) ||
            status >= STATUS_ABANDONED_WAIT_0  && status < (NTSTATUS)(STATUS_ABANDONED_WAIT_0 + NumberOfHandles)
            )
        {
            return status;
        }
        else if (status == (STATUS_WAIT_0 + NumberOfHandles))
        {
            MSG msg;

            // Pump messages

            while (PeekMessage(&msg, WindowHandle, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            return status;
        }

        // Recompute the timeout value.

        if (Timeout != INFINITE)
        {
            currentTickCount = NtGetTickCount64();
            currentTimeout = Timeout - (currentTickCount - startTickCount);

            if ((LONG64)currentTimeout < 0)
                return STATUS_TIMEOUT;
        }
    }
}

/**
 * Creates a native process and an initial thread.
 *
 * \param FileName The Win32 file name of the image.
 * \param CommandLine The command line string to pass to the process. This string cannot be used to
 * specify the image to execute.
 * \param Environment The environment block for the process. Specify NULL to use the environment of
 * the current process.
 * \param CurrentDirectory The current directory string to pass to the process.
 * \param Information Additional parameters to pass to the process.
 * \param Flags A combination of the following:
 * \li \c PH_CREATE_PROCESS_INHERIT_HANDLES Inheritable handles will be duplicated to the process
 * from the parent process.
 * \li \c PH_CREATE_PROCESS_SUSPENDED The initial thread will be created suspended.
 * \li \c PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB The process will not be assigned to the job object
 * associated with the parent process.
 * \li \c PH_CREATE_PROCESS_NEW_CONSOLE The process will have its own console, instead of inheriting
 * the console of the parent process.
 * \param ParentProcessHandle The process from which the new process will inherit attributes.
 * Specify NULL for the current process.
 * \param ClientId A variable which receives the identifier of the initial thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcess(
    _In_ PCWSTR FileName,
    _In_opt_ PCPH_STRINGREF CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCPH_STRINGREF CurrentDirectory,
    _In_opt_ PPH_CREATE_PROCESS_INFO Information,
    _In_ ULONG Flags,
    _In_opt_ HANDLE ParentProcessHandle,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    )
{
    NTSTATUS status;
    RTL_USER_PROCESS_INFORMATION processInfo;
    PRTL_USER_PROCESS_PARAMETERS parameters;
    UNICODE_STRING fileName;
    UNICODE_STRING commandLine;
    UNICODE_STRING currentDirectory;
    PUNICODE_STRING windowTitle;
    PUNICODE_STRING desktopInfo;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (CommandLine)
    {
        if (!PhStringRefToUnicodeString(CommandLine, &commandLine))
            return STATUS_NAME_TOO_LONG;
    }

    if (CurrentDirectory)
    {
        if (!PhStringRefToUnicodeString(CurrentDirectory, &currentDirectory))
            return STATUS_NAME_TOO_LONG;
    }

    if (Information)
    {
        windowTitle = Information->WindowTitle;
        desktopInfo = Information->DesktopInfo;
    }
    else
    {
        windowTitle = NULL;
        desktopInfo = NULL;
    }

    if (!windowTitle)
        windowTitle = &fileName;

    if (!desktopInfo)
        desktopInfo = &NtCurrentPeb()->ProcessParameters->DesktopInfo;

    status = RtlCreateProcessParameters(
        &parameters,
        &fileName,
        Information ? Information->DllPath : NULL,
        CurrentDirectory ? &currentDirectory : NULL,
        CommandLine ? &commandLine : &fileName,
        Environment,
        windowTitle,
        desktopInfo,
        Information ? Information->ShellInfo : NULL,
        Information ? Information->RuntimeData : NULL
        );

    if (NT_SUCCESS(status))
    {
        status = RtlCreateUserProcess(
            &fileName,
            0,
            parameters,
            NULL,
            NULL,
            ParentProcessHandle,
            !!(Flags & PH_CREATE_PROCESS_INHERIT_HANDLES),
            NULL,
            NULL,
            &processInfo
            );
        RtlDestroyProcessParameters(parameters);
    }

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        if (!(Flags & PH_CREATE_PROCESS_SUSPENDED))
            NtResumeThread(processInfo.ThreadHandle, NULL);

        if (ClientId)
            *ClientId = processInfo.ClientId;

        if (ProcessHandle)
            *ProcessHandle = processInfo.ProcessHandle;
        else
            NtClose(processInfo.ProcessHandle);

        if (ThreadHandle)
            *ThreadHandle = processInfo.ThreadHandle;
        else
            NtClose(processInfo.ThreadHandle);
    }

    return status;
}

/**
 * Creates a Win32 process and an initial thread.
 *
 * \param FileName The Win32 file name of the image.
 * \param CommandLine The command line to execute. This can be specified instead of \a FileName to
 * indicate the image to execute.
 * \param Environment The environment block for the process. Specify NULL to use the environment of
 * the current process.
 * \param CurrentDirectory The current directory string to pass to the process.
 * \param Flags See PhCreateProcess().
 * \param TokenHandle The token of the process. Specify NULL for the token of the parent process.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcessWin32(
    _In_opt_ PCWSTR FileName,
    _In_opt_ PCWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCWSTR CurrentDirectory,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    )
{
    return PhCreateProcessWin32Ex(
        FileName,
        CommandLine,
        Environment,
        CurrentDirectory,
        NULL,
        Flags,
        TokenHandle,
        NULL,
        ProcessHandle,
        ThreadHandle
        );
}

static const PH_FLAG_MAPPING PhpCreateProcessMappings[] =
{
    { PH_CREATE_PROCESS_DEBUG, DEBUG_PROCESS },
    { PH_CREATE_PROCESS_DEBUG_ONLY_THIS_PROCESS, DEBUG_ONLY_THIS_PROCESS },
    { PH_CREATE_PROCESS_SUSPENDED, CREATE_SUSPENDED },
    { PH_CREATE_PROCESS_NEW_CONSOLE, CREATE_NEW_CONSOLE },
    { PH_CREATE_PROCESS_UNICODE_ENVIRONMENT, CREATE_UNICODE_ENVIRONMENT },
    { PH_CREATE_PROCESS_EXTENDED_STARTUPINFO, EXTENDED_STARTUPINFO_PRESENT },
    { PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB, CREATE_BREAKAWAY_FROM_JOB },
    { PH_CREATE_PROCESS_DEFAULT_ERROR_MODE, CREATE_DEFAULT_ERROR_MODE },
};

FORCEINLINE VOID PhpConvertProcessInformation(
    _In_ PPROCESS_INFORMATION ProcessInfo,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    )
{
    if (ClientId)
    {
        ClientId->UniqueProcess = UlongToHandle(ProcessInfo->dwProcessId);
        ClientId->UniqueThread = UlongToHandle(ProcessInfo->dwThreadId);
    }

    if (ProcessHandle)
        *ProcessHandle = ProcessInfo->hProcess;
    else if (ProcessInfo->hProcess)
        NtClose(ProcessInfo->hProcess);

    if (ThreadHandle)
        *ThreadHandle = ProcessInfo->hThread;
    else if (ProcessInfo->hThread)
        NtClose(ProcessInfo->hThread);
}

/**
 * Creates a Win32 process and an initial thread.
 *
 * \param FileName The Win32 file name of the image.
 * \param CommandLine The command line to execute. This can be specified instead of \a FileName to
 * indicate the image to execute.
 * \param Environment The environment block for the process. Specify NULL to use the environment of
 * the current process.
 * \param CurrentDirectory The current directory string to pass to the process.
 * \param StartupInfo A STARTUPINFO structure containing additional parameters for the process.
 * \param Flags See PhCreateProcess().
 * \param TokenHandle The token of the process. Specify NULL for the token of the parent process.
 * \param ClientId A variable which receives the identifier of the initial thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcessWin32Ex(
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
    )
{
    NTSTATUS status;
    PPH_STRING fileName = NULL;
    PPH_STRING commandLine = NULL;
    PPH_STRING currentDirectory = NULL;
    STARTUPINFOEX startupInfo;
    PROCESS_INFORMATION processInfo = { 0 };
    ULONG newFlags;

    if (CommandLine) // duplicate because CreateProcess modifies the string (wj32)
        commandLine = PhCreateString(CommandLine);

    if (FileName)
        fileName = PhCreateString(FileName);
    else
    {
        PH_STRINGREF commandLineFileName;
        PH_STRINGREF commandLineArguments;

        PhParseCommandLineFuzzy(&commandLine->sr, &commandLineFileName, &commandLineArguments, &fileName);

        if (PhIsNullOrEmptyString(fileName))
            PhMoveReference(&fileName, PhCreateString2(&commandLineFileName));

        // Escape the commandline (or uncomment CommandLineToArgvW to clear the filename when it contains \\program files\\) (dmex)
        {
            static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");
            static CONST PH_STRINGREF space = PH_STRINGREF_INIT(L" ");
            PPH_STRING escaped;

            escaped = PhConcatStringRef3(&separator, &commandLineFileName, &separator);

            if (commandLineArguments.Length)
            {
                PhMoveReference(&escaped, PhConcatStringRef3(&escaped->sr, &space, &commandLineArguments));
            }

            PhMoveReference(&commandLine, escaped);
        }

        //INT cmdlineArgCount;
        //PWSTR* cmdlineArgList;
        //
        //// Try extract the filename or CreateProcess might execute the wrong executable. (dmex)
        //if (commandLine && (cmdlineArgList = PhCommandLineToArgv(commandLine->Buffer, &cmdlineArgCount)))
        //{
        //    PhMoveReference(&fileName, PhCreateString(cmdlineArgList[0]));
        //    LocalFree(cmdlineArgList);
        //}

        if (fileName && !PhDoesFileExistWin32(PhGetString(fileName)))
        {
            PPH_STRING filePath;

            // The user typed a name without a path so attempt to locate the executable. (dmex)
            if (filePath = PhSearchFilePath(PhGetString(fileName), L".exe"))
                PhMoveReference(&fileName, filePath);
            else
                PhClearReference(&fileName);
        }
    }

    // Set the current directory to the same location as the target executable
    // or CreateProcess uses our current directory. (dmex)
    if (CurrentDirectory)
        currentDirectory = PhCreateString(CurrentDirectory);
    else
    {
        if (!PhIsNullOrEmptyString(fileName))
            currentDirectory = PhGetBaseDirectory(fileName);
    }

    newFlags = 0;
    PhMapFlags1(&newFlags, Flags, PhpCreateProcessMappings, sizeof(PhpCreateProcessMappings) / sizeof(PH_FLAG_MAPPING));

    if (!StartupInfo)
    {
        SetFlag(newFlags, EXTENDED_STARTUPINFO_PRESENT);
        memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
        startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    }

    if (TokenHandle)
    {
        if (CreateProcessAsUser(
            TokenHandle,
            PhGetString(fileName),
            PhGetString(commandLine),
            NULL,
            NULL,
            !!(Flags & PH_CREATE_PROCESS_INHERIT_HANDLES),
            newFlags,
            Environment,
            PhGetString(currentDirectory),
            StartupInfo ? StartupInfo : &startupInfo,
            &processInfo
            ))
            status = STATUS_SUCCESS;
        else
            status = PhGetLastWin32ErrorAsNtStatus();
    }
    else
    {
        if (CreateProcess(
            PhGetString(fileName),
            PhGetString(commandLine),
            NULL,
            NULL,
            !!(Flags & PH_CREATE_PROCESS_INHERIT_HANDLES),
            newFlags,
            Environment,
            PhGetString(currentDirectory),
            StartupInfo ? StartupInfo : &startupInfo,
            &processInfo
            ))
            status = STATUS_SUCCESS;
        else
            status = PhGetLastWin32ErrorAsNtStatus();
    }

    if (fileName)
        PhDereferenceObject(fileName);
    if (commandLine)
        PhDereferenceObject(commandLine);
    if (currentDirectory)
        PhDereferenceObject(currentDirectory);

    if (NT_SUCCESS(status))
    {
        if (ClientId)
        {
            ClientId->UniqueProcess = UlongToHandle(processInfo.dwProcessId);
            ClientId->UniqueThread = UlongToHandle(processInfo.dwThreadId);
        }

        if (ProcessHandle)
        {
            *ProcessHandle = processInfo.hProcess;
        }
        else if (processInfo.hProcess)
        {
            NtClose(processInfo.hProcess);
        }

        if (ThreadHandle)
        {
            *ThreadHandle = processInfo.hThread;
        }
        else if (processInfo.hThread)
        {
            NtClose(processInfo.hThread);
        }
    }
    else
    {
        if (processInfo.hProcess)
            NtClose(processInfo.hProcess);
        if (processInfo.hThread)
            NtClose(processInfo.hThread);
    }

    return status;
}

/**
 * Creates a Win32 process and an initial thread under the specified user.
 *
 * \param Information Parameters specifying how to create the process.
 * \param Flags See PhCreateProcess(). Additional flags may be used:
 * \li \c PH_CREATE_PROCESS_USE_PROCESS_TOKEN Use the token of the process specified by
 * \a ProcessIdWithToken in \a Information.
 * \li \c PH_CREATE_PROCESS_USE_SESSION_TOKEN Use the token of the session specified by
 * \a SessionIdWithToken in \a Information.
 * \li \c PH_CREATE_PROCESS_USE_LINKED_TOKEN Use the linked token to create the process; this causes
 * the process to be elevated or unelevated depending on the specified options.
 * \li \c PH_CREATE_PROCESS_SET_SESSION_ID \a SessionId is specified in \a Information.
 * \li \c PH_CREATE_PROCESS_WITH_PROFILE Load the user profile, if supported.
 * \param StartupInfo A STARTUPINFO structure containing additional parameters for the process.
 * \param ClientId A variable which receives the identifier of the initial thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcessAsUser(
    _In_ PPH_CREATE_PROCESS_AS_USER_INFO Information,
    _In_ ULONG Flags,
    _In_opt_ PVOID StartupInfo,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    BOOLEAN needsDuplicate = FALSE;
    PVOID defaultEnvironment = NULL;
    STARTUPINFOEX startupInfo;

    if ((Flags & PH_CREATE_PROCESS_USE_PROCESS_TOKEN) && (Flags & PH_CREATE_PROCESS_USE_SESSION_TOKEN))
        return STATUS_INVALID_PARAMETER_2;
    if (!Information->ApplicationName && !Information->CommandLine)
        return STATUS_INVALID_PARAMETER_MIX;

    if (!StartupInfo)
    {
        RtlZeroMemory(&startupInfo, sizeof(STARTUPINFOEX));
        startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
        startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.StartupInfo.wShowWindow = SW_NORMAL;
        startupInfo.StartupInfo.lpDesktop = (PWSTR)Information->DesktopName;
    }

    // Try to use CreateProcessWithLogonW if we need to load the user profile.
    // This isn't compatible with some options.
    if (Flags & PH_CREATE_PROCESS_WITH_PROFILE)
    {
        BOOLEAN useWithLogon;

        useWithLogon = TRUE;

        if (Flags & (PH_CREATE_PROCESS_USE_PROCESS_TOKEN | PH_CREATE_PROCESS_USE_SESSION_TOKEN))
            useWithLogon = FALSE;

        if (Flags & PH_CREATE_PROCESS_USE_LINKED_TOKEN)
            useWithLogon = FALSE;

        if (Flags & PH_CREATE_PROCESS_SET_SESSION_ID)
        {
            ULONG sessionId = ULONG_MAX;

            PhGetProcessSessionId(NtCurrentProcess(), &sessionId);

            if (Information->SessionId != sessionId)
                useWithLogon = FALSE;
        }

        if (Information->LogonType && Information->LogonType != LOGON32_LOGON_INTERACTIVE)
            useWithLogon = FALSE;

        if (useWithLogon)
        {
            PPH_STRING applicationName = NULL;
            PPH_STRING commandLine = NULL;
            PPH_STRING currentDirectory = NULL;
            PROCESS_INFORMATION processInfo = { NULL };
            ULONG newFlags = 0;

            if (!Information->LogonFlags)
                Information->LogonFlags = LOGON_WITH_PROFILE;

            if (Information->ApplicationName)
            {
                applicationName = PhCreateString(Information->ApplicationName);
            }

            if (Information->CommandLine) // duplicate because CreateProcess modifies the string
            {
                commandLine = PhCreateString(Information->CommandLine);
            }

            if (Information->CurrentDirectory)
            {
                currentDirectory = PhCreateString(Information->CurrentDirectory);
            }

            if (!Information->Environment)
            {
                SetFlag(Flags, PH_CREATE_PROCESS_UNICODE_ENVIRONMENT);
            }

            PhMapFlags1(&newFlags, Flags, PhpCreateProcessMappings, sizeof(PhpCreateProcessMappings) / sizeof(PH_FLAG_MAPPING));

            if (CreateProcessWithLogonW(
                Information->UserName,
                Information->DomainName,
                Information->Password,
                Information->LogonFlags,
                PhGetString(applicationName),
                PhGetString(commandLine),
                newFlags,
                Information->Environment,
                PhGetString(currentDirectory),
                StartupInfo ? StartupInfo : &startupInfo,
                &processInfo
                ))
                status = STATUS_SUCCESS;
            else
                status = PhGetLastWin32ErrorAsNtStatus();

            if (applicationName)
                PhDereferenceObject(applicationName);
            if (commandLine)
                PhDereferenceObject(commandLine);
            if (currentDirectory)
                PhDereferenceObject(currentDirectory);

            if (NT_SUCCESS(status))
            {
                if (ClientId)
                {
                    ClientId->UniqueProcess = UlongToHandle(processInfo.dwProcessId);
                    ClientId->UniqueThread = UlongToHandle(processInfo.dwThreadId);
                }

                if (ProcessHandle)
                {
                    *ProcessHandle = processInfo.hProcess;
                }
                else if (processInfo.hProcess)
                {
                    NtClose(processInfo.hProcess);
                }

                if (ThreadHandle)
                {
                    *ThreadHandle = processInfo.hThread;
                }
                else if (processInfo.hThread)
                {
                    NtClose(processInfo.hThread);
                }
            }
            else
            {
                if (processInfo.hProcess)
                    NtClose(processInfo.hProcess);
                if (processInfo.hThread)
                    NtClose(processInfo.hThread);
            }

            return status;
        }
    }

    // Get the token handle. Various methods are supported.

    if (Flags & PH_CREATE_PROCESS_USE_PROCESS_TOKEN)
    {
        HANDLE processHandle;

        if (!NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            Information->ProcessIdWithToken
            )))
            return status;

        status = PhOpenProcessToken(
            processHandle,
            TOKEN_ALL_ACCESS,
            &tokenHandle
            );
        NtClose(processHandle);

        if (!NT_SUCCESS(status))
            return status;

        if (Flags & PH_CREATE_PROCESS_SET_SESSION_ID)
            needsDuplicate = TRUE; // can't set the session ID of a token in use by a process
    }
    else if (Flags & PH_CREATE_PROCESS_USE_SESSION_TOKEN)
    {
        WINSTATIONUSERTOKEN userToken;
        ULONG returnLength;

        memset(&userToken, 0, sizeof(WINSTATIONUSERTOKEN));
        userToken.ProcessId = NtCurrentProcessId();
        userToken.ThreadId = NtCurrentThreadId();

        if (!WinStationQueryInformationW(
            WINSTATION_CURRENT_SERVER,
            Information->SessionIdWithToken,
            WinStationUserToken,
            &userToken,
            sizeof(WINSTATIONUSERTOKEN),
            &returnLength
            ))
        {
            return PhGetLastWin32ErrorAsNtStatus();
        }

        tokenHandle = userToken.UserToken;

        if (Flags & PH_CREATE_PROCESS_SET_SESSION_ID)
            needsDuplicate = TRUE; // not sure if this is necessary
    }
    else
    {
        ULONG logonType;

        if (Information->LogonType)
        {
            logonType = Information->LogonType;
        }
        else
        {
            logonType = LOGON32_LOGON_INTERACTIVE;

            // Check if this is a service logon.
            if (PhEqualStringZ(Information->DomainName, L"NT AUTHORITY", TRUE))
            {
                if (PhEqualStringZ(Information->UserName, L"SYSTEM", TRUE) ||
                    PhEqualStringZ(Information->UserName, L"LOCAL SERVICE", TRUE) ||
                    PhEqualStringZ(Information->UserName, L"NETWORK SERVICE", TRUE))
                {
                    logonType = LOGON32_LOGON_SERVICE;
                }
            }
        }

        if (!LogonUser(
            Information->UserName,
            Information->DomainName,
            Information->Password,
            logonType,
            LOGON32_PROVIDER_DEFAULT,
            &tokenHandle
            ))
            return PhGetLastWin32ErrorAsNtStatus();
    }

    if (Flags & PH_CREATE_PROCESS_USE_LINKED_TOKEN)
    {
        HANDLE linkedTokenHandle;
        TOKEN_TYPE tokenType;

        // NtQueryInformationToken normally returns an impersonation token with
        // SecurityIdentification, but if the process is running with SeTcbPrivilege, it returns a
        // primary token. We can never duplicate a SecurityIdentification impersonation token to
        // make it a primary token, so we just check if the token is primary before using it.

        if (NT_SUCCESS(PhGetTokenLinkedToken(tokenHandle, &linkedTokenHandle)))
        {
            if (NT_SUCCESS(PhGetTokenType(linkedTokenHandle, &tokenType)) && tokenType == TokenPrimary)
            {
                NtClose(tokenHandle);
                tokenHandle = linkedTokenHandle;
                needsDuplicate = FALSE; // the linked token that is returned is always a copy, so no need to duplicate
            }
            else
            {
                NtClose(linkedTokenHandle);
            }
        }
    }

    if (needsDuplicate)
    {
        HANDLE newTokenHandle;
        OBJECT_ATTRIBUTES objectAttributes;

        InitializeObjectAttributes(
            &objectAttributes,
            NULL,
            0,
            NULL,
            NULL
            );

        status = NtDuplicateToken(
            tokenHandle,
            TOKEN_ALL_ACCESS,
            &objectAttributes,
            FALSE,
            TokenPrimary,
            &newTokenHandle
            );
        NtClose(tokenHandle);

        if (!NT_SUCCESS(status))
            return status;

        tokenHandle = newTokenHandle;
    }

    // Set the session ID if needed.

    if (Flags & PH_CREATE_PROCESS_SET_SESSION_ID)
    {
        if (!NT_SUCCESS(status = PhSetTokenSessionId(
            tokenHandle,
            Information->SessionId
            )))
        {
            NtClose(tokenHandle);
            return status;
        }
    }

    // Set the UIAccess flag if needed.

    if (Flags & PH_CREATE_PROCESS_SET_UIACCESS)
    {
        if (!NT_SUCCESS(status = PhSetTokenUIAccess(
            tokenHandle,
            TRUE
            )))
        {
            NtClose(tokenHandle);
            return status;
        }
    }

    if (!Information->Environment)
    {
        PhCreateEnvironmentBlock(&defaultEnvironment, tokenHandle, FALSE);

        if (defaultEnvironment)
            Flags |= PH_CREATE_PROCESS_UNICODE_ENVIRONMENT;
    }

    status = PhCreateProcessWin32Ex(
        Information->ApplicationName,
        Information->CommandLine,
        Information->Environment ? Information->Environment : defaultEnvironment,
        Information->CurrentDirectory,
        StartupInfo ? StartupInfo : &startupInfo,
        Flags,
        tokenHandle,
        ClientId,
        ProcessHandle,
        ThreadHandle
        );

    if (defaultEnvironment)
    {
        PhDestroyEnvironmentBlock(defaultEnvironment);
    }

    NtClose(tokenHandle);

    return status;
}

/**
 * Filters a token to create a limited user security context.
 *
 * \param TokenHandle A handle to an existing token. The handle must have TOKEN_DUPLICATE,
 * TOKEN_QUERY, TOKEN_ADJUST_GROUPS, TOKEN_ADJUST_DEFAULT, READ_CONTROL and WRITE_DAC access.
 * \param NewTokenHandle A variable which receives a handle to the filtered token. The handle will
 * have the same granted access as \a TokenHandle.
 */
NTSTATUS PhFilterTokenForLimitedUser(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE NewTokenHandle
    )
{
    static SID_IDENTIFIER_AUTHORITY mandatoryLabelAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
    static const LUID_AND_ATTRIBUTES defaultAllowedPrivileges[] =
    {
        { { SE_SHUTDOWN_PRIVILEGE, 0 }, 0 },
        { { SE_CHANGE_NOTIFY_PRIVILEGE, 0 }, 0 },
        { { SE_UNDOCK_PRIVILEGE, 0 }, 0 },
        { { SE_INC_WORKING_SET_PRIVILEGE, 0 }, 0 },
        { { SE_TIME_ZONE_PRIVILEGE, 0 }, 0 }
    };

    NTSTATUS status;
    PSID administratorsSid = PhSeAdministratorsSid();
    PSID usersSid = PhSeUsersSid();
    UCHAR sidsToDisableBuffer[FIELD_OFFSET(TOKEN_GROUPS, Groups) + sizeof(SID_AND_ATTRIBUTES)];
    PTOKEN_GROUPS sidsToDisable;
    ULONG i;
    ULONG j;
    ULONG deleteIndex;
    BOOLEAN found;
    PTOKEN_PRIVILEGES privilegesOfToken;
    PTOKEN_PRIVILEGES privilegesOfUsers;
    PTOKEN_PRIVILEGES privilegesToDelete;
    UCHAR lowMandatoryLevelSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG)];
    PSID lowMandatoryLevelSid;
    TOKEN_MANDATORY_LABEL mandatoryLabel;
    PSECURITY_DESCRIPTOR currentSecurityDescriptor;
    BOOLEAN currentDaclPresent;
    BOOLEAN currentDaclDefaulted;
    PACL currentDacl;
    PH_TOKEN_USER currentUser;
    PACE_HEADER currentAce;
    ULONG newDaclLength;
    PACL newDacl;
    SECURITY_DESCRIPTOR newSecurityDescriptor;
    TOKEN_DEFAULT_DACL newDefaultDacl;
    HANDLE newTokenHandle;

    // Set up the SIDs to Disable structure.

    sidsToDisable = (PTOKEN_GROUPS)sidsToDisableBuffer;
    sidsToDisable->GroupCount = 1;
    sidsToDisable->Groups[0].Sid = administratorsSid;
    sidsToDisable->Groups[0].Attributes = 0;

    // Set up the Privileges to Delete structure.

    // Get the privileges that the input token contains.
    if (!NT_SUCCESS(status = PhGetTokenPrivileges(TokenHandle, &privilegesOfToken)))
        return status;

    // Get the privileges of the Users group - the privileges that we are going to allow.
    if (!NT_SUCCESS(PhGetAccountPrivileges(usersSid, &privilegesOfUsers)))
    {
        // Unsuccessful, so use the default set of privileges.
        privilegesOfUsers = PhAllocate(FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(defaultAllowedPrivileges));
        privilegesOfUsers->PrivilegeCount = sizeof(defaultAllowedPrivileges) / sizeof(LUID_AND_ATTRIBUTES);
        memcpy(privilegesOfUsers->Privileges, defaultAllowedPrivileges, sizeof(defaultAllowedPrivileges));
    }

    // Allocate storage for the privileges we need to delete. The worst case scenario is that all
    // privileges in the token need to be deleted.
    privilegesToDelete = PhAllocate(FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * privilegesOfToken->PrivilegeCount);
    deleteIndex = 0;

    // Compute the privileges that we need to delete.
    for (i = 0; i < privilegesOfToken->PrivilegeCount; i++)
    {
        found = FALSE;

        // Is the privilege allowed?
        for (j = 0; j < privilegesOfUsers->PrivilegeCount; j++)
        {
            if (RtlIsEqualLuid(&privilegesOfToken->Privileges[i].Luid, &privilegesOfUsers->Privileges[j].Luid))
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            // This privilege needs to be deleted.
            privilegesToDelete->Privileges[deleteIndex].Attributes = 0;
            privilegesToDelete->Privileges[deleteIndex].Luid = privilegesOfToken->Privileges[i].Luid;
            deleteIndex++;
        }
    }

    privilegesToDelete->PrivilegeCount = deleteIndex;

    // Filter the token.

    status = NtFilterToken(
        TokenHandle,
        0,
        sidsToDisable,
        privilegesToDelete,
        NULL,
        &newTokenHandle
        );
    PhFree(privilegesToDelete);
    PhFree(privilegesOfUsers);
    PhFree(privilegesOfToken);

    if (!NT_SUCCESS(status))
        return status;

    // Set the integrity level to Low if we're on Vista and above.
    {
        lowMandatoryLevelSid = (PSID)lowMandatoryLevelSidBuffer;
        PhInitializeSid(lowMandatoryLevelSid, &mandatoryLabelAuthority, 1);
        *PhSubAuthoritySid(lowMandatoryLevelSid, 0) = SECURITY_MANDATORY_LOW_RID;

        mandatoryLabel.Label.Sid = lowMandatoryLevelSid;
        mandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY;

        NtSetInformationToken(newTokenHandle, TokenIntegrityLevel, &mandatoryLabel, sizeof(TOKEN_MANDATORY_LABEL));
    }

    // Fix up the security descriptor and default DACL.
    if (NT_SUCCESS(PhGetObjectSecurity(newTokenHandle, DACL_SECURITY_INFORMATION, &currentSecurityDescriptor)))
    {
        if (NT_SUCCESS(PhGetTokenUser(TokenHandle, &currentUser)))
        {
            if (!NT_SUCCESS(PhGetDaclSecurityDescriptor(
                currentSecurityDescriptor,
                &currentDaclPresent,
                &currentDacl,
                &currentDaclDefaulted
                )))
            {
                currentDaclPresent = FALSE;
            }

            newDaclLength = sizeof(ACL) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + PhLengthSid(currentUser.User.Sid);

            if (currentDaclPresent && currentDacl)
                newDaclLength += currentDacl->AclSize - sizeof(ACL);

            newDacl = PhAllocateZero(newDaclLength);
            PhCreateAcl(newDacl, newDaclLength, ACL_REVISION);

            // Add the existing DACL entries.
            if (currentDaclPresent && currentDacl)
            {
                for (i = 0; i < currentDacl->AceCount; i++)
                {
                    if (NT_SUCCESS(PhGetAce(currentDacl, i, &currentAce)))
                    {
                        status = PhAddAce(newDacl, ACL_REVISION, ULONG_MAX, currentAce, currentAce->AceSize);

                        if (!NT_SUCCESS(status))
                            break;
                    }
                }
            }

            // Allow access for the current user.

            if (NT_SUCCESS(status))
            {
                status = PhAddAccessAllowedAce(newDacl, ACL_REVISION, GENERIC_ALL, currentUser.User.Sid);
            }

            // Set the security descriptor of the new token.

            if (NT_SUCCESS(status))
            {
                status = PhCreateSecurityDescriptor(&newSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
            }

            if (NT_SUCCESS(status))
            {
                status = PhSetDaclSecurityDescriptor(&newSecurityDescriptor, TRUE, newDacl, FALSE);
            }

            if (NT_SUCCESS(status))
            {
                assert(RtlValidSecurityDescriptor(&newSecurityDescriptor));

                status = PhSetObjectSecurity(newTokenHandle, DACL_SECURITY_INFORMATION, &newSecurityDescriptor);
            }

            // Set the default DACL.

            newDefaultDacl.DefaultDacl = newDacl;
            NtSetInformationToken(newTokenHandle, TokenDefaultDacl, &newDefaultDacl, sizeof(TOKEN_DEFAULT_DACL));

            PhFree(newDacl);
        }

        PhFree(currentSecurityDescriptor);
    }

    *NewTokenHandle = newTokenHandle;

    return STATUS_SUCCESS;
}

NTSTATUS PhGetSecurityDescriptorAsString(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PPH_STRING* SecurityDescriptorString
    )
{
    ULONG stringSecurityDescriptorLength = 0;
    PWSTR stringSecurityDescriptor = NULL;

    if (!ConvertSecurityDescriptorToStringSecurityDescriptorW_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    if (ConvertSecurityDescriptorToStringSecurityDescriptorW_Import()(
        SecurityDescriptor,
        SDDL_REVISION,
        SecurityInformation,
        &stringSecurityDescriptor,
        &stringSecurityDescriptorLength
        ))
    {
        if (stringSecurityDescriptorLength & 1) // validate the string length
        {
            LocalFree(stringSecurityDescriptor);
            return STATUS_FAIL_CHECK;
        }

        *SecurityDescriptorString = PhCreateStringEx(
            stringSecurityDescriptor,
            stringSecurityDescriptorLength * sizeof(WCHAR)
            );
        LocalFree(stringSecurityDescriptor);
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

PSECURITY_DESCRIPTOR PhGetSecurityDescriptorFromString(
    _In_ PCWSTR SecurityDescriptorString
    )
{
    PVOID securityDescriptor = NULL;
    ULONG securityDescriptorLength = 0;
    PSECURITY_DESCRIPTOR securityDescriptorBuffer = NULL;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW_Import())
        return NULL;

    if (ConvertStringSecurityDescriptorToSecurityDescriptorW_Import()(
        SecurityDescriptorString,
        SDDL_REVISION,
        &securityDescriptorBuffer,
        &securityDescriptorLength
        ) &&
        securityDescriptorBuffer &&
        securityDescriptorLength
        )
    {
        securityDescriptor = PhAllocateCopy(
            securityDescriptorBuffer,
            securityDescriptorLength
            );
        assert(securityDescriptorLength == RtlLengthSecurityDescriptor(securityDescriptor));

        LocalFree(securityDescriptorBuffer);
    }

    return securityDescriptor;
}

NTSTATUS PhGetObjectSecurityDescriptorAsString(
    _In_ HANDLE Handle,
    _Out_ PPH_STRING* SecurityDescriptorString
    )
{
    NTSTATUS status;
    PSECURITY_DESCRIPTOR securityDescriptor;
    PPH_STRING securityDescriptorString;

    status = PhGetObjectSecurity(
        Handle,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION |
        ATTRIBUTE_SECURITY_INFORMATION | SCOPE_SECURITY_INFORMATION,
        &securityDescriptor
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetSecurityDescriptorAsString(
        securityDescriptor,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION |
        ATTRIBUTE_SECURITY_INFORMATION | SCOPE_SECURITY_INFORMATION,
        &securityDescriptorString
        );

    PhFree(securityDescriptor);

    if (NT_SUCCESS(status))
    {
        *SecurityDescriptorString = securityDescriptorString;
    }

    return status;
}

/**
 * Opens a file or location through the shell.
 *
 * \param ExecInfo A pointer to a SHELLEXECUTEINFO structure that contains and receives information about the application being executed.
 *
 * \return Successful or errant status.
 */
BOOLEAN PhShellExecuteWin32(
    _Inout_ PSHELLEXECUTEINFOW ExecInfo
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&ShellExecuteExW) ShellExecuteExW_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            ShellExecuteExW_I = PhGetDllBaseProcedureAddress(baseAddress, "ShellExecuteExW", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!ShellExecuteExW_I)
        return FALSE;

    return !!ShellExecuteExW_I(ExecInfo);
}

/**
 * Opens a file or location through the shell.
 *
 * \param WindowHandle The window to display user interface components on.
 * \param FileName A file name or location.
 * \param Parameters The parameters to pass to the executed application.
 */
VOID PhShellExecute(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Parameters
    )
{
    SHELLEXECUTEINFO info;

    memset(&info, 0, sizeof(SHELLEXECUTEINFO));
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
    info.nShow = SW_SHOW;
    info.hwnd = WindowHandle;

    if (!PhShellExecuteWin32(&info))
    {
        PhShowStatus(WindowHandle, L"Unable to execute the program.", 0, PhGetLastError());
    }
}

/**
 * Opens a file or location through the shell.
 *
 * \param WindowHandle The window to display user interface components on.
 * \param FileName A file name or location.
 * \param Parameters The parameters to pass to the executed application.
 * \param Directory The default working directory. If this value is NULL, the current process working directory is used.
 * \param ShowWindowType A value specifying how to show the application.
 * \param Flags A combination of the following:
 * \li \c PH_SHELL_EXECUTE_ADMIN Execute the application elevated.
 * \li \c PH_SHELL_EXECUTE_PUMP_MESSAGES Waits on the application while pumping messages, if
 * \a Timeout is specified.
 * \param Timeout The number of milliseconds to wait on the application, or 0 to return immediately
 * after the application is started.
 * \param ProcessHandle A variable which receives a handle to the new process.
 */
NTSTATUS PhShellExecuteEx(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Parameters,
    _In_opt_ PCWSTR Directory,
    _In_ LONG ShowWindowType,
    _In_ ULONG Flags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    SHELLEXECUTEINFO info;

    memset(&info, 0, sizeof(SHELLEXECUTEINFO));
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOZONECHECKS;
    info.hwnd = WindowHandle;
    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.lpDirectory = Directory;
    info.nShow = ShowWindowType;

    if (FlagOn(Flags, PH_SHELL_EXECUTE_ADMIN))
    {
        info.lpVerb = L"runas";
    }

    if (PhShellExecuteWin32(&info))
    {
        if (Timeout)
        {
            if (FlagOn(Flags, PH_SHELL_EXECUTE_PUMP_MESSAGES))
            {
                PhWaitForMultipleObjectsAndPump(NULL, 1, &info.hProcess, Timeout, QS_ALLEVENTS);
            }
            else
            {
                if (info.hProcess)
                {
                    PhWaitForSingleObject(info.hProcess, Timeout);
                }
            }
        }

        if (ProcessHandle)
            *ProcessHandle = info.hProcess;
        else if (info.hProcess)
            NtClose(info.hProcess);

        return STATUS_SUCCESS;
    }
    else
    {
        if (ProcessHandle)
            *ProcessHandle = NULL;

        return PhGetLastWin32ErrorAsNtStatus();
    }
}

/**
 * Opens Windows Explorer with a file selected.
 *
 * \param WindowHandle A handle to the parent window.
 * \param FileName A file name.
 */
VOID PhShellExploreFile(
    _In_ HWND WindowHandle,
    _In_ PCWSTR FileName
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&SHOpenFolderAndSelectItems) SHOpenFolderAndSelectItems_I = NULL;
    static typeof(&SHParseDisplayName) SHParseDisplayName_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            SHOpenFolderAndSelectItems_I = PhGetDllBaseProcedureAddress(baseAddress, "SHOpenFolderAndSelectItems", 0);
            SHParseDisplayName_I = PhGetDllBaseProcedureAddress(baseAddress, "SHParseDisplayName", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (SHOpenFolderAndSelectItems_I && SHParseDisplayName_I)
    {
        LPITEMIDLIST item;

        if (SUCCEEDED(SHParseDisplayName_I(FileName, NULL, &item, 0, NULL)))
        {
            SHOpenFolderAndSelectItems_I(item, 0, NULL, 0);
            CoTaskMemFree(item);
        }
        else
        {
            PhShowError2(WindowHandle, L"The location could not be found.", L"%s", FileName);
        }
    }
    else
    {
        PPH_STRING selectFileName;

        selectFileName = PhConcatStrings2(L"/select,", FileName);
        PhShellExecute(WindowHandle, L"explorer.exe", selectFileName->Buffer);
        PhDereferenceObject(selectFileName);
    }
}

/**
 * Shows properties for a file.
 *
 * \param WindowHandle A handle to the parent window.
 * \param FileName A file name.
 */
VOID PhShellProperties(
    _In_ HWND WindowHandle,
    _In_ PCWSTR FileName
    )
{
    SHELLEXECUTEINFO info;

    memset(&info, 0, sizeof(SHELLEXECUTEINFO));
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.lpFile = FileName;
    info.nShow = SW_SHOW;
    info.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"properties";
    info.hwnd = WindowHandle;

    if (!PhShellExecuteWin32(&info))
    {
        PhShowStatus(WindowHandle, L"Unable to execute the program.", 0, PhGetLastError());
    }
}

/**
 * Sends a message to the taskbar's notification area.
 *
 * \param Message A value that specifies the action to be taken by this function.
 * \param NotifyIconData A pointer to a NOTIFYICONDATA structure. The content of the structure depends on the value of Message.
 *
 * \return Successful or errant status.
 */
BOOLEAN PhShellNotifyIcon(
    _In_ ULONG Message,
    _In_ PNOTIFYICONDATAW NotifyIconData
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&Shell_NotifyIconW) Shell_NotifyIconW_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            Shell_NotifyIconW_I = PhGetDllBaseProcedureAddress(baseAddress, "Shell_NotifyIconW", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!Shell_NotifyIconW_I)
        return FALSE;

    return !!Shell_NotifyIconW_I(Message, NotifyIconData);
}

HRESULT PhShellGetKnownFolderPath(
    _In_ PCGUID rfid,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Outptr_ PWSTR* FolderPath
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&SHGetKnownFolderPath) SHGetKnownFolderPath_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            SHGetKnownFolderPath_I = PhGetDllBaseProcedureAddress(baseAddress, "SHGetKnownFolderPath", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!SHGetKnownFolderPath_I)
        return E_FAIL;

    return SHGetKnownFolderPath_I(rfid, Flags, TokenHandle, FolderPath);
}

HRESULT PhShellGetKnownFolderItem(
    _In_ PCGUID rfid,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_ REFIID riid,
    _Outptr_ PVOID* ppv
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&SHGetKnownFolderItem) SHGetKnownFolderItem_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            SHGetKnownFolderItem_I = PhGetDllBaseProcedureAddress(baseAddress, "SHGetKnownFolderItem", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!SHGetKnownFolderItem_I)
        return E_FAIL;

    return SHGetKnownFolderItem_I(rfid, Flags, TokenHandle, riid, ppv);
}

/**
 * Expands registry name abbreviations.
 *
 * \param KeyName The key name.
 * \param Computer TRUE to prepend "Computer" or "My Computer" for use with the Registry Editor.
 */
PPH_STRING PhExpandKeyName(
    _In_ PPH_STRING KeyName,
    _In_ BOOLEAN Computer
    )
{
    PPH_STRING keyName;
    PPH_STRING tempString;

    if (PhStartsWithString2(KeyName, L"HKCU", TRUE))
    {
        keyName = PhConcatStrings2(L"HKEY_CURRENT_USER", &KeyName->Buffer[4]);
    }
    else if (PhStartsWithString2(KeyName, L"HKU", TRUE))
    {
        keyName = PhConcatStrings2(L"HKEY_USERS", &KeyName->Buffer[3]);
    }
    else if (PhStartsWithString2(KeyName, L"HKCR", TRUE))
    {
        keyName = PhConcatStrings2(L"HKEY_CLASSES_ROOT", &KeyName->Buffer[4]);
    }
    else if (PhStartsWithString2(KeyName, L"HKLM", TRUE))
    {
        keyName = PhConcatStrings2(L"HKEY_LOCAL_MACHINE", &KeyName->Buffer[4]);
    }
    else
    {
        PhSetReference(&keyName, KeyName);
    }

    if (Computer)
    {
        tempString = PhConcatStrings2(L"Computer\\", keyName->Buffer);
        PhDereferenceObject(keyName);
        keyName = tempString;
    }

    return keyName;
}

/**
 * Gets a registry string value.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 * \return A pointer to a string containing the value, or NULL if the function failed. You must free
 * the string using PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName
    )
{
    PPH_STRING string = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;

    if (NT_SUCCESS(PhQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, &buffer)))
    {
        if (buffer->Type == REG_SZ ||
            buffer->Type == REG_MULTI_SZ ||
            buffer->Type == REG_EXPAND_SZ)
        {
            if (!(buffer->DataLength & 1)) // validate the string length
            {
                if (buffer->DataLength >= sizeof(UNICODE_NULL))
                    string = PhCreateStringEx((PWCHAR)buffer->Data, buffer->DataLength - sizeof(UNICODE_NULL));
                else
                    string = PhReferenceEmptyString();
            }
        }

        PhFree(buffer);
    }

    return string;
}

ULONG PhQueryRegistryUlong(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName
    )
{
    ULONG ulong = ULONG_MAX;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;

    if (NT_SUCCESS(PhQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, &buffer)))
    {
        if (buffer->Type == REG_DWORD)
        {
            if (buffer->DataLength == sizeof(ULONG))
                ulong = *(PULONG)buffer->Data;
        }

        PhFree(buffer);
    }

    return ulong;
}

ULONG64 PhQueryRegistryUlong64(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName
    )
{
    ULONG64 ulong64 = ULLONG_MAX;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;

    if (NT_SUCCESS(PhQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, &buffer)))
    {
        if (buffer->Type == REG_DWORD)
        {
            if (buffer->DataLength == sizeof(ULONG))
                ulong64 = *(PULONG)buffer->Data;
        }
        else if (buffer->Type == REG_QWORD)
        {
            if (buffer->DataLength == sizeof(ULONG64))
                ulong64 = *(PULONG64)buffer->Data;
        }

        PhFree(buffer);
    }

    return ulong64;
}

VOID PhMapFlags1(
    _Inout_ PULONG Value2,
    _In_ ULONG Value1,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    )
{
    ULONG i;
    ULONG value2;

    value2 = *Value2;

    if (value2 != 0)
    {
        // There are existing flags. Map the flags we know about by clearing/setting them. The flags
        // we don't know about won't be affected.

        for (i = 0; i < NumberOfMappings; i++)
        {
            if (Value1 & Mappings[i].Flag1)
                value2 |= Mappings[i].Flag2;
            else
                value2 &= ~Mappings[i].Flag2;
        }
    }
    else
    {
        // There are no existing flags, which means we can build the value from scratch, with no
        // clearing needed.

        for (i = 0; i < NumberOfMappings; i++)
        {
            if (Value1 & Mappings[i].Flag1)
                value2 |= Mappings[i].Flag2;
        }
    }

    *Value2 = value2;
}

VOID PhMapFlags2(
    _Inout_ PULONG Value1,
    _In_ ULONG Value2,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    )
{
    ULONG i;
    ULONG value1;

    value1 = *Value1;

    if (value1 != 0)
    {
        for (i = 0; i < NumberOfMappings; i++)
        {
            if (Value2 & Mappings[i].Flag2)
                value1 |= Mappings[i].Flag1;
            else
                value1 &= ~Mappings[i].Flag1;
        }
    }
    else
    {
        for (i = 0; i < NumberOfMappings; i++)
        {
            if (Value2 & Mappings[i].Flag2)
                value1 |= Mappings[i].Flag1;
        }
    }

    *Value1 = value1;
}

UINT_PTR CALLBACK PhpOpenFileNameHookProc(
    _In_ HWND hdlg,
    _In_ UINT uiMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uiMsg)
    {
    case WM_NOTIFY:
        {
            LPOFNOTIFY header = (LPOFNOTIFY)lParam;

            // We can't use CDN_FILEOK because it's not sent if the buffer is too small, defeating
            // the entire purpose of this callback function.

            switch (header->hdr.code)
            {
            case CDN_SELCHANGE:
                {
                    ULONG returnLength;

                    returnLength = CommDlg_OpenSave_GetFilePath(
                        header->hdr.hwndFrom,
                        header->lpOFN->lpstrFile,
                        header->lpOFN->nMaxFile
                        );

                    if ((LONG)returnLength > 0 && returnLength > header->lpOFN->nMaxFile)
                    {
                        PhFree(header->lpOFN->lpstrFile);
                        header->lpOFN->nMaxFile = returnLength + 0x200; // pre-allocate some more
                        header->lpOFN->lpstrFile = PhAllocate(header->lpOFN->nMaxFile * sizeof(WCHAR));

                        returnLength = CommDlg_OpenSave_GetFilePath(
                            header->hdr.hwndFrom,
                            header->lpOFN->lpstrFile,
                            header->lpOFN->nMaxFile
                            );
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

OPENFILENAME *PhpCreateOpenFileName(
    VOID
    )
{
    OPENFILENAME *ofn;

    ofn = PhAllocateZero(sizeof(OPENFILENAME));
    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->nMaxFile = 0x400;
    ofn->lpstrFile = PhAllocate(ofn->nMaxFile * sizeof(WCHAR));
    ofn->lpstrFileTitle = NULL;
    ofn->Flags = OFN_ENABLEHOOK | OFN_EXPLORER;
    ofn->lpfnHook = PhpOpenFileNameHookProc;

    ofn->lpstrFile[0] = UNICODE_NULL;

    return ofn;
}

VOID PhpFreeOpenFileName(
    _In_ OPENFILENAME *OpenFileName
    )
{
    if (OpenFileName->lpstrFilter) PhFree((PVOID)OpenFileName->lpstrFilter);
    if (OpenFileName->lpstrFile) PhFree((PVOID)OpenFileName->lpstrFile);

    PhFree(OpenFileName);
}

typedef struct _PHP_FILE_DIALOG
{
    BOOLEAN UseIFileDialog;
    BOOLEAN Save;
    union
    {
        OPENFILENAME *OpenFileName;
        IFileDialog *FileDialog;
    } u;
} PHP_FILE_DIALOG, *PPHP_FILE_DIALOG;

PPHP_FILE_DIALOG PhpCreateFileDialog(
    _In_ BOOLEAN Save,
    _In_opt_ OPENFILENAME *OpenFileName,
    _In_opt_ IFileDialog *FileDialog
    )
{
    PPHP_FILE_DIALOG fileDialog;

    assert(!!OpenFileName != !!FileDialog);
    fileDialog = PhAllocate(sizeof(PHP_FILE_DIALOG));
    fileDialog->Save = Save;

    if (OpenFileName)
    {
        fileDialog->UseIFileDialog = FALSE;
        fileDialog->u.OpenFileName = OpenFileName;
    }
    else if (FileDialog)
    {
        fileDialog->UseIFileDialog = TRUE;
        fileDialog->u.FileDialog = FileDialog;
    }
    else
    {
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    return fileDialog;
}

/**
 * Creates a file dialog for the user to select a file to open.
 *
 * \return An opaque pointer representing the file dialog. You must free the file dialog using
 * PhFreeFileDialog() when you no longer need it.
 */
PVOID PhCreateOpenFileDialog(
    VOID
    )
{
    IFileDialog *fileDialog;

    if (SUCCEEDED(PhGetClassObject(
        L"comdlg32.dll",
        &CLSID_FileOpenDialog,
        &IID_IFileDialog,
        &fileDialog
        )))
    {
        // The default options are fine.
        return PhpCreateFileDialog(FALSE, NULL, fileDialog);
    }

    return NULL;
}

/**
 * Creates a file dialog for the user to select a file to save to.
 *
 * \return An opaque pointer representing the file dialog. You must free the file dialog using
 * PhFreeFileDialog() when you no longer need it.
 */
PVOID PhCreateSaveFileDialog(
    VOID
    )
{
    IFileDialog *fileDialog;

    if (SUCCEEDED(PhGetClassObject(
        L"comdlg32.dll",
        &CLSID_FileSaveDialog,
        &IID_IFileDialog,
        &fileDialog
        )))
    {
        // The default options are fine.
        return PhpCreateFileDialog(TRUE, NULL, fileDialog);
    }

    return NULL;
}

/**
 * Frees a file dialog.
 *
 * \param FileDialog The file dialog.
 */
VOID PhFreeFileDialog(
    _In_ PVOID FileDialog
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        IFileDialog_Release(fileDialog->u.FileDialog);
    }
    else
    {
        PhpFreeOpenFileName(fileDialog->u.OpenFileName);
    }

    PhFree(fileDialog);
}

/**
 * Shows a file dialog to the user.
 *
 * \param WindowHandle A handle to the parent window.
 * \param FileDialog The file dialog.
 *
 * \return TRUE if the user selected a file, FALSE if the user cancelled the operation or an error
 * occurred.
 */
BOOLEAN PhShowFileDialog(
    _In_opt_ HWND WindowHandle,
    _In_ PVOID FileDialog
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        // Set a blank default extension. This will have an effect when the user selects a different
        // file type.
        IFileDialog_SetDefaultExtension(fileDialog->u.FileDialog, L"");

        return SUCCEEDED(IFileDialog_Show(fileDialog->u.FileDialog, WindowHandle));
    }
    else
    {
        OPENFILENAME *ofn = fileDialog->u.OpenFileName;

        ofn->hwndOwner = WindowHandle;

        // Determine whether the structure represents a open or save dialog and call the appropriate
        // function.
        if (!fileDialog->Save)
        {
            return GetOpenFileName(ofn);
        }
        else
        {
            return GetSaveFileName(ofn);
        }
    }
}

static const PH_FLAG_MAPPING PhpFileDialogIfdMappings[] =
{
    { PH_FILEDIALOG_CREATEPROMPT, FOS_CREATEPROMPT },
    { PH_FILEDIALOG_PATHMUSTEXIST, FOS_PATHMUSTEXIST },
    { PH_FILEDIALOG_FILEMUSTEXIST, FOS_FILEMUSTEXIST },
    { PH_FILEDIALOG_SHOWHIDDEN, FOS_FORCESHOWHIDDEN },
    { PH_FILEDIALOG_NODEREFERENCELINKS, FOS_NODEREFERENCELINKS },
    { PH_FILEDIALOG_OVERWRITEPROMPT, FOS_OVERWRITEPROMPT },
    { PH_FILEDIALOG_DEFAULTEXPANDED, FOS_DEFAULTNOMINIMODE },
    { PH_FILEDIALOG_STRICTFILETYPES, FOS_STRICTFILETYPES },
    { PH_FILEDIALOG_PICKFOLDERS, FOS_PICKFOLDERS },
    { PH_FILEDIALOG_NOPATHVALIDATE, FOS_NOVALIDATE },
    { PH_FILEDIALOG_DONTADDTORECENT, FOS_DONTADDTORECENT },
};

static const PH_FLAG_MAPPING PhpFileDialogOfnMappings[] =
{
    { PH_FILEDIALOG_CREATEPROMPT, OFN_CREATEPROMPT },
    { PH_FILEDIALOG_PATHMUSTEXIST, OFN_PATHMUSTEXIST },
    { PH_FILEDIALOG_FILEMUSTEXIST, OFN_FILEMUSTEXIST },
    { PH_FILEDIALOG_SHOWHIDDEN, OFN_FORCESHOWHIDDEN },
    { PH_FILEDIALOG_NODEREFERENCELINKS, OFN_NODEREFERENCELINKS },
    { PH_FILEDIALOG_OVERWRITEPROMPT, OFN_OVERWRITEPROMPT },
    { PH_FILEDIALOG_NOPATHVALIDATE, OFN_NOVALIDATE },
    { PH_FILEDIALOG_DONTADDTORECENT, OFN_DONTADDTORECENT },
};

/**
 * Gets the options for a file dialog.
 *
 * \param FileDialog The file dialog.
 *
 * \return The currently enabled options. See the documentation for PhSetFileDialogOptions() for
 * details.
 */
ULONG PhGetFileDialogOptions(
    _In_ PVOID FileDialog
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        FILEOPENDIALOGOPTIONS dialogOptions;
        ULONG options;

        if (SUCCEEDED(IFileDialog_GetOptions(fileDialog->u.FileDialog, &dialogOptions)))
        {
            options = 0;

            PhMapFlags2(
                &options,
                dialogOptions,
                PhpFileDialogIfdMappings,
                sizeof(PhpFileDialogIfdMappings) / sizeof(PH_FLAG_MAPPING)
                );

            return options;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        OPENFILENAME *ofn = fileDialog->u.OpenFileName;
        ULONG options;

        options = 0;

        PhMapFlags2(
            &options,
            ofn->Flags,
            PhpFileDialogOfnMappings,
            sizeof(PhpFileDialogOfnMappings) / sizeof(PH_FLAG_MAPPING)
            );

        return options;
    }
}

/**
 * Sets the options for a file dialog.
 *
 * \param FileDialog The file dialog.
 * \param Options A combination of flags specifying the options.
 * \li \c PH_FILEDIALOG_CREATEPROMPT A prompt for creation will be displayed when the selected item
 * does not exist. This is only valid for Save dialogs.
 * \li \c PH_FILEDIALOG_PATHMUSTEXIST The selected item must be in an existing folder. This is
 * enabled by default.
 * \li \c PH_FILEDIALOG_FILEMUSTEXIST The selected item must exist. This is enabled by default and
 * is only valid for Open dialogs.
 * \li \c PH_FILEDIALOG_SHOWHIDDEN Items with the System and Hidden attributes will be displayed.
 * \li \c PH_FILEDIALOG_NODEREFERENCELINKS Shortcuts will not be followed, allowing .lnk files to be
 * opened.
 * \li \c PH_FILEDIALOG_OVERWRITEPROMPT An overwrite prompt will be displayed if an existing item is
 * selected. This is enabled by default and is only valid for Save dialogs.
 * \li \c PH_FILEDIALOG_DEFAULTEXPANDED The file dialog should be expanded by default (i.e. the
 * folder browser should be displayed). This is only valid for Save dialogs.
 */
VOID PhSetFileDialogOptions(
    _In_ PVOID FileDialog,
    _In_ ULONG Options
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        FILEOPENDIALOGOPTIONS dialogOptions;

        if (SUCCEEDED(IFileDialog_GetOptions(fileDialog->u.FileDialog, &dialogOptions)))
        {
            PhMapFlags1(
                &dialogOptions,
                Options,
                PhpFileDialogIfdMappings,
                sizeof(PhpFileDialogIfdMappings) / sizeof(PH_FLAG_MAPPING)
                );

            IFileDialog_SetOptions(fileDialog->u.FileDialog, dialogOptions);
        }
    }
    else
    {
        OPENFILENAME *ofn = fileDialog->u.OpenFileName;

        PhMapFlags1(
            &ofn->Flags,
            Options,
            PhpFileDialogOfnMappings,
            sizeof(PhpFileDialogOfnMappings) / sizeof(PH_FLAG_MAPPING)
            );
    }
}

/**
 * Gets the index of the currently selected file type filter for a file dialog.
 *
 * \param FileDialog The file dialog.
 *
 * \return The one-based index of the selected file type, or 0 if an error occurred.
 */
ULONG PhGetFileDialogFilterIndex(
    _In_ PVOID FileDialog
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        ULONG index;

        if (SUCCEEDED(IFileDialog_GetFileTypeIndex(fileDialog->u.FileDialog, &index)))
        {
            return index;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        OPENFILENAME *ofn = fileDialog->u.OpenFileName;

        return ofn->nFilterIndex;
    }
}

/**
 * Sets the file type filter for a file dialog.
 *
 * \param FileDialog The file dialog.
 * \param Filters A pointer to an array of file type structures.
 * \param NumberOfFilters The number of file types.
 */
VOID PhSetFileDialogFilter(
    _In_ PVOID FileDialog,
    _In_ PPH_FILETYPE_FILTER Filters,
    _In_ ULONG NumberOfFilters
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        IFileDialog_SetFileTypes(
            fileDialog->u.FileDialog,
            NumberOfFilters,
            (COMDLG_FILTERSPEC *)Filters
            );
    }
    else
    {
        OPENFILENAME *ofn = fileDialog->u.OpenFileName;
        PPH_STRING filterString;
        PH_STRING_BUILDER filterBuilder;
        ULONG i;

        PhInitializeStringBuilder(&filterBuilder, 10);

        for (i = 0; i < NumberOfFilters; i++)
        {
            PhAppendStringBuilder2(&filterBuilder, Filters[i].Name);
            PhAppendCharStringBuilder(&filterBuilder, 0);
            PhAppendStringBuilder2(&filterBuilder, Filters[i].Filter);
            PhAppendCharStringBuilder(&filterBuilder, 0);
        }

        filterString = PhFinalStringBuilderString(&filterBuilder);

        if (ofn->lpstrFilter)
            PhFree((PVOID)ofn->lpstrFilter);

        ofn->lpstrFilter = PhAllocateCopy(filterString->Buffer, filterString->Length + sizeof(UNICODE_NULL));
        PhDereferenceObject(filterString);
    }
}

/**
 * Gets the file name selected in a file dialog.
 *
 * \param FileDialog The file dialog.
 *
 * \return A pointer to a string containing the file name. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhGetFileDialogFileName(
    _In_ PVOID FileDialog
    )
{
    PPHP_FILE_DIALOG fileDialog = FileDialog;

    if (fileDialog->UseIFileDialog)
    {
        IShellItem *result;
        PPH_STRING fileName = NULL;

        if (SUCCEEDED(IFileDialog_GetResult(fileDialog->u.FileDialog, &result)))
        {
            PWSTR name;

            if (SUCCEEDED(IShellItem_GetDisplayName(result, SIGDN_FILESYSPATH, &name)))
            {
                fileName = PhCreateString(name);
                CoTaskMemFree(name);
            }

            IShellItem_Release(result);
        }

        if (PhIsNullOrEmptyString(fileName))
        {
            PWSTR name;

            if (SUCCEEDED(IFileDialog_GetFileName(fileDialog->u.FileDialog, &name)))
            {
                fileName = PhCreateString(name);
                CoTaskMemFree(name);
            }
        }

        if (PhIsNullOrEmptyString(fileName))
        {
            // NOTE: When the user selects IShellItem types with SIGDN_URL from the OpenFileDialog/SaveFileDialog
            // the shell returns S_OK for the file dialog then immediately returns E_FAIL when we query the filename.
            // This case was unexpected, and plugins/legacy callers will auto-dereference the filename when the dialog
            // returns success, so return an empty string to prevent plugins/legacy callers referencing invalid memory. (dmex)
            PhMoveReference(&fileName, PhReferenceEmptyString());
        }

        return fileName;
    }
    else
    {
        return PhCreateString(fileDialog->u.OpenFileName->lpstrFile);
    }
}

/**
 * Sets the file name of a file dialog.
 *
 * \param FileDialog The file dialog.
 * \param FileName The new file name.
 */
VOID PhSetFileDialogFileName(
    _In_ PVOID FileDialog,
    _In_ PCWSTR FileName
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&SHParseDisplayName) SHParseDisplayName_I = NULL;
    static typeof(&SHCreateItemFromIDList) SHCreateItemFromIDList_I = NULL;
    PPHP_FILE_DIALOG fileDialog = FileDialog;
    PH_STRINGREF fileName;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            SHParseDisplayName_I = PhGetDllBaseProcedureAddress(baseAddress, "SHParseDisplayName", 0);
            SHCreateItemFromIDList_I = PhGetDllBaseProcedureAddress(baseAddress, "SHCreateItemFromIDList", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    PhInitializeStringRefLongHint(&fileName, FileName);

    if (fileDialog->UseIFileDialog)
    {
        IShellItem *shellItem = NULL;
        PH_STRINGREF pathNamePart;
        PH_STRINGREF baseNamePart;

        if (PhGetBasePath(&fileName, &pathNamePart, &baseNamePart) && SHParseDisplayName_I && SHCreateItemFromIDList_I)
        {
            LPITEMIDLIST item;
            PPH_STRING pathName;

            pathName = PhCreateString2(&pathNamePart);

            if (SUCCEEDED(SHParseDisplayName_I(pathName->Buffer, NULL, &item, 0, NULL)))
            {
                SHCreateItemFromIDList_I(item, &IID_IShellItem, &shellItem);
                CoTaskMemFree(item);
            }

            PhDereferenceObject(pathName);
        }

        if (shellItem)
        {
            IFileDialog_SetFolder(fileDialog->u.FileDialog, shellItem);
            IFileDialog_SetFileName(fileDialog->u.FileDialog, baseNamePart.Buffer);
            IShellItem_Release(shellItem);
        }
        else
        {
            IFileDialog_SetFileName(fileDialog->u.FileDialog, FileName);
        }
    }
    else
    {
        OPENFILENAME *ofn = fileDialog->u.OpenFileName;

        if (PhFindCharInStringRef(&fileName, L'/', FALSE) != SIZE_MAX || PhFindCharInStringRef(&fileName, L'\"', FALSE) != SIZE_MAX)
        {
            // It refuses to take any filenames with a slash or quotation mark.
            return;
        }

        PhFree(ofn->lpstrFile);

        ofn->nMaxFile = (ULONG)__max(fileName.Length / sizeof(WCHAR) + 1, 0x400);
        ofn->lpstrFile = PhAllocate(ofn->nMaxFile * sizeof(WCHAR));
        memcpy(ofn->lpstrFile, fileName.Buffer, fileName.Length + sizeof(UNICODE_NULL));
    }
}

/**
 * Determines if an executable image is packed.
 *
 * \param FileName The file name of the image.
 * \param IsPacked A variable that receives TRUE if the image is packed, otherwise FALSE.
 * \param NumberOfModules A variable that receives the number of DLLs that the image imports
 * functions from.
 * \param NumberOfFunctions A variable that receives the number of functions that the image imports.
 */
NTSTATUS PhIsExecutablePacked(
    _In_ PPH_STRING FileName,
    _Out_ PBOOLEAN IsPacked,
    _Out_opt_ PULONG NumberOfModules,
    _Out_opt_ PULONG NumberOfFunctions
    )
{
    // An image is packed if:
    //
    // 1. It references fewer than 3 modules, and
    // 2. It imports fewer than 5 functions, and
    // 3. It does not use the Native subsystem.
    //
    // Or:
    //
    // 1. The function-to-module ratio is lower than 3 (on average fewer than 3 functions are
    //    imported from each module), and
    // 2. It references more than 2 modules but fewer than 6 modules.
    //
    // Or:
    //
    // 1. The function-to-module ratio is lower than 2 (on average fewer than 2 functions are
    //    imported from each module), and
    // 2. It references more than 5 modules but fewer than 31 modules.
    //
    // Or:
    //
    // 1. It does not have a section named ".text".
    //
    // An image is not considered to be packed if it has only one import from a module named
    // "mscoree.dll".

    NTSTATUS status;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_IMPORT_DLL importDll;
    ULONG i;
    //ULONG limitNumberOfSections;
    ULONG limitNumberOfModules;
    ULONG numberOfModules;
    ULONG numberOfFunctions = 0;
    BOOLEAN hasTextSection = FALSE;
    BOOLEAN isModuleMscoree = FALSE;
    BOOLEAN isPacked;

    status = PhLoadMappedImageEx(
        &FileName->sr,
        NULL,
        &mappedImage
        );

    if (!NT_SUCCESS(status))
        return status;

    // Go through the sections and look for the ".text" section.

#if defined(PH_ISEXECUTABLEPACKED_SECTION)
    limitNumberOfSections = min(mappedImage.NumberOfSections, 64);

    for (i = 0; i < limitNumberOfSections; i++)
    {
        WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

        if (NT_SUCCESS(PhGetMappedImageSectionName(
            &mappedImage.Sections[i],
            sectionName,
            RTL_NUMBER_OF(sectionName),
            NULL
            )))
        {
            if (PhEqualStringZ(sectionName, ".text", TRUE))
            {
                hasTextSection = TRUE;
                break;
            }
        }
    }
#else
    hasTextSection = TRUE;
#endif

    status = PhGetMappedImageImports(
        &imports,
        &mappedImage
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Get the module and function totals.

    numberOfModules = imports.NumberOfDlls;
    limitNumberOfModules = __min(numberOfModules, 64);

    for (i = 0; i < limitNumberOfModules; i++)
    {
        if (!NT_SUCCESS(status = PhGetMappedImageImportDll(
            &imports,
            i,
            &importDll
            )))
            goto CleanupExit;

        if (PhEqualBytesZ(importDll.Name, "mscoree.dll", TRUE))
            isModuleMscoree = TRUE;

        numberOfFunctions += importDll.NumberOfEntries;
    }

    // Determine if the image is packed.

    if (
        numberOfModules != 0 &&
        (
        // Rule 1
        (numberOfModules < 3 && numberOfFunctions < 5 &&
        mappedImage.NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE) ||
        // Rule 2
        ((numberOfFunctions / numberOfModules) < 3 &&
        numberOfModules > 2 && numberOfModules < 5) ||
        // Rule 3
        ((numberOfFunctions / numberOfModules) < 2 &&
        numberOfModules > 4 && numberOfModules < 31) ||
        // Rule 4
        !hasTextSection
        ) &&
        // Additional .NET rule
        !(numberOfModules == 1 && numberOfFunctions == 1 && isModuleMscoree)
        )
    {
        isPacked = TRUE;
    }
    else
    {
        isPacked = FALSE;
    }

    *IsPacked = isPacked;

    if (NumberOfModules)
        *NumberOfModules = numberOfModules;
    if (NumberOfFunctions)
        *NumberOfFunctions = numberOfFunctions;

CleanupExit:
    PhUnloadMappedImage(&mappedImage);

    return status;
}

/**
 * \brief CRC-32 (Ethernet, ZIP - polynomial 0xedb88320 - TEST=0xEEEA93B8)
 *
 * \param Crc Crc
 * \param Buffer Buffer
 * \param Length Length
 * \return Crc
 */
ULONG PhCrc32(
    _In_ ULONG Crc,
    _In_reads_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length
    )
{
    Crc ^= 0xffffffff;

    while (Length--)
        Crc = (Crc >> 8) ^ PhCrc32Table[(Crc ^ *Buffer++) & 0xff];

    return Crc ^ 0xffffffff;
}

/**
 * \brief CRC-32C (iSCSI - polynomial 0x82f63b78 - TEST=0xEFF7F083)
 *
 * \param Crc Crc
 * \param Buffer Buffer
 * \param Length Length
 * \return Crc
 */
ULONG PhCrc32C(
    _In_ ULONG Crc,
    _In_reads_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    )
{
#if defined(_WIN64)
    SIZE_T u64_blocks = Length / sizeof(ULONG64);
    SIZE_T u64_remaining = Length % sizeof(ULONG64);
#else
    SIZE_T u64_remaining = Length;
#endif
    SIZE_T u32_blocks = u64_remaining / sizeof(ULONG);
    SIZE_T u32_remaining = u64_remaining % sizeof(ULONG);
    SIZE_T u16_blocks = u32_remaining / sizeof(USHORT);
    SIZE_T u8_blocks = u32_remaining % sizeof(UCHAR);

    Crc ^= 0xffffffff;

#if defined(_WIN64)
    while (u64_blocks--)
    {
#if defined(_M_ARM64)
        Crc = __crc32d(Crc, *(PULONG64)Buffer);
#else
        Crc = (ULONG)_mm_crc32_u64(Crc, *(PULONG64)Buffer);
#endif
        Buffer = (PULONG64)Buffer + 1;
    }
#endif

    while (u32_blocks--)
    {
#if defined(_M_ARM64)
        Crc = __crc32w(Crc, *(PULONG)Buffer);
#else
        Crc = _mm_crc32_u32(Crc, *(PULONG)Buffer);
#endif
        Buffer = (PULONG)Buffer + 1;
    }

    while (u16_blocks--)
    {
#if defined(_M_ARM64)
        Crc = __crc32h(Crc, *(PUSHORT)Buffer);
#else
        Crc = _mm_crc32_u16(Crc, *(PUSHORT)Buffer);
#endif
        Buffer = (PUSHORT)Buffer + 1;
    }

    while (u8_blocks--)
    {
#if defined(_M_ARM64)
        Crc = __crc32b(Crc, *(PUCHAR)Buffer);
#else
        Crc = _mm_crc32_u8(Crc, *(PUCHAR)Buffer);
#endif
        Buffer = (PUCHAR)Buffer + 1;
    }

    return Crc ^ 0xffffffff;
}

static_assert(RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(MD5_CTX), "RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(MD5_CTX)");
static_assert(RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(A_SHA_CTX), "RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(A_SHA_CTX)");
static_assert(RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(sha256_context), "RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(sha256_context)");

/**
 * Initializes hashing.
 *
 * \param Context A hashing context structure.
 * \param Algorithm The hash algorithm to use:
 * \li \c Md5HashAlgorithm MD5 (128 bits)
 * \li \c Sha1HashAlgorithm SHA-1 (160 bits)
 * \li \c Crc32HashAlgorithm CRC-32-IEEE 802.3 (32 bits)
 * \li \c Sha256HashAlgorithm SHA-256 (256 bits)
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhInitializeHash(
    _Out_ PPH_HASH_CONTEXT Context,
    _In_ PH_HASH_ALGORITHM Algorithm
    )
{
    Context->Algorithm = Algorithm;

    switch (Algorithm)
    {
    case Crc32HashAlgorithm:
    case Crc32CHashAlgorithm:
        Context->Context[0] = 0;
        return STATUS_SUCCESS;
    case Md5HashAlgorithm:
        MD5Init((MD5_CTX *)Context->Context);
        return STATUS_SUCCESS;
    case Sha1HashAlgorithm:
        A_SHAInit((A_SHA_CTX *)Context->Context);
        return STATUS_SUCCESS;
    case Sha256HashAlgorithm:
        sha256_starts((sha256_context *)Context->Context);
        return STATUS_SUCCESS;
    default:
        return STATUS_INVALID_PARAMETER_2;
    }
}

/**
 * Hashes a block of data.
 *
 * \param Context A hashing context structure.
 * \param Buffer The block of data.
 * \param Length The number of bytes in the block.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhUpdateHash(
    _In_ PPH_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    switch (Context->Algorithm)
    {
    case Crc32HashAlgorithm:
        Context->Context[0] = PhCrc32(Context->Context[0], (PUCHAR)Buffer, Length);
        return STATUS_SUCCESS;
    case Crc32CHashAlgorithm:
        Context->Context[0] = PhCrc32C(Context->Context[0], (PUCHAR)Buffer, Length);
        return STATUS_SUCCESS;
    case Md5HashAlgorithm:
        MD5Update((MD5_CTX *)Context->Context, (PUCHAR)Buffer, Length);
        return STATUS_SUCCESS;
    case Sha1HashAlgorithm:
        A_SHAUpdate((A_SHA_CTX *)Context->Context, (PUCHAR)Buffer, Length);
        return STATUS_SUCCESS;
    case Sha256HashAlgorithm:
        sha256_update((sha256_context *)Context->Context, (PUCHAR)Buffer, Length);
        return STATUS_SUCCESS;
    default:
        return STATUS_INVALID_PARAMETER;
    }
}

/**
 * Computes the final hash value.
 *
 * \param Context A hashing context structure.
 * \param Hash A buffer which receives the final hash value.
 * \param HashLength The size of the buffer, in bytes.
 * \param ReturnLength A variable which receives the required size of the buffer, in bytes.
 */
NTSTATUS PhFinalHash(
    _In_ PPH_HASH_CONTEXT Context,
    _Out_writes_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;

    switch (Context->Algorithm)
    {
    case Crc32HashAlgorithm:
    case Crc32CHashAlgorithm:
        {
            if (HashLength >= PH_HASH_CRC32_LENGTH)
            {
                *(PULONG)Hash = Context->Context[0];
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            if (ReturnLength)
            {
                *ReturnLength = PH_HASH_CRC32_LENGTH;
            }
        }
        break;
    case Md5HashAlgorithm:
        {
            if (HashLength >= PH_HASH_MD5_LENGTH)
            {
                MD5Final((MD5_CTX*)Context->Context);
                memcpy(Hash, ((MD5_CTX*)Context->Context)->digest, PH_HASH_MD5_LENGTH);
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            if (ReturnLength)
            {
                *ReturnLength = PH_HASH_MD5_LENGTH;
            }
        }
        break;
    case Sha1HashAlgorithm:
        {
            if (HashLength >= PH_HASH_SHA1_LENGTH)
            {
                A_SHAFinal((A_SHA_CTX*)Context->Context, (PUCHAR)Hash);
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            if (ReturnLength)
                *ReturnLength = PH_HASH_SHA1_LENGTH;
        }
        break;
    case Sha256HashAlgorithm:
        {
            if (HashLength >= PH_HASH_SHA256_LENGTH)
            {
                sha256_finish((sha256_context*)Context->Context, (PUCHAR)Hash);
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            if (ReturnLength)
            {
                *ReturnLength = PH_HASH_SHA256_LENGTH;
            }
        }
        break;
    default:
        {
            if (ReturnLength)
            {
                *ReturnLength = 0;
            }

            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }

    return status;
}

/**
 * Parses one part of a command line string. Quotation marks and backslashes are handled
 * appropriately.
 *
 * \param CommandLine The entire command line string.
 * \param Index The starting index of the command line part to be parsed. There should be no leading
 * whitespace at this index. The index is updated to point to the end of the command line part.
 */
PPH_STRING PhParseCommandLinePart(
    _In_ PCPH_STRINGREF CommandLine,
    _Inout_ PULONG_PTR Index
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T length;
    SIZE_T i;

    ULONG numberOfBackslashes;
    BOOLEAN inQuote;
    BOOLEAN endOfValue;

    length = CommandLine->Length / sizeof(WCHAR);
    i = *Index;

    // This function follows the rules used by CommandLineToArgvW:
    //
    // * 2n backslashes and a quotation mark produces n backslashes and a quotation mark
    //   (non-literal).
    // * 2n + 1 backslashes and a quotation mark produces n and a quotation mark (literal).
    // * n backslashes and no quotation mark produces n backslashes.

    PhInitializeStringBuilder(&stringBuilder, 0x80);
    numberOfBackslashes = 0;
    inQuote = FALSE;
    endOfValue = FALSE;

    for (; i < length; i++)
    {
        switch (CommandLine->Buffer[i])
        {
        case L'\\':
            numberOfBackslashes++;
            break;
        case L'\"':
            if (numberOfBackslashes != 0)
            {
                if (numberOfBackslashes & 1)
                {
                    numberOfBackslashes /= 2;

                    if (numberOfBackslashes != 0)
                    {
                        PhAppendCharStringBuilder2(&stringBuilder, OBJ_NAME_PATH_SEPARATOR, numberOfBackslashes);
                        numberOfBackslashes = 0;
                    }

                    PhAppendCharStringBuilder(&stringBuilder, L'\"');

                    break;
                }
                else
                {
                    numberOfBackslashes /= 2;
                    PhAppendCharStringBuilder2(&stringBuilder, OBJ_NAME_PATH_SEPARATOR, numberOfBackslashes);
                    numberOfBackslashes = 0;
                }
            }

            if (!inQuote)
                inQuote = TRUE;
            else
                inQuote = FALSE;

            break;
        default:
            if (numberOfBackslashes != 0)
            {
                PhAppendCharStringBuilder2(&stringBuilder, OBJ_NAME_PATH_SEPARATOR, numberOfBackslashes);
                numberOfBackslashes = 0;
            }

            if (CommandLine->Buffer[i] == L' ' && !inQuote)
            {
                endOfValue = TRUE;
            }
            else
            {
                PhAppendCharStringBuilder(&stringBuilder, CommandLine->Buffer[i]);
            }

            break;
        }

        if (endOfValue)
            break;
    }

    *Index = i;

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Parses a command line string.
 *
 * \param CommandLine The command line string.
 * \param Options An array of supported command line options.
 * \param NumberOfOptions The number of elements in \a Options.
 * \param Flags A combination of flags.
 * \li \c PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS Unknown command line options are ignored instead of
 * failing the function.
 * \li \c PH_COMMAND_LINE_IGNORE_FIRST_PART The first part of the command line string is ignored.
 * This is used when the first part of the string contains the executable file name.
 * \param Callback A callback function to execute for each command line option found.
 * \param Context A user-defined value to pass to \a Callback.
 */
BOOLEAN PhParseCommandLine(
    _In_ PCPH_STRINGREF CommandLine,
    _In_opt_ PCPH_COMMAND_LINE_OPTION Options,
    _In_ ULONG NumberOfOptions,
    _In_ ULONG Flags,
    _In_ PPH_COMMAND_LINE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    SIZE_T i;
    SIZE_T j;
    SIZE_T length;
    BOOLEAN cont = TRUE;
    BOOLEAN wasFirst = TRUE;

    PH_STRINGREF optionName;
    PCPH_COMMAND_LINE_OPTION option = NULL;
    PPH_STRING optionValue;

    if (CommandLine->Length == 0)
        return TRUE;

    i = 0;
    length = CommandLine->Length / sizeof(WCHAR);

    while (TRUE)
    {
        // Skip spaces.
        while (i < length && CommandLine->Buffer[i] == L' ')
            i++;

        if (i >= length)
            break;

        if (option &&
            (option->Type == MandatoryArgumentType ||
            (option->Type == OptionalArgumentType && CommandLine->Buffer[i] != L'-')))
        {
            // Read the value and execute the callback function.

            optionValue = PhParseCommandLinePart(CommandLine, &i);
            cont = Callback(option, optionValue, Context);
            PhDereferenceObject(optionValue);

            if (!cont)
                break;

            option = NULL;
        }
        else if (CommandLine->Buffer[i] == L'-')
        {
            ULONG_PTR originalIndex;
            SIZE_T optionNameLength;

            // Read the option (only alphanumeric characters allowed).

            // Skip the dash.
            i++;

            originalIndex = i;

            for (; i < length; i++)
            {
                if (!iswalnum(CommandLine->Buffer[i]) && CommandLine->Buffer[i] != L'-')
                    break;
            }

            optionNameLength = i - originalIndex;

            optionName.Buffer = &CommandLine->Buffer[originalIndex];
            optionName.Length = optionNameLength * sizeof(WCHAR);

            // Take care of any pending optional argument.

            if (option && option->Type == OptionalArgumentType)
            {
                cont = Callback(option, NULL, Context);

                if (!cont)
                    break;

                option = NULL;
            }

            // Find the option descriptor.

            option = NULL;

            for (j = 0; j < NumberOfOptions; j++)
            {
                if (PhEqualStringRef2(&optionName, Options[j].Name, FALSE))
                {
                    option = &Options[j];
                    break;
                }
            }

            if (!option && !(Flags & PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS))
                return FALSE;

            if (option && option->Type == NoArgumentType)
            {
                cont = Callback(option, NULL, Context);

                if (!cont)
                    break;

                option = NULL;
            }

            wasFirst = FALSE;
        }
        else
        {
            PPH_STRING value;

            value = PhParseCommandLinePart(CommandLine, &i);

            if ((Flags & PH_COMMAND_LINE_IGNORE_FIRST_PART) && wasFirst)
            {
                PhDereferenceObject(value);
                value = NULL;
            }

            if (value)
            {
                cont = Callback(NULL, value, Context);
                PhDereferenceObject(value);

                if (!cont)
                    break;
            }

            wasFirst = FALSE;
        }
    }

    if (cont && option && option->Type == OptionalArgumentType)
        Callback(option, NULL, Context);

    return TRUE;
}

/**
 * Escapes a string for use in a command line.
 *
 * \param String The string to escape.
 * \return The escaped string.
 * \remarks Only the double quotation mark is escaped.
 */
PPH_STRING PhEscapeCommandLinePart(
    _In_ PCPH_STRINGREF String
    )
{
    static CONST PH_STRINGREF backslashAndQuote = PH_STRINGREF_INIT(L"\\\"");
    PH_STRING_BUILDER stringBuilder;
    ULONG numberOfBackslashes;
    SIZE_T length;
    ULONG i;

    length = String->Length / sizeof(WCHAR);
    PhInitializeStringBuilder(&stringBuilder, String->Length / sizeof(WCHAR) * 3);
    numberOfBackslashes = 0;

    // Simply replacing " with \" won't work here. See PhParseCommandLinePart for the quoting rules.

    for (i = 0; i < length; i++)
    {
        switch (String->Buffer[i])
        {
        case L'\\':
            numberOfBackslashes++;
            break;
        case L'\"':
            if (numberOfBackslashes != 0)
            {
                PhAppendCharStringBuilder2(&stringBuilder, OBJ_NAME_PATH_SEPARATOR, UInt32x32To64(numberOfBackslashes, 2));
                numberOfBackslashes = 0;
            }

            PhAppendStringBuilder(&stringBuilder, &backslashAndQuote);

            break;
        default:
            if (numberOfBackslashes != 0)
            {
                PhAppendCharStringBuilder2(&stringBuilder, OBJ_NAME_PATH_SEPARATOR, numberOfBackslashes);
                numberOfBackslashes = 0;
            }

            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);

            break;
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Parses a command line string. If the string does not contain quotation marks around the file name
 * part, the function determines the file name to use.
 *
 * \param CommandLine The command line string.
 * \param FileName A variable which receives the part of \a CommandLine that contains the file name.
 * \param Arguments A variable which receives the part of \a CommandLine that contains the arguments.
 * \param FullFileName A variable which receives the full path and file name. This may be NULL if
 * the file was not found.
 */
BOOLEAN PhParseCommandLineFuzzy(
    _In_ PCPH_STRINGREF CommandLine,
    _Out_ PPH_STRINGREF FileName,
    _Out_ PPH_STRINGREF Arguments,
    _Out_opt_ PPH_STRING *FullFileName
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PH_STRINGREF commandLine;
    PH_STRINGREF temp;
    PH_STRINGREF currentPart;
    PH_STRINGREF remainingPart;
    PPH_STRING filePath;

    commandLine = *CommandLine;
    PhTrimStringRef(&commandLine, &whitespace, 0);

    if (commandLine.Length == 0)
    {
        PhInitializeEmptyStringRef(FileName);
        PhInitializeEmptyStringRef(Arguments);

        if (FullFileName)
            *FullFileName = NULL;

        return FALSE;
    }

    if (*commandLine.Buffer == L'"')
    {
        PH_STRINGREF arguments;

        PhSkipStringRef(&commandLine, sizeof(WCHAR));

        // Find the matching quote character and we have our file name.

        if (!PhSplitStringRefAtChar(&commandLine, L'"', &commandLine, &arguments))
        {
            PhSkipStringRef(&commandLine, -(LONG_PTR)sizeof(WCHAR)); // Unskip the initial quote character
            *FileName = commandLine;
            PhInitializeEmptyStringRef(Arguments);

            if (FullFileName)
                *FullFileName = NULL;

            return FALSE;
        }

        PhTrimStringRef(&arguments, &whitespace, PH_TRIM_START_ONLY);
        *FileName = commandLine;
        *Arguments = arguments;

        if (FullFileName)
        {
            PPH_STRING tempCommandLine;

            tempCommandLine = PhCreateString2(&commandLine);

            if (filePath = PhSearchFilePath(tempCommandLine->Buffer, L".exe"))
            {
                *FullFileName = filePath;
            }
            else
            {
                *FullFileName = NULL;
            }

            PhDereferenceObject(tempCommandLine);
        }

        return TRUE;
    }

    // Try to find an existing executable file, starting with the first part of the command line and
    // successively restoring the rest of the command line.
    // For example, in "C:\Program Files\Internet   Explorer\iexplore", we try to match:
    // * "C:\Program"
    // * "C:\Program Files\Internet"
    // * "C:\Program Files\Internet "
    // * "C:\Program Files\Internet  "
    // * "C:\Program Files\Internet   Explorer\iexplore"
    //
    // Note that we do not trim whitespace in each part because filenames can contain trailing
    // whitespace before the extension (e.g. "Internet  .exe").

    temp.Buffer = PhAllocate(commandLine.Length + sizeof(UNICODE_NULL));
    memcpy(temp.Buffer, commandLine.Buffer, commandLine.Length);
    temp.Buffer[commandLine.Length / sizeof(WCHAR)] = UNICODE_NULL;
    temp.Length = commandLine.Length;
    remainingPart = temp;

    while (remainingPart.Length != 0)
    {
        WCHAR originalChar = UNICODE_NULL;
        BOOLEAN found;

        found = PhSplitStringRefAtChar(&remainingPart, L' ', &currentPart, &remainingPart);

        if (found)
        {
            originalChar = *(remainingPart.Buffer - 1);
            *(remainingPart.Buffer - 1) = UNICODE_NULL;
        }

        filePath = PhSearchFilePath(temp.Buffer, L".exe");

        if (found)
        {
            *(remainingPart.Buffer - 1) = originalChar;
        }

        if (filePath)
        {
            FileName->Buffer = commandLine.Buffer;
            FileName->Length = (SIZE_T)PTR_SUB_OFFSET(currentPart.Buffer, temp.Buffer) + currentPart.Length;

            if (Arguments)
            {
                PhTrimStringRef(&remainingPart, &whitespace, PH_TRIM_START_ONLY);

                if (remainingPart.Length)
                {
                    Arguments->Buffer = PTR_ADD_OFFSET(commandLine.Buffer, PTR_SUB_OFFSET(remainingPart.Buffer, temp.Buffer));
                    Arguments->Length = commandLine.Length - (SIZE_T)PTR_SUB_OFFSET(remainingPart.Buffer, temp.Buffer);
                }
                else
                {
                    PhInitializeEmptyStringRef(Arguments);
                }
            }

            if (FullFileName)
                *FullFileName = filePath;
            else
                PhDereferenceObject(filePath);

            PhFree(temp.Buffer);

            return TRUE;
        }
    }

    PhFree(temp.Buffer);

    *FileName = *CommandLine;
    PhInitializeEmptyStringRef(Arguments);

    if (FullFileName)
        *FullFileName = NULL;

    return FALSE;
}

/**
 * Parses a Unicode command line string and returns an array of pointers to the command line arguments, along with a count of such arguments.
 *
 * \param CommandLine Pointer to a null-terminated Unicode string that contains the full command line. If this parameter is an empty string the function returns the path to the current executable file.
 * \param NumberOfArguments The number of array elements returned, similar to argc.
 * \return A pointer to an array of LPWSTR values, similar to argv. If the function fails, the return value is NULL.
 */
_Success_(return != NULL)
PWSTR* PhCommandLineToArgv(
    _In_ PCWSTR CommandLine,
    _Out_ PLONG NumberOfArguments
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&CommandLineToArgvW) CommandLineToArgvW_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"shell32.dll"))
        {
            CommandLineToArgvW_I = PhGetDllBaseProcedureAddress(baseAddress, "CommandLineToArgvW", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!CommandLineToArgvW_I)
        return NULL;

    return CommandLineToArgvW_I(CommandLine, NumberOfArguments);
}

PPH_LIST PhCommandLineToList(
    _In_ PCWSTR CommandLine
    )
{
    PPH_LIST commandLineList = NULL;
    LONG commandLineCount;
    PWSTR* commandLineArray;

    if (commandLineArray = PhCommandLineToArgv(CommandLine, &commandLineCount))
    {
        commandLineList = PhCreateList(commandLineCount);

        for (LONG i = 0; i < commandLineCount; i++)
            PhAddItemList(commandLineList, PhCreateString(commandLineArray[i]));

        LocalFree(commandLineArray);
    }

    return commandLineList;
}

PPH_STRING PhCommandLineQuoteSpaces(
    _In_ PCPH_STRINGREF CommandLine
    )
{
    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");
    static CONST PH_STRINGREF space = PH_STRINGREF_INIT(L" ");
    PH_STRINGREF commandLineFileName;
    PH_STRINGREF commandLineArguments;
    PPH_STRING fileNameEscaped;
    PPH_STRING argumentsEscaped;

    if (!PhParseCommandLineFuzzy(CommandLine, &commandLineFileName, &commandLineArguments, NULL))
        return NULL;

    fileNameEscaped = PhConcatStringRef3(&separator, &commandLineFileName, &separator);

    if (commandLineArguments.Length)
    {
        argumentsEscaped = PhConcatStringRef3(&separator, &commandLineArguments, &separator);
        PhMoveReference(&argumentsEscaped, PhConcatStringRef3(&fileNameEscaped->sr, &space, &argumentsEscaped->sr));
        PhMoveReference(&fileNameEscaped, PhConcatStringRef3(&separator, &argumentsEscaped->sr, &separator));
    }

    return fileNameEscaped;
}

PPH_STRING PhSearchFilePath(
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Extension
    )
{
    PPH_STRING fullPath;
    ULONG bufferSize;
    ULONG returnLength;
    FILE_BASIC_INFORMATION basicInfo;

    bufferSize = MAX_PATH;
    fullPath = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));

    returnLength = SearchPath(
        NULL,
        FileName,
        Extension,
        (ULONG)fullPath->Length / sizeof(WCHAR),
        fullPath->Buffer,
        NULL
        );

    if (returnLength == 0 && returnLength <= bufferSize)
        goto CleanupExit;

    if (returnLength > bufferSize)
    {
        bufferSize = returnLength;
        PhDereferenceObject(fullPath);
        fullPath = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));

        returnLength = SearchPath(
            NULL,
            FileName,
            Extension,
            (ULONG)fullPath->Length / sizeof(WCHAR),
            fullPath->Buffer,
            NULL
            );
    }

    if (returnLength == 0 && returnLength <= bufferSize)
        goto CleanupExit;

    PhTrimToNullTerminatorString(fullPath);

    // Make sure this is not a directory.

    if (!NT_SUCCESS(PhQueryAttributesFileWin32(fullPath->Buffer, &basicInfo)))
        goto CleanupExit;

    if (FlagOn(basicInfo.FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        goto CleanupExit;

    return fullPath;

CleanupExit:
    PhDereferenceObject(fullPath);
    return NULL;
}

//PPH_STRING PhCreateCacheFile(
//    _In_ PPH_STRING FileName
//    )
//{
//    PPH_STRING tempFileName;
//    PPH_STRING cachefileName;
//
//    if (tempFileName = PhGetTemporaryDirectoryRandomAlphaFileName())
//    {
//        cachefileName = PhConcatStringRef3(
//            &tempFileName->sr,
//            &PhNtPathSeparatorString,
//            &FileName->sr
//            );
//
//        if (NT_SUCCESS(PhCreateDirectoryFullPathWin32(&cachefileName->sr)))
//        {
//            PhDereferenceObject(tempFileName);
//            return cachefileName;
//        }
//
//        PhDereferenceObject(cachefileName);
//        PhDereferenceObject(tempFileName);
//    }
//
//    return NULL;
//}

PPH_STRING PhCreateCacheFile(
    _In_ BOOLEAN PortableDirectory,
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    static CONST PH_STRINGREF settingsDirectory = PH_STRINGREF_INIT(L"cache");
    PPH_STRING cacheDirectory;
    PPH_STRING cacheFilePath;
    PH_STRINGREF randomAlphaStringRef;
    WCHAR randomAlphaString[32] = L"";

    if (PortableDirectory)
        cacheDirectory = PhGetApplicationDirectoryFileName(&settingsDirectory, TRUE);
    else
        cacheDirectory = PhGetRoamingAppDataDirectory(&settingsDirectory, TRUE);

    if (PhIsNullOrEmptyString(cacheDirectory))
        return NULL;

    PhGenerateRandomAlphaString(randomAlphaString, RTL_NUMBER_OF(randomAlphaString));
    randomAlphaStringRef.Buffer = randomAlphaString;
    randomAlphaStringRef.Length = sizeof(randomAlphaString) - sizeof(UNICODE_NULL);

    cacheFilePath = PhConcatStringRef3(
        &cacheDirectory->sr,
        &PhNtPathSeparatorString,
        &randomAlphaStringRef
        );

    PhMoveReference(&cacheFilePath, PhConcatStringRef3(
        &cacheFilePath->sr,
        &PhNtPathSeparatorString,
        &FileName->sr
        ));

    if (!NT_SUCCESS(PhCreateDirectoryFullPath(&cacheFilePath->sr)))
    {
        PhDereferenceObject(cacheFilePath);
        PhDereferenceObject(cacheDirectory);
        return NULL;
    }

    if (NativeFileName)
    {
        PPH_STRING cacheFileName;

        if (cacheFileName = PhDosPathNameToNtPathName(&cacheFilePath->sr))
        {
            PhMoveReference(&cacheFilePath, cacheFileName);
        }
    }
    else
    {
        PPH_STRING cacheFileName;

        if (cacheFileName = PhResolveDevicePrefix(&cacheFilePath->sr))
        {
            PhMoveReference(&cacheFilePath, cacheFileName);
        }
    }

    PhDereferenceObject(cacheDirectory);

    return cacheFilePath;
}

//VOID PhDeleteCacheFile(
//    _In_ PPH_STRING FileName
//    )
//{
//    PPH_STRING cacheDirectory;
//
//    if (PhDoesFileExistWin32(PhGetString(FileName)))
//    {
//        PhDeleteFileWin32(PhGetString(FileName));
//    }
//
//    if (cacheDirectory = PhGetBaseDirectory(FileName))
//    {
//        PhDeleteDirectoryWin32(&cacheDirectory->sr);
//        PhDereferenceObject(cacheDirectory);
//    }
//}

VOID PhDeleteCacheFile(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    if (NativeFileName)
    {
        if (PhDoesFileExist(&FileName->sr))
        {
            PhDeleteDirectoryFullPath(&FileName->sr);
        }
    }
    else
    {
        PPH_STRING cacheDirectory;
        PPH_STRING cacheFullFilePath;
        ULONG indexOfFileName = ULONG_MAX;

        if (PhDoesFileExistWin32(PhGetString(FileName)))
        {
            PhDeleteFileWin32(PhGetString(FileName));
        }

        if (NT_SUCCESS(PhGetFullPath(PhGetString(FileName), &cacheFullFilePath, &indexOfFileName)))
        {
            if (indexOfFileName != ULONG_MAX && (cacheDirectory = PhSubstring(cacheFullFilePath, 0, indexOfFileName)))
            {
                PhDeleteDirectoryWin32(&cacheDirectory->sr);

                PhDereferenceObject(cacheDirectory);
            }

            PhDereferenceObject(cacheFullFilePath);
        }
    }
}

VOID PhClearCacheDirectory(
    _In_ BOOLEAN PortableDirectory
    )
{
    static CONST PH_STRINGREF settingsDirectory = PH_STRINGREF_INIT(L"cache");
    PPH_STRING cacheDirectory;

    if (PortableDirectory)
        cacheDirectory = PhGetApplicationDirectoryFileName(&settingsDirectory, TRUE);
    else
        cacheDirectory = PhGetRoamingAppDataDirectory(&settingsDirectory, TRUE);

    if (cacheDirectory)
    {
        PhDeleteDirectory(&cacheDirectory->sr);

        PhDereferenceObject(cacheDirectory);
    }
}

HANDLE PhGetNamespaceHandle(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HANDLE directoryHandle = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + 0x70];
        PSID administratorsSid = PhSeAdministratorsSid();
        UNICODE_STRING objectName;
        OBJECT_ATTRIBUTES objectAttributes;
        PSECURITY_DESCRIPTOR securityDescriptor;
        ULONG sdAllocationLength;
        PACL dacl;

        // Create the default namespace DACL.

        sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
            (ULONG)sizeof(ACL) +
            (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
            PhLengthSid(&PhSeLocalSid) +
            (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
            PhLengthSid(administratorsSid) +
            (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
            PhLengthSid(&PhSeInteractiveSid);

        securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;
        dacl = PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

        PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        PhCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
        PhAddAccessAllowedAce(dacl, ACL_REVISION, DIRECTORY_ALL_ACCESS, &PhSeLocalSid);
        PhAddAccessAllowedAce(dacl, ACL_REVISION, DIRECTORY_ALL_ACCESS, administratorsSid);
        PhAddAccessAllowedAce(dacl, ACL_REVISION, DIRECTORY_QUERY | DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT, &PhSeInteractiveSid);
        PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

        RtlInitUnicodeString(&objectName, L"\\BaseNamedObjects\\SystemInformer");
        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_OPENIF | (WindowsVersion < WINDOWS_10 ? 0 : OBJ_DONT_REPARSE),
            NULL,
            securityDescriptor
            );

        NtCreateDirectoryObject(&directoryHandle, MAXIMUM_ALLOWED, &objectAttributes);

        assert(RtlValidSecurityDescriptor(securityDescriptor));
        assert(sdAllocationLength < sizeof(securityDescriptorBuffer));
        assert(RtlLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));
        PhEndInitOnce(&initOnce);
    }

    return directoryHandle;
}

/**
 * Retrieves the hung window represented by a ghost window.
 * If the specified window is not a ghost window, the function returns NULL.
 *
 * \param WindowHandle The handle to the ghost window.
 * \return The handle to the original hung window if successful, otherwise NULL.
 */
HWND PhHungWindowFromGhostWindow(
    _In_ HWND WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    // This is an undocumented function exported by user32.dll that
    // retrieves the hung window represented by a ghost window. (wj32)
    static HWND (WINAPI *HungWindowFromGhostWindow_I)(
        _In_ HWND GhostWindowHandle
        );

    if (PhBeginInitOnce(&initOnce))
    {
        HungWindowFromGhostWindow_I = PhGetDllProcedureAddress(L"user32.dll", "HungWindowFromGhostWindow", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!HungWindowFromGhostWindow_I)
        return NULL;

    // The call will return NULL if the window wasn't actually a ghost window. (wj32)
    return HungWindowFromGhostWindow_I(WindowHandle);
}

NTSTATUS PhGetFileData(
    _In_ HANDLE FileHandle,
    _Out_ PVOID* Buffer,
    _Out_ PULONG BufferLength
    )
{
    NTSTATUS status;
    PSTR data = NULL;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    IO_STATUS_BLOCK isb;
    BYTE buffer[PAGE_SIZE * 2];

    allocatedLength = sizeof(buffer);
    data = PhAllocate(allocatedLength);
    dataLength = 0;

    while (NT_SUCCESS(status = NtReadFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        buffer,
        sizeof(buffer),
        NULL,
        NULL
        )))
    {
        returnLength = (ULONG)isb.Information;

        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = PhReAllocate(data, allocatedLength);
        }

        memcpy(data + dataLength, buffer, returnLength);

        dataLength += returnLength;
    }

    if (status == STATUS_END_OF_FILE)
    {
        status = STATUS_SUCCESS;
    }

    if (status == STATUS_PIPE_BROKEN)
    {
        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(data);

        *Buffer = NULL;
        *BufferLength = 0;

        return status;
    }

    if (allocatedLength < dataLength + sizeof(ANSI_NULL))
    {
        allocatedLength++;
        data = PhReAllocate(data, allocatedLength);
    }

    data[dataLength] = ANSI_NULL;

    *Buffer = data;
    *BufferLength = dataLength;

    return status;
}

NTSTATUS PhGetFileText(
    _Out_ PVOID* String,
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Unicode
    )
{
    NTSTATUS status;
    ULONG dataLength;
    PSTR data;

    status = PhGetFileData(
        FileHandle,
        &data,
        &dataLength
        );

    if (NT_SUCCESS(status))
    {
        if (dataLength)
        {
            if (Unicode)
                *String = PhConvertUtf8ToUtf16Ex(data, dataLength);
            else
                *String = PhCreateBytesEx(data, dataLength);
        }
        else
        {
            *String = PhReferenceEmptyString();
        }

        PhFree(data);
    }

    return status;
}

NTSTATUS PhFileReadAllText(
    _Out_ PVOID* String,
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN Unicode
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetFileText(
            String,
            fileHandle,
            Unicode
            );
        NtClose(fileHandle);
    }

    return status;
}

NTSTATUS PhFileReadAllTextWin32(
    _Out_ PVOID* String,
    _In_ PCWSTR FileName,
    _In_ BOOLEAN Unicode
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetFileText(
            String,
            fileHandle,
            Unicode
            );
        NtClose(fileHandle);
    }

    return status;
}

NTSTATUS PhFileWriteAllText(
    _In_ PCPH_STRINGREF FileName,
    _In_ PCPH_STRINGREF String
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhWriteFile(
            fileHandle,
            String->Buffer,
            (ULONG)String->Length,
            NULL,
            NULL
            );

        NtClose(fileHandle);
    }

    return status;
}

HRESULT PhGetClassObjectDllBase(
    _In_ PVOID DllBase,
    _In_ REFCLSID Rclsid,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
    HRESULT (WINAPI* DllGetClassObject_I)(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ PVOID* ppv);
    HRESULT status;
    IClassFactory* classFactory;

    if (!(DllGetClassObject_I = PhGetDllBaseProcedureAddress(DllBase, "DllGetClassObject", 0)))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    status = DllGetClassObject_I(
        Rclsid,
        &IID_IClassFactory,
        &classFactory
        );

    if (SUCCEEDED(status))
    {
        status = IClassFactory_CreateInstance(
            classFactory,
            NULL,
            Riid,
            Ppv
            );
        IClassFactory_Release(classFactory);
    }

    return status;
}

/**
 * \brief Provides a pointer to an interface on a class object associated with a specified CLSID.
 * Most class objects implement the IClassFactory interface. You would then call CreateInstance to
 * create an uninitialized object. It is not always necessary to go through this process however.
 *
 * \param DllName The file containing the object to initialize.
 * \param Rclsid A pointer to the class identifier of the object to be created.
 * \param Riid Reference to the identifier of the interface to communicate with the class object.
 * \a Typically this value is IID_IClassFactory, although other values such as IID_IClassFactory2 which supports a form of licensing are allowed.
 * \param Ppv The address of pointer variable that contains the requested interface.
 * \return Successful or errant status.
 */
HRESULT PhGetClassObject(
    _In_ PCWSTR DllName,
    _In_ REFCLSID Rclsid,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
#if defined(PH_BUILD_MSIX)
    HRESULT status;
    IClassFactory* classFactory;

    status = CoGetClassObject(
        Rclsid,
        CLSCTX_INPROC_SERVER,
        NULL,
        &IID_IClassFactory,
        &classFactory
        );

    if (HR_SUCCESS(status))
    {
        status = IClassFactory_CreateInstance(
            classFactory,
            NULL,
            Riid,
            Ppv
            );
        IClassFactory_Release(classFactory);
    }

    return status;
#else
    PVOID baseAddress;
    PH_STRINGREF baseDllName;

    PhInitializeStringRefLongHint(&baseDllName, DllName);

    if (!(baseAddress = PhGetLoaderEntryDllBase(NULL, &baseDllName)))
    {
        if (!(baseAddress = PhLoadLibrary(DllName)))
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    return PhGetClassObjectDllBase(baseAddress, Rclsid, Riid, Ppv);
#endif
}

HRESULT PhGetActivationFactoryDllBase(
    _In_ PVOID DllBase,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
    HRESULT (WINAPI* DllGetActivationFactory_I)(_In_ HSTRING RuntimeClassId, _Out_ PVOID* ActivationFactory);
    HRESULT status;
    HSTRING_REFERENCE string;
    IActivationFactory* activationFactory;

    if (FAILED(status = PhCreateWindowsRuntimeStringReference(RuntimeClass, &string)))
        return status;

    if (!(DllGetActivationFactory_I = PhGetDllBaseProcedureAddress(DllBase, "DllGetActivationFactory", 0)))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    status = DllGetActivationFactory_I(
        HSTRING_FROM_STRING(string),
        &activationFactory
        );

    if (SUCCEEDED(status))
    {
        status = IActivationFactory_QueryInterface(
            activationFactory,
            Riid,
            Ppv
            );
        IActivationFactory_Release(activationFactory);
    }

    return status;
}

/**
 * \brief Activates the specified Windows Runtime class.
 *
 * \param DllName The file containing the object to initialize.
 * \param RuntimeClass The class identifier string that is associated with the activatable runtime class.
 * \param Riid The reference ID of the interface.
 * \param Ppv The activation factory.
 * \return Successful or errant status.
 */
HRESULT PhGetActivationFactory(
    _In_ PCWSTR DllName,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
#if defined(PH_BUILD_MSIX)
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&WindowsCreateStringReference) WindowsCreateStringReference_I = NULL;
    static typeof(&RoGetActivationFactory) RoGetActivationFactory_I = NULL;
    HRESULT status;
    HSTRING runtimeClassStringHandle = NULL;
    HSTRING_HEADER runtimeClassStringHeader;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"combase.dll"))
        {
            WindowsCreateStringReference_I = PhGetDllBaseProcedureAddress(baseAddress, "WindowsCreateStringReference", 0);
            RoGetActivationFactory_I = PhGetDllBaseProcedureAddress(baseAddress, "RoGetActivationFactory", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(WindowsCreateStringReference_I && RoGetActivationFactory_I))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    status = WindowsCreateStringReference_I(
        RuntimeClass,
        (UINT32)PhCountStringZ(RuntimeClass),
        &runtimeClassStringHeader,
        &runtimeClassStringHandle
        );

    if (HR_SUCCESS(status))
    {
        status = RoGetActivationFactory_I(
            runtimeClassStringHandle,
            Riid,
            Ppv
            );
    }

    return status;
#else
    PVOID baseAddress;
    PH_STRINGREF baseDllName;

    PhInitializeStringRefLongHint(&baseDllName, DllName);

    if (!(baseAddress = PhGetLoaderEntryDllBase(NULL, &baseDllName)))
    {
        if (!(baseAddress = PhLoadLibrary(DllName)))
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    return PhGetActivationFactoryDllBase(baseAddress, RuntimeClass, Riid, Ppv);
#endif
}

/**
 * Activates an instance of a Windows Runtime class from a specified DLL base address.
 *
 * This function attempts to create and return an instance of a Windows Runtime class (specified by
 * the class name) from a loaded DLL image. It uses the DllGetActivationFactory export if available
 * to obtain the activation factory, then calls IActivationFactory::ActivateInstance to create the
 * object and query the requested interface.
 *
 * \param DllBase The base address of the loaded DLL containing the Windows Runtime class.
 * \param RuntimeClass The name of the runtime class to activate (e.g., L"Windows.Storage.StorageFile").
 * \param Riid The interface identifier (IID) of the interface to retrieve from the created instance.
 * \param Ppv A pointer that receives the interface pointer requested in Riid.
 * \return Returns an HRESULT indicating success or failure.
 *
 * \remarks
 *      - This function is typically used for COM/WinRT activation scenarios where the class is not
 *        registered globally but is available in a specific DLL.
 *      - The function first creates an HSTRING reference for the class name, then calls the
 *        DllGetActivationFactory export to obtain an activation factory. It then calls
 *        IActivationFactory::ActivateInstance to create the object and queries the requested
 *        interface.
 *      - The caller must ensure that the DLL is loaded and that the class is available.
 *      - On success, the caller is responsible for releasing the returned interface pointer.
 */
HRESULT PhActivateInstanceDllBase(
    _In_ PVOID DllBase,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
    HRESULT (WINAPI* DllGetActivationFactory_I)(_In_ HSTRING RuntimeClassId, _Out_ PVOID * ActivationFactory);
    HRESULT status;
    HSTRING_REFERENCE string;
    IActivationFactory* activationFactory;
    IInspectable* inspectableObject;

    if (FAILED(status = PhCreateWindowsRuntimeStringReference(RuntimeClass, &string)))
        return status;

    if (!(DllGetActivationFactory_I = PhGetDllBaseProcedureAddress(DllBase, "DllGetActivationFactory", 0)))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    status = DllGetActivationFactory_I(
        HSTRING_FROM_STRING(string),
        &activationFactory
        );

    if (SUCCEEDED(status))
    {
        status = IActivationFactory_ActivateInstance(
            activationFactory,
            &inspectableObject
            );

        if (SUCCEEDED(status))
        {
            status = IInspectable_QueryInterface(
                inspectableObject,
                Riid,
                Ppv
                );

            IInspectable_Release(inspectableObject);
        }

        IActivationFactory_Release(activationFactory);
    }

    return status;
}

/**
 * \brief Activates the specified Windows Runtime class.
 *
 * \param DllName The file containing the object to initialize.
 * \param RuntimeClass The class identifier string that is associated with the activatable runtime class.
 * \param Riid The reference ID of the interface.
 * \param Ppv A pointer to the activated instance of the runtime class.
 * \return Successful or errant status.
 */
HRESULT PhActivateInstance(
    _In_ PCWSTR DllName,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
#if defined(PH_BUILD_MSIX)
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&WindowsCreateStringReference) WindowsCreateStringReference_I = NULL;
    static typeof(&RoActivateInstance) RoActivateInstance_I = NULL;
    HRESULT status;
    HSTRING runtimeClassStringHandle = NULL;
    HSTRING_HEADER runtimeClassStringHeader;
    IInspectable* inspectableObject;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"combase.dll"))
        {
            WindowsCreateStringReference_I = PhGetDllBaseProcedureAddress(baseAddress, "WindowsCreateStringReference", 0);
            RoActivateInstance_I = PhGetDllBaseProcedureAddress(baseAddress, "RoActivateInstance", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(WindowsCreateStringReference_I && RoActivateInstance_I))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    status = WindowsCreateStringReference_I(
        RuntimeClass,
        (UINT32)PhCountStringZ(RuntimeClass),
        &runtimeClassStringHeader,
        &runtimeClassStringHandle
        );

    if (HR_SUCCESS(status))
    {
        status = RoActivateInstance_I(
            runtimeClassStringHandle,
            &inspectableObject
            );

        if (HR_SUCCESS(status))
        {
            status = IInspectable_QueryInterface(
                inspectableObject,
                Riid,
                Ppv
                );
            IInspectable_Release(inspectableObject);
        }
    }

    return status;
#else
    PVOID baseAddress;
    PH_STRINGREF baseDllName;

    PhInitializeStringRefLongHint(&baseDllName, DllName);

    if (!(baseAddress = PhGetLoaderEntryDllBase(NULL, &baseDllName)))
    {
        if (!(baseAddress = PhLoadLibrary(DllName)))
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    return PhActivateInstanceDllBase(baseAddress, RuntimeClass, Riid, Ppv);
#endif
}

#if defined(PH_NATIVE_RING_BUFFER)
// This function creates a ring buffer by allocating a pagefile-backed section
// and mapping two views of that section next to each other. This way if the
// last record in the buffer wraps it can still be accessed in a linear fashion
// using its base VA. (Win10 RS5 and above only) (dmex)
// Based on Win32 version: https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2#examples
NTSTATUS PhCreateRingBuffer(
    _In_ SIZE_T BufferSize,
    _Out_ PVOID* RingBuffer,
    _Out_ PVOID* SecondaryView
    )
{
    NTSTATUS status;
    SIZE_T regionSize;
    LARGE_INTEGER sectionSize;
    HANDLE sectionHandle = NULL;
    PVOID placeholder1 = NULL;
    PVOID placeholder2 = NULL;
    PVOID sectionView1 = NULL;
    PVOID sectionView2 = NULL;

    if ((BufferSize % PhSystemBasicInformation.AllocationGranularity) != 0)
        return STATUS_UNSUCCESSFUL;

    //
    // Reserve a placeholder region where the buffer will be mapped.
    //

    regionSize = 2 * BufferSize;
    status = NtAllocateVirtualMemoryEx(
        NtCurrentProcess(),
        &placeholder1,
        &regionSize,
        MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
        PAGE_NOACCESS,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //
    // Split the placeholder region into two regions of equal size.
    //

    regionSize = BufferSize;
    status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &placeholder1,
        &regionSize,
        MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    placeholder2 = PTR_ADD_OFFSET(placeholder1, BufferSize);

    //
    // Create a pagefile-backed section for the buffer.
    //

    sectionSize.QuadPart = BufferSize;
    status = NtCreateSectionEx(
        &sectionHandle,
        SECTION_ALL_ACCESS,
        NULL,
        &sectionSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        NULL,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //
    // Map the section into the first placeholder region.
    //

    regionSize = BufferSize;
    sectionView1 = placeholder1;
    status = NtMapViewOfSectionEx(
        sectionHandle,
        NtCurrentProcess(),
        &sectionView1,
        0,
        &regionSize,
        MEM_REPLACE_PLACEHOLDER,
        PAGE_READWRITE,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //
    // Ownership transferred, don't free this now.
    //

    placeholder1 = NULL;

    //
    // Map the section into the second placeholder region.
    //

    regionSize = BufferSize;
    sectionView2 = placeholder2;
    status = NtMapViewOfSectionEx(
        sectionHandle,
        NtCurrentProcess(),
        &sectionView2,
        0,
        &regionSize,
        MEM_REPLACE_PLACEHOLDER,
        PAGE_READWRITE,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //
    // Success, return both mapped views to the caller.
    //

    *RingBuffer = sectionView1;
    *SecondaryView = sectionView2;

#ifdef DEBUG
    // assert the buffer wraps properly.
    ((PCHAR)sectionView1)[0] = 'a';
    assert(((PCHAR)sectionView1)[BufferSize] == 'a');
#endif

    placeholder2 = NULL;
    sectionView1 = NULL;
    sectionView2 = NULL;

CleanupExit:
    if (sectionHandle)
    {
        NtClose(sectionHandle);
    }

    if (placeholder1)
    {
        regionSize = 0;
        NtFreeVirtualMemory(NtCurrentProcess(), placeholder1, &regionSize, MEM_RELEASE);
    }

    if (placeholder2)
    {
        regionSize = 0;
        NtFreeVirtualMemory(NtCurrentProcess(), placeholder2, &regionSize, MEM_RELEASE);
    }

    if (sectionView1)
        PhUnmapViewOfSection(NtCurrentProcess(), sectionView1);
    if (sectionView2)
        PhUnmapViewOfSection(NtCurrentProcess(), sectionView2);

    return status;
}
#endif

/**
 * Suspends the current thread for a specified interval.
 *
 * This function delays execution for the given interval, optionally allowing the wait to be alertable.
 * On Windows 11 and later, it uses RtlDelayExecution if available; otherwise, it falls back to NtDelayExecution.
 *
 * \param Alertable If TRUE, the wait is alertable and can be interrupted by an APC; otherwise, the wait is not alertable.
 * \param DelayInterval A pointer to a LARGE_INTEGER that specifies the interval to wait, in 100-nanosecond units. A negative value indicates relative time.
 * \return An NTSTATUS code indicating success or failure of the delay operation.
 * \note
 * RtlDelayExecution is preferred on Windows 11 and later because it provides additional logic and optimizations
 * over NtDelayExecution and ensures more efficient and robust thread suspension on modern Windows versions plus
 * future compatibility with OS-level enhancements
 *
 * RtlDelayExecution:
 * - Wraps NtDelayExecution/ZwDelayExecution and adds additional logic before and after the system call.
 * - Spin-Wait Optimization: If the delay interval is zero (i.e., a "yield" or very short sleep), and certain internal
 * configuration variables are set, it may perform a short spin-wait (using _mm_pause()) before calling the system call.
 * This is to avoid excessive context switches and improve performance for very short waits.
 * - Performance Counter Tracking: Uses RtlQueryPerformanceCounter to track the time since the last sleep and adjust spin-waiting accordingly.
 * - TEB State Tracking: Updates fields in the thread environment block (TEB), such as SpinCallCount and LastSleepCounter, to manage spin-wait heuristics.
 * - Adaptive Backoff: The amount of spin-waiting can increase if the thread is calling delay execution in rapid succession, up to a configured maximum.
 * - Resets Spin Count on Real Wait: if the delay was not interrupted (STATUS_ALERTED), resets the spin count.
 */
NTSTATUS PhDelayExecutionEx(
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER DelayInterval
    )
{
    if (WindowsVersion >= WINDOWS_11 && RtlDelayExecution_Import())
    {
        return RtlDelayExecution_Import()(Alertable, DelayInterval);
    }

    return NtDelayExecution(Alertable, DelayInterval);
}

/**
 * Suspends the current thread for a specified number of milliseconds.
 *
 * This function delays execution for the specified number of milliseconds. If the value is INFINITE,
 * the thread is suspended indefinitely. Otherwise, the delay is converted to a negative relative interval
 * in 100-nanosecond units and passed to PhDelayExecutionEx.
 *
 * \note
 * On Windows 11 and later, PhDelayExecutionEx uses RtlDelayExecution, which provides improved behavior
 * for short waits and better compatibility with future Windows updates.
 * \param Milliseconds The number of milliseconds to delay. Use INFINITE to wait indefinitely.
 * \return An NTSTATUS code indicating success or failure of the delay operation.
 */
NTSTATUS PhDelayExecution(
    _In_ ULONG Milliseconds
    )
{
    if (Milliseconds == INFINITE)
    {
        LARGE_INTEGER interval;

        interval.QuadPart = LLONG_MIN;

        return PhDelayExecutionEx(FALSE, &interval);
    }
    else
    {
        LARGE_INTEGER interval;

        interval.QuadPart = -(LONGLONG)UInt32x32To64(Milliseconds, PH_TIMEOUT_MS);

        return PhDelayExecutionEx(FALSE, &interval);
    }
}

/**
 * Computes a case-insensitive hash value for a Unicode string using a specified hash factor.
 *
 * This function is used for hashing API Set names in the Windows API Set schema resolution logic.
 * It iterates over each character in the string, normalizes uppercase ASCII letters to lowercase,
 * and accumulates the hash using the provided hash factor.
 *
 * \param String A pointer to a PH_STRINGREF structure representing the Unicode string to hash.
 * \param HashFactor The multiplier used in the hash calculation (typically provided by the schema).
 * \return The computed hash value as an unsigned long.
 */
FORCEINLINE
ULONG PhApiSetHashString(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG HashFactor
    )
{
    ULONG hash = 0;

    for (ULONG i = 0; i < String->Length / sizeof(WCHAR); i++)
    {
        hash = hash * HashFactor + ((USHORT)(String->Buffer[i] - L'A') <= L'Z' - L'A' ? String->Buffer[i] + L'a' - L'A' : String->Buffer[i]);
    }

    return hash;
}

/**
 * Resolves an API Set name to a host DLL name.
 *
 * \param[in] Schema The API Set definition, such as PEB->ApiSetMap.
 * \param[in] ApiSetName The name of the API Set, with or without a .dll extension.
 * \param[in] ParentName An optional name of the parent (importing) module.
 * \param[out] HostBinary The resolved name of the DLL.
 * \return Successful or errant status.
 */
NTSTATUS PhApiSetResolveToHost(
    _In_ PAPI_SET_NAMESPACE Schema,
    _In_ PCPH_STRINGREF ApiSetName,
    _In_opt_ PCPH_STRINGREF ParentName,
    _Out_ PPH_STRINGREF HostBinary
    )
{
    PH_STRINGREF parentNameFilePart;

    if (ApiSetName->Length >= 4 * sizeof(WCHAR))
    {
        // Extract and downcase the first 4 characters
        ULONGLONG apisetNameBufferPrefix = ((PULONGLONG)ApiSetName->Buffer)[0] & 0xFFFFFFDFFFDFFFDF;

        // Verify the API Set prefixes
        if (apisetNameBufferPrefix != 0x2D004900500041ULL && // L"api-"
            apisetNameBufferPrefix != 0x2D005400580045ULL) // L"ext-"
            return STATUS_INVALID_PARAMETER;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (ParentName)
    {
        // If given a path, we are only interested in the file component
        if (!PhGetBasePath(ParentName, NULL, &parentNameFilePart))
        {
            parentNameFilePart.Buffer = ParentName->Buffer;
            parentNameFilePart.Length = ParentName->Length;
        }
    }

    switch (Schema->Version)
    {
    case API_SET_SCHEMA_VERSION_V6:
        {
            PAPI_SET_NAMESPACE_ENTRY namespaceEntries = PTR_ADD_OFFSET(Schema, Schema->EntryOffset);
            PAPI_SET_NAMESPACE_ENTRY namespaceEntry;
            PAPI_SET_HASH_ENTRY hashEntries = PTR_ADD_OFFSET(Schema, Schema->HashOffset);
            PAPI_SET_HASH_ENTRY hashEntry;
            PAPI_SET_VALUE_ENTRY valueEntry;
            PH_STRINGREF apisetNamespacePart;
            PH_STRINGREF apisetNameHashedPart;
            ULONG_PTR hyphenIndex;
            ULONG hash;
            LONG minIndex;
            LONG maxIndex;
            LONG midIndex;
            LONG comparison;

            if (!Schema->Count)
                return STATUS_NOT_FOUND;

            if (Schema->Count > MAXINT32)
                return STATUS_INTEGER_OVERFLOW;

            // The hash covers the length up to but not including the last hyphen
            hyphenIndex = PhFindLastCharInStringRef(ApiSetName, L'-', FALSE);

            // There must be a hyphen if it passed the prefix check
            assert(hyphenIndex != SIZE_MAX);

            apisetNameHashedPart.Buffer = ApiSetName->Buffer;
            apisetNameHashedPart.Length = hyphenIndex * sizeof(WCHAR);
            hash = PhApiSetHashString(&apisetNameHashedPart, Schema->HashFactor);

            minIndex = 0;
            maxIndex = Schema->Count - 1;

            // Perform a binary search for the API Set name hash
            do
            {
                midIndex = (maxIndex + minIndex) / 2;
                hashEntry = &hashEntries[midIndex];

                if (hash < hashEntry->Hash)
                    maxIndex = midIndex - 1;
                else if (hash > hashEntry->Hash)
                    minIndex = midIndex + 1;
                else
                {
                    namespaceEntry = &namespaceEntries[hashEntry->Index];

                    // Verify the API Set name matches, in addition to its hash
                    apisetNamespacePart.Buffer = PTR_ADD_OFFSET(Schema, namespaceEntry->NameOffset);
                    apisetNamespacePart.Length = namespaceEntry->HashedLength;

                    comparison = PhCompareStringRef(
                        &apisetNameHashedPart,
                        &apisetNamespacePart,
                        TRUE
                        );

                    //comparison = RtlCompareUnicodeStrings(
                    //    apiSetNameHashedPart.Buffer,
                    //    apiSetNameHashedPart.Length / sizeof(WCHAR),
                    //    PTR_ADD_OFFSET(Schema, namespaceEntry->NameOffset),
                    //    namespaceEntry->HashedLength / sizeof(WCHAR),
                    //    TRUE
                    //    );

                    if (comparison < 0)
                        maxIndex = midIndex - 1;
                    else if (comparison > 0)
                        minIndex = midIndex + 1;
                    else
                        break;
                }

                if (minIndex > maxIndex)
                    return STATUS_NOT_FOUND;

            } while (TRUE);

            valueEntry = PTR_ADD_OFFSET(Schema, namespaceEntry->ValueOffset);

            // Try module-specific search
            if (ParentName && namespaceEntry->ValueCount > 1)
            {
                if (namespaceEntry->ValueCount > MAXINT32)
                    return STATUS_INTEGER_OVERFLOW;

                minIndex = 0;
                maxIndex = namespaceEntry->ValueCount - 1;

                // Perform binary search for the parent name
                do
                {
                    midIndex = (maxIndex + minIndex) / 2;

                    apisetNamespacePart.Buffer = PTR_ADD_OFFSET(Schema, valueEntry[midIndex].NameOffset);
                    apisetNamespacePart.Length = valueEntry[midIndex].NameLength;

                    comparison = PhCompareStringRef(
                        &parentNameFilePart,
                        &apisetNamespacePart,
                        TRUE
                        );

                    //comparison = RtlCompareUnicodeStrings(
                    //    parentNameFilePart.Buffer,
                    //    parentNameFilePart.Length / sizeof(WCHAR),
                    //    PTR_ADD_OFFSET(Schema, valueEntry[midIndex].NameOffset),
                    //    valueEntry[midIndex].NameLength / sizeof(WCHAR),
                    //    TRUE
                    //    );

                    if (comparison < 0)
                        maxIndex = midIndex - 1;
                    else if (comparison > 0)
                        minIndex = midIndex + 1;
                    else
                    {
                        if (!valueEntry[midIndex].ValueLength)
                            return STATUS_APISET_NOT_HOSTED;

                        // Return the parent-specific result
                        HostBinary->Buffer = PTR_ADD_OFFSET(Schema, valueEntry[midIndex].ValueOffset);
                        HostBinary->Length = valueEntry[midIndex].ValueLength;
                        return STATUS_SUCCESS;
                    }

                    // No match; fall back to the default value
                    if (minIndex > maxIndex)
                        break;

                } while (TRUE);
            }

            // Use the default value
            if (namespaceEntry->ValueCount)
            {
                if (!valueEntry[0].ValueLength)
                    return STATUS_APISET_NOT_HOSTED;

                HostBinary->Buffer = PTR_ADD_OFFSET(Schema, valueEntry[0].ValueOffset);
                HostBinary->Length = valueEntry[0].ValueLength;
                return STATUS_SUCCESS;
            }

            return STATUS_NOT_FOUND;
        }
        break;
    case API_SET_SCHEMA_VERSION_V4:
        {
            PAPI_SET_NAMESPACE_ARRAY_V4 schema = (PAPI_SET_NAMESPACE_ARRAY_V4)Schema;
            PAPI_SET_VALUE_ARRAY_V4 valueArray;
            PH_STRINGREF apisetNamespacePart;
            PH_STRINGREF apisetNameShort;
            LONG comparison;
            LONG minIndex;
            LONG maxIndex;
            LONG midIndex;

            if (!schema->Count)
                return STATUS_NOT_FOUND;

            if (schema->Count > MAXINT32)
                return STATUS_INTEGER_OVERFLOW;

            // Should be covered by the prefix check
            assert(ApiSetName->Length >= 4 * sizeof(WCHAR));

            // Skip the "api-" and "ext-" prefixes
            apisetNameShort.Buffer = ApiSetName->Buffer + 4;
            apisetNameShort.Length = ApiSetName->Length - 4 * sizeof(WCHAR);

            // Ignore file extension (when present)
            if (apisetNameShort.Length >= 4 * sizeof(WCHAR) &&
                apisetNameShort.Buffer[apisetNameShort.Length / sizeof(WCHAR) - 4] == L'.')
                apisetNameShort.Length -= 4 * sizeof(WCHAR);

            minIndex = 0;
            maxIndex = schema->Count - 1;

            // Perform a binary search for the API Set name
            do
            {
                midIndex = (maxIndex + minIndex) / 2;

                apisetNamespacePart.Buffer = PTR_ADD_OFFSET(schema, schema->Array[midIndex].NameOffset);
                apisetNamespacePart.Length = schema->Array[midIndex].NameLength;

                comparison = PhCompareStringRef(
                    &apisetNameShort,
                    &apisetNamespacePart,
                    TRUE
                    );

                //comparison = RtlCompareUnicodeStrings(
                //    apisetNameShort.Buffer,
                //    apisetNameShort.Length / sizeof(WCHAR),
                //    PTR_ADD_OFFSET(schema, schema->Array[midIndex].NameOffset),
                //    schema->Array[midIndex].NameLength / sizeof(WCHAR),
                //    TRUE
                //    );

                if (comparison < 0)
                    maxIndex = midIndex - 1;
                else if (comparison > 0)
                    minIndex = midIndex + 1;
                else
                    break;

                if (minIndex > maxIndex)
                    return STATUS_NOT_FOUND;

            } while (TRUE);

            valueArray = PTR_ADD_OFFSET(schema, schema->Array[midIndex].DataOffset);

            // Try module-specific search
            if (ParentName && valueArray->Count > 1)
            {
                if (valueArray->Count > MAXINT32)
                    return STATUS_INTEGER_OVERFLOW;

                minIndex = 0;
                maxIndex = valueArray->Count - 1;

                // Perform binary search for the parent name
                do
                {
                    midIndex = (maxIndex + minIndex) / 2;

                    apisetNamespacePart.Buffer = PTR_ADD_OFFSET(schema, valueArray->Array[midIndex].NameOffset);
                    apisetNamespacePart.Length = valueArray->Array[midIndex].NameLength;

                    comparison = PhCompareStringRef(
                        &parentNameFilePart,
                        &apisetNamespacePart,
                        TRUE
                        );

                    //comparison = RtlCompareUnicodeStrings(
                    //    parentNameFilePart.Buffer,
                    //    parentNameFilePart.Length / sizeof(WCHAR),
                    //    PTR_ADD_OFFSET(schema, valueArray->Array[midIndex].NameOffset),
                    //    valueArray->Array[midIndex].NameLength / sizeof(WCHAR),
                    //    TRUE
                    //    );

                    if (comparison < 0)
                        maxIndex = midIndex - 1;
                    else if (comparison > 0)
                        minIndex = midIndex + 1;
                    else
                    {
                        if (!valueArray->Array[midIndex].ValueLength)
                            return STATUS_APISET_NOT_HOSTED;

                        // Return the parent-specific result
                        HostBinary->Buffer = PTR_ADD_OFFSET(schema, valueArray->Array[midIndex].ValueOffset);
                        HostBinary->Length = valueArray->Array[midIndex].ValueLength;
                        return STATUS_SUCCESS;
                    }

                    // No match; fall back to the default value
                    if (minIndex > maxIndex)
                        break;

                } while (TRUE);
            }

            // Use the default value
            if (valueArray->Count)
            {
                if (!valueArray->Array[0].ValueLength)
                    return STATUS_APISET_NOT_HOSTED;

                HostBinary->Buffer = PTR_ADD_OFFSET(schema, valueArray->Array[0].ValueOffset);
                HostBinary->Length = valueArray->Array[0].ValueLength;
                return STATUS_SUCCESS;
            }

            return STATUS_NOT_FOUND;
        }
        break;
    case API_SET_SCHEMA_VERSION_V2:
        {
            PAPI_SET_NAMESPACE_ARRAY_V2 schema = (PAPI_SET_NAMESPACE_ARRAY_V2)Schema;
            PAPI_SET_VALUE_ARRAY_V2 valueArray;
            PH_STRINGREF apisetNamespacePart;
            PH_STRINGREF apiSetNameShort;
            LONG comparison;
            LONG minIndex;
            LONG maxIndex;
            LONG midIndex;

            if (!schema->Count)
                return STATUS_NOT_FOUND;

            if (schema->Count > MAXINT32)
                return STATUS_INTEGER_OVERFLOW;

            // Should be covered by the prefix check
            assert(ApiSetName->Length >= 4 * sizeof(WCHAR));

            // Skip the "api-" and "ext-" prefixes
            apiSetNameShort.Buffer = ApiSetName->Buffer + 4;
            apiSetNameShort.Length = ApiSetName->Length - 4 * sizeof(WCHAR);

            // Ignore file extension (when present)
            if (apiSetNameShort.Length >= 4 * sizeof(WCHAR) &&
                apiSetNameShort.Buffer[apiSetNameShort.Length / sizeof(WCHAR) - 4] == L'.')
                apiSetNameShort.Length -= 4 * sizeof(WCHAR);

            minIndex = 0;
            maxIndex = schema->Count - 1;

            // Perform a binary search for the API Set name
            do
            {
                midIndex = (maxIndex + minIndex) / 2;

                apisetNamespacePart.Buffer = PTR_ADD_OFFSET(schema, schema->Array[midIndex].NameOffset);
                apisetNamespacePart.Length = schema->Array[midIndex].NameLength;

                comparison = PhCompareStringRef(
                    &apiSetNameShort,
                    &apisetNamespacePart,
                    TRUE
                    );

                //comparison = RtlCompareUnicodeStrings(
                //    apiSetNameShort.Buffer,
                //    apiSetNameShort.Length / sizeof(WCHAR),
                //    PTR_ADD_OFFSET(schema, schema->Array[midIndex].NameOffset),
                //    schema->Array[midIndex].NameLength / sizeof(WCHAR),
                //    TRUE
                //    );

                if (comparison < 0)
                    maxIndex = midIndex - 1;
                else if (comparison > 0)
                    minIndex = midIndex + 1;
                else
                    break;

                if (minIndex > maxIndex)
                    return STATUS_NOT_FOUND;

            } while (TRUE);

            valueArray = PTR_ADD_OFFSET(schema, schema->Array[midIndex].DataOffset);

            // Try module-specific search
            if (ParentName && valueArray->Count > 1)
            {
                if (valueArray->Count > MAXINT32)
                    return STATUS_INTEGER_OVERFLOW;

                minIndex = 0;
                maxIndex = valueArray->Count - 1;

                // Perform binary search for the parent name
                do
                {
                    midIndex = (maxIndex + minIndex) / 2;

                    apisetNamespacePart.Buffer = PTR_ADD_OFFSET(schema, valueArray->Array[midIndex].NameOffset);
                    apisetNamespacePart.Length = valueArray->Array[midIndex].NameLength;

                    comparison = PhCompareStringRef(
                        &apiSetNameShort,
                        &apisetNamespacePart,
                        TRUE
                        );

                    //comparison = RtlCompareUnicodeStrings(
                    //    parentNameFilePart.Buffer,
                    //    parentNameFilePart.Length / sizeof(WCHAR),
                    //    PTR_ADD_OFFSET(schema, valueArray->Array[midIndex].NameOffset),
                    //    valueArray->Array[midIndex].NameLength / sizeof(WCHAR),
                    //    TRUE
                    //    );

                    if (comparison < 0)
                        maxIndex = midIndex - 1;
                    else if (comparison > 0)
                        minIndex = midIndex + 1;
                    else
                    {
                        if (!valueArray->Array[midIndex].ValueLength)
                            return STATUS_APISET_NOT_HOSTED;

                        // Return the parent-specific result
                        HostBinary->Buffer = PTR_ADD_OFFSET(schema, valueArray->Array[midIndex].ValueOffset);
                        HostBinary->Length = valueArray->Array[midIndex].ValueLength;
                        return STATUS_SUCCESS;
                    }

                    // No match; fall back to the default value
                    if (minIndex > maxIndex)
                        break;

                } while (TRUE);
            }

            // Use the default value
            if (valueArray->Count)
            {
                if (!valueArray->Array[0].ValueLength)
                    return STATUS_APISET_NOT_HOSTED;

                HostBinary->Buffer = PTR_ADD_OFFSET(schema, valueArray->Array[0].ValueOffset);
                HostBinary->Length = valueArray->Array[0].ValueLength;
                return STATUS_SUCCESS;
            }

            return STATUS_NOT_FOUND;
        }
        break;
    }

    return STATUS_UNKNOWN_REVISION;
}

/**
 * Creates a process as the interactive user using the Windows Desktop Controller (WDC) API.
 *
 * This function attempts to launch a process in the context of the currently active interactive user session.
 * It uses the undocumented WdcRunTaskAsInteractiveUser function from wdc.dll, if available, to perform the operation.
 *
 * \param CommandLine The command line to execute. This should include the full path to the executable and any arguments.
 * \param CurrentDirectory The working directory for the new process. May be NULL to use the default.
 * \return HRESULT Returns S_OK on success, or an error HRESULT code on failure.
 * \remarks This function is only available on Windows 10 and later, and requires wdc.dll to be present.
 */
HRESULT PhCreateProcessAsInteractiveUser(
    _In_ PCWSTR CommandLine,
    _In_ PCWSTR CurrentDirectory
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* WdcRunTaskAsInteractiveUser_I)(
        _In_ PCWSTR CommandLine,
        _In_ PCWSTR CurrentDirectory,
        _In_opt_ ULONG Flags
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"wdc.dll"))
        {
            WdcRunTaskAsInteractiveUser_I = PhGetDllBaseProcedureAddress(baseAddress, "WdcRunTaskAsInteractiveUser", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (WdcRunTaskAsInteractiveUser_I)
    {
        return WdcRunTaskAsInteractiveUser_I(CommandLine, CurrentDirectory, 0);
    }

    return E_NOTIMPL;
}

/**
 * Clones a process by creating a new process object that is a copy of the specified process.
 *
 * This function opens the source process with PROCESS_CREATE_PROCESS access and uses NtCreateProcessEx
 * to create a new process that inherits from the specified process. The new process is created in a suspended state
 * and inherits handles and address space from the parent process.
 *
 * \param[out] ProcessHandle A pointer that receives a handle to the newly created (cloned) process.
 * \param[in] ProcessId The process ID (handle) of the process to clone.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateProcessClone(
    _Out_ PHANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE cloneProcessHandle;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_PROCESS,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    status = NtCreateProcessEx(
        &cloneProcessHandle,
        PROCESS_ALL_ACCESS,
        &objectAttributes,
        processHandle,
        PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT | PROCESS_CREATE_FLAGS_INHERIT_HANDLES | PROCESS_CREATE_FLAGS_CREATE_SUSPENDED,
        NULL,
        NULL,
        NULL,
        0
        );

    NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *ProcessHandle = cloneProcessHandle;
    }

    return status;
}

/**
 * Creates a reflection (clone) of a process using RtlCreateProcessReflection.
 *
 * \param ReflectionInformation Pointer to a PROCESS_REFLECTION_INFORMATION structure that receives information about the reflected process.
 * \param ProcessHandle Handle to the process to reflect.
 * \return NTSTATUS Successful or errant status.
 * \remarks On success, the caller is responsible for releasing handles in the returned structure using PhFreeProcessReflection().
 */
NTSTATUS PhCreateProcessReflection(
    _Out_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PROCESS_REFLECTION_INFORMATION reflectionInfo = { 0 };

    status = RtlCreateProcessReflection(
        ProcessHandle,
        RTL_PROCESS_REFLECTION_FLAGS_INHERIT_HANDLES,
        NULL,
        NULL,
        NULL,
        &reflectionInfo
        );

    if (NT_SUCCESS(status))
    {
        *ReflectionInformation = reflectionInfo;
    }

    return status;
}

/**
 * Frees resources associated with a process reflection created by PhCreateProcessReflection.
 *
 * \param ReflectionInformation Pointer to a PROCESS_REFLECTION_INFORMATION structure previously initialized by PhCreateProcessReflection.
 * \remarks This function closes the thread and process handles and terminates the reflected process.
 */
VOID PhFreeProcessReflection(
    _In_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation
    )
{
    if (ReflectionInformation->ReflectionThreadHandle)
    {
        NtClose(ReflectionInformation->ReflectionThreadHandle);
    }

    if (ReflectionInformation->ReflectionProcessHandle)
    {
        NtTerminateProcess(ReflectionInformation->ReflectionProcessHandle, STATUS_SUCCESS);
        NtClose(ReflectionInformation->ReflectionProcessHandle);
    }
}

/**
 * Captures a snapshot of a process using the Process Snapshotting API.
 *
 * \param SnapshotHandle Receives a handle to the process snapshot.
 * \param ProcessHandle Handle to the process to snapshot.
 * \return NTSTATUS code indicating success or failure.
 * \remarks The caller must release the snapshot using PhFreeProcessSnapshot().
 */
NTSTATUS PhCreateProcessSnapshot(
    _Out_ PHANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE snapshotHandle = NULL;

    if (!PssNtCaptureSnapshot_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    status = PssNtCaptureSnapshot_Import()(
        &snapshotHandle,
        ProcessHandle,
        PSS_CAPTURE_VA_CLONE | PSS_CAPTURE_HANDLES | PSS_CAPTURE_HANDLE_NAME_INFORMATION |
        PSS_CAPTURE_HANDLE_BASIC_INFORMATION | PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION |
        PSS_CAPTURE_HANDLE_TRACE | PSS_CAPTURE_THREADS | PSS_CAPTURE_THREAD_CONTEXT |
        PSS_CAPTURE_THREAD_CONTEXT_EXTENDED | PSS_CAPTURE_VA_SPACE | PSS_CAPTURE_VA_SPACE_SECTION_INFORMATION |
        PSS_CREATE_BREAKAWAY | PSS_CREATE_BREAKAWAY_OPTIONAL | PSS_CREATE_USE_VM_ALLOCATIONS,
        CONTEXT_ALL // WOW64_CONTEXT_ALL?
        );

    if (NT_SUCCESS(status))
    {
        *SnapshotHandle = snapshotHandle;
    }

    return status;
}

/**
 * Frees a process snapshot and associated resources.
 *
 * \param SnapshotHandle Handle to the process snapshot.
 * \param ProcessHandle Handle to the original process.
 */
VOID PhFreeProcessSnapshot(
    _In_ HANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle
    )
{
    if (PssNtQuerySnapshot_Import())
    {
        PSS_VA_CLONE_INFORMATION processInfo = { 0 };
        PSS_HANDLE_TRACE_INFORMATION handleInfo = { 0 };

        if (NT_SUCCESS(PssNtQuerySnapshot_Import()(
            SnapshotHandle,
            PSSNT_QUERY_VA_CLONE_INFORMATION,
            &processInfo,
            sizeof(PSS_VA_CLONE_INFORMATION)
            )))
        {
            if (processInfo.VaCloneHandle)
            {
                NtClose(processInfo.VaCloneHandle);
            }
        }

        if (NT_SUCCESS(PssNtQuerySnapshot_Import()(
            SnapshotHandle,
            PSSNT_QUERY_HANDLE_TRACE_INFORMATION,
            &handleInfo,
            sizeof(PSS_HANDLE_TRACE_INFORMATION)
            )))
        {
            if (handleInfo.SectionHandle)
            {
                NtClose(handleInfo.SectionHandle);
            }
        }
    }

    if (PssNtFreeRemoteSnapshot_Import())
    {
        PssNtFreeRemoteSnapshot_Import()(ProcessHandle, SnapshotHandle);
    }

    if (PssNtFreeSnapshot_Import())
    {
        PssNtFreeSnapshot_Import()(SnapshotHandle);
    }
}

/**
 * Creates a process with redirected input/output, optionally providing input and capturing output.
 *
 * \param CommandLine The command line to execute (as a PPH_STRING).
 * \param CommandInput Optional input to write to the process's standard input.
 * \param CommandOutput Optional pointer to receive the process's standard output as a PPH_STRING.
 * \return NTSTATUS code indicating the process exit status or error.
 */
NTSTATUS PhCreateProcessRedirection(
    _In_opt_ PCPH_STRINGREF FileName,
    _In_opt_ PCPH_STRINGREF CommandLine,
    _In_opt_ PCPH_STRINGREF CommandInput,
    _Out_opt_ PPH_STRING *CommandOutput
    )
{
    static SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    NTSTATUS status;
    PPH_STRING output = NULL;
    STARTUPINFOEX startupInfo = { 0 };
    HANDLE processHandle = NULL;
    HANDLE outputReadHandle = NULL;
    HANDLE outputWriteHandle = NULL;
    HANDLE inputReadHandle = NULL;
    HANDLE inputWriteHandle = NULL;
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;
    HANDLE handleList[2];
#if defined(PH_BUILD_MSIX)
    ULONG appPolicy;
#endif
    PROCESS_BASIC_INFORMATION basicInfo;

    status = PhCreatePipeEx(
        &inputReadHandle,
        &inputWriteHandle,
        &securityAttributes,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreatePipeEx(
        &outputReadHandle,
        &outputWriteHandle,
        NULL,
        &securityAttributes
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#if !defined(PH_BUILD_MSIX)
    status = PhInitializeProcThreadAttributeList(&attributeList, 1);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    handleList[0] = inputReadHandle;
    handleList[1] = outputWriteHandle;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        handleList,
        sizeof(handleList)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;
#else
    status = PhInitializeProcThreadAttributeList(&attributeList, 2);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    handleList[0] = inputReadHandle;
    handleList[1] = outputWriteHandle;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        handleList,
        sizeof(handleList)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    appPolicy = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY,
        &appPolicy,
        sizeof(ULONG)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;
#endif

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
    startupInfo.StartupInfo.wShowWindow = SW_HIDE;
    startupInfo.StartupInfo.hStdInput = inputReadHandle;
    startupInfo.StartupInfo.hStdOutput = outputWriteHandle;
    startupInfo.StartupInfo.hStdError = outputWriteHandle;
    startupInfo.lpAttributeList = attributeList;

    status = PhCreateProcessWin32Ex(
        PhGetStringRefZ(FileName),
        PhGetStringRefZ(CommandLine),
        NULL,
        NULL,
        &startupInfo,
        PH_CREATE_PROCESS_INHERIT_HANDLES | PH_CREATE_PROCESS_NEW_CONSOLE |
        PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO,
        NULL,
        NULL,
        &processHandle,
        NULL
        );

    NtClose(inputReadHandle);
    inputReadHandle = NULL;
    NtClose(outputWriteHandle);
    outputWriteHandle = NULL;

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (CommandInput)
    {
        PhWriteFile(
            inputWriteHandle,
            CommandInput->Buffer,
            (ULONG)CommandInput->Length,
            NULL,
            NULL
            );
    }

    status = PhGetFileText(&output, outputReadHandle, TRUE);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //if (PhIsNullOrEmptyString(output))
    //{
    //    status = STATUS_UNSUCCESSFUL;
    //    goto CleanupExit;
    //}

    status = PhGetProcessBasicInformation(processHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInfo.ExitStatus;

CleanupExit:
    if (processHandle)
        NtClose(processHandle);
    if (outputWriteHandle)
        NtClose(outputWriteHandle);
    if (outputReadHandle)
        NtClose(outputReadHandle);
    if (inputReadHandle)
        NtClose(inputReadHandle);
    if (inputWriteHandle)
        NtClose(inputWriteHandle);
    if (attributeList)
        PhDeleteProcThreadAttributeList(attributeList);

    if (CommandOutput)
        *CommandOutput = output;
    else
        PhClearReference(&output);

    return status;
}

// rev from InitializeProcThreadAttributeList (dmex)
/**
 * Initializes a PROC_THREAD_ATTRIBUTE_LIST structure for process/thread attribute updates.
 *
 * \param AttributeList Receives a pointer to the allocated attribute list.
 * \param AttributeCount The number of attributes to reserve space for.
 * \return NTSTATUS code indicating success or failure.
 * \remarks The caller must free the attribute list using PhDeleteProcThreadAttributeList().
 */
NTSTATUS PhInitializeProcThreadAttributeList(
    _Out_ PPROC_THREAD_ATTRIBUTE_LIST* AttributeList,
    _In_ ULONG AttributeCount
    )
{
#if defined(PHNT_NATIVE_PROCATTRIBUTELIST)
    PPROC_THREAD_ATTRIBUTE_LIST attributeList;
    SIZE_T attributeListLength;

    InitializeProcThreadAttributeList(NULL, AttributeCount, 0, &attributeListLength);

    attributeList = PhAllocateZero(attributeListLength);
    attributeList->AttributeCount = AttributeCount;
    *AttributeList = attributeList;

    return STATUS_SUCCESS;
#else
    PPROC_THREAD_ATTRIBUTE_LIST attributeList;
    SIZE_T attributeListLength;

    attributeListLength = FIELD_OFFSET(PROC_THREAD_ATTRIBUTE_LIST, Attributes);
    attributeListLength += sizeof(PROC_THREAD_ATTRIBUTE) * AttributeCount;
    attributeList = PhAllocateZero(attributeListLength);
    attributeList->AttributeCount = AttributeCount;
    *AttributeList = attributeList;

    return STATUS_SUCCESS;
#endif
}

// rev from DeleteProcThreadAttributeList (dmex)
/**
 * Frees a PROC_THREAD_ATTRIBUTE_LIST structure previously allocated by PhInitializeProcThreadAttributeList.
 * 
 * \param AttributeList Pointer to the attribute list to free.
 * \return NTSTATUS code indicating success or failure.
 */
NTSTATUS PhDeleteProcThreadAttributeList(
    _In_ PPROC_THREAD_ATTRIBUTE_LIST AttributeList
    )
{
    PhFree(AttributeList);
    return STATUS_SUCCESS;
}

// rev from UpdateProcThreadAttribute (dmex)
/**
 * Updates a process/thread attribute in a PROC_THREAD_ATTRIBUTE_LIST.
 *
 * \param AttributeList Pointer to the attribute list.
 * \param AttributeNumber The attribute to update (e.g., PROC_THREAD_ATTRIBUTE_HANDLE_LIST).
 * \param Buffer Pointer to the attribute data.
 * \param BufferLength Size of the attribute data in bytes.
 * \return NTSTATUS code indicating success or failure.
 */
NTSTATUS PhUpdateProcThreadAttribute(
    _In_ PPROC_THREAD_ATTRIBUTE_LIST AttributeList,
    _In_ ULONG_PTR AttributeNumber,
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength
    )
{
#if defined(PHNT_NATIVE_PROCATTRIBUTELIST)
    if (!UpdateProcThreadAttribute(AttributeList, 0, AttributeNumber, Buffer, BufferLength, NULL, NULL))
        return STATUS_NO_MEMORY;

    return STATUS_SUCCESS;
#else
    if (AttributeList->LastAttribute >= AttributeList->AttributeCount)
        return STATUS_NO_MEMORY;

    AttributeList->PresentFlags |= (1 << (AttributeNumber & PROC_THREAD_ATTRIBUTE_NUMBER));
    AttributeList->Attributes[AttributeList->LastAttribute].Attribute = AttributeNumber;
    AttributeList->Attributes[AttributeList->LastAttribute].Size = BufferLength;
    AttributeList->Attributes[AttributeList->LastAttribute].Value = (ULONG_PTR)Buffer;
    AttributeList->LastAttribute++;

    return STATUS_SUCCESS;
#endif
}

/**
 * Retrieves the active computer name from the registry.
 *
 * \return A PPH_STRING containing the active computer name, or NULL if not found.
 */
PPH_STRING PhGetActiveComputerName(
    VOID
    )
{
    //static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName");
    PPH_STRING keyName;
    PPH_STRING computerName = NULL;
    HANDLE keyHandle;
    PH_FORMAT format[8];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Control");
    PhInitFormatC(&format[1], OBJ_NAME_PATH_SEPARATOR);
    PhInitFormatS(&format[2], L"Computer");
    PhInitFormatS(&format[3], L"Name");
    PhInitFormatC(&format[4], OBJ_NAME_PATH_SEPARATOR);
    PhInitFormatS(&format[5], L"Active");
    PhInitFormatS(&format[6], L"Computer");
    PhInitFormatS(&format[7], L"Name");

    keyName = PhFormat(format, RTL_NUMBER_OF(format), 0);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        computerName = PhQueryRegistryStringZ(keyHandle, L"ComputerName");
        NtClose(keyHandle);
    }

    //WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    //ULONG length = MAX_COMPUTERNAME_LENGTH + 1;
    //
    //if (GetComputerNameW(computerName, &length))
    //{
    //   return PhCreateStringEx(computerName, length * sizeof(WCHAR));
    //}

    PhDereferenceObject(keyName);

    return computerName;
}

/**
 * Queries device objects using the DevGetObjects API.
 *
 * \param ObjectType The type of device object to query.
 * \param QueryFlags Flags controlling the query.
 * \param RequestedPropertiesCount Number of requested properties.
 * \param RequestedProperties Array of requested property keys.
 * \param FilterExpressionCount Number of filter expressions.
 * \param FilterExpressions Array of filter expressions.
 * \param ObjectCount Receives the number of objects returned.
 * \param Objects Receives a pointer to the array of device objects.
 * \return HRESULT indicating success or failure.
 * \remarks The returned objects must be freed with PhDevFreeObjects().
 */
HRESULT PhDevGetObjects(
    _In_ DEV_OBJECT_TYPE ObjectType,
    _In_ DEV_QUERY_FLAGS QueryFlags,
    _In_ ULONG RequestedPropertiesCount,
    _In_reads_opt_(RequestedPropertiesCount) const DEVPROPCOMPKEY* RequestedProperties,
    _In_ ULONG FilterExpressionCount,
    _In_reads_opt_(FilterExpressionCount) const DEVPROP_FILTER_EXPRESSION* FilterExpressions,
    _Out_ PULONG ObjectCount,
    _Outptr_result_buffer_maybenull_(*ObjectCount) const DEV_OBJECT** Objects
    )
{
    HRESULT status;

    if (!DevGetObjects_Import())
        return E_FAIL;

    status = DevGetObjects_Import()(
        ObjectType,
        QueryFlags,
        RequestedPropertiesCount,
        RequestedProperties,
        FilterExpressionCount,
        FilterExpressions,
        ObjectCount,
        Objects
        );

    if (SUCCEEDED(status))
    {
        if (*ObjectCount == 0)
        {
            status = E_FAIL;
        }
    }

    return status;
}

/**
 * Frees device objects previously returned by PhDevGetObjects.
 *
 * \param ObjectCount Number of objects.
 * \param Objects Pointer to the array of device objects.
 */
VOID PhDevFreeObjects(
    _In_ ULONG ObjectCount,
    _In_reads_(ObjectCount) const DEV_OBJECT* Objects
    )
{
    if (DevFreeObjects_Import())
    {
        DevFreeObjects_Import()(ObjectCount, Objects);
    }
}

/**
 * Queries properties of a device object.
 *
 * \param ObjectType The type of device object.
 * \param ObjectId The object ID.
 * \param QueryFlags Flags controlling the query.
 * \param RequestedPropertiesCount Number of requested properties.
 * \param RequestedProperties Array of requested property keys.
 * \param PropertiesCount Receives the number of properties returned.
 * \param Properties Receives a pointer to the array of properties.
 * \return HRESULT indicating success or failure.
 * \remarks The returned properties must be freed with PhDevFreeObjectProperties().
 */
HRESULT PhDevGetObjectProperties(
    _In_ DEV_OBJECT_TYPE ObjectType,
    _In_ PCWSTR ObjectId,
    _In_ DEV_QUERY_FLAGS QueryFlags,
    _In_ ULONG RequestedPropertiesCount,
    _In_reads_(RequestedPropertiesCount) const DEVPROPCOMPKEY* RequestedProperties,
    _Out_ PULONG PropertiesCount,
    _Outptr_result_buffer_(*PropertiesCount) const DEVPROPERTY** Properties
    )
{
    if (!DevGetObjectProperties_Import())
        return E_FAIL;

    return DevGetObjectProperties_Import()(
        ObjectType,
        ObjectId,
        QueryFlags,
        RequestedPropertiesCount,
        RequestedProperties,
        PropertiesCount,
        Properties
        );
}

/**
 * Frees device object properties previously returned by PhDevGetObjectProperties.
 *
 * \param PropertiesCount Number of properties.
 * \param Properties Pointer to the array of properties.
 */
VOID PhDevFreeObjectProperties(
    _In_ ULONG PropertiesCount,
    _In_reads_(PropertiesCount) const DEVPROPERTY* Properties
    )
{
    if (DevFreeObjectProperties_Import())
    {
        DevFreeObjectProperties_Import()(PropertiesCount, Properties);
    }
}

/**
 * Creates a device object query using the DevCreateObjectQuery API.
 *
 * \param ObjectType The type of device object.
 * \param QueryFlags Flags controlling the query.
 * \param RequestedPropertiesCount Number of requested properties.
 * \param RequestedProperties Array of requested property keys.
 * \param FilterExpressionCount Number of filter expressions.
 * \param Filter Array of filter expressions.
 * \param Callback Callback function to receive query results.
 * \param Context User-defined context for the callback.
 * \param DevQuery Receives the query handle.
 * \return HRESULT indicating success or failure.
 * \remarks The query handle must be closed with PhDevCloseObjectQuery().
 */
HRESULT PhDevCreateObjectQuery(
    _In_ DEV_OBJECT_TYPE ObjectType,
    _In_ DEV_QUERY_FLAGS QueryFlags,
    _In_ ULONG RequestedPropertiesCount,
    _In_reads_opt_(RequestedPropertiesCount) const DEVPROPCOMPKEY* RequestedProperties,
    _In_ ULONG FilterExpressionCount,
    _In_reads_opt_(FilterExpressionCount) const DEVPROP_FILTER_EXPRESSION* Filter,
    _In_ PDEV_QUERY_RESULT_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_ PHDEVQUERY DevQuery
    )
{
    if (!DevCreateObjectQuery_Import())
        return E_FAIL;

    return DevCreateObjectQuery_Import()(
        ObjectType,
        QueryFlags,
        RequestedPropertiesCount,
        RequestedProperties,
        FilterExpressionCount,
        Filter,
        Callback,
        Context,
        DevQuery
        );
}

/**
 * Closes a device object query handle.
 *
 * \param QueryHandle The query handle to close.
 */
VOID PhDevCloseObjectQuery(
    _In_ HDEVQUERY QueryHandle
    )
{
    if (DevCloseObjectQuery_Import())
    {
        DevCloseObjectQuery_Import()(QueryHandle);
    }
}

/**
 * Creates a taskbar list object for progress and overlay icon management.
 *
 * \param TaskbarHandle Receives the ITaskbarList3 interface pointer as a HANDLE.
 * \return HRESULT indicating success or failure.
 * \remarks The returned handle must be released with PhTaskbarListDestroy().
 */
HRESULT PhTaskbarListCreate(
    _Out_ PHANDLE TaskbarHandle
    )
{
    HRESULT status;
    ITaskbarList3* taskbarListClass;

    status = PhGetClassObject(
        L"explorerframe.dll",
        &CLSID_TaskbarList,
        &IID_ITaskbarList3,
        &taskbarListClass
        );

    if (SUCCEEDED(status))
    {
        status = ITaskbarList3_HrInit(taskbarListClass);

        if (FAILED(status))
        {
            ITaskbarList3_Release(taskbarListClass);
            taskbarListClass = NULL;
        }
    }

    if (SUCCEEDED(status))
    {
        *TaskbarHandle = taskbarListClass;
    }

    return status;
}

/**
 * Releases a taskbar list object previously created by PhTaskbarListCreate.
 *
 * \param TaskbarHandle The taskbar handle to release.
 */
VOID PhTaskbarListDestroy(
    _In_ HANDLE TaskbarHandle
    )
{
    ITaskbarList3_Release((ITaskbarList3*)TaskbarHandle);
}

/**
 * Sets the progress value for a window on the taskbar.
 *
 * \param TaskbarHandle The taskbar handle.
 * \param WindowHandle The window handle.
 * \param Completed The completed value.
 * \param Total The total value.
 */
VOID PhTaskbarListSetProgressValue(
    _In_ HANDLE TaskbarHandle,
    _In_ HWND WindowHandle,
    _In_ ULONGLONG Completed,
    _In_ ULONGLONG Total
    )
{
    ITaskbarList3_SetProgressValue((ITaskbarList3*)TaskbarHandle, WindowHandle, Completed, Total);
}

/**
 * Sets the progress state for a window on the taskbar.
 *
 * \param TaskbarHandle The taskbar handle.
 * \param WindowHandle The window handle.
 * \param Flags Progress state flags (PH_TBLF_*).
 */
VOID PhTaskbarListSetProgressState(
    _In_ HANDLE TaskbarHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags
    )
{
    static CONST PH_FLAG_MAPPING PhTaskbarListFlagMappings[] =
    {
        { PH_TBLF_NOPROGRESS, TBPF_NOPROGRESS },
        { PH_TBLF_INDETERMINATE, TBPF_INDETERMINATE },
        { PH_TBLF_NORMAL, TBPF_NORMAL },
        { PH_TBLF_ERROR, TBPF_ERROR },
        { PH_TBLF_PAUSED, TBPF_PAUSED },
    };
    ULONG flags;

    flags = 0;
    PhMapFlags1(&flags, Flags, PhTaskbarListFlagMappings, RTL_NUMBER_OF(PhTaskbarListFlagMappings));

    ITaskbarList3_SetProgressState((ITaskbarList3*)TaskbarHandle, WindowHandle, flags);
}

/**
 * Sets an overlay icon for a window on the taskbar.
 *
 * \param TaskbarHandle The taskbar handle.
 * \param WindowHandle The window handle.
 * \param IconHandle The icon handle to set as overlay.
 * \param IconDescription Optional description for accessibility.
 */
VOID PhTaskbarListSetOverlayIcon(
    _In_ HANDLE TaskbarHandle,
    _In_ HWND WindowHandle,
    _In_opt_ HICON IconHandle,
    _In_opt_ PCWSTR IconDescription
    )
{
    ITaskbarList3_SetOverlayIcon((ITaskbarList3*)TaskbarHandle, WindowHandle, IconHandle, IconDescription);
}

/**
 * Determines if DirectX is currently running in exclusive full-screen mode.
 *
 * \return TRUE if DirectX is running full-screen, otherwise FALSE.
 */
BOOLEAN PhIsDirectXRunningFullScreen(
    VOID
    )
{
    static typeof(&D3DKMTCheckExclusiveOwnership) D3DKMTCheckExclusiveOwnership_I = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"gdi32.dll"))
        {
            D3DKMTCheckExclusiveOwnership_I = PhGetDllBaseProcedureAddress(baseAddress, "D3DKMTCheckExclusiveOwnership", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!D3DKMTCheckExclusiveOwnership_I)
        return FALSE;

    return D3DKMTCheckExclusiveOwnership_I();
}

/**
 * Releases exclusive ownership of the display from a DirectX full-screen process.
 *
 * \param ProcessHandle Handle to the process to restore.
 * \return NTSTATUS code indicating success or failure.
 */
NTSTATUS PhRestoreFromDirectXRunningFullScreen(
    _In_ HANDLE ProcessHandle
    )
{
    static typeof(&D3DKMTReleaseProcessVidPnSourceOwners) D3DKMTReleaseProcessVidPnSourceOwners_I = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"gdi32.dll"))
        {
            D3DKMTReleaseProcessVidPnSourceOwners_I = PhGetDllBaseProcedureAddress(baseAddress, "D3DKMTReleaseProcessVidPnSourceOwners", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!D3DKMTReleaseProcessVidPnSourceOwners_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return D3DKMTReleaseProcessVidPnSourceOwners_I(ProcessHandle);
}

/**
 * Queries exclusive ownership information for DirectX video present network (VidPn) sources.
 *
 * \param QueryExclusiveOwnership Pointer to a D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP structure.
 * \return NTSTATUS code indicating success or failure.
 */
NTSTATUS PhQueryDirectXExclusiveOwnership(
    _Inout_ PD3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP QueryExclusiveOwnership
    )
{
    static typeof(&D3DKMTQueryVidPnExclusiveOwnership) D3DKMTQueryVidPnExclusiveOwnership_I = NULL; // same typedef as NtGdiDdDDIQueryVidPnExclusiveOwnership
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"gdi32.dll")) // win32u.dll
        {
            D3DKMTQueryVidPnExclusiveOwnership_I = PhGetDllBaseProcedureAddress(baseAddress, "D3DKMTQueryVidPnExclusiveOwnership", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!D3DKMTQueryVidPnExclusiveOwnership_I)
        return FALSE;

    return D3DKMTQueryVidPnExclusiveOwnership_I(QueryExclusiveOwnership);
}

typedef struct _PH_ENDSESSION_CONTEXT
{
    CLIENT_ID ClientId;
    ULONG_PTR Result; // Result from the last successful PhSendMessageTimeout
    BOOLEAN Valid; // TRUE if FinalResult contains a valid value
} PH_ENDSESSION_CONTEXT, * PPH_ENDSESSION_CONTEXT;

/**
 * Callback used to send WM_QUERYENDSESSION/WM_ENDSESSION to windows of a specific process.
 *
 * Enumerates all top-level windows and, for those belonging to the process specified by Context,
 * sends WM_QUERYENDSESSION and, if accepted, WM_ENDSESSION messages.
 *
 * \param WindowHandle Handle to the window being enumerated.
 * \param Context Pointer to the process ID (as CLIENT_ID) to match against window's process.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN CALLBACK PhQueryEndSessionCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_ENDSESSION_CONTEXT context = (PPH_ENDSESSION_CONTEXT)Context;
    CLIENT_ID processId;

    // Skip invisible windows; they do not respond to session‑end queries.
    if (!IsWindowVisible(WindowHandle))
        return TRUE;

    // Check whether this window belongs to the target process.
    if (
        NT_SUCCESS(PhGetWindowClientId(WindowHandle, &processId)) &&
        processId.UniqueProcess == context->ClientId.UniqueProcess
    )
    {
        ULONG_PTR result = 0;

        // Ask the window whether it consents to session termination.
        if (PhSendMessageTimeout(
            WindowHandle,
            WM_QUERYENDSESSION,
            0,
            ENDSESSION_CLOSEAPP,
            5000,
            &result
            ))
        {
            // If the window agrees, notify it that the session is ending.
            if (result)
            {
                if (PhSendMessageTimeout(
                    WindowHandle,
                    WM_ENDSESSION,
                    TRUE,
                    ENDSESSION_LOGOFF,
                    5000,
                    &result
                    ))
                {
                    context->Result = result;
                    context->Valid = TRUE;
                }
            }

            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Sends WM_QUERYENDSESSION/WM_ENDSESSION messages to all windows belonging to the specified process.
 *
 * \param WindowHandle Handle to a window of the process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEndWindowSession(
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PH_ENDSESSION_CONTEXT context;

    status = PhGetWindowClientId(WindowHandle, &clientId);

    if (!NT_SUCCESS(status))
        return status;

    RtlZeroMemory(&context, sizeof(PH_ENDSESSION_CONTEXT));
    context.ClientId = clientId;

    status = PhEnumWindows(PhQueryEndSessionCallback, &clientId);

    if (!NT_SUCCESS(status))
        return status;

    if (context.Valid && context.Result != 0)
        return STATUS_SUCCESS;

    return STATUS_FAIL_CHECK;
}

static CONST PPH_STRINGREF PhLuidKnownTypeStrings[] =
{
    SREF(L"SYSTEM"),
    SREF(L"PROTECTED_TO_SYSTEM"),
    SREF(L"NETWORKSERVICE"),
    SREF(L"LOCALSERVICE"),
    SREF(L"ANONYMOUS_LOGON"),
    SREF(L"IUSER"),
};

/**
 * Returns a string representation for well‑known authentication LUIDs.
 *
 * \param Luid Pointer to the LUID to classify.
 * \return A PCPH_STRINGREF containing the name of the known LUID, or NULL if the
 * LUID is not one of the recognized well‑known identifiers.
 */
PCPH_STRINGREF PhGetLuidKnownTypeToString(
    _In_ PLUID Luid
    )
{
    if (RtlIsEqualLuid(Luid, &((LUID)SYSTEM_LUID)))
    {
        return (PCPH_STRINGREF)&PhLuidKnownTypeStrings[0];
    }
    else if (RtlIsEqualLuid(Luid, &((LUID)PROTECTED_TO_SYSTEM_LUID)))
    {
        return (PCPH_STRINGREF)&PhLuidKnownTypeStrings[1];
    }
    else if (RtlIsEqualLuid(Luid, &((LUID)NETWORKSERVICE_LUID)))
    {
        return (PCPH_STRINGREF)&PhLuidKnownTypeStrings[2];
    }
    else if (RtlIsEqualLuid(Luid, &((LUID)LOCALSERVICE_LUID)))
    {
        return (PCPH_STRINGREF)&PhLuidKnownTypeStrings[3];
    }
    else if (RtlIsEqualLuid(Luid, &((LUID)ANONYMOUS_LOGON_LUID)))
    {
        return (PCPH_STRINGREF)&PhLuidKnownTypeStrings[4];
    }
    else if (RtlIsEqualLuid(Luid, &((LUID)IUSER_LUID)))
    {
        return (PCPH_STRINGREF)&PhLuidKnownTypeStrings[5];
    }

    return NULL;
}


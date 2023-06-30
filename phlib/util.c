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

#include <ph.h>

#include <commdlg.h>
#include <processsnapshot.h>
#include <sddl.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <winsta.h>

#include <apiimport.h>
#include <appresolver.h>
#include <mapimg.h>
#include <mapldr.h>
#include <lsasup.h>
#include <wslsup.h>

#include "../tools/thirdparty/md5/md5.h"
#include "../tools/thirdparty/sha/sha.h"
#include "../tools/thirdparty/sha256/sha256.h"

DECLSPEC_SELECTANY WCHAR *PhSizeUnitNames[7] = { L"B", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB" };
DECLSPEC_SELECTANY ULONG PhMaxSizeUnit = ULONG_MAX;
DECLSPEC_SELECTANY USHORT PhMaxPrecisionUnit = 2;

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
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };

    if (WindowHandle)
    {
        monitor = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);
    }
    else
    {
        RECT rect;

        rect = PhRectangleToRect(*Rectangle);
        monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
    }

    if (GetMonitorInfo(monitor, &monitorInfo))
    {
        PH_RECTANGLE bounds;

        bounds = PhRectToRectangle(monitorInfo.rcWork);
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
        PH_RECTANGLE rectangle, parentRectangle;

        if (!IsWindowVisible(ParentWindowHandle) || IsMinimized(ParentWindowHandle))
            return;

        GetWindowRect(WindowHandle, &rect);
        GetWindowRect(ParentWindowHandle, &parentRect);
        rectangle = PhRectToRectangle(rect);
        parentRectangle = PhRectToRectangle(parentRect);

        PhCenterRectangle(&rectangle, &parentRectangle);
        PhAdjustRectangleToWorkingArea(WindowHandle, &rectangle);
        MoveWindow(WindowHandle, rectangle.Left, rectangle.Top,
            rectangle.Width, rectangle.Height, FALSE);
    }
    else
    {
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };

        if (GetMonitorInfo(
            MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST),
            &monitorInfo
            ))
        {
            RECT rect;
            PH_RECTANGLE rectangle;
            PH_RECTANGLE bounds;

            GetWindowRect(WindowHandle, &rect);
            rectangle = PhRectToRectangle(rect);
            bounds = PhRectToRectangle(monitorInfo.rcWork);

            PhCenterRectangle(&rectangle, &bounds);
            MoveWindow(WindowHandle, rectangle.Left, rectangle.Top,
                rectangle.Width, rectangle.Height, FALSE);
        }
    }
}

// rev from GetSystemDefaultLCID
LCID PhGetSystemDefaultLCID(
    VOID
    )
{
#if (PHNT_NATIVE_LOCALE)
    return GetSystemDefaultLCID();
#else
    if (NtQueryDefaultLocale_Import())
    {
        LCID localeId = LOCALE_SYSTEM_DEFAULT;

        if (NT_SUCCESS(NtQueryDefaultLocale_Import()(FALSE, &localeId)))
            return localeId;
    }

    return LOCALE_SYSTEM_DEFAULT; // MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT);
#endif
}

// rev from GetUserDefaultLCID
LCID PhGetUserDefaultLCID(
    VOID
    )
{
#if (PHNT_NATIVE_LOCALE)
    return GetUserDefaultLCID();
#else
    if (NtQueryDefaultLocale_Import())
    {
        LCID localeId = LOCALE_USER_DEFAULT;

        if (NT_SUCCESS(NtQueryDefaultLocale_Import()(TRUE, &localeId)))
            return localeId;
    }

    return LOCALE_USER_DEFAULT; // MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT);
#endif
}

// rev from GetThreadLocale
LCID PhGetCurrentThreadLCID(
    VOID
    )
{
#if (PHNT_NATIVE_LOCALE)
    return GetThreadLocale();
#else
    PTEB currentTeb;

    currentTeb = NtCurrentTeb();

    if (!currentTeb->CurrentLocale)
        currentTeb->CurrentLocale = PhGetUserDefaultLCID();

    return currentTeb->CurrentLocale;
#endif
}

// rev from GetSystemDefaultLangID
LANGID PhGetSystemDefaultLangID(
    VOID
    )
{
#if (PHNT_NATIVE_LOCALE)
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
#if (PHNT_NATIVE_LOCALE)
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
#if (PHNT_NATIVE_LOCALE)
    return GetUserDefaultUILanguage();
#else
    if (NtQueryDefaultUILanguage_Import())
    {
        LANGID languageId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);

        if (NT_SUCCESS(NtQueryDefaultUILanguage_Import()(&languageId)))
            return languageId;
    }

    return MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
#endif
}

// rev from GetUserDefaultLocaleName
PPH_STRING PhGetUserDefaultLocaleName(
    VOID
    )
{
#if (PHNT_NATIVE_LOCALE)
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
#if (PHNT_NATIVE_LOCALE)
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
#if (PHNT_NATIVE_TIME)
    FILETIME fileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;
    FileTimeToSystemTime(&fileTime, SystemTime);
#else
    TIME_FIELDS timeFields;

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
#if (PHNT_NATIVE_TIME)
    FILETIME fileTime;

    if (!SystemTimeToFileTime(SystemTime, &fileTime))
        return FALSE;

    LargeInteger->LowPart = fileTime.dwLowDateTime;
    LargeInteger->HighPart = fileTime.dwHighDateTime;
    return TRUE;
#else
    TIME_FIELDS timeFields;

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
#if (PHNT_NATIVE_TIME)
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

    if (!NT_NTWIN32(Status))
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
        ULONG_PTR index;

        PhTrimToNullTerminatorString(message);

        // Remove any trailing newline.
        //if (message && message->Length >= 2 * sizeof(WCHAR) &&
        //    message->Buffer[message->Length / sizeof(WCHAR) - 2] == L'\r' &&
        //    message->Buffer[message->Length / sizeof(WCHAR) - 1] == L'\n')
        //{
        //    PhMoveReference(&message, PhCreateStringEx(message->Buffer, message->Length - 2 * sizeof(WCHAR)));
        //}

        if ((index = PhFindStringInStringRefZ(&message->sr, L"\r\n", FALSE)) != SIZE_MAX)
        {
            PhMoveReference(&message, PhCreateStringEx(message->Buffer, index * sizeof(WCHAR)));
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
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
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

/**
 * Displays a message box.
 *
 * \param WindowHandle The owner window of the message box.
 * \param Type The type of message box to display.
 * \param Format A format string.
 *
 * \return The user's response.
 */
INT PhShowMessage(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ PWSTR Format,
    ...
    )
{
    INT result;
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

INT PhShowMessage2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PWSTR Icon,
    _In_opt_ PWSTR Title,
    _In_ PWSTR Format,
    ...
    )
{
    INT result;
    va_list argptr;
    PPH_STRING message;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
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

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | ((WindowHandle && IsWindowVisible(WindowHandle) && !IsMinimized(WindowHandle)) ? TDF_POSITION_RELATIVE_TO_WINDOW : 0);
    config.dwCommonButtons = buttonsFlags;
    config.hwndParent = WindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = Icon;
    config.pszMainInstruction = Title;
    config.pszContent = message->Buffer;

    if (SUCCEEDED(TaskDialogIndirect(
        &config,
        &result,
        NULL,
        NULL
        )))
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

BOOLEAN PhShowMessageOneTime(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PWSTR Icon,
    _In_opt_ PWSTR Title,
    _In_ PWSTR Format,
    ...
    )
{
    INT result;
    va_list argptr;
    PPH_STRING message;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    BOOL verificationFlagChecked = FALSE;
    ULONG buttonsFlags;

    va_start(argptr, Format);
    message = PhFormatString_V(Format, argptr);
    va_end(argptr);

    if (!message)
        return FALSE;

    buttonsFlags = 0;
    PhMapFlags1(
        &buttonsFlags,
        Buttons,
        PhShowMessageTaskDialogButtonFlagMappings,
        ARRAYSIZE(PhShowMessageTaskDialogButtonFlagMappings)
        );

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | ((WindowHandle && IsWindowVisible(WindowHandle) && !IsMinimized(WindowHandle)) ? TDF_POSITION_RELATIVE_TO_WINDOW : 0);
    config.dwCommonButtons = buttonsFlags;
    config.hwndParent = WindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = Icon;
    config.pszMainInstruction = Title;
    config.pszContent = PhGetString(message);
    config.pszVerificationText = L"Don't show this message again";
    config.cxWidth = 200;

    if (SUCCEEDED(TaskDialogIndirect(
        &config,
        &result,
        NULL,
        &verificationFlagChecked
        )))
    {
        PhDereferenceObject(message);
        return !!verificationFlagChecked;
    }
    else
    {
        PhDereferenceObject(message);
        return FALSE;
    }
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
            Status == STATUS_ACCESS_VIOLATION
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

    if (!Win32Result)
        return PhGetNtMessage(Status);
    else
        return PhGetWin32Message(Win32Result);
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
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    PPH_STRING statusMessage;

    if (statusMessage = PhGetStatusMessage(Status, Win32Result))
    {
        if (Message)
        {
            PhShowError2(WindowHandle, Message, L"%s", statusMessage->Buffer);
        }
        else
        {
            PhShowError(WindowHandle, L"%s", statusMessage->Buffer);
        }

        PhDereferenceObject(statusMessage);
    }
    else
    {
        if (Message)
        {
            PhShowError(WindowHandle, L"%s", Message);
        }
        else
        {
            PhShowError(WindowHandle, L"%s", L"Unable to perform the operation.");
        }
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
 *
 * \return TRUE if the user wishes to continue with the current operation, otherwise FALSE.
 */
BOOLEAN PhShowContinueStatus(
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    PPH_STRING statusMessage;
    INT result;

    statusMessage = PhGetStatusMessage(Status, Win32Result);

    if (Message && statusMessage)
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CLOSE_BUTTON, TD_ERROR_ICON, Message, L"%s", PhGetString(statusMessage));
    else if (Message)
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CANCEL_BUTTON, TD_ERROR_ICON, L"", L"%s", Message);
    else if (statusMessage)
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CANCEL_BUTTON, TD_ERROR_ICON, L"", L"%s", PhGetString(statusMessage));
    else
        result = PhShowMessage2(WindowHandle, TD_OK_BUTTON | TD_CANCEL_BUTTON, TD_ERROR_ICON, L"Unable to perform the operation.", L"%s", L"");

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
 *
 * \return TRUE if the user wishes to continue, otherwise FALSE.
 */
BOOLEAN PhShowConfirmMessage(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PWSTR Object,
    _In_opt_ PWSTR Message,
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
        INT button;
        TASKDIALOGCONFIG config;
        TASKDIALOG_BUTTON buttons[2];

        memset(&config, 0, sizeof(TASKDIALOGCONFIG));
        config.cbSize = sizeof(TASKDIALOGCONFIG);
        config.hwndParent = WindowHandle;
        config.hInstance = PhInstanceHandle;
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

        if (SUCCEEDED(TaskDialogIndirect(
            &config,
            &button,
            NULL,
            NULL
            )))
        {
            return button == IDYES;
        }
        else
        {
            return PhShowMessage(
                WindowHandle,
                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
                L"Are you sure you want to %s?",
                action->Buffer
                ) == IDYES;
        }
    }
}

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
BOOLEAN PhFindIntegerSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PWSTR String,
    _Out_ PULONG Integer
    )
{
    for (ULONG i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (PhEqualStringZ(KeyValuePairs[i].Key, String, TRUE))
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
BOOLEAN PhFindStringSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
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
BOOLEAN PhFindIntegerSiKeyValuePairsStringRef(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PPH_STRINGREF String,
    _Out_ PULONG Integer
    )
{
    for (ULONG i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (PhEqualStringRef(KeyValuePairs[i].Key, String, TRUE))
        {
            *Integer = PtrToUlong(KeyValuePairs[i].Value);
            return TRUE;
        }
    }

    return FALSE;
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
    LARGE_INTEGER seed;
    // The top/sign bit is always unusable for RtlRandomEx (the result is always unsigned), so we'll
    // take the bottom 24 bits. We need 128 bits in total, so we'll call the function 6 times.
    ULONG random[6];
    ULONG i;

    PhQueryPerformanceCounter(&seed);

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

/**
 * Fills a buffer with random uppercase alphabetical characters.
 *
 * \param Buffer The buffer to fill with random characters, plus a null terminator.
 * \param Count The number of characters available in the buffer, including space for the null
 * terminator.
 */
VOID PhGenerateRandomAlphaString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ ULONG Count
    )
{
    LARGE_INTEGER seed;
    ULONG i;

    if (Count == 0)
        return;

    PhQueryPerformanceCounter(&seed);

    for (i = 0; i < Count - 1; i++)
    {
        Buffer[i] = L'A' + (RtlRandomEx(&seed.LowPart) % 26);
    }

    Buffer[Count - 1] = UNICODE_NULL;
}

ULONG64 PhGenerateRandomNumber64(
    VOID
    )
{
    LARGE_INTEGER seed;

    PhQueryPerformanceCounter(&seed);

    return (ULONG64)RtlRandomEx(&seed.LowPart) | ((ULONG64)RtlRandomEx(&seed.LowPart) << 31);
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
 *
 * \return The new string.
 */
PPH_STRING PhEllipsisString(
    _In_ PPH_STRING String,
    _In_ SIZE_T DesiredCount
    )
{
    if (
        (ULONG)String->Length / sizeof(WCHAR) <= DesiredCount ||
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
 *
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
    _In_ PWSTR Pattern,
    _In_ PWSTR String,
    _In_ BOOLEAN IgnoreCase
    )
{
    PWCHAR s, p;
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
    _In_ PWSTR Pattern,
    _In_ PWSTR String,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return PhpMatchWildcards(Pattern, String, FALSE);
    else
        return PhpMatchWildcards(Pattern, String, TRUE);
}

/**
 * Escapes a string for prefix characters (ampersands).
 *
 * \param String The string to process.
 *
 * \return The escaped string, with each ampersand replaced by 2 ampersands.
 */
PPH_STRING PhEscapeStringForMenuPrefix(
    _In_ PPH_STRINGREF String
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T i;
    SIZE_T length;
    PWCHAR runStart;
    SIZE_T runCount;

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
    _In_ PWSTR A,
    _In_ PWSTR B,
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
    _In_opt_ PWSTR Format
    )
{
    PPH_STRING string;
    INT bufferSize;

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
    _In_opt_ PWSTR Format
    )
{
    PPH_STRING string;
    INT bufferSize;

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
    INT timeBufferSize;
    INT dateBufferSize;
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
    DOUBLE days;
    DOUBLE weeks;
    DOUBLE fortnights;
    DOUBLE months;
    DOUBLE years;
    DOUBLE centuries;

    days = (DOUBLE)TimeSpan / PH_TICKS_PER_DAY;
    weeks = days / 7;
    fortnights = weeks / 2;
    years = days / 365.2425;
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
        DOUBLE milliseconds;
        DOUBLE seconds;
        DOUBLE minutes;
        DOUBLE hours;
        ULONG secondsPartial;
        ULONG minutesPartial;
        ULONG hoursPartial;

        milliseconds = (DOUBLE)TimeSpan / PH_TICKS_PER_MS;
        seconds = (DOUBLE)TimeSpan / PH_TICKS_PER_SEC;
        minutes = (DOUBLE)TimeSpan / PH_TICKS_PER_MIN;
        hours = (DOUBLE)TimeSpan / PH_TICKS_PER_HOUR;

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

// Formats using prefix (1000=1k, 1000000=1M, 1000000000000=1B) (dmex)
PPH_STRING PhFormatUInt64Prefix(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    )
{
    static PH_STRINGREF PhpPrefixUnitNamesCounted[7] =
    {
        PH_STRINGREF_INIT(L""),
        PH_STRINGREF_INIT(L"k"),
        PH_STRINGREF_INIT(L"M"),
        PH_STRINGREF_INIT(L"B"),
        PH_STRINGREF_INIT(L"T"),
        PH_STRINGREF_INIT(L"P"),
        PH_STRINGREF_INIT(L"E")
    };
    DOUBLE number = (DOUBLE)Value;
    ULONG i = 0;
    PH_FORMAT format[2];

    while (
        number >= 1000 &&
        i < RTL_NUMBER_OF(PhpPrefixUnitNamesCounted) &&
        i < PhMaxSizeUnit
        )
    {
        number /= 1000;
        i++;
    }

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros | (GroupDigits ? FormatGroupDigits : 0);
    format[0].Precision = 2;
    format[0].u.Double = number;
    PhInitFormatSR(&format[1], PhpPrefixUnitNamesCounted[i]);

    return PhFormat(format, 2, 0);
}

PPH_STRING PhFormatDecimal(
    _In_ PWSTR Value,
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
        if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimalSeparator, 4))
        {
            decimalSeparator[0] = L'.';
            decimalSeparator[1] = UNICODE_NULL;
        }

        if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousandSeparator, 4))
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

    bufferSize = GetNumberFormat(LOCALE_USER_DEFAULT, 0, Value, &format, NULL, 0);
    string = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));

    if (!GetNumberFormat(LOCALE_USER_DEFAULT, 0, Value, &format, string->Buffer, bufferSize))
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

NTSTATUS PhStringToGuid(
    _In_ PPH_STRINGREF GuidString,
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
 *
 * \return A version information block. You must free this using PhFree() when you no longer need
 * it.
 */
PVOID PhGetFileVersionInfo(
    _In_ PWSTR FileName
    )
{
    PVOID libraryModule;
    PVOID versionInfo;

    libraryModule = LoadLibraryEx(
        FileName,
        NULL,
        LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE
        );

    if (!libraryModule)
        return NULL;

    if (PhLoadResourceCopy(
        libraryModule,
        MAKEINTRESOURCE(VS_VERSION_INFO),
        VS_FILE_INFO,
        NULL,
        &versionInfo
        ))
    {
        if (PhIsFileVersionInfo32(versionInfo))
        {
            PhFreeLibrary(libraryModule);
            return versionInfo;
        }

        PhFree(versionInfo);
    }

    PhFreeLibrary(libraryModule);
    return NULL;
}

PVOID PhGetFileVersionInfoEx(
    _In_ PPH_STRINGREF FileName
    )
{
    PVOID imageBaseAddress;
    PVOID versionInfo;

    if (!NT_SUCCESS(PhLoadLibraryAsImageResource(FileName, &imageBaseAddress)))
        return NULL;

    if (PhLoadResourceCopy(
        imageBaseAddress,
        MAKEINTRESOURCE(VS_VERSION_INFO),
        VS_FILE_INFO,
        NULL,
        &versionInfo
        ))
    {
        if (PhIsFileVersionInfo32(versionInfo))
        {
            PhFreeLibraryAsImageResource(imageBaseAddress);
            return versionInfo;
        }

        PhFree(versionInfo);
    }

    PhFreeLibraryAsImageResource(imageBaseAddress);
    return NULL;
}

PVOID PhGetFileVersionInfoValue(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo
    )
{
    PWSTR keyOffset = VersionInfo->Key + PhCountStringZ(VersionInfo->Key) + 1;

    return PTR_ADD_OFFSET(VersionInfo, ALIGN_UP(PTR_SUB_OFFSET(keyOffset, VersionInfo), ULONG));
}

_Success_(return)
BOOLEAN PhGetFileVersionInfoKey(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo,
    _In_ SIZE_T KeyLength,
    _In_ PWSTR Key,
    _Out_opt_ PVOID* Buffer
    )
{
    PVOID value;
    ULONG valueOffset;
    PVS_VERSION_INFO_STRUCT32 child;

    if (!(value = PhGetFileVersionInfoValue(VersionInfo)))
        return FALSE;

    valueOffset = VersionInfo->ValueLength * (VersionInfo->Type ? sizeof(WCHAR) : sizeof(BYTE));
    child = PTR_ADD_OFFSET(value, ALIGN_UP(valueOffset, ULONG));

    while ((ULONG_PTR)child < (ULONG_PTR)PTR_ADD_OFFSET(VersionInfo, VersionInfo->Length))
    {
        if (_wcsnicmp(child->Key, Key, KeyLength) == 0 && child->Key[KeyLength] == UNICODE_NULL)
        {
            if (Buffer)
                *Buffer = child;

            return TRUE;
        }

        if (child->Length == 0)
            break;

        child = PTR_ADD_OFFSET(child, ALIGN_UP(child->Length, ULONG));
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhGetFileVersionVarFileInfoValue(
    _In_ PVOID VersionInfo,
    _In_ PPH_STRINGREF KeyName,
    _Out_opt_ PVOID* Buffer,
    _Out_opt_ PULONG BufferLength
    )
{
    static PH_STRINGREF varfileBlockName = PH_STRINGREF_INIT(L"VarFileInfo");
    PVS_VERSION_INFO_STRUCT32 varfileBlockInfo;
    PVS_VERSION_INFO_STRUCT32 varfileBlockValue;

    if (PhGetFileVersionInfoKey(
        VersionInfo,
        varfileBlockName.Length / sizeof(WCHAR),
        varfileBlockName.Buffer,
        &varfileBlockInfo
        ))
    {
        if (PhGetFileVersionInfoKey(
            varfileBlockInfo,
            KeyName->Length / sizeof(WCHAR),
            KeyName->Buffer,
            &varfileBlockValue
            ))
        {
            if (BufferLength)
                *BufferLength = varfileBlockValue->ValueLength;
            if (Buffer)
                *Buffer = PhGetFileVersionInfoValue(varfileBlockValue);
            return TRUE;
        }
    }

    return FALSE;
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
    static PH_STRINGREF translationName = PH_STRINGREF_INIT(L"Translation");
    PLANGANDCODEPAGE codePage;
    ULONG codePageLength;

    if (PhGetFileVersionVarFileInfoValue(VersionInfo, &translationName, &codePage, &codePageLength))
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
    _In_ PWSTR SubBlock
    )
{
    PH_STRINGREF name;
    PWSTR buffer;
    ULONG length;

    PhInitializeStringRefLongHint(&name, SubBlock);

    if (PhGetFileVersionVarFileInfoValue(
        VersionInfo,
        &name,
        &buffer,
        &length
        ))
    {
        PPH_STRING string;

        // Check if the string has a valid length.
        if (length <= sizeof(UNICODE_NULL))
            return NULL;

        string = PhCreateStringEx(buffer, length * sizeof(WCHAR));
        // length may include the null terminator.
        PhTrimToNullTerminatorString(string);

        return string;
    }
    else
    {
        return NULL;
    }
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
    _In_ PPH_STRINGREF KeyName
    )
{
    static PH_STRINGREF blockInfoName = PH_STRINGREF_INIT(L"StringFileInfo");
    PVS_VERSION_INFO_STRUCT32 blockStringInfo;
    PVS_VERSION_INFO_STRUCT32 blockLangInfo;
    PVS_VERSION_INFO_STRUCT32 stringNameBlockInfo;
    PWSTR stringNameBlockValue;
    PPH_STRING string;
    SIZE_T returnLength;
    PH_FORMAT format[3];
    WCHAR langNameString[65];

    if (!PhGetFileVersionInfoKey(
        VersionInfo,
        blockInfoName.Length / sizeof(WCHAR),
        blockInfoName.Buffer,
        &blockStringInfo
        ))
    {
        return NULL;
    }

    PhInitFormatX(&format[0], LangCodePage);
    format[0].Type |= FormatPadZeros | FormatUpperCase;
    format[0].Width = 8;

    if (!PhFormatToBuffer(format, 1, langNameString, sizeof(langNameString), &returnLength))
        return NULL;

    if (!PhGetFileVersionInfoKey(
        blockStringInfo,
        returnLength / sizeof(WCHAR) - sizeof(ANSI_NULL),
        langNameString,
        &blockLangInfo
        ))
    {
        return NULL;
    }

    if (!PhGetFileVersionInfoKey(
        blockLangInfo,
        KeyName->Length / sizeof(WCHAR),
        KeyName->Buffer,
        &stringNameBlockInfo
        ))
    {
        return NULL;
    }

    if (stringNameBlockInfo->ValueLength <= sizeof(UNICODE_NULL)) // Check if the string has a valid length.
        return NULL;
    if (!(stringNameBlockValue = PhGetFileVersionInfoValue(stringNameBlockInfo)))
        return NULL;

    string = PhCreateStringEx(
        stringNameBlockValue,
        UInt32x32To64((stringNameBlockInfo->ValueLength - 1), sizeof(WCHAR))
        );
    //PhTrimToNullTerminatorString(string); // length may include the null terminator.

    return string;
}

PPH_STRING PhGetFileVersionInfoStringEx(
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage,
    _In_ PPH_STRINGREF KeyName
    )
{
    PPH_STRING string;

    // Use the default language code page.
    if (string = PhGetFileVersionInfoString2(VersionInfo, LangCodePage, KeyName))
        return string;

    // Use the windows-1252 code page.
    if (string = PhGetFileVersionInfoString2(VersionInfo, (LangCodePage & 0xffff0000) + 1252, KeyName))
        return string;

    // Use the default language (US English).
    if (string = PhGetFileVersionInfoString2(VersionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 1252, KeyName))
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
    static PH_STRINGREF companyName = PH_STRINGREF_INIT(L"CompanyName");
    static PH_STRINGREF fileDescription = PH_STRINGREF_INIT(L"FileDescription");
    static PH_STRINGREF productName = PH_STRINGREF_INIT(L"ProductName");

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
    static PH_STRINGREF fileVersion = PH_STRINGREF_INIT(L"FileVersion");
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
_Success_(return)
BOOLEAN PhInitializeImageVersionInfo(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PWSTR FileName
    )
{
    PVOID versionInfo;
    ULONG langCodePage;

    versionInfo = PhGetFileVersionInfo(FileName);

    if (!versionInfo)
        return FALSE;

    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);
    PhpGetImageVersionVersionString(ImageVersionInfo, versionInfo);
    PhpGetImageVersionInfoFields(ImageVersionInfo, versionInfo, langCodePage);

    PhFree(versionInfo);
    return TRUE;
}

_Success_(return)
BOOLEAN PhInitializeImageVersionInfoEx(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN ExtendedVersionInfo
    )
{
    PVOID versionInfo;
    ULONG langCodePage;

    versionInfo = PhGetFileVersionInfoEx(FileName);

    if (!versionInfo)
        return FALSE;

    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);
    if (ExtendedVersionInfo)
        PhpGetImageVersionVersionStringEx(ImageVersionInfo, versionInfo, langCodePage);
    else
        PhpGetImageVersionVersionString(ImageVersionInfo, versionInfo);
    PhpGetImageVersionInfoFields(ImageVersionInfo, versionInfo, langCodePage);

    PhFree(versionInfo);
    return TRUE;
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
    _In_opt_ PPH_STRINGREF Indent,
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

static BOOLEAN PhpImageVersionInfoCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry1 = Entry1;
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry2 = Entry2;

    return PhEqualString(entry1->FileName, entry2->FileName, FALSE);
}

static ULONG PhpImageVersionInfoCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_FILE_VERSIONINFO_CACHE_ENTRY entry = Entry;

    return PhHashStringRefEx(&entry->FileName->sr, FALSE, PH_STRING_HASH_X65599);
}

_Success_(return)
BOOLEAN PhInitializeImageVersionInfoCached(
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

            return TRUE;
        }
    }

    if (IsSubsystemProcess)
    {
        if (!PhInitializeLxssImageVersionInfo(&versionInfo, &FileName->sr))
        {
            // Note: If the function fails the version info is empty. For extra performance
            // we still update the cache with a reference to the filename (without version info)
            // and just return the empty result from the cache instead of loading the same file again.
            // We flush every few hours so the cache doesn't waste memory or become state. (dmex)
        }
    }
    else
    {
        if (!PhInitializeImageVersionInfoEx(&versionInfo, &FileName->sr, ExtendedVersion))
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
    return TRUE;
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
 * Gets an absolute file name.
 *
 * \param FileName A file name.
 * \param FullPath An absolute file name, or NULL if the function failed.
 * \param IndexOfFileName A variable which receives the index of the base name.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetFullPath(
    _In_ PWSTR FileName,
    _Out_ PPH_STRING *FullPath,
    _Out_opt_ PULONG IndexOfFileName
    )
{
    NTSTATUS status;
    PPH_STRING fullPath;
    ULONG fullPathLength;
    ULONG returnLength;
    PWSTR filePart;

#ifdef DEBUG
    assert(RtlEqualMemory(FileName, RtlNtPathSeperatorString.Buffer, RtlNtPathSeperatorString.Length) == FALSE);
#endif

    fullPathLength = DOS_MAX_PATH_LENGTH;
    fullPath = PhCreateStringEx(NULL, fullPathLength);

    status = RtlGetFullPathName_UEx(
        FileName,
        fullPathLength,
        fullPath->Buffer,
        &filePart,
        &returnLength
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(fullPath);
        return status;
    }

    if (returnLength > fullPathLength)
    {
        PhDereferenceObject(fullPath);
        fullPathLength = returnLength;
        fullPath = PhCreateStringEx(NULL, fullPathLength);

        status = RtlGetFullPathName_UEx(
            FileName,
            fullPathLength,
            fullPath->Buffer,
            &filePart,
            &returnLength
            );

        if (!NT_SUCCESS(status))
        {
            PhDereferenceObject(fullPath);
            return status;
        }
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
    _In_ PPH_STRINGREF String
    )
{
    NTSTATUS status;
    UNICODE_STRING inputString;
    UNICODE_STRING outputString;
    PPH_STRING string;
    ULONG bufferLength;

    if (!PhStringRefToUnicodeString(String, &inputString))
        return NULL;

    bufferLength = 0x80;
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
        return PhReferenceObject(FileName);

    return PhCreateString2(&baseNamePart);
}

PPH_STRING PhGetBaseNameChangeExtension(
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_STRINGREF FileExtension
    )
{
    ULONG_PTR indexOfBackslash;
    ULONG_PTR indexOfLastDot;
    PH_STRINGREF baseFileName;
    PH_STRINGREF baseFilePath;

    if ((indexOfBackslash = PhFindLastCharInStringRef(FileName, OBJ_NAME_PATH_SEPARATOR, FALSE)) == SIZE_MAX)
        return NULL;
    if ((indexOfLastDot = PhFindLastCharInStringRef(FileName, L'.', FALSE)) == SIZE_MAX)
        return NULL;
    if (indexOfLastDot < indexOfBackslash)
        return NULL;

    baseFileName.Buffer = FileName->Buffer + indexOfBackslash + 1;
    baseFileName.Length = (indexOfLastDot - indexOfBackslash - 1) * sizeof(WCHAR);
    baseFilePath.Buffer = FileName->Buffer;
    baseFilePath.Length = (indexOfBackslash + 1) * sizeof(WCHAR);

    return PhConcatStringRef3(&baseFilePath, &baseFileName, FileExtension);
}

_Success_(return)
BOOLEAN PhGetBasePath(
    _In_ PPH_STRINGREF FileName,
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
    static PH_STRINGREF system32String = PH_STRINGREF_INIT(L"\\System32");
    static PPH_STRING cachedSystemDirectory = NULL;
    PPH_STRING systemDirectory;
    PH_STRINGREF systemRootString;

    // Use the cached value if possible.

    if (systemDirectory = InterlockedCompareExchangePointer(&cachedSystemDirectory, NULL, NULL))
        return PhReferenceObject(systemDirectory);

    PhGetSystemRoot(&systemRootString);
    systemDirectory = PhConcatStringRef2(&systemRootString, &system32String);

    // Try to cache the value.
    if (InterlockedCompareExchangePointer(
        &cachedSystemDirectory,
        systemDirectory,
        NULL
        ) == NULL)
    {
        // Success, add one more reference for the cache.
        PhReferenceObject(systemDirectory);
    }

    return systemDirectory;
}

PPH_STRING PhGetSystemDirectoryWin32(
    _In_opt_ PPH_STRINGREF AppendPath
    )
{
    static PH_STRINGREF system32String = PH_STRINGREF_INIT(L"\\System32");
    PH_STRINGREF systemRootString;

    PhGetSystemRoot(&systemRootString);

    if (AppendPath)
    {
        return PhConcatStringRef3(&systemRootString, &system32String, AppendPath);
    }

    return PhConcatStringRef2(&systemRootString, &system32String);
}

/**
 * Retrieves the Windows directory path.
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

    localSystemRoot.Buffer = USER_SHARED_DATA->NtSystemRoot;
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
 * Retrieves the Windows directory path.
 */
PPH_STRING PhGetWindowsDirectory(
    _In_opt_ PPH_STRINGREF AppendPath
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
    _In_opt_ PPH_STRINGREF AppendPath
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

PPH_STRING PhGetApplicationFileName(
    VOID
    )
{
    static PPH_STRING cachedFileName = NULL;
    PPH_STRING fileName;

    if (fileName = InterlockedCompareExchangePointer(
        &cachedFileName,
        NULL,
        NULL
        ))
    {
        return PhReferenceObject(fileName);
    }

    if (!NT_SUCCESS(PhGetProcessImageFileName(NtCurrentProcess(), &fileName)))
    {
        if (!NT_SUCCESS(PhGetProcessImageFileNameByProcessId(NtCurrentProcessId(), &fileName)))
        {
            PhGetProcessMappedFileName(NtCurrentProcess(), PhInstanceHandle, &fileName);
        }
    }

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

    if (fileName = InterlockedCompareExchangePointer(
        &cachedFileName,
        NULL,
        NULL
        ))
    {
        return PhReferenceObject(fileName);
    }

    //if (fileName = PhGetDllFileName(PhInstanceHandle, NULL))
    //{
    //    PPH_STRING fullPath;
    //
    //    if (NT_SUCCESS(PhGetFullPath(PhGetString(fileName), &fullPath, NULL)))
    //    {
    //        PhMoveReference(&fileName, fullPath);
    //    }
    //}

    if (!NT_SUCCESS(PhGetProcessImageFileNameWin32(NtCurrentProcess(), &fileName)))
    {
        if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), PhInstanceHandle, &fileName)))
        {
            PhMoveReference(&fileName, PhGetFileName(fileName));
        }
        else if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(NtCurrentProcessId(), &fileName)))
        {
            PhMoveReference(&fileName, PhGetFileName(fileName));
        }
    }

    //if (fileName = PhGetApplicationFileName())
    //{
    //    PhMoveReference(&fileName, PhGetFileName(fileName));
    //}

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

PPH_STRING PhGetApplicationDirectory(
    VOID
    )
{
    static PPH_STRING cachedDirectoryPath = NULL;
    PPH_STRING directoryPath;
    PPH_STRING fileName;

    if (directoryPath = InterlockedCompareExchangePointer(
        &cachedDirectoryPath,
        NULL,
        NULL
        ))
    {
        return PhReferenceObject(directoryPath);
    }

    if (fileName = PhGetApplicationFileName())
    {
        ULONG_PTR indexOfFileName;

        indexOfFileName = PhFindLastCharInString(fileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfFileName != SIZE_MAX)
            indexOfFileName++;
        else
            indexOfFileName = 0;

        if (indexOfFileName != 0)
        {
            directoryPath = PhSubstring(fileName, 0, indexOfFileName);
        }

        PhDereferenceObject(fileName);
    }

    if (InterlockedCompareExchangePointer(
        &cachedDirectoryPath,
        directoryPath,
        NULL
        ) == NULL)
    {
        PhReferenceObject(directoryPath);
    }

    return directoryPath;
}

/**
 * Retrieves the directory of the current process image.
 */
PPH_STRING PhGetApplicationDirectoryWin32(
    VOID
    )
{
    static PPH_STRING cachedDirectoryPath = NULL;
    PPH_STRING directoryPath;
    PPH_STRING fileName;

    if (directoryPath = InterlockedCompareExchangePointer(
        &cachedDirectoryPath,
        NULL,
        NULL
        ))
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

        if (indexOfFileName != 0)
        {
            directoryPath = PhSubstring(fileName, 0, indexOfFileName);
        }

        PhDereferenceObject(fileName);
    }

    if (InterlockedCompareExchangePointer(
        &cachedDirectoryPath,
        directoryPath,
        NULL
        ) == NULL)
    {
        PhReferenceObject(directoryPath);
    }

    return directoryPath;
}

PPH_STRING PhGetApplicationDirectoryFileName(
    _In_ PPH_STRINGREF FileName,
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
        PhReferenceObject(applicationDirectory);
    }

    return applicationFileName;
}

PPH_STRING PhGetTemporaryDirectoryRandomAlphaFileName(
    VOID
    )
{
    PH_STRINGREF randomAlphaString;
    WCHAR randomAlphaStringBuffer[33] = L"";

    PhGenerateRandomAlphaString(randomAlphaStringBuffer, ARRAYSIZE(randomAlphaStringBuffer));
    randomAlphaStringBuffer[0] = OBJ_NAME_PATH_SEPARATOR;
    randomAlphaString.Buffer = randomAlphaStringBuffer;
    randomAlphaString.Length = sizeof(randomAlphaStringBuffer) - sizeof(UNICODE_NULL);

    return PhGetTemporaryDirectory(&randomAlphaString);
}

PPH_STRING PhGetRoamingAppDataDirectory(
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    PPH_STRING roamingAppDataFileName = NULL;
    PPH_STRING roamingAppDataDirectory;

    if (roamingAppDataDirectory = PhGetKnownLocationZ(PH_FOLDERID_RoamingAppData, L"\\SystemInformer\\", NativeFileName))
    {
        roamingAppDataFileName = PhConcatStringRef2(&roamingAppDataDirectory->sr, FileName);
        PhReferenceObject(roamingAppDataDirectory);
    }

    return roamingAppDataFileName;
}

PPH_STRING PhGetApplicationDataFileName(
    _In_ PPH_STRINGREF FileName,
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

/**
 * Retrieves the full path of a known folder.
 *
 * \param Folder A reference that identifies the folder.
 * \param AppendPath The string to append to the path.
 * \param NativeFileName A boolean to return the native path or Win32 path.
 */
PPH_STRING PhGetKnownLocation(
    _In_ ULONG Folder,
    _In_opt_ PPH_STRINGREF AppendPath,
    _In_ BOOLEAN NativeFileName
    )
{
#if !defined(PH_BUILD_MSIX)
    switch (Folder)
    {
    case PH_FOLDERID_LocalAppData:
        {
            static UNICODE_STRING variableName = RTL_CONSTANT_STRING(L"LOCALAPPDATA");
            static UNICODE_STRING variableNameProfile = RTL_CONSTANT_STRING(L"USERPROFILE");
            NTSTATUS status;
            UNICODE_STRING variableValue;
            WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

            RtlInitEmptyUnicodeString(&variableValue, variableBuffer, sizeof(variableBuffer));

            status = RtlQueryEnvironmentVariable_U(
                NULL,
                &variableName,
                &variableValue
                );

            if (!NT_SUCCESS(status))
            {
                status = RtlQueryEnvironmentVariable_U(
                    NULL,
                    &variableNameProfile,
                    &variableValue
                    );
            }

            if (NT_SUCCESS(status))
            {
                PH_STRINGREF string;

                PhUnicodeStringToStringRef(&variableValue, &string);

                if (AppendPath)
                {
                    if (NativeFileName)
                    {
                        PPH_STRING fileName;

                        if (fileName = PhDosPathNameToNtPathName(&string))
                        {
                            PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, AppendPath));
                        }

                        return fileName;
                    }

                    return PhConcatStringRef2(&string, AppendPath);
                }
                else
                {
                    if (NativeFileName)
                    {
                        return PhDosPathNameToNtPathName(&string);
                    }

                    return PhCreateString2(&string);
                }
            }
        }
        break;
    case PH_FOLDERID_RoamingAppData:
        {
            static UNICODE_STRING variableName = RTL_CONSTANT_STRING(L"APPDATA");
            static UNICODE_STRING variableNameProfile = RTL_CONSTANT_STRING(L"USERPROFILE");
            NTSTATUS status;
            UNICODE_STRING variableValue;
            WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

            RtlInitEmptyUnicodeString(&variableValue, variableBuffer, sizeof(variableBuffer));

            status = RtlQueryEnvironmentVariable_U(
                NULL,
                &variableName,
                &variableValue
                );

            if (!NT_SUCCESS(status))
            {
                status = RtlQueryEnvironmentVariable_U(
                    NULL,
                    &variableNameProfile,
                    &variableValue
                    );
            }

            if (NT_SUCCESS(status))
            {
                PH_STRINGREF string;

                PhUnicodeStringToStringRef(&variableValue, &string);

                if (AppendPath)
                {
                    if (NativeFileName)
                    {
                        PPH_STRING fileName;

                        if (fileName = PhDosPathNameToNtPathName(&string))
                        {
                            PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, AppendPath));
                        }

                        return fileName;
                    }

                    return PhConcatStringRef2(&string, AppendPath);
                }
                else
                {
                    if (NativeFileName)
                    {
                        return PhDosPathNameToNtPathName(&string);
                    }

                    return PhCreateString2(&string);
                }
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
    }
#endif
    return NULL;
}

PPH_STRING PhGetKnownFolderPath(
    _In_ PCGUID Folder,
    _In_opt_ PPH_STRINGREF AppendPath
    )
{
    return PhGetKnownFolderPathEx(Folder, 0, NULL, AppendPath);
}

static const PH_FLAG_MAPPING PhpKnownFolderFlagMappings[] =
{
    { PH_KF_FLAG_FORCE_PACKAGE_REDIRECTION, KF_FLAG_FORCE_APP_DATA_REDIRECTION },
    { PH_KF_FLAG_FORCE_APPCONTAINER_REDIRECTION, KF_FLAG_FORCE_APPCONTAINER_REDIRECTION },
};

PPH_STRING PhGetKnownFolderPathEx(
    _In_ PCGUID Folder,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PPH_STRINGREF AppendPath
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
PPH_STRING PhGetTemporaryDirectory(
    _In_opt_ PPH_STRINGREF AppendPath
    )
{
    static UNICODE_STRING variableNameTmp = RTL_CONSTANT_STRING(L"TMP");
    static UNICODE_STRING variableNameTemp = RTL_CONSTANT_STRING(L"TEMP");
    static UNICODE_STRING variableNameProfile = RTL_CONSTANT_STRING(L"USERPROFILE");
    NTSTATUS status;
    UNICODE_STRING variableValue;
    WCHAR variableBuffer[DOS_MAX_PATH_LENGTH];

    if (
        PhGetOwnTokenAttributes().Elevated ||
        PhEqualSid(PhGetOwnTokenAttributes().TokenSid, &PhSeLocalSystemSid)
        )
    {
        static PH_STRINGREF systemTemp = PH_STRINGREF_INIT(L"SystemTemp");
        PH_STRINGREF systemRoot;
        PPH_STRING systemPath;

        PhGetSystemRoot(&systemRoot);

        if (AppendPath)
        {
            systemPath = PhConcatStringRef4(&systemRoot, &PhNtPathSeperatorString, &systemTemp, AppendPath);
        }
        else
        {
            systemPath = PhConcatStringRef3(&systemRoot, &PhNtPathSeperatorString, &systemTemp);
        }

        if (PhDoesDirectoryExistWin32(PhGetString(systemPath))) // Required for Windows 7/8 (dmex)
        {
            return systemPath;
        }

        PhDereferenceObject(systemPath);
    }

    RtlInitEmptyUnicodeString(&variableValue, variableBuffer, sizeof(variableBuffer));

    status = RtlQueryEnvironmentVariable_U(
        NULL,
        &variableNameTmp,
        &variableValue
        );

    if (!NT_SUCCESS(status))
    {
        status = RtlQueryEnvironmentVariable_U(
            NULL,
            &variableNameTemp,
            &variableValue
            );

        if (!NT_SUCCESS(status))
        {
            status = RtlQueryEnvironmentVariable_U(
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
            PH_STRINGREF string;

            PhUnicodeStringToStringRef(&variableValue, &string);

            return PhConcatStringRef2(&string, AppendPath);
        }
        else
        {
            return PhCreateStringFromUnicodeString(&variableValue);
        }
    }

    return NULL;
}

/**
 * Waits on multiple objects while processing window messages.
 *
 * \param WindowHandle The window to process messages for, or NULL to process all messages for the current
 * thread.
 * \param NumberOfHandles The number of handles specified in \a Handles. This must not be greater
 * than MAXIMUM_WAIT_OBJECTS - 1.
 * \param Handles An array of handles.
 * \param Timeout The number of milliseconds to wait on the objects, or INFINITE for no timeout.
 * \param WakeMask The input types for which an input event object handle will be added to the array of object handles.
 *
 * \remarks The wait is always in WaitAny mode.
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
            OBJ_CASE_INSENSITIVE,
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
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PWSTR CurrentDirectory,
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
    )
{
    NTSTATUS status;
    PPH_STRING fileName = NULL;
    PPH_STRING commandLine = NULL;
    PPH_STRING currentDirectory = NULL;
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
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
            static PH_STRINGREF seperator = PH_STRINGREF_INIT(L"\"");
            static PH_STRINGREF space = PH_STRINGREF_INIT(L" ");
            PPH_STRING escaped;

            escaped = PhConcatStringRef3(&seperator, &commandLineFileName, &seperator);

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
        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
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
 * \param ClientId A variable which receives the identifier of the initial thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcessAsUser(
    _In_ PPH_CREATE_PROCESS_AS_USER_INFO Information,
    _In_ ULONG Flags,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    PVOID defaultEnvironment = NULL;
    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    BOOLEAN needsDuplicate = FALSE;

    if ((Flags & PH_CREATE_PROCESS_USE_PROCESS_TOKEN) && (Flags & PH_CREATE_PROCESS_USE_SESSION_TOKEN))
        return STATUS_INVALID_PARAMETER_2;
    if (!Information->ApplicationName && !Information->CommandLine)
        return STATUS_INVALID_PARAMETER_MIX;

    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_NORMAL;
    startupInfo.lpDesktop = Information->DesktopName;

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
            PPH_STRING commandLine = NULL;
            PROCESS_INFORMATION processInfo = { 0 };
            ULONG newFlags = 0;

            if (Information->CommandLine) // duplicate because CreateProcess modifies the string
                commandLine = PhCreateString(Information->CommandLine);

            if (!Information->Environment)
                Flags |= PH_CREATE_PROCESS_UNICODE_ENVIRONMENT;

            PhMapFlags1(&newFlags, Flags, PhpCreateProcessMappings, sizeof(PhpCreateProcessMappings) / sizeof(PH_FLAG_MAPPING));

            if (CreateProcessWithLogonW(
                Information->UserName,
                Information->DomainName,
                Information->Password,
                LOGON_WITH_PROFILE,
                Information->ApplicationName,
                PhGetString(commandLine),
                newFlags,
                Information->Environment,
                Information->CurrentDirectory,
                &startupInfo,
                &processInfo
                ))
                status = STATUS_SUCCESS;
            else
                status = PhGetLastWin32ErrorAsNtStatus();

            if (commandLine)
                PhDereferenceObject(commandLine);

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

        if (!WinStationQueryInformationW_Import())
            return STATUS_PROCEDURE_NOT_FOUND;

        if (!WinStationQueryInformationW_Import()(
            NULL,
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
        ULONG returnLength;

        // NtQueryInformationToken normally returns an impersonation token with
        // SecurityIdentification, but if the process is running with SeTcbPrivilege, it returns a
        // primary token. We can never duplicate a SecurityIdentification impersonation token to
        // make it a primary token, so we just check if the token is primary before using it.

        if (NT_SUCCESS(PhGetTokenLinkedToken(tokenHandle, &linkedTokenHandle)))
        {
            if (NT_SUCCESS(NtQueryInformationToken(
                linkedTokenHandle,
                TokenType,
                &tokenType,
                sizeof(TOKEN_TYPE),
                &returnLength
                )) && tokenType == TokenPrimary)
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
        if (CreateEnvironmentBlock_Import())
        {
            CreateEnvironmentBlock_Import()(&defaultEnvironment, tokenHandle, FALSE);

            if (defaultEnvironment)
                Flags |= PH_CREATE_PROCESS_UNICODE_ENVIRONMENT;
        }
    }

    status = PhCreateProcessWin32Ex(
        Information->ApplicationName,
        Information->CommandLine,
        Information->Environment ? Information->Environment : defaultEnvironment,
        Information->CurrentDirectory,
        &startupInfo,
        Flags,
        tokenHandle,
        ClientId,
        ProcessHandle,
        ThreadHandle
        );

    if (DestroyEnvironmentBlock_Import() && defaultEnvironment)
        DestroyEnvironmentBlock_Import()(defaultEnvironment);

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
    static LUID_AND_ATTRIBUTES defaultAllowedPrivileges[] =
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
            if (!NT_SUCCESS(RtlGetDaclSecurityDescriptor(
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

            newDacl = PhAllocate(newDaclLength);
            RtlCreateAcl(newDacl, newDaclLength, ACL_REVISION);

            // Add the existing DACL entries.
            if (currentDaclPresent && currentDacl)
            {
                for (i = 0; i < currentDacl->AceCount; i++)
                {
                    if (NT_SUCCESS(RtlGetAce(currentDacl, i, &currentAce)))
                        RtlAddAce(newDacl, ACL_REVISION, ULONG_MAX, currentAce, currentAce->AceSize);
                }
            }

            // Allow access for the current user.
            RtlAddAccessAllowedAce(newDacl, ACL_REVISION, GENERIC_ALL, currentUser.User.Sid);

            // Set the security descriptor of the new token.

            RtlCreateSecurityDescriptor(&newSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

            if (NT_SUCCESS(RtlSetDaclSecurityDescriptor(&newSecurityDescriptor, TRUE, newDacl, FALSE)))
                PhSetObjectSecurity(newTokenHandle, DACL_SECURITY_INFORMATION, &newSecurityDescriptor);

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

PPH_STRING PhGetSecurityDescriptorAsString(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PPH_STRING securityDescriptorString = NULL;
    ULONG stringSecurityDescriptorLength = 0;
    PWSTR stringSecurityDescriptor = NULL;

    if (!ConvertSecurityDescriptorToStringSecurityDescriptorW_Import())
        return NULL;

    if (ConvertSecurityDescriptorToStringSecurityDescriptorW_Import()(
        SecurityDescriptor,
        SDDL_REVISION,
        SecurityInformation,
        &stringSecurityDescriptor,
        &stringSecurityDescriptorLength
        ))
    {
        securityDescriptorString = PhCreateStringEx(
            stringSecurityDescriptor,
            stringSecurityDescriptorLength * sizeof(WCHAR)
            );
        LocalFree(stringSecurityDescriptor);
    }

    return securityDescriptorString;
}

PSECURITY_DESCRIPTOR PhGetSecurityDescriptorFromString(
    _In_ PWSTR SecurityDescriptorString
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
        //assert(securityDescriptorLength == RtlLengthSecurityDescriptor(securityDescriptor));
        securityDescriptor = PhAllocateCopy(
            securityDescriptorBuffer,
            securityDescriptorLength
            );
        LocalFree(securityDescriptorBuffer);
    }

    return securityDescriptor;
}

_Success_(return)
BOOLEAN PhGetObjectSecurityDescriptorAsString(
    _In_ HANDLE Handle,
    _Out_ PPH_STRING* SecurityDescriptorString
    )
{
    PSECURITY_DESCRIPTOR securityDescriptor;
    PPH_STRING securityDescriptorString;

    if (NT_SUCCESS(PhGetObjectSecurity(
        Handle,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION |
        ATTRIBUTE_SECURITY_INFORMATION | SCOPE_SECURITY_INFORMATION,
        &securityDescriptor
        )))
    {
        if (securityDescriptorString = PhGetSecurityDescriptorAsString(
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION |
            ATTRIBUTE_SECURITY_INFORMATION | SCOPE_SECURITY_INFORMATION,
            securityDescriptor
            ))
        {
            *SecurityDescriptorString = securityDescriptorString;

            PhFree(securityDescriptor);
            return TRUE;
        }

        PhFree(securityDescriptor);
    }

    return FALSE;
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
    static BOOL (WINAPI *ShellExecuteExW_I)(
        _Inout_ SHELLEXECUTEINFOW *pExecInfo
        ) = NULL;

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
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters
    )
{
    SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };

    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
    info.nShow = SW_SHOW;
    info.hwnd = WindowHandle;

    if (!PhShellExecuteWin32(&info))
    {
        PhShowStatus(WindowHandle, L"Unable to execute the program.", 0, GetLastError());
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
_Success_(return)
BOOLEAN PhShellExecuteEx(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_opt_ PWSTR Directory,
    _In_ INT32 ShowWindowType,
    _In_ ULONG Flags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };

    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.lpDirectory = Directory;
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_NOZONECHECKS;
    info.nShow = ShowWindowType;
    info.hwnd = WindowHandle;

    if (Flags & PH_SHELL_EXECUTE_ADMIN)
        info.lpVerb = L"runas";

    if (PhShellExecuteWin32(&info))
    {
        if (Timeout)
        {
            if (!(Flags & PH_SHELL_EXECUTE_PUMP_MESSAGES))
            {
                if (info.hProcess)
                {
                    PhWaitForSingleObject(info.hProcess, PhTimeoutFromMillisecondsEx(Timeout));
                }
            }
            else
            {
                PhWaitForMultipleObjectsAndPump(NULL, 1, &info.hProcess, Timeout, QS_ALLEVENTS);
            }
        }

        if (ProcessHandle)
            *ProcessHandle = info.hProcess;
        else if (info.hProcess)
            NtClose(info.hProcess);

        return TRUE;
    }
    else
    {
        return FALSE;
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
    _In_ PWSTR FileName
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* SHOpenFolderAndSelectItems_I)(
        _In_ PCIDLIST_ABSOLUTE pidlFolder,
        _In_ UINT cidl,
        _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY* apidl,
        _In_ DWORD dwFlags
        ) = NULL;
    static HRESULT (WINAPI* SHParseDisplayName_I)(
        _In_ PCWSTR pszName,
        _In_opt_ IBindCtx* pbc,
        _Outptr_ PIDLIST_ABSOLUTE* ppidl,
        _In_ SFGAOF sfgaoIn,
        _Out_opt_ SFGAOF* psfgaoOut
        ) = NULL;

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
    _In_ PWSTR FileName
    )
{
    SHELLEXECUTEINFO info = { sizeof(info) };

    info.lpFile = FileName;
    info.nShow = SW_SHOW;
    info.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"properties";
    info.hwnd = WindowHandle;

    if (!PhShellExecuteWin32(&info))
    {
        PhShowStatus(WindowHandle, L"Unable to execute the program.", 0, GetLastError());
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
    static BOOL (WINAPI *Shell_NotifyIconW_I)(
        _In_ ULONG dwMessage,
        _In_ PNOTIFYICONDATAW Data
        ) = NULL;

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
    static HRESULT (WINAPI *SHGetKnownFolderPath_I)(
        _In_ REFKNOWNFOLDERID rfid,
        _In_ ULONG dwFlags, // KNOWN_FOLDER_FLAG
        _In_opt_ HANDLE hToken,
        _Outptr_ PWSTR* ppszPath // CoTaskMemFree
        ) = NULL;

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
    static HRESULT (WINAPI *SHGetKnownFolderItem_I)(
        _In_ REFKNOWNFOLDERID rfid,
        _In_ ULONG flags, // KNOWN_FOLDER_FLAG
        _In_opt_ HANDLE hToken,
        _In_ REFIID riid,
        _Outptr_ void** ppv
        ) = NULL;

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
 *
 * \return A pointer to a string containing the value, or NULL if the function failed. You must free
 * the string using PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
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
            if (buffer->DataLength >= sizeof(UNICODE_NULL))
                string = PhCreateStringEx((PWCHAR)buffer->Data, buffer->DataLength - sizeof(UNICODE_NULL));
            else
                string = PhReferenceEmptyString();
        }

        PhFree(buffer);
    }

    return string;
}

ULONG PhQueryRegistryUlong(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
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
    _In_opt_ PPH_STRINGREF ValueName
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
    _In_ PWSTR FileName
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* SHParseDisplayName_I)(
        _In_ PCWSTR pszName,
        _In_opt_ IBindCtx* pbc,
        _Out_ PIDLIST_ABSOLUTE* ppidl,
        _In_ SFGAOF sfgaoIn,
        _Out_opt_ SFGAOF* psfgaoOut
        ) = NULL;
    static HRESULT (WINAPI* SHCreateItemFromIDList_I)(
        _In_ PCIDLIST_ABSOLUTE pidl,
        _In_ REFIID riid,
        _Outptr_ void** ppv
        ) = NULL;
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

        ofn->nMaxFile = (ULONG)max(fileName.Length / sizeof(WCHAR) + 1, 0x400);
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

    // This rule is currently disabled.
    hasTextSection = TRUE;

    //limitNumberOfSections = min(mappedImage.NumberOfSections, 64);

    //for (i = 0; i < limitNumberOfSections; i++)
    //{
    //    CHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

    //    if (PhGetMappedImageSectionName(
    //        &mappedImage.Sections[i],
    //        sectionName,
    //        IMAGE_SIZEOF_SHORT_NAME + 1,
    //        NULL
    //        ))
    //    {
    //        if (STR_IEQUAL(sectionName, ".text"))
    //        {
    //            hasTextSection = TRUE;
    //            break;
    //        }
    //    }
    //}

    status = PhGetMappedImageImports(
        &imports,
        &mappedImage
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Get the module and function totals.

    numberOfModules = imports.NumberOfDlls;
    limitNumberOfModules = min(numberOfModules, 64);

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
 *
 * \return Crc
 */
ULONG PhCrc32(
    _In_ ULONG Crc,
    _In_reads_(Length) PCHAR Buffer,
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
 *
 * \return Crc
 */
ULONG PhCrc32C(
    _In_ ULONG Crc,
    _In_reads_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    )
{
#ifndef _ARM64_
#ifdef _M_X64
    SIZE_T u64_blocks = Length / sizeof(ULONG64);
#endif
    SIZE_T u64_remaining = Length % sizeof(ULONG64);
    SIZE_T u32_blocks = u64_remaining / sizeof(ULONG);
    SIZE_T u32_remaining = u64_remaining % sizeof(ULONG);
    SIZE_T u16_blocks = u32_remaining / sizeof(USHORT);
    SIZE_T u8_blocks = u32_remaining % sizeof(UCHAR);

    Crc ^= 0xffffffff;

    while (u8_blocks--)
    {
        Crc = _mm_crc32_u8(Crc, (*(PUCHAR)Buffer)++);
    }

    while (u16_blocks--)
    {
        Crc = _mm_crc32_u16(Crc, (*(PUSHORT)Buffer)++);
    }

    while (u32_blocks--)
    {
        Crc = _mm_crc32_u32(Crc, (*(PULONG)Buffer)++);
    }

#ifdef _M_X64
    while (u64_blocks--)
    {
        Crc = (ULONG)_mm_crc32_u64(Crc, (*(PULONG64)Buffer)++);
    }
#endif

    return Crc ^ 0xffffffff;
#else
    PhRaiseStatus(STATUS_NOT_SUPPORTED);
#endif
}

C_ASSERT(RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(MD5_CTX));
C_ASSERT(RTL_FIELD_SIZE(PH_HASH_CONTEXT, Context) >= sizeof(A_SHA_CTX));

/**
 * Initializes hashing.
 *
 * \param Context A hashing context structure.
 * \param Algorithm The hash algorithm to use:
 * \li \c Md5HashAlgorithm MD5 (128 bits)
 * \li \c Sha1HashAlgorithm SHA-1 (160 bits)
 * \li \c Crc32HashAlgorithm CRC-32-IEEE 802.3 (32 bits)
 * \li \c Sha256HashAlgorithm SHA-256 (256 bits)
 */
VOID PhInitializeHash(
    _Out_ PPH_HASH_CONTEXT Context,
    _In_ PH_HASH_ALGORITHM Algorithm
    )
{
    Context->Algorithm = Algorithm;

    switch (Algorithm)
    {
    case Md5HashAlgorithm:
        MD5Init((MD5_CTX *)Context->Context);
        break;
    case Sha1HashAlgorithm:
        A_SHAInit((A_SHA_CTX *)Context->Context);
        break;
    case Crc32HashAlgorithm:
        Context->Context[0] = 0;
        break;
    case Sha256HashAlgorithm:
        sha256_starts((sha256_context *)Context->Context);
        break;
    default:
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);
        break;
    }
}

/**
 * Hashes a block of data.
 *
 * \param Context A hashing context structure.
 * \param Buffer The block of data.
 * \param Length The number of bytes in the block.
 */
VOID PhUpdateHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    switch (Context->Algorithm)
    {
    case Md5HashAlgorithm:
        MD5Update((MD5_CTX *)Context->Context, (PUCHAR)Buffer, Length);
        break;
    case Sha1HashAlgorithm:
        A_SHAUpdate((A_SHA_CTX *)Context->Context, (PUCHAR)Buffer, Length);
        break;
    case Crc32HashAlgorithm:
        Context->Context[0] = PhCrc32(Context->Context[0], (PUCHAR)Buffer, Length);
        break;
    case Sha256HashAlgorithm:
        sha256_update((sha256_context *)Context->Context, (PUCHAR)Buffer, Length);
        break;
    default:
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
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
BOOLEAN PhFinalHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _Out_writes_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    BOOLEAN result;
    ULONG returnLength;

    result = FALSE;

    switch (Context->Algorithm)
    {
    case Md5HashAlgorithm:
        if (HashLength >= 16)
        {
            MD5Final((MD5_CTX *)Context->Context);
            memcpy(Hash, ((MD5_CTX *)Context->Context)->digest, 16);
            result = TRUE;
        }

        returnLength = 16;

        break;
    case Sha1HashAlgorithm:
        if (HashLength >= 20)
        {
            A_SHAFinal((A_SHA_CTX *)Context->Context, (PUCHAR)Hash);
            result = TRUE;
        }

        returnLength = 20;

        break;
    case Crc32HashAlgorithm:
        if (HashLength >= 4)
        {
            *(PULONG)Hash = Context->Context[0];
            result = TRUE;
        }

        returnLength = 4;

        break;
    case Sha256HashAlgorithm:
        if (HashLength >= 32)
        {
            sha256_finish((sha256_context *)Context->Context, (PUCHAR)Hash);
            result = TRUE;
        }

        returnLength = 32;

        break;
    default:
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    if (ReturnLength)
        *ReturnLength = returnLength;

    return result;
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
    _In_ PPH_STRINGREF CommandLine,
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
    _In_ PPH_STRINGREF CommandLine,
    _In_opt_ PPH_COMMAND_LINE_OPTION Options,
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
    PPH_COMMAND_LINE_OPTION option = NULL;
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
 *
 * \return The escaped string.
 *
 * \remarks Only the double quotation mark is escaped.
 */
PPH_STRING PhEscapeCommandLinePart(
    _In_ PPH_STRINGREF String
    )
{
    static PH_STRINGREF backslashAndQuote = PH_STRINGREF_INIT(L"\\\"");

    PH_STRING_BUILDER stringBuilder;
    ULONG length;
    ULONG i;

    ULONG numberOfBackslashes;

    length = (ULONG)String->Length / sizeof(WCHAR);
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
 * \param Arguments A variable which receives the part of \a CommandLine that contains the
 * arguments.
 * \param FullFileName A variable which receives the full path and file name. This may be NULL if
 * the file was not found.
 */
BOOLEAN PhParseCommandLineFuzzy(
    _In_ PPH_STRINGREF CommandLine,
    _Out_ PPH_STRINGREF FileName,
    _Out_ PPH_STRINGREF Arguments,
    _Out_opt_ PPH_STRING *FullFileName
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
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
 *
 * \return A pointer to an array of LPWSTR values, similar to argv. If the function fails, the return value is NULL.
 */
_Success_(return != NULL)
PWSTR* PhCommandLineToArgv(
    _In_ PCWSTR CommandLine,
    _Out_ PINT NumberOfArguments
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PWSTR* (WINAPI *CommandLineToArgvW_I)(
        _In_ PCWSTR CmdLine,
        _Out_ PINT NumArgs
        ) = NULL;

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
    INT32 commandLineCount;
    PWSTR* commandLineArray;

    if (commandLineArray = PhCommandLineToArgv(CommandLine, &commandLineCount))
    {
        commandLineList = PhCreateList(commandLineCount);

        for (INT32 i = 0; i < commandLineCount; i++)
            PhAddItemList(commandLineList, PhCreateString(commandLineArray[i]));

        LocalFree(commandLineArray);
    }

    return commandLineList;
}

PPH_STRING PhSearchFilePath(
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Extension
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
//            &PhNtPathSeperatorString,
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
    _In_ PPH_STRING FileName
    )
{
    static PH_STRINGREF settingsSuffix = PH_STRINGREF_INIT(L".settings.xml");
    static PH_STRINGREF settingsDirectory = PH_STRINGREF_INIT(L"cache");
    PPH_STRING fileName;
    PPH_STRING settingsFileName;
    PPH_STRING cacheDirectory;
    PPH_STRING cacheFilePath;
    PH_STRINGREF randomAlphaStringRef;
    WCHAR randomAlphaString[32] = L"";

    fileName = PhGetApplicationFileName();
    settingsFileName = PhConcatStringRef2(&fileName->sr, &settingsSuffix);

    if (PhDoesFileExist(&settingsFileName->sr))
    {
        HANDLE fileHandle;
        PPH_STRING applicationDirectory;
        PPH_STRING applicationFileName;

        PhGenerateRandomAlphaString(randomAlphaString, RTL_NUMBER_OF(randomAlphaString));
        randomAlphaStringRef.Buffer = randomAlphaString;
        randomAlphaStringRef.Length = sizeof(randomAlphaString) - sizeof(UNICODE_NULL);

        applicationDirectory = PhGetApplicationDirectoryWin32();
        applicationFileName = PhConcatStringRef3(
            &applicationDirectory->sr,
            &PhNtPathSeperatorString,
            &randomAlphaStringRef
            );

        if (NT_SUCCESS(PhCreateFileWin32(
            &fileHandle,
            PhGetString(applicationFileName),
            FILE_GENERIC_WRITE | DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE
            )))
        {
            cacheDirectory = PhConcatStringRef2(&applicationDirectory->sr, &settingsDirectory);
            NtClose(fileHandle);
        }
        else
        {
            cacheDirectory = PhGetRoamingAppDataDirectory(&settingsDirectory, FALSE);
        }

        PhDereferenceObject(applicationFileName);
        PhDereferenceObject(applicationDirectory);
    }
    else
    {
        cacheDirectory = PhGetRoamingAppDataDirectory(&settingsDirectory, FALSE);

        if (PhIsNullOrEmptyString(cacheDirectory))
        {
            PhDereferenceObject(settingsFileName);
            PhDereferenceObject(fileName);
            return FileName;
        }
    }

    PhGenerateRandomAlphaString(randomAlphaString, RTL_NUMBER_OF(randomAlphaString));
    randomAlphaStringRef.Buffer = randomAlphaString;
    randomAlphaStringRef.Length = sizeof(randomAlphaString) - sizeof(UNICODE_NULL);

    cacheFilePath = PhConcatStringRef3(
        &cacheDirectory->sr,
        &PhNtPathSeperatorString,
        &randomAlphaStringRef
        );

    PhMoveReference(&cacheFilePath, PhConcatStringRef3(
        &cacheFilePath->sr,
        &PhNtPathSeperatorString,
        &FileName->sr
        ));

    if (!NT_SUCCESS(PhCreateDirectoryFullPathWin32(&cacheFilePath->sr)))
    {
        PhClearReference(&cacheFilePath);
    }

    PhDereferenceObject(cacheDirectory);
    PhDereferenceObject(settingsFileName);
    PhDereferenceObject(fileName);

    return cacheFilePath;
}
//
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
    _In_ PPH_STRING FileName
    )
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

VOID PhClearCacheDirectory(
    VOID
    )
{
    static PH_STRINGREF settingsSuffix = PH_STRINGREF_INIT(L".settings.xml");
    static PH_STRINGREF settingsDirectory = PH_STRINGREF_INIT(L"cache");
    PPH_STRING fileName;
    PPH_STRING settingsFileName;
    PPH_STRING cacheDirectory;

    fileName = PhGetApplicationFileName();
    settingsFileName = PhConcatStringRef2(&fileName->sr, &settingsSuffix);

    if (PhDoesFileExist(&settingsFileName->sr))
    {
        HANDLE fileHandle;
        PPH_STRING applicationDirectory;
        PPH_STRING applicationFileName;
        PH_STRINGREF randomAlphaStringRef;
        WCHAR randomAlphaString[32] = L"";

        applicationDirectory = PhGetApplicationDirectoryWin32();

        PhGenerateRandomAlphaString(randomAlphaString, RTL_NUMBER_OF(randomAlphaString));
        randomAlphaStringRef.Buffer = randomAlphaString;
        randomAlphaStringRef.Length = sizeof(randomAlphaString) - sizeof(UNICODE_NULL);

        applicationFileName = PhConcatStringRef3(
            &applicationDirectory->sr,
            &PhNtPathSeperatorString,
            &randomAlphaStringRef
            );

        if (NT_SUCCESS(PhCreateFileWin32(
            &fileHandle,
            PhGetString(applicationFileName),
            FILE_GENERIC_WRITE | DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE
            )))
        {
            cacheDirectory = PhConcatStringRef2(&applicationDirectory->sr, &settingsDirectory);
            NtClose(fileHandle);
        }
        else
        {
            cacheDirectory = PhGetRoamingAppDataDirectory(&settingsDirectory, FALSE);
        }

        PhDereferenceObject(applicationFileName);
        PhDereferenceObject(applicationDirectory);
    }
    else
    {
        cacheDirectory = PhGetRoamingAppDataDirectory(&settingsDirectory, FALSE);
    }

    if (cacheDirectory)
    {
        PhDeleteDirectoryWin32(&cacheDirectory->sr);

        PhDereferenceObject(cacheDirectory);
    }

    PhDereferenceObject(settingsFileName);
    PhDereferenceObject(fileName);
}

HANDLE PhGetNamespaceHandle(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static UNICODE_STRING namespacePathUs = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\SystemInformer");
    static HANDLE directory = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + 0x80];
        PSID administratorsSid = PhSeAdministratorsSid();
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
        dacl = (PACL)PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

        RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        RtlCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
        RtlAddAccessAllowedAce(dacl, ACL_REVISION, DIRECTORY_ALL_ACCESS, &PhSeLocalSid);
        RtlAddAccessAllowedAce(dacl, ACL_REVISION, DIRECTORY_ALL_ACCESS, administratorsSid);
        RtlAddAccessAllowedAce(dacl, ACL_REVISION, DIRECTORY_QUERY | DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT, &PhSeInteractiveSid);
        RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

        InitializeObjectAttributes(
            &objectAttributes,
            &namespacePathUs,
            OBJ_OPENIF | (WindowsVersion < WINDOWS_10 ? 0 : OBJ_DONT_REPARSE),
            NULL,
            securityDescriptor
            );

        NtCreateDirectoryObject(
            &directory,
            MAXIMUM_ALLOWED,
            &objectAttributes
            );

#ifdef DEBUG
        assert(sdAllocationLength < sizeof(securityDescriptorBuffer));
        assert(RtlLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));
#endif
        PhEndInitOnce(&initOnce);
    }

    return directory;
}

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
    PSTR data = NULL;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    IO_STATUS_BLOCK isb;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = PhAllocate(allocatedLength);
    dataLength = 0;

    while (NT_SUCCESS(NtReadFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        buffer,
        PAGE_SIZE,
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

    if (allocatedLength < dataLength + sizeof(ANSI_NULL))
    {
        allocatedLength++;
        data = PhReAllocate(data, allocatedLength);
    }

    if (dataLength > 0)
    {
        data[dataLength] = ANSI_NULL;

        *Buffer = data;
        *BufferLength = dataLength;

        return STATUS_SUCCESS;
    }
    else
    {
        *Buffer = NULL;
        *BufferLength = 0;

        PhFree(data);

        return STATUS_UNSUCCESSFUL;
    }
}

PVOID PhGetFileText(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Unicode
    )
{
    PVOID string = NULL;
    ULONG dataLength;
    PSTR data;

    if (NT_SUCCESS(PhGetFileData(FileHandle, &data, &dataLength)))
    {
        if (Unicode)
            string = PhConvertUtf8ToUtf16Ex(data, dataLength);
        else
            string = PhCreateBytesEx(data, dataLength);

        PhFree(data);
    }

    return string;
}

PVOID PhFileReadAllText(
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN Unicode
    )
{
    PVOID string = NULL;
    HANDLE fileHandle;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        string = PhGetFileText(fileHandle, Unicode);
        NtClose(fileHandle);
    }

    return string;
}

PVOID PhFileReadAllTextWin32(
    _In_ PWSTR FileName,
    _In_ BOOLEAN Unicode
    )
{
    PVOID string = NULL;
    HANDLE fileHandle;

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        string = PhGetFileText(fileHandle, Unicode);
        NtClose(fileHandle);
    }

    return string;
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
 *
 * \return Successful or errant status.
 */
HRESULT PhGetClassObject(
    _In_ PCWSTR DllName,
    _In_ REFCLSID Rclsid,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
#if (PH_NATIVE_COM_CLASS_FACTORY)
    HRESULT status;
    IClassFactory* classFactory;

    status = CoGetClassObject(
        Rclsid,
        CLSCTX_INPROC_SERVER,
        NULL,
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
#else
    PVOID baseAddress;

    if (!(baseAddress = PhGetLoaderEntryDllBaseZ((PWSTR)DllName)))
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
 *
 * \return Successful or errant status.
 */
HRESULT PhGetActivationFactory(
    _In_ PCWSTR DllName,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
#if (PH_NATIVE_COM_CLASS_FACTORY || PH_BUILD_MSIX)
    #include <roapi.h>
    #include <winstring.h>
    HRESULT status;
    HSTRING runtimeClassStringHandle = NULL;
    HSTRING_HEADER runtimeClassStringHeader;

    status = WindowsCreateStringReference(
        RuntimeClass,
        (UINT32)PhCountStringZ((PWSTR)RuntimeClass),
        &runtimeClassStringHeader,
        &runtimeClassStringHandle
        );

    if (SUCCEEDED(status))
    {
        status = RoGetActivationFactory(
            runtimeClassStringHandle,
            Riid,
            Ppv
            );
    }

    return status;
#else
    PVOID baseAddress;

    if (!(baseAddress = PhGetLoaderEntryDllBaseZ((PWSTR)DllName)))
    {
        if (!(baseAddress = PhLoadLibrary(DllName)))
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    return PhGetActivationFactoryDllBase(baseAddress, RuntimeClass, Riid, Ppv);
#endif
}

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
 *
 * \return Successful or errant status.
 */
HRESULT PhActivateInstance(
    _In_ PCWSTR DllName,
    _In_ PCWSTR RuntimeClass,
    _In_ REFIID Riid,
    _Out_ PVOID* Ppv
    )
{
#if (PH_NATIVE_COM_CLASS_FACTORY || PH_BUILD_MSIX)
    #include <roapi.h>
    #include <winstring.h>
    HRESULT status;
    HSTRING runtimeClassStringHandle = NULL;
    HSTRING_HEADER runtimeClassStringHeader;
    IInspectable* inspectableObject;

    status = WindowsCreateStringReference(
        RuntimeClass,
        (UINT32)PhCountStringZ((PWSTR)RuntimeClass),
        &runtimeClassStringHeader,
        &runtimeClassStringHandle
        );

    if (SUCCEEDED(status))
    {
        status = RoActivateInstance(
            runtimeClassStringHandle,
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
    }

    return status;
#else
    PVOID baseAddress;

    if (!(baseAddress = PhGetLoaderEntryDllBaseZ((PWSTR)DllName)))
    {
        if (!(baseAddress = PhLoadLibrary(DllName)))
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    return PhActivateInstanceDllBase(baseAddress, RuntimeClass, Riid, Ppv);
#endif
}

#if (PH_NATIVE_RING_BUFFER)
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
        NtUnmapViewOfSection(NtCurrentProcess(), sectionView1);
    if (sectionView2)
        NtUnmapViewOfSection(NtCurrentProcess(), sectionView2);

    return status;
}
#endif

NTSTATUS PhDelayExecutionEx(
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER DelayInterval
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static NTSTATUS (NTAPI* RtlDelayExecution_I)(
        _In_ BOOLEAN Alertable,
        _In_opt_ PLARGE_INTEGER DelayInterval
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID ntdll;

        if (ntdll = PhGetLoaderEntryDllBaseZ(L"ntdll.dll"))
        {
            RtlDelayExecution_I = PhGetDllBaseProcedureAddress(ntdll, "RtlDelayExecution", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (RtlDelayExecution_I)
    {
        return RtlDelayExecution_I(Alertable, DelayInterval);
    }

    return NtDelayExecution(Alertable, DelayInterval);
}

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

ULONGLONG PhReadTimeStampCounter(
    VOID
    )
{
    return ReadTimeStampCounter();
}

// rev from QueryPerformanceCounter (dmex)
BOOLEAN PhQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter
    )
{
    if (RtlQueryPerformanceCounter(PerformanceCounter))
        return TRUE;

    return NT_SUCCESS(NtQueryPerformanceCounter(PerformanceCounter, NULL));
}

// rev from QueryPerformanceFrequency (dmex)
BOOLEAN PhQueryPerformanceFrequency(
    _Out_ PLARGE_INTEGER PerformanceFrequency
    )
{
    LARGE_INTEGER performanceCounter;

    if (RtlQueryPerformanceFrequency(PerformanceFrequency))
        return TRUE;

    return NT_SUCCESS(NtQueryPerformanceCounter(&performanceCounter, PerformanceFrequency));
}

PPH_STRING PhApiSetResolveToHost(
    _In_ PPH_STRINGREF ApiSetName
    )
{
    PAPI_SET_NAMESPACE apisetMap;
    PAPI_SET_NAMESPACE_ENTRY apisetMapEntry;

    if (ApiSetName->Length >= 8)
    {
        ULONGLONG apisetNameBufferPrefix = ((PULONGLONG)ApiSetName->Buffer)[0] & 0xFFFFFFDFFFDFFFDF;

        if (apisetNameBufferPrefix != 0x2D004900500041ULL && // L"api-"
            apisetNameBufferPrefix != 0x2D005400580045ULL) // L"ext-"
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }

    apisetMap = NtCurrentPeb()->ApiSetMap;
    apisetMapEntry = PTR_ADD_OFFSET(apisetMap->EntryOffset, apisetMap);

    if (apisetMap->Version != 6)
        return NULL;

    for (ULONG i = 0; i < apisetMap->Count; i++)
    {
        PAPI_SET_VALUE_ENTRY apisetMapValue = PTR_ADD_OFFSET(apisetMap, apisetMapEntry->ValueOffset);
        PH_STRINGREF nameStringRef;

        nameStringRef.Length = apisetMapEntry->HashedLength; // NameLength
        nameStringRef.Buffer = PTR_ADD_OFFSET(apisetMap, apisetMapEntry->NameOffset);

        if (PhStartsWithStringRef(ApiSetName, &nameStringRef, TRUE))
        {
            for (ULONG ii = 0; ii < apisetMapEntry->ValueCount; ii++)
            {
                if (apisetMapValue->ValueLength)
                {
                    PH_STRINGREF valueStringRef;

                    valueStringRef.Length = apisetMapValue->ValueLength;
                    valueStringRef.Buffer = PTR_ADD_OFFSET(apisetMap, apisetMapValue->ValueOffset);

                    return PhCreateString2(&valueStringRef);
                }

                apisetMapValue++;
            }
        }

        apisetMapEntry++;
    }

    return NULL;
}

HRESULT PhCreateProcessAsInteractiveUser(
    _In_ PWSTR CommandLine,
    _In_ PWSTR CurrentDirectory
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* WdcRunTaskAsInteractiveUser_I)(
        _In_ PWSTR CommandLine,
        _In_ PWSTR CurrentDirectory,
        _In_opt_ ULONG Flags
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (!(baseAddress = PhGetLoaderEntryDllBaseZ(L"wdc.dll")))
            baseAddress = PhLoadLibrary(L"wdc.dll");

        if (baseAddress)
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

NTSTATUS PhCreateProcessClone(
    _Out_ PHANDLE ProcessHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE cloneProcessHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_PROCESS,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtCreateProcessEx(
        &cloneProcessHandle,
        PROCESS_ALL_ACCESS,
        NULL,
        processHandle,
        PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT | PROCESS_CREATE_FLAGS_INHERIT_HANDLES | PROCESS_CREATE_FLAGS_SUSPENDED,
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

NTSTATUS PhCreateProcessReflection(
    _Out_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ProcessId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE processHandle = ProcessHandle;
    PROCESS_REFLECTION_INFORMATION reflectionInfo = { 0 };

    if (!ProcessHandle)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE,
            ProcessId
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    status = RtlCreateProcessReflection(
        processHandle,
        RTL_PROCESS_REFLECTION_FLAGS_INHERIT_HANDLES,
        NULL,
        NULL,
        NULL,
        &reflectionInfo
        );

    if (!ProcessHandle && processHandle)
        NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *ReflectionInformation = reflectionInfo;
    }

    return status;
}

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

NTSTATUS PhCreateProcessSnapshot(
    _Out_ PHANDLE SnapshotHandle,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ProcessId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE processHandle = ProcessHandle;
    HANDLE snapshotHandle = NULL;

    if (!PssCaptureSnapshot_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    if (!ProcessHandle)
    {
        status = PhOpenProcess(
            &processHandle,
            MAXIMUM_ALLOWED,
            ProcessId
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    status = PssCaptureSnapshot_Import()(
        processHandle,
        PSS_CAPTURE_VA_CLONE | PSS_CAPTURE_HANDLES | PSS_CAPTURE_HANDLE_NAME_INFORMATION |
        PSS_CAPTURE_HANDLE_BASIC_INFORMATION | PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION |
        PSS_CAPTURE_HANDLE_TRACE | PSS_CAPTURE_THREADS | PSS_CAPTURE_THREAD_CONTEXT |
        PSS_CAPTURE_THREAD_CONTEXT_EXTENDED | PSS_CAPTURE_VA_SPACE | PSS_CAPTURE_VA_SPACE_SECTION_INFORMATION |
        PSS_CREATE_BREAKAWAY | PSS_CREATE_BREAKAWAY_OPTIONAL | PSS_CREATE_USE_VM_ALLOCATIONS,
        CONTEXT_ALL, // WOW64_CONTEXT_ALL?
        &snapshotHandle
        );
    status = PhDosErrorToNtStatus(status);

    if (!ProcessHandle && processHandle)
        NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *SnapshotHandle = snapshotHandle;
    }

    return status;
}

VOID PhFreeProcessSnapshot(
    _In_ HANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle
    )
{
    if (PssQuerySnapshot_Import())
    {
        PSS_VA_CLONE_INFORMATION processInfo = { 0 };
        PSS_HANDLE_TRACE_INFORMATION handleInfo = { 0 };

        if (PssQuerySnapshot_Import()(
            SnapshotHandle,
            PSS_QUERY_VA_CLONE_INFORMATION,
            &processInfo,
            sizeof(PSS_VA_CLONE_INFORMATION)
            ) == ERROR_SUCCESS)
        {
            if (processInfo.VaCloneHandle)
            {
                NtClose(processInfo.VaCloneHandle);
            }
        }

        if (PssQuerySnapshot_Import()(
            SnapshotHandle,
            PSS_QUERY_HANDLE_TRACE_INFORMATION,
            &handleInfo,
            sizeof(PSS_HANDLE_TRACE_INFORMATION)
            ) == ERROR_SUCCESS)
        {
            if (handleInfo.SectionHandle)
            {
                NtClose(handleInfo.SectionHandle);
            }
        }
    }

    if (PssFreeSnapshot_Import())
    {
        PssFreeSnapshot_Import()(ProcessHandle, SnapshotHandle);
    }
}

NTSTATUS PhCreateProcessRedirection(
    _In_ PPH_STRING CommandLine,
    _In_opt_ PPH_STRINGREF CommandInput,
    _Out_opt_ PPH_STRING *CommandOutput
    )
{
    NTSTATUS status;
    PPH_STRING output = NULL;
    STARTUPINFOEX startupInfo = { 0 };
    HANDLE processHandle = NULL;
    HANDLE outputReadHandle = NULL;
    HANDLE outputWriteHandle = NULL;
    HANDLE inputReadHandle = NULL;
    HANDLE inputWriteHandle = NULL;
    PROCESS_BASIC_INFORMATION basicInfo;

    status = PhCreatePipeEx(
        &inputReadHandle,
        &inputWriteHandle,
        TRUE,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreatePipeEx(
        &outputReadHandle,
        &outputWriteHandle,
        TRUE,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
    startupInfo.StartupInfo.wShowWindow = SW_HIDE;
    startupInfo.StartupInfo.hStdInput = inputReadHandle;
    startupInfo.StartupInfo.hStdOutput = outputWriteHandle;
    startupInfo.StartupInfo.hStdError = outputWriteHandle;

#if !defined(PH_BUILD_MSIX)
    status = PhInitializeProcThreadAttributeList(&startupInfo.lpAttributeList, 1);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhUpdateProcThreadAttribute(
        startupInfo.lpAttributeList,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        &(HANDLE[2]){ inputReadHandle, outputWriteHandle },
        sizeof(HANDLE[2])
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;
#else
    status = PhInitializeProcThreadAttributeList(&startupInfo.lpAttributeList, 2);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhUpdateProcThreadAttribute(
        startupInfo.lpAttributeList,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        &(HANDLE[2]){ inputReadHandle, outputWriteHandle },
        sizeof(HANDLE[2])
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhUpdateProcThreadAttribute(
        startupInfo.lpAttributeList,
        PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY,
        &(ULONG){ PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE },
        sizeof(ULONG)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;
#endif

    status = PhCreateProcessWin32Ex(
        NULL,
        PhGetString(CommandLine),
        NULL,
        NULL,
        &startupInfo.StartupInfo,
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
        IO_STATUS_BLOCK isb;

        NtWriteFile(
            inputWriteHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            CommandInput->Buffer,
            (ULONG)CommandInput->Length,
            NULL,
            NULL
            );
    }

    output = PhGetFileText(outputReadHandle, TRUE);

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
    if (startupInfo.lpAttributeList)
        PhDeleteProcThreadAttributeList(startupInfo.lpAttributeList);

    if (CommandOutput)
        *CommandOutput = output;
    else
        PhClearReference(&output);

    return status;
}

// rev from InitializeProcThreadAttributeList (dmex)
NTSTATUS PhInitializeProcThreadAttributeList(
    _Out_ PPROC_THREAD_ATTRIBUTE_LIST* AttributeList,
    _In_ ULONG AttributeCount
    )
{
#if (PHNT_NATIVE_PROCATTRIBUTELIST)
    PPROC_THREAD_ATTRIBUTE_LIST attributeList;
    SIZE_T attributeListLength;

    if (!InitializeProcThreadAttributeList(NULL, AttributeCount, 0, &attributeListLength))
        return STATUS_NO_MEMORY;

    attributeList = PhAllocateZero(attributeListLength);
    attributeList->AttributeCount = AttributeCount;
    *AttributeList = attributeList;

    return STATUS_SUCCESS;
#else
    PPROC_THREAD_ATTRIBUTE_LIST attributeList;
    SIZE_T attributeListLength;

    attributeListLength = FIELD_OFFSET(PROC_THREAD_ATTRIBUTE_LIST, Attributes[AttributeCount]);
    attributeList = PhAllocateZero(attributeListLength);
    attributeList->AttributeCount = AttributeCount;
    *AttributeList = attributeList;

    return STATUS_SUCCESS;
#endif
}

// rev from DeleteProcThreadAttributeList (dmex)
NTSTATUS PhDeleteProcThreadAttributeList(
    _In_ PPROC_THREAD_ATTRIBUTE_LIST AttributeList
    )
{
    PhFree(AttributeList);
    return STATUS_SUCCESS;
}

// rev from UpdateProcThreadAttribute (dmex)
NTSTATUS PhUpdateProcThreadAttribute(
    _In_ PPROC_THREAD_ATTRIBUTE_LIST AttributeList,
    _In_ ULONG_PTR AttributeNumber,
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength
    )
{
#if (PHNT_NATIVE_PROCATTRIBUTELIST)
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

/*
 * Process Hacker - 
 *   general support functions
 * 
 * Copyright (C) 2009-2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#define SUPPORT_PRIVATE
#include <phgui.h>
#include <md5.h>

typedef BOOL (WINAPI *_CreateEnvironmentBlock)(
    __out LPVOID *lpEnvironment,
    __in_opt HANDLE hToken,
    __in BOOL bInherit
    );

typedef BOOL (WINAPI *_DestroyEnvironmentBlock)(
    __in LPVOID lpEnvironment
    );

WCHAR *PhSizeUnitNames[7] = { L"B", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB" };
ULONG PhMaxSizeUnit = MAXULONG32;

/**
 * Ensures a rectangle is positioned within the 
 * specified bounds.
 *
 * \param Rectangle The rectangle to be adjusted.
 * \param Bounds The bounds.
 *
 * \remarks If the rectangle is too large to fit 
 * inside the bounds, it is positioned at the 
 * top-left of the bounds.
 */
VOID PhAdjustRectangleToBounds(
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
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
}

/**
 * Positions a rectangle in the center of the 
 * specified bounds.
 *
 * \param Rectangle The rectangle to be adjusted.
 * \param Bounds The bounds.
 */
VOID PhCenterRectangle(
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
    )
{
    Rectangle->Left = Bounds->Left + (Bounds->Width - Rectangle->Width) / 2;
    Rectangle->Top = Bounds->Top + (Bounds->Height - Rectangle->Height) / 2;
}

/**
 * Ensures a rectangle is positioned within the 
 * working area of the specified window's monitor.
 *
 * \param hWnd A handle to a window.
 * \param Rectangle The rectangle to be adjusted.
 */
VOID PhAdjustRectangleToWorkingArea(
    __in HWND hWnd,
    __inout PPH_RECTANGLE Rectangle
    )
{
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };

    if (GetMonitorInfo(
        MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),
        &monitorInfo
        ))
    {
        PH_RECTANGLE bounds;

        bounds = PhRectToRectangle(monitorInfo.rcWork);
        PhAdjustRectangleToBounds(Rectangle, &bounds);
    }
}

VOID PhCenterWindow(
    __in HWND WindowHandle,
    __in_opt HWND ParentWindowHandle
    )
{
    if (ParentWindowHandle)
    {
        RECT rect, parentRect;
        PH_RECTANGLE rectangle, parentRectangle;

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

VOID PhReferenceObjects(
    __in PPVOID Objects,
    __in ULONG NumberOfObjects
    )
{
    ULONG i;

    for (i = 0; i < NumberOfObjects; i++)
        PhReferenceObject(Objects[i]);
}

VOID PhDereferenceObjects(
    __in PPVOID Objects,
    __in ULONG NumberOfObjects
    )
{
    ULONG i;

    for (i = 0; i < NumberOfObjects; i++)
        PhDereferenceObject(Objects[i]);
}

/**
 * Gets a string stored in a DLL's message table.
 *
 * \param DllHandle The base address of the DLL.
 * \param MessageTableId The identifier of the message table.
 * \param MessageLanguageId The language ID of the message.
 * \param MessageId The identifier of the message.
 *
 * \return A pointer to a string containing the message. 
 * You must free the string using PhDereferenceObject() 
 * when you no longer need it.
 */
PPH_STRING PhGetMessage(
    __in PVOID DllHandle,
    __in ULONG MessageTableId,
    __in ULONG MessageLanguageId,
    __in ULONG MessageId
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
            GetSystemDefaultLangID(),
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

    if (messageEntry->Flags & MESSAGE_RESOURCE_UNICODE)
    {
        return PhCreateStringEx((PWSTR)messageEntry->Text, messageEntry->Length);
    }
    else
    {
        return PhCreateStringFromAnsiEx((PSTR)messageEntry->Text, messageEntry->Length);
    }
}

/**
 * Gets a message describing a NT status value.
 *
 * \param Status The NT status value.
 */
PPH_STRING PhGetNtMessage(
    __in NTSTATUS Status
    )
{
    PPH_STRING message;

    if (!NT_NTWIN32(Status))
        message = PhGetMessage(GetModuleHandle(L"ntdll.dll"), 0xb, GetUserDefaultLangID(), (ULONG)Status);
    else
        message = PhGetWin32Message(WIN32_FROM_NTSTATUS(Status));

    if (!message)
        return NULL;
    if (message->Length == 0)
        return message;

    // Fix those messages which are formatted like:
    // {Asdf}\r\nAsdf asdf asdf...
    if (message->Buffer[0] == '{')
    {
        ULONG indexOfNewLine = PhFindCharInString(message, 0, '\n');

        if (indexOfNewLine != -1)
        {
            PPH_STRING newMessage;

            newMessage = PhSubstring(
                message,
                indexOfNewLine + 1,
                message->Length / 2 - indexOfNewLine - 1
                );
            PhDereferenceObject(message);

            message = newMessage;
        }
    }

    return message;
}

/**
 * Gets a message describing a Win32 error code.
 *
 * \param Result The Win32 error code.
 */
PPH_STRING PhGetWin32Message(
    __in ULONG Result
    )
{
    return PhGetMessage(GetModuleHandle(L"kernel32.dll"), 0xb, GetUserDefaultLangID(), Result);
}

INT PhShowMessage(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PhShowMessage_V(hWnd, Type, Format, argptr);
}

INT PhShowMessage_V(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    __in va_list ArgPtr
    )
{
    INT result;
    WCHAR message[PH_MAX_MESSAGE_SIZE + 1];

    result = _vsnwprintf(message, PH_MAX_MESSAGE_SIZE, Format, ArgPtr);

    if (result == -1)
        return -1;

    return MessageBox(hWnd, message, PhApplicationName, Type);
}

PPH_STRING PhpGetStatusMessage(
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
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
            Win32Result = RtlNtStatusToDosError(Status);
        }
        // Process NTSTATUS values with the NT-Win32 facility.
        else if (NT_NTWIN32(Status))
        {
            Win32Result = WIN32_FROM_NTSTATUS(Status);
        }
    }

    if (!Win32Result)
        return PhGetNtMessage(Status);
    else
        return PhGetWin32Message(Win32Result);
}

VOID PhShowStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    PPH_STRING statusMessage;

    statusMessage = PhpGetStatusMessage(Status, Win32Result);

    if (!statusMessage)
    {
        if (Message)
        {
            PhShowError(hWnd, L"%s.", Message);
        }
        else
        {
            PhShowError(hWnd, L"Unable to perform the operation.");
        }

        return;
    }

    if (Message)
    {
        PhShowError(hWnd, L"%s: %s", Message, statusMessage->Buffer);
    }
    else
    {
        PhShowError(hWnd, L"%s", statusMessage->Buffer);
    }

    PhDereferenceObject(statusMessage);
}

BOOLEAN PhShowContinueStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    PPH_STRING statusMessage;
    INT result;

    statusMessage = PhpGetStatusMessage(Status, Win32Result);

    if (!statusMessage)
    {
        if (Message)
        {
            result = PhShowMessage(hWnd, MB_ICONERROR | MB_OKCANCEL, L"%s.", Message);
        }
        else
        {
            result = PhShowMessage(hWnd, MB_ICONERROR | MB_OKCANCEL, L"Unable to perform the operation.");
        }

        return result == IDOK;
    }

    if (Message)
    {
        result = PhShowMessage(hWnd, MB_ICONERROR | MB_OKCANCEL, L"%s: %s", Message, statusMessage->Buffer);
    }
    else
    {
        result = PhShowMessage(hWnd, MB_ICONERROR | MB_OKCANCEL, L"%s", statusMessage->Buffer);
    }

    PhDereferenceObject(statusMessage);

    return result == IDOK;
}

BOOLEAN PhShowConfirmMessage(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PWSTR Object,
    __in_opt PWSTR Message,
    __in BOOLEAN Warning
    )
{
    PPH_STRING verb;
    PPH_STRING verbCaps;
    PPH_STRING action;

    // Make sure the verb is all lowercase.
    verb = PhaLowerString(PhaCreateString(Verb));

    // "terminate" -> "Terminate"
    verbCaps = PhaDuplicateString(verb);
    if (verbCaps->Length > 0) verbCaps->Buffer[0] = towupper(verbCaps->Buffer[0]);

    // "terminate", "the process" -> "terminate the process"
    action = PhaConcatStrings(3, verb->Buffer, L" ", Object);

    if (TaskDialogIndirect_I)
    {
        TASKDIALOGCONFIG config = { sizeof(config) };
        TASKDIALOG_BUTTON buttons[2];
        INT button;

        config.hwndParent = hWnd;
        config.hInstance = PhInstanceHandle;
        config.pszWindowTitle = L"Process Hacker";
        config.pszMainIcon = Warning ? TD_WARNING_ICON : NULL;
        config.pszMainInstruction = PhaConcatStrings(3, L"Do you want to ", action->Buffer, L"?")->Buffer;

        if (Message)
            config.pszContent = PhaConcatStrings2(Message, L" Are you sure you want to continue?")->Buffer;

        buttons[0].nButtonID = IDYES;
        buttons[0].pszButtonText = verbCaps->Buffer;
        buttons[1].nButtonID = IDNO;
        buttons[1].pszButtonText = L"Cancel";

        config.cButtons = 2;
        config.pButtons = buttons;
        config.nDefaultButton = IDNO;

        if (TaskDialogIndirect_I(
            &config,
            &button,
            NULL,
            NULL
            ) == S_OK)
        {
            return button == IDYES;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return PhShowMessage(
            hWnd,
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
            L"Are you sure you want to %s?",
            action->Buffer
            ) == IDYES;
    }
}

BOOLEAN PhFindIntegerSiKeyValuePairs(
    __in PPH_KEY_VALUE_PAIR KeyValuePairs,
    __in ULONG SizeOfKeyValuePairs,
    __in PWSTR String,
    __out PULONG Integer
    )
{
    ULONG i;

    for (i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if (WSTR_IEQUAL(KeyValuePairs[i].Key, String))
        {
            *Integer = (ULONG)KeyValuePairs[i].Value;
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PhFindStringSiKeyValuePairs(
    __in PPH_KEY_VALUE_PAIR KeyValuePairs,
    __in ULONG SizeOfKeyValuePairs,
    __in ULONG Integer,
    __out PWSTR *String
    )
{
    ULONG i;

    for (i = 0; i < SizeOfKeyValuePairs / sizeof(PH_KEY_VALUE_PAIR); i++)
    {
        if ((ULONG)KeyValuePairs[i].Value == Integer)
        {
            *String = (PWSTR)KeyValuePairs[i].Key;
            return TRUE;
        }
    }

    return FALSE;
}

VOID PhGenerateGuid(
    __out PGUID Guid
    )
{
    static ULONG seed = 0;
    // The top/sign bit is always unusable for RtlRandomEx 
    // (the result is always unsigned), so we'll take the 
    // bottom 24 bits. We need 128 bits in total, so we'll 
    // call the function 6 times.
    ULONG random[6];
    ULONG i;

    for (i = 0; i < 6; i++)
        random[i] = RtlRandomEx(&seed);

    // random[0] is usable
    *(PSHORT)&Guid->Data1 = (SHORT)random[0];
    // top byte from random[0] is usable
    *((PSHORT)&Guid->Data1 + 1) = (SHORT)((random[0] >> 16) | (random[1] & 0xff));
    // top 2 bytes from random[1] are usable
    Guid->Data2 = (SHORT)(random[1] >> 8);
    // random[2] is usable
    Guid->Data3 = (SHORT)random[2];
    // top byte from random[2] is usable
    *(PSHORT)&Guid->Data4[0] = (SHORT)((random[2] >> 16) | (random[3] & 0xff));
    // top 2 bytes from random[3] are usable
    *(PSHORT)&Guid->Data4[2] = (SHORT)(random[3] >> 8);
    // random[4] is usable
    *(PSHORT)&Guid->Data4[4] = (SHORT)random[4];
    // top byte from random[4] is usable
    *(PSHORT)&Guid->Data4[6] = (SHORT)((random[4] >> 16) | (random[5] & 0xff));
}

VOID PhGenerateRandomAlphaString(
    __out_ecount_z(Count) PWSTR Buffer,
    __in ULONG Count
    )
{
    static ULONG seed = 0;
    ULONG i;

    if (Count == 0)
        return;

    for (i = 0; i < Count - 1; i++)
    {
        Buffer[i] = 'A' + (RtlRandomEx(&seed) % 26);
    }

    Buffer[Count - 1] = 0;
}

PPH_STRING PhEllipsisString(
    __in PPH_STRING String,
    __in ULONG DesiredCount
    )
{
    if (
        (ULONG)String->Length / 2 <= DesiredCount ||
        DesiredCount < 3
        )
    {
        PhReferenceObject(String);
        return String;
    }
    else
    {
        PPH_STRING string;

        string = PhCreateStringEx(NULL, DesiredCount * 2);
        memcpy(string->Buffer, String->Buffer, (DesiredCount - 3) * 2);
        memcpy(&string->Buffer[DesiredCount - 3], L"...", 6);

        return string;
    }
}

PPH_STRING PhEllipsisStringPath(
    __in PPH_STRING String,
    __in ULONG DesiredCount
    )
{
    ULONG secondPartIndex;

    secondPartIndex = PhFindLastCharInString(String, 0, L'\\');

    if (secondPartIndex == -1)
        secondPartIndex = PhFindLastCharInString(String, 0, L'/');
    if (secondPartIndex == -1)
        return PhEllipsisString(String, DesiredCount);

    if (
        (ULONG)String->Length / 2 <= DesiredCount ||
        DesiredCount < 3
        )
    {
        PhReferenceObject(String);
        return String;
    }
    else
    {
        PPH_STRING string;
        ULONG firstPartCopyLength;
        ULONG secondPartCopyLength;

        string = PhCreateStringEx(NULL, DesiredCount * 2);
        secondPartCopyLength = String->Length / 2 - secondPartIndex;

        // Check if we have enough space for the entire second part of the string.
        if (secondPartCopyLength + 3 <= DesiredCount)
        {
            // Yes, copy part of the first part and the entire second part.
            firstPartCopyLength = DesiredCount - secondPartCopyLength - 3;
        }
        else
        {
            // No, copy part of both, from the beginning of the first part and 
            // the end of the second part.
            firstPartCopyLength = (DesiredCount - 3) / 2;
            secondPartCopyLength = DesiredCount - 3 - firstPartCopyLength;
            secondPartIndex = String->Length / 2 - secondPartCopyLength;
        }

        memcpy(
            string->Buffer,
            String->Buffer,
            firstPartCopyLength * 2
            );
        memcpy(
            &string->Buffer[firstPartCopyLength],
            L"...",
            6
            );
        memcpy(
            &string->Buffer[firstPartCopyLength + 3],
            &String->Buffer[secondPartIndex],
            secondPartCopyLength * 2
            );

        return string;
    }
}

PPH_STRING PhFormatDate(
    __in_opt PSYSTEMTIME Date,
    __in_opt PWSTR Format
    )
{
    PPH_STRING string;
    ULONG bufferSize;

    bufferSize = GetDateFormat(LOCALE_USER_DEFAULT, 0, Date, Format, NULL, 0);
    string = PhCreateStringEx(NULL, bufferSize * 2);

    if (!GetDateFormat(LOCALE_USER_DEFAULT, 0, Date, Format, string->Buffer, bufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

PPH_STRING PhFormatTime(
    __in_opt PSYSTEMTIME Time,
    __in_opt PWSTR Format
    )
{
    PPH_STRING string;
    ULONG bufferSize;

    bufferSize = GetTimeFormat(LOCALE_USER_DEFAULT, 0, Time, Format, NULL, 0);
    string = PhCreateStringEx(NULL, bufferSize * 2);

    if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0, Time, Format, string->Buffer, bufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

PPH_STRING PhFormatDateTime(
    __in_opt PSYSTEMTIME DateTime
    )
{
    PPH_STRING string;
    ULONG dateBufferSize;
    ULONG timeBufferSize;
    ULONG count;

    dateBufferSize = GetDateFormat(LOCALE_USER_DEFAULT, 0, DateTime, NULL, NULL, 0);
    timeBufferSize = GetTimeFormat(LOCALE_USER_DEFAULT, 0, DateTime, NULL, NULL, 0);

    string = PhCreateStringEx(NULL, (dateBufferSize + 1 + timeBufferSize) * 2);

    if (!GetDateFormat(LOCALE_USER_DEFAULT, 0, DateTime, NULL, &string->Buffer[0], dateBufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    count = (ULONG)wcslen(string->Buffer);
    string->Buffer[count] = ' ';

    if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0, DateTime, NULL, &string->Buffer[count + 1], timeBufferSize))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

PPH_STRING PhFormatTimeSpanRelative(
    __in ULONG64 TimeSpan
    )
{
    PH_AUTO_POOL autoPool;
    PPH_STRING string;
    DOUBLE days;
    DOUBLE weeks;
    DOUBLE fortnights;
    DOUBLE months;
    DOUBLE years;
    DOUBLE centuries;

    PhInitializeAutoPool(&autoPool);

    days = (DOUBLE)TimeSpan / PH_TICKS_PER_DAY;
    weeks = days / 7;
    fortnights = weeks / 2;
    years = days / 365.2425;
    months = years * 12;
    centuries = years / 100;

    if (centuries >= 1)
    {
        string = PhaFormatString(L"%u %s", (ULONG)centuries, (ULONG)centuries == 1 ? L"century" : L"centuries");
    }
    else if (years >= 1)
    {
        string = PhaFormatString(L"%u %s", (ULONG)years, (ULONG)years == 1 ? L"year" : L"years");
    }
    else if (months >= 1)
    {
        string = PhaFormatString(L"%u %s", (ULONG)months, (ULONG)months == 1 ? L"month" : L"months");
    }
    else if (fortnights >= 1)
    {
        string = PhaFormatString(L"%u %s", (ULONG)fortnights, (ULONG)fortnights == 1 ? L"fortnight" : L"fortnights");
    }
    else if (weeks >= 1)
    {
        string = PhaFormatString(L"%u %s", (ULONG)weeks, (ULONG)weeks == 1 ? L"week" : L"weeks");
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
            string = PhaFormatString(L"%u %s", (ULONG)days, (ULONG)days == 1 ? L"day" : L"days");
            hoursPartial = (ULONG)PH_TICKS_PARTIAL_HOURS(TimeSpan);

            if (hoursPartial >= 1)
            {
                string = PhaFormatString(L"%s and %u %s", string->Buffer, hoursPartial, hoursPartial == 1 ? L"hour" : L"hours");
            }
        }
        else if (hours >= 1)
        {
            string = PhaFormatString(L"%u %s", (ULONG)hours, (ULONG)hours == 1 ? L"hour" : L"hours");
            minutesPartial = (ULONG)PH_TICKS_PARTIAL_MIN(TimeSpan);

            if (minutesPartial >= 1)
            {
                string = PhaFormatString(L"%s and %u %s", string->Buffer, (ULONG)minutesPartial, (ULONG)minutesPartial == 1 ? L"minute" : L"minutes");
            }
        }
        else if (minutes >= 1)
        {
            string = PhaFormatString(L"%u %s", (ULONG)minutes, (ULONG)minutes == 1 ? L"minute" : L"minutes");
            secondsPartial = (ULONG)PH_TICKS_PARTIAL_SEC(TimeSpan);

            if (secondsPartial >= 1)
            {
                string = PhaFormatString(L"%s and %u %s", string->Buffer, (ULONG)secondsPartial, (ULONG)secondsPartial == 1 ? L"second" : L"seconds");
            }
        }
        else if (seconds >= 1)
        {
            string = PhaFormatString(L"%u %s", (ULONG)seconds, (ULONG)seconds == 1 ? L"second" : L"seconds");
        }
        else if (milliseconds >= 1)
        {
            string = PhaFormatString(L"%u %s", (ULONG)milliseconds, (ULONG)milliseconds == 1 ? L"millisecond" : L"milliseconds");
        }
        else
        {
            string = PhaCreateString(L"a very short time");
        }
    }

    // Turn 1 into "a", e.g. 1 minute -> a minute
    if (PhStartsWithString2(string, L"1 ", FALSE))
    {
        // Special vowel case: a hour -> an hour
        if (string->Buffer[2] != 'h')
            string = PhaConcatStrings2(L"a ", &string->Buffer[2]);
        else
            string = PhaConcatStrings2(L"an ", &string->Buffer[2]);
    }

    string = PhConcatStrings2(string->Buffer, L" ago");

    PhDeleteAutoPool(&autoPool);

    return string;
}

PPH_STRING PhFormatUInt64(
    __in ULONG64 Value,
    __in BOOLEAN GroupDigits
    )
{
    WCHAR string[PH_INT64_STR_LEN_1];

    PhPrintUInt64(string, Value);

    return PhFormatDecimal(string, 0, GroupDigits);
}

PPH_STRING PhpFormatDecimalFast(
    __in PWSTR Value,
    __in ULONG FractionalDigits,
    __in WCHAR DecimalSeparator,
    __in WCHAR ThousandSeparator
    )
{
    PPH_STRING string;
    ULONG inCount;
    PWCHAR whole;
    PWCHAR dot;
    PWCHAR fractional;
    ULONG inWholeCount;
    ULONG inFractionalCount;
    ULONG count;
    ULONG outFractionalCount;
    PWCHAR c;

    inCount = (ULONG)wcslen(Value);

    dot = wcschr(Value, '.');

    if (dot)
    {
        inWholeCount = (ULONG)(dot - Value);
        inFractionalCount = inCount - inWholeCount - 1;
        fractional = dot + 1;
    }
    else
    {
        inWholeCount = inCount;
        inFractionalCount = 0;
        fractional = NULL;
    }

    // Ignore leading zeros.

    whole = Value;

    while (inWholeCount != 0 && *whole == '0')
    {
        whole++;
        inWholeCount--;
    }

    // Ignore trailing zeros.

    while (inFractionalCount != 0 && fractional[inFractionalCount - 1] == '0')
    {
        inFractionalCount--;
    }

    if (inWholeCount == 0)
    {
        if (FractionalDigits != 0)
        {
            string = PhCreateStringEx(NULL, (2 + FractionalDigits) * sizeof(WCHAR));
            string->Buffer[0] = '0';
            string->Buffer[1] = DecimalSeparator;
            wmemset(&string->Buffer[2], '0', FractionalDigits);
        }
        else
        {
            return PhCreateString(L"0");
        }

        return string;
    }

    // Calculate the number of characters in the output.

    // Start with the number of whole digits.
    count = inWholeCount;
    // Add the space needed for the thousand separators.
    count += (inWholeCount - 1) / 3;

    // Add space for the decimal separator and fractional digits.
    if (FractionalDigits != 0)
    {
        count += 1;
        count += FractionalDigits;
    }

    string = PhCreateStringEx(NULL, count * sizeof(WCHAR));
    c = string->Buffer;

    while (inWholeCount != 0)
    {
        *c++ = *whole++;

        if (inWholeCount != 1 && (inWholeCount - 1) % 3 == 0)
            *c++ = ThousandSeparator;

        inWholeCount--;
    }

    if (FractionalDigits != 0)
    {
        *c++ = DecimalSeparator;
        outFractionalCount = inFractionalCount;

        if (outFractionalCount > FractionalDigits)
            outFractionalCount = FractionalDigits;

        while (outFractionalCount != 0)
        {
            *c++ = *fractional++;
            outFractionalCount--;
        }

        // Pad the remaining space with zeros.
        if (inFractionalCount < FractionalDigits)
        {
            outFractionalCount = FractionalDigits - inFractionalCount;

            while (outFractionalCount != 0)
            {
                *c++ = '0';
                outFractionalCount--;
            }
        }
    }

    return string;
}

PPH_STRING PhFormatDecimal(
    __in PWSTR Value,
    __in ULONG FractionalDigits,
    __in BOOLEAN GroupDigits
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static WCHAR decimalSeparator[4];
    static WCHAR thousandSeparator[4];
    static BOOLEAN canUseFast;

    PPH_STRING string;
    NUMBERFMT format;
    ULONG bufferSize;

    if (PhBeginInitOnce(&initOnce))
    {
        if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimalSeparator, 4))
        {
            decimalSeparator[0] = '.';
            decimalSeparator[1] = 0;
        }

        if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousandSeparator, 4))
        {
            thousandSeparator[0] = ',';
            thousandSeparator[1] = 0;
        }

        canUseFast =
            decimalSeparator[0] != 0 && decimalSeparator[1] == 0 &&
            thousandSeparator[0] != 0 && thousandSeparator[1] == 0;

        PhEndInitOnce(&initOnce);
    }

    if (canUseFast && GroupDigits)
    {
        return PhpFormatDecimalFast(
            Value,
            FractionalDigits,
            decimalSeparator[0],
            thousandSeparator[0]
            );
    }

    format.NumDigits = FractionalDigits;
    format.LeadingZero = 0;
    format.Grouping = GroupDigits ? 3 : 0;
    format.lpDecimalSep = decimalSeparator;
    format.lpThousandSep = thousandSeparator;
    format.NegativeOrder = 1;

    bufferSize = GetNumberFormat(LOCALE_USER_DEFAULT, 0, Value, &format, NULL, 0);
    string = PhCreateStringEx(NULL, bufferSize * 2);

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
 * \param MaxSizeUnit The largest unit of size to 
 * use, -1 to use PhMaxSizeUnit, or -2 for no limit.
 * \li \c 0 Bytes.
 * \li \c 1 Kilobytes.
 * \li \c 2 Megabytes.
 * \li \c 3 Gigabytes.
 * \li \c 4 Terabytes.
 * \li \c 5 Petabytes.
 * \li \c 6 Exabytes.
 */
PPH_STRING PhFormatSize(
    __in ULONG64 Size,
    __in ULONG MaxSizeUnit
    )
{
    ULONG i = 0;
    ULONG maxSizeUnit;
    DOUBLE s = (DOUBLE)Size;

    if (Size == 0)
        return PhCreateString(L"0");

    if (MaxSizeUnit != -1)
        maxSizeUnit = MaxSizeUnit;
    else
        maxSizeUnit = PhMaxSizeUnit;

    while (
        s > 1024 &&
        i < sizeof(PhSizeUnitNames) / sizeof(PWSTR) &&
        i < maxSizeUnit
        )
    {
        s /= 1024;
        i++;
    }

    {
        WCHAR numberString[512]; // perf hack
        PPH_STRING formattedString;
        PPH_STRING outputString;

        swprintf_s(numberString, sizeof(numberString) / 2, L"%.2f", s);
        formattedString = PhFormatDecimal(numberString, 2, TRUE);

        if (!formattedString)
        {
            return PhFormatString(L"%.2g %s", s, PhSizeUnitNames[i]);
        }

        if (
            PhEndsWithString2(formattedString, L"00", FALSE) &&
            formattedString->Length >= 6
            )
        {
            // Remove the last three characters by making sure 
            // PhConcatStrings doesn't include them.
            formattedString->Buffer[formattedString->Length / 2 - 3] = 0;
        }
        else if (PhEndsWithString2(formattedString, L"0", FALSE))
        {
            // Remove the last character.
            formattedString->Buffer[formattedString->Length / 2 - 1] = 0;
        }

        outputString = PhConcatStrings(3, formattedString->Buffer, L" ", PhSizeUnitNames[i]);
        PhDereferenceObject(formattedString);

        return outputString;
    }
}

PPH_STRING PhFormatGuid(
    __in PGUID Guid
    )
{
    PPH_STRING string;
    UNICODE_STRING unicodeString;

    if (!NT_SUCCESS(RtlStringFromGUID(Guid, &unicodeString)))
        return NULL;

    string = PhCreateStringEx(unicodeString.Buffer, unicodeString.Length);
    RtlFreeUnicodeString(&unicodeString);

    return string;
}

PVOID PhGetFileVersionInfo(
    __in PWSTR FileName
    )
{
    ULONG versionInfoSize;
    ULONG dummy;
    PVOID versionInfo;

    versionInfoSize = GetFileVersionInfoSize(
        FileName,
        &dummy
        );

    if (versionInfoSize)
    {
        versionInfo = PhAllocate(versionInfoSize);

        if (!GetFileVersionInfo(
            FileName,
            0,
            versionInfoSize,
            versionInfo
            ))
        {
            PhFree(versionInfo);

            return NULL;
        }
    }
    else
    {
        return NULL;
    }

    return versionInfo;
}

ULONG PhGetFileVersionInfoLangCodePage(
    __in PVOID VersionInfo
    )
{
    PVOID buffer;
    ULONG length;

    if (VerQueryValue(VersionInfo, L"\\VarFileInfo\\Translation", &buffer, &length))
    {
        // Combine the language ID and code page.
        return (*(PUSHORT)buffer << 16) + *((PUSHORT)buffer + 1);
    }
    else
    {
        return 0x40904e4;
    }
}

PPH_STRING PhGetFileVersionInfoString(
    __in PVOID VersionInfo,
    __in PWSTR SubBlock
    )
{
    PVOID buffer;
    ULONG length;

    if (VerQueryValue(VersionInfo, SubBlock, &buffer, &length))
    {
        PPH_STRING string;

        string = PhCreateStringEx((PWSTR)buffer, length * sizeof(WCHAR));
        // length may include the null terminator.
        PhTrimToNullTerminatorString(string);

        return string;
    }
    else
    {
        return NULL;
    }
}

PPH_STRING PhGetFileVersionInfoString2(
    __in PVOID VersionInfo,
    __in ULONG LangCodePage,
    __in PWSTR StringName
    )
{
    WCHAR subBlock[65];

    _snwprintf(subBlock, 64, L"\\StringFileInfo\\%08X\\%s", LangCodePage, StringName);

    return PhGetFileVersionInfoString(VersionInfo, subBlock);
}

BOOLEAN PhInitializeImageVersionInfo(
    __out PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in PWSTR FileName
    )
{
    PVOID versionInfo;
    ULONG langCodePage;

    versionInfo = PhGetFileVersionInfo(FileName);

    if (!versionInfo)
        return FALSE;

    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);

    ImageVersionInfo->CompanyName = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"CompanyName");
    ImageVersionInfo->FileDescription = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"FileDescription");
    ImageVersionInfo->FileVersion = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"FileVersion");
    ImageVersionInfo->ProductName = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"ProductName");

    PhFree(versionInfo);

    return TRUE;
}

VOID PhDeleteImageVersionInfo(
    __inout PPH_IMAGE_VERSION_INFO ImageVersionInfo
    )
{
    if (ImageVersionInfo->CompanyName) PhDereferenceObject(ImageVersionInfo->CompanyName);
    if (ImageVersionInfo->FileDescription) PhDereferenceObject(ImageVersionInfo->FileDescription);
    if (ImageVersionInfo->FileVersion) PhDereferenceObject(ImageVersionInfo->FileVersion);
    if (ImageVersionInfo->ProductName) PhDereferenceObject(ImageVersionInfo->ProductName);
}

PPH_STRING PhFormatImageVersionInfo(
    __in_opt PPH_STRING FileName,
    __in PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in_opt PWSTR Indent,
    __in_opt ULONG LineLimit
    )
{
    PPH_STRING string;
    PH_STRING_BUILDER stringBuilder;
    ULONG indentLength;

    if (Indent)
        indentLength = (ULONG)wcslen(Indent) * sizeof(WCHAR);
    if (LineLimit == 0)
        LineLimit = MAXULONG32;

    PhInitializeStringBuilder(&stringBuilder, 40);

    // File name

    if (!PhIsNullOrEmptyString(FileName))
    {
        PPH_STRING temp;

        if (Indent) PhAppendStringBuilderEx(&stringBuilder, Indent, indentLength);

        temp = PhEllipsisStringPath(FileName, LineLimit);
        PhAppendStringBuilderEx(&stringBuilder, temp->Buffer, temp->Length);
        PhDereferenceObject(temp);
        PhAppendCharStringBuilder(&stringBuilder, '\n');
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

        if (LineLimit != MAXULONG32)
        {
            limitForVersion = (LineLimit - 1) / 5; // 1/5 space for version (and space character)
            limitForDescription = LineLimit - limitForVersion;
        }
        else
        {
            limitForDescription = MAXULONG32;
            limitForVersion = MAXULONG32;
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

        if (Indent) PhAppendStringBuilderEx(&stringBuilder, Indent, indentLength);

        if (tempDescription)
        {
            PhAppendStringBuilderEx(
                &stringBuilder,
                tempDescription->Buffer,
                tempDescription->Length
                );

            if (tempVersion)
                PhAppendCharStringBuilder(&stringBuilder, ' '); 
        }

        if (tempVersion)
        {
            PhAppendStringBuilderEx(
                &stringBuilder,
                tempVersion->Buffer,
                tempVersion->Length
                );
        }

        if (tempDescription)
            PhDereferenceObject(tempDescription);
        if (tempVersion)
            PhDereferenceObject(tempVersion);

        PhAppendCharStringBuilder(&stringBuilder, '\n');
    }

    // File company

    if (!PhIsNullOrEmptyString(ImageVersionInfo->CompanyName))
    {
        PPH_STRING temp;

        if (Indent) PhAppendStringBuilderEx(&stringBuilder, Indent, indentLength);

        temp = PhEllipsisString(ImageVersionInfo->CompanyName, LineLimit);
        PhAppendStringBuilderEx(&stringBuilder, temp->Buffer, temp->Length);
        PhDereferenceObject(temp);
        PhAppendCharStringBuilder(&stringBuilder, '\n');
    }

    // Remove the extra newline.
    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    string = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

    return string;
}

PPH_STRING PhGetFullPath(
    __in PWSTR FileName,
    __out_opt PULONG IndexOfFileName
    )
{
    PPH_STRING fullPath;
    ULONG bufferSize;
    ULONG returnLength;
    PWSTR filePart;

    bufferSize = 0x80;
    fullPath = PhCreateStringEx(NULL, bufferSize * 2);

    returnLength = RtlGetFullPathName_U(FileName, bufferSize, fullPath->Buffer, &filePart);

    if (returnLength > bufferSize)
    {
        PhDereferenceObject(fullPath);
        bufferSize = returnLength;
        fullPath = PhCreateStringEx(NULL, bufferSize * 2);

        returnLength = RtlGetFullPathName_U(FileName, bufferSize, fullPath->Buffer, &filePart);
    }

    if (returnLength == 0)
    {
        PhDereferenceObject(fullPath);
        return NULL;
    }

    PhTrimToNullTerminatorString(fullPath);

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
            *IndexOfFileName = -1;
        }
    }

    return fullPath;
}

PPH_STRING PhExpandEnvironmentStrings(
    __in PPH_STRINGREF String
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG bufferLength;

    bufferLength = 0x40;
    string = PhCreateStringEx(NULL, bufferLength);
    string->us.Length = 0;

    status = RtlExpandEnvironmentStrings_U(
        NULL,
        &String->us,
        &string->us,
        &bufferLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhDereferenceObject(string);
        string = PhCreateStringEx(NULL, bufferLength);
        string->us.Length = 0;

        status = RtlExpandEnvironmentStrings_U(
            NULL,
            &String->us,
            &string->us,
            &bufferLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    string->Buffer[string->Length / 2] = 0; // make sure there is a null terminator
    string->us.MaximumLength = string->us.Length;

    return string;
}

PPH_STRING PhGetBaseName(
    __in PPH_STRING FileName
    )
{
    ULONG lastIndexOfBackslash;

    lastIndexOfBackslash = PhFindLastCharInString(FileName, 0, '\\');

    if (lastIndexOfBackslash == -1)
    {
        PhReferenceObject(FileName);
        return FileName;
    }

    return PhSubstring(
        FileName,
        lastIndexOfBackslash + 1,
        FileName->Length / 2 - lastIndexOfBackslash - 1
        );
}

PPH_STRING PhGetSystemDirectory()
{
    PPH_STRING systemDirectory;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x40;
    systemDirectory = PhCreateStringEx(NULL, bufferSize * 2);

    returnLength = GetSystemDirectory(systemDirectory->Buffer, bufferSize);

    if (returnLength > bufferSize)
    {
        PhDereferenceObject(systemDirectory);
        bufferSize = returnLength;
        systemDirectory = PhCreateStringEx(NULL, bufferSize * 2);

        returnLength = GetSystemDirectory(systemDirectory->Buffer, bufferSize);
    }

    if (returnLength == 0)
    {
        PhDereferenceObject(systemDirectory);
        return NULL;
    }

    PhTrimToNullTerminatorString(systemDirectory);

    return systemDirectory;
}

PPH_STRING PhGetSystemRoot()
{
    return PhCreateString(USER_SHARED_DATA->NtSystemRoot);
}

PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    __in PVOID DllHandle
    )
{
    PLDR_DATA_TABLE_ENTRY result = NULL;
    PLDR_DATA_TABLE_ENTRY entry;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    listHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    listEntry = listHead->Flink;

    while (listEntry != listHead)
    {
        entry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (entry->DllBase == DllHandle)
        {
            result = entry;
            break;
        }

        listEntry = listEntry->Flink;
    }

    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    return result;
}

PPH_STRING PhGetDllFileName(
    __in PVOID DllHandle,
    __out_opt PULONG IndexOfFileName
    )
{
    PLDR_DATA_TABLE_ENTRY entry;
    PPH_STRING fileName;
    PPH_STRING newFileName;
    ULONG indexOfFileName;

    entry = PhFindLoaderEntry(DllHandle);

    if (!entry)
        return NULL;

    fileName = PhCreateStringEx(entry->FullDllName.Buffer, entry->FullDllName.Length);
    newFileName = PhGetFileName(fileName);
    PhDereferenceObject(fileName);
    fileName = newFileName;

    if (IndexOfFileName)
    {
        indexOfFileName = PhFindLastCharInString(fileName, 0, '\\');

        if (indexOfFileName != -1)
            indexOfFileName++;
        else
            indexOfFileName = -1;

        *IndexOfFileName = indexOfFileName;
    }

    return fileName;
}

PPH_STRING PhGetApplicationFileName()
{
    return PhGetDllFileName(NtCurrentPeb()->ImageBaseAddress, NULL);
}

PPH_STRING PhGetApplicationDirectory()
{
    PPH_STRING fileName;
    ULONG indexOfFileName;
    PPH_STRING path = NULL;

    fileName = PhGetDllFileName(NtCurrentPeb()->ImageBaseAddress, &indexOfFileName);

    if (fileName)
    {
        if (indexOfFileName != -1)
        {
            // Remove the file name from the path.
            path = PhSubstring(fileName, 0, indexOfFileName);
        }

        PhDereferenceObject(fileName);
    }

    return path;
}

/**
 * Gets a known location as a file name.
 *
 * \param Folder A CSIDL value representing the known location.
 * \param AppendPath A string to append to the folder path.
 */
PPH_STRING PhGetKnownLocation(
    __in ULONG Folder,
    __in_opt PWSTR AppendPath
    )
{
    WCHAR folderPath[MAX_PATH];

    if (SHGetFolderPath(
        NULL,
        Folder,
        NULL,
        SHGFP_TYPE_CURRENT,
        folderPath
        ) != E_INVALIDARG)
    {
        if (AppendPath)
        {
            return PhConcatStrings2(folderPath, AppendPath);
        }
        else
        {
            return PhCreateString(folderPath);
        }
    }

    return NULL;
}

NTSTATUS PhWaitForMultipleObjectsAndPump(
    __in_opt HWND hWnd,
    __in ULONG NumberOfHandles,
    __in PHANDLE Handles,
    __in ULONG Timeout
    )
{
    NTSTATUS status;
    ULONG64 startTickCount;
    ULONG64 currentTickCount;
    LONG64 currentTimeout;

    startTickCount = NtGetTickCount64();
    currentTimeout = Timeout;

    while (TRUE)
    {
        status = MsgWaitForMultipleObjects(
            NumberOfHandles,
            Handles,
            FALSE,
            (ULONG)currentTimeout,
            QS_ALLEVENTS
            );

        if (status >= STATUS_WAIT_0 && status < (NTSTATUS)(STATUS_WAIT_0 + NumberOfHandles))
        {
            return status;
        }
        else if (status == (STATUS_WAIT_0 + NumberOfHandles))
        {
            MSG msg;

            // Pump messages

            while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE))
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
            currentTimeout = (LONG64)Timeout - (currentTickCount - startTickCount);

            if (currentTimeout < 0)
                return STATUS_TIMEOUT;
        }
    }
}

NTSTATUS PhCreateProcessWin32(
    __in_opt PWSTR FileName,
    __in_opt PWSTR CommandLine,
    __in_opt PVOID Environment,
    __in_opt PWSTR CurrentDirectory,
    __in ULONG Flags,
    __in_opt HANDLE TokenHandle,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
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

static PH_FLAG_MAPPING PhpCreateProcessMappings[] =
{
    { PH_CREATE_PROCESS_UNICODE_ENVIRONMENT, CREATE_UNICODE_ENVIRONMENT },
    { PH_CREATE_PROCESS_SUSPENDED, CREATE_SUSPENDED },
    { PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB, CREATE_BREAKAWAY_FROM_JOB },
    { PH_CREATE_PROCESS_NEW_CONSOLE, CREATE_NEW_CONSOLE }
};

FORCEINLINE VOID PhpConvertProcessInformation(
    __in PPROCESS_INFORMATION ProcessInfo,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    )
{
    if (ClientId)
    {
        ClientId->UniqueProcess = UlongToHandle(ProcessInfo->dwProcessId);
        ClientId->UniqueThread = UlongToHandle(ProcessInfo->dwThreadId);
    }

    if (ProcessHandle)
        *ProcessHandle = ProcessInfo->hProcess;
    else
        NtClose(ProcessInfo->hProcess);

    if (ThreadHandle)
        *ThreadHandle = ProcessInfo->hThread;
    else
        NtClose(ProcessInfo->hThread);
}

NTSTATUS PhCreateProcessWin32Ex(
    __in_opt PWSTR FileName,
    __in_opt PWSTR CommandLine,
    __in_opt PVOID Environment,
    __in_opt PWSTR CurrentDirectory,
    __in_opt STARTUPINFO *StartupInfo,
    __in ULONG Flags,
    __in_opt HANDLE TokenHandle,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    )
{
    NTSTATUS status;
    PPH_STRING commandLine = NULL;
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    ULONG newFlags;

    if (CommandLine) // duplicate because CreateProcess modifies the string
        commandLine = PhCreateString(CommandLine);

    newFlags = 0;
    PhMapFlags1(&newFlags, Flags, PhpCreateProcessMappings, sizeof(PhpCreateProcessMappings) / sizeof(PH_FLAG_MAPPING));

    if (StartupInfo)
    {
        startupInfo = *StartupInfo;
    }
    else
    {
        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
    }

    if (!TokenHandle)
    {
        if (CreateProcess(
            FileName,
            PhGetString(commandLine),
            NULL,
            NULL,
            !!(Flags & PH_CREATE_PROCESS_INHERIT_HANDLES),
            newFlags,
            Environment,
            CurrentDirectory,
            &startupInfo,
            &processInfo
            ))
            status = STATUS_SUCCESS;
        else
            status = NTSTATUS_FROM_WIN32(GetLastError());
    }
    else
    {
        if (CreateProcessAsUser(
            TokenHandle,
            FileName,
            PhGetString(commandLine),
            NULL,
            NULL,
            !!(Flags & PH_CREATE_PROCESS_INHERIT_HANDLES),
            newFlags,
            Environment,
            CurrentDirectory,
            &startupInfo,
            &processInfo
            ))
            status = STATUS_SUCCESS;
        else
            status = NTSTATUS_FROM_WIN32(GetLastError());
    }

    if (commandLine)
        PhDereferenceObject(commandLine);

    if (NT_SUCCESS(status))
    {
        PhpConvertProcessInformation(&processInfo, ClientId, ProcessHandle, ThreadHandle);
    }

    return status;
}

NTSTATUS PhCreateProcessAsUser(
    __in PPH_CREATE_PROCESS_AS_USER_INFO Information,
    __in ULONG Flags,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    )
{
    static PH_INITONCE userEnvInitOnce = PH_INITONCE_INIT;
    static _CreateEnvironmentBlock CreateEnvironmentBlock_I = NULL;
    static _DestroyEnvironmentBlock DestroyEnvironmentBlock_I = NULL;

    NTSTATUS status;
    HANDLE tokenHandle;
    PVOID defaultEnvironment;
    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    BOOLEAN needsDuplicate = FALSE;

    if (PhBeginInitOnce(&userEnvInitOnce))
    {
        HMODULE userEnv;

        userEnv = LoadLibrary(L"userenv.dll");
        CreateEnvironmentBlock_I = (_CreateEnvironmentBlock)GetProcAddress(userEnv, "CreateEnvironmentBlock");
        DestroyEnvironmentBlock_I = (_DestroyEnvironmentBlock)GetProcAddress(userEnv, "DestroyEnvironmentBlock");

        PhEndInitOnce(&userEnvInitOnce);
    }

    if (!Information->ApplicationName && !Information->CommandLine)
        return STATUS_INVALID_PARAMETER_MIX;
    if ((!Information->DomainName || !Information->UserName || !Information->Password) && !Information->ProcessIdWithToken)
        return STATUS_INVALID_PARAMETER_MIX;

    // Whenever we can, try not to set the desktop name; it breaks a lot of things.
    if (Information->DesktopName && !WSTR_IEQUAL(Information->DesktopName, L"WinSta0\\Default"))
        startupInfo.lpDesktop = Information->DesktopName;

    // Try to use CreateProcessWithLogonW if we need to load the user profile. 
    // This isn't compatible with some options.
    if (Flags & PH_CREATE_PROCESS_WITH_PROFILE)
    {
        BOOLEAN useWithLogon;

        useWithLogon = TRUE;

        if (!Information->DomainName || !Information->UserName || !Information->Password)
            useWithLogon = FALSE;

        if (Flags & PH_CREATE_PROCESS_USE_LINKED_TOKEN)
            useWithLogon = FALSE;

        if (Flags & PH_CREATE_PROCESS_SET_SESSION_ID)
        {
            if (Information->SessionId != NtCurrentPeb()->SessionId)
                useWithLogon = FALSE;
        }

        if (Information->LogonType && Information->LogonType != LOGON32_LOGON_INTERACTIVE)
            useWithLogon = FALSE;

        if (useWithLogon)
        {
            PPH_STRING commandLine;
            PROCESS_INFORMATION processInfo;
            ULONG newFlags;

            if (Information->CommandLine) // duplicate because CreateProcess modifies the string
                commandLine = PhCreateString(Information->CommandLine);
            else
                commandLine = NULL;

            if (!Information->Environment)
                Flags |= PH_CREATE_PROCESS_UNICODE_ENVIRONMENT;

            newFlags = 0;
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
                status = NTSTATUS_FROM_WIN32(GetLastError());

            if (commandLine)
                PhDereferenceObject(commandLine);

            if (NT_SUCCESS(status))
            {
                PhpConvertProcessInformation(&processInfo, ClientId, ProcessHandle, ThreadHandle);
            }

            return status;
        }
    }

    // Get the token handle, either obtained by 
    // logging in as a user or stealing a token from 
    // another process.

    if (!Information->ProcessIdWithToken)
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
            if (WSTR_IEQUAL(Information->DomainName, L"NT AUTHORITY"))
            {
                if (WSTR_IEQUAL(Information->UserName, L"SYSTEM"))
                {
                    if (WindowsVersion >= WINDOWS_VISTA)
                        logonType = LOGON32_LOGON_SERVICE;
                    else
                        logonType = LOGON32_LOGON_NEW_CREDENTIALS; // HACK
                }

                if (
                    WSTR_IEQUAL(Information->UserName, L"LOCAL SERVICE") ||
                    WSTR_IEQUAL(Information->UserName, L"NETWORK SERVICE")
                    )
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
            return NTSTATUS_FROM_WIN32(GetLastError());
    }
    else
    {
        HANDLE processHandle;

        if (!NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            ProcessQueryAccess,
            Information->ProcessIdWithToken
            )))
            return status;

        status = PhOpenProcessToken(
            &tokenHandle,
            TOKEN_ALL_ACCESS,
            processHandle
            );
        NtClose(processHandle);

        if (!NT_SUCCESS(status))
            return status;

        if (Flags & PH_CREATE_PROCESS_SET_SESSION_ID)
            needsDuplicate = TRUE; // can't set the session ID of a token in use by a process
    }

    if (Flags & PH_CREATE_PROCESS_USE_LINKED_TOKEN)
    {
        HANDLE linkedTokenHandle;

        if (NT_SUCCESS(PhGetTokenLinkedToken(tokenHandle, &linkedTokenHandle)))
        {
            NtClose(tokenHandle);
            tokenHandle = linkedTokenHandle;
            needsDuplicate = TRUE; // returned linked token handle is an impersonation token; need to convert to primary
        }
    }

    if (needsDuplicate)
    {
        HANDLE newTokenHandle;
        OBJECT_ATTRIBUTES oa;
        SECURITY_QUALITY_OF_SERVICE securityQos;

        securityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        securityQos.ImpersonationLevel = SecurityImpersonation;
        securityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        securityQos.EffectiveOnly = FALSE;

        InitializeObjectAttributes(
            &oa,
            NULL,
            0,
            NULL,
            NULL
            );
        oa.SecurityQualityOfService = &securityQos;

        status = NtDuplicateToken(
            tokenHandle,
            TOKEN_ALL_ACCESS,
            &oa,
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

    if (!Information->Environment)
    {
        defaultEnvironment = NULL;

        if (CreateEnvironmentBlock_I)
        {
            CreateEnvironmentBlock_I(&defaultEnvironment, tokenHandle, FALSE);

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

    if (defaultEnvironment)
    {
        if (DestroyEnvironmentBlock_I)
            DestroyEnvironmentBlock_I(defaultEnvironment);
    }

    NtClose(tokenHandle);

    return status;
}

VOID PhShellExecute(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in_opt PWSTR Parameters
    )
{
    SHELLEXECUTEINFO info = { sizeof(info) };

    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.nShow = SW_SHOW;
    info.hwnd = hWnd;

    if (!ShellExecuteEx(&info))
    {
        // It already displays error messages by itself.
        //PhShowStatus(hWnd, L"Unable to execute the program", 0, GetLastError());
    }
}

BOOLEAN PhShellExecuteEx(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in_opt PWSTR Parameters,
    __in ULONG ShowWindowType,
    __in ULONG Flags,
    __in_opt ULONG Timeout,
    __out_opt PHANDLE ProcessHandle
    )
{
    SHELLEXECUTEINFO info = { sizeof(info) };

    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.nShow = ShowWindowType;
    info.hwnd = hWnd;

    if ((Flags & PH_SHELL_EXECUTE_ADMIN) && WINDOWS_HAS_UAC)
        info.lpVerb = L"runas";

    if (ShellExecuteEx(&info))
    {
        if (Timeout)
        {
            if (!(Flags & PH_SHELL_EXECUTE_PUMP_MESSAGES))
            {
                LARGE_INTEGER timeout;

                NtWaitForSingleObject(info.hProcess, FALSE, PhTimeoutFromMilliseconds(&timeout, Timeout));
            }
            else
            {
                PhWaitForMultipleObjectsAndPump(NULL, 1, &info.hProcess, Timeout);
            }
        }

        if (ProcessHandle)
            *ProcessHandle = info.hProcess;
        else
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
 * \param hWnd A handle to the parent window.
 * \param FileName A file name.
 */
VOID PhShellExploreFile(
    __in HWND hWnd,
    __in PWSTR FileName
    )
{
    if (SHOpenFolderAndSelectItems_I && SHParseDisplayName_I)
    {
        LPITEMIDLIST item;
        SFGAOF attributes;

        if (SUCCEEDED(SHParseDisplayName_I(FileName, NULL, &item, 0, &attributes)))
        {
            SHOpenFolderAndSelectItems_I(item, 0, NULL, 0);
            CoTaskMemFree(item);
        }
        else
        {
            PhShowError(hWnd, L"The location \"%s\" could not be found.", FileName);
        }
    }
    else
    {
        PPH_STRING selectFileName;

        selectFileName = PhConcatStrings2(L"/select,", FileName);
        PhShellExecute(hWnd, L"explorer.exe", selectFileName->Buffer);
        PhDereferenceObject(selectFileName);
    }
}

/**
 * Shows properties for a file.
 *
 * \param hWnd A handle to the parent window.
 * \param FileName A file name.
 */
VOID PhShellProperties(
    __in HWND hWnd,
    __in PWSTR FileName
    )
{
    SHELLEXECUTEINFO info = { sizeof(info) };

    info.lpFile = FileName;
    info.nShow = SW_SHOW;
    info.fMask = SEE_MASK_INVOKEIDLIST;
    info.lpVerb = L"properties";

    if (!ShellExecuteEx(&info))
    {
        // It already displays error messages by itself.
        //PhShowStatus(hWnd, L"Unable to execute the program", 0, GetLastError());
    }
}

/**
 * Opens a key in the Registry Editor.
 *
 * \param hWnd A handle to the parent window.
 * \param KeyName The key name to open.
 */
VOID PhShellOpenKey(
    __in HWND hWnd,
    __in PPH_STRING KeyName
    )
{
    PPH_STRING lastKey;
    PPH_STRING tempString;
    HKEY regeditKeyHandle;
    PPH_STRING regeditFileName;

    if (RegCreateKey(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit",
        &regeditKeyHandle
        ) != ERROR_SUCCESS)
        return;

    // Expand the abbreviations.
    if (PhStartsWithString2(KeyName, L"HKCU", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_CURRENT_USER", &KeyName->Buffer[4]);
    }
    else if (PhStartsWithString2(KeyName, L"HKU", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_USERS", &KeyName->Buffer[3]);
    }
    else if (PhStartsWithString2(KeyName, L"HKCR", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_CLASSES_ROOT", &KeyName->Buffer[4]);
    }
    else if (PhStartsWithString2(KeyName, L"HKLM", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_LOCAL_MACHINE", &KeyName->Buffer[4]);
    }
    else
    {
        lastKey = KeyName;
        PhReferenceObject(KeyName);
    }

    // Set the last opened key in regedit's configuration. Note that 
    // if we are on Vista, we need to prepend "Computer\".

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        tempString = PhConcatStrings2(L"Computer\\", lastKey->Buffer);
        PhDereferenceObject(lastKey);
        lastKey = tempString;
    }

    RegSetValueEx(regeditKeyHandle, L"LastKey", 0, REG_SZ, (PBYTE)lastKey->Buffer, lastKey->Length + 2);
    PhDereferenceObject(lastKey);
    RegCloseKey(regeditKeyHandle);

    // Start regedit.
    // If we aren't elevated, request that regedit be elevated. 
    // This is so we can get the consent dialog in the center of 
    // the specified window.

    regeditFileName = PhGetKnownLocation(CSIDL_WINDOWS, L"\\regedit.exe");

    if (!regeditFileName)
        regeditFileName = PhCreateString(L"regedit.exe");

    if (!PhElevated)
    {
        PhShellExecuteEx(hWnd, regeditFileName->Buffer, L"", SW_NORMAL, PH_SHELL_EXECUTE_ADMIN, 0, NULL);
    }
    else
    {
        PhShellExecute(hWnd, regeditFileName->Buffer, L"");
    }

    PhDereferenceObject(regeditFileName);
}

/**
 * Gets a registry string value.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 *
 * \return A pointer to a string containing the 
 * value. You must free the string using 
 * PhDereferenceObject() when you no longer need 
 * it.
 */
PPH_STRING PhQueryRegistryString(
    __in HKEY KeyHandle,
    __in_opt PWSTR ValueName
    )
{
    ULONG win32Result;
    PPH_STRING string = NULL;
    PVOID buffer;
    ULONG bufferSize = 0x80;
    ULONG type;

    buffer = PhAllocate(bufferSize);

    if ((win32Result = RegQueryValueEx(
        KeyHandle,
        ValueName,
        NULL,
        &type,
        buffer,
        &bufferSize
        )) == ERROR_MORE_DATA)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        win32Result = RegQueryValueEx(
            KeyHandle,
            ValueName,
            NULL,
            &type,
            buffer,
            &bufferSize
            );
    }

    if (win32Result != ERROR_SUCCESS ||
        (
        type != REG_SZ &&
        type != REG_MULTI_SZ &&
        type != REG_EXPAND_SZ
        ))
    {
        PhFree(buffer);
        SetLastError(win32Result);
        return NULL;
    }

    if (bufferSize > 2)
        string = PhCreateStringEx((PWSTR)buffer, bufferSize - 2);

    PhFree(buffer);

    return string;
}

VOID PhMapFlags1(
    __inout PULONG Value2,
    __in ULONG Value1,
    __in PPH_FLAG_MAPPING Mappings,
    __in ULONG NumberOfMappings
    )
{
    ULONG i;
    ULONG value2;

    value2 = *Value2;

    if (value2 != 0)
    {
        // There are existing flags. Map the flags 
        // we know about by clearing/setting them. The flags 
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
        // There are no existing flags, which means 
        // we can build the value from scratch, with no 
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
    __inout PULONG Value1,
    __in ULONG Value2,
    __in PPH_FLAG_MAPPING Mappings,
    __in ULONG NumberOfMappings
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
    __in HWND hdlg,
    __in UINT uiMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uiMsg)
    {
    case WM_NOTIFY:
        {
            LPOFNOTIFY header = (LPOFNOTIFY)lParam;

            if (header->hdr.code == CDN_FILEOK)
            {
                ULONG returnLength;

                returnLength = CommDlg_OpenSave_GetFilePath(
                    header->hdr.hwndFrom,
                    header->lpOFN->lpstrFile,
                    header->lpOFN->nMaxFile
                    );

                if (returnLength > header->lpOFN->nMaxFile)
                {
                    PhFree(header->lpOFN->lpstrFile);
                    header->lpOFN->nMaxFile = returnLength;
                    header->lpOFN->lpstrFile = PhAllocate(header->lpOFN->nMaxFile * 2);

                    returnLength = CommDlg_OpenSave_GetFilePath(
                        header->hdr.hwndFrom,
                        header->lpOFN->lpstrFile,
                        header->lpOFN->nMaxFile
                        );
                }

                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

OPENFILENAME *PhpCreateOpenFileName(
    __in ULONG Type
    )
{
    OPENFILENAME *ofn;

    ofn = PhAllocate(sizeof(OPENFILENAME) + sizeof(ULONG));
    memset(ofn, 0, sizeof(OPENFILENAME));
    *(PULONG)PTR_ADD_OFFSET(ofn, sizeof(OPENFILENAME)) = Type;

    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->nMaxFile = 0x100;
    ofn->lpstrFile = PhAllocate(ofn->nMaxFile * 2);
    ofn->lpstrFileTitle = NULL;
    ofn->Flags = OFN_ENABLEHOOK | OFN_EXPLORER;
    ofn->lpfnHook = PhpOpenFileNameHookProc;

    ofn->lpstrFile[0] = 0;

    return ofn;
}

VOID PhpFreeOpenFileName(
    __in OPENFILENAME *OpenFileName
    )
{
    if (OpenFileName->lpstrFilter) PhFree((PVOID)OpenFileName->lpstrFilter);
    if (OpenFileName->lpstrFile) PhFree((PVOID)OpenFileName->lpstrFile);

    PhFree(OpenFileName);
}

/**
 * Creates a file dialog for the user to select 
 * a file to open.
 *
 * \return An opaque pointer representing the file 
 * dialog. You must free the file dialog using 
 * PhFreeFileDialog() when you no longer need it.
 */
PVOID PhCreateOpenFileDialog()
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        IFileDialog *fileDialog;

        if (SUCCEEDED(CoCreateInstance(
            &CLSID_FileOpenDialog,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IFileDialog,
            &fileDialog
            )))
        {
            // The default options are fine.
            return fileDialog;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        OPENFILENAME *ofn;

        ofn = PhpCreateOpenFileName(1);
        PhSetFileDialogOptions(ofn, PH_FILEDIALOG_PATHMUSTEXIST | PH_FILEDIALOG_FILEMUSTEXIST | PH_FILEDIALOG_STRICTFILETYPES);

        return ofn;
    }
}

/**
 * Creates a file dialog for the user to select 
 * a file to save to.
 *
 * \return An opaque pointer representing the file 
 * dialog. You must free the file dialog using 
 * PhFreeFileDialog() when you no longer need it.
 */
PVOID PhCreateSaveFileDialog()
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        IFileDialog *fileDialog;

        if (SUCCEEDED(CoCreateInstance(
            &CLSID_FileSaveDialog,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IFileDialog,
            &fileDialog
            )))
        {
            // The default options are fine.
            return fileDialog;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        OPENFILENAME *ofn;

        ofn = PhpCreateOpenFileName(2);
        PhSetFileDialogOptions(ofn, PH_FILEDIALOG_PATHMUSTEXIST | PH_FILEDIALOG_OVERWRITEPROMPT | PH_FILEDIALOG_STRICTFILETYPES);

        return ofn;
    }
}

/**
 * Frees a file dialog.
 *
 * \param FileDialog The file dialog.
 */
VOID PhFreeFileDialog(
    __in PVOID FileDialog
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        IFileDialog_Release((IFileDialog *)FileDialog);
    }
    else
    {
        PhpFreeOpenFileName((OPENFILENAME *)FileDialog);
    }
}

/**
 * Shows a file dialog to the user.
 *
 * \param hWnd A handle to the parent window.
 * \param FileDialog The file dialog.
 *
 * \return TRUE if the user selected a file, FALSE if 
 * the user cancelled the operation or an error 
 * occurred.
 */
BOOLEAN PhShowFileDialog(
    __in HWND hWnd,
    __in PVOID FileDialog
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        // Set a blank default extension. This will have an effect when the user 
        // selects a different file type.
        IFileDialog_SetDefaultExtension((IFileDialog *)FileDialog, L"");

        return SUCCEEDED(IFileDialog_Show((IFileDialog *)FileDialog, hWnd));
    }
    else
    {
        OPENFILENAME *ofn = (OPENFILENAME *)FileDialog;

        ofn->hwndOwner = hWnd;

        // Determine whether the structure represents 
        // a open or save dialog and call the appropriate 
        // function.
        if (*(PULONG)PTR_ADD_OFFSET(FileDialog, sizeof(OPENFILENAME)) == 1)
        {
            return GetOpenFileName(ofn);
        }
        else
        {
            return GetSaveFileName(ofn);
        }
    }
}

static PH_FLAG_MAPPING PhpFileDialogIfdMappings[] =
{
    { PH_FILEDIALOG_CREATEPROMPT, FOS_CREATEPROMPT },
    { PH_FILEDIALOG_PATHMUSTEXIST, FOS_PATHMUSTEXIST },
    { PH_FILEDIALOG_FILEMUSTEXIST, FOS_FILEMUSTEXIST },
    { PH_FILEDIALOG_SHOWHIDDEN, FOS_FORCESHOWHIDDEN },
    { PH_FILEDIALOG_NODEREFERENCELINKS, FOS_NODEREFERENCELINKS },
    { PH_FILEDIALOG_OVERWRITEPROMPT, FOS_OVERWRITEPROMPT },
    { PH_FILEDIALOG_DEFAULTEXPANDED, FOS_DEFAULTNOMINIMODE },
    { PH_FILEDIALOG_STRICTFILETYPES, FOS_STRICTFILETYPES }
};

static PH_FLAG_MAPPING PhpFileDialogOfnMappings[] =
{
    { PH_FILEDIALOG_CREATEPROMPT, OFN_CREATEPROMPT },
    { PH_FILEDIALOG_PATHMUSTEXIST, OFN_PATHMUSTEXIST },
    { PH_FILEDIALOG_FILEMUSTEXIST, OFN_FILEMUSTEXIST },
    { PH_FILEDIALOG_SHOWHIDDEN, OFN_FORCESHOWHIDDEN },
    { PH_FILEDIALOG_NODEREFERENCELINKS, OFN_NODEREFERENCELINKS },
    { PH_FILEDIALOG_OVERWRITEPROMPT, OFN_OVERWRITEPROMPT }
};

/**
 * Gets the options for a file dialog.
 *
 * \param FileDialog The file dialog.
 *
 * \return The currently enabled options. See the 
 * documentation for PhSetFileDialogOptions() for details.
 */
ULONG PhGetFileDialogOptions(
    __in PVOID FileDialog
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        FILEOPENDIALOGOPTIONS dialogOptions;
        ULONG options;

        if (SUCCEEDED(IFileDialog_GetOptions((IFileDialog *)FileDialog, &dialogOptions)))
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
        OPENFILENAME *ofn = (OPENFILENAME *)FileDialog;
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
 * \li \c PH_FILEDIALOG_CREATEPROMPT A prompt for creation will 
 * be displayed when the selected item does not exist. This is only 
 * valid for Save dialogs.
 * \li \c PH_FILEDIALOG_PATHMUSTEXIST The selected item must be 
 * in an existing folder. This is enabled by default.
 * \li \c PH_FILEDIALOG_FILEMUSTEXIST The selected item must exist. 
 * This is enabled by default and is only valid for Open dialogs.
 * \li \c PH_FILEDIALOG_SHOWHIDDEN Items with the System and Hidden 
 * attributes will be displayed.
 * \li \c PH_FILEDIALOG_NODEREFERENCELINKS Shortcuts will not be 
 * followed, allowing .lnk files to be opened.
 * \li \c PH_FILEDIALOG_OVERWRITEPROMPT An overwrite prompt will be 
 * displayed if an existing item is selected. This is enabled by 
 * default and is only valid for Save dialogs.
 * \li \c PH_FILEDIALOG_DEFAULTEXPANDED The file dialog should be 
 * expanded by default (i.e. the folder browser should be displayed). 
 * This is only valid for Save dialogs.
 */
VOID PhSetFileDialogOptions(
    __in PVOID FileDialog,
    __in ULONG Options
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        FILEOPENDIALOGOPTIONS dialogOptions;

        if (SUCCEEDED(IFileDialog_GetOptions((IFileDialog *)FileDialog, &dialogOptions)))
        {
            PhMapFlags1(
                &dialogOptions,
                Options,
                PhpFileDialogIfdMappings,
                sizeof(PhpFileDialogIfdMappings) / sizeof(PH_FLAG_MAPPING)
                );

            IFileDialog_SetOptions((IFileDialog *)FileDialog, dialogOptions);
        }
    }
    else
    {
        OPENFILENAME *ofn = (OPENFILENAME *)FileDialog;

        PhMapFlags1(
            &ofn->Flags,
            Options,
            PhpFileDialogOfnMappings,
            sizeof(PhpFileDialogOfnMappings) / sizeof(PH_FLAG_MAPPING)
            );
    }
}

/**
 * Sets the file type filter for a file dialog.
 *
 * \param FileDialog The file dialog.
 * \param Filters A pointer to an array of file 
 * type structures.
 * \param NumberOfFilters The number of file types.
 */
VOID PhSetFileDialogFilter(
    __in PVOID FileDialog,
    __in PPH_FILETYPE_FILTER Filters,
    __in ULONG NumberOfFilters
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        IFileDialog_SetFileTypes(
            (IFileDialog *)FileDialog,
            NumberOfFilters,
            (COMDLG_FILTERSPEC *)Filters
            );
    }
    else
    {
        OPENFILENAME *ofn = (OPENFILENAME *)FileDialog;
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

        filterString = PhReferenceStringBuilderString(&filterBuilder);
        PhDeleteStringBuilder(&filterBuilder);

        if (ofn->lpstrFilter)
            PhFree((PVOID)ofn->lpstrFilter);

        ofn->lpstrFilter = PhAllocateCopy(filterString->Buffer, filterString->Length + 2);
        PhDereferenceObject(filterString);
    }
}

/**
 * Gets the file name selected in a file dialog.
 *
 * \param FileDialog The file dialog.
 *
 * \return A pointer to a string containing the 
 * file name. You must free the string using 
 * PhDereferenceObject() when you no longer need 
 * it.
 */
PPH_STRING PhGetFileDialogFileName(
    __in PVOID FileDialog
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        IShellItem *result;
        PPH_STRING fileName = NULL;

        if (SUCCEEDED(IFileDialog_GetResult((IFileDialog *)FileDialog, &result)))
        {
            PWSTR name;

            if (SUCCEEDED(IShellItem_GetDisplayName(result, SIGDN_FILESYSPATH, &name)))
            {
                fileName = PhCreateString(name);
                CoTaskMemFree(name);
            }

            IShellItem_Release(result);
        }

        if (!fileName)
        {
            PWSTR name;

            if (SUCCEEDED(IFileDialog_GetFileName((IFileDialog *)FileDialog, &name)))
            {
                fileName = PhCreateString(name);
                CoTaskMemFree(name);
            }
        }

        return fileName;
    }
    else
    {
        return PhCreateString(((OPENFILENAME *)FileDialog)->lpstrFile);
    }
}

/**
 * Sets the file name of a file dialog.
 *
 * \param FileDialog The file dialog.
 * \param FileName The new file name.
 */
VOID PhSetFileDialogFileName(
    __in PVOID FileDialog,
    __in PWSTR FileName
    )
{
    if (WINDOWS_HAS_IFILEDIALOG)
    {
        IShellItem *shellItem = NULL;
        PWSTR baseName;

        baseName = wcsrchr(FileName, '\\');

        if (baseName && SHParseDisplayName_I && SHCreateShellItem_I)
        {
            LPITEMIDLIST item;
            SFGAOF attributes;
            PPH_STRING pathName;

            // Remove the base name.
            pathName = PhCreateStringEx(FileName, (baseName - FileName) * 2);

            if (SUCCEEDED(SHParseDisplayName_I(pathName->Buffer, NULL, &item, 0, &attributes)))
            {
                SHCreateShellItem_I(NULL, NULL, item, &shellItem);
                CoTaskMemFree(item);
            }

            PhDereferenceObject(pathName);
        }

        if (shellItem)
        {
            IFileDialog_SetFolder((IFileDialog *)FileDialog, shellItem);
            IFileDialog_SetFileName((IFileDialog *)FileDialog, baseName + 1);
            IShellItem_Release(shellItem);
        }
        else
        {
            IFileDialog_SetFileName((IFileDialog *)FileDialog, FileName);
        }
    }
    else
    {
        OPENFILENAME *ofn = (OPENFILENAME *)FileDialog;

        if (wcschr(FileName, '/'))
        {
            // It refuses to take any filenames with a slash.
            return;
        }

        PhFree((PVOID)ofn->lpstrFile);
        ofn->nMaxFile = (ULONG)wcslen(FileName) + 1;
        ofn->lpstrFile = PhAllocateCopy(FileName, ofn->nMaxFile * 2);
    }
}

NTSTATUS PhIsExecutablePacked(
    __in PWSTR FileName,
    __out PBOOLEAN IsPacked,
    __out_opt PULONG NumberOfModules,
    __out_opt PULONG NumberOfFunctions
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
    // 1. The function-to-module ratio is lower than 3 
    //    (on average fewer than 3 functions are imported 
    //    from each module), and
    // 2. It references more than 2 modules but fewer than 
    //    5 modules.
    //
    // Or:
    //
    // 1. The function-to-module ratio is lower than 4 
    //    (on average fewer than 4 functions are imported 
    //    from each module), and
    // 2. It references more than 4 modules but fewer than 
    //    31 modules.
    //
    // Or:
    //
    // 1. It does not have a section named ".text".
    //
    // An image is not considered to be packed if it has only 
    // one import from a module named "mscoree.dll".

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

    status = PhLoadMappedImage(
        FileName,
        NULL,
        TRUE,
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

    status = PhInitializeMappedImageImports(
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

        if (STR_IEQUAL(importDll.Name, "mscoree.dll"))
            isModuleMscoree = TRUE;

        numberOfFunctions += importDll.NumberOfEntries;
    }

    // Determine if the image is packed.

    if (
        (
        // Rule 1
        (numberOfModules < 3 && numberOfFunctions < 5 &&
        mappedImage.NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE) ||
        // Rule 2
        ((numberOfFunctions / numberOfModules) < 3 &&
        numberOfModules > 2 && numberOfModules < 5) ||
        // Rule 3
        ((numberOfFunctions / numberOfModules) < 4 &&
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

NTSTATUS PhMd5File(
    __in PWSTR FileName,
    __out_bcount(16) PUCHAR Digest
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    IO_STATUS_BLOCK isb;

    if (NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        // Don't try to hash files over 16 MB in size.
        if (
            NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)) &&
            fileSize.QuadPart < 16 * 1024 * 1024
            )
        {
            MD5_CTX context;
            UCHAR buffer[4096];
            ULONG64 bytesRemaining;

            bytesRemaining = fileSize.QuadPart;

            MD5Init(&context);

            while (bytesRemaining)
            {
                status = NtReadFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    sizeof(buffer),
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;

                MD5Update(&context, buffer, (ULONG)isb.Information);

                bytesRemaining -= (ULONG)isb.Information;
            }

            if (status == STATUS_END_OF_FILE)
                status = STATUS_SUCCESS;

            MD5Final(&context);

            if (NT_SUCCESS(status))
                memcpy(Digest, context.digest, 16);
        }
        else
        {
            status = STATUS_FILE_TOO_LARGE;
        }

        NtClose(fileHandle);
    }

    return status;
}

PPH_STRING PhParseCommandLinePart(
    __in PPH_STRINGREF CommandLine,
    __inout PULONG Index
    )
{
    PPH_STRING string;
    PH_STRING_BUILDER stringBuilder;
    ULONG length;
    ULONG i;

    ULONG numberOfBackslashes;
    BOOLEAN inQuote;
    BOOLEAN endOfValue;

    length = CommandLine->Length / 2;
    i = *Index;

    // This function follows the rules used by CommandLineToArgvW:
    //
    // * 2n backslashes and a quotation mark produces n backslashes and a quotation mark (non-literal).
    // * 2n + 1 backslashes and a quotation mark produces n and a quotation mark (literal).
    // * n backslashes and no quotation mark produces n backslashes.

    PhInitializeStringBuilder(&stringBuilder, 10);
    numberOfBackslashes = 0;
    inQuote = FALSE;
    endOfValue = FALSE;

    for (; i < length; i++)
    {
        switch (CommandLine->Buffer[i])
        {
        case '\\':
            numberOfBackslashes++;
            break;
        case '\"':
            if (numberOfBackslashes != 0)
            {
                if (numberOfBackslashes & 1)
                {
                    numberOfBackslashes /= 2;

                    if (numberOfBackslashes != 0)
                    {
                        PhAppendCharStringBuilder2(&stringBuilder, '\\', numberOfBackslashes);
                        numberOfBackslashes = 0;
                    }

                    PhAppendCharStringBuilder(&stringBuilder, '\"');

                    break;
                }
                else
                {
                    numberOfBackslashes /= 2;
                    PhAppendCharStringBuilder2(&stringBuilder, '\\', numberOfBackslashes);
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
                PhAppendCharStringBuilder2(&stringBuilder, '\\', numberOfBackslashes);
                numberOfBackslashes = 0;
            }

            if (CommandLine->Buffer[i] == ' ' && !inQuote)
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

    string = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

    return string;
}

BOOLEAN PhParseCommandLine(
    __in PPH_STRINGREF CommandLine,
    __in_opt PPH_COMMAND_LINE_OPTION Options,
    __in ULONG NumberOfOptions,
    __in ULONG Flags,
    __in PPH_COMMAND_LINE_CALLBACK Callback,
    __in_opt PVOID Context
    )
{
    ULONG i;
    ULONG j;
    ULONG length;
    BOOLEAN cont;
    BOOLEAN wasFirst;

    PH_STRINGREF optionName;
    PPH_COMMAND_LINE_OPTION option = NULL;
    PPH_STRING optionValue;

    if (CommandLine->Length == 0)
        return TRUE;

    i = 0;
    length = CommandLine->Length / 2;
    wasFirst = TRUE;

    while (TRUE)
    {
        // Skip spaces.
        while (i < length && CommandLine->Buffer[i] == ' ')
            i++;

        if (i >= length)
            break;

        if (option &&
            (option->Type == MandatoryArgumentType ||
            (option->Type == OptionalArgumentType && CommandLine->Buffer[i] != '-')))
        {
            // Read the value and execute the callback function.

            optionValue = PhParseCommandLinePart(CommandLine, &i);
            cont = Callback(option, optionValue, Context);
            PhDereferenceObject(optionValue);

            if (!cont)
                break;

            option = NULL;
        }
        else if (CommandLine->Buffer[i] == '-')
        {
            ULONG originalIndex;
            ULONG optionNameLength;

            // Read the option (only alphanumeric characters allowed).

            // Skip the dash.
            i++;

            originalIndex = i;

            for (; i < length; i++)
            {
                if (!iswalnum(CommandLine->Buffer[i]))
                    break;
            }

            optionNameLength = i - originalIndex;

            optionName.Buffer = &CommandLine->Buffer[originalIndex];
            optionName.Length = (USHORT)(optionNameLength * 2);

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
    __in PPH_STRINGREF String
    )
{
    static WCHAR backslashAndQuote[2] = { '\\', '\"' };

    PPH_STRING string;
    PH_STRING_BUILDER stringBuilder;
    ULONG length;
    ULONG i;

    ULONG numberOfBackslashes;

    length = String->Length / 2;
    PhInitializeStringBuilder(&stringBuilder, String->Length / 2 * 3);
    numberOfBackslashes = 0;

    // Simply replacing " with \" won't work here. See PhParseCommandLinePart 
    // for the quoting rules.

    for (i = 0; i < length; i++)
    {
        switch (String->Buffer[i])
        {
        case '\\':
            numberOfBackslashes++;
            break;
        case '\"':
            if (numberOfBackslashes != 0)
            {
                PhAppendCharStringBuilder2(&stringBuilder, '\\', numberOfBackslashes * 2);
                numberOfBackslashes = 0;
            }

            PhAppendStringBuilderEx(&stringBuilder, backslashAndQuote, sizeof(backslashAndQuote));

            break;
        default:
            if (numberOfBackslashes != 0)
            {
                PhAppendCharStringBuilder2(&stringBuilder, '\\', numberOfBackslashes);
                numberOfBackslashes = 0;
            }

            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);

            break;
        }
    }

    string = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

    return string;
}

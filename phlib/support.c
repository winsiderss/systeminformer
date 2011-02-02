/*
 * Process Hacker - 
 *   general support functions
 * 
 * Copyright (C) 2009-2011 wj32
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

#define _PH_SUPPORT_PRIVATE
#include <phgui.h>
#include <guisupp.h>
#include <winsta.h>
#include <md5.h>
#include <sha.h>

// We may want to change this for debugging purposes.
#define PHP_USE_IFILEDIALOG (WINDOWS_HAS_IFILEDIALOG)

typedef BOOLEAN (NTAPI *_WinStationQueryInformationW)(
    __in_opt HANDLE ServerHandle,
    __in ULONG LogonId,
    __in WINSTATIONINFOCLASS WinStationInformationClass,
    __out_bcount(WinStationInformationLength) PVOID WinStationInformation,
    __in ULONG WinStationInformationLength,
    __out PULONG ReturnLength
    );

typedef BOOL (WINAPI *_CreateEnvironmentBlock)(
    __out LPVOID *lpEnvironment,
    __in_opt HANDLE hToken,
    __in BOOL bInherit
    );

typedef BOOL (WINAPI *_DestroyEnvironmentBlock)(
    __in LPVOID lpEnvironment
    );

DECLSPEC_SELECTANY WCHAR *PhSizeUnitNames[7] = { L"B", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB" };
DECLSPEC_SELECTANY ULONG PhMaxSizeUnit = MAXULONG32;

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

/**
 * Centers a window.
 *
 * \param WindowHandle The window to center.
 * \param ParentWindowHandle If specified, the window will be positioned 
 * at the center of this window. Otherwise, the window will be positioned 
 * at the center of the monitor.
 */
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

/**
 * References an array of objects.
 *
 * \param Objects An array of objects.
 * \param NumberOfObjects The number of elements in \a Objects.
 */
VOID PhReferenceObjects(
    __in_ecount(NumberOfObjects) PPVOID Objects,
    __in ULONG NumberOfObjects
    )
{
    ULONG i;

    for (i = 0; i < NumberOfObjects; i++)
        PhReferenceObject(Objects[i]);
}

/**
 * Dereferences an array of objects.
 *
 * \param Objects An array of objects.
 * \param NumberOfObjects The number of elements in \a Objects.
 */
VOID PhDereferenceObjects(
    __in_ecount(NumberOfObjects) PPVOID Objects,
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

/**
 * Displays a message box.
 *
 * \param hWnd The owner window of the message box.
 * \param Type The type of message box to display.
 * \param Format A format string.
 *
 * \return The user's response.
 */
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

/**
 * Displays an error message for a NTSTATUS value or Win32 error code.
 *
 * \param hWnd The owner window of the message box.
 * \param Message A message describing the operation that failed.
 * \param Status A NTSTATUS value, or 0 if there is none.
 * \param Win32Result A Win32 error code, or 0 if there is none.
 */
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

/**
 * Displays an error message for a NTSTATUS value or Win32 error code, 
 * and allows the user to cancel the current operation.
 *
 * \param hWnd The owner window of the message box.
 * \param Message A message describing the operation that failed.
 * \param Status A NTSTATUS value, or 0 if there is none.
 * \param Win32Result A Win32 error code, or 0 if there is none.
 *
 * \return TRUE if the user wishes to continue with the current operation, 
 * otherwise FALSE.
 */
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

/**
 * Displays a confirmation message.
 *
 * \param hWnd The owner window of the message box.
 * \param Verb A verb describing the operation, e.g. "terminate".
 * \param Object The object of the operation, e.g. "the process".
 * \param Message A message describing the operation.
 * \param Warning TRUE to display the confirmation message as a warning, 
 * otherwise FALSE.
 *
 * \return TRUE if the user wishes to continue, otherwise FALSE.
 */
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
        config.hInstance = PhLibImageBase;
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
        if (PhEqualStringZ(KeyValuePairs[i].Key, String, TRUE))
        {
            *Integer = (ULONG)KeyValuePairs[i].Value;
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

/**
 * Creates a random (type 4) UUID.
 *
 * \param Guid The destination UUID.
 */
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

FORCEINLINE VOID PhpReverseGuid(
    __inout PGUID Guid
    )
{
    Guid->Data1 = _byteswap_ulong(Guid->Data1);
    Guid->Data2 = _byteswap_ushort(Guid->Data2);
    Guid->Data3 = _byteswap_ushort(Guid->Data3);
}

/**
 * Creates a name-based (type 3 or 5) UUID.
 *
 * \param Guid The destination UUID.
 * \param Namespace The UUID of the namespace.
 * \param Name The input name.
 * \param NameLength The length of the input name, not 
 * including the null terminator if present.
 * \param Version The type of UUID.
 * \li \c GUID_VERSION_MD5 Creates a type 3, MD5-based UUID.
 * \li \c GUID_VERSION_SHA1 Creates a type 5, SHA1-based UUID.
 */
VOID PhGenerateGuidFromName(
    __out PGUID Guid,
    __in PGUID Namespace,
    __in PCHAR Name,
    __in ULONG NameLength,
    __in UCHAR Version
    )
{
    PGUID_EX guid;
    PUCHAR data;
    ULONG dataLength;
    GUID ns;
    UCHAR hash[20];

    // Convert the namespace to big endian.

    ns = *Namespace;
    PhpReverseGuid(&ns);

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
    PhpReverseGuid(&guid->Guid);
    guid->s2.Version = Version;
    guid->s2.Variant &= ~GUID_VARIANT_STANDARD_MASK;
    guid->s2.Variant |= GUID_VARIANT_STANDARD;
}

/**
 * Fills a buffer with random uppercase alphabetical characters.
 *
 * \param Buffer The buffer to fill with random characters, plus 
 * a null terminator.
 * \param Count The number of characters available in the buffer, 
 * including space for the null terminator.
 */
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

/**
 * Modifies a string to ensure it is within the specified length.
 *
 * \param String The input string.
 * \param DesiredCount The desired number of characters in the 
 * new string. If necessary, parts of the string are replaced with 
 * an ellipsis to indicate characters have been omitted.
 *
 * \return The new string.
 */
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

/**
 * Modifies a string to ensure it is within the specified length, 
 * parsing the string as a path.
 *
 * \param String The input string.
 * \param DesiredCount The desired number of characters in the 
 * new string. If necessary, parts of the string are replaced with 
 * an ellipsis to indicate characters have been omitted.
 *
 * \return The new string.
 */
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

FORCEINLINE BOOLEAN PhpMatchWildcards(
    __in PWSTR Pattern,
    __in PWSTR String,
    __in BOOLEAN IgnoreCase
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
        case '?':
            break;
        case '*':
            star = TRUE;
            String = s;
            Pattern = p;

            do
            {
                Pattern++;
            } while (*Pattern == '*');

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
                if (towupper(*s) != towupper(*p))
                    goto StarCheck;
            }

            break;
        }
    }

    while (*p == '*')
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
 * \param Pattern The pattern, which can contain asterisks and 
 * question marks.
 * \param String The string which the pattern is matched against.
 * \param IgnoreCase Whether to ignore character cases.
 */
BOOLEAN PhMatchWildcards(
    __in PWSTR Pattern,
    __in PWSTR String,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return PhpMatchWildcards(Pattern, String, FALSE);
    else
        return PhpMatchWildcards(Pattern, String, TRUE);
}

/**
 * Compares two strings, ignoring prefix characters (ampersands).
 *
 * \param A The first string.
 * \param B The second string.
 * \param IgnoreCase Whether to ignore character cases.
 * \param MatchIfPrefix Specify TRUE to return 0 when \a A is a 
 * prefix of \a B.
 */
LONG PhCompareUnicodeStringZIgnoreMenuPrefix(
    __in PWSTR A,
    __in PWSTR B,
    __in BOOLEAN IgnoreCase,
    __in BOOLEAN MatchIfPrefix
    )
{
    WCHAR t;

    if (!IgnoreCase)
    {
        while (TRUE)
        {
            // This takes care of double ampersands as well (they are treated as 
            // one literal ampersand).
            if (*A == '&')
                A++;
            if (*B == '&')
                B++;

            t = *A;

            if (t == 0)
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
            if (*A == '&')
                A++;
            if (*B == '&')
                B++;

            t = *A;

            if (t == 0)
            {
                if (MatchIfPrefix)
                    return 0;

                break;
            }

            if (towupper(t) != towupper(*B))
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
 * \param Format The format of the date. If NULL, the format appropriate 
 * to the user's locale is used.
 */
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

/**
 * Formats a time using the user's default locale.
 *
 * \param Time The time structure. If NULL, the current time is used.
 * \param Format The format of the time. If NULL, the format appropriate 
 * to the user's locale is used.
 */
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

/**
 * Formats a date and time using the user's default locale.
 *
 * \param DateTime The time structure. If NULL, the current time is used.
 *
 * \return A string containing the date, a space character, then the time.
 */
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

/**
 * Formats a relative time span.
 *
 * \param TimeSpan The time span, in ticks.
 */
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

/**
 * Formats a 64-bit unsigned integer.
 *
 * \param Value The integer.
 * \param GroupDigits TRUE to group digits, otherwise FALSE.
 */
PPH_STRING PhFormatUInt64(
    __in ULONG64 Value,
    __in BOOLEAN GroupDigits
    )
{
    PH_FORMAT format;

    format.Type = UInt64FormatType | (GroupDigits ? FormatGroupDigits : 0);
    format.u.UInt64 = Value;

    return PhFormat(&format, 1, 0);
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
    PH_FORMAT format;

    // PhFormat handles this better than the old method.

    format.Type = SizeFormatType | FormatUseRadix;
    format.Radix = (UCHAR)(MaxSizeUnit != -1 ? MaxSizeUnit : PhMaxSizeUnit);
    format.u.Size = Size;

    return PhFormat(&format, 1, 0);
}

/**
 * Converts a UUID to its string representation.
 *
 * \param Guid A UUID.
 */
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

/**
 * Retrieves image version information for a file.
 *
 * \param FileName The file name.
 *
 * \return A version information block. You must 
 * free this using PhFree() when you no longer need it.
 */
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

/**
 * Retrieves the language ID and code page used by a version 
 * information block.
 *
 * \param VersionInfo The version information block.
 */
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

/**
 * Retrieves a string in a version information block.
 *
 * \param VersionInfo The version information block.
 * \param SubBlock The path to the sub-block.
 */
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

/**
 * Retrieves a string in a version information block.
 *
 * \param VersionInfo The version information block.
 * \param LangCodePage The language ID and code page of the string.
 * \param StringName The name of the string.
 */
PPH_STRING PhGetFileVersionInfoString2(
    __in PVOID VersionInfo,
    __in ULONG LangCodePage,
    __in PWSTR StringName
    )
{
    WCHAR subBlock[65];
    PH_FORMAT format[4];

    PhInitFormatS(&format[0], L"\\StringFileInfo\\");
    PhInitFormatX(&format[1], LangCodePage);
    format[1].Type |= FormatPadZeros | FormatUpperCase;
    format[1].Width = 8;
    PhInitFormatC(&format[2], '\\');
    PhInitFormatS(&format[3], StringName);

    if (PhFormatToBuffer(format, 4, subBlock, sizeof(subBlock), NULL))
        return PhGetFileVersionInfoString(VersionInfo, subBlock);
    else
        return NULL;
}

/**
 * Initializes a structure with version information.
 *
 * \param ImageVersionInfo The version information structure.
 * \param FileName The file name of an image.
 */
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

/**
 * Frees a version information structure initialized by 
 * PhInitializeImageVersionInfo().
 *
 * \param ImageVersionInfo The version information structure.
 */
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

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Gets an absolute file name.
 *
 * \param FileName A file name.
 * \param IndexOfFileName A variable which receives the index of the base name.
 *
 * \return An absolute file name, or NULL if the function failed.
 */
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

/**
 * Expands environment variables in a string.
 *
 * \param String The string.
 */
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

/**
 * Gets the base name from a file name.
 *
 * \param FileName The file name.
 */
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

/**
 * Retrieves the system directory path.
 */
PPH_STRING PhGetSystemDirectory()
{
    static PPH_STRING cachedSystemDirectory = NULL;

    PPH_STRING systemDirectory;
    ULONG bufferSize;
    ULONG returnLength;

    // Use the cached value if possible.

    systemDirectory = cachedSystemDirectory;

    if (systemDirectory)
    {
        PhReferenceObject(systemDirectory);
        return systemDirectory;
    }

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

    // Try to cache the value.
    if (_InterlockedCompareExchangePointer(
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

/**
 * Retrieves the Windows directory path.
 */
PPH_STRING PhGetSystemRoot()
{
    return PhCreateString(USER_SHARED_DATA->NtSystemRoot);
}

/**
 * Locates a loader entry in the current process.
 *
 * \param DllBase The base address of the DLL. Specify NULL 
 * if this is not a search criteria.
 * \param FullDllName The full name of the DLL. Specify NULL 
 * if this is not a search criteria.
 * \param BaseDllName The base name of the DLL. Specify NULL 
 * if this is not a search criteria.
 *
 * \remarks This function must be called with the loader lock 
 * acquired. The first entry matching all of the specified 
 * values is returned.
 */
PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    __in_opt PVOID DllBase,
    __in_opt PPH_STRINGREF FullDllName,
    __in_opt PPH_STRINGREF BaseDllName
    )
{
    PLDR_DATA_TABLE_ENTRY result = NULL;
    PLDR_DATA_TABLE_ENTRY entry;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    listHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    listEntry = listHead->Flink;

    while (listEntry != listHead)
    {
        entry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (
            (!DllBase || entry->DllBase == DllBase) &&
            (!FullDllName || RtlEqualUnicodeString(&entry->FullDllName, &FullDllName->us, TRUE)) &&
            (!BaseDllName || RtlEqualUnicodeString(&entry->BaseDllName, &BaseDllName->us, TRUE))
            )
        {
            result = entry;
            break;
        }

        listEntry = listEntry->Flink;
    }

    return result;
}

/**
 * Retrieves the file name of a DLL loaded by the current process.
 *
 * \param DllHandle The base address of the DLL.
 * \param IndexOfFileName A variable which receives the index of 
 * the base name of the DLL in the returned string.
 *
 * \return The file name of the DLL, or NULL if the DLL could not 
 * be found.
 */
PPH_STRING PhGetDllFileName(
    __in PVOID DllHandle,
    __out_opt PULONG IndexOfFileName
    )
{
    PLDR_DATA_TABLE_ENTRY entry;
    PPH_STRING fileName;
    PPH_STRING newFileName;
    ULONG indexOfFileName;

    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    entry = PhFindLoaderEntry(DllHandle, NULL, NULL);

    if (entry)
        fileName = PhCreateStringEx(entry->FullDllName.Buffer, entry->FullDllName.Length);
    else
        fileName = NULL;

    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    if (!fileName)
        return NULL;

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

/**
 * Retrieves the file name of the current process image.
 */
PPH_STRING PhGetApplicationFileName()
{
    return PhGetDllFileName(NtCurrentPeb()->ImageBaseAddress, NULL);
}

/**
 * Retrieves the directory of the current process image.
 */
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
    PPH_STRING path;
    SIZE_T appendPathLength;

    if (AppendPath)
        appendPathLength = wcslen(AppendPath) * 2;
    else
        appendPathLength = 0;

    path = PhCreateStringEx(NULL, MAX_PATH * 2 + appendPathLength);

    if (SUCCEEDED(SHGetFolderPath(
        NULL,
        Folder,
        NULL,
        SHGFP_TYPE_CURRENT,
        path->Buffer
        )))
    {
        PhTrimToNullTerminatorString(path);

        if (AppendPath)
        {
            memcpy(&path->Buffer[path->Length / 2], AppendPath, appendPathLength + 2); // +2 for null terminator
            path->Length += (USHORT)appendPathLength;
        }

        return path;
    }

    PhDereferenceObject(path);

    return NULL;
}

/**
 * Waits on multiple objects while processing window messages.
 *
 * \param hWnd The window to process messages for, or NULL to 
 * process all messages for the current thread.
 * \param NumberOfHandles The number of handles specified in 
 * \a Handles. This must not be greater than MAXIMUM_WAIT_OBJECTS - 1.
 * \param Handles An array of handles.
 * \param Timeout The number of milliseconds to wait on the objects, 
 * or INFINITE for no timeout.
 *
 * \remarks The wait is always in WaitAny mode.
 */
NTSTATUS PhWaitForMultipleObjectsAndPump(
    __in_opt HWND hWnd,
    __in ULONG NumberOfHandles,
    __in PHANDLE Handles,
    __in ULONG Timeout
    )
{
    NTSTATUS status;
    ULONG startTickCount;
    ULONG currentTickCount;
    LONG currentTimeout;

    startTickCount = GetTickCount();
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
            currentTickCount = GetTickCount();
            currentTimeout = Timeout - (currentTickCount - startTickCount);

            if (currentTimeout < 0)
                return STATUS_TIMEOUT;
        }
    }
}

/**
 * Creates a native process and an initial thread.
 *
 * \param FileName The Win32 file name of the image.
 * \param CommandLine The command line string to pass to the process. 
 * This string cannot be used to specify the image to execute.
 * \param Environment The environment block for the process. Specify 
 * NULL to use the environment of the current process.
 * \param CurrentDirectory The current directory string to pass to the 
 * process.
 * \param Information Additional parameters to pass to the process.
 * \param Flags A combination of the following:
 * \li \c PH_CREATE_PROCESS_INHERIT_HANDLES Inheritable handles will be 
 * duplicated to the process from the parent process.
 * \li \c PH_CREATE_PROCESS_SUSPENDED The initial thread will be created 
 * suspended.
 * \li \c PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB The process will not be 
 * assigned to the job object associated with the parent process.
 * \li \c PH_CREATE_PROCESS_NEW_CONSOLE The process will have its own 
 * console, instead of inheriting the console of the parent process.
 * \param ParentProcessHandle The process from which the new process will 
 * inherit attributes. Specify NULL for the current process.
 * \param ClientId A variable which recieves the identifier of the initial 
 * thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcess(
    __in PWSTR FileName,
    __in_opt PPH_STRINGREF CommandLine,
    __in_opt PVOID Environment,
    __in_opt PPH_STRINGREF CurrentDirectory,
    __in_opt PPH_CREATE_PROCESS_INFO Information,
    __in ULONG Flags,
    __in_opt HANDLE ParentProcessHandle,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    )
{
    NTSTATUS status;
    RTL_USER_PROCESS_INFORMATION processInfo;
    PRTL_USER_PROCESS_PARAMETERS parameters;
    UNICODE_STRING fileName;
    PUNICODE_STRING windowTitle;
    PUNICODE_STRING desktopInfo;

    if (!RtlDosPathNameToNtPathName_U(
        FileName,
        &fileName,
        NULL,
        NULL
        ))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    if (Information)
    {
        windowTitle = (PUNICODE_STRING)Information->WindowTitle;
        desktopInfo = (PUNICODE_STRING)Information->DesktopInfo;
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
        Information ? (PUNICODE_STRING)Information->DllPath : NULL,
        (PUNICODE_STRING)CurrentDirectory,
        CommandLine ? &CommandLine->us : &fileName,
        Environment,
        windowTitle,
        desktopInfo,
        Information ? (PUNICODE_STRING)Information->ShellInfo : NULL,
        Information ? (PUNICODE_STRING)Information->RuntimeData : NULL
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

    RtlFreeHeap(RtlProcessHeap(), 0, fileName.Buffer);

    if (NT_SUCCESS(status))
    {
        if (!(Flags & PH_CREATE_PROCESS_SUSPENDED))
            NtResumeThread(processInfo.Thread, NULL);

        if (ClientId)
            *ClientId = processInfo.ClientId;

        if (ProcessHandle)
            *ProcessHandle = processInfo.Process;
        else
            NtClose(processInfo.Process);

        if (ThreadHandle)
            *ThreadHandle = processInfo.Thread;
        else
            NtClose(processInfo.Thread);
    }

    return status;
}

/**
 * Creates a Win32 process and an initial thread.
 *
 * \param FileName The Win32 file name of the image.
 * \param CommandLine The command line to execute. This can be specified 
 * instead of \a FileName to indicate the image to execute.
 * \param Environment The environment block for the process. Specify 
 * NULL to use the environment of the current process.
 * \param CurrentDirectory The current directory string to pass to the 
 * process.
 * \param Flags See PhCreateProcess().
 * \param TokenHandle The token of the process. Specify NULL for the token 
 * of the parent process.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
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

static const PH_FLAG_MAPPING PhpCreateProcessMappings[] =
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

/**
 * Creates a Win32 process and an initial thread.
 *
 * \param FileName The Win32 file name of the image.
 * \param CommandLine The command line to execute. This can be specified 
 * instead of \a FileName to indicate the image to execute.
 * \param Environment The environment block for the process. Specify 
 * NULL to use the environment of the current process.
 * \param CurrentDirectory The current directory string to pass to the 
 * process.
 * \param StartupInfo A STARTUPINFO structure containing additional 
 * parameters for the process.
 * \param Flags See PhCreateProcess().
 * \param TokenHandle The token of the process. Specify NULL for the token 
 * of the parent process.
 * \param ClientId A variable which recieves the identifier of the initial 
 * thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
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
            status = PhGetLastWin32ErrorAsNtStatus();
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
            status = PhGetLastWin32ErrorAsNtStatus();
    }

    if (commandLine)
        PhDereferenceObject(commandLine);

    if (NT_SUCCESS(status))
    {
        PhpConvertProcessInformation(&processInfo, ClientId, ProcessHandle, ThreadHandle);
    }

    return status;
}

/**
 * Creates a Win32 process and an initial thread under the specified 
 * user.
 *
 * \param Information Parameters specifying how to create the process.
 * \param Flags See PhCreateProcess(). Additional flags may be used:
 * \li \c PH_CREATE_PROCESS_USE_PROCESS_TOKEN Use the token of the process specified 
 * by \a ProcessIdWithToken in \a Information.
 * \li \c PH_CREATE_PROCESS_USE_SESSION_TOKEN Use the token of the session specified 
 * by \a SessionIdWithToken in \a Information.
 * \li \c PH_CREATE_PROCESS_USE_LINKED_TOKEN Use the linked token to create the 
 * process; this causes the process to be elevated or unelevated depending on the 
 * specified options.
 * \li \c PH_CREATE_PROCESS_SET_SESSION_ID \a SessionId is specified in \a Information.
 * \li \c PH_CREATE_PROCESS_WITH_PROFILE Load the user profile, if supported.
 * \param ClientId A variable which recieves the identifier of the initial 
 * thread.
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param ThreadHandle A variable which receives a handle to the initial thread.
 */
NTSTATUS PhCreateProcessAsUser(
    __in PPH_CREATE_PROCESS_AS_USER_INFO Information,
    __in ULONG Flags,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static _WinStationQueryInformationW WinStationQueryInformationW_I = NULL;
    static _CreateEnvironmentBlock CreateEnvironmentBlock_I = NULL;
    static _DestroyEnvironmentBlock DestroyEnvironmentBlock_I = NULL;

    NTSTATUS status;
    HANDLE tokenHandle;
    PVOID defaultEnvironment = NULL;
    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    BOOLEAN needsDuplicate = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        HMODULE winsta;
        HMODULE userEnv;

        winsta = LoadLibrary(L"winsta.dll");
        WinStationQueryInformationW_I = (_WinStationQueryInformationW)GetProcAddress(winsta, "WinStationQueryInformationW");

        userEnv = LoadLibrary(L"userenv.dll");
        CreateEnvironmentBlock_I = (_CreateEnvironmentBlock)GetProcAddress(userEnv, "CreateEnvironmentBlock");
        DestroyEnvironmentBlock_I = (_DestroyEnvironmentBlock)GetProcAddress(userEnv, "DestroyEnvironmentBlock");

        PhEndInitOnce(&initOnce);
    }

    if ((Flags & PH_CREATE_PROCESS_USE_PROCESS_TOKEN) && (Flags & PH_CREATE_PROCESS_USE_SESSION_TOKEN))
        return STATUS_INVALID_PARAMETER_2;
    if (!Information->ApplicationName && !Information->CommandLine)
        return STATUS_INVALID_PARAMETER_MIX;

    // Whenever we can, try not to set the desktop name; it breaks a lot of things.
    // Note that on XP we must set it, otherwise the program doesn't display correctly.
    if (WindowsVersion < WINDOWS_VISTA || (Information->DesktopName && !WSTR_IEQUAL(Information->DesktopName, L"WinSta0\\Default")))
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
                status = PhGetLastWin32ErrorAsNtStatus();

            if (commandLine)
                PhDereferenceObject(commandLine);

            if (NT_SUCCESS(status))
            {
                PhpConvertProcessInformation(&processInfo, ClientId, ProcessHandle, ThreadHandle);
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
    else if (Flags & PH_CREATE_PROCESS_USE_SESSION_TOKEN)
    {
        WINSTATIONUSERTOKEN userToken;
        ULONG returnLength;

        if (!WinStationQueryInformationW_I)
            return STATUS_PROCEDURE_NOT_FOUND;

        if (!WinStationQueryInformationW_I(
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
            return PhGetLastWin32ErrorAsNtStatus();
    }

    if (Flags & PH_CREATE_PROCESS_USE_LINKED_TOKEN)
    {
        HANDLE linkedTokenHandle;
        TOKEN_TYPE tokenType;
        ULONG returnLength;

        // NtQueryInformationToken normally returns an impersonation token with SecurityIdentification,
        // but if the process is running with SeTcbPrivilege, it returns a primary token. We can never 
        // duplicate a SecurityIdentification impersonation token to make it a primary token, so we just 
        // check if the token is primary before using it.

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

    if (!Information->Environment)
    {
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

NTSTATUS PhpGetAccountPrivileges(
    __in PSID AccountSid,
    __out PTOKEN_PRIVILEGES *Privileges
    )
{
    NTSTATUS status;
    LSA_HANDLE accountHandle;
    PPRIVILEGE_SET accountPrivileges;
    PTOKEN_PRIVILEGES privileges;

    status = LsaOpenAccount(PhGetLookupPolicyHandle(), AccountSid, ACCOUNT_VIEW, &accountHandle);

    if (!NT_SUCCESS(status))
        return status;

    status = LsaEnumeratePrivilegesOfAccount(accountHandle, &accountPrivileges);
    LsaClose(accountHandle);

    if (!NT_SUCCESS(status))
        return status;

    privileges = PhAllocate(FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * accountPrivileges->PrivilegeCount);
    privileges->PrivilegeCount = accountPrivileges->PrivilegeCount;
    memcpy(privileges->Privileges, accountPrivileges->Privilege, sizeof(LUID_AND_ATTRIBUTES) * accountPrivileges->PrivilegeCount);

    LsaFreeMemory(accountPrivileges);

    *Privileges = privileges;

    return status;
}

/**
 * Filters a token to create a limited user security context.
 *
 * \param TokenHandle A handle to an existing token. The handle must have 
 * TOKEN_DUPLICATE, TOKEN_QUERY, TOKEN_ADJUST_GROUPS, TOKEN_ADJUST_DEFAULT, 
 * READ_CONTROL and WRITE_DAC access.
 * \param NewTokenHandle A variable which receives a handle to the filtered 
 * token. The handle will have the same granted access as \a TokenHandle.
 */
NTSTATUS PhFilterTokenForLimitedUser(
    __in HANDLE TokenHandle,
    __out PHANDLE NewTokenHandle
    )
{
    static SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
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
    UCHAR administratorsSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
    PSID administratorsSid;
    UCHAR usersSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
    PSID usersSid;
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
    PTOKEN_USER currentUser;
    PACE_HEADER currentAce;
    ULONG newDaclLength;
    PACL newDacl;
    SECURITY_DESCRIPTOR newSecurityDescriptor;
    TOKEN_DEFAULT_DACL newDefaultDacl;
    HANDLE newTokenHandle;

    // Set up the SIDs to Disable structure.

    // Initialize the Administrators SID.
    administratorsSid = (PSID)administratorsSidBuffer;
    RtlInitializeSid(administratorsSid, &ntAuthority, 2);
    *RtlSubAuthoritySid(administratorsSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(administratorsSid, 1) = DOMAIN_ALIAS_RID_ADMINS;

    // Initialize the Users SID.
    usersSid = (PSID)usersSidBuffer;
    RtlInitializeSid(usersSid, &ntAuthority, 2);
    *RtlSubAuthoritySid(usersSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(usersSid, 1) = DOMAIN_ALIAS_RID_USERS;

    sidsToDisable = (PTOKEN_GROUPS)sidsToDisableBuffer;
    sidsToDisable->GroupCount = 1;
    sidsToDisable->Groups[0].Sid = administratorsSid;
    sidsToDisable->Groups[0].Attributes = 0;

    // Set up the Privileges to Delete structure.

    // Get the privileges that the input token contains.
    if (!NT_SUCCESS(status = PhGetTokenPrivileges(TokenHandle, &privilegesOfToken)))
        return status;

    // Get the privileges of the Users group - the privileges that we are going to allow.
    if (!NT_SUCCESS(PhpGetAccountPrivileges(usersSid, &privilegesOfUsers)))
    {
        // Unsuccessful, so use the default set of privileges.
        privilegesOfUsers = PhAllocate(FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(defaultAllowedPrivileges));
        privilegesOfUsers->PrivilegeCount = sizeof(defaultAllowedPrivileges) / sizeof(LUID_AND_ATTRIBUTES);
        memcpy(privilegesOfUsers->Privileges, defaultAllowedPrivileges, sizeof(defaultAllowedPrivileges));
    }

    // Allocate storage for the privileges we need to delete. The worst case scenario is that 
    // all privileges in the token need to be deleted.
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
    if (WINDOWS_HAS_UAC)
    {
        lowMandatoryLevelSid = (PSID)lowMandatoryLevelSidBuffer;
        RtlInitializeSid(lowMandatoryLevelSid, &mandatoryLabelAuthority, 1);
        *RtlSubAuthoritySid(lowMandatoryLevelSid, 0) = SECURITY_MANDATORY_LOW_RID;

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

            newDaclLength = sizeof(ACL) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + RtlLengthSid(currentUser->User.Sid);

            if (currentDaclPresent)
                newDaclLength += currentDacl->AclSize - sizeof(ACL);

            newDacl = PhAllocate(newDaclLength);
            RtlCreateAcl(newDacl, newDaclLength, ACL_REVISION);

            // Add the existing DACL entries.
            if (currentDaclPresent)
            {
                for (i = 0; i < currentDacl->AceCount; i++)
                {
                    if (NT_SUCCESS(RtlGetAce(currentDacl, i, &currentAce)))
                        RtlAddAce(newDacl, ACL_REVISION, MAXULONG32, currentAce, currentAce->AceSize);
                }
            }

            // Allow access for the current user.
            RtlAddAccessAllowedAce(newDacl, ACL_REVISION, GENERIC_ALL, currentUser->User.Sid);

            // Set the security descriptor of the new token.

            RtlCreateSecurityDescriptor(&newSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

            if (NT_SUCCESS(RtlSetDaclSecurityDescriptor(&newSecurityDescriptor, TRUE, newDacl, FALSE)))
                PhSetObjectSecurity(newTokenHandle, DACL_SECURITY_INFORMATION, &newSecurityDescriptor);

            // Set the default DACL.

            newDefaultDacl.DefaultDacl = newDacl;
            NtSetInformationToken(newTokenHandle, TokenDefaultDacl, &newDefaultDacl, sizeof(TOKEN_DEFAULT_DACL));

            PhFree(newDacl);

            PhFree(currentUser);
        }

        PhFree(currentSecurityDescriptor);
    }

    *NewTokenHandle = newTokenHandle;

    return STATUS_SUCCESS;
}

/**
 * Opens a file or location through the shell.
 *
 * \param hWnd The window to display user interface components on.
 * \param FileName A file name or location.
 * \param Parameters The parameters to pass to the executed application.
 */
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

/**
 * Opens a file or location through the shell.
 *
 * \param hWnd The window to display user interface components on.
 * \param FileName A file name or location.
 * \param Parameters The parameters to pass to the executed application.
 * \param ShowWindowType A value specifying how to show the application.
 * \param Flags A combination of the following:
 * \li \c PH_SHELL_EXECUTE_ADMIN Execute the application elevated.
 * \li \c PH_SHELL_EXECUTE_PUMP_MESSAGES Waits on the application while 
 * pumping messages, if \a Timeout is specified.
 * \param Timeout The number of milliseconds to wait on the application, or 0 
 * to return immediately after the application is started.
 * \param ProcessHandle A variable which receives a handle to the new process.
 */
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
    HANDLE regeditKeyHandle;
    PH_STRINGREF regeditKeyName;
    UNICODE_STRING valueName;
    PPH_STRING regeditFileName;

    PhInitializeStringRef(&regeditKeyName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit");

    if (!NT_SUCCESS(PhCreateKey(
        &regeditKeyHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &regeditKeyName,
        0,
        0,
        NULL
        )))
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

    RtlInitUnicodeString(&valueName, L"LastKey");
    NtSetValueKey(regeditKeyHandle, &valueName, 0, REG_SZ, lastKey->Buffer, lastKey->Length + 2);
    PhDereferenceObject(lastKey);
    NtClose(regeditKeyHandle);

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
 * Gets a registry value of any type.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 *
 * \return A buffer containing information about the 
 * registry value, or NULL if the function failed. You 
 * must free the buffer with PhFree() when you no longer 
 * need it.
 */
PKEY_VALUE_PARTIAL_INFORMATION PhQueryRegistryValue(
    __in HANDLE KeyHandle,
    __in_opt PWSTR ValueName
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    RtlInitUnicodeString(&valueName, ValueName);

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryValueKey(
            KeyHandle,
            &valueName,
            KeyValuePartialInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return NULL;
        }
    } while (--attempts);

    return buffer;
}

/**
 * Gets a registry string value.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 *
 * \return A pointer to a string containing the 
 * value, or NULL if the function failed. You must 
 * free the string using PhDereferenceObject() when 
 * you no longer need it.
 */
PPH_STRING PhQueryRegistryString(
    __in HANDLE KeyHandle,
    __in_opt PWSTR ValueName
    )
{
    PPH_STRING string = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;

    buffer = PhQueryRegistryValue(KeyHandle, ValueName);

    if (buffer)
    {
        if (
            buffer->Type == REG_SZ ||
            buffer->Type == REG_MULTI_SZ ||
            buffer->Type == REG_EXPAND_SZ
            )
        {
            if (buffer->DataLength >= sizeof(WCHAR))
                string = PhCreateStringEx((PWSTR)buffer->Data, buffer->DataLength - sizeof(WCHAR));
            else
                string = PhReferenceEmptyString();
        }

        PhFree(buffer);
    }

    return string;
}

VOID PhMapFlags1(
    __inout PULONG Value2,
    __in ULONG Value1,
    __in const PH_FLAG_MAPPING *Mappings,
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
    __in const PH_FLAG_MAPPING *Mappings,
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

            // We can't use CDN_FILEOK because it's not sent if the buffer is too small, 
            // defeating the entire purpose of this callback function.

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
                        header->lpOFN->lpstrFile = PhAllocate(header->lpOFN->nMaxFile * 2);

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
    __in ULONG Type
    )
{
    OPENFILENAME *ofn;

    ofn = PhAllocate(sizeof(OPENFILENAME) + sizeof(ULONG));
    memset(ofn, 0, sizeof(OPENFILENAME));
    *(PULONG)PTR_ADD_OFFSET(ofn, sizeof(OPENFILENAME)) = Type;

    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->nMaxFile = 0x400;
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
    if (PHP_USE_IFILEDIALOG)
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
    if (PHP_USE_IFILEDIALOG)
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
    if (PHP_USE_IFILEDIALOG)
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
    if (PHP_USE_IFILEDIALOG)
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

static const PH_FLAG_MAPPING PhpFileDialogIfdMappings[] =
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

static const PH_FLAG_MAPPING PhpFileDialogOfnMappings[] =
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
    if (PHP_USE_IFILEDIALOG)
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
    if (PHP_USE_IFILEDIALOG)
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
    if (PHP_USE_IFILEDIALOG)
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

        filterString = PhFinalStringBuilderString(&filterBuilder);

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
    if (PHP_USE_IFILEDIALOG)
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
    if (PHP_USE_IFILEDIALOG)
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

        if (wcschr(FileName, '/') || wcschr(FileName, '\"'))
        {
            // It refuses to take any filenames with a slash or quotation mark.
            return;
        }

        PhFree(ofn->lpstrFile);
        ofn->nMaxFile = max((ULONG)wcslen(FileName) + 1, 0x400);
        ofn->lpstrFile = PhAllocateCopy(FileName, ofn->nMaxFile * 2);
    }
}

/**
 * Determines if an executable image is packed.
 *
 * \param FileName The file name of the image.
 * \param IsPacked A variable that receives TRUE if the image is packed,
 * otherwise FALSE.
 * \param NumberOfModules A variable that receives the number of DLLs that 
 * the image imports functions from.
 * \param NumberOfFunctions A variable that receives the number of functions 
 * that the image imports.
 */
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
 */
VOID PhInitializeHash(
    __out PPH_HASH_CONTEXT Context,
    __in PH_HASH_ALGORITHM Algorithm
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
    __inout PPH_HASH_CONTEXT Context,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
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
        Context->Context[0] = ph_crc32(Context->Context[0], (PUCHAR)Buffer, Length);
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
 * \param ReturnLength A variable which receives the required size of 
 * the buffer, in bytes.
 */
BOOLEAN PhFinalHash(
    __inout PPH_HASH_CONTEXT Context,
    __out_bcount(HashLength) PVOID Hash,
    __in ULONG HashLength,
    __out_opt PULONG ReturnLength
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
    default:
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    if (ReturnLength)
        *ReturnLength = returnLength;

    return result;
}

PPH_STRING PhParseCommandLinePart(
    __in PPH_STRINGREF CommandLine,
    __inout PULONG Index
    )
{
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

    return PhFinalStringBuilderString(&stringBuilder);
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

    return PhFinalStringBuilderString(&stringBuilder);
}

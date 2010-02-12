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

WCHAR *PhSizeUnitNames[] = { L"B", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB" };
ULONG PhMaxSizeUnit = MAXULONG32;

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

VOID PhCenterRectangle(
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
    )
{
    Rectangle->Left = Bounds->Left + (Bounds->Width - Rectangle->Width) / 2;
    Rectangle->Top = Bounds->Top + (Bounds->Height - Rectangle->Height) / 2;
}

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

PPH_STRING PhGetMessage(
    __in HANDLE DllHandle,
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

PPH_STRING PhGetNtMessage(
    __in NTSTATUS Status
    )
{
    PPH_STRING message;

    message = PhGetMessage(GetModuleHandle(L"ntdll.dll"), 0xb, GetUserDefaultLangID(), (ULONG)Status);

    if (!message)
        return NULL;
    if (message->Length == 0)
        return message;

    // Fix those messages which are formatted like:
    // {Asdf}\r\nAsdf asdf asdf...
    if (message->Buffer[0] == '{')
    {
        ULONG indexOfNewLine = PhStringIndexOfChar(message, 0, '\n');

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

    return MessageBox(hWnd, message, PH_APP_NAME, Type);
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
            Status != STATUS_ACCESS_DENIED &&
            Status != STATUS_ACCESS_VIOLATION
            )
        {
            return PhGetNtMessage(Status);
        }
        else
        {
            return PhGetWin32Message(RtlNtStatusToDosError(Status));
        }
    }
    else
    {
        return PhGetWin32Message(Win32Result);
    }
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

PPH_STRING PhFormatDate(
    __in_opt PSYSTEMTIME Date,
    __in_opt PWSTR Format
    )
{
    PPH_STRING string = NULL;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = GetDateFormat(
        LOCALE_USER_DEFAULT,
        0,
        Date,
        Format,
        NULL,
        0
        );
    buffer = PhAllocate(bufferSize * 2);

    if (GetDateFormat(
        LOCALE_USER_DEFAULT,
        0,
        Date,
        Format,
        buffer,
        bufferSize
        ))
    {
        string = PhCreateString(buffer);
    }

    PhFree(buffer);

    return string;
}

PPH_STRING PhFormatTime(
    __in_opt PSYSTEMTIME Time,
    __in_opt PWSTR Format
    )
{
    PPH_STRING string = NULL;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = GetTimeFormat(
        LOCALE_USER_DEFAULT,
        0,
        Time,
        Format,
        NULL,
        0
        );
    buffer = PhAllocate(bufferSize * 2);

    if (GetTimeFormat(
        LOCALE_USER_DEFAULT,
        0,
        Time,
        Format,
        buffer,
        bufferSize
        ))
    {
        string = PhCreateString(buffer);
    }

    PhFree(buffer);

    return string;
}

PPH_STRING PhFormatDecimal(
    __in PWSTR Value,
    __in ULONG FractionalDigits,
    __in BOOLEAN GroupDigits
    )
{
    PPH_STRING string = NULL;
    NUMBERFMT format;
    ULONG bufferSize;
    PVOID buffer;
    WCHAR decimalSeparator[4];
    WCHAR thousandSeparator[4];

    format.NumDigits = FractionalDigits;
    format.LeadingZero = 0;
    format.Grouping = GroupDigits ? 3 : 0;
    format.lpDecimalSep = decimalSeparator;
    format.lpThousandSep = thousandSeparator;
    format.NegativeOrder = 1;

    if (!GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_SDECIMAL,
        decimalSeparator,
        4
        ))
        return NULL;

    if (!GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_STHOUSAND,
        thousandSeparator,
        4
        ))
        return NULL;

    bufferSize = GetNumberFormat(
        LOCALE_USER_DEFAULT,
        0,
        Value,
        &format,
        NULL,
        0
        );
    buffer = PhAllocate(bufferSize * 2);

    if (GetNumberFormat(
        LOCALE_USER_DEFAULT,
        0,
        Value,
        &format,
        buffer,
        bufferSize
        ))
    {
        string = PhCreateString(buffer);
    }

    PhFree(buffer);

    return string;
}

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
        PPH_STRING numberString;
        PPH_STRING formattedString;
        PPH_STRING processedString;
        PPH_STRING outputString;

        numberString = PhFormatString(L"%f", s);
        formattedString = PhFormatDecimal(numberString->Buffer, 2, TRUE);
        PhDereferenceObject(numberString);

        if (!formattedString)
        {
            return PhFormatString(L"%.2g %s", s, PhSizeUnitNames[i]);
        }

        if (
            PhStringEndsWith2(formattedString, L"00", FALSE) &&
            formattedString->Length >= 6
            )
        {
            // Remove the last three characters.
            processedString = PhSubstring(
                formattedString,
                0,
                formattedString->Length / 2 - 3
                );
            PhDereferenceObject(formattedString);
        }
        else if (PhStringEndsWith2(formattedString, L"0", FALSE))
        {
            // Remove the last character.
            processedString = PhSubstring(
                formattedString,
                0,
                formattedString->Length / 2 - 1
                );
            PhDereferenceObject(formattedString);
        }
        else
        {
            processedString = formattedString;
        }

        outputString = PhConcatStrings(3, processedString->Buffer, L" ", PhSizeUnitNames[i]);
        PhDereferenceObject(processedString);

        return outputString;
    }
}

PPH_STRING PhFormatGuid(
    __in PGUID Guid
    )
{
    PVOID buffer;
    PPH_STRING string = NULL;

    buffer = PhAllocate(50 * 2);

    if (StringFromGUID2(Guid, buffer, 50))
    {
        string = PhCreateString(buffer);
        PhLowerString(string);
    }

    PhFree(buffer);

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
        return PhCreateStringEx((PWSTR)buffer, length * sizeof(WCHAR));
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

PPH_STRING PhGetFullPath(
    __in PWSTR FileName,
    __out_opt PULONG IndexOfFileName
    )
{
    PPH_STRING fullPath;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    PWSTR filePart;

    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize * 2);

    returnLength = GetFullPathName(FileName, bufferSize, buffer, &filePart);

    if (returnLength > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize * 2);

        returnLength = GetFullPathName(FileName, bufferSize, buffer, &filePart);
    }

    if (returnLength == 0)
    {
        PhFree(buffer);
        return NULL;
    }

    fullPath = PhCreateString(buffer);

    if (IndexOfFileName)
    {
        *IndexOfFileName = (ULONG)(filePart - (PWSTR)buffer);
    }

    PhFree(buffer);

    return fullPath;
}

PPH_STRING PhExpandEnvironmentStrings(
    __in PWSTR String
    )
{
    PPH_STRING string;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize * 2);

    returnLength = ExpandEnvironmentStrings(String, buffer, bufferSize);

    if (returnLength > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize * 2);

        returnLength = ExpandEnvironmentStrings(String, buffer, bufferSize);
    }

    if (returnLength == 0)
    {
        PhFree(buffer);
        return NULL;
    }

    string = PhCreateString(buffer);
    PhFree(buffer);

    return string;
}

PPH_STRING PhGetBaseName(
    __in PPH_STRING FileName
    )
{
    ULONG lastIndexOfBackslash;

    lastIndexOfBackslash = PhStringLastIndexOfChar(FileName, 0, '\\');

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
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x40;
    buffer = PhAllocate(bufferSize * 2);

    returnLength = GetSystemDirectory(buffer, bufferSize);

    if (returnLength > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize * 2);

        returnLength = GetSystemDirectory(buffer, bufferSize);
    }

    if (returnLength == 0)
    {
        PhFree(buffer);
        return NULL;
    }

    systemDirectory = PhCreateString(buffer);
    PhFree(buffer);

    return systemDirectory;
}

PPH_STRING PhGetSystemRoot()
{
    return PhCreateString(USER_SHARED_DATA->NtSystemRoot);
}

PPH_STRING PhGetApplicationModuleFileName(
    __in HMODULE ModuleHandle,
    __out_opt PULONG IndexOfFileName
    )
{
    PPH_STRING fileName;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x40;
    buffer = PhAllocate(bufferSize * 2);

    while (TRUE)
    {
        returnLength = GetModuleFileName(ModuleHandle, buffer, bufferSize);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            PhFree(buffer);
            bufferSize *= 2;
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (returnLength == 0)
    {
        PhFree(buffer);
        return NULL;
    }

    fileName = PhGetFullPath((PWSTR)buffer, IndexOfFileName);

    if (!fileName)
    {
        ULONG indexOfFileName;

        fileName = PhCreateString((PWSTR)buffer);
        indexOfFileName = PhStringLastIndexOfChar((PWSTR)buffer, 0, '\\');

        if (indexOfFileName != -1)
            indexOfFileName++;
        else
            indexOfFileName = 0;

        *IndexOfFileName = indexOfFileName;
    }

    PhFree(buffer);

    return fileName;
}

PPH_STRING PhGetApplicationFileName()
{
    return PhGetApplicationModuleFileName(GetModuleHandle(NULL), NULL);
}

PPH_STRING PhGetApplicationDirectory()
{
    PPH_STRING fileName;
    ULONG indexOfFileName;
    PPH_STRING path = NULL;

    fileName = PhGetApplicationModuleFileName(GetModuleHandle(NULL), &indexOfFileName);

    if (fileName)
    {
        // Remove the file name from the path.
        path = PhSubstring(fileName, 0, indexOfFileName);
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

VOID PhShellExecute(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in PWSTR Parameters
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
    __in PWSTR Parameters,
    __in ULONG ShowWindowType,
    __in BOOLEAN StartAsAdmin,
    __in_opt ULONG Timeout
    )
{
    SHELLEXECUTEINFO info = { sizeof(info) };

    info.lpFile = FileName;
    info.lpParameters = Parameters;
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.nShow = ShowWindowType;
    info.hwnd = hWnd;

    if (StartAsAdmin)
        info.lpVerb = L"runas";

    if (ShellExecuteEx(&info))
    {
        if (Timeout)
            WaitForSingleObject(info.hProcess, Timeout);

        CloseHandle(info.hProcess);

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
    PPH_STRING selectFileName;

    selectFileName = PhConcatStrings2(L"/select,", FileName);
    PhShellExecute(hWnd, L"explorer.exe", selectFileName->Buffer);
    PhDereferenceObject(selectFileName);
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
    if (PhStringStartsWith2(KeyName, L"HKCU", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_CURRENT_USER", &KeyName->Buffer[4]);
    }
    else if (PhStringStartsWith2(KeyName, L"HKU", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_USERS", &KeyName->Buffer[3]);
    }
    else if (PhStringStartsWith2(KeyName, L"HKCR", TRUE))
    {
        lastKey = PhConcatStrings2(L"HKEY_CLASSES_ROOT", &KeyName->Buffer[4]);
    }
    else if (PhStringStartsWith2(KeyName, L"HKLM", TRUE))
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
        PhShellExecuteEx(hWnd, regeditFileName->Buffer, L"", SW_NORMAL, TRUE, 0);
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
            return fileDialog;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return PhpCreateOpenFileName(1); 
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
            return fileDialog;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return PhpCreateOpenFileName(2); 
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
        PPH_STRING_BUILDER filterBuilder;
        PPH_STRING filterString;
        ULONG i;

        filterBuilder = PhCreateStringBuilder(10);

        for (i = 0; i < NumberOfFilters; i++)
        {
            PhStringBuilderAppend2(filterBuilder, Filters[i].Name);
            PhStringBuilderAppendEx(filterBuilder, L"\0", 2);
            PhStringBuilderAppend2(filterBuilder, Filters[i].Filter);
            PhStringBuilderAppendEx(filterBuilder, L"\0", 2);
        }

        filterString = PhReferenceStringBuilderString(filterBuilder);
        PhDereferenceObject(filterBuilder);

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
        IFileDialog_SetFileName((IFileDialog *)FileDialog, FileName);
    }
    else
    {
        OPENFILENAME *ofn = (OPENFILENAME *)FileDialog;

        PhFree((PVOID)ofn->lpstrFile);
        ofn->lpstrFile = PhAllocateCopy(FileName, ((ULONG)wcslen(FileName) + 1) * 2);
    }
}

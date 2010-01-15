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

#include <phgui.h>

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

VOID PhShowStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    PPH_STRING statusMessage;

    if (!Win32Result)
    {
        // In some cases we want the simple Win32 messages.
        if (
            Status != STATUS_ACCESS_DENIED &&
            Status != STATUS_ACCESS_VIOLATION
            )
        {
            statusMessage = PhGetNtMessage(Status);
        }
        else
        {
            statusMessage = PhGetWin32Message(RtlNtStatusToDosError(Status));
        }
    }
    else
    {
        statusMessage = PhGetWin32Message(Win32Result);
    }

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

PPH_STRING PhGetBaseName(
    __in PPH_STRING FileName
    )
{
    ULONG lastIndexOfBackslash;

    lastIndexOfBackslash = PhStringLastIndexOfChar(FileName, 0, '\\');

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

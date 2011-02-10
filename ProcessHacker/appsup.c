/*
 * Process Hacker - 
 *   application support functions
 * 
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <settings.h>
#include "mxml/mxml.h"
#include <winsta.h>
#include <dbghelp.h>

/**
 * Determines whether a process is suspended.
 *
 * \param Process The SYSTEM_PROCESS_INFORMATION structure 
 * of the process.
 */
BOOLEAN PhGetProcessIsSuspended(
    __in PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ULONG i;

    for (i = 0; i < Process->NumberOfThreads; i++)
    {
        if (
            Process->Threads[i].ThreadState != Waiting ||
            Process->Threads[i].WaitReason != Suspended
            )
            return FALSE;
    }

    return Process->NumberOfThreads != 0;
}

/**
 * Determines the type of a process based on its image file name.
 *
 * \param ProcessHandle A handle to a process.
 * \param KnownProcessType A variable which receives the process 
 * type.
 */
NTSTATUS PhGetProcessKnownType(
    __in HANDLE ProcessHandle,
    __out PH_KNOWN_PROCESS_TYPE *KnownProcessType
    )
{
    NTSTATUS status;
    PH_KNOWN_PROCESS_TYPE knownProcessType;
    PROCESS_BASIC_INFORMATION basicInfo;
    PH_STRINGREF systemRootPrefix;
    PPH_STRING fileName;
    PPH_STRING newFileName;
    PH_STRINGREF name;
#ifdef _M_X64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetProcessBasicInformation(
        ProcessHandle,
        &basicInfo
        )))
        return status;

    if (basicInfo.UniqueProcessId == SYSTEM_PROCESS_ID)
    {
        *KnownProcessType = SystemProcessType;
        return STATUS_SUCCESS;
    }

    PhInitializeStringRef(&systemRootPrefix, USER_SHARED_DATA->NtSystemRoot);

    // Make sure the system root string doesn't have a trailing backslash.
    if (systemRootPrefix.Buffer[systemRootPrefix.Length / 2 - 1] == '\\')
        systemRootPrefix.Length -= 2;

    if (!NT_SUCCESS(status = PhGetProcessImageFileName(
        ProcessHandle,
        &fileName
        )))
    {
        return status;
    }

    newFileName = PhGetFileName(fileName);
    PhDereferenceObject(fileName);
    name = newFileName->sr;

    knownProcessType = UnknownProcessType;

    if (PhStartsWithStringRef(&name, &systemRootPrefix, TRUE))
    {
        // Skip the system root, and we now have three cases:
        // 1. \\xyz.exe - Windows executable.
        // 2. \\System32\\xyz.exe - system32 executable.
        // 3. \\SysWow64\\xyz.exe - system32 executable + WOW64.
        name.Buffer += systemRootPrefix.Length / 2;
        name.Length -= systemRootPrefix.Length;

        if (PhEqualStringRef2(&name, L"\\explorer.exe", TRUE))
        {
            knownProcessType = ExplorerProcessType;
        }
        else if (
            PhStartsWithStringRef2(&name, L"\\System32", TRUE)
#ifdef _M_X64
            || (PhStartsWithStringRef2(&name, L"\\SysWow64", TRUE) && (isWow64 = TRUE, TRUE)) // ugly but necessary
#endif
            )
        {
            // SysTem32 and SysWow64 are both 8 characters long.
            name.Buffer += 9;
            name.Length -= 9 * 2;

            if (FALSE)
                ; // Dummy
            else if (PhEqualStringRef2(&name, L"\\smss.exe", TRUE))
                knownProcessType = SessionManagerProcessType;
            else if (PhEqualStringRef2(&name, L"\\csrss.exe", TRUE))
                knownProcessType = WindowsSubsystemProcessType;
            else if (PhEqualStringRef2(&name, L"\\wininit.exe", TRUE))
                knownProcessType = WindowsStartupProcessType;
            else if (PhEqualStringRef2(&name, L"\\services.exe", TRUE))
                knownProcessType = ServiceControlManagerProcessType;
            else if (PhEqualStringRef2(&name, L"\\lsass.exe", TRUE))
                knownProcessType = LocalSecurityAuthorityProcessType;
            else if (PhEqualStringRef2(&name, L"\\lsm.exe", TRUE))
                knownProcessType = LocalSessionManagerProcessType;
            else if (PhEqualStringRef2(&name, L"\\winlogon.exe", TRUE))
                knownProcessType = WindowsLogonProcessType;
            else if (PhEqualStringRef2(&name, L"\\svchost.exe", TRUE))
                knownProcessType = ServiceHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\rundll32.exe", TRUE))
                knownProcessType = RunDllAsAppProcessType;
            else if (PhEqualStringRef2(&name, L"\\dllhost.exe", TRUE))
                knownProcessType = ComSurrogateProcessType;
            else if (PhEqualStringRef2(&name, L"\\taskeng.exe", TRUE))
                knownProcessType = TaskHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\taskhost.exe", TRUE))
                knownProcessType = TaskHostProcessType;
        }
    }

    PhDereferenceObject(newFileName);

#ifdef _M_X64
    if (isWow64)
        knownProcessType |= KnownProcessWow64;
#endif

    *KnownProcessType = knownProcessType;

    return status;
}

static BOOLEAN NTAPI PhpSvchostCommandLineCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in_opt PVOID Context
    )
{
    PPH_KNOWN_PROCESS_COMMAND_LINE knownCommandLine = Context;

    if (Option && Option->Id == 1)
    {
        PhSwapReference(&knownCommandLine->ServiceHost.GroupName, Value);
    }

    return TRUE;
}

BOOLEAN PhaGetProcessKnownCommandLine(
    __in PPH_STRING CommandLine,
    __in PH_KNOWN_PROCESS_TYPE KnownProcessType,
    __out PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    )
{
    switch (KnownProcessType & KnownProcessTypeMask)
    {
    case ServiceHostProcessType:
        {
            // svchost.exe -k <GroupName>

            static PH_COMMAND_LINE_OPTION options[] =
            {
                { 1, L"k", MandatoryArgumentType }
            };

            KnownCommandLine->ServiceHost.GroupName = NULL;

            PhParseCommandLine(
                &CommandLine->sr,
                options,
                sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
                PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS,
                PhpSvchostCommandLineCallback,
                KnownCommandLine
                );

            if (KnownCommandLine->ServiceHost.GroupName)
            {
                PhaDereferenceObject(KnownCommandLine->ServiceHost.GroupName);
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        break;
    case RunDllAsAppProcessType:
        {
            // rundll32.exe <DllName>,<ProcedureName> ...

            ULONG i;
            ULONG lastIndexOfComma;
            PPH_STRING dllName;
            PPH_STRING procedureName;

            i = 0;

            // Get the rundll32.exe part.

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PhDereferenceObject(dllName);

            // Get the DLL name part.

            while (i < (ULONG)CommandLine->Length / 2 && CommandLine->Buffer[i] == ' ')
                i++;

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PhaDereferenceObject(dllName);

            // The procedure name begins after the last comma.

            lastIndexOfComma = PhFindLastCharInString(dllName, 0, ',');

            if (lastIndexOfComma == -1)
                return FALSE;

            procedureName = PhaSubstring(
                dllName,
                lastIndexOfComma + 1,
                dllName->Length / 2 - lastIndexOfComma - 1
                );
            dllName = PhaSubstring(dllName, 0, lastIndexOfComma);

            // If the DLL name isn't an absolute path, assume it's in system32.
            // TODO: Use a proper search function.

            if (RtlDetermineDosPathNameType_U(dllName->Buffer) == RtlPathTypeRelative)
            {
                dllName = PhaConcatStrings(
                    3,
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetSystemDirectory()))->Buffer,
                    L"\\",
                    dllName->Buffer
                    );
            }

            KnownCommandLine->RunDllAsApp.FileName = dllName;
            KnownCommandLine->RunDllAsApp.ProcedureName = procedureName;
        }
        break;
    case ComSurrogateProcessType:
        {
            // dllhost.exe /processid:<Guid>

            static PH_STRINGREF inprocServer32Name = PH_STRINGREF_INIT(L"InprocServer32");

            ULONG i;
            ULONG indexOfProcessId;
            PPH_STRING argPart;
            PPH_STRING guidString;
            GUID guid;
            HANDLE clsidKeyHandle;
            HANDLE inprocServer32KeyHandle;
            PPH_STRING fileName;

            i = 0;

            // Get the dllhost.exe part.

            argPart = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!argPart)
                return FALSE;

            PhDereferenceObject(argPart);

            // Get the argument part.

            while (i < (ULONG)CommandLine->Length / 2 && CommandLine->Buffer[i] == ' ')
                i++;

            argPart = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!argPart)
                return FALSE;

            PhaDereferenceObject(argPart);

            // Find "/processid:"; the GUID is just after that.

            PhUpperString(argPart);
            indexOfProcessId = PhFindStringInString(argPart, 0, L"/PROCESSID:");

            if (indexOfProcessId == -1)
                return FALSE;

            guidString = PhaSubstring(
                argPart,
                indexOfProcessId + 11,
                argPart->Length / 2 - indexOfProcessId - 11
                );

            if (!NT_SUCCESS(RtlGUIDFromString(
                &guidString->us,
                &guid
                )))
                return FALSE;

            KnownCommandLine->ComSurrogate.Guid = guid;
            KnownCommandLine->ComSurrogate.Name = NULL;
            KnownCommandLine->ComSurrogate.FileName = NULL;

            // Lookup the GUID in the registry to determine the name and file name.

            if (NT_SUCCESS(PhOpenKey(
                &clsidKeyHandle,
                KEY_READ,
                PH_KEY_CLASSES_ROOT,
                &PhaConcatStrings2(L"CLSID\\", guidString->Buffer)->sr,
                0
                )))
            {
                KnownCommandLine->ComSurrogate.Name =
                    PHA_DEREFERENCE(PhQueryRegistryString(clsidKeyHandle, NULL));

                if (NT_SUCCESS(PhOpenKey(
                    &inprocServer32KeyHandle,
                    KEY_READ,
                    clsidKeyHandle,
                    &inprocServer32Name,
                    0
                    )))
                {
                    KnownCommandLine->ComSurrogate.FileName =
                        PHA_DEREFERENCE(PhQueryRegistryString(inprocServer32KeyHandle, NULL));

                    if (fileName = PHA_DEREFERENCE(PhExpandEnvironmentStrings(
                        &KnownCommandLine->ComSurrogate.FileName->sr
                        )))
                    {
                        KnownCommandLine->ComSurrogate.FileName = fileName;
                    }

                    NtClose(inprocServer32KeyHandle);
                }

                NtClose(clsidKeyHandle);
            }
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

PPH_STRING PhEscapeStringForDelimiter(
    __in PPH_STRING String,
    __in WCHAR Delimiter
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG length;
    ULONG i;
    WCHAR temp[2];

    length = String->Length / 2;
    PhInitializeStringBuilder(&stringBuilder, String->Length / 2 * 3);

    temp[0] = '\\';

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == '\\' || String->Buffer[i] == Delimiter)
        {
            temp[1] = String->Buffer[i];
            PhAppendStringBuilderEx(&stringBuilder, temp, 4);
        }
        else
        {
            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PhUnescapeStringForDelimiter(
    __in PPH_STRING String,
    __in WCHAR Delimiter
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG length;
    ULONG i;

    length = String->Length / 2;
    PhInitializeStringBuilder(&stringBuilder, String->Length / 2 * 3);

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == '\\')
        {
            if (i != length - 1)
            {
                PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i + 1]);
                i++;
            }
            else
            {
                // Trailing backslash. Just ignore it.
                break;
            }
        }
        else
        {
            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PhGetOpaqueXmlNodeText(
    __in mxml_node_t *node
    )
{
    if (node->child && node->child->type == MXML_OPAQUE && node->child->value.opaque)
    {
        return PhCreateStringFromAnsi(node->child->value.opaque);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

VOID PhSearchOnlineString(
    __in HWND hWnd,
    __in PWSTR String
    )
{
    PhShellExecuteUserString(hWnd, L"SearchEngine", String, TRUE, NULL);
}

VOID PhShellExecuteUserString(
    __in HWND hWnd,
    __in PWSTR Setting,
    __in PWSTR String,
    __in BOOLEAN UseShellExecute,
    __in_opt PWSTR ErrorMessage
    )
{
    PPH_STRING executeString = PhGetStringSetting(Setting);
    ULONG indexOfReplacement;
    PPH_STRING stringBefore;
    PPH_STRING stringAfter;
    PPH_STRING newString;
    PPH_STRING ntMessage;

    // Make sure the user executable string is absolute.
    if (RtlDetermineDosPathNameType_U(executeString->Buffer) == RtlPathTypeRelative)
    {
        newString = PhConcatStringRef2(&PhApplicationDirectory->sr, &executeString->sr);
        PhDereferenceObject(executeString);
        executeString = newString;
    }

    indexOfReplacement = PhFindStringInString(executeString, 0, L"%s");

    if (indexOfReplacement != -1)
    {
        // Replace "%s" with the string.

        stringBefore = PhSubstring(executeString, 0, indexOfReplacement);
        stringAfter = PhSubstring(
            executeString,
            indexOfReplacement + 2,
            executeString->Length / 2 - indexOfReplacement - 2
            );

        newString = PhConcatStrings(
            3,
            stringBefore->Buffer,
            String,
            stringAfter->Buffer
            );

        if (UseShellExecute)
        {
            PhShellExecute(hWnd, newString->Buffer, NULL);
        }
        else
        {
            NTSTATUS status;

            status = PhCreateProcessWin32(NULL, newString->Buffer, NULL, NULL, 0, NULL, NULL, NULL);

            if (!NT_SUCCESS(status))
            {
                if (ErrorMessage)
                {
                    ntMessage = PhGetNtMessage(status);
                    PhShowError(hWnd, L"Unable to execute the command: %s\n%s", PhGetStringOrDefault(ntMessage, L"An unknown error occurred."), ErrorMessage);
                    PhDereferenceObject(ntMessage);
                }
                else
                {
                    PhShowStatus(hWnd, L"Unable to execute the command", status, 0);
                }
            }
        }

        PhDereferenceObject(newString);
        PhDereferenceObject(stringAfter);
        PhDereferenceObject(stringBefore);
    }

    PhDereferenceObject(executeString);
}

VOID PhLoadSymbolProviderOptions(
    __inout PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    PPH_STRING searchPath;

    PhSetOptionsSymbolProvider(
        SYMOPT_UNDNAME,
        PhGetIntegerSetting(L"DbgHelpUndecorate") ? SYMOPT_UNDNAME : 0
        );

    searchPath = PhGetStringSetting(L"DbgHelpSearchPath");

    if (searchPath->Length != 0)
        PhSetSearchPathSymbolProvider(SymbolProvider, searchPath->Buffer);

    PhDereferenceObject(searchPath);
}

VOID PhSetExtendedListViewWithSettings(
    __in HWND hWnd
    )
{
    PhSetExtendedListView(hWnd);
    ExtendedListView_EnableState(hWnd, TRUE); // enable item state storage and state highlighting
    ExtendedListView_SetNewColor(hWnd, PhCsColorNew);
    ExtendedListView_SetRemovingColor(hWnd, PhCsColorRemoved);
    ExtendedListView_SetHighlightingDuration(hWnd, PhCsHighlightingDuration);
}

PWSTR PhMakeContextAtom()
{
    PH_DEFINE_MAKE_ATOM(L"PH2_Context");
}

/**
 * Copies a string into a NMLVGETINFOTIP structure.
 *
 * \param GetInfoTip The NMLVGETINFOTIP structure.
 * \param Tip The string to copy.
 *
 * \remarks The text is truncated if it is too long.
 */
VOID PhCopyListViewInfoTip(
    __inout LPNMLVGETINFOTIP GetInfoTip,
    __in PPH_STRINGREF Tip
    )
{
    ULONG copyIndex;
    ULONG bufferRemaining;
    ULONG copyLength;

    if (GetInfoTip->dwFlags == 0)
    {
        copyIndex = (ULONG)wcslen(GetInfoTip->pszText) + 1; // plus one for newline

        if (GetInfoTip->cchTextMax - copyIndex < 2) // need at least two bytes
            return;

        bufferRemaining = GetInfoTip->cchTextMax - copyIndex - 1;
        GetInfoTip->pszText[copyIndex - 1] = '\n';
    }
    else
    {
        copyIndex = 0;
        bufferRemaining = GetInfoTip->cchTextMax;
    }

    copyLength = min((ULONG)Tip->Length / 2, bufferRemaining - 1);
    memcpy(
        &GetInfoTip->pszText[copyIndex],
        Tip->Buffer,
        copyLength * 2
        );
    GetInfoTip->pszText[copyIndex + copyLength] = 0;
}

VOID PhCopyListView(
    __in HWND ListViewHandle
    )
{
    PPH_FULL_STRING text;

    text = PhGetListViewText(ListViewHandle);
    PhSetClipboardStringEx(ListViewHandle, text->Buffer, text->Length);
    PhDereferenceObject(text);
}

VOID PhHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    )
{
    if (((LPNMHDR)lParam)->hwndFrom == ListViewHandle)
    {
        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;

        switch (keyDown->wVKey)
        {
        case 'C':
            if (GetKeyState(VK_CONTROL) < 0)
                PhCopyListView(ListViewHandle);
            break;
        }
    }
}

BOOLEAN PhGetListViewContextMenuPoint(
    __in HWND ListViewHandle,
    __out PPOINT Point
    )
{
    INT selectedIndex;
    RECT bounds;

    // The user pressed a key to display the context menu. 
    // Suggest where the context menu should display.

    if (selectedIndex = ListView_GetNextItem(ListViewHandle, -1, LVNI_SELECTED))
    {
        if (ListView_GetItemRect(ListViewHandle, selectedIndex, &bounds, LVIR_BOUNDS))
        {
            Point->x = bounds.left + PhSmallIconSize.X / 2;
            Point->y = bounds.top + PhSmallIconSize.Y / 2;
            ClientToScreen(ListViewHandle, Point);

            return TRUE;
        }
    }

    return FALSE;
}

VOID PhLoadWindowPlacementFromSetting(
    __in_opt PWSTR PositionSettingName,
    __in_opt PWSTR SizeSettingName,
    __in HWND WindowHandle
    )
{
    PH_RECTANGLE windowRectangle;

    if (PositionSettingName && SizeSettingName)
    {
        RECT rectForAdjust;

        windowRectangle.Position = PhGetIntegerPairSetting(PositionSettingName);
        windowRectangle.Size = PhGetIntegerPairSetting(SizeSettingName);

        PhAdjustRectangleToWorkingArea(
            WindowHandle,
            &windowRectangle
            );

        // Let the window adjust the minimum size if needed.
        rectForAdjust = PhRectangleToRect(windowRectangle);
        SendMessage(WindowHandle, WM_SIZING, WMSZ_BOTTOMRIGHT, (LPARAM)&rectForAdjust);
        windowRectangle = PhRectToRectangle(rectForAdjust);

        MoveWindow(WindowHandle, windowRectangle.Left, windowRectangle.Top,
            windowRectangle.Width, windowRectangle.Height, FALSE);
    }
    else
    {
        PH_INTEGER_PAIR position;
        PH_INTEGER_PAIR size;
        ULONG flags;

        flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER;

        if (PositionSettingName)
        {
            position = PhGetIntegerPairSetting(PositionSettingName);
            flags &= ~SWP_NOMOVE;
        }
        else
        {
            position.X = 0;
            position.Y = 0;
        }

        if (SizeSettingName)
        {
            size = PhGetIntegerPairSetting(SizeSettingName);
            flags &= ~SWP_NOSIZE;
        }
        else
        {
            size.X = 16;
            size.Y = 16;
        }

        SetWindowPos(WindowHandle, NULL, position.X, position.Y, size.X, size.Y, flags);
    }
}

VOID PhSaveWindowPlacementToSetting(
    __in_opt PWSTR PositionSettingName,
    __in_opt PWSTR SizeSettingName,
    __in HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };
    PH_RECTANGLE windowRectangle;

    GetWindowPlacement(WindowHandle, &placement);
    windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

    if (PositionSettingName)
        PhSetIntegerPairSetting(PositionSettingName, windowRectangle.Position);
    if (SizeSettingName)
        PhSetIntegerPairSetting(SizeSettingName, windowRectangle.Size);
}

VOID PhLoadListViewColumnsFromSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(Name);
    PhLoadListViewColumnSettings(ListViewHandle, string);
    PhDereferenceObject(string);
}

VOID PhSaveListViewColumnsToSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhSaveListViewColumnSettings(ListViewHandle);
    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}

VOID PhLoadTreeListColumnsFromSetting(
    __in PWSTR Name,
    __in HWND TreeListHandle
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(Name);
    PhLoadTreeListColumnSettings(TreeListHandle, string);
    PhDereferenceObject(string);
}

VOID PhSaveTreeListColumnsToSetting(
    __in PWSTR Name,
    __in HWND TreeListHandle
    )
{
    PPH_STRING string;

    string = PhSaveTreeListColumnSettings(TreeListHandle);
    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}

PPH_STRING PhGetPhVersion()
{
    PPH_STRING version = NULL;
    PH_IMAGE_VERSION_INFO versionInfo;

    if (PhInitializeImageVersionInfo(&versionInfo, PhApplicationFileName->Buffer))
    {
        PhReferenceObject(versionInfo.FileVersion);
        version = versionInfo.FileVersion;
        PhDeleteImageVersionInfo(&versionInfo);
    }

    return version;
}

VOID PhWritePhTextHeader(
    __inout PPH_FILE_STREAM FileStream
    )
{
    PPH_STRING version;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING dateString;
    PPH_STRING timeString;

    PhWriteStringAsAnsiFileStream2(FileStream, L"Process Hacker ");

    if (version = PhGetPhVersion())
    {
        PhWriteStringAsAnsiFileStream(FileStream, &version->sr);
        PhDereferenceObject(version);
    }

    PhWriteStringFormatFileStream(FileStream, L"\r\nWindows NT %u.%u", PhOsVersion.dwMajorVersion, PhOsVersion.dwMinorVersion);

    if (PhOsVersion.szCSDVersion[0] != 0)
        PhWriteStringFormatFileStream(FileStream, L" %s", PhOsVersion.szCSDVersion);

#ifdef _M_IX86
    PhWriteStringAsAnsiFileStream2(FileStream, L" (32-bit)");
#else
    PhWriteStringAsAnsiFileStream2(FileStream, L" (64-bit)");
#endif

    PhQuerySystemTime(&time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    dateString = PhFormatDate(&systemTime, NULL);
    timeString = PhFormatTime(&systemTime, NULL);
    PhWriteStringFormatFileStream(FileStream, L"\r\n%s %s\r\n\r\n", dateString->Buffer, timeString->Buffer);
    PhDereferenceObject(dateString);
    PhDereferenceObject(timeString);
}

BOOLEAN PhShellProcessHacker(
    __in HWND hWnd,
    __in_opt PWSTR Parameters,
    __in ULONG ShowWindowType,
    __in ULONG Flags,
    __in BOOLEAN PropagateParameters,
    __in_opt ULONG Timeout,
    __out_opt PHANDLE ProcessHandle
    )
{
    BOOLEAN result;
    PH_STRING_BUILDER sb;
    PWSTR parameters;
    PPH_STRING temp;

    if (PropagateParameters)
    {
        PhInitializeStringBuilder(&sb, 128);

        if (Parameters)
            PhAppendStringBuilder2(&sb, Parameters);

        // Propagate parameters.

        if (PhStartupParameters.NoSettings)
        {
            PhAppendStringBuilder2(&sb, L" -nosettings");
        }
        else if (PhStartupParameters.SettingsFileName && PhSettingsFileName)
        {
            PhAppendStringBuilder2(&sb, L" -settings \"");
            temp = PhEscapeCommandLinePart(&PhSettingsFileName->sr);
            PhAppendStringBuilder(&sb, temp);
            PhDereferenceObject(temp);
            PhAppendCharStringBuilder(&sb, '\"');
        }

        if (PhStartupParameters.NoKph)
        {
            PhAppendStringBuilder2(&sb, L" -nokph");
        }

        parameters = sb.String->Buffer;
    }
    else
    {
        parameters = Parameters;
    }

    result = PhShellExecuteEx(
        hWnd,
        PhApplicationFileName->Buffer,
        parameters,
        ShowWindowType,
        Flags,
        Timeout,
        ProcessHandle
        );

    if (PropagateParameters)
        PhDeleteStringBuilder(&sb);

    return result;
}

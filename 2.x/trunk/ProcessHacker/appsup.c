/*
 * Process Hacker - 
 *   application support functions
 * 
 * Copyright (C) 2010-2011 wj32
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
#include <cpysave.h>
#include <phappres.h>
#include <emenu.h>
#include "mxml/mxml.h"
#include <winsta.h>
#include <dbghelp.h>

GUID XP_CONTEXT_GUID = { 0xbeb1b341, 0x6837, 0x4c83, { 0x83, 0x66, 0x2b, 0x45, 0x1e, 0x7c, 0xe6, 0x9b } };
GUID VISTA_CONTEXT_GUID = { 0xe2011457, 0x1546, 0x43c5, { 0xa5, 0xfe, 0x00, 0x8d, 0xee, 0xe3, 0xd3, 0xf0 } };
GUID WIN7_CONTEXT_GUID = { 0x35138b9a, 0x5d96, 0x4fbd, { 0x8e, 0x2d, 0xa2, 0x44, 0x02, 0x25, 0xf9, 0x3a } };

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
 * Determines the OS compatibility context of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Guid A variable which receives a GUID identifying an 
 * operating system version.
 */
NTSTATUS PhGetProcessSwitchContext(
    __in HANDLE ProcessHandle,
    __out PGUID Guid
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
#ifdef _M_X64
    PVOID peb32;
    ULONG contextData32;
#endif
    PVOID contextData;

    // Reverse-engineered from WdcGetProcessSwitchContext (wdc.dll).

#ifdef _M_X64
    if (NT_SUCCESS(PhGetProcessPeb32(ProcessHandle, &peb32)) && peb32)
    {
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, pContextData)),
            &contextData32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        contextData = UlongToPtr(contextData32);
    }
    else
    {
#endif
        if (!NT_SUCCESS(status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, pContextData)),
            &contextData,
            sizeof(PVOID),
            NULL
            )))
            return status;
#ifdef _M_X64
    }
#endif

    if (!contextData)
        return STATUS_UNSUCCESSFUL; // no compatibility context data

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(contextData, 32), // Magic value from WdcGetProcessSwitchContext
        Guid,
        sizeof(GUID),
        NULL
        )))
        return status;

    return STATUS_SUCCESS;
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
    static PH_STRINGREF replacementToken = PH_STRINGREF_INIT(L"%s");

    PPH_STRING executeString;
    PH_STRINGREF stringBefore;
    PH_STRINGREF stringMiddle;
    PH_STRINGREF stringAfter;
    PPH_STRING newString;
    PPH_STRING ntMessage;

    executeString = PhGetStringSetting(Setting);

    // Make sure the user executable string is absolute.
    // We can't use RtlDetermineDosPathNameType_U here because the string 
    // may be a URL.
    if (PhFindCharInString(executeString, 0, ':') == -1)
    {
        newString = PhConcatStringRef2(&PhApplicationDirectory->sr, &executeString->sr);
        PhDereferenceObject(executeString);
        executeString = newString;
    }

    // Replace "%s" with the string, or use the original string if "%s" is not present.
    if (PhSplitStringRefAtString(&executeString->sr, &replacementToken, FALSE, &stringBefore, &stringAfter))
    {
        PhInitializeStringRef(&stringMiddle, String);
        newString = PhConcatStringRef3(&stringBefore, &stringMiddle, &stringAfter);
    }
    else
    {
        newString = executeString;
        PhReferenceObject(newString);
    }

    PhDereferenceObject(executeString);

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
    RECT clientRect;

    // The user pressed a key to display the context menu. 
    // Suggest where the context menu should display.

    if ((selectedIndex = ListView_GetNextItem(ListViewHandle, -1, LVNI_SELECTED)) != -1)
    {
        if (ListView_GetItemRect(ListViewHandle, selectedIndex, &bounds, LVIR_BOUNDS))
        {
            Point->x = bounds.left + PhSmallIconSize.X / 2;
            Point->y = bounds.top + PhSmallIconSize.Y / 2;

            GetClientRect(ListViewHandle, &clientRect);

            if (Point->x < 0 || Point->y < 0 || Point->x >= clientRect.right || Point->y >= clientRect.bottom)
            {
                // The menu is going to be outside of the control. Just put it at the top-left.
                Point->x = 0;
                Point->y = 0;
            }

            ClientToScreen(ListViewHandle, Point);

            return TRUE;
        }
    }

    Point->x = 0;
    Point->y = 0;
    ClientToScreen(ListViewHandle, Point);

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
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

    GetWindowPlacement(WindowHandle, &placement);
    windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

    // The rectangle is in workspace coordinates. Convert the values back to screen coordinates.
    if (GetMonitorInfo(MonitorFromRect(&placement.rcNormalPosition, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
    {
        windowRectangle.Left += monitorInfo.rcWork.left;
        windowRectangle.Top += monitorInfo.rcWork.top;
    }

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

PPH_STRING PhGetPhVersion()
{
    PH_FORMAT format[3];

    PhInitFormatU(&format[0], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[1], '.');
    PhInitFormatU(&format[2], PHAPP_VERSION_MINOR);

    return PhFormat(format, 3, 16);
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

        if (PhStartupParameters.NoPlugins)
        {
            PhAppendStringBuilder2(&sb, L" -noplugins");
        }

        if (PhStartupParameters.NewInstance)
        {
            PhAppendStringBuilder2(&sb, L" -newinstance");
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

VOID PhInitializeTreeNewColumnMenu(
    __inout PPH_TN_COLUMN_MENU_DATA Data
    )
{
    PPH_EMENU_ITEM sizeColumnToFitMenuItem;
    PPH_EMENU_ITEM sizeAllColumnsToFitMenuItem;
    PPH_EMENU_ITEM hideColumnMenuItem;
    PPH_EMENU_ITEM chooseColumnsMenuItem;

    Data->Menu = PhCreateEMenu();
    Data->Selection = NULL;
    Data->ProcessedId = 0;

    sizeColumnToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID, L"Size Column to Fit", NULL, NULL);
    sizeAllColumnsToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID, L"Size All Columns to Fit", NULL, NULL);
    hideColumnMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_HIDE_COLUMN_ID, L"Hide Column", NULL, NULL);
    chooseColumnsMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID, L"Choose Columns...", NULL, NULL);

    PhInsertEMenuItem(Data->Menu, sizeColumnToFitMenuItem, -1);
    PhInsertEMenuItem(Data->Menu, sizeAllColumnsToFitMenuItem, -1);
    PhInsertEMenuItem(Data->Menu, hideColumnMenuItem, -1);
    PhInsertEMenuItem(Data->Menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, L"", NULL, NULL), -1);
    PhInsertEMenuItem(Data->Menu, chooseColumnsMenuItem, -1);

    if (!Data->MouseEvent || !Data->MouseEvent->Column)
    {
        sizeColumnToFitMenuItem->Flags |= PH_EMENU_DISABLED;
    }

    if (!Data->MouseEvent || !Data->MouseEvent->Column ||
        Data->MouseEvent->Column->Fixed || // don't allow the fixed column to be hidden
        TreeNew_GetVisibleColumnCount(Data->TreeNewHandle) < 2 // don't allow the user to remove all columns
        )
    {
        hideColumnMenuItem->Flags |= PH_EMENU_DISABLED;
    }
}

VOID PhpEnsureValidSortColumnTreeNew(
    __inout HWND TreeNewHandle,
    __in ULONG DefaultSortColumn,
    __in PH_SORT_ORDER DefaultSortOrder
    )
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    // Make sure the column we're sorting by is actually visible, and if not, don't sort anymore.

    TreeNew_GetSort(TreeNewHandle, &sortColumn, &sortOrder);

    if (sortOrder != NoSortOrder)
    {
        PH_TREENEW_COLUMN column;

        TreeNew_GetColumn(TreeNewHandle, sortColumn, &column);

        if (!column.Visible)
        {
            if (DefaultSortOrder != NoSortOrder)
            {
                // Make sure the default sort column is visible.
                TreeNew_GetColumn(TreeNewHandle, DefaultSortColumn, &column);

                if (!column.Visible)
                {
                    ULONG maxId;
                    ULONG id;
                    BOOLEAN found;

                    // Use the first visible column.
                    maxId = TreeNew_GetMaxId(TreeNewHandle);
                    id = 0;
                    found = FALSE;

                    while (id <= maxId)
                    {
                        if (TreeNew_GetColumn(TreeNewHandle, id, &column))
                        {
                            if (column.Visible)
                            {
                                DefaultSortColumn = id;
                                found = TRUE;
                                break;
                            }
                        }

                        id++;
                    }

                    if (!found)
                    {
                        DefaultSortColumn = 0;
                        DefaultSortOrder = NoSortOrder;
                    }
                }
            }

            TreeNew_SetSort(TreeNewHandle, DefaultSortColumn, DefaultSortOrder);
        }
    }
}

BOOLEAN PhHandleTreeNewColumnMenu(
    __inout PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (!Data->Selection)
        return FALSE;

    switch (Data->Selection->Id)
    {
    case PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID:
        {
            if (Data->MouseEvent && Data->MouseEvent->Column)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, Data->MouseEvent->Column->Id);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID:
        {
            ULONG maxId;
            ULONG id;

            maxId = TreeNew_GetMaxId(Data->TreeNewHandle);
            id = 0;

            while (id <= maxId)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, id);
                id++;
            }
        }
        break;
    case PH_TN_COLUMN_MENU_HIDE_COLUMN_ID:
        {
            PH_TREENEW_COLUMN column;

            if (Data->MouseEvent && Data->MouseEvent->Column && !Data->MouseEvent->Column->Fixed)
            {
                column.Id = Data->MouseEvent->Column->Id;
                column.Visible = FALSE;
                TreeNew_SetColumn(Data->TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &column);
                PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
                InvalidateRect(Data->TreeNewHandle, NULL, FALSE);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID:
        {
            PhShowChooseColumnsDialog(Data->TreeNewHandle, Data->TreeNewHandle, PH_CONTROL_TYPE_TREE_NEW);
            PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    default:
        return FALSE;
    }

    Data->ProcessedId = Data->Selection->Id;

    return TRUE;
}

VOID PhDeleteTreeNewColumnMenu(
    __in PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (Data->Menu)
    {
        PhDestroyEMenu(Data->Menu);
        Data->Menu = NULL;
    }
}

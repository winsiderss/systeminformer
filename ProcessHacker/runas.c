/*
 * Process Hacker - 
 *   run as dialog
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
#include <shlwapi.h>
#include <wtsapi32.h>
#include <windowsx.h>

typedef BOOL (WINAPI *_CreateEnvironmentBlock)(
    __out LPVOID *lpEnvironment,
    __in_opt HANDLE hToken,
    __in BOOL bInherit
    );

typedef BOOL (WINAPI *_DestroyEnvironmentBlock)(
    __in LPVOID lpEnvironment
    );

typedef struct _RUNAS_DIALOG_CONTEXT
{
    HANDLE ProcessId;
    PPH_LIST SessionIdList;
} RUNAS_DIALOG_CONTEXT, *PRUNAS_DIALOG_CONTEXT;

typedef struct _RUNAS_SERVICE_PARAMETERS
{
    ULONG ProcessId;
    PPH_STRING UserName;
    PPH_STRING Password;
    ULONG LogonType;
    ULONG SessionId;
    PPH_STRING CurrentDirectory;
    PPH_STRING CommandLine;
    PPH_STRING FileName;
    PPH_STRING ErrorMailslot;
} RUNAS_SERVICE_PARAMETERS, *PRUNAS_SERVICE_PARAMETERS;

INT_PTR CALLBACK PhpRunAsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhSetDesktopWinStaAccess();

NTSTATUS PhRunAsCommandStart(
    __in PWSTR ServiceCommandLine,
    __in PWSTR ServiceName
    );

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR PhpLogonTypePairs[] =
{
    SIP(L"Batch", LOGON32_LOGON_BATCH),
    SIP(L"Interactive", LOGON32_LOGON_INTERACTIVE),
    SIP(L"Network", LOGON32_LOGON_NETWORK),
    SIP(L"New credentials", LOGON32_LOGON_NEW_CREDENTIALS),
    SIP(L"Service", LOGON32_LOGON_SERVICE)
};

static _CreateEnvironmentBlock CreateEnvironmentBlock_I = NULL;
static _DestroyEnvironmentBlock DestroyEnvironmentBlock_I = NULL;

static RUNAS_SERVICE_PARAMETERS RunAsServiceParameters;

VOID PhShowRunAsDialog(
    __in HWND ParentWindowHandle,
    __in_opt HANDLE ProcessId
    )
{
    RUNAS_DIALOG_CONTEXT context;

    context.ProcessId = ProcessId;
    context.SessionIdList = NULL;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_RUNAS),
        ParentWindowHandle,
        PhpRunAsDlgProc,
        (LPARAM)&context
        );
}

static BOOLEAN NTAPI PhpRunAsEnumAccountsCallback(
    __in PSID Sid,
    __in PVOID Context
    )
{
    PPH_STRING name;
    SID_NAME_USE nameUse;

    name = PhGetSidFullName(Sid, TRUE, &nameUse);

    if (name)
    {
        if (nameUse == SidTypeUser)
            ComboBox_AddString((HWND)Context, name->Buffer);

        PhDereferenceObject(name);
    }

    return TRUE;
}

static BOOLEAN IsServiceAccount(
    __in PPH_STRING UserName
    )
{
    if (
        PhStringEquals2(UserName, L"NT AUTHORITY\\LOCAL SERVICE", TRUE) || 
        PhStringEquals2(UserName, L"NT AUTHORITY\\NETWORK SERVICE", TRUE) ||
        PhStringEquals2(UserName, L"NT AUTHORITY\\SYSTEM", TRUE)
        )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

INT_PTR CALLBACK PhpRunAsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PRUNAS_DIALOG_CONTEXT context;

    if (uMsg != WM_INITDIALOG)
    {
        context = (PRUNAS_DIALOG_CONTEXT)GetProp(hwndDlg, L"Context");
    }
    else
    {
        context = (PRUNAS_DIALOG_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND typeComboBoxHandle = GetDlgItem(hwndDlg, IDC_TYPE);
            HWND userNameComboBoxHandle = GetDlgItem(hwndDlg, IDC_USERNAME);
            LSA_HANDLE policyHandle;
            ULONG sessionId;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            SHAutoComplete_I(
                GetDlgItem(hwndDlg, IDC_PROGRAM), 
                SHACF_AUTOAPPEND_FORCE_ON | SHACF_AUTOSUGGEST_FORCE_ON | SHACF_FILESYS_ONLY
                );

            ComboBox_AddString(typeComboBoxHandle, L"Batch");
            ComboBox_AddString(typeComboBoxHandle, L"Interactive");
            ComboBox_AddString(typeComboBoxHandle, L"Network");
            ComboBox_AddString(typeComboBoxHandle, L"New credentials");
            ComboBox_AddString(typeComboBoxHandle, L"Service");
            ComboBox_SelectString(typeComboBoxHandle, -1, L"Interactive");

            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\SYSTEM");
            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\LOCAL SERVICE");
            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\NETWORK SERVICE");

            if (NT_SUCCESS(PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
            {
                PhEnumAccounts(policyHandle, PhpRunAsEnumAccountsCallback, (PVOID)GetDlgItem(hwndDlg, IDC_USERNAME));
                LsaClose(policyHandle);
            }

            if (NT_SUCCESS(PhGetProcessSessionId(NtCurrentProcess(), &sessionId)))
                SetDlgItemInt(hwndDlg, IDC_SESSIONID, sessionId, FALSE); 

            SetDlgItemText(hwndDlg, IDC_PROGRAM,
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStringSetting(L"RunAsProgram")))->Buffer);
            SetDlgItemText(hwndDlg, IDC_USERNAME,
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStringSetting(L"RunAsUserName")))->Buffer);

            // Fire the user name changed event so we can fix the logon type.
            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_USERNAME, CBN_EDITCHANGE), 0);

            SetFocus(GetDlgItem(hwndDlg, IDC_PROGRAM));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_PROGRAM), 0, -1);

            if (!PhElevated)
                SendMessage(GetDlgItem(hwndDlg, IDOK), BCM_SETSHIELD, 0, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            if (context->SessionIdList)
                PhDereferenceObject(context->SessionIdList);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    NTSTATUS status;
                    PPH_STRING program;
                    PPH_STRING userName;
                    PPH_STRING password;
                    PPH_STRING logonTypeString;
                    ULONG logonType;

                    program = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_PROGRAM);
                    userName = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_USERNAME);
                    logonTypeString = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_TYPE);

                    if (!IsServiceAccount(userName))
                        password = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_PASSWORD);
                    else
                        password = NULL;

                    if (PhFindIntegerSiKeyValuePairs(
                        PhpLogonTypePairs,
                        sizeof(PhpLogonTypePairs),
                        logonTypeString->Buffer,
                        &logonType
                        ))
                    {
                        status = PhRunAsCommandStart2(
                            hwndDlg,
                            program->Buffer,
                            userName->Buffer,
                            PhGetStringOrEmpty(password),
                            logonType,
                            context->ProcessId,
                            GetDlgItemInt(hwndDlg, IDC_SESSIONID, NULL, FALSE)
                            );
                    }
                    else
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }

                    if (!NT_SUCCESS(status))
                    {
                        if (status != STATUS_CANCELLED)
                            PhShowStatus(hwndDlg, L"Unable to start the program", status, 0);
                    }
                    else if (status != STATUS_TIMEOUT)
                    {
                        PhSetStringSetting2(L"RunAsProgram", &program->sr);
                        PhSetStringSetting2(L"RunAsUserName", &userName->sr);
                        EndDialog(hwndDlg, IDOK);
                    }
                }
                break;
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Programs (*.exe;*.pif;*.com;*.bat)", L"*.exe;*.pif;*.com;*.bat" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                    PhSetFileDialogFileName(fileDialog, PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_PROGRAM)->Buffer);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        PPH_STRING fileName;

                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_PROGRAM, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_USERNAME:
                {
                    PPH_STRING userName = NULL;

                    if (!context->ProcessId && HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        userName = PHA_DEREFERENCE(PhGetComboBoxString(GetDlgItem(hwndDlg, IDC_USERNAME), -1));
                    }
                    else if (!context->ProcessId && (
                        HIWORD(wParam) == CBN_EDITCHANGE ||
                        HIWORD(wParam) == CBN_CLOSEUP
                        ))
                    {
                        userName = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_USERNAME);
                    }

                    if (userName)
                    {
                        if (IsServiceAccount(userName))
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), FALSE);

                            // Hack for Windows XP
                            if (
                                PhStringEquals2(userName, L"NT AUTHORITY\\SYSTEM", TRUE) &&
                                WindowsVersion <= WINDOWS_XP
                                )
                            { 
                                ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_TYPE), -1, L"New credentials");
                            }
                            else
                            {
                                ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_TYPE), -1, L"Service");
                            }
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
                            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_TYPE), -1, L"Interactive");
                        }
                    }
                }
                break;
            case IDC_SESSIONS:
                {
                    HMENU sessionsMenu;
                    PWTS_SESSION_INFO sessions;
                    ULONG numberOfSessions;
                    ULONG i;
                    RECT buttonRect;
                    POINT point;
                    UINT selectedItem;

                    sessionsMenu = CreatePopupMenu();

                    if (WTSEnumerateSessions(
                        WTS_CURRENT_SERVER_HANDLE,
                        0,
                        1,
                        &sessions,
                        &numberOfSessions
                        ))
                    {
                        if (!context->SessionIdList)
                            context->SessionIdList = PhCreateList(numberOfSessions);
                        else
                            PhClearList(context->SessionIdList);

                        for (i = 0; i < numberOfSessions; i++)
                        {
                            PPH_STRING domainName;
                            PPH_STRING userName;
                            PPH_STRING menuString;

                            domainName = PHA_DEREFERENCE(PhGetSessionInformationString(
                                WTS_CURRENT_SERVER_HANDLE,
                                sessions[i].SessionId,
                                WTSDomainName
                                ));
                            userName = PHA_DEREFERENCE(PhGetSessionInformationString(
                                WTS_CURRENT_SERVER_HANDLE,
                                sessions[i].SessionId,
                                WTSUserName
                                ));

                            if (
                                !PhIsStringNullOrEmpty(userName) &&
                                sessions[i].pWinStationName &&
                                !WSTR_EQUAL(sessions[i].pWinStationName, L"")
                                )
                            {
                                menuString = PhFormatString(
                                    L"%u: %s (%s\\%s)",
                                    sessions[i].SessionId,
                                    sessions[i].pWinStationName,
                                    PhGetStringOrEmpty(domainName),
                                    userName->Buffer
                                    );
                            }
                            else if (
                                sessions[i].pWinStationName &&
                                !WSTR_EQUAL(sessions[i].pWinStationName, L"")
                                )
                            {
                                menuString = PhFormatString(
                                    L"%u: %s",
                                    sessions[i].SessionId,
                                    sessions[i].pWinStationName
                                    );
                            }
                            else
                            {
                                menuString = PhFormatString(L"%u", sessions[i].SessionId);
                            }

                            AppendMenu(sessionsMenu, MF_STRING, 1 + i, menuString->Buffer);
                            PhDereferenceObject(menuString);

                            PhAddListItem(context->SessionIdList, (PVOID)sessions[i].SessionId);
                        }

                        WTSFreeMemory(sessions);

                        GetClientRect(GetDlgItem(hwndDlg, IDC_SESSIONS), &buttonRect);
                        point.x = buttonRect.right;
                        point.y = 0;

                        selectedItem = PhShowContextMenu2(
                            hwndDlg,
                            GetDlgItem(hwndDlg, IDC_SESSIONS),
                            sessionsMenu,
                            point
                            );

                        if (selectedItem != 0)
                        {
                            SetDlgItemInt(
                                hwndDlg,
                                IDC_SESSIONID,
                                (ULONG)context->SessionIdList->Items[selectedItem - 1],
                                FALSE
                                );
                        }

                        DestroyMenu(sessionsMenu);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

/**
 * Sets the access control lists of the current window station 
 * and desktop to allow all access.
 */
VOID PhSetDesktopWinStaAccess()
{
    HWINSTA wsHandle;
    HDESK desktopHandle;
    SECURITY_DESCRIPTOR securityDescriptor;

    // Create a security descriptor with a NULL DACL, 
    // thereby allowing everyone to access the object.
    InitializeSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION); 

    if (wsHandle = OpenWindowStation(
        L"WinSta0",
        FALSE,
        WRITE_DAC
        ))
    {
        PhSetObjectSecurity(wsHandle, DACL_SECURITY_INFORMATION, &securityDescriptor);
        CloseWindowStation(wsHandle);
    }

    if (desktopHandle = OpenDesktop(
        L"Default",
        0,
        FALSE,
        WRITE_DAC | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS
        ))
    {
        PhSetObjectSecurity(desktopHandle, DACL_SECURITY_INFORMATION, &securityDescriptor);
        CloseDesktop(desktopHandle);
    }
}

/**
 * Escapes a string C-style for use in a command line.
 *
 * \param String The string to escape.
 *
 * \return The escaped string.
 *
 * \remarks Only backslash (\\) and quote characters (", ') are 
 * escaped.
 */
PPH_STRING PhpCEscapeString(
    __in PPH_STRINGREF String
    )
{
    PPH_STRING_BUILDER stringBuilder;
    PPH_STRING string;
    ULONG length;
    ULONG i;
    WCHAR temp[2];

    length = String->Length / 2;
    stringBuilder = PhCreateStringBuilder(String->Length / 3);

    temp[0] = '\\';

    for (i = 0; i < length; i++)
    {
        switch (String->Buffer[i])
        {
        case '\\':
        case '\"':
        case '\'':
            temp[1] = String->Buffer[i];
            PhStringBuilderAppendEx(stringBuilder, temp, 4);
            break;
        default:
            PhStringBuilderAppendChar(stringBuilder, String->Buffer[i]);
            break;
        }
    }

    string = PhReferenceStringBuilderString(stringBuilder);
    PhDereferenceObject(stringBuilder);

    return string;
}

PPH_STRING PhpBuildRunAsServiceCommandLine(
    __in PWSTR Program,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId,
    __in PWSTR ErrorMailslot
    )
{
    PH_STRINGREF stringRef;
    PPH_STRING string;
    PPH_STRING_BUILDER commandLineBuilder;

    if ((!UserName || !Password) && !ProcessIdWithToken)
        return NULL;

    commandLineBuilder = PhCreateStringBuilder(PhApplicationFileName->Length + 70);

    PhStringBuilderAppendChar(commandLineBuilder, '\"');
    PhStringBuilderAppend(commandLineBuilder, PhApplicationFileName);
    PhStringBuilderAppend2(commandLineBuilder, L"\" -ras");

    PhInitializeStringRef(&stringRef, Program);
    string = PhpCEscapeString(&stringRef);
    PhStringBuilderAppend2(commandLineBuilder, L" -c \"");
    PhStringBuilderAppend(commandLineBuilder, string);
    PhStringBuilderAppendChar(commandLineBuilder, '\"');
    PhDereferenceObject(string);

    if (!ProcessIdWithToken)
    {
        PhInitializeStringRef(&stringRef, UserName);
        string = PhpCEscapeString(&stringRef);
        PhStringBuilderAppend2(commandLineBuilder, L" -u \"");
        PhStringBuilderAppend(commandLineBuilder, string);
        PhStringBuilderAppendChar(commandLineBuilder, '\"');
        PhDereferenceObject(string);

        PhInitializeStringRef(&stringRef, Password);
        string = PhpCEscapeString(&stringRef);
        PhStringBuilderAppend2(commandLineBuilder, L" -p \"");
        PhStringBuilderAppend(commandLineBuilder, string);
        PhStringBuilderAppendChar(commandLineBuilder, '\"');
        PhDereferenceObject(string);

        PhStringBuilderAppendFormat(
            commandLineBuilder, 
            L" -t %u",
            LogonType
            );
    }
    else
    {
        PhStringBuilderAppendFormat(
            commandLineBuilder, 
            L" -P %u",
            (ULONG)ProcessIdWithToken
            );
    }

    PhStringBuilderAppendFormat(
        commandLineBuilder,
        L" -s %u -E %s",
        SessionId,
        ErrorMailslot
        );

    string = PhReferenceStringBuilderString(commandLineBuilder);
    PhDereferenceObject(commandLineBuilder);

    return string;
}

/**
 * Executes the run-as service.
 *
 * \param ServiceCommandLine The full command line of the 
 * service, including file name and parameters.
 * \param ServiceName The name of the service. This will 
 * also be used as the name of the error mailslot.
 */
NTSTATUS PhRunAsCommandStart(
    __in PWSTR ServiceCommandLine,
    __in PWSTR ServiceName
    )
{
    NTSTATUS status;
    ULONG win32Result;
    SC_HANDLE scManagerHandle = NULL;
    SC_HANDLE serviceHandle = NULL;
    HANDLE mailslotFileHandle = NULL;

    if (!(scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)))
        return NTSTATUS_FROM_WIN32(GetLastError());

    serviceHandle = CreateService(
        scManagerHandle,
        ServiceName,
        ServiceName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        ServiceCommandLine,
        NULL,
        NULL,
        NULL,
        L"LocalSystem",
        L""
        );
    win32Result = GetLastError();

    CloseServiceHandle(scManagerHandle);

    if (!serviceHandle)
    {
        status = NTSTATUS_FROM_WIN32(win32Result);
        goto CleanupExit;
    }

    {
        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK isb;
        LARGE_INTEGER timeout;
        PPH_STRING mailslotFileName;
        NTSTATUS exitStatus;

        mailslotFileName = PhConcatStrings2(L"\\Device\\Mailslot\\", ServiceName);
        timeout.QuadPart = -5 * PH_TIMEOUT_SEC;

        InitializeObjectAttributes(
            &oa,
            &mailslotFileName->us,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtCreateMailslotFile(
            &mailslotFileHandle,
            FILE_GENERIC_READ,
            &oa,
            &isb,
            FILE_SYNCHRONOUS_IO_NONALERT,
            0,
            MAILSLOT_SIZE_AUTO,
            &timeout
            );
        PhDereferenceObject(mailslotFileName);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        PhSetDesktopWinStaAccess();

        StartService(serviceHandle, 0, NULL);
        DeleteService(serviceHandle);

        status = NtReadFile(mailslotFileHandle, NULL, NULL, NULL, &isb, &exitStatus, sizeof(NTSTATUS), NULL, NULL);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = exitStatus;
    }

CleanupExit:
    if (serviceHandle)
        CloseServiceHandle(serviceHandle);
    if (mailslotFileHandle)
        NtClose(mailslotFileHandle);

    return status;
}

/**
 * Starts a program as another user.
 *
 * \param hWnd A handle to the parent window.
 * \param Program The command line of the program to start.
 * \param UserName The user to start the program as. The user 
 * name should be specified as: domain\\name. This parameter 
 * can be NULL if \a ProcessIdWithToken is specified.
 * \param Password The password for the specified user. If there 
 * is no password, specify an empty string. This parameter 
 * can be NULL if \a ProcessIdWithToken is specified.
 * \param LogonType The logon type for the specified user. This 
 * parameter can be 0 if \a ProcessIdWithToken is specified.
 * \param ProcessIdWithToken The ID of a process from which 
 * to duplicate the token.
 * \param SessionId The ID of the session to run the program 
 * under.
 *
 * \retval STATUS_CANCELLED The user cancelled the operation.
 *
 * \remarks This function will cause another instance of 
 * Process Hacker to be executed if the current security context 
 * does not have sufficient system access. This is done 
 * through a UAC elevation prompt.
 */
NTSTATUS PhRunAsCommandStart2(
    __in HWND hWnd,
    __in PWSTR Program,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_STRING commandLine;
    WCHAR randomString[9];
    WCHAR serviceName[41];

    PhGenerateRandomAlphaString(randomString, 9);
    _snwprintf(serviceName, 40, L"ProcessHacker%s", randomString);

    commandLine = PhpBuildRunAsServiceCommandLine(
        Program,
        UserName,
        Password,
        LogonType,
        ProcessIdWithToken,
        SessionId,
        serviceName
        );

    if (!commandLine)
        return STATUS_INVALID_PARAMETER_MIX;

    if (PhElevated)
    {
        status = PhRunAsCommandStart(commandLine->Buffer, serviceName);
    }
    else
    {
        PPH_STRING_BUILDER argumentsBuilder;
        PPH_STRING string;
        HANDLE processHandle;
        LARGE_INTEGER timeout;

        argumentsBuilder = PhCreateStringBuilder(100);

        PhStringBuilderAppend2(
            argumentsBuilder,
            L"-c -ctype processhacker -caction runas -cobj \""
            );

        string = PhpCEscapeString(&commandLine->sr);
        PhStringBuilderAppend(argumentsBuilder, string);
        PhDereferenceObject(string);

        PhStringBuilderAppendFormat(
            argumentsBuilder,
            L"\" -hwnd %Iu -servicename %s",
            (PVOID)hWnd,
            serviceName
            );

        if (PhShellExecuteEx(
            hWnd,
            PhApplicationFileName->Buffer,
            argumentsBuilder->String->Buffer,
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            0,
            &processHandle
            ))
        {
            timeout.QuadPart = -10 * PH_TIMEOUT_SEC;
            status = NtWaitForSingleObject(processHandle, FALSE, &timeout);

            if (status == STATUS_WAIT_0)
            {
                PROCESS_BASIC_INFORMATION basicInfo;

                status = STATUS_SUCCESS;

                if (NT_SUCCESS(PhGetProcessBasicInformation(processHandle, &basicInfo)))
                {
                    status = basicInfo.ExitStatus;
                }
            }

            NtClose(processHandle);
        }
        else
        {
            status = STATUS_CANCELLED;
        }

        PhDereferenceObject(argumentsBuilder);
    }

    PhDereferenceObject(commandLine);

    return status;
}

VOID PhpRunAsServiceExit(
    __in NTSTATUS ExitStatus
    )
{
    if (RunAsServiceParameters.ErrorMailslot)
    {
        HANDLE fileHandle;
        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK isb;
        PPH_STRING fileName;

        fileName = PhConcatStrings2(
            L"\\Device\\Mailslot\\",
            RunAsServiceParameters.ErrorMailslot->Buffer
            );

        InitializeObjectAttributes(
            &oa,
            &fileName->us,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        if (NT_SUCCESS(NtOpenFile(
            &fileHandle,
            FILE_GENERIC_WRITE,
            &oa,
            &isb,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            NtWriteFile(fileHandle, NULL, NULL, NULL, &isb, &ExitStatus, sizeof(NTSTATUS), NULL, NULL); 
            NtClose(fileHandle);
        }

        PhDereferenceObject(fileName);
    }

    RtlExitUserProcess(ExitStatus);
}

#define PH_RUNAS_OPTION_USERNAME 1
#define PH_RUNAS_OPTION_PASSWORD 2
#define PH_RUNAS_OPTION_COMMANDLINE 3
#define PH_RUNAS_OPTION_FILENAME 4
#define PH_RUNAS_OPTION_LOGONTYPE 5
#define PH_RUNAS_OPTION_SESSIONID 6
#define PH_RUNAS_OPTION_PROCESSID 7
#define PH_RUNAS_OPTION_ERRORMAILSLOT 8
#define PH_RUNAS_OPTION_CURRENTDIRECTORY 9

BOOLEAN NTAPI PhpRunAsServiceOptionCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in PVOID Context
    )
{
    ULONG64 integer;

    if (Option)
    {
        switch (Option->Id)
        {
        case PH_RUNAS_OPTION_USERNAME:
            PhSwapReference(&RunAsServiceParameters.UserName, Value);
            break;
        case PH_RUNAS_OPTION_PASSWORD:
            PhSwapReference(&RunAsServiceParameters.Password, Value);
            break;
        case PH_RUNAS_OPTION_COMMANDLINE:
            PhSwapReference(&RunAsServiceParameters.CommandLine, Value);
            break;
        case PH_RUNAS_OPTION_FILENAME:
            PhSwapReference(&RunAsServiceParameters.FileName, Value);
            break;
        case PH_RUNAS_OPTION_LOGONTYPE:
            if (PhStringToInteger64(Value->Buffer, 10, &integer))
                RunAsServiceParameters.LogonType = (ULONG)integer;
            break;
        case PH_RUNAS_OPTION_SESSIONID:
            if (PhStringToInteger64(Value->Buffer, 10, &integer))
                RunAsServiceParameters.SessionId = (ULONG)integer;
            break;
        case PH_RUNAS_OPTION_PROCESSID:
            if (PhStringToInteger64(Value->Buffer, 10, &integer))
                RunAsServiceParameters.ProcessId = (ULONG)integer;
            break;
        case PH_RUNAS_OPTION_ERRORMAILSLOT:
            PhSwapReference(&RunAsServiceParameters.ErrorMailslot, Value);
            break;
        case PH_RUNAS_OPTION_CURRENTDIRECTORY:
            PhSwapReference(&RunAsServiceParameters.CurrentDirectory, Value);
            break;
        }
    }

    return TRUE;
}

VOID PhRunAsServiceStart()
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_RUNAS_OPTION_USERNAME, L"u", MandatoryArgumentType },
        { PH_RUNAS_OPTION_PASSWORD, L"p", MandatoryArgumentType },
        { PH_RUNAS_OPTION_COMMANDLINE, L"c", MandatoryArgumentType },
        { PH_RUNAS_OPTION_FILENAME, L"f", MandatoryArgumentType },
        { PH_RUNAS_OPTION_LOGONTYPE, L"t", MandatoryArgumentType },
        { PH_RUNAS_OPTION_SESSIONID, L"s", MandatoryArgumentType },
        { PH_RUNAS_OPTION_PROCESSID, L"P", MandatoryArgumentType },
        { PH_RUNAS_OPTION_ERRORMAILSLOT, L"E", MandatoryArgumentType },
        { PH_RUNAS_OPTION_CURRENTDIRECTORY, L"d", MandatoryArgumentType }
    };
    NTSTATUS status;
    PH_STRINGREF commandLine;
    HANDLE tokenHandle;
    ULONG indexOfBackslash;
    PPH_STRING domainName;
    PPH_STRING userName;

    // Enable some required privileges.

    if (NT_SUCCESS(PhOpenProcessToken(
        &tokenHandle,
        TOKEN_ADJUST_PRIVILEGES,
        NtCurrentProcess()
        )))
    {
        PhSetTokenPrivilege(tokenHandle, L"SeAssignPrimaryTokenPrivilege", NULL, SE_PRIVILEGE_ENABLED);
        PhSetTokenPrivilege(tokenHandle, L"SeBackupPrivilege", NULL, SE_PRIVILEGE_ENABLED);
        PhSetTokenPrivilege(tokenHandle, L"SeRestorePrivilege", NULL, SE_PRIVILEGE_ENABLED);
        NtClose(tokenHandle);
    }

    // Process command line options.

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    memset(&RunAsServiceParameters, 0, sizeof(RUNAS_SERVICE_PARAMETERS));

    if (!PhParseCommandLine(
        &commandLine,
        options, 
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS,
        PhpRunAsServiceOptionCallback,
        NULL
        ))
    {
        PhpRunAsServiceExit(STATUS_INVALID_PARAMETER);
    }

    if (RunAsServiceParameters.UserName)
    {
        indexOfBackslash = PhStringIndexOfChar(RunAsServiceParameters.UserName, 0, '\\');

        if (indexOfBackslash != -1)
        {
            domainName = PhSubstring(RunAsServiceParameters.UserName, 0, indexOfBackslash);
            userName = PhSubstring(
                RunAsServiceParameters.UserName,
                indexOfBackslash + 1,
                RunAsServiceParameters.UserName->Length / 2 - indexOfBackslash - 1
                );
        }
        else
        {
            domainName = NULL;
            userName = RunAsServiceParameters.UserName;
            PhReferenceObject(userName);
        }
    }
    else
    {
        domainName = NULL;
        userName = NULL;
    }

    status = PhCreateProcessAsUser(
        PhGetString(RunAsServiceParameters.FileName),
        PhGetString(RunAsServiceParameters.CommandLine),
        PhGetString(RunAsServiceParameters.CurrentDirectory),
        NULL,
        PhGetString(domainName),
        PhGetString(userName),
        PhGetString(RunAsServiceParameters.Password),
        RunAsServiceParameters.LogonType,
        (HANDLE)RunAsServiceParameters.ProcessId,
        RunAsServiceParameters.SessionId,
        NULL,
        NULL
        );

    if (domainName) PhDereferenceObject(domainName);
    if (userName) PhDereferenceObject(userName);

    PhpRunAsServiceExit(status);
}

static VOID PhpImportUserEnv()
{
    HMODULE userenv;

    if (!CreateEnvironmentBlock_I || !DestroyEnvironmentBlock_I)
    {
        userenv = LoadLibrary(L"userenv.dll");
        CreateEnvironmentBlock_I = (_CreateEnvironmentBlock)GetProcAddress(userenv, "CreateEnvironmentBlock");
        DestroyEnvironmentBlock_I = (_DestroyEnvironmentBlock)GetProcAddress(userenv, "DestroyEnvironmentBlock");
    }
}

NTSTATUS PhCreateProcessAsUser(
    __in_opt PWSTR ApplicationName,
    __in_opt PWSTR CommandLine,
    __in_opt PWSTR CurrentDirectory,
    __in_opt PVOID Environment,
    __in_opt PWSTR DomainName,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    )
{
    NTSTATUS status;
    LOGICAL result;
    ULONG win32Result;
    HANDLE tokenHandle;
    PVOID defaultEnvironment;
    STARTUPINFO startupInfo = { sizeof(startupInfo) };
    PROCESS_INFORMATION processInfo;

    if (!ApplicationName && !CommandLine)
        return STATUS_INVALID_PARAMETER_MIX;
    if ((!DomainName || !UserName || !Password) && !ProcessIdWithToken)
        return STATUS_INVALID_PARAMETER_MIX;

    // Get the token handle, either obtained by 
    // logging in as a user or stealing a token from 
    // another process.

    if (!ProcessIdWithToken)
    {
        if (!LogonUser(
            UserName,
            DomainName,
            Password,
            LogonType,
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
            ProcessIdWithToken
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

        // If we're going to set the session ID, we need 
        // to duplicate the token.
        if (SessionId != -1)
        {
            HANDLE newTokenHandle;

            result = DuplicateTokenEx(
                tokenHandle,
                TOKEN_ALL_ACCESS,
                NULL,
                SecurityImpersonation,
                TokenPrimary,
                &newTokenHandle
                );
            NtClose(tokenHandle);

            if (!result)
                return NTSTATUS_FROM_WIN32(GetLastError());

            tokenHandle = newTokenHandle;
        }
    }

    // Set the session ID if needed.

    if (SessionId != -1)
    {
        if (!NT_SUCCESS(status = PhSetTokenSessionId(
            tokenHandle,
            SessionId
            )))
        {
            NtClose(tokenHandle);
            return status;
        }
    }

    if (!Environment)
    {
        PhpImportUserEnv();
        defaultEnvironment = NULL;
        CreateEnvironmentBlock_I(&defaultEnvironment, tokenHandle, FALSE);
    }

    startupInfo.lpDesktop = L"WinSta0\\Default";

    result = CreateProcessAsUser(
        tokenHandle,
        ApplicationName,
        CommandLine,
        NULL,
        NULL,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        Environment ? Environment : defaultEnvironment,
        CurrentDirectory,
        &startupInfo,
        &processInfo
        );
    win32Result = GetLastError();

    if (defaultEnvironment)
    {
        DestroyEnvironmentBlock_I(defaultEnvironment);
    }

    NtClose(tokenHandle);

    if (result)
    {
        if (ProcessHandle)
            *ProcessHandle = processInfo.hProcess;
        else
            NtClose(processInfo.hProcess);

        if (ThreadHandle)
            *ThreadHandle = processInfo.hThread;
        else
            NtClose(processInfo.hThread);
    }

    return result ? STATUS_SUCCESS : NTSTATUS_FROM_WIN32(win32Result);
}

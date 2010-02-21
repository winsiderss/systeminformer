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

#include <phgui.h>
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

INT_PTR CALLBACK PhpRunAsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhSetDesktopWinStaAccess();

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

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

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

            SetFocus(GetDlgItem(hwndDlg, IDC_PROGRAM));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_PROGRAM), 0, -1);

            if (!PhElevated)
                SendMessage(GetDlgItem(hwndDlg, IDOK), BCM_SETSHIELD, 0, TRUE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
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
            case IDC_SESSIONS:
                {
                    HMENU sessionsMenu;
                    PWTS_SESSION_INFO sessions;
                    ULONG numberOfSessions;
                    ULONG i;
                    RECT buttonRect;
                    POINT point;

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
                            ULONG length;
                            PWSTR domainName = NULL;
                            PWSTR userName = NULL;
                            PPH_STRING menuString;

                            WTSQuerySessionInformation(
                                WTS_CURRENT_SERVER_HANDLE,
                                sessions[i].SessionId,
                                WTSDomainName,
                                &domainName,
                                &length
                                );
                            WTSQuerySessionInformation(
                                WTS_CURRENT_SERVER_HANDLE,
                                sessions[i].SessionId,
                                WTSUserName,
                                &userName,
                                &length
                                );

                            if (
                                userName &&
                                !WSTR_EQUAL(userName, L"") &&
                                sessions[i].pWinStationName &&
                                !WSTR_EQUAL(sessions[i].pWinStationName, L"")
                                )
                            {
                                menuString = PhFormatString(
                                    L"%u: %s (%s\\%s)",
                                    sessions[i].SessionId,
                                    sessions[i].pWinStationName,
                                    domainName ? domainName : L"",
                                    userName
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

                            if (domainName) WTSFreeMemory(domainName);
                            if (userName) WTSFreeMemory(userName);

                            AppendMenu(sessionsMenu, MF_STRING, IDDYNAMIC + i, menuString->Buffer);
                            PhDereferenceObject(menuString);

                            PhAddListItem(context->SessionIdList, (PVOID)sessions[i].SessionId);
                        }

                        WTSFreeMemory(sessions);

                        GetClientRect(GetDlgItem(hwndDlg, IDC_SESSIONS), &buttonRect);
                        point.x = buttonRect.right;
                        point.y = 0;
                        PhShowContextMenu(hwndDlg, GetDlgItem(hwndDlg, IDC_SESSIONS), sessionsMenu, point);

                        DestroyMenu(sessionsMenu);
                    }
                }
                break;
            default:
                {
                    if (LOWORD(wParam) >= IDDYNAMIC)
                    {
                        ULONG index = LOWORD(wParam) - IDDYNAMIC;

                        if (context->SessionIdList && index < context->SessionIdList->Count)
                        {
                            ULONG sessionId;

                            sessionId = (ULONG)context->SessionIdList->Items[index];
                            SetDlgItemInt(hwndDlg, IDC_SESSIONID, sessionId, FALSE);
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

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
            return STATUS_UNSUCCESSFUL;
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
                return STATUS_UNSUCCESSFUL;

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

    return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

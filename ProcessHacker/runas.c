/*
 * Process Hacker - 
 *   run as dialog
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

/*
 * The run-as mechanism has three stages:
 * 1. The user enters the information into the dialog box. Here it is decided 
 *    whether the run-as service is needed. If it is not, PhCreateProcessAsUser 
 *    is called directly. Otherwise, PhExecuteRunAsCommand2 is called for 
 *    stage 2.
 * 2. PhExecuteRunAsCommand2 creates a random service name and tries to create 
 *    the service and execute it (using PhExecuteRunAsCommand). If the process 
 *    has insufficient permissions, an elevated instance of phsvc is started 
 *    and PhSvcCallExecuteRunAsCommand is called.
 * 3. The service is started, and sets up an instance of phsvc with the same 
 *    random service name as its port name. Either the original or elevated 
 *    Process Hacker instance then calls PhSvcCallInvokeRunAsService to complete 
 *    the operation.
 *
 * ProcessHacker.exe (user, limited privileges)
 *   *                       | ^
 *   |                       | | phsvc API (LPC)
 *   |                       | |
 *   |                       v |
 *   ProcessHacker.exe (user, full privileges)
 *         | ^                    | ^
 *         | | SCM API (RPC)      | |
 *         | |                    | |
 *         v |                    | | phsvc API (LPC)
 * services.exe                   | |
 *   *                            | |
 *   |                            | |
 *   |                            | |
 *   |                            v |
 *   ProcessHacker.exe (NT AUTHORITY\SYSTEM)
 *     *
 *     |
 *     |
 *     |
 *     program.exe
 */

#include <phapp.h>
#include <phsvc.h>
#include <phsvccl.h>
#include <settings.h>
#include <emenu.h>
#include <shlwapi.h>
#include <winsta.h>
#include <windowsx.h>

typedef struct _RUNAS_DIALOG_CONTEXT
{
    HANDLE ProcessId;
    PPH_LIST DesktopList;
    PPH_STRING CurrentWinStaName;
} RUNAS_DIALOG_CONTEXT, *PRUNAS_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpRunAsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhSetDesktopWinStaAccess();

VOID PhpSplitUserName(
    __in PWSTR UserName,
    __out PPH_STRING *DomainPart,
    __out PPH_STRING *UserPart
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

static WCHAR RunAsOldServiceName[32] = L"";
static PH_QUEUED_LOCK RunAsOldServiceLock = PH_QUEUED_LOCK_INIT;

static PPH_STRING RunAsServiceName;
static SERVICE_STATUS_HANDLE RunAsServiceStatusHandle;
static PHSVC_STOP RunAsServiceStop;

VOID PhShowRunAsDialog(
    __in HWND ParentWindowHandle,
    __in_opt HANDLE ProcessId
    )
{
    RUNAS_DIALOG_CONTEXT context;

    context.ProcessId = ProcessId;
    context.DesktopList = NULL;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_RUNAS),
        ParentWindowHandle,
        PhpRunAsDlgProc,
        (LPARAM)&context
        );
}

static VOID PhpAddAccountsToComboBox(
    __in HWND ComboBoxHandle
    )
{
    LSA_HANDLE policyHandle;
    LSA_ENUMERATION_HANDLE enumerationContext = 0;
    PLSA_ENUMERATION_INFORMATION buffer;
    ULONG count;
    ULONG i;
    PPH_STRING name;
    SID_NAME_USE nameUse;

    if (NT_SUCCESS(PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
    {
        while (NT_SUCCESS(LsaEnumerateAccounts(
            policyHandle,
            &enumerationContext,
            &buffer,
            0x100,
            &count
            )))
        {
            for (i = 0; i < count; i++)
            {
                name = PhGetSidFullName(buffer[i].Sid, TRUE, &nameUse);

                if (name)
                {
                    if (nameUse == SidTypeUser)
                        ComboBox_AddString(ComboBoxHandle, name->Buffer);

                    PhDereferenceObject(name);
                }
            }

            LsaFreeMemory(buffer);
        }

        LsaClose(policyHandle);
    }
}

static BOOLEAN IsServiceAccount(
    __in PPH_STRING UserName
    )
{
    if (
        PhEqualString2(UserName, L"NT AUTHORITY\\LOCAL SERVICE", TRUE) || 
        PhEqualString2(UserName, L"NT AUTHORITY\\NETWORK SERVICE", TRUE) ||
        PhEqualString2(UserName, L"NT AUTHORITY\\SYSTEM", TRUE)
        )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static PPH_STRING GetCurrentWinStaName()
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, 0x200);

    if (GetUserObjectInformation(
        GetProcessWindowStation(),
        UOI_NAME,
        string->Buffer,
        string->Length + 2,
        NULL
        ))
    {
        PhTrimToNullTerminatorString(string);
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return PhCreateString(L"WinSta0"); // assume the current window station is WinSta0
    }
}

static BOOL CALLBACK EnumDesktopsCallback(
    __in PWSTR DesktopName,
    __in LPARAM Context
    )
{
    PRUNAS_DIALOG_CONTEXT context = (PRUNAS_DIALOG_CONTEXT)Context;

    PhAddItemList(context->DesktopList, PhConcatStrings(
        3,
        context->CurrentWinStaName->Buffer,
        L"\\",
        DesktopName
        ));

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
        context = (PRUNAS_DIALOG_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
    }
    else
    {
        context = (PRUNAS_DIALOG_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND typeComboBoxHandle = GetDlgItem(hwndDlg, IDC_TYPE);
            HWND userNameComboBoxHandle = GetDlgItem(hwndDlg, IDC_USERNAME);
            ULONG sessionId;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (SHAutoComplete_I)
            {
                SHAutoComplete_I(
                    GetDlgItem(hwndDlg, IDC_PROGRAM), 
                    SHACF_AUTOAPPEND_FORCE_ON | SHACF_AUTOSUGGEST_FORCE_ON | SHACF_FILESYS_ONLY
                    );
            }

            ComboBox_AddString(typeComboBoxHandle, L"Batch");
            ComboBox_AddString(typeComboBoxHandle, L"Interactive");
            ComboBox_AddString(typeComboBoxHandle, L"Network");
            ComboBox_AddString(typeComboBoxHandle, L"New credentials");
            ComboBox_AddString(typeComboBoxHandle, L"Service");
            ComboBox_SelectString(typeComboBoxHandle, -1, L"Interactive");

            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\SYSTEM");
            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\LOCAL SERVICE");
            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\NETWORK SERVICE");

            PhpAddAccountsToComboBox(userNameComboBoxHandle);

            if (NT_SUCCESS(PhGetProcessSessionId(NtCurrentProcess(), &sessionId)))
                SetDlgItemInt(hwndDlg, IDC_SESSIONID, sessionId, FALSE);

            SetDlgItemText(hwndDlg, IDC_DESKTOP, L"WinSta0\\Default");

            SetDlgItemText(hwndDlg, IDC_PROGRAM,
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStringSetting(L"RunAsProgram")))->Buffer);

            if (!context->ProcessId)
            {
                SetDlgItemText(hwndDlg, IDC_USERNAME,
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetStringSetting(L"RunAsUserName")))->Buffer);

                // Fire the user name changed event so we can fix the logon type.
                SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_USERNAME, CBN_EDITCHANGE), 0);
            }
            else
            {
                HANDLE processHandle;
                HANDLE tokenHandle;
                PTOKEN_USER user;
                PPH_STRING userName;

                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    ProcessQueryAccess,
                    context->ProcessId
                    )))
                {
                    if (NT_SUCCESS(PhOpenProcessToken(
                        &tokenHandle,
                        TOKEN_QUERY,
                        processHandle
                        )))
                    {
                        if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &user)))
                        {
                            if (userName = PhGetSidFullName(user->User.Sid, TRUE, NULL))
                            {
                                SetDlgItemText(hwndDlg, IDC_USERNAME, userName->Buffer);
                                PhDereferenceObject(userName);
                            }

                            PhFree(user);
                        }

                        NtClose(tokenHandle);
                    }

                    NtClose(processHandle);
                }

                EnableWindow(GetDlgItem(hwndDlg, IDC_USERNAME), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TYPE), FALSE);
            }

            SetFocus(GetDlgItem(hwndDlg, IDC_PROGRAM));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_PROGRAM), 0, -1);

            //if (!PhElevated)
            //    SendMessage(GetDlgItem(hwndDlg, IDOK), BCM_SETSHIELD, 0, TRUE);

            if (!WINDOWS_HAS_UAC)
                ShowWindow(GetDlgItem(hwndDlg, IDC_TOGGLEELEVATION), SW_HIDE);
        }
        break;
    case WM_DESTROY:
        {
            if (context->DesktopList)
                PhDereferenceObject(context->DesktopList);

            RemoveProp(hwndDlg, PhMakeContextAtom());
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
                    ULONG sessionId;
                    PPH_STRING desktopName;
                    BOOLEAN useLinkedToken;

                    program = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_PROGRAM);
                    userName = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_USERNAME);
                    logonTypeString = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_TYPE);

                    // Fix up the user name if it doesn't have a domain.
                    if (PhFindCharInString(userName, 0, '\\') == -1)
                    {
                        PSID sid;
                        PPH_STRING newUserName;

                        if (NT_SUCCESS(PhLookupName(&userName->sr, &sid, NULL, NULL)))
                        {
                            newUserName = PhGetSidFullName(sid, TRUE, NULL);

                            if (newUserName)
                            {
                                PhaDereferenceObject(newUserName);
                                userName = newUserName;
                            }

                            PhFree(sid);
                        }
                    }

                    if (!IsServiceAccount(userName))
                        password = PhGetWindowText(GetDlgItem(hwndDlg, IDC_PASSWORD));
                    else
                        password = NULL;

                    sessionId = GetDlgItemInt(hwndDlg, IDC_SESSIONID, NULL, FALSE);
                    desktopName = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_DESKTOP);

                    if (WINDOWS_HAS_UAC)
                        useLinkedToken = Button_GetCheck(GetDlgItem(hwndDlg, IDC_TOGGLEELEVATION)) == BST_CHECKED;
                    else
                        useLinkedToken = FALSE;

                    if (PhFindIntegerSiKeyValuePairs(
                        PhpLogonTypePairs,
                        sizeof(PhpLogonTypePairs),
                        logonTypeString->Buffer,
                        &logonType
                        ))
                    {
                        if (
                            logonType == LOGON32_LOGON_INTERACTIVE &&
                            !context->ProcessId &&
                            sessionId == NtCurrentPeb()->SessionId &&
                            !useLinkedToken
                            )
                        {
                            // We are eligible to load the user profile.
                            // This must be done here, not in the service, because 
                            // we need to be in the target session.

                            PH_CREATE_PROCESS_AS_USER_INFO createInfo;
                            PPH_STRING domainPart;
                            PPH_STRING userPart;

                            PhpSplitUserName(userName->Buffer, &domainPart, &userPart);

                            memset(&createInfo, 0, sizeof(PH_CREATE_PROCESS_AS_USER_INFO));
                            createInfo.CommandLine = program->Buffer;
                            createInfo.UserName = userPart->Buffer;
                            createInfo.DomainName = domainPart->Buffer;
                            createInfo.Password = PhGetStringOrEmpty(password);

                            // Whenever we can, try not to set the desktop name; it breaks a lot of things.
                            // Note that on XP we must set it, otherwise the program doesn't display correctly.
                            if (WindowsVersion < WINDOWS_VISTA || (desktopName->Length != 0 && !PhEqualString2(desktopName, L"WinSta0\\Default", TRUE)))
                                createInfo.DesktopName = desktopName->Buffer;

                            PhSetDesktopWinStaAccess();

                            status = PhCreateProcessAsUser(
                                &createInfo,
                                PH_CREATE_PROCESS_WITH_PROFILE,
                                NULL,
                                NULL,
                                NULL
                                );

                            if (domainPart) PhDereferenceObject(domainPart);
                            if (userPart) PhDereferenceObject(userPart);
                        }
                        else
                        {
                            status = PhExecuteRunAsCommand2(
                                hwndDlg,
                                program->Buffer,
                                userName->Buffer,
                                PhGetStringOrEmpty(password),
                                logonType,
                                context->ProcessId,
                                sessionId,
                                desktopName->Buffer,
                                useLinkedToken
                                );
                        }
                    }
                    else
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }

                    if (password)
                    {
                        RtlSecureZeroMemory(password->Buffer, password->Length);
                        PhDereferenceObject(password);
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
                                PhEqualString2(userName, L"NT AUTHORITY\\SYSTEM", TRUE) &&
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
                    PPH_EMENU sessionsMenu;
                    PSESSIONIDW sessions;
                    ULONG numberOfSessions;
                    ULONG i;
                    RECT buttonRect;
                    PPH_EMENU_ITEM selectedItem;

                    sessionsMenu = PhCreateEMenu();

                    if (WinStationEnumerateW(NULL, &sessions, &numberOfSessions))
                    {
                        for (i = 0; i < numberOfSessions; i++)
                        {
                            PPH_STRING menuString;
                            WINSTATIONINFORMATION winStationInfo;
                            ULONG returnLength;

                            if (!WinStationQueryInformationW(
                                NULL,
                                sessions[i].SessionId,
                                WinStationInformation,
                                &winStationInfo,
                                sizeof(WINSTATIONINFORMATION),
                                &returnLength
                                ))
                            {
                                winStationInfo.Domain[0] = 0;
                                winStationInfo.UserName[0] = 0;
                            }

                            if (
                                winStationInfo.UserName[0] != 0 &&
                                sessions[i].WinStationName[0] != 0
                                )
                            {
                                menuString = PhFormatString(
                                    L"%u: %s (%s\\%s)",
                                    sessions[i].SessionId,
                                    sessions[i].WinStationName,
                                    winStationInfo.Domain,
                                    winStationInfo.UserName
                                    );
                            }
                            else if (winStationInfo.UserName[0] != 0)
                            {
                                menuString = PhFormatString(
                                    L"%u: %s\\%s",
                                    sessions[i].SessionId,
                                    winStationInfo.Domain,
                                    winStationInfo.UserName
                                    );
                            }
                            else if (sessions[i].WinStationName[0] != 0)
                            {
                                menuString = PhFormatString(
                                    L"%u: %s",
                                    sessions[i].SessionId,
                                    sessions[i].WinStationName
                                    );
                            }
                            else
                            {
                                menuString = PhFormatString(L"%u", sessions[i].SessionId);
                            }

                            PhInsertEMenuItem(sessionsMenu,
                                PhCreateEMenuItem(0, 0, menuString->Buffer, NULL, (PVOID)sessions[i].SessionId), -1);
                            PhaDereferenceObject(menuString);
                        }

                        WinStationFreeMemory(sessions);

                        GetWindowRect(GetDlgItem(hwndDlg, IDC_SESSIONS), &buttonRect);

                        selectedItem = PhShowEMenu(
                            sessionsMenu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            buttonRect.right,
                            buttonRect.top
                            );

                        if (selectedItem)
                        {
                            SetDlgItemInt(
                                hwndDlg,
                                IDC_SESSIONID,
                                (ULONG)selectedItem->Context,
                                FALSE
                                );
                        }

                        PhDestroyEMenu(sessionsMenu);
                    }
                }
                break;
            case IDC_DESKTOPS:
                {
                    PPH_EMENU desktopsMenu;
                    ULONG i;
                    RECT buttonRect;
                    PPH_EMENU_ITEM selectedItem;

                    desktopsMenu = PhCreateEMenu();

                    if (!context->DesktopList)
                        context->DesktopList = PhCreateList(10);

                    context->CurrentWinStaName = GetCurrentWinStaName();

                    EnumDesktops(GetProcessWindowStation(), EnumDesktopsCallback, (LPARAM)context); 

                    for (i = 0; i < context->DesktopList->Count; i++)
                    {
                        PhInsertEMenuItem(
                            desktopsMenu,
                            PhCreateEMenuItem(0, 0, ((PPH_STRING)context->DesktopList->Items[i])->Buffer, NULL, NULL),
                            -1
                            );
                    }

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_DESKTOPS), &buttonRect);

                    selectedItem = PhShowEMenu(
                        desktopsMenu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        buttonRect.right,
                        buttonRect.top
                        );

                    if (selectedItem)
                    {
                        SetDlgItemText(
                            hwndDlg,
                            IDC_DESKTOP,
                            selectedItem->Text
                            );
                    }

                    for (i = 0; i < context->DesktopList->Count; i++)
                        PhDereferenceObject(context->DesktopList->Items[i]);

                    PhClearList(context->DesktopList);
                    PhDereferenceObject(context->CurrentWinStaName);
                    PhDestroyEMenu(desktopsMenu);
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

    // TODO: Set security on the correct window station and desktop.

    // Create a security descriptor with a NULL DACL, 
    // thereby allowing everyone to access the object.
    RtlCreateSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);

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
 * Executes the run-as service.
 *
 * \param Parameters The run-as parameters.
 *
 * \remarks This function requires administrator-level access.
 */
NTSTATUS PhExecuteRunAsCommand(
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    ULONG win32Result;
    PPH_STRING commandLine;
    SC_HANDLE scManagerHandle;
    SC_HANDLE serviceHandle;
    PPH_STRING portName;
    ULONG attempts;
    LARGE_INTEGER interval;

    if (!(scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)))
        return PhGetLastWin32ErrorAsNtStatus();

    commandLine = PhFormatString(L"\"%s\" -ras \"%s\"", PhApplicationFileName->Buffer, Parameters->ServiceName);

    serviceHandle = CreateService(
        scManagerHandle,
        Parameters->ServiceName,
        Parameters->ServiceName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        commandLine->Buffer,
        NULL,
        NULL,
        NULL,
        L"LocalSystem",
        L""
        );
    win32Result = GetLastError();

    PhDereferenceObject(commandLine);

    CloseServiceHandle(scManagerHandle);

    if (!serviceHandle)
    {
        return NTSTATUS_FROM_WIN32(win32Result);
    }

    PhSetDesktopWinStaAccess();

    StartService(serviceHandle, 0, NULL);
    DeleteService(serviceHandle);

    portName = PhConcatStrings2(L"\\BaseNamedObjects\\", Parameters->ServiceName);
    attempts = 10;

    // Try to connect several times because the server may take 
    // a while to initialize.
    do
    {
        status = PhSvcConnectToServer(&portName->us, 0);

        if (NT_SUCCESS(status))
            break;

        interval.QuadPart = -50 * PH_TIMEOUT_MS;
        NtDelayExecution(FALSE, &interval);
    } while (--attempts != 0);

    PhDereferenceObject(portName);

    if (NT_SUCCESS(status))
    {
        status = PhSvcCallInvokeRunAsService(Parameters);
        PhSvcDisconnectFromServer();
    }

    if (serviceHandle)
        CloseServiceHandle(serviceHandle);

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
 * \param DesktopName The window station and desktop to run the 
 * program under.
 * \param UseLinkedToken Uses the linked token if possible.
 *
 * \retval STATUS_CANCELLED The user cancelled the operation.
 *
 * \remarks This function will cause another instance of 
 * Process Hacker to be executed if the current security context 
 * does not have sufficient system access. This is done 
 * through a UAC elevation prompt.
 */
NTSTATUS PhExecuteRunAsCommand2(
    __in HWND hWnd,
    __in PWSTR Program,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId,
    __in PWSTR DesktopName,
    __in BOOLEAN UseLinkedToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PH_RUNAS_SERVICE_PARAMETERS parameters;
    WCHAR serviceName[32];
    PPH_STRING portName;

    memset(&parameters, 0, sizeof(PH_RUNAS_SERVICE_PARAMETERS));
    parameters.ProcessId = (ULONG)ProcessIdWithToken;
    parameters.UserName = UserName;
    parameters.Password = Password;
    parameters.LogonType = LogonType;
    parameters.SessionId = SessionId;
    parameters.CommandLine = Program;
    parameters.DesktopName = DesktopName;
    parameters.UseLinkedToken = UseLinkedToken;

    // Try to use an existing instance of the service if possible.
    if (RunAsOldServiceName[0] != 0)
    {
        PhAcquireQueuedLockExclusive(&RunAsOldServiceLock);

        portName = PhConcatStrings2(L"\\BaseNamedObjects\\", RunAsOldServiceName);

        if (NT_SUCCESS(PhSvcConnectToServer(&portName->us, 0)))
        {
            parameters.ServiceName = RunAsOldServiceName;
            status = PhSvcCallInvokeRunAsService(&parameters);
            PhSvcDisconnectFromServer();

            PhDereferenceObject(portName);
            PhReleaseQueuedLockExclusive(&RunAsOldServiceLock);

            return status;
        }

        PhDereferenceObject(portName);
        PhReleaseQueuedLockExclusive(&RunAsOldServiceLock);
    }

    // An existing instance was not available. Proceed normally.

    memcpy(serviceName, L"ProcessHacker", 13 * sizeof(WCHAR));
    PhGenerateRandomAlphaString(&serviceName[13], 16);
    PhAcquireQueuedLockExclusive(&RunAsOldServiceLock);
    memcpy(RunAsOldServiceName, serviceName, sizeof(serviceName));
    PhReleaseQueuedLockExclusive(&RunAsOldServiceLock);

    parameters.ServiceName = serviceName;

    if (PhElevated)
    {
        status = PhExecuteRunAsCommand(&parameters);
    }
    else
    {
        if (PhUiConnectToPhSvc(hWnd, FALSE))
        {
            status = PhSvcCallExecuteRunAsCommand(&parameters);
            PhUiDisconnectFromPhSvc();
        }
        else
        {
            status = STATUS_CANCELLED;
        }
    }

    return status;
}

static VOID PhpSplitUserName(
    __in PWSTR UserName,
    __out PPH_STRING *DomainPart,
    __out PPH_STRING *UserPart
    )
{
    PH_STRINGREF userName;
    ULONG indexOfBackslash;

    PhInitializeStringRef(&userName, UserName);
    indexOfBackslash = PhFindCharInStringRef(&userName, 0, '\\');

    if (indexOfBackslash != -1)
    {
        *DomainPart = PhCreateStringEx(UserName, indexOfBackslash * 2);
        *UserPart = PhCreateStringEx(UserName + indexOfBackslash + 1, userName.Length - indexOfBackslash * 2 - 2);
    }
    else
    {
        *DomainPart = NULL;
        *UserPart = PhCreateStringEx(userName.Buffer, userName.Length);
    }
}

static VOID SetRunAsServiceStatus(
    __in ULONG State
    )
{
    SERVICE_STATUS status;

    memset(&status, 0, sizeof(SERVICE_STATUS));
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = State;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    SetServiceStatus(RunAsServiceStatusHandle, &status);
}

static DWORD WINAPI RunAsServiceHandlerEx(
    __in DWORD dwControl,
    __in DWORD dwEventType,
    __in LPVOID lpEventData,
    __in LPVOID lpContext
    )
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        RunAsServiceStop.Stop = TRUE;
        if (RunAsServiceStop.Event1)
            NtSetEvent(RunAsServiceStop.Event1, NULL);
        if (RunAsServiceStop.Event2)
            NtSetEvent(RunAsServiceStop.Event2, NULL);
        return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

static VOID WINAPI RunAsServiceMain(
    __in DWORD dwArgc,
    __in LPTSTR *lpszArgv
    )
{
    PPH_STRING portName;
    LARGE_INTEGER timeout;

    memset(&RunAsServiceStop, 0, sizeof(PHSVC_STOP));
    RunAsServiceStatusHandle = RegisterServiceCtrlHandlerEx(RunAsServiceName->Buffer, RunAsServiceHandlerEx, NULL);
    SetRunAsServiceStatus(SERVICE_RUNNING);

    portName = PhConcatStrings2(L"\\BaseNamedObjects\\", RunAsServiceName->Buffer);
    // Use a shorter timeout value to reduce the time spent running as SYSTEM.
    timeout.QuadPart = -5 * PH_TIMEOUT_SEC;

    PhSvcMain(&portName->sr, &timeout, &RunAsServiceStop);

    SetRunAsServiceStatus(SERVICE_STOPPED);
}

NTSTATUS PhRunAsServiceStart(
    __in PPH_STRING ServiceName
    )
{
    HANDLE tokenHandle;
    SERVICE_TABLE_ENTRY entry;

    // Enable some required privileges.

    if (NT_SUCCESS(PhOpenProcessToken(
        &tokenHandle,
        TOKEN_ADJUST_PRIVILEGES,
        NtCurrentProcess()
        )))
    {
        PhSetTokenPrivilege(tokenHandle, L"SeAssignPrimaryTokenPrivilege", NULL, SE_PRIVILEGE_ENABLED);
        PhSetTokenPrivilege(tokenHandle, L"SeBackupPrivilege", NULL, SE_PRIVILEGE_ENABLED);
        PhSetTokenPrivilege(tokenHandle, L"SeImpersonatePrivilege", NULL, SE_PRIVILEGE_ENABLED);
        PhSetTokenPrivilege(tokenHandle, L"SeIncreaseQuotaPrivilege", NULL, SE_PRIVILEGE_ENABLED);
        PhSetTokenPrivilege(tokenHandle, L"SeRestorePrivilege", NULL, SE_PRIVILEGE_ENABLED);
        NtClose(tokenHandle);
    }

    RunAsServiceName = ServiceName;

    entry.lpServiceName = ServiceName->Buffer;
    entry.lpServiceProc = RunAsServiceMain;

    StartServiceCtrlDispatcher(&entry);

    return STATUS_SUCCESS;
}

NTSTATUS PhInvokeRunAsService(
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PPH_STRING domainName;
    PPH_STRING userName;
    PH_CREATE_PROCESS_AS_USER_INFO createInfo;
    ULONG flags;

    if (Parameters->UserName)
    {
        PhpSplitUserName(Parameters->UserName, &domainName, &userName);
    }
    else
    {
        domainName = NULL;
        userName = NULL;
    }

    memset(&createInfo, 0, sizeof(PH_CREATE_PROCESS_AS_USER_INFO));
    createInfo.ApplicationName = Parameters->FileName;
    createInfo.CommandLine = Parameters->CommandLine;
    createInfo.CurrentDirectory = Parameters->CurrentDirectory;
    createInfo.DomainName = PhGetString(domainName);
    createInfo.UserName = PhGetString(userName);
    createInfo.Password = Parameters->Password;
    createInfo.LogonType = Parameters->LogonType;
    createInfo.SessionId = Parameters->SessionId;
    createInfo.DesktopName = Parameters->DesktopName;

    flags = PH_CREATE_PROCESS_SET_SESSION_ID;

    if (Parameters->ProcessId)
    {
        createInfo.ProcessIdWithToken = UlongToHandle(Parameters->ProcessId);
        flags |= PH_CREATE_PROCESS_USE_PROCESS_TOKEN;
    }

    if (Parameters->UseLinkedToken)
        flags |= PH_CREATE_PROCESS_USE_LINKED_TOKEN;

    status = PhCreateProcessAsUser(
        &createInfo,
        flags,
        NULL,
        NULL,
        NULL
        );

    if (domainName) PhDereferenceObject(domainName);
    if (userName) PhDereferenceObject(userName);

    return status;
}

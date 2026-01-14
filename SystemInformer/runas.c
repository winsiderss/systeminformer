/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2018-2023
 *
 */

/*
 * The run-as mechanism has three stages:
 * 1. The user enters the information into the dialog box. Here it is decided whether the run-as
 *    service is needed. If it is not, PhCreateProcessAsUser is called directly. Otherwise,
 *    PhExecuteRunAsCommand2 is called for stage 2.
 * 2. PhExecuteRunAsCommand2 creates a random service name and tries to create the service and
 *    execute it (using PhExecuteRunAsCommand). If the process has insufficient permissions, an
 *    elevated instance of phsvc is started and PhSvcCallExecuteRunAsCommand is called.
 * 3. The service is started, and sets up an instance of phsvc with the same random service name as
 *    its port name. Either the original or elevated Process Hacker instance then calls
 *    PhSvcCallInvokeRunAsService to complete the operation.
 */

/*
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

#include <shlwapi.h>
#include <winsta.h>

#include <apiimport.h>
#include <appresolver.h>
#include <actions.h>
#include <lsasup.h>
#include <phconsole.h>
#include <phsvc.h>
#include <phsvccl.h>
#include <phsettings.h>
#include <settings.h>
#include <svcsup.h>
#include <mainwnd.h>

typedef struct _RUNAS_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND ProgramComboBoxWindowHandle;
    HWND UserComboBoxWindowHandle;
    HWND TypeComboBoxWindowHandle;
    HWND PasswordEditWindowHandle;
    HWND SessionEditWindowHandle;
    HWND DesktopEditWindowHandle;
    HANDLE ProcessId;
    PPH_STRING CurrentWinStaName;
} RUNAS_DIALOG_CONTEXT, *PRUNAS_DIALOG_CONTEXT;

typedef struct _PH_RUNAS_SESSION_ITEM
{
    ULONG SessionId;
    PPH_STRING SessionName;
} PH_RUNAS_SESSION_ITEM, *PPH_RUNAS_SESSION_ITEM;

typedef struct _PH_RUNAS_DESKTOP_ITEM
{
    PPH_STRING DesktopName;
} PH_RUNAS_DESKTOP_ITEM, *PPH_RUNAS_DESKTOP_ITEM;

INT_PTR CALLBACK PhpRunAsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpRunFileWndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhRunAsPackageWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS PhRunAsUpdateDesktop(
    _In_ PSID UserSid
    );

NTSTATUS PhRunAsUpdateWindowStation(
    _In_opt_ PSID UserSid,
    _In_opt_ PSID LogonSid
    );

//NTSTATUS PhSetDesktopWinStaAccess(
//    _In_ HWND WindowHandle
//    );

BOOLEAN PhRunAsExecuteCommandPrompt(
    _In_ HWND WindowHandle
    );

VOID PhpSplitUserName(
    _In_ PCWSTR UserName,
    _Out_opt_ PPH_STRING* DomainPart,
    _Out_opt_ PPH_STRING* UserPart
    );

static CONST PH_KEY_VALUE_PAIR PhpLogonTypePairs[] =
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
    _In_ HWND ParentWindowHandle,
    _In_opt_ HANDLE ProcessId
    )
{
    PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_RUNAS),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhpRunAsDlgProc,
        ProcessId
        );
}

BOOLEAN PhShowRunFileDialog(
    _In_ HWND ParentWindowHandle
    )
{
    // Note: Task Manager launches the command prompt instead of RunFileDlg
    // when holding CTRL and selecting the 'Run New Task' menu. (dmex)
    // Todo: The CTRL key is required for the menu hotkey. (GH #1859)
    //if (PhGetKeyState(VK_CONTROL))
    //{
    //    if (PhRunAsExecuteCommandPrompt(ParentWindowHandle))
    //        return TRUE;
    //}

    if (PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_RUNFILEDLG),
        ParentWindowHandle,
        PhpRunFileWndProc,
        NULL
        ) == IDOK)
    {
        return TRUE;
    }

    return FALSE;

    // Removed from guisup.c (dmex)
    //BOOL (WINAPI *RunFileDlg_I)(
    //    _In_ HWND hwndOwner,
    //    _In_opt_ HICON hIcon,
    //    _In_opt_ LPCWSTR lpszDirectory,
    //    _In_opt_ LPCWSTR lpszTitle,
    //    _In_opt_ LPCWSTR lpszDescription,
    //    _In_ ULONG uFlags
    //    );
    //PVOID shell32Handle;
    //
    //if (shell32Handle = PhLoadLibrary(L"shell32.dll"))
    //{
    //    if (RunFileDlg_I = PhGetDllBaseProcedureAddress(shell32Handle, NULL, 61))
    //    {
    //        result = !!RunFileDlg_I(
    //            WindowHandle,
    //            WindowIcon,
    //            WorkingDirectory,
    //            WindowTitle,
    //            WindowDescription,
    //            Flags
    //            );
    //    }
    //
    //    PhFreeLibrary(shell32Handle);
    //}
}

VOID PhShowRunAsPackageDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_RUNPACKAGE),
        NULL,
        PhRunAsPackageWndProc,
        PhCsForceNoParent ? NULL : ParentWindowHandle
        );
}

BOOLEAN IsServiceAccount(
    _In_ PPH_STRING UserName
    )
{
    BOOLEAN serviceAccount = FALSE;
    PPH_STRING localSystemSidName;
    PPH_STRING localServiceSidName;
    PPH_STRING localNetworkSidName;

    localSystemSidName = PhGetSidFullName((PSID)&PhSeLocalSystemSid, TRUE, NULL);
    localServiceSidName = PhGetSidFullName((PSID)&PhSeLocalServiceSid, TRUE, NULL);
    localNetworkSidName = PhGetSidFullName((PSID)&PhSeNetworkServiceSid, TRUE, NULL);

    if (
        PhEqualString(localSystemSidName, UserName, TRUE) ||
        PhEqualString(localServiceSidName, UserName, TRUE) ||
        PhEqualString(localNetworkSidName, UserName, TRUE)
        )
    {
        serviceAccount = TRUE;
    }

    PhDereferenceObject(localNetworkSidName);
    PhDereferenceObject(localServiceSidName);
    PhDereferenceObject(localSystemSidName);

    return serviceAccount;
}

BOOLEAN IsCurrentUserAccount(
    _In_ PPH_STRING UserName
    )
{
    PPH_STRING userName;

    if (userName = PhGetTokenUserString(PhGetOwnTokenAttributes().TokenHandle, TRUE))
    {
        if (PhEndsWithString(userName, UserName, TRUE))
        {
            PhDereferenceObject(userName);
            return TRUE;
        }

        PhDereferenceObject(userName);
    }

    return FALSE;
}

PPH_STRING PhpGetCurrentDesktopInfo(
    VOID
    )
{
    PPH_STRING desktopInfo = NULL;
    PPH_STRING winstationName = NULL;
    PPH_STRING desktopName = NULL;

    winstationName = PhGetCurrentWindowStationName();
    desktopName = PhGetCurrentThreadDesktopName();

    if (winstationName && desktopName)
    {
        desktopInfo = PhConcatStringRef3(&winstationName->sr, &PhNtPathSeparatorString, &desktopName->sr);
    }

    if (PhIsNullOrEmptyString(desktopInfo))
    {
        PhMoveReference(&desktopInfo, PhCreateStringFromUnicodeString(&NtCurrentPeb()->ProcessParameters->DesktopInfo));
    }

    if (winstationName)
        PhDereferenceObject(winstationName);
    if (desktopName)
        PhDereferenceObject(desktopName);

    return desktopInfo;
}

BOOLEAN PhpEnumerateRecentProgramsToComboBox(
    _In_ PPH_STRINGREF Command,
    _In_ PVOID Context
    )
{
    ComboBox_AddString(Context, PhGetStringRefZ(Command));
    return TRUE;
}

NTSTATUS PhpEnumerateAccountsToComboBox(
    _In_ PPH_STRINGREF AccountName,
    _In_ PVOID Context
    )
{
    ComboBox_AddString(Context, PhGetStringRefZ(AccountName));
    return STATUS_SUCCESS;
}

static VOID PhpAddProgramsToComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    PhDeleteComboBoxStrings(ComboBoxHandle, TRUE);

    PhEnumerateRecentList(PhpEnumerateRecentProgramsToComboBox, ComboBoxHandle);
}

static VOID PhpAddAccountsToComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    PhDeleteComboBoxStrings(ComboBoxHandle, TRUE);

    ComboBox_AddString(ComboBoxHandle, PH_AUTO_T(PH_STRING, PhGetSidFullName((PSID)&PhSeLocalSystemSid, TRUE, NULL))->Buffer);
    ComboBox_AddString(ComboBoxHandle, PH_AUTO_T(PH_STRING, PhGetSidFullName((PSID)&PhSeLocalServiceSid, TRUE, NULL))->Buffer);
    ComboBox_AddString(ComboBoxHandle, PH_AUTO_T(PH_STRING, PhGetSidFullName((PSID)&PhSeNetworkServiceSid, TRUE, NULL))->Buffer);

    PhEnumerateAccounts(PhpEnumerateAccountsToComboBox, ComboBoxHandle);
}

static VOID PhpFreeSessionsComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    PPH_RUNAS_SESSION_ITEM entry;
    INT total;
    INT i;

    if ((total = ComboBox_GetCount(ComboBoxHandle)) == CB_ERR)
        return;

    for (i = 0; i < total; i++)
    {
        entry = (PPH_RUNAS_SESSION_ITEM)ComboBox_GetItemData(ComboBoxHandle, i);

        if (entry->SessionName)
            PhDereferenceObject(entry->SessionName);

        PhFree(entry);
    }

    ComboBox_ResetContent(ComboBoxHandle);
}

static VOID PhpAddSessionsToComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    PSESSIONIDW sessions;
    ULONG numberOfSessions;
    ULONG i;

    PhpFreeSessionsComboBox(ComboBoxHandle);

    if (WinStationEnumerateW(WINSTATION_CURRENT_SERVER, &sessions, &numberOfSessions))
    {
        for (i = 0; i < numberOfSessions; i++)
        {
            PPH_STRING menuString;
            WINSTATIONINFORMATION winStationInfo;
            ULONG returnLength;

            if (!WinStationQueryInformationW(
                WINSTATION_CURRENT_SERVER,
                sessions[i].SessionId,
                WinStationInformation,
                &winStationInfo,
                sizeof(WINSTATIONINFORMATION),
                &returnLength
                ))
            {
                winStationInfo.Domain[0] = UNICODE_NULL;
                winStationInfo.UserName[0] = UNICODE_NULL;
            }

            if (
                winStationInfo.UserName[0] != UNICODE_NULL &&
                sessions[i].WinStationName[0] != UNICODE_NULL
                )
            {
                SIZE_T formatLength;
                PH_FORMAT format[8];
                WCHAR formatBuffer[0x80];

                // %lu: %s (%s\\%s)
                PhInitFormatU(&format[0], sessions[i].SessionId);
                PhInitFormatS(&format[1], L": ");
                PhInitFormatS(&format[2], sessions[i].WinStationName);
                PhInitFormatS(&format[3], L" (");
                PhInitFormatS(&format[4], winStationInfo.Domain);
                PhInitFormatC(&format[5], L'\\');
                PhInitFormatS(&format[6], winStationInfo.UserName);
                PhInitFormatC(&format[7], L')');

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &formatLength))
                {
                    PH_STRINGREF text;

                    text.Length = formatLength - sizeof(UNICODE_NULL);
                    text.Buffer = formatBuffer;

                    menuString = PhCreateString2(&text);
                }
                else
                {
                    //menuString = PhFormatString(L"%lu: %s (%s\\%s)",
                    //    sessions[i].SessionId,
                    //    sessions[i].WinStationName,
                    //    winStationInfo.Domain,
                    //    winStationInfo.UserName
                    //    );

                    menuString = PhFormat(format, RTL_NUMBER_OF(format), 0);
                }
            }
            else if (winStationInfo.UserName[0] != UNICODE_NULL)
            {
                menuString = PhFormatString(L"%lu: %s\\%s",
                    sessions[i].SessionId,
                    winStationInfo.Domain,
                    winStationInfo.UserName
                    );
            }
            else if (sessions[i].WinStationName[0] != UNICODE_NULL)
            {
                SIZE_T formatLength;
                PH_FORMAT format[3];
                WCHAR formatBuffer[0x80];

                // %lu: %s
                PhInitFormatU(&format[0], sessions[i].SessionId);
                PhInitFormatS(&format[1], L": ");
                PhInitFormatS(&format[2], sessions[i].WinStationName);

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &formatLength))
                {
                    PH_STRINGREF text;

                    text.Length = formatLength - sizeof(UNICODE_NULL);
                    text.Buffer = formatBuffer;

                    menuString = PhCreateString2(&text);
                }
                else
                {
                    //menuString = PhFormatString(L"%lu: %s",
                    //    sessions[i].SessionId,
                    //    sessions[i].WinStationName
                    //    );

                    menuString = PhFormat(format, RTL_NUMBER_OF(format), 0);
                }
            }
            else
            {
                menuString = PhFormatUInt64(sessions[i].SessionId, FALSE);
            }

            {
                PPH_RUNAS_SESSION_ITEM entry;
                INT itemIndex;

                entry = PhAllocate(sizeof(PH_RUNAS_SESSION_ITEM));
                entry->SessionId = sessions[i].SessionId;
                entry->SessionName = menuString;

                if ((itemIndex = ComboBox_AddString(ComboBoxHandle, menuString->Buffer)) != CB_ERR)
                {
                    ComboBox_SetItemData(ComboBoxHandle, itemIndex, entry);
                }
            }
        }

        WinStationFreeMemory(sessions);
    }
}

typedef struct _RUNAS_DIALOG_DESKTOP_CALLBACK
{
    PPH_LIST DesktopList;
    PPH_STRING WinStaName;
} RUNAS_DIALOG_DESKTOP_CALLBACK, *PRUNAS_DIALOG_DESKTOP_CALLBACK;

static BOOL CALLBACK EnumDesktopsCallback(
    _In_ PWSTR DesktopName,
    _In_ LPARAM Context
    )
{
    PRUNAS_DIALOG_DESKTOP_CALLBACK context = (PRUNAS_DIALOG_DESKTOP_CALLBACK)Context;

    PhAddItemList(context->DesktopList, PhConcatStrings(
        3,
        PhGetString(context->WinStaName),
        L"\\",
        DesktopName
        ));

    return TRUE;
}

static VOID PhpFreeDesktopsComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    PPH_RUNAS_DESKTOP_ITEM entry;
    INT total;
    INT i;

    if ((total = ComboBox_GetCount(ComboBoxHandle)) == CB_ERR)
        return;

    for (i = 0; i < total; i++)
    {
        entry = (PPH_RUNAS_DESKTOP_ITEM)ComboBox_GetItemData(ComboBoxHandle, i);

        if (entry->DesktopName)
            PhDereferenceObject(entry->DesktopName);

        PhFree(entry);
    }

    ComboBox_ResetContent(ComboBoxHandle);
}

static VOID PhpAddDesktopsToComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    ULONG i;
    RUNAS_DIALOG_DESKTOP_CALLBACK callback;

    PhpFreeDesktopsComboBox(ComboBoxHandle);

    callback.DesktopList = PhCreateList(10);
    callback.WinStaName = PhGetCurrentWindowStationName();

    EnumDesktops(GetProcessWindowStation(), EnumDesktopsCallback, (LPARAM)&callback);

    for (i = 0; i < callback.DesktopList->Count; i++)
    {
        INT itemIndex = ComboBox_AddString(
            ComboBoxHandle,
            PhGetString(callback.DesktopList->Items[i])
            );

        if (itemIndex != CB_ERR)
        {
            PPH_RUNAS_DESKTOP_ITEM entry;

            entry = PhAllocate(sizeof(PH_RUNAS_DESKTOP_ITEM));
            entry->DesktopName = callback.DesktopList->Items[i];

            ComboBox_SetItemData(ComboBoxHandle, itemIndex, entry);
        }
    }

    PhDereferenceObject(callback.DesktopList);
    PhDereferenceObject(callback.WinStaName);
}

VOID SetDefaultProgramEntry(
    _In_ HWND ComboBoxHandle
    )
{
    //Edit_SetText(ComboBoxHandle, PhaGetStringSetting(SETTING_RUN_AS_PROGRAM)->Buffer);
    ComboBox_SetCurSel(ComboBoxHandle, 0);
}

VOID SetDefaultUserEntry(
    _In_ PRUNAS_DIALOG_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HWND ComboBoxHandle
    )
{
    if (!Context->ProcessId)
    {
        PPH_STRING runAsUserName = PhaGetStringSetting(SETTING_RUN_AS_USER_NAME);
        INT runAsUserNameIndex = CB_ERR;

        // Fire the user name changed event so we can fix the logon type.
        if (!PhIsNullOrEmptyString(runAsUserName))
        {
            runAsUserNameIndex = ComboBox_FindString(
                ComboBoxHandle,
                0,
                PhGetString(runAsUserName)
                );
        }

        if (runAsUserNameIndex != CB_ERR)
            ComboBox_SetCurSel(ComboBoxHandle, runAsUserNameIndex);
        else
            ComboBox_SetCurSel(ComboBoxHandle, 0);

        SendMessage(WindowHandle, WM_COMMAND, MAKEWPARAM(IDC_USERNAME, CBN_EDITCHANGE), 0);
    }
    else
    {
        HANDLE processHandle;
        HANDLE tokenHandle;
        PPH_STRING userName;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            Context->ProcessId
            )))
        {
            if (NT_SUCCESS(PhOpenProcessToken(
                processHandle,
                TOKEN_QUERY,
                &tokenHandle
                )))
            {
                if (userName = PhGetTokenUserString(tokenHandle, TRUE))
                {
                    PhSetWindowText(ComboBoxHandle, userName->Buffer);
                    PhDereferenceObject(userName);
                }

                NtClose(tokenHandle);
            }

            NtClose(processHandle);
        }

        EnableWindow(Context->UserComboBoxWindowHandle, FALSE);
        EnableWindow(Context->PasswordEditWindowHandle, FALSE);
        EnableWindow(Context->TypeComboBoxWindowHandle, FALSE);
    }
}

VOID SetDefaultSessionEntry(
    _In_ HWND ComboBoxHandle
    )
{
    INT sessionCount;
    ULONG currentSessionId = 0;

    if (!NT_SUCCESS(PhGetProcessSessionId(NtCurrentProcess(), &currentSessionId)))
        return;

    if ((sessionCount = ComboBox_GetCount(ComboBoxHandle)) == CB_ERR)
        return;

    for (INT i = 0; i < sessionCount; i++)
    {
        PPH_RUNAS_SESSION_ITEM entry = (PPH_RUNAS_SESSION_ITEM)ComboBox_GetItemData(ComboBoxHandle, i);

        if (entry && entry->SessionId == currentSessionId)
        {
            ComboBox_SetCurSel(ComboBoxHandle, i);
            break;
        }
    }
}

VOID SetDefaultDesktopEntry(
    _In_ PRUNAS_DIALOG_CONTEXT Context,
    _In_ HWND ComboBoxHandle
    )
{
    INT sessionCount;
    PPH_STRING desktopName;

    if (!(desktopName = PhpGetCurrentDesktopInfo()))
        return;

    if ((sessionCount = ComboBox_GetCount(ComboBoxHandle)) == CB_ERR)
    {
        PhClearReference(&desktopName);
        return;
    }

    for (INT i = 0; i < sessionCount; i++)
    {
        PPH_RUNAS_DESKTOP_ITEM entry = (PPH_RUNAS_DESKTOP_ITEM)ComboBox_GetItemData(ComboBoxHandle, i);

        if (PhEqualStringRef(&entry->DesktopName->sr, &desktopName->sr, TRUE))
        {
            ComboBox_SetCurSel(ComboBoxHandle, i);
            break;
        }
    }

    PhClearReference(&desktopName);
}

_Success_(return)
BOOLEAN PhRunAsGetLogonSid(
    _In_ HANDLE ProcessHandle,
    _Out_ PSID* UserSid,
    _Out_ PSID* LogonSid
    )
{
    PSID userSid = NULL;
    PSID groupSid = NULL;
    HANDLE tokenHandle;

    if (NT_SUCCESS(PhOpenProcessToken(
        ProcessHandle,
        TOKEN_QUERY,
        &tokenHandle
        )))
    {
        PTOKEN_GROUPS tokenGroups = NULL;
        PH_TOKEN_USER tokenUser;

        if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
        {
            userSid = PhAllocateCopy(tokenUser.User.Sid, PhLengthSid(tokenUser.User.Sid));
        }

        if (NT_SUCCESS(PhGetTokenGroups(
            tokenHandle,
            &tokenGroups
            )))
        {
            for (ULONG i = 0; i < tokenGroups->GroupCount; i++)
            {
                PSID_AND_ATTRIBUTES group = &tokenGroups->Groups[i];

                if (FlagOn(group->Attributes, SE_GROUP_LOGON_ID))
                {
                    groupSid = PhAllocateCopy(group->Sid, PhLengthSid(group->Sid));
                    break;
                }
            }

            PhFree(tokenGroups);
        }
    }

    if (userSid && groupSid)
    {
        *UserSid = userSid;
        *LogonSid = groupSid;
        return TRUE;
    }

    if (userSid)
        PhFree(userSid);
    if (groupSid)
        PhFree(groupSid);
    return FALSE;
}

NTSTATUS PhRunAsExecutionAlias(
    _In_ PPH_STRING Command
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    PPH_STRING fullFileName = NULL;
    PPH_STRING commandString = NULL;
    PH_STRINGREF fileName;
    PH_STRINGREF arguments;

    commandString = PhExpandEnvironmentStrings(&Command->sr);

    if (PhIsNullOrEmptyString(commandString))
    {
        PhMoveReference(&fullFileName, PhCreateString2(&Command->sr));
    }

    PhParseCommandLineFuzzy(&commandString->sr, &fileName, &arguments, &fullFileName);

    if (PhIsNullOrEmptyString(fullFileName))
    {
        PhMoveReference(&fullFileName, PhCreateString2(&fileName));
    }

    if (!PhIsNullOrEmptyString(fullFileName))
    {
        // NOTE: The CreateProcess function will ignore PROC_THREAD_ATTRIBUTE_PARENT_PROCESS when redirecting execution
        // of the filename via execution alias. The new process incorrectly inherits our elevated process token
        // instead of using the non-elevated parent process. To work around the issue we execute the alias using the
        // WdcRunTaskAsInteractiveUser function and also skip resetting the token and current directory. (dmex)

        if (PhIsAppExecutionAliasTarget(fullFileName))
        {
            status = STATUS_SUCCESS;
        }
    }

    PhClearReference(&fullFileName);
    PhClearReference(&commandString);

    return status;
}

NTSTATUS PhRunAsExecuteParentCommand(
    _In_ HWND WindowHandle,
    _In_ PCWSTR CommandLine,
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN CreateSuspendedProcess
    )
{
    NTSTATUS status;
    HANDLE processHandle = NULL;
    HANDLE newProcessHandle = NULL;
    STARTUPINFOEX startupInfo = { 0 };
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;
    PSECURITY_DESCRIPTOR processSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR tokenSecurityDescriptor = NULL;
    PVOID environment = NULL;
    HANDLE tokenHandle;
    ULONG flags = 0;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_PROCESS | (PhGetOwnTokenAttributes().Elevated ? PROCESS_QUERY_LIMITED_INFORMATION | READ_CONTROL : 0),
        ProcessId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInitializeProcThreadAttributeList(&attributeList, 1);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
        &processHandle,
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (PhGetOwnTokenAttributes().Elevated)
    {
        PhGetObjectSecurity(
            processHandle,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
            &processSecurityDescriptor
            );
    }

    if (NT_SUCCESS(PhOpenProcessToken(
        processHandle,
        TOKEN_QUERY | (PhGetOwnTokenAttributes().Elevated ? READ_CONTROL : 0),
        &tokenHandle
        )))
    {
        if (PhGetOwnTokenAttributes().Elevated)
        {
            PhGetObjectSecurity(
                tokenHandle,
                OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
                &tokenSecurityDescriptor
                );
        }

        if (NT_SUCCESS(PhCreateEnvironmentBlock(&environment, tokenHandle, FALSE)))
        {
            flags |= PH_CREATE_PROCESS_UNICODE_ENVIRONMENT;
        }

        NtClose(tokenHandle);
    }

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;
    startupInfo.lpAttributeList = attributeList;

    status = PhCreateProcessWin32Ex(
        NULL,
        CommandLine,
        environment,
        NULL,
        &startupInfo,
        PH_CREATE_PROCESS_SUSPENDED | PH_CREATE_PROCESS_NEW_CONSOLE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO | PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | flags,
        NULL,
        NULL,
        &newProcessHandle,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PSID userSid, logonSid;
        
        if (PhRunAsGetLogonSid(newProcessHandle, &userSid, &logonSid))
        {
            status = PhRunAsUpdateDesktop(userSid);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhRunAsUpdateWindowStation(userSid, logonSid);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (PhGetOwnTokenAttributes().Elevated)
        {
            // Note: This is needed to workaround a severe bug with PROC_THREAD_ATTRIBUTE_PARENT_PROCESS
            // where the process and token security descriptors are created without an ACE for the current user,
            // owned by the wrong user and with a High-IL when the process token is Medium-IL
            // preventing the new process from accessing user/system resources above Low-IL. (dmex)

            if (processSecurityDescriptor)
            {
                PhSetObjectSecurity(
                    newProcessHandle,
                    OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
                    processSecurityDescriptor
                    );
            }

            if (tokenSecurityDescriptor && NT_SUCCESS(PhOpenProcessToken(
                newProcessHandle,
                WRITE_DAC | WRITE_OWNER,
                &tokenHandle
                )))
            {
                PhSetObjectSecurity(
                    tokenHandle,
                    OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
                    tokenSecurityDescriptor
                    );
                NtClose(tokenHandle);
            }
        }

        if (!CreateSuspendedProcess)
        {
            if (NT_SUCCESS(PhGetProcessBasicInformation(newProcessHandle, &basicInfo)))
            {
                AllowSetForegroundWindow(HandleToUlong(basicInfo.UniqueProcessId));
            }

            PhConsoleSetForeground(newProcessHandle, TRUE);

            NtResumeProcess(newProcessHandle);
        }
    }

CleanupExit:
    if (newProcessHandle)
    {
        NtClose(newProcessHandle);
    }

    if (environment)
    {
        PhDestroyEnvironmentBlock(environment);
    }

    if (tokenSecurityDescriptor)
    {
        PhFree(tokenSecurityDescriptor);
    }

    if (processSecurityDescriptor)
    {
        PhFree(processSecurityDescriptor);
    }

    if (attributeList)
    {
        PhDeleteProcThreadAttributeList(attributeList);
    }

    if (processHandle)
    {
        NtClose(processHandle);
    }

    return status;
}

VOID PhRunAsExecuteCommmand(
    _In_ PRUNAS_DIALOG_CONTEXT Context,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    BOOLEAN useLinkedToken;
    BOOLEAN createSuspended;
    BOOLEAN createUIAccess;
    ULONG currentSessionId = ULONG_MAX;
    ULONG logonType = ULONG_MAX;
    ULONG sessionId = ULONG_MAX;
    PPH_STRING program = NULL;
    PPH_STRING username = NULL;
    PPH_STRING password = NULL;
    PPH_STRING logonTypeString;
    PPH_STRING desktopName = NULL;
    INT selectionIndex = CB_ERR;

    program = PH_AUTO(PhGetWindowText(Context->ProgramComboBoxWindowHandle));
    username = PH_AUTO(PhGetWindowText(Context->UserComboBoxWindowHandle));
    logonTypeString = PH_AUTO(PhGetWindowText(Context->TypeComboBoxWindowHandle));
    useLinkedToken = Button_GetCheck(GetDlgItem(Context->WindowHandle, IDC_TOGGLEELEVATION)) == BST_CHECKED;
    createSuspended = Button_GetCheck(GetDlgItem(Context->WindowHandle, IDC_TOGGLESUSPENDED)) == BST_CHECKED;
    createUIAccess = Button_GetCheck(GetDlgItem(Context->WindowHandle, IDC_TOGGLEUIACCESS)) == BST_CHECKED;

    if (PhIsNullOrEmptyString(program))
        return;

    if ((selectionIndex = ComboBox_GetCurSel(Context->SessionEditWindowHandle)) != CB_ERR)
    {
        PPH_RUNAS_SESSION_ITEM sessionEntry;

        if (sessionEntry = (PPH_RUNAS_SESSION_ITEM)ComboBox_GetItemData(Context->SessionEditWindowHandle, selectionIndex))
        {
            sessionId = sessionEntry->SessionId;
        }
    }

    if ((selectionIndex = ComboBox_GetCurSel(Context->DesktopEditWindowHandle)) != CB_ERR)
    {
        PPH_RUNAS_DESKTOP_ITEM desktopEntry;

        if (desktopEntry = (PPH_RUNAS_DESKTOP_ITEM)ComboBox_GetItemData(Context->DesktopEditWindowHandle, selectionIndex))
        {
            desktopName = desktopEntry->DesktopName;
        }
    }

    if (selectionIndex == CB_ERR)
        return;
    if (sessionId == ULONG_MAX)
        return;

    // Fix up the user name if it doesn't have a domain.
    if (PhFindCharInString(username, 0, L'\\') == SIZE_MAX)
    {
        PSID sid;
        PPH_STRING newUserName;

        if (NT_SUCCESS(PhLookupName(&username->sr, &sid, NULL, NULL)))
        {
            if (newUserName = PH_AUTO(PhGetSidFullName(sid, TRUE, NULL)))
                PhSwapReference(&username, newUserName);

            PhFree(sid);
        }
    }

    //if (IsCurrentUserAccount(username))
    //{
    //    status = PhCreateProcessWin32(
    //        NULL,
    //        program->Buffer,
    //        NULL,
    //        NULL,
    //        0,
    //        NULL,
    //        NULL,
    //        NULL
    //        );
    //}

    {
        PSID userSid;

        if (NT_SUCCESS(PhLookupName(
            &username->sr,
            &userSid,
            NULL,
            NULL
            )))
        {
            status = PhRunAsUpdateDesktop(userSid);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhRunAsUpdateWindowStation(userSid, NULL);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }
    }

    if (!PhFindIntegerSiKeyValuePairs(
        PhpLogonTypePairs,
        sizeof(PhpLogonTypePairs),
        logonTypeString->Buffer,
        &logonType
        ))
    {
        PhShowStatus(Context->WindowHandle, L"Unable to start the program.", STATUS_INVALID_PARAMETER, 0);
        return;
    }

    if (!IsServiceAccount(username))
    {
        password = PhGetWindowText(Context->PasswordEditWindowHandle);
        PhSetWindowText(Context->PasswordEditWindowHandle, L"");
    }

    PhGetProcessSessionId(NtCurrentProcess(), &currentSessionId);

    if (
        logonType == LOGON32_LOGON_INTERACTIVE &&
        !ProcessId &&
        sessionId == currentSessionId &&
        !useLinkedToken
        )
    {
        // We are eligible to load the user profile.
        // This must be done here, not in the service, because
        // we need to be in the target session.

        PH_CREATE_PROCESS_AS_USER_INFO createInfo;
        PPH_STRING domainPart = NULL;
        PPH_STRING userPart = NULL;
        HANDLE newProcessHandle;

        PhpSplitUserName(username->Buffer, &domainPart, &userPart);

        memset(&createInfo, 0, sizeof(PH_CREATE_PROCESS_AS_USER_INFO));
        createInfo.CommandLine = PhGetString(program);
        createInfo.UserName = PhGetString(userPart);
        createInfo.DomainName = PhGetString(domainPart);
        createInfo.Password = PhGetStringOrEmpty(password);

        // Whenever we can, try not to set the desktop name; it breaks a lot of things.
        if (!PhIsNullOrEmptyString(desktopName) && !PhEqualString2(desktopName, L"WinSta0\\Default", TRUE))
            createInfo.DesktopName = PhGetString(desktopName);

        status = PhCreateProcessAsUser(
            &createInfo,
            PH_CREATE_PROCESS_WITH_PROFILE | PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | PH_CREATE_PROCESS_SUSPENDED,
            NULL,
            NULL,
            &newProcessHandle,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            PROCESS_BASIC_INFORMATION basicInfo;
            PSID userSid, logonSid;

            if (PhRunAsGetLogonSid(newProcessHandle, &userSid, &logonSid))
            {
                status = PhRunAsUpdateDesktop(userSid);

                if (!NT_SUCCESS(status))
                    goto CleanupExit;

                status = PhRunAsUpdateWindowStation(userSid, logonSid);

                if (!NT_SUCCESS(status))
                    goto CleanupExit;
            }

            if (!createSuspended)
            {
                if (NT_SUCCESS(PhGetProcessBasicInformation(newProcessHandle, &basicInfo)))
                {
                    AllowSetForegroundWindow(HandleToUlong(basicInfo.UniqueProcessId));
                }

                PhConsoleSetForeground(newProcessHandle, TRUE);

                NtResumeProcess(newProcessHandle);
            }

            NtClose(newProcessHandle);
        }

        PhClearReference(&domainPart);
        PhClearReference(&userPart);
    }
    else
    {
        if (ProcessId)
        {
            status = PhRunAsExecutionAlias(program);

            if (!NT_SUCCESS(status))
            {
                status = PhRunAsExecuteParentCommand(
                    Context->WindowHandle,
                    PhGetString(program),
                    ProcessId,
                    createSuspended
                    );
            }
        }
        else
        {
            status = PhExecuteRunAsCommand3(
                Context->WindowHandle,
                PhGetString(program),
                PhGetString(username),
                PhGetStringOrEmpty(password),
                logonType,
                ProcessId,
                sessionId,
                PhGetString(desktopName),
                useLinkedToken,
                createSuspended,
                createUIAccess
                );
        }
    }

CleanupExit:
    if (password)
    {
        RtlSecureZeroMemory(password->Buffer, password->Length);
        PhDereferenceObject(password);
    }

    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_CANCELLED)
        {
            if (status == STATUS_NOT_IMPLEMENTED)
            {
                PhShowError2(
                    Context->WindowHandle,
                    L"Unable to start the program.",
                    L"%s",
                    L"Unable to start the execution alias with a process token."
                    );
            }
            else
            {
                PhShowStatus(
                    Context->WindowHandle,
                    L"Unable to start the program.",
                    status,
                    0
                    );
            }
        }
    }
    else if (status != STATUS_TIMEOUT)
    {
        PhRecentListAddCommand(&program->sr);
        //PhSetStringSetting2(SETTING_RUN_AS_PROGRAM, &program->sr);
        PhSetStringSetting2(SETTING_RUN_AS_USER_NAME, &username->sr);
        EndDialog(Context->WindowHandle, IDOK);
    }
}

INT_PTR CALLBACK PhpRunAsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PRUNAS_DIALOG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(RUNAS_DIALOG_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ProgramComboBoxWindowHandle = GetDlgItem(hwndDlg, IDC_PROGRAMCOMBO);
            context->SessionEditWindowHandle = GetDlgItem(hwndDlg, IDC_SESSIONCOMBO);
            context->DesktopEditWindowHandle = GetDlgItem(hwndDlg, IDC_DESKTOPCOMBO);
            context->TypeComboBoxWindowHandle = GetDlgItem(hwndDlg, IDC_TYPE);
            context->UserComboBoxWindowHandle = GetDlgItem(hwndDlg, IDC_USERNAME);
            context->PasswordEditWindowHandle = GetDlgItem(hwndDlg, IDC_PASSWORD);
            context->ProcessId = (HANDLE)lParam;

            PhSetApplicationWindowIcon(hwndDlg);

            if (PhValidWindowPlacementFromSetting(SETTING_RUN_AS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_RUN_AS_WINDOW_POSITION, NULL, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (PhGetIntegerSetting(SETTING_RUN_AS_ENABLE_AUTO_COMPLETE))
            {
                COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

                if (SendMessage(context->ProgramComboBoxWindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
                {
                    if (SHAutoComplete_Import())
                        SHAutoComplete_Import()(info.hwndItem, SHACF_DEFAULT);
                }
            }

            ComboBox_AddString(context->TypeComboBoxWindowHandle, L"Batch");
            ComboBox_AddString(context->TypeComboBoxWindowHandle, L"Interactive");
            ComboBox_AddString(context->TypeComboBoxWindowHandle, L"Network");
            ComboBox_AddString(context->TypeComboBoxWindowHandle, L"New credentials");
            ComboBox_AddString(context->TypeComboBoxWindowHandle, L"Service");
            PhSelectComboBoxString(context->TypeComboBoxWindowHandle, L"Interactive", FALSE);

            PhpAddProgramsToComboBox(context->ProgramComboBoxWindowHandle);
            PhpAddAccountsToComboBox(context->UserComboBoxWindowHandle);
            PhpAddSessionsToComboBox(context->SessionEditWindowHandle);
            PhpAddDesktopsToComboBox(context->DesktopEditWindowHandle);

            SetDefaultProgramEntry(context->ProgramComboBoxWindowHandle);
            SetDefaultUserEntry(context, hwndDlg, context->UserComboBoxWindowHandle);
            SetDefaultSessionEntry(context->SessionEditWindowHandle);
            SetDefaultDesktopEntry(context, context->DesktopEditWindowHandle);

            PhSetDialogFocus(hwndDlg, context->ProgramComboBoxWindowHandle);
            Edit_SetSel(context->ProgramComboBoxWindowHandle, -1, -1);

            //if (!PhGetOwnTokenAttributes().Elevated)
            //    Button_SetElevationRequiredState(GetDlgItem(hwndDlg, IDOK), TRUE);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(SETTING_RUN_AS_WINDOW_POSITION, NULL, hwndDlg);

            PhpFreeDesktopsComboBox(context->DesktopEditWindowHandle);
            PhpFreeSessionsComboBox(context->SessionEditWindowHandle);
            PhDeleteComboBoxStrings(context->UserComboBoxWindowHandle, FALSE);
            PhDeleteComboBoxStrings(context->ProgramComboBoxWindowHandle, FALSE);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case CBN_DROPDOWN:
                {
                    if (GET_WM_COMMAND_HWND(wParam, lParam) == context->UserComboBoxWindowHandle)
                    {
                        //PhpAddAccountsToComboBox(context->UserComboBoxWindowHandle);
                    }

                    if (GET_WM_COMMAND_HWND(wParam, lParam) == context->SessionEditWindowHandle)
                    {
                        PhpAddSessionsToComboBox(context->SessionEditWindowHandle);
                        SetDefaultSessionEntry(context->SessionEditWindowHandle);
                    }

                    if (GET_WM_COMMAND_HWND(wParam, lParam) == context->DesktopEditWindowHandle)
                    {
                        PhpAddDesktopsToComboBox(context->DesktopEditWindowHandle);
                        SetDefaultDesktopEntry(context, context->DesktopEditWindowHandle);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                PhRunAsExecuteCommmand(context, context->ProcessId);
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
                    PhSetFileDialogFileName(fileDialog, PH_AUTO_T(PH_STRING, PhGetWindowText(context->ProgramComboBoxWindowHandle))->Buffer);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        PPH_STRING fileName;

                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhSetWindowText(context->ProgramComboBoxWindowHandle, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_USERNAME:
                {
                    PPH_STRING username = NULL;

                    if (!context->ProcessId && GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        username = PH_AUTO(PhGetComboBoxString(context->UserComboBoxWindowHandle, INT_ERROR));
                    }
                    else if (!context->ProcessId && (
                        GET_WM_COMMAND_CMD(wParam, lParam) == CBN_EDITCHANGE ||
                        GET_WM_COMMAND_CMD(wParam, lParam) == CBN_CLOSEUP
                        ))
                    {
                        username = PH_AUTO(PhGetWindowText(context->UserComboBoxWindowHandle));
                    }

                    if (username)
                    {
                        if (IsServiceAccount(username))
                        {
                            EnableWindow(context->PasswordEditWindowHandle, FALSE);
                            PhSelectComboBoxString(context->TypeComboBoxWindowHandle, L"Service", FALSE);
                        }
                        else
                        {
                            EnableWindow(context->PasswordEditWindowHandle, TRUE);
                            PhSelectComboBoxString(context->TypeComboBoxWindowHandle, L"Interactive", FALSE);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

NTSTATUS PhRunAsUpdateDesktop(
    _In_ PSID UserSid
    )
{
    NTSTATUS status;
    HDESK desktopHandle;

    if (desktopHandle = OpenDesktop(
        L"Default",
        0,
        FALSE,
        READ_CONTROL | WRITE_DAC | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS
        ))
    {
        ULONG i;
        BOOLEAN currentDaclPresent;
        BOOLEAN currentDaclDefaulted;
        PACL currentDacl;
        PACE_HEADER currentAce;
        ULONG newDaclLength;
        PACL newDacl;
        SECURITY_DESCRIPTOR newSecurityDescriptor;
        PSECURITY_DESCRIPTOR currentSecurityDescriptor;

        status = PhGetObjectSecurity(
            desktopHandle,
            DACL_SECURITY_INFORMATION,
            &currentSecurityDescriptor
            );

        if (NT_SUCCESS(status))
        {
            if (!NT_SUCCESS(PhGetDaclSecurityDescriptor(
                currentSecurityDescriptor,
                &currentDaclPresent,
                &currentDacl,
                &currentDaclDefaulted
                )))
            {
                currentDaclPresent = FALSE;
            }

            newDaclLength = sizeof(ACL) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + PhLengthSid(UserSid);

            if (currentDaclPresent && currentDacl)
                newDaclLength += currentDacl->AclSize - sizeof(ACL);

            newDacl = PhAllocateStack(newDaclLength);

            if (!newDacl)
            {
                status = STATUS_NO_MEMORY;
                goto CleanupExit;
            }

            RtlZeroMemory(newDacl, newDaclLength);

            status = PhCreateAcl(newDacl, newDaclLength, ACL_REVISION);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            // Add the existing DACL entries.

            if (currentDaclPresent && currentDacl)
            {
                for (i = 0; i < currentDacl->AceCount; i++)
                {
                    if (NT_SUCCESS(PhGetAce(currentDacl, i, &currentAce)))
                    {
                        if (currentAce->AceType == ACCESS_ALLOWED_ACE_TYPE)
                        {
                            PSID aceSid = (PSID)&((PACCESS_ALLOWED_ACE)currentAce)->SidStart;

                            if (PhEqualSid(aceSid, UserSid))
                            {
                                if (((PACCESS_ALLOWED_ACE)currentAce)->Mask == DESKTOP_ALL_ACCESS)
                                    continue;
                            }
                        }

                        status = PhAddAce(
                            newDacl,
                            ACL_REVISION,
                            ULONG_MAX,
                            currentAce,
                            currentAce->AceSize
                            );

                        if (!NT_SUCCESS(status))
                            break;
                    }
                }
            }

            // Allow access for the user.

            if (NT_SUCCESS(status))
            {
                status = PhAddAccessAllowedAce(newDacl, ACL_REVISION, DESKTOP_ALL_ACCESS, UserSid);
            }

            // Set the security descriptor of the new token.

            if (NT_SUCCESS(status))
            {
                status = PhCreateSecurityDescriptor(&newSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
            }

            if (NT_SUCCESS(status))
            {
                status = PhSetDaclSecurityDescriptor(&newSecurityDescriptor, TRUE, newDacl, FALSE);
            }

            if (NT_SUCCESS(status))
            {
                assert(RtlValidSecurityDescriptor(&newSecurityDescriptor));

                status = PhSetObjectSecurity(desktopHandle, DACL_SECURITY_INFORMATION, &newSecurityDescriptor);
            }

            PhFreeStack(newDacl);
        }

    CleanupExit:
        CloseDesktop(desktopHandle);
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    return status;
}

NTSTATUS PhRunAsUpdateWindowStation(
    _In_opt_ PSID UserSid,
    _In_opt_ PSID LogonSid
    )
{
    NTSTATUS status;
    HWINSTA wsHandle;

    if (wsHandle = OpenWindowStation(
        L"WinSta0",
        FALSE,
        READ_CONTROL | WRITE_DAC
        ))
    {
        ULONG i;
        BOOLEAN currentDaclPresent;
        BOOLEAN currentDaclDefaulted;
        PACL currentDacl;
        PACE_HEADER currentAce;
        ULONG newDaclLength;
        PACL newDacl;
        SECURITY_DESCRIPTOR newSecurityDescriptor;
        PSECURITY_DESCRIPTOR currentSecurityDescriptor;

        status = PhGetObjectSecurity(
            wsHandle,
            DACL_SECURITY_INFORMATION,
            &currentSecurityDescriptor
            );

        if (NT_SUCCESS(status))
        {
            if (!NT_SUCCESS(PhGetDaclSecurityDescriptor(
                currentSecurityDescriptor,
                &currentDaclPresent,
                &currentDacl,
                &currentDaclDefaulted
                )))
            {
                currentDaclPresent = FALSE;
            }

            newDaclLength = (sizeof(ACL) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) * 3) +
                (UserSid ? PhLengthSid(UserSid) : 0) + (LogonSid ? PhLengthSid(LogonSid) : 0);

            if (currentDaclPresent && currentDacl)
                newDaclLength += currentDacl->AclSize - sizeof(ACL);

            newDacl = PhAllocate(newDaclLength);
            PhCreateAcl(newDacl, newDaclLength, ACL_REVISION);

            // Add the existing DACL entries.

            if (currentDaclPresent && currentDacl)
            {
                for (i = 0; i < currentDacl->AceCount; i++)
                {
                    if (NT_SUCCESS(PhGetAce(currentDacl, i, &currentAce)))
                    {
                        if (currentAce->AceType == ACCESS_ALLOWED_ACE_TYPE)
                        {
                            PSID aceSid = (PSID)&((PACCESS_ALLOWED_ACE)currentAce)->SidStart;

                            if (UserSid && PhEqualSid(aceSid, UserSid))
                            {
                                if (((PACCESS_ALLOWED_ACE)currentAce)->Mask == (WINSTA_ACCESSCLIPBOARD | WINSTA_ACCESSGLOBALATOMS))
                                    continue;
                            }

                            if (LogonSid && PhEqualSid(aceSid, LogonSid))
                            {
                                if (((PACCESS_ALLOWED_ACE)currentAce)->Mask == WINSTA_ALL_ACCESS)
                                    continue;
                            }
                        }

                        status = PhAddAce(
                            newDacl,
                            ACL_REVISION,
                            ULONG_MAX,
                            currentAce,
                            currentAce->AceSize
                            );

                        if (!NT_SUCCESS(status))
                            break;
                    }
                }
            }

            if (NT_SUCCESS(status))
            {
                if (UserSid)
                {
                    PhAddAccessAllowedAce(
                        newDacl,
                        ACL_REVISION,
                        WINSTA_ACCESSCLIPBOARD | WINSTA_ACCESSGLOBALATOMS,
                        UserSid
                        );

                    //PhAddAccessAllowedAce(
                    //    newDacl,
                    //    ACL_REVISION,
                    //    WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ACCESSGLOBALATOMS |
                    //    WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | READ_CONTROL,
                    //    UserSid
                    //    );
                }

                if (UserSid)
                {
                    PhAddAccessAllowedAceEx(
                        newDacl,
                        ACL_REVISION,
                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
                        GENERIC_ALL,
                        LogonSid
                        );
                    PhAddAccessAllowedAceEx(
                        newDacl,
                        ACL_REVISION,
                        NO_PROPAGATE_INHERIT_ACE,
                        WINSTA_ALL_ACCESS,
                        LogonSid
                        );
                }

                // Set the security descriptor of the new token.

                status = PhCreateSecurityDescriptor(&newSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
            }

            if (NT_SUCCESS(status))
            {
                status = PhSetDaclSecurityDescriptor(&newSecurityDescriptor, TRUE, newDacl, FALSE);
            }

            if (NT_SUCCESS(status))
            {
                assert(RtlValidSecurityDescriptor(&newSecurityDescriptor));

                status = PhSetObjectSecurity(wsHandle, DACL_SECURITY_INFORMATION, &newSecurityDescriptor);
            }
        }

        CloseWindowStation(wsHandle);
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    return status;
}

/**
 * Sets the access control lists of the current window station
 * and desktop to allow all access.
 */
//NTSTATUS PhSetDesktopWinStaAccess(
//    _In_ HWND WindowHandle
//    )
//{
//    HWINSTA wsHandle;
//    HDESK desktopHandle;
//    ULONG allocationLength;
//    PSID allAppPackagesSid = PhSeAnyPackageSid();
//    UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + 0x50];
//    PSECURITY_DESCRIPTOR securityDescriptor;
//    PACL dacl;
//
//    if (!PhStartupParameters.RunAsServiceMode && WindowHandle && PhGetIntegerSetting(SETTING_ENABLE_WARNINGS))
//    {
//        if (PhGetIntegerSetting(SETTING_ENABLE_WARNINGS_RUNAS) && PhShowMessageOneTime(
//            WindowHandle,
//            TD_YES_BUTTON | TD_NO_BUTTON,
//            TD_WARNING_ICON,
//            L"WARNING: This will grant Everyone access to the current window station and desktop.",
//            L"Are you sure you want to continue?"
//            ) == IDNO)
//        {
//            PhSetIntegerSetting(SETTING_ENABLE_WARNINGS_RUNAS, 0);
//            return STATUS_ACCESS_DENIED;
//        }
//    }
//
//    // TODO: Set security on the correct window station and desktop.
//
//    // We create a DACL that allows everyone to access everything.
//
//    allocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
//        (ULONG)sizeof(ACL) +
//        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
//        PhLengthSid(&PhSeEveryoneSid) +
//        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
//        PhLengthSid(allAppPackagesSid);
//
//    securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;
//    dacl = PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
//
//    PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
//    PhCreateAcl(dacl, allocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
//    PhAddAccessAllowedAce(dacl, ACL_REVISION, GENERIC_ALL, &PhSeEveryoneSid);
//
//    if (WindowsVersion >= WINDOWS_8)
//    {
//        PhAddAccessAllowedAce(dacl, ACL_REVISION, GENERIC_ALL, allAppPackagesSid);
//    }
//
//    PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);
//
//    if (wsHandle = OpenWindowStation(
//        L"WinSta0",
//        FALSE,
//        WRITE_DAC
//        ))
//    {
//        PhSetObjectSecurity(wsHandle, DACL_SECURITY_INFORMATION, securityDescriptor);
//        CloseWindowStation(wsHandle);
//    }
//    else
//    {
//        return PhGetLastWin32ErrorAsNtStatus();
//    }
//
//    if (desktopHandle = OpenDesktop(
//        L"Default",
//        0,
//        FALSE,
//        WRITE_DAC | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS
//        ))
//    {
//        PhSetObjectSecurity(desktopHandle, DACL_SECURITY_INFORMATION, securityDescriptor);
//        CloseDesktop(desktopHandle);
//    }
//    else
//    {
//        return PhGetLastWin32ErrorAsNtStatus();
//    }
//
//#ifdef DEBUG
//    assert(RtlValidSecurityDescriptor(securityDescriptor));
//    assert(allocationLength < sizeof(securityDescriptorBuffer));
//    assert(RtlLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));
//#endif
//
//    return STATUS_SUCCESS;
//}

PPH_STRING PhFormatRunAsCommand(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    PPH_STRING command;
    PPH_STRING fileName;
    PH_FORMAT format[5];

    if (!(fileName = PhGetApplicationFileNameWin32()))
        return NULL;

    // L"\"%s\" -ras \"%s\""
    PhInitFormatS(&format[0], L"\"");
    PhInitFormatSR(&format[1], fileName->sr);
    PhInitFormatS(&format[2], L"\" -ras \"");
    PhInitFormatS(&format[3], Parameters->ServiceName);
    PhInitFormatS(&format[4], L"\"");

    command = PhFormat(format, RTL_NUMBER_OF(format), 0);

    PhDereferenceObject(fileName);
    return command;
}

/**
 * Executes the run-as service.
 *
 * \param Parameters The run-as parameters.
 *
 * \remarks This function requires administrator-level access.
 */
NTSTATUS PhExecuteRunAsCommand(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PPH_STRING commandLine;
    SC_HANDLE serviceHandle;
    PPH_STRING portName;
    UNICODE_STRING portNameUs;
    ULONG attempts;

    if (!(commandLine = PhFormatRunAsCommand(Parameters)))
        return STATUS_UNSUCCESSFUL;

    status = PhCreateService(
        &serviceHandle,
        Parameters->ServiceName,
        Parameters->ServiceName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        PhGetString(commandLine),
        L"LocalSystem",
        L""
        );

    PhDereferenceObject(commandLine);

    if (!NT_SUCCESS(status))
        return status;

    PhStartService(serviceHandle, 0, NULL);
    PhDeleteService(serviceHandle);

    portName = PhConcatStrings2(L"\\BaseNamedObjects\\", Parameters->ServiceName);
    PhStringRefToUnicodeString(&portName->sr, &portNameUs);
    attempts = 50;

    // Try to connect several times because the server may take
    // a while to initialize.
    do
    {
        status = PhSvcConnectToServer(&portNameUs, 0);

        if (NT_SUCCESS(status))
            break;

        PhDelayExecution(100);

    } while (--attempts != 0);

    PhDereferenceObject(portName);

    if (NT_SUCCESS(status))
    {
        status = PhSvcCallInvokeRunAsService(Parameters);
        PhSvcDisconnectFromServer();
    }

    PhCloseServiceHandle(serviceHandle);

    return status;
}

/**
 * Starts a program as another user.
 *
 * \param WindowHandle A handle to the parent window.
 * \param CommandLine The command line of the program to start.
 * \param UserName The user to start the program as. The username should be specified as: domain\\name.
 *        This parameter can be NULL if \a ProcessIdWithToken is specified.
 * \param Password The password for the specified user. If there is no password, specify an empty string.
 *        This parameter can be NULL if \a ProcessIdWithToken is specified.
 * \param LogonType The logon type for the specified user. This
 *        parameter can be 0 if \a ProcessIdWithToken is specified.
 * \param ProcessIdWithToken The ID of a process from which to duplicate the token.
 * \param SessionId The ID of the session to run the program under.
 * \param DesktopName The window station and desktop to run the program under.
 * \param UseLinkedToken Uses the linked token if possible.
 *
 * \retval STATUS_CANCELLED The user cancelled the operation.
 *
 * \remarks This function will cause another instance of System Informer to be executed if the current security context
 * does not have sufficient system access. This is done through a UAC elevation prompt.
 */
NTSTATUS PhExecuteRunAsCommand2(
    _In_ HWND WindowHandle,
    _In_ PCWSTR CommandLine,
    _In_opt_ PCWSTR UserName,
    _In_opt_ PCWSTR Password,
    _In_opt_ ULONG LogonType,
    _In_opt_ HANDLE ProcessIdWithToken,
    _In_opt_ ULONG SessionId,
    _In_opt_ PCWSTR DesktopName,
    _In_ BOOLEAN UseLinkedToken
    )
{
    return PhExecuteRunAsCommand3(
        WindowHandle,
        CommandLine,
        UserName,
        Password,
        LogonType,
        ProcessIdWithToken,
        SessionId,
        DesktopName,
        UseLinkedToken,
        FALSE,
        FALSE
        );
}

NTSTATUS PhExecuteRunAsCommand3(
    _In_ HWND WindowHandle,
    _In_ PCWSTR CommandLine,
    _In_opt_ PCWSTR UserName,
    _In_opt_ PCWSTR Password,
    _In_opt_ ULONG LogonType,
    _In_opt_ HANDLE ProcessIdWithToken,
    _In_opt_ ULONG SessionId,
    _In_opt_ PCWSTR DesktopName,
    _In_ BOOLEAN UseLinkedToken,
    _In_ BOOLEAN CreateSuspendedProcess,
    _In_ BOOLEAN CreateUIAccessProcess
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PH_RUNAS_SERVICE_PARAMETERS parameters;
    WCHAR serviceName[32];
    PPH_STRING portName;
    UNICODE_STRING portNameUs;

    memset(&parameters, 0, sizeof(PH_RUNAS_SERVICE_PARAMETERS));
    parameters.ProcessId = HandleToUlong(ProcessIdWithToken);
    parameters.UserName = UserName;
    parameters.Password = Password;
    parameters.LogonType = LogonType;
    parameters.SessionId = SessionId;
    parameters.CommandLine = CommandLine;
    parameters.DesktopName = DesktopName;
    parameters.UseLinkedToken = UseLinkedToken;
    parameters.CreateSuspendedProcess = CreateSuspendedProcess;
    parameters.WindowHandle = WindowHandle;
    parameters.CreateUIAccessProcess = CreateUIAccessProcess;

    // Try to use an existing instance of the service if possible.
    if (RunAsOldServiceName[0] != UNICODE_NULL)
    {
        PhAcquireQueuedLockExclusive(&RunAsOldServiceLock);

        portName = PhConcatStrings2(L"\\BaseNamedObjects\\", RunAsOldServiceName);

        if (!PhStringRefToUnicodeString(&portName->sr, &portNameUs))
        {
            PhDereferenceObject(portName);
            PhReleaseQueuedLockExclusive(&RunAsOldServiceLock);
            return STATUS_NAME_TOO_LONG;
        }

        status = PhSvcConnectToServer(&portNameUs, 0);

        if (NT_SUCCESS(status))
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

    memset(serviceName, 0, sizeof(serviceName));
    memcpy(serviceName, L"SystemInformer", 14 * sizeof(WCHAR));
    PhGenerateRandomAlphaString(&serviceName[14], ARRAYSIZE(serviceName) - 14);
    PhAcquireQueuedLockExclusive(&RunAsOldServiceLock);
    memcpy(RunAsOldServiceName, serviceName, sizeof(serviceName) - sizeof(UNICODE_NULL));
    PhReleaseQueuedLockExclusive(&RunAsOldServiceLock);

    parameters.ServiceName = serviceName;

    if (PhGetOwnTokenAttributes().Elevated)
    {
        status = PhExecuteRunAsCommand(&parameters);
    }
    else
    {
        if (PhUiConnectToPhSvc(WindowHandle, FALSE))
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

VOID PhpSplitUserName(
    _In_ PCWSTR UserName,
    _Out_opt_ PPH_STRING *DomainPart,
    _Out_opt_ PPH_STRING *UserPart
    )
{
    PH_STRINGREF userName;
    PH_STRINGREF domainPart;
    PH_STRINGREF userPart;

    PhInitializeStringRefLongHint(&userName, UserName);

    if (PhSplitStringRefAtChar(&userName, OBJ_NAME_PATH_SEPARATOR, &domainPart, &userPart))
    {
        if (DomainPart)
            *DomainPart = PhCreateString2(&domainPart);
        if (UserPart)
            *UserPart = PhCreateString2(&userPart);
    }
    else
    {
        if (DomainPart)
            *DomainPart = NULL;
        if (UserPart)
            *UserPart = PhCreateString2(&userName);
    }
}

static VOID SetRunAsServiceStatus(
    _In_ ULONG State
    )
{
    SERVICE_STATUS status;

    memset(&status, 0, sizeof(SERVICE_STATUS));
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = State;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    SetServiceStatus(RunAsServiceStatusHandle, &status);
}

static ULONG WINAPI RunAsServiceHandlerEx(
    _In_ ULONG dwControl,
    _In_ ULONG dwEventType,
    _In_ PVOID lpEventData,
    _In_ PVOID lpContext
    )
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        PhSvcStop(&RunAsServiceStop);
        return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

static VOID WINAPI RunAsServiceMain(
    _In_ ULONG dwArgc,
    _In_ PWSTR *lpszArgv
    )
{
    PPH_STRING portName;

    memset(&RunAsServiceStop, 0, sizeof(RunAsServiceStop));

    RunAsServiceStatusHandle = RegisterServiceCtrlHandlerEx(RunAsServiceName->Buffer, RunAsServiceHandlerEx, NULL);
    SetRunAsServiceStatus(SERVICE_RUNNING);

    portName = PhConcatStrings2(
        L"\\BaseNamedObjects\\",
        RunAsServiceName->Buffer
        );

    PhSvcMain(portName, &RunAsServiceStop);

    SetRunAsServiceStatus(SERVICE_STOPPED);
}

NTSTATUS PhRunAsServiceStart(
    _In_ PPH_STRING ServiceName
    )
{
    const SERVICE_TABLE_ENTRY serviceDispatchTable[] =
    {
        { PhGetString(ServiceName), RunAsServiceMain },
        { NULL, NULL }
    };
    HANDLE tokenHandle;

    // Enable some required privileges.

    if (NT_SUCCESS(PhOpenProcessToken(
        NtCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES,
        &tokenHandle
        )))
    {
        const LUID_AND_ATTRIBUTES privileges[] =
        {
            { RtlConvertUlongToLuid(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_INCREASE_QUOTA_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_BACKUP_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_RESTORE_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_IMPERSONATE_PRIVILEGE), SE_PRIVILEGE_ENABLED },
        };
        UCHAR privilegesBuffer[FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(privileges)];
        PTOKEN_PRIVILEGES tokenPrivileges;

        tokenPrivileges = (PTOKEN_PRIVILEGES)privilegesBuffer;
        tokenPrivileges->PrivilegeCount = RTL_NUMBER_OF(privileges);
        memcpy(tokenPrivileges->Privileges, privileges, sizeof(privileges));

        NtAdjustPrivilegesToken(
            tokenHandle,
            FALSE,
            tokenPrivileges,
            0,
            NULL,
            NULL
            );

        NtClose(tokenHandle);
    }

    RunAsServiceName = ServiceName;

    StartServiceCtrlDispatcher(serviceDispatchTable);

    return STATUS_SUCCESS;
}

NTSTATUS PhInvokeRunAsService(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PPH_STRING domainName;
    PPH_STRING userName;
    PH_CREATE_PROCESS_AS_USER_INFO createInfo;
    HANDLE newProcessHandle = NULL;
    ULONG flags;

    //status = PhSetDesktopWinStaAccess(Parameters->WindowHandle);

    //if (!NT_SUCCESS(status))
    //    return status;

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

    flags = PH_CREATE_PROCESS_SET_SESSION_ID | PH_CREATE_PROCESS_DEFAULT_ERROR_MODE;

    if (Parameters->ProcessId)
    {
        createInfo.ProcessIdWithToken = UlongToHandle(Parameters->ProcessId);
        flags |= PH_CREATE_PROCESS_USE_PROCESS_TOKEN;
    }

    if (Parameters->UseLinkedToken)
        flags |= PH_CREATE_PROCESS_USE_LINKED_TOKEN;
    //if (Parameters->CreateSuspendedProcess)
    //    flags |= PH_CREATE_PROCESS_SUSPENDED;
    if (Parameters->CreateUIAccessProcess)
        flags |= PH_CREATE_PROCESS_SET_UIACCESS;

    status = PhCreateProcessAsUser(
        &createInfo,
        flags | PH_CREATE_PROCESS_SUSPENDED,
        NULL,
        NULL,
        &newProcessHandle,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PSID userSid, logonSid;

        if (PhRunAsGetLogonSid(newProcessHandle, &userSid, &logonSid))
        {
            status = PhRunAsUpdateDesktop(userSid);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhRunAsUpdateWindowStation(userSid, logonSid);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (!Parameters->CreateSuspendedProcess)
        {
            status = PhGetProcessBasicInformation(newProcessHandle, &basicInfo);

            if (NT_SUCCESS(status))
            {
                AllowSetForegroundWindow(HandleToUlong(basicInfo.UniqueProcessId));
            }

            PhConsoleSetForeground(newProcessHandle, TRUE);

            PhResumeProcess(newProcessHandle);
        }
    }

CleanupExit:
    if (newProcessHandle) NtClose(newProcessHandle);
    if (domainName) PhDereferenceObject(domainName);
    if (userName) PhDereferenceObject(userName);

    return status;
}

typedef struct _PHP_RUNFILEDLG
{
    HWND WindowHandle;
    HWND ComboBoxHandle;
    HWND RunAsCheckboxHandle;
    HWND RunAsInstallerCheckboxHandle;
    HIMAGELIST ImageListHandle;
    BOOLEAN RunAsInstallerCheckboxDisabled;
    LONG WindowDpi;
} PHP_RUNFILEDLG, *PPHP_RUNFILEDLG;

PPH_STRING PhpQueryRunFileParentDirectory(
    _In_ BOOLEAN Elevated
    )
{
    // Note: Explorer creates new processes with the parent directory as SystemRoot when elevated or
    // the below environment variables when not elevated. (dmex)
    if (!Elevated)
    {
        static CONST PH_STRINGREF homeDriveNameSr = PH_STRINGREF_INIT(L"HOMEDRIVE");
        static CONST PH_STRINGREF homePathNameSr = PH_STRINGREF_INIT(L"HOMEPATH");
        PPH_STRING parentDirectoryString = NULL;
        PPH_STRING homeDriveNameString = NULL;
        PPH_STRING homePathNameString = NULL;

        PhQueryEnvironmentVariable(NULL, &homeDriveNameSr, &homeDriveNameString);
        PhQueryEnvironmentVariable(NULL, &homePathNameSr, &homePathNameString);

        if (homeDriveNameString && homePathNameString)
        {
            parentDirectoryString = PhConcatStringRef2(
                &homeDriveNameString->sr,
                &homePathNameString->sr
                );
        }

        if (homeDriveNameString)
            PhDereferenceObject(homeDriveNameString);
        if (homePathNameString)
            PhDereferenceObject(homePathNameString);

        return parentDirectoryString;
    }
    else
    {
        return PhGetSystemDirectory();
    }
}

BOOLEAN PhRunAsExecuteCommandPrompt(
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    BOOLEAN elevated;
    PPH_STRING commandFileName;
    PPH_STRING commandDirectory;

    elevated = !!PhGetOwnTokenAttributes().Elevated;
    commandFileName = PhGetSystemDirectoryWin32Z(L"\\cmd.exe");
    commandDirectory = PhpQueryRunFileParentDirectory(elevated);

    status = PhShellExecuteEx(
        WindowHandle,
        PhGetString(commandFileName),
        NULL,
        PhGetString(commandDirectory),
        SW_SHOW,
        PH_SHELL_EXECUTE_DEFAULT,
        0,
        NULL
        );

    PhClearReference(&commandDirectory);
    PhClearReference(&commandFileName);

    return NT_SUCCESS(status);
}

NTSTATUS PhRunAsShellExecute(
    _In_ HWND WindowHandle,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_ BOOLEAN Elevated
    )
{
    NTSTATUS status;
    PPH_STRING parentDirectory;

    parentDirectory = PhpQueryRunFileParentDirectory(Elevated);

    status = PhShellExecuteEx(
        WindowHandle,
        FileName,
        Parameters,
        PhGetString(parentDirectory),
        SW_SHOW,
        Elevated ? PH_SHELL_EXECUTE_ADMIN : PH_SHELL_EXECUTE_DEFAULT,
        0,
        NULL
        );

    PhClearReference(&parentDirectory);

    return status;
}

BOOLEAN PhpRunFileAsInteractiveUser(
    _In_ PPHP_RUNFILEDLG Context,
    _In_ PPH_STRING Command
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING executeString = NULL;
    PPH_STRING fileName = NULL;
    PPH_STRING fileArgs = NULL;
    PPH_LIST cmdlineArgList;

    // Extract the filename.
    if (cmdlineArgList = PhCommandLineToList(Command->Buffer))
    {
        fileName = PhReferenceObject(cmdlineArgList->Items[0]);

        if (cmdlineArgList->Count == 2)
        {
            fileArgs = PhReferenceObject(cmdlineArgList->Items[1]);
        }

        PhDereferenceObjects(cmdlineArgList->Items, cmdlineArgList->Count);
        PhDereferenceObject(cmdlineArgList);
    }

    if (fileName && !PhDoesFileExistWin32(PhGetString(fileName)))
    {
        PPH_STRING filePathString;

        // The user typed a name without a path so attempt to locate the executable.
        if (filePathString = PhSearchFilePath(PhGetString(fileName), L".exe"))
            PhMoveReference(&fileName, filePathString);
        else
            PhClearReference(&fileName);
    }

    if (fileName)
    {
        static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");
        static CONST PH_STRINGREF space = PH_STRINGREF_INIT(L" ");

        // Escape the filename.
        PhMoveReference(&fileName, PhConcatStringRef3(&separator, &fileName->sr, &separator));

        if (fileArgs)
        {
            // Escape the parameters.
            PhMoveReference(&fileArgs, PhConcatStringRef3(&separator, &fileArgs->sr, &separator));

            // Create the escaped execute string.
            executeString = PhConcatStringRef3(&fileName->sr, &space, &fileArgs->sr);

            // Cleanup.
            PhClearReference(&fileArgs);
        }
        else
        {
            executeString = fileName;
        }
    }

    if (!PhIsNullOrEmptyString(executeString))
    {
        PPH_STRING parentDirectory = PhpQueryRunFileParentDirectory(FALSE);

        if (PhCreateProcessAsInteractiveUser(PhGetString(executeString), PhGetString(parentDirectory)) == S_OK)
        {
            success = TRUE;
        }

        PhClearReference(&parentDirectory);
    }

    PhClearReference(&executeString);

    return success;
}

NTSTATUS PhpRunFileProgram(
    _In_ PPHP_RUNFILEDLG Context,
    _In_ PPH_STRING Command
    )
{
    NTSTATUS status;
    PPH_STRING commandString = NULL;
    PPH_STRING fullFileName = NULL;
    PPH_STRING argumentsString = NULL;
    PH_STRINGREF fileName;
    PH_STRINGREF arguments;
    FILE_BASIC_INFORMATION basicInfo;
    BOOLEAN isDirectory = FALSE;

    if (PhIsNullOrEmptyString(Command))
        return STATUS_UNSUCCESSFUL;

    if (!(commandString = PhExpandEnvironmentStrings(&Command->sr)))
        commandString = PhCreateString2(&Command->sr);

    PhParseCommandLineFuzzy(&commandString->sr, &fileName, &arguments, &fullFileName);

    if (PhIsNullOrEmptyString(fullFileName))
        PhMoveReference(&fullFileName, PhCreateString2(&fileName));

    if (PhIsNullOrEmptyString(fullFileName))
    {
        if (fullFileName)
            PhDereferenceObject(fullFileName);

        return STATUS_UNSUCCESSFUL;
    }

    if (arguments.Length)
    {
        argumentsString = PhCreateString2(&arguments);
    }

    if (NT_SUCCESS(PhQueryAttributesFileWin32(PhGetString(fullFileName), &basicInfo)))
    {
        isDirectory = !!FlagOn(basicInfo.FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }

    if (isDirectory || !PhDoesFileExistWin32(PhGetString(fullFileName)))
    {
        status = PhRunAsShellExecute(
            Context->WindowHandle,
            PhGetString(commandString),
            NULL,
            FALSE
            );
    }
    else if (Button_GetCheck(Context->RunAsCheckboxHandle) == BST_CHECKED ||
        // The Windows run dialog executes programs with elevation when
        // holding the ctrl + shift keys and selecting the OK button. (dmex)
        (!!(GetKeyState(VK_CONTROL) < 0 && !!(GetKeyState(VK_SHIFT) < 0))))
    {
        status = PhRunAsShellExecute(
            Context->WindowHandle,
            PhGetString(fullFileName),
            PhGetString(argumentsString),
            TRUE
            );
    }
    else
    {
        status = PhRunAsShellExecute(
            Context->WindowHandle,
            PhGetString(fullFileName),
            PhGetString(argumentsString),
            FALSE
            );

        if (status == STATUS_ELEVATION_REQUIRED)
        {
            status = PhRunAsShellExecute(
                Context->WindowHandle,
                PhGetString(fullFileName),
                PhGetString(argumentsString),
                TRUE
                );
        }
    }

    if (fullFileName) PhDereferenceObject(fullFileName);
    if (argumentsString) PhDereferenceObject(argumentsString);
    if (commandString) PhDereferenceObject(commandString);

    return status;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS RunAsCreateProcessThread(
    _In_ PVOID Parameter
    )
{
    PPH_STRING command = Parameter;
    NTSTATUS status;
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;
    SERVICE_STATUS_PROCESS serviceStatus = { 0 };
    SC_HANDLE serviceHandle = NULL;
    HANDLE processHandle = NULL;
    HANDLE newProcessHandle;
    STARTUPINFOEX startupInfo;
    PPH_STRING commandLine = NULL;
    PPH_STRING filePathString;

    if (filePathString = PhSearchFilePath(command->Buffer, L".exe"))
        PhMoveReference(&commandLine, filePathString);
    else
        PhSetReference(&commandLine, command);

    if (!NT_SUCCESS(status = PhOpenService(&serviceHandle, SERVICE_QUERY_STATUS | SERVICE_START, L"TrustedInstaller")))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhQueryServiceStatus(serviceHandle, &serviceStatus)))
        goto CleanupExit;

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        ULONG attempts = 10;

        PhStartService(serviceHandle, 0, NULL);

        do
        {
            status = PhQueryServiceStatus(serviceHandle, &serviceStatus);

            if (NT_SUCCESS(status))
            {
                if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
                {
                    status = STATUS_SUCCESS;
                    break;
                }
            }

            PhDelayExecution(1000);

        } while (--attempts != 0);
    }

    if (!NT_SUCCESS(status))
    {
        status = STATUS_SERVICES_FAILED_AUTOSTART;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_CREATE_PROCESS, UlongToHandle(serviceStatus.dwProcessId))))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhInitializeProcThreadAttributeList(&attributeList, 1)))
        goto CleanupExit;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
        &processHandle,
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;
    startupInfo.lpAttributeList = attributeList;

    status = PhCreateProcessWin32Ex(
        NULL,
        PhGetString(commandLine),
        NULL,
        NULL,
        &startupInfo,
        PH_CREATE_PROCESS_NEW_CONSOLE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO | PH_CREATE_PROCESS_DEFAULT_ERROR_MODE,
        NULL,
        NULL,
        &newProcessHandle,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PROCESS_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetProcessBasicInformation(newProcessHandle, &basicInfo)))
        {
            AllowSetForegroundWindow(HandleToUlong(basicInfo.UniqueProcessId));
        }

        PhConsoleSetForeground(newProcessHandle, TRUE);
        NtClose(newProcessHandle);
    }

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

    if (serviceHandle)
        PhCloseServiceHandle(serviceHandle);

    if (attributeList)
    {
        PhDeleteProcThreadAttributeList(attributeList);
    }

    if (commandLine)
    {
        PhDereferenceObject(commandLine);
    }

    if (command)
    {
        PhDereferenceObject(command);
    }

    return status;
}

static VOID PhpRunFileSetImageList(
    _Inout_ PPHP_RUNFILEDLG Context
    )
{
    if (Context->ImageListHandle)
    {
        PhImageListDestroy(Context->ImageListHandle);
        Context->ImageListHandle = NULL;
    }

    Context->ImageListHandle = PhImageListCreate(
        PhGetSystemMetrics(SM_CXSMICON, Context->WindowDpi),
        PhGetSystemMetrics(SM_CYSMICON, Context->WindowDpi),
        ILC_MASK | ILC_COLOR32,
        1, 1
        );

    if (Context->ImageListHandle)
    {
        HBITMAP shieldBitmap;

        if (shieldBitmap = PhGetShieldBitmap(Context->WindowDpi, PhSmallIconSize.X, PhSmallIconSize.Y))
        {
            PhImageListAddBitmap(Context->ImageListHandle, shieldBitmap, NULL);
            DeleteBitmap(shieldBitmap);
        }
    }
}

INT_PTR CALLBACK PhpRunFileWndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_RUNFILEDLG context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_RUNFILEDLG));
        PhSetDialogContext(hwndDlg, context);
    }
    else
    {
        context = PhGetDialogContext(hwndDlg);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ComboBoxHandle = GetDlgItem(hwndDlg, IDC_PROGRAMCOMBO);
            context->RunAsCheckboxHandle = GetDlgItem(hwndDlg, IDC_TOGGLEELEVATION);
            context->RunAsInstallerCheckboxHandle = GetDlgItem(hwndDlg, IDC_TRUSTEDINSTALLER);
            context->WindowDpi = PhGetWindowDpi(hwndDlg);

            PhSetApplicationWindowIconEx(hwndDlg, context->WindowDpi);
            PhSetStaticWindowIcon(GetDlgItem(hwndDlg, IDC_FILEICON), context->WindowDpi);

            PhpAddProgramsToComboBox(context->ComboBoxHandle);
            ComboBox_SetCurSel(context->ComboBoxHandle, 0);

            if (PhGetIntegerSetting(SETTING_RUN_AS_ENABLE_AUTO_COMPLETE))
            {
                COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

                if (SendMessage(context->ComboBoxHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)& info))
                {
                    if (SHAutoComplete_Import() && info.hwndItem)
                        SHAutoComplete_Import()(info.hwndItem, SHACF_DEFAULT);
                }
            }

            Button_SetCheck(context->RunAsCheckboxHandle, PhGetIntegerSetting(SETTING_RUN_FILE_DLG_STATE) ? TRUE : FALSE);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                Button_Enable(context->RunAsInstallerCheckboxHandle, FALSE);
                context->RunAsInstallerCheckboxDisabled = TRUE;

                PhpRunFileSetImageList(context);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveDialogContext(hwndDlg);

            PhSetIntegerSetting(SETTING_RUN_FILE_DLG_STATE, Button_GetCheck(context->RunAsCheckboxHandle) == BST_CHECKED);

            PhImageListDestroy(context->ImageListHandle);

            PhDestroyWindowIcon(hwndDlg);
            PhDeleteStaticWindowIcon(GetDlgItem(hwndDlg, IDC_FILEICON));

            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            context->WindowDpi = PhGetWindowDpi(hwndDlg);

            PhSetApplicationWindowIconEx(hwndDlg, context->WindowDpi);
            PhSetStaticWindowIcon(GetDlgItem(hwndDlg, IDC_FILEICON), context->WindowDpi);

            PhpRunFileSetImageList(context);
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;

            if (PhEnableThemeSupport)
                break;

            SetBkMode(hdc, TRANSPARENT);

            return (INT_PTR)PhGetStockBrush(WHITE_BRUSH);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    NTSTATUS status;
                    PPH_STRING commandString;

                    if (commandString = PhGetWindowText(context->ComboBoxHandle))
                    {
                        if (Button_GetCheck(context->RunAsInstallerCheckboxHandle) == BST_CHECKED)
                        {
                            PhReferenceObject(commandString);
                            status = PhCreateThread2(RunAsCreateProcessThread, commandString);
                        }
                        else
                        {
                            status = PhpRunFileProgram(context, commandString);
                        }

                        if (NT_SUCCESS(status))
                        {
                            PhRecentListAddCommand(&commandString->sr);

                            EndDialog(hwndDlg, IDOK);
                        }
                        else
                        {
                            if (!(NT_NTWIN32(status) && WIN32_FROM_NTSTATUS(status) == ERROR_CANCELLED))
                            {
                                PhShowStatus(hwndDlg, L"Unable to execute the command.", status, 0);
                            }
                        }

                        PhDereferenceObject(commandString);
                    }
                }
                break;
            case IDC_BROWSE:
                {
                    PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Executable files (*.exe;*.pif;*.com;*.bat;*.cmd)", L"*.exe;*.pif;*.com;*.bat;*.cmd" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog = PhCreateOpenFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        PPH_STRING fileName;

                        if (fileName = PhGetFileDialogFileName(fileDialog))
                        {
                            ComboBox_SetText(context->ComboBoxHandle, PhGetString(fileName));
                            PhDereferenceObject(fileName);
                        }
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_TRUSTEDINSTALLER:
                {
                    EnableWindow(context->RunAsCheckboxHandle, Button_GetCheck(context->RunAsInstallerCheckboxHandle) == BST_UNCHECKED);
                }
                break;
            }
        }
        break;
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            if (!PhGetClientRect(hwndDlg, &clientRect))
                break;

            SetBkMode(hdc, TRANSPARENT);

            clientRect.bottom -= PhGetDpi(60, context->WindowDpi);
            FillRect(hdc, &clientRect, PhEnableThemeSupport ? PhThemeWindowBackgroundBrush : (HBRUSH)(COLOR_WINDOW + 1));

            clientRect.top = clientRect.bottom;
            clientRect.bottom = clientRect.top + PhGetDpi(60, context->WindowDpi);
            FillRect(hdc, &clientRect, PhEnableThemeSupport ? PhThemeWindowBackgroundBrush : (HBRUSH)(COLOR_3DFACE + 1));

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
        }
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            if (data->hwndFrom != context->RunAsInstallerCheckboxHandle || !context->RunAsInstallerCheckboxDisabled)
                break;

            switch (data->code)
            {
            case NM_CUSTOMDRAW:
                {
                    LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                    WCHAR className[MAX_PATH];

                    if (!GetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className)))
                        className[0] = UNICODE_NULL;

                    if (PhEqualStringZ(className, L"Button", FALSE))
                    {
                        ULONG buttonStyle = PhGetWindowStyle(customDraw->hdr.hwndFrom);

                        if (FlagOn(buttonStyle, BS_CHECKBOX) == BS_CHECKBOX)
                        {
                            switch (customDraw->dwDrawStage)
                            {
                            case CDDS_PREPAINT:
                                {
                                    PPH_STRING buttonText;
                                    LONG width;

                                    SetTextColor(customDraw->hdc, RGB(0, 0, 0));
                                    SetDCBrushColor(customDraw->hdc, RGB(0xff, 0xff, 0xff));
                                    FillRect(customDraw->hdc, &customDraw->rc, PhGetStockBrush(DC_BRUSH));

                                    if (buttonText = PhGetWindowText(customDraw->hdr.hwndFrom))
                                    {
                                        width = PhGetSystemMetrics(SM_CXSMICON, context->WindowDpi);

                                        customDraw->rc.left += width;
                                        DrawText(
                                            customDraw->hdc,
                                            buttonText->Buffer,
                                            (UINT)buttonText->Length / sizeof(WCHAR),
                                            &customDraw->rc,
                                            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX
                                            );
                                        customDraw->rc.left -= width;

                                        PhDereferenceObject(buttonText);
                                    }

                                    PhImageListDrawIcon(
                                        context->ImageListHandle,
                                        0,
                                        customDraw->hdc,
                                        customDraw->rc.left,
                                        customDraw->rc.top + 1, // offset
                                        ILD_TRANSPARENT,
                                        FALSE
                                        );

                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT);
                                    return TRUE;
                                }
                                break;
                            }

                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                            return TRUE;
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

typedef struct _PH_RUNAS_PACKAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ComboBoxHandle;
    HWND SearchBoxHandle;
    HWND TreeNewHandle;
    HIMAGELIST ImageListHandle;
    LONG WindowDpi;
    PH_LAYOUT_MANAGER LayoutManager;

    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    ULONG_PTR SearchMatchHandle;

    HFONT NormalFontHandle;
    HFONT TitleFontHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

} PH_RUNAS_PACKAGE_CONTEXT, *PPH_RUNAS_PACKAGE_CONTEXT;

typedef enum _PH_RUNASPACKAGE_TREE_COLUMN_ITEM
{
    PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME,
    PH_RUNASPACKAGE_TREE_COLUMN_ITEM_APPID,
    PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM
} PH_RUNASPACKAGE_TREE_COLUMN_ITEM;

typedef struct _PH_RUNASPACKAGE_TREE_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    PPH_STRING AppUserModelId;
    PPH_STRING DisplayName;
    PPH_STRING PackageInstallPath;
    PPH_STRING PackageFullName;
    PPH_STRING SmallLogoPath;

    INT IconIndex;

    PH_STRINGREF TextCache[PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM];
} PH_RUNASPACKAGE_TREE_ROOT_NODE, *PPH_RUNASPACKAGE_TREE_ROOT_NODE;

#pragma region RunAsPackage TreeList

#define SORT_FUNCTION(Column) PhRunAsPackageTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhRunAsPackageTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node1 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)_elem1; \
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node2 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_RUNAS_PACKAGE_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->DisplayName, node2->DisplayName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Version)
{
    sortResult = PhCompareStringWithNull(node1->AppUserModelId, node2->AppUserModelId, TRUE);
}
END_SORT_FUNCTION

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN PhRunAsPackageNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node1 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)Entry1;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node2 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)Entry2;

    return PhEqualString(node1->AppUserModelId, node2->AppUserModelId, TRUE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG PhRunAsPackageNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRef(&(*(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)Entry)->AppUserModelId->sr, TRUE);
}

VOID PhRunAsPackageDestroyNode(
    _In_ PPH_RUNASPACKAGE_TREE_ROOT_NODE Node
    )
{
    PhClearReference(&Node->AppUserModelId);
    PhClearReference(&Node->DisplayName);
    PhClearReference(&Node->PackageInstallPath);
    PhClearReference(&Node->PackageFullName);
    PhClearReference(&Node->SmallLogoPath);

    PhFree(Node);
}

PPH_RUNASPACKAGE_TREE_ROOT_NODE PhRunAsPackageAddNode(
    _Inout_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_APPUSERMODELID_ENUM_ENTRY Package
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node;

    node = PhAllocate(sizeof(PH_RUNASPACKAGE_TREE_ROOT_NODE));
    memset(node, 0, sizeof(PH_RUNASPACKAGE_TREE_ROOT_NODE));

    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM;

    PhSetReference(&node->AppUserModelId, Package->AppUserModelId);
    PhSetReference(&node->DisplayName, Package->PackageDisplayName);
    PhSetReference(&node->PackageInstallPath, Package->PackageInstallPath);
    PhSetReference(&node->PackageFullName, Package->PackageFullName);
    PhSetReference(&node->SmallLogoPath, Package->SmallLogoPath);

    if (Package->SmallLogoPath && PhDoesFileExistWin32(Package->SmallLogoPath->Buffer))
    {
        HICON iconHandle = NULL;
        HBITMAP bitmap;
        LONG width;
        LONG height;

        width = PhGetSystemMetrics(SM_CXICON, Context->WindowDpi);
        height = PhGetSystemMetrics(SM_CYICON, Context->WindowDpi);

        if (bitmap = PhLoadImageFromFile(Package->SmallLogoPath->Buffer, width, height))
        {
            iconHandle = PhGdiplusConvertBitmapToIcon(bitmap, width, height, RGB(0, 0, 0));
            DeleteBitmap(bitmap);
        }

        if (iconHandle)
        {
            node->IconIndex = PhImageListAddIcon(Context->ImageListHandle, iconHandle);
            DestroyIcon(iconHandle);
        }
    }

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return node;
}

PPH_RUNASPACKAGE_TREE_ROOT_NODE PhRunAsPackageFindNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_STRING AppUserModelId
    )
{
    PH_RUNASPACKAGE_TREE_ROOT_NODE lookupNode;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE lookupNodePtr = &lookupNode;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE *node;

    lookupNode.AppUserModelId = AppUserModelId;

    node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupNodePtr
        );

    if (node)
        return *node;
    else
        return NULL;
}

VOID PhRunAsPackageRemoveNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_RUNASPACKAGE_TREE_ROOT_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PhRunAsPackageDestroyNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhRunAsPackageUpdateNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_RUNASPACKAGE_TREE_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI PhRunAsPackageTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Version)
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->DisplayName);
                break;
            case PH_RUNASPACKAGE_TREE_COLUMN_ITEM_APPID:
                getCellText->Text = PhGetStringRef(node->AppUserModelId);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_COPY, 0);
                break;
            case VK_DELETE:
                SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_CLOSE, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            RECT rect;

            rect = customDraw->CellRect;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)customDraw->Node;

            switch (customDraw->Column->Id)
            {
            case PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME:
                {
                    PH_STRINGREF text;
                    SIZE nameSize;
                    SIZE textSize;
                    LONG dpiValue;

                    dpiValue = PhGetWindowDpi(hwnd);

                    rect.left += PhGetDpi(15, dpiValue);
                    rect.top += PhGetDpi(5, dpiValue);
                    rect.right -= PhGetDpi(5, dpiValue);
                    rect.bottom -= PhGetDpi(8, dpiValue);

                    rect.left += 32;

                    // top
                    if (PhEnableThemeSupport)
                        SetTextColor(customDraw->Dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    else
                        SetTextColor(customDraw->Dc, RGB(0x0, 0x0, 0x0));

                    SelectFont(customDraw->Dc, context->TitleFontHandle);
                    text = PhIsNullOrEmptyString(node->DisplayName) ? PhGetStringRef(node->AppUserModelId) : PhGetStringRef(node->DisplayName);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &nameSize);
                    DrawText(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &rect, DT_TOP | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE);

                    // bottom
                    if (PhEnableThemeSupport)
                        SetTextColor(customDraw->Dc, RGB(0x90, 0x90, 0x90));
                    else
                        SetTextColor(customDraw->Dc, RGB(0x64, 0x64, 0x64));

                    SelectFont(customDraw->Dc, context->NormalFontHandle);
                    text = PhGetStringRef(node->AppUserModelId);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textSize);
                    DrawText(
                        customDraw->Dc,
                        text.Buffer,
                        (ULONG)text.Length / sizeof(WCHAR),
                        &rect,
                        DT_BOTTOM | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                        );

                    if (context->ImageListHandle)
                    {
                        LONG width;
                        LONG height;

                        width = PhGetSystemMetrics(SM_CXICON, context->WindowDpi);
                        height = PhGetSystemMetrics(SM_CYICON, context->WindowDpi);

                        PhImageListDrawEx(
                            context->ImageListHandle,
                            (ULONG)(ULONG_PTR)node->IconIndex,
                            customDraw->Dc,
                            customDraw->CellRect.left + 5,//((customDraw->CellRect.right - customDraw->CellRect.left) - width) / 2,
                            customDraw->CellRect.top + ((customDraw->CellRect.bottom - customDraw->CellRect.top) - height) / 2,
                            width,
                            height,
                            CLR_DEFAULT,
                            CLR_NONE,
                            ILD_TRANSPARENT,
                            ILS_NORMAL
                            );
                    }

                }
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhRunAsPackageClearTree(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhRunAsPackageDestroyNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPH_RUNASPACKAGE_TREE_ROOT_NODE PhRunAsPackageGetSelectedNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PhRunAsPackageGetSelectedNodes(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _Out_ PPH_RUNASPACKAGE_TREE_ROOT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_RUNASPACKAGE_TREE_ROOT_NODE node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN PhRunAsPackageTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)Node;
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (node->AppUserModelId && PhSearchControlMatch(context->SearchMatchHandle, &node->AppUserModelId->sr))
        return TRUE;
    if (node->DisplayName && PhSearchControlMatch(context->SearchMatchHandle, &node->DisplayName->sr))
        return TRUE;
    if (node->PackageInstallPath && PhSearchControlMatch(context->SearchMatchHandle, &node->PackageInstallPath->sr))
        return TRUE;
    if (node->PackageFullName && PhSearchControlMatch(context->SearchMatchHandle, &node->PackageFullName->sr))
        return TRUE;

    return FALSE;
}

VOID PhRunAsPackageInitializeTree(
    _Inout_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    static CONST PH_STRINGREF PhRunAsPackageLoadingText = PH_STRINGREF_INIT(L"Loading package information...");

    Context->NodeList = PhCreateList(20);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_RUNASPACKAGE_TREE_ROOT_NODE),
        PhRunAsPackageNodeHashtableEqualFunction,
        PhRunAsPackageNodeHashtableHashFunction,
        20
        );

    Context->NormalFontHandle = PhCreateCommonFont(-10, FW_NORMAL, NULL, Context->WindowDpi);
    Context->TitleFontHandle = PhCreateCommonFont(-14, FW_BOLD, NULL, Context->WindowDpi);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    TreeNew_SetCallback(Context->TreeNewHandle, PhRunAsPackageTreeNewCallback, Context);
    TreeNew_SetRowHeight(Context->TreeNewHandle, PhGetDpi(48, Context->WindowDpi));

    PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TRUE, L"Package", 80, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
    //PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_VERSION, TRUE, L"Version", 80, PH_ALIGN_CENTER, 1, DT_CENTER, 0);

    //PhRunAsPackageLoadSettingsTreeList(Context);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->TreeFilterSupport, PhRunAsPackageTreeFilterCallback, Context);

    TreeNew_SetEmptyText(Context->TreeNewHandle, &PhRunAsPackageLoadingText, 0);
    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

VOID PhRunAsPackageDeleteTree(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    PhRemoveTreeNewFilter(&Context->TreeFilterSupport, Context->TreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    if (Context->TitleFontHandle)
        DeleteFont(Context->TitleFontHandle);
    if (Context->NormalFontHandle)
        DeleteFont(Context->NormalFontHandle);

    //PhRunAsPackageSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhRunAsPackageDestroyNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

#pragma endregion

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS PhEnumPackageThreadCallback(
    _In_ PVOID Context
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;

    PPH_LIST apps = PhEnumPackageApplicationUserModelIds();
    PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, (LPARAM)apps);

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

static VOID PhRunAsPackageSetImagelist(
    _Inout_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    if (Context->ImageListHandle)
    {
        PhImageListDestroy(Context->ImageListHandle);
        Context->ImageListHandle = NULL;
    }

    Context->ImageListHandle = PhImageListCreate(
        PhGetSystemMetrics(SM_CXICON, Context->WindowDpi),
        PhGetSystemMetrics(SM_CYICON, Context->WindowDpi),
        ILC_MASK | ILC_COLOR32,
        20,
        10
        );
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhPackageWindowContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    //PPH_RUNAS_PACKAGE_CONTEXT context = (PPH_RUNAS_PACKAGE_CONTEXT)Object;
    NOTHING;
}

PPH_RUNAS_PACKAGE_CONTEXT PhCreatePackageWindowContext(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_OBJECT_TYPE PhPackageWindowContextObjectType;
    PPH_RUNAS_PACKAGE_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        PhPackageWindowContextObjectType = PhCreateObjectType(L"RunAsPackageWindowContext", 0, PhPackageWindowContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObject(sizeof(PH_RUNAS_PACKAGE_CONTEXT), PhPackageWindowContextObjectType);
    memset(context, 0, sizeof(PH_RUNAS_PACKAGE_CONTEXT));

    return context;
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpRunAsPackageSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->TreeFilterSupport);
}

INT_PTR CALLBACK PhRunAsPackageWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhCreatePackageWindowContext();

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ParentWindowHandle = (HWND)lParam;
            context->ComboBoxHandle = GetDlgItem(WindowHandle, IDC_PROGRAMCOMBO);
            context->SearchBoxHandle = GetDlgItem(WindowHandle, IDC_SEARCH);
            context->TreeNewHandle = GetDlgItem(WindowHandle, IDC_LIST);
            context->WindowDpi = PhGetWindowDpi(WindowHandle);

            PhSetApplicationWindowIconEx(WindowHandle, context->WindowDpi);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ComboBoxHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_BROWSE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SearchBoxHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_REFRESH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhValidWindowPlacementFromSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION, SETTING_RUN_AS_PACKAGE_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);

            PhpAddProgramsToComboBox(context->ComboBoxHandle);
            ComboBox_SetCurSel(context->ComboBoxHandle, 0);

            PhRunAsPackageSetImagelist(context);
            PhRunAsPackageInitializeTree(context);

            PhCreateSearchControl(
                WindowHandle,
                context->SearchBoxHandle,
                L"Search Packages",
                PhpRunAsPackageSearchControlCallback,
                context
                );

            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);

            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDCANCEL));

            EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), FALSE);
            PhReferenceObject(context);
            PhCreateThread2(PhEnumPackageThreadCallback, context);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION, SETTING_RUN_AS_PACKAGE_WINDOW_SIZE, WindowHandle);

            PhRunAsPackageDeleteTree(context);

            PhImageListDestroy(context->ImageListHandle);

            PhDestroyWindowIcon(WindowHandle);

            PhDereferenceObject(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    case WM_DPICHANGED:
        {
            context->WindowDpi = LOWORD(wParam);
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            PPH_LIST apps = (PPH_LIST)lParam;

            EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), TRUE);

            if (apps)
            {
                TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

                for (ULONG i = 0; i < apps->Count; i++)
                {
                    PhRunAsPackageAddNode(context, apps->Items[i]);
                }

                TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
                TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);

                PhDestroyEnumPackageApplicationUserModelIds(apps);
            }
            else
            {
                // Error
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDC_BROWSE:
                {
                    PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Executable files (*.exe;*.pif;*.com;*.bat;*.cmd)", L"*.exe;*.pif;*.com;*.bat;*.cmd" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog = PhCreateOpenFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

                    if (PhShowFileDialog(WindowHandle, fileDialog))
                    {
                        PPH_STRING fileName;

                        if (fileName = PhGetFileDialogFileName(fileDialog))
                        {
                            ComboBox_SetText(context->ComboBoxHandle, PhGetString(fileName));
                            PhDereferenceObject(fileName);
                        }
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDOK:
                {
                    PPH_RUNASPACKAGE_TREE_ROOT_NODE node;
                    HRESULT status;
                    PPH_STRING windowText;

                    if (node = PhRunAsPackageGetSelectedNode(context))
                    {
                        if (windowText = PhGetWindowText(context->ComboBoxHandle))
                        {
                            PPH_STRING argumentsString = NULL;
                            PPH_STRING commandString = NULL;
                            PPH_STRING directoryString = NULL;
                            PPH_STRING fullFileName = NULL;
                            PH_STRINGREF fileName;
                            PH_STRINGREF arguments;

                            if (PhIsNullOrEmptyString(windowText))
                                break;

                            if (!(commandString = PhExpandEnvironmentStrings(&windowText->sr)))
                                commandString = PhCreateString2(&windowText->sr);

                            PhParseCommandLineFuzzy(&commandString->sr, &fileName, &arguments, &fullFileName);

                            if (PhIsNullOrEmptyString(fullFileName))
                                PhMoveReference(&fullFileName, PhCreateString2(&fileName));

                            if (arguments.Length)
                            {
                                argumentsString = PhCreateString2(&arguments);
                            }

                            directoryString = PhpQueryRunFileParentDirectory(!!PhGetOwnTokenAttributes().Elevated);

                            status = PhCreateProcessDesktopPackage(
                                PhGetString(node->AppUserModelId),
                                PhGetString(fullFileName),
                                PhGetString(argumentsString),
                                PhGetString(directoryString),
                                FALSE,
                                NULL,
                                NULL
                                );

                            if (HR_SUCCESS(status))
                            {
                                PhRecentListAddCommand(&commandString->sr);

                                EndDialog(WindowHandle, IDOK);
                            }
                            else
                            {
                                PhShowStatus(WindowHandle, L"Unable to execute the command.", 0, status);
                            }

                            PhClearReference(&directoryString);
                            PhClearReference(&argumentsString);
                            PhClearReference(&fullFileName);
                            PhClearReference(&commandString);
                        }
                    }
                }
                break;
            case IDC_REFRESH:
                {
                    EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), FALSE);

                    PhRunAsPackageClearTree(context);
                    PhRunAsPackageSetImagelist(context);

                    PhReferenceObject(context);
                    PhCreateThread2(PhEnumPackageThreadCallback, context);
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

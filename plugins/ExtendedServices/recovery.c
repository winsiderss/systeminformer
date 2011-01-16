/*
 * Process Hacker Extended Services - 
 *   recovery information
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

#include <phdk.h>
#include <windowsx.h>
#include "extsrv.h"
#include "resource.h"

typedef struct _SERVICE_RECOVERY_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;

    ULONG NumberOfActions;
    BOOLEAN EnableFlagCheckBox;
    ULONG RebootAfter; // in ms
    PPH_STRING RebootMessage;

    BOOLEAN Ready;
    BOOLEAN Dirty;
} SERVICE_RECOVERY_CONTEXT, *PSERVICE_RECOVERY_CONTEXT;

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR ServiceActionPairs[] =
{
    SIP(L"Take no action", SC_ACTION_NONE),
    SIP(L"Restart the service", SC_ACTION_RESTART),
    SIP(L"Run a program", SC_ACTION_RUN_COMMAND),
    SIP(L"Restart the computer", SC_ACTION_REBOOT)
};

INT_PTR CALLBACK RestartComputerDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EspAddServiceActionStrings(
    __in HWND ComboBoxHandle
    )
{
    ULONG i;

    for (i = 0; i < sizeof(ServiceActionPairs) / sizeof(PH_KEY_VALUE_PAIR); i++)
        ComboBox_AddString(ComboBoxHandle, (PWSTR)ServiceActionPairs[i].Key);

    ComboBox_SelectString(ComboBoxHandle, -1, (PWSTR)ServiceActionPairs[0].Key);
}

SC_ACTION_TYPE EspStringToServiceAction(
    __in PWSTR String
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(ServiceActionPairs, sizeof(ServiceActionPairs), String, &integer))
        return integer;
    else
        return 0;
}

PWSTR EspServiceActionToString(
    __in SC_ACTION_TYPE ActionType
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(ServiceActionPairs, sizeof(ServiceActionPairs), ActionType, &string))
        return string;
    else
        return NULL;
}

static SC_ACTION_TYPE ComboBoxToServiceAction(
    __in HWND ComboBoxHandle
    )
{
    SC_ACTION_TYPE actionType;
    PPH_STRING string;

    string = PhGetComboBoxString(ComboBoxHandle, ComboBox_GetCurSel(ComboBoxHandle));

    if (!string)
        return SC_ACTION_NONE;

    actionType = EspStringToServiceAction(string->Buffer);
    PhDereferenceObject(string);

    return actionType;
}

static VOID ServiceActionToComboBox(
    __in HWND ComboBoxHandle,
    __in SC_ACTION_TYPE ActionType
    )
{
    PWSTR string;

    if (string = EspServiceActionToString(ActionType))
        ComboBox_SelectString(ComboBoxHandle, -1, string);
    else
        ComboBox_SelectString(ComboBoxHandle, -1, (PWSTR)ServiceActionPairs[0].Key);
}

static VOID EspFixControls(
    __in HWND hwndDlg,
    __in PSERVICE_RECOVERY_CONTEXT Context
    )
{
    SC_ACTION_TYPE action1;
    SC_ACTION_TYPE action2;
    SC_ACTION_TYPE actionS;
    BOOLEAN enableRestart;
    BOOLEAN enableReboot;
    BOOLEAN enableCommand;

    action1 = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_FIRSTFAILURE));
    action2 = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_SECONDFAILURE));
    actionS = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_SUBSEQUENTFAILURES));

    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEFORERRORSTOPS), Context->EnableFlagCheckBox);

    enableRestart = action1 == SC_ACTION_RESTART || action2 == SC_ACTION_RESTART || actionS == SC_ACTION_RESTART;
    enableReboot = action1 == SC_ACTION_REBOOT || action2 == SC_ACTION_REBOOT || actionS == SC_ACTION_REBOOT;
    enableCommand = action1 == SC_ACTION_RUN_COMMAND || action2 == SC_ACTION_RUN_COMMAND || actionS == SC_ACTION_RUN_COMMAND;

    EnableWindow(GetDlgItem(hwndDlg, IDC_RESTARTSERVICEAFTER_LABEL), enableRestart);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RESTARTSERVICEAFTER), enableRestart);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RESTARTSERVICEAFTER_MINUTES), enableRestart);

    EnableWindow(GetDlgItem(hwndDlg, IDC_RESTARTCOMPUTEROPTIONS), enableReboot);

    EnableWindow(GetDlgItem(hwndDlg, IDC_RUNPROGRAM_GROUP), enableCommand);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RUNPROGRAM_LABEL), enableCommand);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RUNPROGRAM), enableCommand);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), enableCommand);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RUNPROGRAM_INFO), enableCommand);
}

NTSTATUS EspLoadRecoveryInfo(
    __in HWND hwndDlg,
    __in PSERVICE_RECOVERY_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE serviceHandle;
    LPSERVICE_FAILURE_ACTIONS failureActions;
    SERVICE_FAILURE_ACTIONS_FLAG failureActionsFlag;
    SC_ACTION_TYPE lastType;
    ULONG returnLength;
    ULONG i;

    if (!(serviceHandle = PhOpenService(Context->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG)))
        return NTSTATUS_FROM_WIN32(GetLastError());

    if (!(failureActions = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS)))
    {
        CloseServiceHandle(serviceHandle);
        return NTSTATUS_FROM_WIN32(GetLastError());
    }

    // Failure action types

    Context->NumberOfActions = failureActions->cActions;

    if (failureActions->cActions != 0 && failureActions->cActions != 3)
        status = STATUS_SOME_NOT_MAPPED;

    // If failure actions are not defined for a particular fail count, the 
    // last failure action is used. Here we duplicate this behaviour when there 
    // are fewer than 3 failure actions.
    lastType = SC_ACTION_NONE;

    ServiceActionToComboBox(GetDlgItem(hwndDlg, IDC_FIRSTFAILURE),
        failureActions->cActions >= 1 ? (lastType = failureActions->lpsaActions[0].Type) : lastType);
    ServiceActionToComboBox(GetDlgItem(hwndDlg, IDC_SECONDFAILURE),
        failureActions->cActions >= 2 ? (lastType = failureActions->lpsaActions[1].Type) : lastType);
    ServiceActionToComboBox(GetDlgItem(hwndDlg, IDC_SUBSEQUENTFAILURES),
        failureActions->cActions >= 3 ? (lastType = failureActions->lpsaActions[2].Type) : lastType);

    // Reset fail count after

    SetDlgItemInt(hwndDlg, IDC_RESETFAILCOUNT, failureActions->dwResetPeriod / (60 * 60 * 24), FALSE); // s to days

    // Restart service after

    SetDlgItemText(hwndDlg, IDC_RESTARTSERVICEAFTER, L"1");

    for (i = 0; i < failureActions->cActions; i++)
    {
        if (failureActions->lpsaActions[i].Type == SC_ACTION_RESTART)
        {
            if (failureActions->lpsaActions[i].Delay != 0)
            {
                SetDlgItemInt(hwndDlg, IDC_RESTARTSERVICEAFTER,
                    failureActions->lpsaActions[i].Delay / (1000 * 60), FALSE); // ms to min
            }

            break;
        }
    }

    // Enable actions for stops with errors

    // This is Vista and above only.
    if (WindowsVersion >= WINDOWS_VISTA && QueryServiceConfig2(
        serviceHandle,
        SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
        (BYTE *)&failureActionsFlag,
        sizeof(SERVICE_FAILURE_ACTIONS_FLAG),
        &returnLength
        ))
    {
        Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLEFORERRORSTOPS),
            failureActionsFlag.fFailureActionsOnNonCrashFailures ? BST_CHECKED : BST_UNCHECKED);
        Context->EnableFlagCheckBox = TRUE;
    }
    else
    {
        Context->EnableFlagCheckBox = FALSE;
    }

    // Restart computer options

    Context->RebootAfter = 1 * 1000 * 60;

    for (i = 0; i < failureActions->cActions; i++)
    {
        if (failureActions->lpsaActions[i].Type == SC_ACTION_REBOOT)
        {
            if (failureActions->lpsaActions[i].Delay != 0)
                Context->RebootAfter = failureActions->lpsaActions[i].Delay;

            break;
        }
    }

    if (failureActions->lpRebootMsg && failureActions->lpRebootMsg[0] != 0)
        PhSwapReference2(&Context->RebootMessage, PhCreateString(failureActions->lpRebootMsg));
    else
        PhSwapReference2(&Context->RebootMessage, NULL);

    // Run program

    SetDlgItemText(hwndDlg, IDC_RUNPROGRAM, failureActions->lpCommand);

    PhFree(failureActions);
    CloseServiceHandle(serviceHandle);

    return status;
}

INT_PTR CALLBACK EspServiceRecoveryDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PSERVICE_RECOVERY_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_RECOVERY_CONTEXT));
        memset(context, 0, sizeof(SERVICE_RECOVERY_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_RECOVERY_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;

            context->ServiceItem = serviceItem;

            EspAddServiceActionStrings(GetDlgItem(hwndDlg, IDC_FIRSTFAILURE));
            EspAddServiceActionStrings(GetDlgItem(hwndDlg, IDC_SECONDFAILURE));
            EspAddServiceActionStrings(GetDlgItem(hwndDlg, IDC_SUBSEQUENTFAILURES));

            status = EspLoadRecoveryInfo(hwndDlg, context);

            if (status == STATUS_SOME_NOT_MAPPED)
            {
                if (context->NumberOfActions > 3)
                {
                    PhShowWarning(
                        hwndDlg,
                        L"The service has %u failure actions configured, but this program only supports editing 3. "
                        L"If you save the recovery information using this program, the additional failure actions will be lost.",
                        context->NumberOfActions
                        );
                }
            }
            else if (!NT_SUCCESS(status))
            {
                SetDlgItemText(hwndDlg, IDC_RESETFAILCOUNT, L"0");

                if (WindowsVersion >= WINDOWS_VISTA)
                {
                    context->EnableFlagCheckBox = TRUE;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEFORERRORSTOPS), TRUE);
                }

                PhShowWarning(hwndDlg, L"Unable to query service recovery information: %s",
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetNtMessage(status)))->Buffer);
            }

            EspFixControls(hwndDlg, context);

            context->Ready = TRUE;
        }
        break;
    case WM_DESTROY:
        {
            PhSwapReference2(&context->RebootMessage, NULL);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_FIRSTFAILURE:
            case IDC_SECONDFAILURE:
            case IDC_SUBSEQUENTFAILURES:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        EspFixControls(hwndDlg, context);
                    }
                }
                break;
            case IDC_RESTARTCOMPUTEROPTIONS:
                {
                    DialogBoxParam(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_RESTARTCOMP),
                        hwndDlg,
                        RestartComputerDlgProc,
                        (LPARAM)context
                        );
                }
                break;
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Executable files (*.exe;*.cmd;*.bat)", L"*.exe;*.cmd;*.bat" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_RUNPROGRAM);
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_RUNPROGRAM, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_ENABLEFORERRORSTOPS:
                {
                    context->Dirty = TRUE;
                }
                break;
            }

            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
            case CBN_SELCHANGE:
                {
                    if (context->Ready)
                        context->Dirty = TRUE;
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_KILLACTIVE:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    PPH_SERVICE_ITEM serviceItem = context->ServiceItem;
                    SC_HANDLE serviceHandle;
                    ULONG restartServiceAfter;
                    SERVICE_FAILURE_ACTIONS failureActions;
                    SC_ACTION actions[3];
                    ULONG i;
                    BOOLEAN enableRestart = FALSE;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (!context->Dirty)
                    {
                        return TRUE;
                    }

                    // Build the failure actions structure.

                    failureActions.dwResetPeriod = GetDlgItemInt(hwndDlg, IDC_RESETFAILCOUNT, NULL, FALSE) * 60 * 60 * 24;
                    failureActions.lpRebootMsg = PhGetStringOrEmpty(context->RebootMessage);
                    failureActions.lpCommand = PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_RUNPROGRAM)->Buffer;
                    failureActions.cActions = 3;
                    failureActions.lpsaActions = actions;

                    actions[0].Type = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_FIRSTFAILURE));
                    actions[1].Type = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_SECONDFAILURE));
                    actions[2].Type = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_SUBSEQUENTFAILURES));

                    restartServiceAfter = GetDlgItemInt(hwndDlg, IDC_RESTARTSERVICEAFTER, NULL, FALSE) * 1000 * 60;

                    for (i = 0; i < 3; i++)
                    {
                        switch (actions[i].Type)
                        {
                        case SC_ACTION_RESTART:
                            actions[i].Delay = restartServiceAfter;
                            enableRestart = TRUE;
                            break;
                        case SC_ACTION_REBOOT:
                            actions[i].Delay = context->RebootAfter;
                            break;
                        case SC_ACTION_RUN_COMMAND:
                            actions[i].Delay = 0;
                            break;
                        }
                    }

                    // Try to save the changes.

                    serviceHandle = PhOpenService(
                        serviceItem->Name->Buffer,
                        SERVICE_CHANGE_CONFIG | (enableRestart ? SERVICE_START : 0) // SC_ACTION_RESTART requires SERVICE_START
                        );

                    if (serviceHandle)
                    {
                        if (ChangeServiceConfig2(
                            serviceHandle,
                            SERVICE_CONFIG_FAILURE_ACTIONS,
                            &failureActions
                            ))
                        {
                            if (context->EnableFlagCheckBox)
                            {
                                SERVICE_FAILURE_ACTIONS_FLAG failureActionsFlag;

                                failureActionsFlag.fFailureActionsOnNonCrashFailures =
                                    Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLEFORERRORSTOPS)) == BST_CHECKED;

                                ChangeServiceConfig2(
                                    serviceHandle,
                                    SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
                                    &failureActionsFlag
                                    );
                            }

                            CloseServiceHandle(serviceHandle);
                        }
                        else
                        {
                            CloseServiceHandle(serviceHandle);
                            goto ErrorCase;
                        }
                    }
                    else
                    {
                        goto ErrorCase;
                    }

                    return TRUE;
ErrorCase:
                    if (PhShowMessage(
                        hwndDlg,
                        MB_ICONERROR | MB_RETRYCANCEL,
                        L"Unable to change service recovery information: %s",
                        ((PPH_STRING)PHA_DEREFERENCE(PhGetWin32Message(GetLastError())))->Buffer
                        ) == IDRETRY)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    }
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK EspServiceRecovery2DlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return FALSE;
}

static INT_PTR CALLBACK RestartComputerDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PSERVICE_RECOVERY_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PSERVICE_RECOVERY_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_RECOVERY_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetDlgItemInt(hwndDlg, IDC_RESTARTCOMPAFTER, context->RebootAfter / (1000 * 60), FALSE); // ms to min
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE), context->RebootMessage ? BST_CHECKED : BST_UNCHECKED);
            SetDlgItemText(hwndDlg, IDC_RESTARTMESSAGE, PhGetString(context->RebootMessage));

            SetFocus(GetDlgItem(hwndDlg, IDC_RESTARTCOMPAFTER));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_RESTARTCOMPAFTER), 0, -1);
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
                    context->RebootAfter = GetDlgItemInt(hwndDlg, IDC_RESTARTCOMPAFTER, NULL, FALSE) * 1000 * 60;

                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE)) == BST_CHECKED)
                        PhSwapReference2(&context->RebootMessage, PhGetWindowText(GetDlgItem(hwndDlg, IDC_RESTARTMESSAGE)));
                    else
                        PhSwapReference2(&context->RebootMessage, NULL);

                    context->Dirty = TRUE;

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_USEDEFAULTMESSAGE:
                {
                    PPH_STRING message;
                    PWSTR computerName;
                    ULONG bufferSize;
                    BOOLEAN allocated = TRUE;

                    // Get the computer name.

                    bufferSize = 64;
                    computerName = PhAllocate((bufferSize + 1) * sizeof(WCHAR));

                    if (!GetComputerName(computerName, &bufferSize))
                    {
                        PhFree(computerName);
                        computerName = PhAllocate((bufferSize + 1) * sizeof(WCHAR));

                        if (!GetComputerName(computerName, &bufferSize))
                        {
                            PhFree(computerName);
                            computerName = L"(unknown)";
                            allocated = FALSE;
                        }
                    }

                    // This message is exactly the same as the one in the Services console, 
                    // except the double spaces are replaced by single spaces.
                    message = PhFormatString(
                        L"Your computer is connected to the computer named %s. "
                        L"The %s service on %s has ended unexpectedly. "
                        L"%s will restart automatically, and then you can reestablish the connection.",
                        computerName,
                        context->ServiceItem->Name->Buffer,
                        computerName,
                        computerName
                        );
                    SetDlgItemText(hwndDlg, IDC_RESTARTMESSAGE, message->Buffer);
                    PhDereferenceObject(message);

                    if (allocated)
                        PhFree(computerName);

                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE), BST_CHECKED);
                }
                break;
            case IDC_RESTARTMESSAGE:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        // A zero length restart message disables it, so we might as well uncheck the box.
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE),
                            GetWindowTextLength(GetDlgItem(hwndDlg, IDC_RESTARTMESSAGE)) != 0 ? BST_CHECKED : BST_UNCHECKED);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

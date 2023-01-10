/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#include "extsrv.h"

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

static PH_KEY_VALUE_PAIR ServiceActionPairs[] =
{
    SIP(L"Take no action", SC_ACTION_NONE),
    SIP(L"Restart the service", SC_ACTION_RESTART),
    SIP(L"Run a program", SC_ACTION_RUN_COMMAND),
    SIP(L"Restart the computer", SC_ACTION_REBOOT)
};

INT_PTR CALLBACK RestartComputerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EspAddServiceActionStrings(
    _In_ HWND ComboBoxHandle
    )
{
    ULONG i;

    for (i = 0; i < sizeof(ServiceActionPairs) / sizeof(PH_KEY_VALUE_PAIR); i++)
        ComboBox_AddString(ComboBoxHandle, (PWSTR)ServiceActionPairs[i].Key);

    PhSelectComboBoxString(ComboBoxHandle, (PWSTR)ServiceActionPairs[0].Key, FALSE);
}

SC_ACTION_TYPE EspStringToServiceAction(
    _In_ PWSTR String
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(ServiceActionPairs, sizeof(ServiceActionPairs), String, &integer))
        return integer;
    else
        return 0;
}

PWSTR EspServiceActionToString(
    _In_ SC_ACTION_TYPE ActionType
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(ServiceActionPairs, sizeof(ServiceActionPairs), ActionType, &string))
        return string;
    else
        return NULL;
}

SC_ACTION_TYPE ComboBoxToServiceAction(
    _In_ HWND ComboBoxHandle
    )
{
    PPH_STRING string;

    string = PH_AUTO(PhGetComboBoxString(ComboBoxHandle, ComboBox_GetCurSel(ComboBoxHandle)));

    if (!string)
        return SC_ACTION_NONE;

    return EspStringToServiceAction(string->Buffer);
}

VOID ServiceActionToComboBox(
    _In_ HWND ComboBoxHandle,
    _In_ SC_ACTION_TYPE ActionType
    )
{
    PWSTR string;

    if (string = EspServiceActionToString(ActionType))
        PhSelectComboBoxString(ComboBoxHandle, string, FALSE);
    else
        PhSelectComboBoxString(ComboBoxHandle, (PWSTR)ServiceActionPairs[0].Key, FALSE);
}

VOID EspFixControls(
    _In_ HWND hwndDlg,
    _In_ PSERVICE_RECOVERY_CONTEXT Context
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
    _In_ HWND hwndDlg,
    _In_ PSERVICE_RECOVERY_CONTEXT Context
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
        return PhDosErrorToNtStatus(GetLastError());

    if (!(failureActions = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS)))
    {
        CloseServiceHandle(serviceHandle);
        return PhDosErrorToNtStatus(GetLastError());
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

    PhSetDialogItemValue(hwndDlg, IDC_RESETFAILCOUNT, failureActions->dwResetPeriod / (60 * 60 * 24), FALSE); // s to days

    // Restart service after

    PhSetDialogItemText(hwndDlg, IDC_RESTARTSERVICEAFTER, L"1");

    for (i = 0; i < failureActions->cActions; i++)
    {
        if (failureActions->lpsaActions[i].Type == SC_ACTION_RESTART)
        {
            if (failureActions->lpsaActions[i].Delay != 0)
            {
                PhSetDialogItemValue(hwndDlg, IDC_RESTARTSERVICEAFTER,
                    failureActions->lpsaActions[i].Delay / (1000 * 60), FALSE); // ms to min
            }

            break;
        }
    }

    // Enable actions for stops with errors

    if (QueryServiceConfig2(
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
        PhMoveReference(&Context->RebootMessage, PhCreateString(failureActions->lpRebootMsg));
    else
        PhClearReference(&Context->RebootMessage);

    // Run program

    PhSetDialogItemText(hwndDlg, IDC_RUNPROGRAM, failureActions->lpCommand);

    PhFree(failureActions);
    CloseServiceHandle(serviceHandle);

    return status;
}

INT_PTR CALLBACK EspServiceRecoveryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_RECOVERY_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_RECOVERY_CONTEXT));
        memset(context, 0, sizeof(SERVICE_RECOVERY_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
                        L"The service has %lu failure actions configured, but this program only supports editing 3. "
                        L"If you save the recovery information using this program, the additional failure actions will be lost.",
                        context->NumberOfActions
                        );
                }
            }
            else if (!NT_SUCCESS(status))
            {
                PPH_STRING errorMessage = PhGetNtMessage(status);

                PhSetDialogItemText(hwndDlg, IDC_RESETFAILCOUNT, L"0");

                context->EnableFlagCheckBox = TRUE;
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEFORERRORSTOPS), TRUE);

                PhShowWarning(
                    hwndDlg,
                    L"Unable to query service recovery information: %s",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    );

                PhClearReference(&errorMessage);
            }

            EspFixControls(hwndDlg, context);

            context->Ready = TRUE;

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhClearReference(&context->RebootMessage);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_FIRSTFAILURE:
            case IDC_SECONDFAILURE:
            case IDC_SUBSEQUENTFAILURES:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
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

                    fileName = PhaGetDlgItemText(hwndDlg, IDC_RUNPROGRAM);
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                        PhSetDialogItemText(hwndDlg, IDC_RUNPROGRAM, fileName->Buffer);
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

            switch (GET_WM_COMMAND_CMD(wParam, lParam))
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
                    NTSTATUS status;
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

                    failureActions.dwResetPeriod = PhGetDialogItemValue(hwndDlg, IDC_RESETFAILCOUNT) * 60 * 60 * 24;
                    failureActions.lpRebootMsg = PhGetStringOrEmpty(context->RebootMessage);
                    failureActions.lpCommand = PhaGetDlgItemText(hwndDlg, IDC_RUNPROGRAM)->Buffer;
                    failureActions.cActions = 3;
                    failureActions.lpsaActions = actions;

                    actions[0].Type = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_FIRSTFAILURE));
                    actions[1].Type = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_SECONDFAILURE));
                    actions[2].Type = ComboBoxToServiceAction(GetDlgItem(hwndDlg, IDC_SUBSEQUENTFAILURES));

                    restartServiceAfter = PhGetDialogItemValue(hwndDlg, IDC_RESTARTSERVICEAFTER) * 1000 * 60;

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
                        if (GetLastError() == ERROR_ACCESS_DENIED && !PhGetOwnTokenAttributes().Elevated)
                        {
                            // Elevate using phsvc.
                            if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                            {
                                if (NT_SUCCESS(status = PhSvcCallChangeServiceConfig2(
                                    serviceItem->Name->Buffer,
                                    SERVICE_CONFIG_FAILURE_ACTIONS,
                                    &failureActions
                                    )))
                                {
                                    if (context->EnableFlagCheckBox)
                                    {
                                        SERVICE_FAILURE_ACTIONS_FLAG failureActionsFlag;

                                        failureActionsFlag.fFailureActionsOnNonCrashFailures =
                                            Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLEFORERRORSTOPS)) == BST_CHECKED;

                                        PhSvcCallChangeServiceConfig2(
                                            serviceItem->Name->Buffer,
                                            SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
                                            &failureActionsFlag
                                            );
                                    }
                                }

                                PhUiDisconnectFromPhSvc();

                                if (!NT_SUCCESS(status))
                                {
                                    SetLastError(PhNtStatusToDosError(status));
                                    goto ErrorCase;
                                }
                            }
                            else
                            {
                                // User cancelled elevation.
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                            }
                        }
                        else
                        {
                            goto ErrorCase;
                        }
                    }

                    return TRUE;
ErrorCase:
                    {
                        PPH_STRING errorMessage = PhGetWin32Message(GetLastError());

                        if (PhShowMessage(
                            hwndDlg,
                            MB_ICONERROR | MB_RETRYCANCEL,
                            L"Unable to change service recovery information: %s",
                            PhGetStringOrDefault(errorMessage, L"Unknown error.")
                            ) == IDRETRY)
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        }

                        PhClearReference(&errorMessage);
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

INT_PTR CALLBACK RestartComputerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_RECOVERY_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PSERVICE_RECOVERY_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogItemValue(hwndDlg, IDC_RESTARTCOMPAFTER, context->RebootAfter / (1000 * 60), FALSE); // ms to min
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE), context->RebootMessage ? BST_CHECKED : BST_UNCHECKED);
            PhSetDialogItemText(hwndDlg, IDC_RESTARTMESSAGE, PhGetString(context->RebootMessage));

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_RESTARTCOMPAFTER));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_RESTARTCOMPAFTER), 0, -1);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
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
                    context->RebootAfter = PhGetDialogItemValue(hwndDlg, IDC_RESTARTCOMPAFTER) * 1000 * 60;

                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE)) == BST_CHECKED)
                        PhMoveReference(&context->RebootMessage, PhGetWindowText(GetDlgItem(hwndDlg, IDC_RESTARTMESSAGE)));
                    else
                        PhClearReference(&context->RebootMessage);

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

                    bufferSize = MAX_COMPUTERNAME_LENGTH + sizeof(UNICODE_NULL);
                    computerName = PhAllocate(bufferSize * sizeof(WCHAR));

                    if (!GetComputerName(computerName, &bufferSize))
                    {
                        PhFree(computerName);
                        computerName = PhAllocate(bufferSize * sizeof(WCHAR));

                        if (!GetComputerName(computerName, &bufferSize))
                        {
                            PhFree(computerName);
                            computerName = L"(unknown)";
                            allocated = FALSE;
                        }
                    }

                    // This message is exactly the same as the one in the Services console,
                    // except the double spaces are replaced by single spaces.
                    message = PhaFormatString(
                        L"Your computer is connected to the computer named %s. "
                        L"The %s service on %s has ended unexpectedly. "
                        L"%s will restart automatically, and then you can reestablish the connection.",
                        computerName,
                        context->ServiceItem->Name->Buffer,
                        computerName,
                        computerName
                        );
                    PhSetDialogItemText(hwndDlg, IDC_RESTARTMESSAGE, message->Buffer);

                    if (allocated)
                        PhFree(computerName);

                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE), BST_CHECKED);
                }
                break;
            case IDC_RESTARTMESSAGE:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        // A zero length restart message disables it, so we might as well uncheck the box.
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLERESTARTMESSAGE),
                            PhGetWindowTextLength(GetDlgItem(hwndDlg, IDC_RESTARTMESSAGE)) != 0 ? BST_CHECKED : BST_UNCHECKED);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
